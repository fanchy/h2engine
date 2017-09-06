//! 消息发送器

#ifndef _MSG_SENDER_H_
#define _MSG_SENDER_H_


#include "net/socket.h"
#include "net/socket_ctrl.h"
#include "net/message.h"
#include "net/codec.h"

namespace ff {

//! #include "zlib_util.h"

class msg_sender_t
{
public:
    static void send(socket_ptr_t socket_ptr_, uint16_t cmd_, const std::string& str_)
    {
        if (socket_ptr_)
        {
            if (0 == socket_ptr_->get_sc()->get_type())
            {
                MessageHead h(cmd_);
                h.size = str_.size();
                h.hton();
                std::string dest((const char*)&h, sizeof(h));
                dest += str_;
                socket_ptr_->asyncSend(dest);
            }
            else
            {
                char msg[128] = {0};
                snprintf(msg, sizeof(msg), "cmd:%u\n", (uint32_t)cmd_);
                std::string dest(msg);
                dest += str_;
                socket_ptr_->asyncSend(dest);
            }
        }
    }
    static void send(socket_ptr_t socket_ptr_, uint16_t cmd_, codec_i& msg_)
    {
        if (socket_ptr_)
        {
            if (0 == socket_ptr_->get_sc()->get_type())
            {
                std::string body = msg_.encode_data();
                MessageHead h(cmd_);
                h.size = body.size();
                h.hton();
                std::string dest((const char*)&h, sizeof(h));
                dest += body;
    
                socket_ptr_->asyncSend(dest);
            }
            else
            {
                char msg[128] = {0};
                snprintf(msg, sizeof(msg), "cmd:%u\n", (uint32_t)cmd_);
                std::string dest(msg);
                dest += msg_.encode_data();
                socket_ptr_->asyncSend(dest);
            }
        }
    }
    static void send(socket_ptr_t socket_ptr_, const std::string& str_)
    {
        if (socket_ptr_)
        {
            socket_ptr_->asyncSend(str_);
        }
    }
    static void send_to_client(socket_ptr_t socket_ptr_, codec_i& msg_)
    {
        if (socket_ptr_)
        {
            std::string body = msg_.encode_data();
            MessageHead h(0);
            h.size = body.size();
            h.hton();
            std::string dest((const char*)&h, sizeof(h));
            dest += body;
            socket_ptr_->asyncSend(body);
        }
    }
};

}
#endif
