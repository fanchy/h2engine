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
#include "base/func.h"

namespace ff
{

class HttpMgr
{
public:
    HttpMgr();
    ~HttpMgr();
    int start();
    int stop();

    void    request(const std::string& url_, int timeoutsec, Function<void(const std::string&)> callback);
    std::string  syncRequest(const std::string& url_, int timeoutsec = -1);
private:
    void    request_impl(const std::string& sql_, int timeoutsec, Function<void(const std::string&)> callback);
private:
    TaskQueue                                m_tq;
};


}

#endif
