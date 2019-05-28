#ifndef _SOCKET_PROTOCOL_I_
#define _SOCKET_PROTOCOL_I_

#include <string>
#include <vector>
#include "net/socket.h"
#include "net/wsprotocol.h"
#include "base/func.h"
#include "net/message.h"

namespace ff {

typedef Function<void(SocketObjPtr, int, const Message&)> SocketProtocolFunc;
class SocketProtocol
{
public:
    virtual ~SocketProtocol(){
        //printf("~SocketProtocol...\n");
    }
    SocketProtocol(SocketProtocolFunc f):m_nGotSize(0), m_funcProtocol(f){}

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
            if (m_funcProtocol){
                m_funcProtocol(sp_, IOEVENT_BROKEN, m_message);
                return;
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
        if (m_funcProtocol){
            m_funcProtocol(sp_, IOEVENT_RECV, m_message);
            return;
        }
        m_message.clear();
    }
public:
    size_t              m_nGotSize;
    Message             m_message;
    WSProtocol          m_oWSProtocol;
    SocketProtocolFunc  m_funcProtocol;
};

typedef SharedPtr<SocketProtocol>  SocketProtocolPtr;

}

#endif
