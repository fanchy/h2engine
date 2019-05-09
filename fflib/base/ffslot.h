
#ifndef _FF_SLOT_H_
#define _FF_SLOT_H_

#include <assert.h>
#include <string>
#include <map>


namespace ff
{

class FFSlot //! 封装回调函数的注册、回调
{
public:
    class CallBackArg
    {
    public:
        virtual ~CallBackArg(){}
        virtual int type() = 0;
    };    
    class FFCallBack
    {
    public:
        virtual ~FFCallBack(){}
        virtual void exe(FFSlot::CallBackArg* args_) = 0;
        virtual FFCallBack* fork() { return NULL; }
    };

public:
    virtual ~FFSlot()
    {
        clear();
    }
    FFSlot& bind(long cmd_, FFCallBack* callback_)
    {
        assert(callback_);
		this->del(cmd_);
        m_cmd2callback.insert(std::make_pair(cmd_, callback_));
        return *this;
    }
    FFSlot& bind(const std::string& cmd_, FFCallBack* callback_)
    {
        assert(callback_);
		this->del(cmd_);
        m_name2callback.insert(std::make_pair(cmd_, callback_));
        return *this;
    }
    FFCallBack* get_callback(long cmd_)
    {
        std::map<long, FFCallBack*>::iterator it = m_cmd2callback.find(cmd_);
        if (it != m_cmd2callback.end())
        {
            return it->second;
        }
        return NULL;
    }
    FFCallBack* get_callback(const std::string& cmd_)
    {
        std::map<std::string, FFCallBack*>::iterator it = m_name2callback.find(cmd_);
        if (it != m_name2callback.end())
        {
            return it->second;
        }
        return NULL;
    }
    void del(long cmd_)
    {
        std::map<long, FFCallBack*>::iterator it = m_cmd2callback.find(cmd_);
        if (it != m_cmd2callback.end())
        {
            delete it->second;
            m_cmd2callback.erase(it);
        }
    }
    void del(const std::string& cmd_)
    {
        std::map<std::string, FFCallBack*>::iterator it = m_name2callback.find(cmd_);
        if (it != m_name2callback.end())
        {
            delete it->second;
            m_name2callback.erase(it);
        }
    }
    void clear()
    {
        std::map<long, FFCallBack*>::iterator it = m_cmd2callback.begin();
        for (; it != m_cmd2callback.end(); ++it)
        {
            delete it->second;
        }
        std::map<std::string, FFCallBack*>::iterator it2 = m_name2callback.begin();
        for (; it2 != m_name2callback.end(); ++it2)
        {
            delete it2->second;
        }
        m_cmd2callback.clear();
        m_name2callback.clear();
    }
    std::map<std::string, FFCallBack*>&  get_str_cmd() { return  m_name2callback; }
private:
    std::map<long, FFCallBack*>       m_cmd2callback;
    std::map<std::string, FFCallBack*>    m_name2callback;
};


}
#endif

