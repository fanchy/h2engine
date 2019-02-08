
//!  连接器
#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <assert.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "net/socketimpl/socket_linux.h"
#include "net/socketimpl/socket_win.h"
#include "base/str_tool.h"
#include "net/msg_handler.h"
#include "net/ctrlimpl/socket_ctrl_common.h"

namespace ff {

class Connector
{
public:
    static SocketObjPtr connect(const std::string& host_, EventLoop* e_, MsgHandler* msg_handler_, TaskQueue* tq_)
    {
        SocketObjPtr ret = NULL;
        //! example tcp://192.168.1.1:1024
        std::vector<std::string> vt;
        StrTool::split(host_, vt, "://");
        if (vt.size() != 2) return NULL;

        std::vector<std::string> vt2;
        StrTool::split(vt[1], vt2, ":");
        if (vt2.size() != 2) return NULL;
        if (vt2[0] == "*")
        {
            vt2[0] = "127.0.0.1";
        }
        SocketFd s;
        struct sockaddr_in addr;

        if((s = socket(AF_INET,SOCK_STREAM,0)) < 0)
        {
            perror("socket");
            return ret;
        }
        /* 填写sockaddr_in结构*/
        #ifndef _WIN32
        bzero(&addr,sizeof(addr));
        #else
        ZeroMemory(&addr,sizeof(addr));
        #endif
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(atoi(vt2[1].c_str()));
        addr.sin_addr.s_addr = inet_addr(vt2[0].c_str());
        /* 尝试连线*/
        if(::connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            return ret;
        }
        #ifndef _WIN32
        SocketLinux* pret = new SocketLinux(e_, new SocketCtrlCommon(msg_handler_), s, tq_);
        ret = pret;
        pret->refSelf(ret);
        #else
        SocketWin* pret = new SocketWin(e_, new SocketCtrlCommon(msg_handler_), s, tq_);
        ret = pret;
        pret->refSelf(ret);
        #endif
        ret->open();
        return ret;
    }

};

}
#endif
