
#ifndef _FF_TYPE_H_
#define _FF_TYPE_H_

#include "base/osdef.h"

#ifndef _WIN32
#define MKDIR(a) ::mkdir((a),0755)
#else
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#define MKDIR(a) ::mkdir((a))
typedef int socklen_t;
typedef int ssize_t;
#endif

#include "base/singleton.h"
#include "base/lock.h"
#include "base/atomic_op.h"

#include <stdint.h>
#include <list>
#include <fstream>
#include <string>
#include <map>
#include <time.h>


#define TYPEID(X)              Singleton<ff::TypeHelper<X> >::instance().id()
#define TYPE_NAME(X)           Singleton<ff::TypeHelper<X> >::instance().getTypeName()
#define TYPE_NAME_TO_ID(name_) Singleton<ff::TypeIdGenerator>::instance().getIdByName(name_)


namespace ff
{



template<typename T>
class SafeStl
{
public:
    SafeStl()
    {
        m_cur_data = new T();
        m_history_data.push_back(m_cur_data);
    }
    virtual ~SafeStl()
    {
        for (typename std::list<T*>::iterator it = m_history_data.begin(); it != m_history_data.end(); ++it)
        {
            delete (*it);
        }
        m_history_data.clear();
    }
    const T& get_data() const { return *m_cur_data; }
    void update_data(T& data_)
    {
        T* pdata = new T(data_);
        m_history_data.push_back(pdata);
        ATOMIC_SET(&m_cur_data, pdata);
    }
private:
    std::list<T*>    m_history_data;
    T*          m_cur_data;
};


struct TypeIdGenerator
{
    TypeIdGenerator():m_id(0){}
    int allocID(const std::string& name_)
    {
        LockGuard lock(m_mutex);
        int id = ++ m_id;
        m_name2id[name_] = id;
        return id;
    }
    int getIdByName(const std::string& name_)
    {
        LockGuard lock(m_mutex);
        std::map<std::string, int>::const_iterator it = m_name2id.find(name_);
        if (it != m_name2id.end())
        {
            return it->second;
        }
        return 0;
    }
    Mutex                       m_mutex;
    int                         m_id;
    std::map<std::string, int>  m_name2id;
};

template<typename T>
struct TypeHelper
{
    TypeHelper():
        m_type_id(0)
    {
        std::string tmp  = __PRETTY_FUNCTION__;
        std::string keystr = "TypeHelper() [with T = ";
        int pos     = tmp.find(keystr);
        if (pos == std::string::npos){
            keystr = "TypeHelper() [T = ";
            pos     = tmp.find(keystr);
        }
        int pos2     = tmp.find("]", pos+1);
        m_type_name = tmp.substr(pos + keystr.size(), pos2 - pos - keystr.size());
        int npos3 = m_type_name.find_last_of("::");
        if (npos3 != std::string::npos){
            m_type_name = m_type_name.substr(npos3+1);
        }
        m_type_id   = Singleton<TypeIdGenerator>::instance().allocID(m_type_name);

        printf("__PRETTY_FUNCTION__:%s,%s,pos:%d,pos2:%d\n", __PRETTY_FUNCTION__, m_type_name.c_str(), pos, pos2);
    }
    int id() const
    {
        return m_type_id;
    }
    const std::string& getTypeName() const
    {
        return m_type_name;
    }
    int     m_type_id;
    std::string  m_type_name;
};


class ObjCounterSumI
{
public:
    ObjCounterSumI():m_ref_count(0){}
    virtual ~ ObjCounterSumI(){}
    void inc(int n = 1) { (void)__sync_add_and_fetch(&m_ref_count, n); }
    void dec(int n = 1) { __sync_sub_and_fetch(&m_ref_count, n); 	   }
    long val() const{ return m_ref_count; 						   }

    virtual const std::string& name() = 0;
protected:
    volatile long m_ref_count;
};


class ObjSumMgr
{
public:
    void reg(ObjCounterSumI* p)
    {
        LockGuard lock(m_mutex);
        m_all_counter.push_back(p);
    }

    std::map<std::string, long> getAllObjNum()
    {
        LockGuard lock(m_mutex);
        std::map<std::string, long> ret;
        for (std::list<ObjCounterSumI*>::iterator it = m_all_counter.begin(); it != m_all_counter.end(); ++it)
        {
            ret.insert(std::make_pair((*it)->name(), (*it)->val()));
        }
        return ret;
    }

    void dump(const std::string& path_)
    {
        FILE* fp = ::fopen(path_.c_str(), "a+");
        if (NULL == fp)
        {
            return;
        }

        char tmp_buff[256] = {0};
        if(getc(fp) == EOF)
        {
            int n = snprintf(tmp_buff, sizeof(tmp_buff), "time,obj,num\n");
            fwrite(tmp_buff, n, 1, fp);
        }

        std::map<std::string, long> ret = getAllObjNum();
        std::map<std::string, long>::iterator it = ret.begin();

        time_t timep   = ::time(NULL);
        struct tm *tmp = ::localtime(&timep);

        sprintf(tmp_buff, "%04d%02d%02d-%02d:%02d:%02d",
                        tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday,
                        tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
        char buff[1024] = {0};

        for (; it != ret.end(); ++it)
        {
            int n = snprintf(buff, sizeof(buff), "%s,%s,%ld\n", tmp_buff, it->first.c_str(), it->second);
            fwrite(buff, n, 1, fp);
        }

        fclose(fp);
    }
protected:
    Mutex                     m_mutex;
    std::list<ObjCounterSumI*>	m_all_counter;
};

template<typename T>
class ObjCounterSum: public ObjCounterSumI
{
public:
    ObjCounterSum()
    {
        m_name = TYPE_NAME(T);
        Singleton<ObjSumMgr>::instance().reg(this);
    }
    virtual const std::string& name() { return m_name; }
protected:
    std::string m_name;
};

template<typename T>
class ObjCounter
{
public:
    ObjCounter()
    {
        Singleton<ObjCounterSum<T> >::instance().inc(1);
    }
    ~ObjCounter()
    {
        Singleton<ObjCounterSum<T> >::instance().dec(1);
    }
};

template<typename ARG_TYPE>
struct RefTypeTraits;

template<typename ARG_TYPE>
struct RefTypeTraits
{
    typedef ARG_TYPE RealType;
};
template<typename ARG_TYPE>
struct RefTypeTraits<ARG_TYPE&>
{
    typedef ARG_TYPE RealType;
};
template<typename ARG_TYPE>
struct RefTypeTraits<const ARG_TYPE&>
{
    typedef ARG_TYPE RealType;
};
template<typename ARG_TYPE>
struct RefTypeTraits<const ARG_TYPE*>
{
    typedef const ARG_TYPE* RealType;
};

template<typename ARG_TYPE>
struct TypeInitValUtil
{
    static ARG_TYPE gInitVal;
    static ARG_TYPE& initVal() { return gInitVal; }
};
template<typename ARG_TYPE>
ARG_TYPE TypeInitValUtil<ARG_TYPE>::gInitVal;

}

typedef int64_t userid_t;
#endif
