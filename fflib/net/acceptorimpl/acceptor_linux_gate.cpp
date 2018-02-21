                                   
#include "net/acceptorimpl/acceptor_linux_gate.h"
#include "net/socketimpl/socket_linux.h"
#include "net/socketimpl/socket_win.h"
#include "net/ctrlimpl/socket_ctrl_gate.h"
using namespace std;
using namespace ff;

AcceptorLinuxGate::AcceptorLinuxGate(EventLoop* e_, MsgHandler* mh_, TaskQueuePool* tq_):
    AcceptorLinux(e_, mh_, tq_)
{
    
}

AcceptorLinuxGate::~AcceptorLinuxGate()
{
    
}

int AcceptorLinuxGate::open(const string& args_, int max_packet_size, int heartbeat_timeout)
{
    m_net_stat.start(max_packet_size, heartbeat_timeout);

    return AcceptorLinux::open(args_);
}

int AcceptorLinuxGate::open(ArgHelper& arg_helper)
{
	m_net_stat.start(arg_helper);
    return AcceptorLinux::open(arg_helper.getOptionValue("-gate_listen"));
}
SocketI* AcceptorLinuxGate::create_socket(SocketFd new_fd_)
{
    #ifdef _WIN32
    return new SocketWin(m_epoll, new SocketCtrlGate(m_msg_handler, &m_net_stat), new_fd_, m_tq->alloc(new_fd_));
	#else
    return new SocketLinux(m_epoll, new SocketCtrlGate(m_msg_handler, &m_net_stat), new_fd_, m_tq->alloc(new_fd_));
	#endif
}
void AcceptorLinuxGate::close()
{
    AcceptorLinux::close();
    m_net_stat.stop();
}

