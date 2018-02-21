//! 消息发送器

#ifndef _MSG_SENDER_H_
#define _MSG_SENDER_H_


#include "net/socket.h"
#include "net/socket_ctrl.h"
#include "net/message.h"
#include "net/codec.h"

namespace ff {

//! #include "zlib_util.h"

class MsgSender
{
public:
    static void send(SocketPtr pSocket, uint16_t cmd_, const std::string& str_)
    {
        if (pSocket)
        {
            if (0 == pSocket->get_sc()->get_type())
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
    static void send(SocketPtr pSocket, uint16_t cmd_, Codec& msg_)
    {
        if (pSocket)
        {
            if (0 == pSocket->get_sc()->get_type())
            {
                std::string body = msg_.encode_data();
                MessageHead h(cmd_);
                h.size = body.size();
                h.hton();
                std::string dest((const char*)&h, sizeof(h));
                dest += body;
    
                pSocket->asyncSend(dest);
            }
            else
            {
                char msg[128] = {0};
                snprintf(msg, sizeof(msg), "cmd:%u\n", (uint32_t)cmd_);
                std::string dest(msg);
                dest += msg_.encode_data();
                pSocket->asyncSend(dest);
            }
        }
    }
    static void send(SocketPtr pSocket, const std::string& str_)
    {
        if (pSocket)
        {
            pSocket->asyncSend(str_);
        }
    }
    static void sendToClient(SocketPtr pSocket, Codec& msg_)
    {
        if (pSocket)
        {
            std::string body = msg_.encode_data();
            MessageHead h(0);
            h.size = body.size();
            h.hton();
            std::string dest((const char*)&h, sizeof(h));
            dest += body;
            pSocket->asyncSend(body);
        }
    }
};

}
#endif
