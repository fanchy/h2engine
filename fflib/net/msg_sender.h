//! 消息发送器

#ifndef _MSG_SENDER_H_
#define _MSG_SENDER_H_

#include "net/socket.h"
#include "net/message.h"

namespace ff {

//! #include "zlib_util.h"

class MsgSender
{
public:
    static void send(SocketObjPtr& pSocket, uint16_t cmd_, const std::string& str_)
    {
        if (pSocket)
        {
            if (SOCKET_WS != pSocket->protocolType)
            {
                MessageHead h(cmd_);
                h.size = str_.size();
                h.hton();
                std::string dest((const char*)&h, sizeof(h));
                dest += str_;
                pSocket->asyncSend(dest);
            }
            else
            {
                char msg[128] = {0};
                snprintf(msg, sizeof(msg), "cmd:%u\n", (uint32_t)cmd_);
                std::string dest(msg);
                dest += str_;
                pSocket->asyncSend(dest);
            }
        }
    }
    static void send(SocketObjPtr& pSocket, const std::string& str_)
    {
        if (pSocket)
        {
            pSocket->asyncSend(str_);
        }
    }
};

}
#endif
