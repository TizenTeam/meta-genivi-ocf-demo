#ifndef GATEWAY_H
#define	GATEWAY_H

#include "defines.h"
#include "properties.h"

using namespace std;

typedef void * sm_session_handle;///< A session handle
typedef void * sm_request_handle;///< A request handle

typedef enum {
    GW_REQ_TEST,
    GW_REQ_OIC_CLIENT_FIND_RESOURCE,
    GW_REQ_OIC_CLIENT_GET_RESOURCE,
    GW_REQ_OIC_CLIENT_PUT_RESOURCE,
    GW_REQ_OIC_CLIENT_OBSERVE_RESOURCE,
    GW_REQ_OIC_SERVER_GET_RESOURCE,
    GW_REQ_OIC_SERVER_PUT_RESOURCE,
    GW_REQ_SESSION_CLOSE,
    GW_REQ_MAX,
} RequestType;

typedef enum sm_error {
    GW_ERROR_NONE					            = 0,///< No errors encountered
    GW_ERROR_BAD_API_VERSION				    ,///< The library version targeted does not match the one you claim you support
    GW_ERROR_NETWORK_INIT_FAILURE          ,///< Network library initialization failed.
    GW_ERROR_DATABASE_INIT_FAILED      	        ,///< database initialization failed
    GW_ERROR_SESSION_CREATION_FAILED		    ,///< Session creation failed.
    GW_ERROR_REQUEST_CREATION_FAILED          ,///< request creation failed.
    GW_ERROR_INVALID_PARAMETER      	        ,///< Invalid function argument
    GW_ERROR_INVALID_SESSION_HANDLE	            ,///< invalid session handle
    GW_ERROR_INVALID_REQUEST_HANDLE	            ,///< invalid request handle
    GW_ERROR_INVALID_OPERATION        ,///< invalid operation
} sm_error;

// for notifying of responses
typedef struct {
    void * rHandle;
} session_data;

typedef void request_complete_cb(sm_request_handle rHandle, void *appdata);

class response;
typedef struct {
    char name[30];
    char param[500];
    char contenttype[50];
}RESTAPI;

extern RESTAPI restapi[GW_REQ_MAX + 1];

class response {
    public:
        response(RequestType type) {
            responseData = eina_strbuf_new();
            nHttpStatusCode = 0;
            strContentType = "";
            contentLength = 0;
            url = NULL;
            reqType = type;
        }

        virtual ~response() {
            if(responseData)
            {
                eina_strbuf_free(responseData);
                EINA_STRBUF_FREE(url);
                responseData = NULL;
                fprintf(stdout,"Response Data Released.");
            }
            fprintf(stdout,"--------------Finished Freeing response Memory---------------\n");
        }

        RequestType reqType;
        int nHttpStatusCode;
        string strContentType;

        Eina_Strbuf *url;
        Eina_Strbuf *responseData;
        int    contentLength;
        string name;
};


class request {

    public:
        request(RequestType type) {
            pBR = NULL;
            reqType = type;
            url = eina_strbuf_new();
            pBR = new response(reqType);
        }

        virtual ~request() {
            eina_strbuf_free(url);
        }

    public:
        Eina_Strbuf *url;
        Eina_Strbuf *data;
        RequestType reqType;
        response *pBR;
        Ecore_Con_Url *url_con;
};

typedef struct {
    sm_error status;
    char * status_string;
    request *req;
    void *appdata;
    sm_session_handle sHandle;
    request_complete_cb *pCallback;
} sm_request;

/**
 * Session
 */
typedef struct {
    pthread_t thread;
    pthread_t client;
    pthread_t server;

    int reqsock[2]; //receive requests
    int ressock[2]; //respond to requests

    int creqsock[2]; //receive requests
    int cressock[2]; //respond to requests

    int sreqsock[2]; //receive requests
    int sressock[2]; //respond to requests

    void *appdata;
} sm_session;

int getArrSize();
void create_session();
void do_request(int req, void *d);
sm_error sm_set_request_appdata(sm_request_handle rHandle, void *appdata);
void * sm_get_request_appdata(sm_request_handle rHandle);
sm_error sm_start_request(sm_request_handle rHandle);

#endif	/* GATEWAY_H */

