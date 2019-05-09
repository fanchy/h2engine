#ifndef _FF_HTTP_MGR_H_
#define _FF_HTTP_MGR_H_

#include <assert.h>
#include <string>
#include <vector>
#include <map>



#include "base/fftype.h"
#include "base/log.h"
#include "base/smart_ptr.h"
#include "db/ffdb.h"
#include "base/lock.h"
#include "base/ffslot.h"

namespace ff
{

class HttpMgr
{
public:
    class http_result_t: public FFSlot::CallBackArg
    {
    public:
        virtual int type()
        {
            return TYPEID(http_result_t);
        }
        std::string ret;
    };

    HttpMgr();
    ~HttpMgr();
    int start();
    int stop();

    void    request(const std::string& url_, int timeoutsec, FFSlot::FFCallBack* callback_);
    std::string  syncRequest(const std::string& url_, int timeoutsec = -1);
private:
    void    request_impl(const std::string& sql_, int timeoutsec, FFSlot::FFCallBack* callback_);
private:
    TaskQueue                                m_tq;
};


}

#endif
