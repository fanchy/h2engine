#ifndef _SOCKET_PROTOCOL_I_
#define _SOCKET_PROTOCOL_I_

#include <string>
#include <vector>
#include "net/socket.h"
#include "net/wsprotocol.h"
#include "net/msg_handler.h"
#include "base/log.h"

namespace ff {

class SocketProtocol
{
public:
    virtual ~SocketProtocol(){
        //printf("~SocketProtocol...\n");
    }
    SocketProtocol(MsgHandler* mh):m_msgHandler(mh), m_nGotSize(0){}

    std::string buildPkg(const std::string& buff)
    {
        if (m_oWSProtocol.isWebSocketConnection())
        {
            return m_oWSProtocol.buildPkg(buff);
        }
        return buff;
    }
    void handleSocketEvent(SocketObjPtr sp_, int eventType, const char* buff, size_t len)
    {
        if (eventType == IOEVENT_BROKEN){
            if (m_msgHandler->getTaskQueue()){
                m_msgHandler->getTaskQueue()->post(funcbind(&MsgHandler::handleBroken, m_msgHandler, sp_));
            }
            else{
                m_msgHandler->handleBroken(sp_);
            }
            return;
        }
        if (eventType != IOEVENT_RECV){
            return;
        }
        //LOGTRACE(("FFNET", "handleSocketEvent event=%s len=%u", eventType == IOEVENT_RECV? "IOEVENT_RECV": "IOEVENT_BROKEN", (unsigned int)len));
        if (m_oWSProtocol.handleRecv(buff, len))
        {
            if (sp_->protocolType == 0){
                sp_->protocolType = SOCKET_WS;
                sp_->funcBuildPkg = funcbind(&SocketProtocol::buildPkg, this);
                //printf("handleSocketEvent ws\n");
            }
            const std::vector<std::string>& waitToSend = m_oWSProtocol.getSendPkg();
            for (size_t i = 0; i < waitToSend.size(); ++i)
            {
                sp_->sendRaw(waitToSend[i]);
            }
            m_oWSProtocol.clearSendPkg();

            const std::vector<std::string>& recvPkg = m_oWSProtocol.getRecvPkg();
            for (size_t i = 0; i < recvPkg.size(); ++i)
            {
                const std::string& eachRecvPkg = recvPkg[i];
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
                    std::string bytesHead(eachRecvPkg.c_str(), nHeadEndIndex);
                    m_message.appendToBody(eachRecvPkg.c_str() + nHeadEndIndex + 1, eachRecvPkg.size() - nHeadEndIndex - 1);

                    std::vector<std::string> strHeads;
                    m_oWSProtocol.strSplit(bytesHead, strHeads, ",");
                    std::vector<std::string> strCmds;
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
                this->postMsg(sp_);
                m_message.clear();
            }
            m_oWSProtocol.clearRecvPkg();
            if (m_oWSProtocol.isClose())
            {
                sp_->close();
            }
            return;
        }
        if (sp_->protocolType != SOCKET_BIN){
            sp_->protocolType = SOCKET_BIN;
        }

        size_t left_len = len;
        size_t tmp      = 0;

        while (left_len > 0)
        {
            if (false == m_message.haveRecvHead(m_nGotSize))
            {
                tmp = m_message.appendHead(m_nGotSize, buff, left_len);

                m_nGotSize += tmp;
                left_len         -= tmp;
                buff             += tmp;
            }

            tmp = m_message.appendMsg(buff, left_len);
            m_nGotSize += tmp;
            left_len         -= tmp;
            buff             += tmp;

            if (m_message.getBody().size() == m_message.size())
            {
                this->postMsg(sp_);
                m_nGotSize = 0;
                m_message.clear();
            }
        }
    }
    void postMsg(SocketObjPtr& sp_)
    {
        if (m_msgHandler->getTaskQueue()){
            m_msgHandler->getTaskQueue()->post(funcbind(&MsgHandler::handleMsg,
                                                 m_msgHandler, m_message, sp_));
        }
        else{
            m_msgHandler->handleMsg(m_message, sp_);
        }
        m_message.clear();
    }
public:
    MsgHandlerPtr       m_msgHandler;
    size_t              m_nGotSize;
    Message             m_message;
    WSProtocol          m_oWSProtocol;
};

typedef SharedPtr<SocketProtocol>  SocketProtocolPtr;

}

#endif
