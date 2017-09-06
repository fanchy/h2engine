
#include "net/net_stat.h"
#include "base/log.h"
using namespace ff;
#define FFNET "FFNET"

NetStat::NetStat():
    m_timer_service(NULL),
    m_max_packet_size(1000*1000)
{
}

NetStat::~NetStat()
{
    if (m_timer_service)
    {
        delete m_timer_service;
        m_timer_service = NULL;
    }
}

static void timer_close(socket_ptr_t s_)
{
    LOGWARN((FFNET, "SocketCtrlGate::timer_close"));
    s_->close();
}

static void timer_check(void* p_)
{
    ((NetStat*)p_)->handle_timer_check();
}

int NetStat::start(ArgHelper& arg_helper_)
{
    if (arg_helper_.isEnableOption("-max_packet_size"))
    {
        m_max_packet_size = ::atoi(arg_helper_.getOptionValue("-max_packet_size").c_str());
    }
    
    m_heartbeat.set_option(arg_helper_, timer_close);
    
    m_timer_service = new TimerService();
    
    m_timer_service->timerCallback(m_heartbeat.timeout(), Task(&timer_check, (void*)this));
    return 0;
}

int NetStat::start(int max_packet_size, int heartbeat_timeout)
{
    m_max_packet_size = max_packet_size;
    m_heartbeat.set_option(heartbeat_timeout, timer_close);
    m_timer_service = new TimerService();
    
    m_timer_service->timerCallback(m_heartbeat.timeout(), Task(&timer_check, (void*)this));
    return 0;
}
void NetStat::handle_timer_check()
{
    m_heartbeat.timer_check();
    if (m_timer_service)
    {
        m_timer_service->timerCallback(m_heartbeat.timeout(), Task(&timer_check, (void*)this));
    }
}

int NetStat::stop()
{
    if (m_timer_service)
    {
    	m_timer_service->stop();
        delete m_timer_service;
        m_timer_service = NULL;
    }
    return 0;
}

