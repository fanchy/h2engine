#ifndef _NET_STAT_H_
#define _NET_STAT_H_

#include "net/socket.h"
#include "base/arg_helper.h"
#include "base/timer_service.h"
#include "base_heartbeat.h"

#include <stdlib.h>
#include <string>


namespace ff {

class NetStat
{
public:
    NetStat();
    ~NetStat();

    int start(int max_packet_size, int heartbeat_timeout);
    int start(ArgHelper& arg_helper_);
    int stop();

    //! 最大消息包大小
    int getMaxPacketSize() const { return m_max_packet_size; }

    //! 获取心跳模块引用
    BaseHeartBeat<SocketPtr>& getHeartBeat() { return m_heartbeat;}

    //! 处理timer 回调
    void handleTimerCheck();
private:
    TimerService*                   m_timerService;
    int                             m_max_packet_size;
    BaseHeartBeat<SocketPtr>        m_heartbeat;
};

}
#endif
