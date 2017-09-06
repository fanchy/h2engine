#ifndef _BASE_HEARTBEAT_H_
#define _BASE_HEARTBEAT_H_

#include <stdint.h>
#include <stdlib.h>
//#include <stdio.h>
#include <map>
#include <list>


#include "base/lock.h"
#include "base/arg_helper.h"

namespace ff {

template <typename T>
class BaseHeartBeat
{
    typedef T type_t;
    struct node_info_t
    {
        node_info_t():
            timeout(0)
        {}
        type_t  value;
        time_t  timeout;
    };
    typedef std::list<node_info_t>                           soft_timeout_list_t;
    typedef std::map<type_t, typename soft_timeout_list_t::iterator>  data_map_t;
    typedef void (*timeout_FFCallBack)(type_t);
public:
    BaseHeartBeat();
    ~BaseHeartBeat();
    
    int set_option(const std::string& opt_, timeout_FFCallBack fn_);
    int set_option(const ArgHelper& arg_helper_, timeout_FFCallBack fn_);
    int set_option(int heartbeat_timeout, timeout_FFCallBack fn_);

    int add(const type_t& v_);
    int del(const type_t& v_);
    int update(const type_t& v_);

    int timer_check();

    time_t tick() const { return m_cur_tick; }
    
    int timeout() const { return (int)m_timeout; }
private:
    Mutex                     m_mutex;
    time_t                      m_timeout;
    time_t                      m_cur_tick;
    data_map_t                  m_data_map;
    soft_timeout_list_t         m_time_sort_list;
    timeout_FFCallBack          m_timeout_callback;
};

template <typename T>
BaseHeartBeat<T>::BaseHeartBeat():
    m_timeout(10*3600*24*365),//! love you 1w year
    m_timeout_callback(NULL)
{
    m_cur_tick = ::time(NULL);
}

template <typename T>
BaseHeartBeat<T>::~BaseHeartBeat()
{
    
}

template <typename T>
int BaseHeartBeat<T>::set_option(const std::string& opt_, timeout_FFCallBack fn_)
{
    ArgHelper arg_helper(opt_);

    return this->set_option(arg_helper, fn_);
}

template <typename T>
int BaseHeartBeat<T>::set_option(const ArgHelper& arg_helper, timeout_FFCallBack fn_)
{
    LockGuard lock(m_mutex);
    if (arg_helper.isEnableOption("-heartbeat_timeout"))
    {
        m_timeout = time_t(atoi(arg_helper.getOptionValue("-heartbeat_timeout").c_str()));
    }
    
    m_timeout_callback = fn_;
    return 0;
}
template <typename T>
int BaseHeartBeat<T>::set_option(int heartbeat_timeout, timeout_FFCallBack fn_)
{
    LockGuard lock(m_mutex);
    m_timeout = heartbeat_timeout;
    m_timeout_callback = fn_;
    return 0;
}
template <typename T>
int BaseHeartBeat<T>::add(const type_t& v_)
{
    node_info_t info;
    info.value   = v_;
    info.timeout = ::time(NULL) + m_timeout;

    LockGuard lock(m_mutex);

    m_time_sort_list.push_front(info);

    std::pair<typename data_map_t::iterator, bool> ret =  m_data_map.insert(make_pair(v_, m_time_sort_list.begin()));

    if (ret.second == false)
    {
        m_time_sort_list.pop_front();
        return -1;
    }

    //printf("add node this=%p m_time_sort_list=%u m_timeout=%ld\n", this, m_time_sort_list.size(), m_timeout);
    return 0;
}

template <typename T>
int BaseHeartBeat<T>::update(const type_t& v_)
{
    time_t new_tm = ::time(NULL) + m_timeout;

    LockGuard lock(m_mutex);

    typename data_map_t::iterator it = m_data_map.find(v_);
    if (m_data_map.end() == it)
    {
        return -1;
    }

    node_info_t tmp_info = *(it->second);
    tmp_info.timeout     = new_tm;
    m_time_sort_list.erase(it->second);

    m_time_sort_list.push_front(tmp_info);
    it->second = m_time_sort_list.begin();

    //printf("update node %ld\n", new_tm);
    return 0;
}

template <typename T>
int BaseHeartBeat<T>::del(const type_t& v_)
{
    LockGuard lock(m_mutex);

    typename data_map_t::iterator it = m_data_map.find(v_);
    if (m_data_map.end() == it)
    {
        return -1;
    }

    m_time_sort_list.erase(it->second);
    m_data_map.erase(it);

    //printf("del node this=%p, m_time_sort_list size=%u\n", this, m_time_sort_list.size());
    return 0;
}

template <typename T>
int BaseHeartBeat<T>::timer_check()
{
    m_cur_tick = ::time(NULL);
    LockGuard lock(m_mutex);

    while (false == m_time_sort_list.empty())
    {
        node_info_t& info = m_time_sort_list.back();
        if (m_cur_tick >= info.timeout)
        {
            (*m_timeout_callback)(info.value);
            //printf("timer_check node curTm=%ld, tm=%ld, this=%p, m_time_sort_list size=%u\n", m_cur_tick, info.timeout, this, m_time_sort_list.size());
            m_data_map.erase(info.value);
            m_time_sort_list.pop_back();
        }
        else
        {
            break;
        }
    }

    return 0;
}

};
#endif
