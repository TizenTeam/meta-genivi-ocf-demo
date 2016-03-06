#include "gateway.h"
#include <string>
#include <sqlite3.h>

extern void *client_thread(void *data);
extern void *server_thread(void *data);

RESTAPI restapi[SM_REQUEST_MAX+1] = {
    {"Test","Test",""},
    {"Cancel","Cancel",""},
    {"Destroy","Destroy",""},
    {"SessionClose","LocalRequest", ""},
    {"Max","Max",""}
};

const char *requestName[] = {
    "SM_REQUEST_TEST",
    "SM_REQUEST_CANCEL",
    "SM_REQUEST_DESTROY",
    "SM_REQUEST_SESSION_CLOSE",
    "SM_REQUEST_MAX",
};

sm_session_handle session;

const char * getRestUrl(RequestType type)
{
    switch(type)
    {
        case SM_REQUEST_TEST:
            return "http://localhost:9091/";
            //return PROPGET(CATALOGURL).c_str();
    }

}

sm_request *request_alloc_handle(sm_session_handle sHandle, RequestType type, request_complete_cb pCallback) {
    if (!sHandle) return NULL;
    sm_request *r = NULL;
    SM_CALLOC(r, 1, sm_request);
    r->req = new request(type);
    r->pCallback = pCallback;
    r->sHandle = sHandle;
    SM_INF("Allocated New Request Handle %p", r);
    return r;
}

int getArrSize()
{
    return sizeof(requestName)/sizeof(requestName[0]);
}


sm_error sm_destroy_session(sm_session_handle sHandle){
    if(!sHandle){
        return SM_ERROR_INVALID_PARAMETER;
    }
    eina_shutdown();
    eet_shutdown();
    sm_session *s = (sm_session *)(sHandle);


    //send a session close message and wait.
    sm_request r;
    r.req = new request(SM_REQUEST_SESSION_CLOSE);
    r.sHandle = sHandle;
    sm_start_request((sm_request_handle)&r);

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

    //Properties::removeInstance();
    if(s){
        SM_FREE(s);
        s=NULL;
    }

    return SM_ERROR_NONE;

}


void complete_cb(sm_request_handle rHandle, void *data){

    SM_DBG("[RESPONSE_CALLBACK]");
    int next = 0;
    switch (next) {
        case SM_REQUEST_TEST:{
                                 SM_DBG("SM_REQUEST_TEST");
                             }
                             break;

    }
    SM_DBG("-----Data Check End---------------");

    if(rHandle){
        sm_destroy_request(rHandle);
        SM_DBG("*******************************Cleaned up previous request**************************");
    }
    return;
}

sm_request_handle sm_create_test(sm_session_handle sHandle, request_complete_cb pCallback) {
    sm_request *r = request_alloc_handle(sHandle, SM_REQUEST_TEST, pCallback);
    eina_strbuf_append(r->req->url, PROPGETURL(r->req->reqType)); //main url
    return (sm_request_handle)r;
}

void do_request(int req, void *d)
{
    bool local = false;

    sm_request_handle request = NULL;

    switch(req) {

        case SM_REQUEST_TEST:{
                                 request = sm_create_test(session, complete_cb);
                             }
                             break;

        case SM_REQUEST_SESSION_CLOSE:{
                                          sm_destroy_session(session);
                                          ecore_main_loop_quit();
                                          SM_DBG("Yippppiiieeeeee!!!!");
                                      }
                                      break;
        default:
                                      ecore_main_loop_quit();
                                      break;
    }


    if(!local && request){
        sm_set_request_appdata(request, d);
        SM_DBG("Performing Request Handle ");
        sm_start_request(request);
        SM_DBG("Sent Request ");
    }
}

sm_error sm_set_request_appdata(sm_request_handle rHandle,
        void *appdata) {
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

sm_error sm_cancel_request(sm_request_handle rHandle, int call_callback_func)
{
    if (!rHandle) {
        return SM_ERROR_INVALID_PARAMETER;
    }
    sm_request *req = (sm_request *)rHandle;
    sm_request *r = request_alloc_handle(req->sHandle, SM_REQUEST_CANCEL, NULL);
    sm_set_request_appdata(r, rHandle);
    return sm_start_request(r);
}

sm_error sm_destroy_request(sm_request_handle rHandle) {
    if (!rHandle) {
        return SM_ERROR_INVALID_PARAMETER;
    }
    sm_request *req = (sm_request *)rHandle;
    sm_request *r = request_alloc_handle(req->sHandle, SM_REQUEST_DESTROY, NULL);
    sm_set_request_appdata(r, rHandle);
    return sm_start_request(r);
}

sm_error sm_start_request(sm_request_handle rHandle)
{
    if(!rHandle){
        SM_INF("Invalid Request Handle");
        return SM_ERROR_INVALID_REQUEST_HANDLE;
    }
    session_data data;
    sm_request *r = (sm_request *)rHandle;
    SM_INF("Performing Request %d", r->req->reqType);
    data.rHandle = r;
    sm_session *s = (sm_session *)(r->sHandle);
    int size = write(s->reqSock[1], &data, sizeof(session_data));
    if(size>0)
    {
        SM_INF("Write %d  bytes to %p", size, s->reqSock);
        return SM_ERROR_NONE;
    }
    SM_INF("Write %d  bytes to %p Errno = %s", size, s->reqSock, strerror(errno));
    return SM_ERROR_OPERATION_FAILED;
}

void free_hash(void *data)
{
    sm_request *req = (sm_request *)data;

    if(req->req){
        delete req->req;
        SM_DBG("Completed Storing in Cache for . %p", data);
    }

    free(req);
    SM_DBG("Finished One Session Item Cleanup.");
}

static void write_response(sm_session *session, sm_request *request)
{
    if(!session || !request){
        return;
    }

    SM_DBG("In sending response - handle= %p handle->req = %p", request, request->req);
    session_data sd;
    sd.rHandle = (sm_request_handle)request;
    int size = write(session->respSock[1],&sd, sizeof(session_data));
    if(size>0)
    {
        SM_INF("Write %d bytes to %d", size, session->respSock[1]);
        return;
    }
    SM_INF("Write %d bytes to Errno = %s", size, strerror(errno));
    return;
}

static void *session_thread(void *data)
{
    sm_session *s = (sm_session *)data;
    session_data sd;
    fd_set R,W,X;
    CURLMsg *msg;

    int M = -1;
    int rc = 0;

    SM_INF("Creating session thread %p", s);


    FD_ZERO(&R);
    FD_ZERO(&W);
    FD_ZERO(&X);
    FD_SET(s->reqSock[0], &R);
    int max_sd = s->reqSock[0];

    //wait for request, indefinitely
    while(true) {
        rc = select(max_sd+1, &R, &W, &X, NULL);

        //process the app request
        if(rc > 0) 
        {
            bool isAriaReq  = FD_ISSET(s->reqSock[0],&R);
            if(isAriaReq)
            {
                int sock = s->reqSock[0];
                SM_DBG("MAX FD IS SET max_sd = %d", max_sd);
                int bytes = read(sock, &sd, sizeof(session_data));
                if(bytes>0)
                {
                    SM_DBG("Read Request Size = %d Expected = %lu", bytes, sizeof(session_data));
                    sm_request *r = (sm_request *)sd.rHandle;
                    CURL *ptr = NULL;
                    SM_INF("Processing request type = %d", r->req->reqType);
                    if(r->req->reqType==SM_REQUEST_SESSION_CLOSE){
                        SM_DBG("Closing the session thread. %p", s);
                        pthread_exit(NULL);
                        return NULL;
                    }
                    if(r->req)
                    {
                        if(r->req->reqType==SM_REQUEST_CANCEL || r->req->reqType==SM_REQUEST_DESTROY){
                            sm_request *hdl = (sm_request *)(sm_get_request_appdata(r));
                            if(hdl && hdl->req){
                                SM_INF("Canceled Request %p", hdl->req);
                                if(hdl->req->eh) curl_multi_remove_handle(s->mh, hdl->req->eh);
                                hdl->pCallback = NULL; // don't callback.
                            }

                            if(r->req->reqType==SM_REQUEST_DESTROY)
                            {
                                delete hdl->req;
                                hdl->req = NULL;
                            }
                            SM_FREE(hdl);
                            r->status = SM_ERROR_NONE;
                            continue;
                        }

                        //int nRet = r->req->start(&ptr);
                        int nRet = 0;
                        SM_DBG("update_request result: %d New Curl Handle = %p", nRet, ptr);

                        if (nRet==0){
                            if(ptr){ //successful network request
                                curl_easy_setopt(ptr, CURLOPT_PRIVATE, r);
                                curl_multi_add_handle(s->mh, ptr);
                                SM_INF("Queued for processing %p", ptr);
                            }else{ //successful local cache load - success but curl handle is null.
                                SM_DBG("Response Loaded from Local Cache");
                                r->status = SM_ERROR_NONE;
                                //if(isAriaReq) write_response(s, (sm_request *)sd.rHandle);
                                write_response(s, (sm_request *)sd.rHandle);
                                continue;
                            }
                        }else{
                            r->status = SM_ERROR_OPERATION_FAILED;
                            write_response(s, (sm_request *)sd.rHandle);
                            continue;
                        }
                    }else{
                        SM_DBG("Request Object is Null.");
                        continue;
                    }
                }
            }
        }
        curl_multi_perform(s->mh, &M);
        SM_DBG("Running Handles %d", M);
        FD_ZERO(&R);
        FD_ZERO(&W);
        FD_ZERO(&X);
        FD_SET(s->reqSock[0], &R);
        if(CURLM_OK==curl_multi_fdset(s->mh, &R, &W, &X, &M))
        {
            max_sd = (s->reqSock[0]>M)?s->reqSock[0]:M;
            SM_DBG("MAX_FD Handles = %d", max_sd);
            int q = 0;
            while ((msg = curl_multi_info_read(s->mh, &q))){
                SM_DBG("curl_multi_info_read %d", msg->msg);
                if (msg->msg == CURLMSG_DONE) {
                    CURL *e = msg->easy_handle;
                    SM_DBG("Easy Handle %p", e);
                    void *p = NULL;
                    curl_easy_getinfo(e,CURLINFO_PRIVATE, &p);
                    sm_request *rl = (sm_request *)p;
                    if(rl)
                    {
                        SM_DBG("Identified Request %p", rl);
                        curl_slist_free_all(rl->req->headers); 
                        response *res = rl->req->pBR;
                        res->url = eina_strbuf_new();
                        eina_strbuf_append(res->url, eina_strbuf_string_get(rl->req->url));
                        eina_strbuf_append(res->url, ESSG(res->displayName));
                        rl->req->onRecvDataCompleted();
                        sm_session *s = (sm_session *)(rl->sHandle);
                        write_response(s, rl);
                        curl_multi_remove_handle(s->mh, e);
                        curl_easy_cleanup(e);
                        SM_DBG("Curl processing finished.");
                    }
                }
            }
        }

        if(rc==0)
        {
            if(s->reqSock[0]>0){
                SM_DBG("Select Timeout %d", s->reqSock[0]);
            }else{
                SM_DBG("Session closed.");
                break;
            }
        }
    }
    SM_INF("Exiting thread.");
    pthread_exit(NULL);
    return NULL;
}


sm_session_handle sm_create_session(const char *rvi) 
{
    sm_session *session = NULL;
    SM_CALLOC(session , 1, sm_session);

    session->mh = curl_multi_init();
    if(!session->mh){
        SM_DBG("Curl Multi Init Failed");
        SM_FREE(session);
        session = NULL;
        return NULL;
    }

    SM_INF("Creating session object RVI =%s", rvi);

    int ret = pipe(session->reqSock);
    ret &= pipe(session->respSock);

    if(!ret)
    {
        //create a thread to handle this session.
        ret = pthread_create(&session->thread, NULL, session_thread, session);
        ret = pthread_create(&session->client, NULL, client_thread, session);
        ret = pthread_create(&session->server, NULL, server_thread, session);

        if(ret)
        {
            SM_INF("Session Creation Failure = %d", ret);
            close(session->reqSock[0]);
            close(session->reqSock[1]);
            close(session->respSock[0]);
            close(session->respSock[1]);
            SM_FREE(session);
            return NULL;
        }

        //Properties::setSessionReference((void *)session);
        //Properties::getInstance()->save();

        SM_INF("Returning Session Object %p", session);
        return session;
    }
    SM_FREE(session);

    return NULL;
}

sm_session_handle sm_get_session(sm_request_handle rHandle){
    if(!rHandle){
        return NULL;
    }
    sm_request * pReq = (sm_request *)rHandle;
    return pReq->sHandle;
}

    static Eina_Bool
_fd_handler_cb(void *data, Ecore_Fd_Handler *handler)
{
    size_t nbytes = 0;
    int fd;

    if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR))
    {
        SM_DBG("An error has occurred. Stop watching this fd and quit.");
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
    }

    session_data sd;
    fd = ecore_main_fd_handler_fd_get(handler);
    SM_DBG("Waiting for data from: %d", fd);
    nbytes = read(fd, &sd, sizeof(session_data));
    if (nbytes <= 0)
    {
        SM_DBG("Nothing to read, exiting...");
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
    }
    else
    {
        SM_DBG("Response Read %lu bytes from input", nbytes);
        sm_request *r = ((sm_request *)(sd.rHandle));

        if(r != NULL){
            if(r->pCallback)
            {
                SM_DBG("Before Calling Callback function of request");
                r->pCallback(r, r->appdata);
            }
        }
    }
    return ECORE_CALLBACK_RENEW;
}


sm_error sm_notify_session_response(sm_session_handle sHandle)
{
    int fd = -1;
    if(sHandle)
    {
        SM_INF("Response Socket = %d", ((sm_session *)sHandle)->respSock[0]);
        fd = ((sm_session *)sHandle)->respSock[0];
    }else{
        SM_INF("Session Invalid.");
        return SM_ERROR_NETWORK_INIT_FAILURE;
    }
    if(ecore_main_fd_handler_add(fd, (Ecore_Fd_Handler_Flags)(ECORE_FD_READ | ECORE_FD_ERROR), _fd_handler_cb, NULL, NULL, NULL))
        return SM_ERROR_NONE;
    else
        return SM_ERROR_NETWORK_INIT_FAILURE;
}

sm_error sm_start_session(sm_session_handle sHandle)
{
    sm_notify_session_response(sHandle);
    return SM_ERROR_NONE;
}

void create_session()
{
    session = sm_create_session("http://localhost:9091/");
    sm_start_session(session);
    SM_DBG("session created");
}



int main(int argc, char *argv[]) {
    ecore_init();

    create_session();

    ecore_main_loop_begin();

    ecore_shutdown();
    return 0;
}
