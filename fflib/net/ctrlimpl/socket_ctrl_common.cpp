#include "net/ctrlimpl/socket_ctrl_common.h"
#include "net/socket.h"
#include "base/str_tool.h"
#include "base/task_queue.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
using namespace std;

using namespace ff;

SocketCtrlCommon::SocketCtrlCommon(MsgHandlerPtr msg_handler_):
    m_msg_handler(msg_handler_),
    m_have_recv_size(0)
{
}

SocketCtrlCommon::~SocketCtrlCommon()
{
}
int SocketCtrlCommon::handleOpen(SocketI* sock){
    return 0;
}
//! when socket broken(whenever), this function will be called
//! this func just callback logic layer to process this event
//! each socket broken event only can happen once
//! logic layer has responsibily to deconstruct the socket object
int SocketCtrlCommon::handleError(SocketI* sp_)
{
    if (m_msg_handler->getTaskQueue()){
        m_msg_handler->getTaskQueue()->post(TaskBinder::gen(&MsgHandler::handleBroken, m_msg_handler, sp_->toSharedPtr()));
    }
    else{
        m_msg_handler->handleBroken(sp_->toSharedPtr());
    }
    return 0;
}

int SocketCtrlCommon::handleRead(SocketI* sp_, const char* buff, size_t len)
{
    size_t left_len = len;
    size_t tmp      = 0;

    while (left_len > 0)
    {
        if (false == m_message.haveRecvHead(m_have_recv_size))
        {
            tmp = m_message.appendHead(m_have_recv_size, buff, left_len);

            m_have_recv_size += tmp;
            left_len         -= tmp;
            buff             += tmp;
        }

        tmp = m_message.appendMsg(buff, left_len);
        m_have_recv_size += tmp;
        left_len         -= tmp;
        buff             += tmp;

        if (m_message.getBody().size() == m_message.size())
        {
            this->post_msg(sp_);
            m_have_recv_size = 0;
            m_message.clear();
        }
    }

    return 0;
}

void SocketCtrlCommon::post_msg(SocketI* sp_)
{
    if (m_msg_handler->getTaskQueue()){
        m_msg_handler->getTaskQueue()->post(TaskBinder::gen(&MsgHandler::handleMsg,
                                             m_msg_handler, m_message, sp_->toSharedPtr()));
    }
    else{
        m_msg_handler->handleMsg(m_message, sp_->toSharedPtr());
    }
    m_message.clear();
}

//! 当数据全部发送完毕后，此函数会被回调
int SocketCtrlCommon::handleWriteCompleted(SocketI* sp_)
{
    return 0;
}

int SocketCtrlCommon::checkPreSend(SocketI* sp_, const string& buff, int flag)
{
    if (sp_->socket() < 0)
    {
        return -1;
    }
    return 0;
}
