#ifndef _GATEWAY_SOCKET_CONTROLLER_H_
#define _GATEWAY_SOCKET_CONTROLLER_H_

#include "net/ctrlimpl/socket_ctrl_common.h"

namespace ff {

class NetStat;

class SocketCtrlGate: public SocketCtrlCommon
{
public:
    enum gateway_socket_status_e
    {
        WAIT_FIRSTPKG       = 0, //!等待第一个包
        CROSSFILE_WAIT_END  = 1, //!等待crossfile结束
        WAIT_BINMSG_PKG     = 2, //!等待二进制普通消息
        WAIT_HANDSHAKE      = 3, //!等待握手
        WAIT_WEBSOCKET_PKG  = 4, //!等待消息
        WAIT_WEBSOCKET_NOMASK_PKG = 5,//!等待无mask消息
    };

    SocketCtrlGate(MsgHandlerPtr msg_handler_, NetStat*);
    ~SocketCtrlGate();

    virtual int handleOpen(SocketI*);
    virtual int handleRead(SocketI*, const char* buff, size_t len);
    virtual int handleError(SocketI*);

    //!处理websocket 的消息
    int handleRead_websocket(SocketI*, const char* buff, size_t len);
    int handleRead_nomask_msg(SocketI*, const char* buff, size_t len);
    int handleRead_mask_msg(SocketI*, const char* buff, size_t len);

    int handle_parse_text_prot(SocketI*, const std::string& str_body_);
    virtual int checkPreSend(SocketI*, std::string& buff, int flag);

    virtual int get_type() const;
private:
    unsigned char   m_state;
    unsigned char   m_type;// 0 bin 1 text websocket
    NetStat*        m_net_stat;
    time_t          m_last_update_tm;
    std::string     m_buff;
    uint32_t        m_recv_pkg_num;
};

}
#endif
