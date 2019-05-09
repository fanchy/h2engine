
#include "net/net_stat.h"
#include "base/log.h"
using namespace ff;
#define FFNET "FFNET"

NetStat::NetStat():
    m_timerService(NULL),
    m_max_packet_size(1000*1000)
{
}

NetStat::~NetStat()
{
    if (m_timerService)
    {
        delete m_timerService;
        m_timerService = NULL;
    }
}

static void timer_close(SocketPtr s_)
{
    LOGWARN((FFNET, "SocketCtrlGate::timer_close"));
    s_->close();
}

static void timer_check(void* p_)
{
    ((NetStat*)p_)->handleTimerCheck();
}

int NetStat::start(ArgHelper& arg_helper_)
{
    if (arg_helper_.isEnableOption("-max_packet_size"))
    {
        m_max_packet_size = ::atoi(arg_helper_.getOptionValue("-max_packet_size").c_str());
    }
    
    m_heartbeat.set_option(arg_helper_, timer_close);
    
    m_timerService = new TimerService();
    
    m_timerService->onceTimer(m_heartbeat.timeout()*1000, Task(&timer_check, (void*)this));
    return 0;
}

int NetStat::start(int max_packet_size, int heartbeat_timeout)
{
    m_max_packet_size = max_packet_size;
    m_heartbeat.set_option(heartbeat_timeout, timer_close);
    m_timerService = new TimerService();
    
    m_timerService->onceTimer(m_heartbeat.timeout()*1000, Task(&timer_check, (void*)this));
    return 0;
}
void NetStat::handleTimerCheck()
{
    m_heartbeat.timer_check();
    if (m_timerService)
    {
        m_timerService->onceTimer(m_heartbeat.timeout()*1000, Task(&timer_check, (void*)this));
    }
}

int NetStat::stop()
{
    if (m_timerService)
    {
    	m_timerService->stop();
        delete m_timerService;
        m_timerService = NULL;
    }
    return 0;
}

