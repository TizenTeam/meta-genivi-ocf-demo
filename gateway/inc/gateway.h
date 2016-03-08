/* 
 * File:   gateway.h
 * Author: sanjeev
 *
 * Created on March 4, 2016, 6:56 PM
 */

#ifndef GATEWAY_H
#define	GATEWAY_H

#include <Ecore.h>
#include <Eina.h>
#include <Eet.h>
#include <curl/curl.h>
#include <unistd.h>
#include <string>
#include <assert.h>
#include "debugger.h"
#include "properties.h"

using namespace std;

#define ESSG(ptr) ((ptr)?eina_strbuf_string_get(ptr):"")

#define PROPGETURL(x) 	getRestUrl(x)

//SM_INF("DEALLOCATING STRBUF %p Value =%s", ptr, eina_strbuf_string_get(ptr));
#define EINA_STRBUF_FREE(ptr) \
    do {	\
        if (ptr!=NULL) {	\
            eina_strbuf_free(ptr); \
            ptr = NULL; \
        }	\
    } while (0);

//	char *tmp = strstr(buf, srch);
//	if(tmp && !strncmp(tmp, srch, strlen(tmp))) {

//c = eina_strbuf_string_get(s);

#define SET_VAL(buf,srch, i,s) {\
    if(g_str_has_suffix(buf, srch)) {\
        i = s; \
        SM_INF("Debug Allocating %p = %p Value =%s", i, s, eina_strbuf_string_get(s)); \
        continue; \
    }\
}

#define EINA_STRBUF_CLONE(a,b)   { \
    a = eina_strbuf_new(); \
    int len = eina_strbuf_length_get(b); \
    const char *val = eina_strbuf_string_get(b); \
    eina_strbuf_append_length(a, val, len); \
}
#define SM_CALLOC(ptr, number, type)	\
    do {	\
        if ((int)(number) <= 0) {	\
            ptr = NULL;	\
            assert(0); \
        } else {	\
            ptr = (type *)calloc(number , sizeof(type));	\
            assert(ptr); \
        }	\
    } while (0);

/**
 * Free memory location.
 */
#define SM_FREE(ptr)    \
    do {	\
        if (ptr!=NULL) {	\
            free(ptr); \
            ptr = NULL; \
        }	\
    } while (0);


typedef void * sm_session_handle;///< A session handle
typedef void * sm_request_handle;///< A request handle



typedef enum {
    SM_REQUEST_TEST,
    SM_REQUEST_CANCEL,
    SM_REQUEST_DESTROY,
    SM_REQUEST_SESSION_CLOSE,
    SM_REQUEST_MAX,
} RequestType;

typedef enum sm_error {
    SM_ERROR_NONE					            = 0,///< No errors encountered
    SM_ERROR_BAD_API_VERSION				    ,///< The library version targeted does not match the one you claim you support
    SM_ERROR_NETWORK_INIT_FAILURE          ,///< Network library initialization failed.
    SM_ERROR_DATABASE_INIT_FAILED      	        ,///< database initialization failed
    SM_ERROR_SESSION_CREATION_FAILED		    ,///< Session creation failed.
    SM_ERROR_SESSION_START_FAILED		    ,///< Session starting failed.
    SM_ERROR_SESSION_ALREADY_STARTED		    ,///< Session already started.
    SM_ERROR_REQUEST_CREATION_FAILED          ,///< request creation failed.
    SM_ERROR_REQUEST_CANCELED                  ,///< An error indicating thate request is canceled.
    SM_ERROR_REQUEST_IN_PROGRESS			    ,///< The request is currently in the process of fetching data.
    SM_ERROR_REQUEST_FAILED                  ,///< An request result is a failure status.
    SM_ERROR_REQUEST_FAILED_DUE_TO_NETWORK_PROBLEM ,///< An request failed due to network problem
    SM_ERROR_OPERATION_FAILED	            	,///< operation failed
    SM_ERROR_INVALID_PARAMETER      	        ,///< Invalid function argument
    SM_ERROR_INVALID_URL      	        ,///< Invalid URL
    SM_ERROR_INVALID_PATH_OR_NOT_EXIT ,///< invalid path or not exist.
    SM_ERROR_FILE_OPEN_OR_CREATION ,///< file open or creation error.
    SM_ERROR_INVALID_SESSION_HANDLE	            ,///< invalid session handle
    SM_ERROR_INVALID_REQUEST_HANDLE	            ,///< invalid request handle
    SM_ERROR_INVALID_OPERATION        ,///< invalid operation
    SM_ERROR_UNSUPPORTED_OPERATION        ,///< unsupported operation
    SM_ERROR_OUT_OF_MEMORY          			///< out of memory
} sm_error;

// for notifying of responses
typedef struct {
    void * rHandle;
} session_data;

typedef void request_complete_cb(sm_request_handle rHandle, void *appdata);


using namespace std;

class response;
typedef struct {
    char name[30];
    char param[500];
    char contentTypes[50];
}RESTAPI;

extern RESTAPI restapi[SM_REQUEST_MAX + 1];

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
                SM_INF("Response Data Released.");
            }
            SM_INF("--------------Finished Freeing response Memory---------------");
        }

        RequestType reqType;
        const static string REQ;
        const static string RES;

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
            eh = NULL;
            reqType = type;
            url = eina_strbuf_new();
            pBR = new response(reqType);
        }

        virtual ~request() {
            eina_strbuf_free(url);
        }

        virtual void onRecvDataCompleted(void);
        int start(CURL **);
        int initurl(CURL **pc);

    public:
        Eina_Strbuf *url;
        Eina_Strbuf *data;
        RequestType reqType;
        response *pBR;

        CURL *eh;
        struct curl_slist *headers;

    private:
        static size_t cb_get_response_header(void *ptr, size_t size, size_t count,
                void *userData);
        static size_t cb_get_response_data(void *ptr, size_t size, size_t count,
                void *userData);
        static int cancel_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
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
    CURLM *mh;
    pthread_t thread;
    pthread_t client;
    pthread_t server;
    int reqSock[2]; //receive requests
    int respSock[2]; //respond to requests
    void *appdata;
} sm_session;

int getArrSize();
void create_session();
void do_request(int req, void *d);
sm_error sm_set_request_appdata(sm_request_handle rHandle, void *appdata);
void * sm_get_request_appdata(sm_request_handle rHandle);
sm_error sm_cancel_request(sm_request_handle rHandle, int call_callback_func);
sm_error sm_destroy_request(sm_request_handle rHandle);
sm_error sm_start_request(sm_request_handle rHandle);

#endif	/* GATEWAY_H */

