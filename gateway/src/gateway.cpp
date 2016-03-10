#include "gateway.h"
#include "debugger.h"

extern void *client_thread(void *data);
extern void *server_thread(void *data);
sm_error sm_destroy_session(sm_session_handle sHandle);

RESTAPI restapi[GW_REQ_MAX + 1] = {
	{ "REQ_TEST",
			"{\"jsonrpc\":\"2.0\", \"id\":\"1\", \"method\": \"message\", \"params\":{\"timeout\":1459388884,\"service_name\": \"genivi.org/node/09e624e8-a44f-4bb6-839b-8befe96c0aed/hello\",\"parameters\":{\"a\":\"b\"}}}",
			"application/json"},
	{"OIC_CLIENT_FIND_RESOURCE", "ToDo", ""},
	{"OIC_CLIENT_GET_RESOURCE", "ToDo", ""},
	{"OIC_CLIENT_PUT_RESOURCE", "ToDo", ""},
	{"OIC_CLIENT_OBSERVE_RESOURCE", "ToDo", ""},
	{"OIC_SERVER_GET_RESOURCE", "ToDo", ""},
	{"OIC_SERVER_PUT_RESOURCE", "ToDo", ""},

	{"REQ_CLOSE", "CloseGateway", ""},
	{ "REQ_MAX", "Max", ""}
};

void complete_cb(sm_request_handle rHandle, void *data) {
	GW_INF("[RESPONSE_CALLBACK]");
	int next = 0;
	switch (next) {
		case GW_REQ_TEST: {
			GW_INF("GW_REQ_TEST");
		}
		break;
	}
	GW_INF("-----Data Check End---------------");
	return;
}

static Eina_Bool _fd_handler_cb(void *data, Ecore_Fd_Handler *handler) {
	size_t nbytes = 0;
	int fd;

	if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR)) {
		GW_INF("An error has occurred. Stop watching this fd and quit.");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	}

	session_data sd;
	fd = ecore_main_fd_handler_fd_get(handler);
	GW_INF("Waiting for data from: %d", fd);
	nbytes = read(fd, &sd, sizeof(session_data));
	if (nbytes <= 0) {
		GW_INF("Nothing to read, exiting...");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	} else {
		GW_INF("Response Read %lu bytes from input", nbytes);
		sm_request *r = ((sm_request *) (sd.rHandle));

		if (r != NULL) {
			if (r->pCallback) {
				GW_INF("Before Calling Callback function of request");
				r->pCallback(r, r->appdata);
			}
		}
	}
	return ECORE_CALLBACK_RENEW;
}

sm_error sm_notify_session_response(sm_session_handle sHandle) {
	int fd = -1;
	if (sHandle) {
		GW_INF("Response Socket = %d", ((sm_session * )sHandle)->ressock[0]);
		fd = ((sm_session *) sHandle)->ressock[0];
	} else {
		GW_INF("Session Invalid.");
		return GW_ERROR_NETWORK_INIT_FAILURE;
	}
	if (ecore_main_fd_handler_add(fd,
			(Ecore_Fd_Handler_Flags)(ECORE_FD_READ | ECORE_FD_ERROR),
			_fd_handler_cb, NULL, NULL, NULL))
		return GW_ERROR_NONE;
	else
		return GW_ERROR_NETWORK_INIT_FAILURE;
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
			GW_INF("%s %s", spaceOffset, (gchar* )list_data);
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
    GW_INF("Data for Request %s", restapi[r->req->reqType].name);

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
    GW_INF("Completed Request %s", restapi[r->req->reqType].name);

    GError *error = NULL;
	JsonParser *jsonParser = json_parser_new ();
	if(jsonParser)
	{
        if (!json_parser_load_from_data (jsonParser, ESSG(r->req->pBR->responseData), r->req->pBR->contentLength, &error))
        {
            GW_INF("ParseNestedObjects Error");g_error_free (error);
            return EINA_TRUE;
        }
	}
	else
	{
		GW_ERR("Unable to create parser");
		return EINA_TRUE;
	}

	if (error)
	{
		GW_ERR("Error->code=%d", error->code);
		GW_ERR("Error->message=%s", error->message);
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
	GW_INF("In sending response - handle= %p handle->req = %p", request,request->req);
	session_data sd;
	sd.rHandle = (sm_request_handle) request;
	int size = write(session->ressock[1], &sd, sizeof(session_data));
	if (size > 0) {
		GW_INF("Write %d bytes to %d", size, session->ressock[1]);
		return;
	}
	GW_INF("Write %d bytes to Errno = %s", size, strerror(errno));
	return;
}

static void *session_thread(void *data) {
	sm_session *s = (sm_session *) data;
	session_data sd;
	fd_set R, W, X;
	int rc = 0;

	GW_INF("Creating session thread %p", s);
	FD_ZERO(&R);
	FD_ZERO(&W);
	FD_ZERO(&X);
	FD_SET(s->reqsock[0], &R);
	int max_sd = s->reqsock[0];

	 //wait for request, indefinitely
	while (true) {
		rc = select(max_sd + 1, &R, &W, &X, NULL);

		//process the app request
		if (rc > 0) {
			bool isAppReq = FD_ISSET(s->reqsock[0], &R);
			if (isAppReq) {
				int sock = s->reqsock[0];
				GW_INF("MAX FD IS SET max_sd = %d", max_sd);
				int bytes = read(sock, &sd, sizeof(session_data));
				GW_INF("Read Request Size = %d....waiting", bytes);

				if (bytes > 0) {
					sm_request *r = (sm_request *) sd.rHandle;
					GW_INF("Processing request type = %d", r->req->reqType);
					if (r->req->reqType == GW_REQ_SESSION_CLOSE) {
						GW_INF("Closing the session thread. %p", s);
						pthread_exit (NULL);
						return NULL;
					}
					if (r->req) {
						write_response(s, (sm_request *) sd.rHandle);
						continue;
					} else {
						GW_INF("Request Object is Null.");
						continue;
					}
				}
			}
		}
		if (rc == 0) {
			if (s->reqsock[0] > 0) {
				GW_INF("Select Timeout %d", s->reqsock[0]);
			} else {
				GW_INF("Session closed.");
				break;
			}
		}
	}
	GW_INF("Exiting thread.");
	pthread_exit (NULL);
	return NULL;
}

const char * getRestUrl(RequestType type) {
	switch (type) {
		case GW_REQ_TEST:{
			GW_INF("URL = %s", PROPGET(RVIURL).c_str());
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
	GW_CALLOC(r, 1, sm_request);
	r->req = new request(type);
	r->pCallback = pCallback;
	r->sHandle = sHandle;
	GW_INF("Allocated New Request Handle %p", r);
	return r;
}

sm_error sm_destroy_session(sm_session_handle sHandle) {
	if (!sHandle) {
		return GW_ERROR_INVALID_PARAMETER;
	}
	eina_shutdown();
	sm_session *s = (sm_session *) (sHandle);
	sm_request r;
	r.req = new request(GW_REQ_SESSION_CLOSE);
	r.sHandle = sHandle;
	sm_start_request((sm_request_handle) &r);
	GW_INF("Waiting on pthread_join");
	int ret = pthread_join(s->thread, NULL);
	close(s->reqsock[0]);
	close(s->reqsock[1]);
	close(s->ressock[0]);
	close(s->ressock[1]);
	s->reqsock[0] = -2;
	s->reqsock[1] = -2;
	s->ressock[0] = -2;
	s->ressock[1] = -2;
	GW_INF("Output of pthread_join = %d", ret);
	properties::removeInstance();
	if (s) {
		GW_FREE(s);
		s = NULL;
	}
	return GW_ERROR_NONE;
}

sm_request_handle sm_create_test(sm_session_handle sHandle,
		request_complete_cb pCallback) {
	sm_request *r = request_alloc_handle(sHandle, GW_REQ_TEST, pCallback);
	eina_strbuf_append(r->req->url, getRestUrl(r->req->reqType)); //main url
	return (sm_request_handle) r;
}

sm_error sm_set_request_appdata(sm_request_handle rHandle, void *appdata) {
	if (rHandle) {
		((sm_request *) rHandle)->appdata = appdata;
		return GW_ERROR_NONE;
	}
	return GW_ERROR_INVALID_PARAMETER;
}

void * sm_get_request_appdata(sm_request_handle rHandle) {
	if (rHandle) {
		return ((sm_request *) rHandle)->appdata;
	}
	return NULL;
}

sm_error sm_start_request(sm_request_handle rHandle) {
	if (!rHandle) {
		GW_INF("Invalid Request Handle");
		return GW_ERROR_INVALID_REQUEST_HANDLE;
	}
	sm_request *r = (sm_request *) rHandle;
	GW_INF("Performing Request %d", r->req->reqType);
	Eina_Bool ret;
	r->req->url_con = ecore_con_url_custom_new("http://localhost:9001/",
			"POST");
	if (!r->req->url_con) {
		printf("error when creating ecore con url object.\n");
		return GW_ERROR_REQUEST_CREATION_FAILED;
	}

	ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _url_data_cb, r);
	ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _url_complete_cb, r);
	ret = ecore_con_url_post(r->req->url_con, restapi[0].param,
			strlen(restapi[0].param), restapi[0].contenttype);
	if (!ret) {
		printf("could not realize request.\n");
	}
	GW_INF("Request Completed %d", ret);
	return GW_ERROR_NONE;
}

sm_session_handle sm_create_session(const char *rvi) {
	sm_session *session = NULL;
	GW_CALLOC(session, 1, sm_session);
	properties::getInstance()->load();

	GW_INF("Creating session object RVI =%s", rvi);

	int ret = pipe(session->reqsock);
	ret &= pipe(session->ressock);

	// for oic client
	ret &= pipe(session->creqsock);
	ret &= pipe(session->cressock);

	// for oic server
	ret &= pipe(session->sreqsock);
	ret &= pipe(session->sressock);

	if (!ret) {
		//create a thread to handle this session.
		//ret = pthread_create(&session->thread, NULL, session_thread, session);
		ret =  pthread_create(&session->client, NULL, client_thread, session);
		ret &= pthread_create(&session->server, NULL, server_thread, session);

		if (ret) {
			GW_INF("Session Creation Failure = %d", ret);
			close(session->reqsock[0]);
			close(session->reqsock[1]);
			close(session->ressock[0]);
			close(session->ressock[1]);

			close(session->creqsock[0]);
			close(session->creqsock[1]);
			close(session->cressock[0]);
			close(session->cressock[1]);

			close(session->sreqsock[0]);
			close(session->sreqsock[1]);
			close(session->sressock[0]);
			close(session->sressock[1]);

			GW_FREE(session);
			return NULL;
		}

		properties::setSessionReference((void *) session);
		properties::getInstance()->save();

		GW_INF("Returning Session Object %p", session);
		return session;
	}
	GW_FREE(session);

	return NULL;
}
sm_error sm_start_session(sm_session_handle sHandle) {
	sm_notify_session_response(sHandle);
	return GW_ERROR_NONE;
}

int main(int argc, const char *argv[]) {
	ecore_init();
	ecore_con_init();
	ecore_con_url_init();
	if (!ecore_con_url_pipeline_get())
		ecore_con_url_pipeline_set (EINA_TRUE);

	sm_session_handle session = sm_create_session("http://localhost:9091/");
	sm_start_session(session);
	GW_INF("session created");

	sm_request_handle h = sm_create_test(session, complete_cb);
	sm_start_request(h);

	ecore_main_loop_begin();
	ecore_con_url_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();
	//sm_destroy_session(session);

	return 0;
}
