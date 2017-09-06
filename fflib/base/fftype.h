
#ifndef _FF_TYPE_H_
#define _FF_TYPE_H_

#ifndef _WIN32
#define MKDIR(a) ::mkdir((a),0755)
#define SOCKET_TYPE int
#else
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#define MKDIR(a) ::mkdir((a)) 
#define SOCKET_TYPE SOCKET
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
#define TYPE_NAME(X)           Singleton<ff::TypeHelper<X> >::instance().get_type_name()
#define TYPE_NAME_TO_ID(name_) Singleton<ff::TypeIdGenerator>::instance().get_id_by_name(name_)


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
    int get_id_by_name(const std::string& name_)
    {
        LockGuard lock(m_mutex);
        std::map<std::string, int>::const_iterator it = m_name2id.find(name_);
        if (it != m_name2id.end())
        {
            return it->second;
        }
        return 0;
    }
    Mutex          m_mutex;
    int              m_id;
    std::map<std::string, int> m_name2id;
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
        int pos2     = tmp.find("T", pos+1);
        m_type_name = tmp.substr(pos + keystr.size(), pos2 - pos - keystr.size());
        m_type_id   = Singleton<TypeIdGenerator>::instance().allocID(m_type_name);
        
        //printf("__PRETTY_FUNCTION__:%s,%s,pos:%d,pos2:%d\n", __PRETTY_FUNCTION__, m_type_name.c_str(), pos, pos2);
    }
    int id() const
    {
        return m_type_id;
    }
    const std::string& get_type_name() const
    {
        return m_type_name;
    }
    int     m_type_id;
    std::string  m_type_name;
};

class type_i
{
public:
    virtual ~ type_i(){}
    virtual int get_type_id() const { return -1; }
    virtual const std::string& get_type_name() const {static std::string foo; return foo; }
    
    virtual void   decode(const std::string& data_) {}
    virtual std::string encode()                    { return "";} 
    
    template<typename T>
    T* cast()
    {
        if (get_type_id() == TYPEID(T))
        {
            return (T*)this;
        }
        return NULL;
    }
};


template<typename SUPERT, typename T>
class AutoType: public SUPERT
{
public:
    virtual ~ AutoType(){}
    virtual int get_type_id() const
    {
        return TYPEID(T);
    }
    virtual const std::string& get_type_name() const
    {
        return TYPE_NAME(T);
    }
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

    std::map<std::string, long> get_all_obj_num()
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

        std::map<std::string, long> ret = get_all_obj_num();
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
class FFattr
{
public:
    typedef uint64_t number_t;

public:
    virtual ~FFattr(){}

    number_t get_num(number_t key_)
    {
        std::map<number_t, number_t>::iterator it = m_num2num.find(key_);
        if (it != m_num2num.end())
        {
            return it->second;
        }
        return 0;
    }
    const std::string& get_string(number_t key_)
    {
        std::map<number_t, std::string>::iterator it = m_num2string.find(key_);
        if (it != m_num2string.end())
        {
            return it->second;
        }
        static std::string dumy_str;
        return dumy_str;
    }
    number_t get_num(const std::string& key_)
    {
        std::map<std::string, number_t>::iterator it = m_string2num.find(key_);
        if (it != m_string2num.end())
        {
            return it->second;
        }
        return 0;
    }
    const std::string& get_string(const std::string& key_)
    {
        std::map<std::string, std::string>::iterator it = m_string2string.find(key_);
        if (it != m_string2string.end())
        {
            return it->second;
        }
        static std::string dumy_str;
        return dumy_str;
    }

    void set_num(number_t key_, number_t val_)
    {
        m_num2num[key_] = val_;
    }
    void set_string(number_t key_, const std::string& val_)
    {
        m_num2string[key_] = val_;
    }
    void set_num(const std::string& key_, number_t val_)
    {
        m_string2num[key_] = val_;
    }
    void set_string(const std::string& key_, const std::string& val_)
    {
        m_string2string[key_] = val_;
    }
    
    bool isExist_num(number_t key_)
    {
        return m_num2num.find(key_) != m_num2num.end();
    }
    bool isExist_string(number_t key_)
    {
        return m_num2string.find(key_) != m_num2string.end();
    }
    bool isExist_num(const std::string& key_)
    {
        return m_string2num.find(key_) != m_string2num.end();
    }
    bool isExist_string(const std::string& key_)
    {
        return m_string2string.find(key_) != m_string2string.end();
    }
    
    std::map<number_t, number_t>& get_num2num()       { return m_num2num;       }
    std::map<number_t, std::string>&   get_num2string()    { return m_num2string;    }
    std::map<std::string, number_t>&   get_string2num()    { return m_string2num;    }
    std::map<std::string, std::string>&     get_string2string() { return m_string2string; }

private:
    std::map<number_t, number_t>    m_num2num;
    std::map<number_t, std::string>      m_num2string;
    std::map<std::string, number_t>      m_string2num;
    std::map<std::string, std::string>        m_string2string;
};


template<typename T>
struct FFTraits
{
    typedef T value_t;
};

template<typename T>
struct FFTraits<const T&>
{
    typedef T value_t;
};

template<typename T>
struct FFTraits<T&>
{
    typedef T value_t;
};

}

typedef int64_t userid_t;
#endif
