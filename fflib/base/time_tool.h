
#ifndef _FF_TIME_TOOL_H_
#define _FF_TIME_TOOL_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <string>

//! 获取特定时间的unix 时间戳
struct TimeTool
{
    static long today_at_zero()//!今日零点时间戳
    {
        time_t now   = ::time(NULL);
        tm    tm_val = *::localtime(&now);
        long ret = (long)now - tm_val.tm_hour*3600 - tm_val.tm_min*60 - tm_val.tm_sec;
        return ret;
    }
    static long next_month()//!下个月开始时间戳
    {
        time_t now   = ::time(NULL);
        tm    tm_val = *::localtime(&now);

        //! 计算这个月有多少天
        static int help_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        int month_day_num = help_month[tm_val.tm_mon];
        if (1 == tm_val.tm_mon)//! 检查2月闰月
        {
            if ((tm_val.tm_year + 1900) % 4 == 0) ++ month_day_num;
        }

        long ret = (long)now + (month_day_num - tm_val.tm_mday)*86400 + (23 - tm_val.tm_hour)*3600 +
                   (59 - tm_val.tm_min)*60 + (60 - tm_val.tm_sec);
        return ret;
    }
    static std::string formattm(time_t curtm){
        struct tm tm_val = *localtime(&curtm);

        std::string ret;
        char buff[512];
        ::snprintf(buff, sizeof(buff), "%d-%02d-%02d %02d:%02d:%02d",
                    tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
                    tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
        ret = buff;
        return ret;
    }
    static time_t str2time(const std::string& strTime){
        tm tm_;
        ::strptime(strTime.c_str(), "%Y-%m-%d %H:%M:%S", &tm_);
        tm_.tm_isdst = -1;  
        return ::mktime(&tm_);
    }
};

#endif

