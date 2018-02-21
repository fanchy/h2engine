#ifndef _GATEWAY_ACCEPTOR_H_
#define _GATEWAY_ACCEPTOR_H_

#include "net/acceptorimpl/acceptor_linux.h"
#include "net/ctrlimpl/socket_ctrl_gate.h"
#include "net/net_stat.h"
#include "base/arg_helper.h"

namespace ff {

class AcceptorLinuxGate: public AcceptorLinux
{
public:
    AcceptorLinuxGate(EventLoop*, MsgHandler*, TaskQueuePool* tq_);
    ~AcceptorLinuxGate();

    int open(const std::string& address_, int max_packet_size = 1000*1000*10, int heartbeat_timeout = 3600*24);
    int open(ArgHelper& arg_helper);
	void close();
protected:
    virtual SocketI* create_socket(SocketFd new_fd_);

private:
    //! data field
    NetStat   m_net_stat;
};

}
#endif
