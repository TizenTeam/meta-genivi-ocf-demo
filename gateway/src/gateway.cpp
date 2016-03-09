#include <stdio.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include "gateway.h"
#include <string>
#include <sqlite3.h>
#include <glib.h>
#include <json-glib/json-glib.h>

extern void *client_thread(void *data);
extern void *server_thread(void *data);
sm_error sm_destroy_session(sm_session_handle sHandle);

RESTAPI restapi[SM_REQUEST_MAX + 1] = {
	{ "REQ_TEST",
			"{\"jsonrpc\":\"2.0\", \"id\":\"1\", \"method\": \"message\", \"params\":{\"timeout\":1459388884,\"service_name\": \"genivi.org/node/09e624e8-a44f-4bb6-839b-8befe96c0aed/hello\",\"parameters\":{\"a\":\"b\"}}}",
			"application/json"},
	{"REQ_CANCEL",  "Cancel", ""},
	{"REQ_DESTROY", "Destroy", ""},
	{"REQ_CLOSE", "CloseGateway", ""},
	{ "REQ_MAX", "Max", ""}
};

void complete_cb(sm_request_handle rHandle, void *data) {

	SM_INF("[RESPONSE_CALLBACK]");
	int next = 0;
	switch (next) {
		case SM_REQUEST_TEST: {
			SM_INF("SM_REQUEST_TEST");
		}
		break;
	}
	SM_INF("-----Data Check End---------------");
	sm_request *r = (sm_request *) rHandle;
	sm_session_handle s = r->sHandle;
	sm_destroy_request(rHandle);
	return;
}

static Eina_Bool _fd_handler_cb(void *data, Ecore_Fd_Handler *handler) {
	size_t nbytes = 0;
	int fd;

	if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR)) {
		SM_INF("An error has occurred. Stop watching this fd and quit.");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	}

	session_data sd;
	fd = ecore_main_fd_handler_fd_get(handler);
	SM_INF("Waiting for data from: %d", fd);
	nbytes = read(fd, &sd, sizeof(session_data));
	if (nbytes <= 0) {
		SM_INF("Nothing to read, exiting...");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	} else {
		SM_INF("Response Read %lu bytes from input", nbytes);
		sm_request *r = ((sm_request *) (sd.rHandle));

		if (r != NULL) {
			if (r->pCallback) {
				SM_INF("Before Calling Callback function of request");
				r->pCallback(r, r->appdata);
			}
		}
	}
	return ECORE_CALLBACK_RENEW;
}

sm_error sm_notify_session_response(sm_session_handle sHandle) {
	int fd = -1;
	if (sHandle) {
		SM_INF("Response Socket = %d", ((sm_session * )sHandle)->respSock[0]);
		fd = ((sm_session *) sHandle)->respSock[0];
	} else {
		SM_INF("Session Invalid.");
		return SM_ERROR_NETWORK_INIT_FAILURE;
	}
	if (ecore_main_fd_handler_add(fd,
			(Ecore_Fd_Handler_Flags)(ECORE_FD_READ | ECORE_FD_ERROR),
			_fd_handler_cb, NULL, NULL, NULL))
		return SM_ERROR_NONE;
	else
		return SM_ERROR_NETWORK_INIT_FAILURE;
}

gchar*
ExtractValue(JsonNode *node) {
	gchar * value;
	GType valueType = json_node_get_value_type(node);
	switch (valueType) {
	case G_TYPE_STRING:
		value = json_node_dup_string(node);
		break;

	case G_TYPE_INT:
		value = malloc(20 * sizeof(gchar));
		sprintf(value, "%d", (int) json_node_get_int(node));
		break;

	case G_TYPE_DOUBLE:
		value = malloc(40 * sizeof(gchar));
		sprintf(value, "%f", json_node_get_double(node));
		break;

	case G_TYPE_BOOLEAN:
		value = malloc(6 * sizeof(gchar));
		if (json_node_get_boolean(node) == TRUE) {
			sprintf(value, "%s", "true");
		} else {
			sprintf(value, "%s", "false");
		}
		break;

	default:
		value = malloc(22 * sizeof(gchar));
		sprintf(value, "%s", "Value of unknown type");
		break;
	}
	return value;
}

Eina_List*
ParseJsonEntity(JsonNode *root, bool isArrayParsing) {
	Eina_List *jsonList = NULL;
	if (JSON_NODE_TYPE(root) == JSON_NODE_ARRAY) {
		JsonArray* array = json_node_get_array(root);
		guint arraySize = json_array_get_length(array);
		JsonNode *arrayElem;
		for (guint i = 0; i < arraySize; i++) {
			Eina_List *jsonArrElemList = NULL;
			arrayElem = json_array_get_element(array, i);
			jsonArrElemList = ParseJsonEntity(arrayElem, true);
			jsonList = eina_list_merge(jsonList, jsonArrElemList);
			//add array member (with pair key/value) to the end of the list
		}
	} else if (JSON_NODE_TYPE(root) == JSON_NODE_VALUE) {

		jsonList = eina_list_append(jsonList, ExtractValue(root));
		if (isArrayParsing) {
			jsonList = eina_list_append(jsonList, NULL);
			//add member of list with data=NULL (for arrays without keys as separator)
		}
	} else if (JSON_NODE_TYPE(root) == JSON_NODE_OBJECT) {
		JsonObject *object = NULL;

		object = json_node_get_object(root);

		if (object != NULL) {
			GList * keysList = NULL;
			GList * valList = NULL;
			guint size;

			size = json_object_get_size(object);

			keysList = json_object_get_members(object);
			valList = json_object_get_values(object);

			JsonNode *tmpNode;
			gchar *keyName;

			for (int j = 0; j < size; j++) {
				if (keysList) {
					keyName = malloc(
							(strlen(keysList->data) + 1) * sizeof(gchar));
					sprintf(keyName, "%s", (gchar*) (keysList->data));
					jsonList = eina_list_append(jsonList, keyName);
				}
				if (valList) {
					tmpNode = (JsonNode*) (valList->data);
				}

				Eina_List *l = ParseJsonEntity(tmpNode, false);
				jsonList = eina_list_append(jsonList, l);

				keysList = g_list_next(keysList);
				valList = g_list_next(valList);

			}
			if (keysList != NULL)
				g_list_free(keysList);
			if (valList != NULL)
				g_list_free(valList);
		}
	}
	return jsonList;
}

void printParsedList(Eina_List * jsonlist, int level) {
	Eina_List *l;
	void *list_data;
	int i = 0;
	EINA_LIST_FOREACH(jsonlist, l, list_data)
	{
		if (i % 2) {
			int level_ = level + 1;
			if (list_data != NULL) {
				printParsedList(list_data, level_);
			}
		} else {
			gchar* spaceOffset = NULL;
			spaceOffset = malloc((2 * level + 1) * sizeof(gchar));
			for (int i = 0; i < level; i++) {
				spaceOffset[2 * i] = '-';
				spaceOffset[2 * i + 1] = '>';
			}
			spaceOffset[2 * level] = '\0';
			SM_INF("%s %s", spaceOffset, (gchar* )list_data);
			if (spaceOffset != NULL)
				free(spaceOffset);
			if (list_data != NULL)
				free(list_data);
			//for all keys and values added into Eina_List memory should be freed manually
		}
		i++;
	}
	eina_list_free(jsonlist);
}

static Eina_Bool
_url_data_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event_info)
{
	Ecore_Con_Event_Url_Data *url_data = event_info;
    sm_request *r = (sm_request *)data;
    SM_INF("Data for Request %s", restapi[r->req->reqType].name);

    int i;
    for (i = 0; i < url_data->size; i++)
		printf("%c", url_data->data[i]);

    if (url_data) {
        eina_strbuf_append_length(r->req->pBR->responseData, (char *)url_data->data, url_data->size);
        r->req->pBR->contentLength += url_data->size;
    }
	return EINA_TRUE;
}


static Eina_Bool
_url_complete_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event_info)
{
	Ecore_Con_Event_Url_Complete *url_complete = event_info;
	const Eina_List *headers, *l;
	char *str;
	headers = ecore_con_url_response_headers_get(url_complete->url_con);
	EINA_LIST_FOREACH(headers, l, str)
		printf("header: %s\n", str);

    sm_request *r = (sm_request *)data;
    SM_INF("Completed Request %s", restapi[r->req->reqType].name);

    GError *error = NULL;
	JsonParser *jsonParser = json_parser_new ();
	if(jsonParser)
	{
        if (!json_parser_load_from_data (jsonParser, ESSG(r->req->pBR->responseData), r->req->pBR->contentLength, &error))
        {
            SM_INF("ParseNestedObjects Error");g_error_free (error);
            return EINA_TRUE;
        }
	}
	else
	{
		SM_ERR("Unable to create parser");
		return EINA_TRUE;
	}

	if (error)
	{
		SM_ERR("Error->code=%d", error->code);
		SM_ERR("Error->message=%s", error->message);
	}
	else
	{
		JsonNode *root;
		root =json_parser_get_root (jsonParser);
		if (NULL != root)
		{
			int level = 1;
			Eina_List *jsonList = NULL;
			jsonList = ParseJsonEntity(root,false);
			printParsedList(jsonList,level);
		}
	}
	return EINA_TRUE;
}

static void write_response(sm_session *session, sm_request *request) {
	if (!session || !request) {
		return;
	}
	SM_INF("In sending response - handle= %p handle->req = %p", request,request->req);
	session_data sd;
	sd.rHandle = (sm_request_handle) request;
	int size = write(session->respSock[1], &sd, sizeof(session_data));
	if (size > 0) {
		SM_INF("Write %d bytes to %d", size, session->respSock[1]);
		return;
	}
	SM_INF("Write %d bytes to Errno = %s", size, strerror(errno));
	return;
}

static void *session_thread(void *data) {
	sm_session *s = (sm_session *) data;
	session_data sd;
	fd_set R, W, X;
	int M = -1;
	int rc = 0;

	SM_INF("Creating session thread %p", s);
	FD_ZERO(&R);
	FD_ZERO(&W);
	FD_ZERO(&X);
	FD_SET(s->reqSock[0], &R);
	int max_sd = s->reqSock[0];

	 //wait for request, indefinitely
	while (true) {
		rc = select(max_sd + 1, &R, &W, &X, NULL);

		//process the app request
		if (rc > 0) {
			bool isAppReq = FD_ISSET(s->reqSock[0], &R);
			if (isAppReq) {
				int sock = s->reqSock[0];
				SM_INF("MAX FD IS SET max_sd = %d", max_sd);
				//int bytes = read(sock, &sd, sizeof(session_data));
				int bytes = read(sock, &sd, 8);
				SM_INF("Read Request Size = %d....waiting", bytes);

				if (bytes > 0) {
					sm_request *r = (sm_request *) sd.rHandle;
					CURL *ptr = NULL;
					SM_INF("Processing request type = %d", r->req->reqType);
					if (r->req->reqType == SM_REQUEST_SESSION_CLOSE) {
						SM_INF("Closing the session thread. %p", s);
						pthread_exit (NULL);
						return NULL;
					}
					if (r->req) {
						if (r->req->reqType == SM_REQUEST_CANCEL
								|| r->req->reqType == SM_REQUEST_DESTROY) {
							sm_request *hdl = (sm_request *) (sm_get_request_appdata(r));
							if (hdl && hdl->req) {
								SM_INF("Canceled Request %p", hdl->req);
								hdl->pCallback = NULL; // don't callback.
							}
							if (r->req->reqType == SM_REQUEST_DESTROY) {
								delete hdl->req;
								hdl->req = NULL;
							}
							SM_FREE(hdl);
							r->status = SM_ERROR_NONE;
							continue;
						}
						r->status = SM_ERROR_OPERATION_FAILED;
						write_response(s, (sm_request *) sd.rHandle);
						continue;
					} else {
						SM_INF("Request Object is Null.");
						continue;
					}
				}
			}
		}
		if (rc == 0) {
			if (s->reqSock[0] > 0) {
				SM_INF("Select Timeout %d", s->reqSock[0]);
			} else {
				SM_INF("Session closed.");
				break;
			}
		}
	}
	SM_INF("Exiting thread.");
	pthread_exit (NULL);
	return NULL;
}

const char * getRestUrl(RequestType type) {
	switch (type) {
		case SM_REQUEST_TEST:{
			SM_INF("URL = %s", PROPGET(RVIURL).c_str());
			return PROPGET(RVIURL).c_str();
		}
	}
	return NULL;
}

sm_request *request_alloc_handle(sm_session_handle sHandle, RequestType type,
		request_complete_cb pCallback) {
	if (!sHandle)
		return NULL;
	sm_request *r = NULL;
	SM_CALLOC(r, 1, sm_request);
	r->req = new request(type);
	r->pCallback = pCallback;
	r->sHandle = sHandle;
	SM_INF("Allocated New Request Handle %p", r);
	return r;
}

sm_error sm_destroy_session(sm_session_handle sHandle) {
	if (!sHandle) {
		return SM_ERROR_INVALID_PARAMETER;
	}
	eina_shutdown();
	sm_session *s = (sm_session *) (sHandle);
	sm_request r;
	r.req = new request(SM_REQUEST_SESSION_CLOSE);
	r.sHandle = sHandle;
	sm_start_request((sm_request_handle) &r);
	SM_INF("Waiting on pthread_join");
	int ret = pthread_join(s->thread, NULL);
	close(s->reqSock[0]);
	close(s->reqSock[1]);
	close(s->respSock[0]);
	close(s->respSock[1]);
	s->reqSock[0] = -2;
	s->reqSock[1] = -2;
	s->respSock[0] = -2;
	s->respSock[1] = -2;
	curl_multi_cleanup(s->mh);
	SM_INF("Output of pthread_join = %d", ret);
	properties::removeInstance();
	if (s) {
		SM_FREE(s);
		s = NULL;
	}
	return SM_ERROR_NONE;
}

sm_request_handle sm_create_test(sm_session_handle sHandle,
		request_complete_cb pCallback) {
	sm_request *r = request_alloc_handle(sHandle, SM_REQUEST_TEST, pCallback);
	eina_strbuf_append(r->req->url, getRestUrl(r->req->reqType)); //main url
	return (sm_request_handle) r;
}

sm_error sm_set_request_appdata(sm_request_handle rHandle, void *appdata) {
	if (rHandle) {
		((sm_request *) rHandle)->appdata = appdata;
		return SM_ERROR_NONE;
	}
	return SM_ERROR_INVALID_PARAMETER;
}

void * sm_get_request_appdata(sm_request_handle rHandle) {
	if (rHandle) {
		return ((sm_request *) rHandle)->appdata;
	}
	return NULL;
}

sm_error sm_cancel_request(sm_request_handle rHandle, int call_callback_func) {
	if (!rHandle) {
		return SM_ERROR_INVALID_PARAMETER;
	}
	sm_request *req = (sm_request *) rHandle;
	sm_request *r = request_alloc_handle(req->sHandle, SM_REQUEST_CANCEL, NULL);
	sm_set_request_appdata(r, rHandle);
	return sm_start_request(r);
}

sm_error sm_destroy_request(sm_request_handle rHandle) {
	if (!rHandle) {
		return SM_ERROR_INVALID_PARAMETER;
	}
	sm_request *req = (sm_request *) rHandle;
	sm_request *r = request_alloc_handle(req->sHandle, SM_REQUEST_DESTROY,
			NULL);
	sm_set_request_appdata(r, rHandle);
	return sm_start_request(r);
}

sm_error sm_start_request(sm_request_handle rHandle) {
	if (!rHandle) {
		SM_INF("Invalid Request Handle");
		return SM_ERROR_INVALID_REQUEST_HANDLE;
	}
	sm_request *r = (sm_request *) rHandle;
	SM_INF("Performing Request %d", r->req->reqType);
	Eina_Bool ret;
	r->req->url_con = ecore_con_url_custom_new("http://localhost:9001/",
			"POST");
	if (!r->req->url_con) {
		printf("error when creating ecore con url object.\n");
		return SM_ERROR_REQUEST_CREATION_FAILED;
	}

	ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _url_data_cb, r);
	ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _url_complete_cb, r);
	ret = ecore_con_url_post(r->req->url_con, restapi[0].param,
			strlen(restapi[0].param), restapi[0].contenttype);
	if (!ret) {
		printf("could not realize request.\n");
	}
	SM_INF("Request Completed %d", ret);
	return SM_ERROR_NONE;
}

sm_session_handle sm_create_session(const char *rvi) {
	sm_session *session = NULL;
	SM_CALLOC(session, 1, sm_session);
	properties::getInstance()->load();

	SM_INF("Creating session object RVI =%s", rvi);

	int ret = pipe(session->reqSock);
	ret &= pipe(session->respSock);

	if (!ret) {
		//create a thread to handle this session.
		ret = pthread_create(&session->thread, NULL, session_thread, session);
		//ret = pthread_create(&session->client, NULL, client_thread, session);
		//ret = pthread_create(&session->server, NULL, server_thread, session);

		if (ret) {
			SM_INF("Session Creation Failure = %d", ret);
			close(session->reqSock[0]);
			close(session->reqSock[1]);
			close(session->respSock[0]);
			close(session->respSock[1]);
			SM_FREE(session);
			return NULL;
		}

		properties::setSessionReference((void *) session);
		properties::getInstance()->save();

		SM_INF("Returning Session Object %p", session);
		return session;
	}
	SM_FREE(session);

	return NULL;
}
sm_error sm_start_session(sm_session_handle sHandle) {
	sm_notify_session_response(sHandle);
	return SM_ERROR_NONE;
}

int main(int argc, const char *argv[]) {
	ecore_init();
	ecore_con_init();
	ecore_con_url_init();
	if (!ecore_con_url_pipeline_get())
		ecore_con_url_pipeline_set (EINA_TRUE);

	sm_session_handle session = sm_create_session("http://localhost:9091/");
	sm_start_session(session);
	SM_INF("session created");

	sm_request_handle h = sm_create_test(session, complete_cb);
	sm_start_request(h);

	ecore_main_loop_begin();
	ecore_con_url_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();
	//sm_destroy_session(session);

	return 0;
}
