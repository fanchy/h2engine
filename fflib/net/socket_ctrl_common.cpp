#include "net/socket_ctrl_common.h"
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
        m_msg_handler->getTaskQueue()->post(funcbind(&MsgHandler::handleBroken, m_msg_handler, sp_->toSharedPtr()));
    }
    else{
        m_msg_handler->handleBroken(sp_->toSharedPtr());
    }
    return 0;
}

int SocketCtrlCommon::handleRead(SocketI* sp_, const char* buff, size_t len)
{
    if (m_oWSProtocol.handleRecv(buff, len))
    {
        const vector<string>& waitToSend = m_oWSProtocol.getSendPkg();
        for (size_t i = 0; i < waitToSend.size(); ++i)
        {
            sp_->sendRaw(waitToSend[i]);
        }
        m_oWSProtocol.clearSendPkg();

        const vector<string>& recvPkg = m_oWSProtocol.getRecvPkg();
        for (size_t i = 0; i < recvPkg.size(); ++i)
        {
            const string& eachRecvPkg = recvPkg[i];
            int nHeadEndIndex = -1;
            for (int i = 0; i < (int)eachRecvPkg.size(); ++i)
            {
                if (eachRecvPkg[i] == '\n')
                {
                    nHeadEndIndex = i;
                    break;
                }
            }
            uint16_t nCmd = 0;
            if (nHeadEndIndex > 0)
            {
                string bytesHead(eachRecvPkg.c_str(), nHeadEndIndex);
                m_message.appendToBody(eachRecvPkg.c_str() + nHeadEndIndex + 1, eachRecvPkg.size() - nHeadEndIndex - 1);

                vector<string> strHeads;
                m_oWSProtocol.strSplit(bytesHead, strHeads, ",");
                vector<string> strCmds;
                m_oWSProtocol.strSplit(strHeads[0], strCmds, ":");
                if (strCmds.size() == 2 && strCmds[1].size() > 0)
                {
                    nCmd = (uint16_t)atoi(strCmds[1].c_str());
                }
            }
            else{
                m_message.appendToBody(eachRecvPkg.c_str(), eachRecvPkg.size());
            }
            m_message.getHead().cmd = nCmd;
            m_message.getHead().size = m_message.getBody().size();
            this->post_msg(sp_);
            m_message.clear();
        }
        m_oWSProtocol.clearRecvPkg();
        if (m_oWSProtocol.isClose())
        {
            sp_->close();
        }
        return 0;
    }

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
        m_msg_handler->getTaskQueue()->post(funcbind(&MsgHandler::handleMsg,
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

int SocketCtrlCommon::checkPreSend(SocketI* sp_, string& buff, int flag)
{
    if (sp_->socket() < 0)
    {
        return -1;
    }
    if (m_oWSProtocol.isWebSocketConnection())
    {
        buff = m_oWSProtocol.buildPkg(buff);
    }
    return 0;
}
int SocketCtrlCommon::get_type() const
{
    if (m_oWSProtocol.isWebSocketConnection())
    {
        return 1;
    }
    return 0;
}
