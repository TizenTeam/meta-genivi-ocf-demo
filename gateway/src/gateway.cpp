#include "gateway.h"

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

void print_choices()
{
	fprintf(stdout, "RVI OIC Gateway Option\n");
	fprintf(stdout, "----------------------\n");
	fprintf(stdout, "1. Test API\n");
	fprintf(stdout, "2. Shutdown Gateway\n");
}


void complete_cb(sm_request_handle rHandle, void *data) {
	fprintf(stdout,"[RESPONSE_CALLBACK]");
	int next = 0;
	switch (next) {
		case GW_REQ_TEST: {
			fprintf(stdout,"GW_REQ_TEST");
		}
		break;
	}
	fprintf(stdout,"-----Data Check End---------------");
	return;
}

static Eina_Bool _fd_handler_cb(void *data, Ecore_Fd_Handler *handler) {
	size_t nbytes = 0;
	int fd;

	if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR)) {
		fprintf(stdout,"An error has occurred. Stop watching this fd and quit.");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	}

	session_data sd;
	fd = ecore_main_fd_handler_fd_get(handler);
	fprintf(stdout,"Waiting for data from: %d\n", fd);
	nbytes = read(fd, &sd, sizeof(session_data));
	if (nbytes <= 0) {
		fprintf(stdout,"Nothing to read, exiting...");
		ecore_main_loop_quit();
		return ECORE_CALLBACK_CANCEL;
	} else {
		fprintf(stdout,"Response Read %lu bytes from input\n", nbytes);
		sm_request *r = ((sm_request *) (sd.rHandle));

		if (r != NULL) {
			if (r->pCallback) {
				fprintf(stdout,"Before Calling Callback function of request");
				r->pCallback(r, r->appdata);
			}
		}
	}
	return ECORE_CALLBACK_RENEW;
}

sm_error sm_notify_session_response(sm_session_handle sHandle) {
	int fd = -1;
	if (sHandle) {
		fprintf(stdout,"Response Socket = %d\n", ((sm_session * )sHandle)->ressock[0]);
		fd = ((sm_session *) sHandle)->ressock[0];
	} else {
		fprintf(stdout,"Session Invalid.");
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
		fprintf(stdout, "DataType = %d String \n", valueType);

		break;

	case G_TYPE_INT64:
	case G_TYPE_INT:
		value = malloc(20 * sizeof(gchar));
		sprintf(value, "%d\n", (int) json_node_get_int(node));
		fprintf(stdout, "DataType = %d INT / INT64 \n", valueType);
		break;

	case G_TYPE_DOUBLE:
		value = malloc(40 * sizeof(gchar));
		sprintf(value, "%f\n", json_node_get_double(node));
		fprintf(stdout, "DataType = %d DOUBLE \n", valueType);
		break;

	case G_TYPE_BOOLEAN:
		value = malloc(6 * sizeof(gchar));
		if (json_node_get_boolean(node) == TRUE) {
			sprintf(value, "%s\n", "true");
		} else {
			sprintf(value, "%s\n", "false");
		}
		fprintf(stdout, "DataType = %d BOOLEAN \n", valueType);
		break;

	default:
		fprintf(stdout, "DataType = %d UNKNOWN \n", valueType);
		value = malloc(22 * sizeof(gchar));
		sprintf(value, "%s\n", "Value of unknown type");
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
					sprintf(keyName, "%s\n", (gchar*) (keysList->data));
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
			fprintf(stdout,"%s %s\n", spaceOffset, (gchar* )list_data);
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
    fprintf(stdout,"Data for Request %s\n", restapi[r->req->reqType].name);
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
		printf("header: %s\n\n", str);

    sm_request *r = (sm_request *)data;
    fprintf(stdout,"Completed Request %s\n", restapi[r->req->reqType].name);

    GError *error = NULL;
	JsonParser *jsonParser = json_parser_new ();
	if(jsonParser)
	{
        if (!json_parser_load_from_data (jsonParser, ESSG(r->req->pBR->responseData), r->req->pBR->contentLength, &error))
        {
            fprintf(stdout,"ParseNestedObjects Error");g_error_free (error);
            return EINA_TRUE;
        }
	}
	else
	{
		fprintf(stdout,"Unable to create parser");
		return EINA_TRUE;
	}

	if (error)
	{
		fprintf(stdout,"Error->code=%d\n", error->code);
		fprintf(stdout,"Error->message=%s\n", error->message);
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
	print_choices();
	return EINA_TRUE;
}

static void write_response(sm_session *session, sm_request *request) {
	if (!session || !request) {
		return;
	}
	fprintf(stdout,"In sending response - handle= %p handle->req = %p\n", request,request->req);
	session_data sd;
	sd.rHandle = (sm_request_handle) request;
	int size = write(session->ressock[1], &sd, sizeof(session_data));
	if (size > 0) {
		fprintf(stdout,"Write %d bytes to %d\n", size, session->ressock[1]);
		return;
	}
	fprintf(stdout,"Write %d bytes to Errno = %s\n", size, strerror(errno));
	return;
}

static void *session_thread(void *data) {
	sm_session *s = (sm_session *) data;
	session_data sd;
	fd_set R, W, X;
	int rc = 0;

	fprintf(stdout,"Creating session thread %p\n", s);
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
				fprintf(stdout,"MAX FD IS SET max_sd = %d\n", max_sd);
				int bytes = read(sock, &sd, sizeof(session_data));
				fprintf(stdout,"Read Request Size = %d....waiting\n", bytes);

				if (bytes > 0) {
					sm_request *r = (sm_request *) sd.rHandle;
					fprintf(stdout,"Processing request type = %d\n", r->req->reqType);



					if (r->req->reqType == GW_REQ_SESSION_CLOSE) {
						fprintf(stdout,"Closing the session thread. %p\n", s);
						pthread_exit (NULL);
						return NULL;
					}
					if (r->req) {
						write_response(s, (sm_request *) sd.rHandle);
						continue;
					} else {
						fprintf(stdout,"Request Object is Null.");
						continue;
					}
				}
			}
		}
		if (rc == 0) {
			if (s->reqsock[0] > 0) {
				fprintf(stdout,"Select Timeout %d\n", s->reqsock[0]);
			} else {
				fprintf(stdout,"Session closed.");
				break;
			}
		}
	}
	fprintf(stdout,"Exiting thread.");
	pthread_exit (NULL);
	return NULL;
}

const char * getRestUrl(RequestType type) {
	switch (type) {
		case GW_REQ_TEST:{
			fprintf(stdout,"URL = %s\n", PROPGET(RVIURL).c_str());
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
	fprintf(stdout,"Allocated New Request Handle %p\n", r);
	return r;
}


void close_sockets(sm_session_handle sHandle) {
	sm_session *s = (sm_session *) (sHandle);
	close(s->reqsock[0]);
	close(s->reqsock[1]);
	close(s->ressock[0]);
	close(s->ressock[1]);

	close(s->creqsock[0]);
	close(s->creqsock[1]);
	close(s->cressock[0]);
	close(s->cressock[1]);

	close(s->sreqsock[0]);
	close(s->sreqsock[1]);
	close(s->sressock[0]);
	close(s->sressock[1]);
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
	fprintf(stdout,"Waiting on pthread_join");
	int ret = pthread_join(s->thread, NULL);
	close_sockets(s);
	fprintf(stdout,"Output of pthread_join = %d\n", ret);
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
		fprintf(stdout,"Invalid Request Handle");
		return GW_ERROR_INVALID_REQUEST_HANDLE;
	}
	sm_request *r = (sm_request *) rHandle;
	fprintf(stdout,"Performing Request %d\n", r->req->reqType);
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
	fprintf(stdout,"Request Completed %d\n", ret);
	return GW_ERROR_NONE;
}

static Eina_Bool _cli_handler_cb(void *data, Ecore_Fd_Handler *handler) {
	   char buf[10];
	   size_t nbytes;
	   int fd;
	   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR))
	     {
	        printf("An error has occurred. Stop watching this fd and quit.\n");
	        ecore_main_loop_quit();
	        return ECORE_CALLBACK_CANCEL;
	     }
	   fd = ecore_main_fd_handler_fd_get(handler);
	   nbytes = read(fd, buf, sizeof(buf));
	   if (nbytes == 0)
	     {
	        printf("Nothing to read, exiting...\n");
	        ecore_main_loop_quit();
	        return ECORE_CALLBACK_CANCEL;
	     }
	   buf[nbytes - 1] = '\0';
	   printf("Read %zd bytes from input: \"%s\"\n", nbytes - 1, buf);
	int input = -1;
	sm_session *session = data;
	input = atoi(buf);
	fprintf(stdout,"Size read = %d Executing CLI Command : %d\n", nbytes, input);

	switch(input-1)
	{
		case GW_REQ_TEST:
		{
			sm_request_handle h = sm_create_test(session, complete_cb);
			sm_start_request(h);
		}
		break;
	}

	return ECORE_CALLBACK_RENEW;
}

int main(int argc, const char *argv[]) {
	int ret = 0;
	ecore_init();
	ecore_con_init();
	ecore_con_url_init();
	if (!ecore_con_url_pipeline_get())
		ecore_con_url_pipeline_set (EINA_TRUE);

	properties::getInstance()->load();

	sm_session *session = NULL;
	GW_CALLOC(session, 1, sm_session);

	ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ,
						_cli_handler_cb, session, NULL, NULL);


	ret  = pipe(session->reqsock);
	ret &= pipe(session->ressock);
	// for oic client
	ret &= pipe(session->creqsock);
	ret &= pipe(session->cressock);
	// for oic server
	ret &= pipe(session->sreqsock);
	ret &= pipe(session->sressock);

	if (!ret) {
		//ret =  pthread_create(&session->client, NULL, client_thread, session);
		//ret &= pthread_create(&session->server, NULL, server_thread, session);
		if (ret) {
			fprintf(stdout,"Session Creation Failure = %d\n", ret);
			return -1;
		}
		fprintf(stdout,"Returning Session Object %p\n", session);
	}
	ecore_main_fd_handler_add(session->ressock[0],
				(Ecore_Fd_Handler_Flags)(ECORE_FD_READ | ECORE_FD_ERROR),
				_fd_handler_cb, NULL, NULL, NULL);

	print_choices();

	ecore_main_loop_begin();
	ecore_con_url_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();
	//sm_destroy_session(session);
	return 0;
}
