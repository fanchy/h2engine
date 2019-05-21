#ifdef FF_ENABLE_CURL
    #include <curl/curl.h>
#endif

#include "server/http_mgr.h"
#include "base/perf_monitor.h"

using namespace std;
using namespace ff;

#define HHTP_MGR "HHTP_MGR"

HttpMgr::HttpMgr()
{
}
HttpMgr::~HttpMgr()
{
}

int HttpMgr::start()
{
    #ifdef FF_ENABLE_CURL

    #endif
    return 0;
}
int HttpMgr::stop()
{
    #ifdef FF_ENABLE_CURL
    LOGINFO((HHTP_MGR, "HttpMgr::stop begin..."));

    m_tq.close();

    LOGINFO((HHTP_MGR, "HttpMgr::stop end"));
    #endif
    return 0;
}

void HttpMgr::request(const string& url_, int timeoutsec, FFSlot::FFCallBack* callback_)
{
    m_tq.post(funcbind(&HttpMgr::request_impl, this, url_, timeoutsec, callback_));
}

void HttpMgr::request_impl(const string& sql_, int timeoutsec, FFSlot::FFCallBack* callback_)
{
    http_result_t result;
    result.ret = syncRequest(sql_, timeoutsec);
    if (callback_)
    {
        callback_->exe(&result);
        delete callback_;
    }
}
#ifdef FF_ENABLE_CURL
static size_t write_data_func(void *ptr, size_t size, size_t nmemb, void *stream)
{
    string* ret = (string*)stream;
    int written = size * nmemb;
    if (ret)
    {
        ret->append((const char*)ptr, written);
    }

    return written;
}
#endif
string HttpMgr::syncRequest(const string& url_, int timeoutsec)
{
    LOGTRACE((HHTP_MGR, "HttpMgr::syncRequest begin...", url_));
    AUTO_PERF();
    string ret;
    #ifdef FF_ENABLE_CURL
    CURL* curl = curl_easy_init();
    if (!curl){
        return ret;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_func);
    if (timeoutsec > 0){
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutsec);
    }
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
    {
        ret = "#HttpError:";
        ret += curl_easy_strerror(res);
    }
    curl_easy_cleanup(curl);
    #endif
    return ret;
}

