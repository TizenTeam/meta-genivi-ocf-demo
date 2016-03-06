#include "gateway.h"

static struct Network_Initializer {
    Network_Initializer() {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~Network_Initializer() {
        curl_global_cleanup();
    }
} g_initializer;


int request::cancel_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    //SM_INF("In Progress Function");
    return 0;
}

int request::initRequestUrl(CURL **pc)
{
    *pc = curl_easy_init();
    if (*pc == NULL) {
        SM_INF("curl_easy_init failed");
        return -1;
    }
    eh = *pc;

    SM_INF("curl_easy_init success" );
    eina_strbuf_replace_all(url, "%20", "+");
    eina_strbuf_replace_all(url, " ", "+");
    SM_INF("url: %s", eina_strbuf_string_get(url));
    return 0;
}

int request::start(CURL **pc) {
    int nRet = 0;
    initRequest();
    nRet = initRequestUrl(pc);

    if(0!=nRet){
        SM_ERR("Error initializing base url.");
        return nRet;
    }

    SM_INF("Checking Cache Validity %s", eina_strbuf_string_get(url));

    if(!pBR)
    {
        SM_INF("Response Object Not Set. %s", ESSG(displayName));
        return -1;
    }

    SM_INF("Cache Invalid. Fetching from Network %s", eina_strbuf_string_get(url));
    curl_easy_setopt(*pc, CURLOPT_URL, eina_strbuf_string_get(url));
    curl_easy_setopt(*pc, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(*pc, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(*pc, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(*pc, CURLOPT_HEADER, 0);
    curl_easy_setopt(*pc, CURLOPT_WRITEFUNCTION, cb_get_response_data);
    curl_easy_setopt(*pc, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(*pc, CURLOPT_WRITEHEADER, this);
    curl_easy_setopt(*pc, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(*pc, CURLOPT_PROGRESSFUNCTION, cancel_cb);
    curl_easy_setopt(*pc, CURLOPT_PROGRESSDATA, this);
    return 0;

}

size_t request::cb_get_response_data(void *ptr, size_t size, size_t count, void *userData){
    SM_INF("Response Callback Received...");
    if (userData) {
        request * pRequest = static_cast<request *>(userData);
        if (ptr) {
            size_t nDataSize = size * count;
            if(pRequest && pRequest->pBR){
                eina_strbuf_append_length(pRequest->pBR->responseData, (char *)ptr, nDataSize);
            }
        }
    }
    static int cnt;
    SM_INF("cb_get_response_data called: %d (%d bytes)", cnt, (int) (size * count));
    cnt++;
    return size * count;
}

void request::initRequest(){
}

void request::onRecvDataCompleted() {
    switch(pBR->nHttpStatusCode){
        case 200:{

                     eina_strbuf_free(pBR->responseData);
                     pBR->responseData = NULL;
                     SM_INF("Response Data has been freed.");
                 }
                 break;

        default:
                 SM_INF("Request Failed With Code= %d", pBR->nHttpStatusCode);
                 break;
    }
}
