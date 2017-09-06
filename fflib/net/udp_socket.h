#ifndef _FF_UDP_SOCKET_
#define _FF_UDP_SOCKET_

#include <stdio.h>  
#include <stdlib.h>  
  
#include <string.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <string>


namespace ff{

class UdpSocket
{
public:
    UdpSocket():
        m_listen_fd(-1),
        m_dest_fd(-1)
    {
    }
    ~UdpSocket()
    {
        this->close();
    }
    int bind(const std::string& bind_ip, int bind_port)
    {
        bzero(&m_listen_sin, sizeof(m_listen_sin));
        m_listen_sin.sin_family = AF_INET;
        if (bind_ip.empty() || bind_ip == "*")
        {
            m_listen_sin.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            m_listen_sin.sin_addr.s_addr = ::inet_addr(bind_ip.c_str());
        }
        m_listen_sin.sin_port = ::htons(bind_port);
        m_listen_fd    = ::socket(AF_INET, SOCK_DGRAM, 0);
        ::bind(m_listen_fd, (struct sockaddr*)&m_listen_sin, sizeof(m_listen_sin));
    }
    int bind_dest(const std::string& dest_ip, int dest_port)
    {
        if (m_dest_fd > 0)
        {
            ::close(m_dest_fd);
            m_dest_fd = -1;
        }
        bzero(&m_dest_sin, sizeof(m_dest_sin));
        m_dest_sin.sin_family = AF_INET;  
        m_dest_sin.sin_addr.s_addr = inet_addr(dest_ip.c_str());
        m_dest_sin.sin_port = htons(dest_port);
        m_dest_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_dest_fd < 0)
            return -1;
        return 0;
    }

    int recv(char* pdata, int len)
    {
        int ret = 0;
        do
        {
            ret = ::recvfrom(m_listen_fd, pdata, len, MSG_NOSIGNAL, (struct sockaddr *)&m_listen_sin, (socklen_t*)&m_listen_sin);
        }while(ret == EINTR);
        if (ret < 0)
        {
            return -1;
        }
        return ret;
    }

    int sendto(const char* pdata, int len)
    {
        int ret = 0;
        do
        {
            ret = ::sendto(m_dest_fd, pdata, len, 0, (struct sockaddr *)&m_dest_sin, sizeof(m_dest_sin));
        }while(ret == EINTR);
        if (ret < 0)
        {
            return -1;
        }
        return ret;
    }
    void close()
    {
        if (m_listen_fd > 0)
        {
            ::close(m_listen_fd);
            m_listen_fd = -1;
        }
        if (m_dest_fd > 0)
        {
            ::close(m_dest_fd);
            m_dest_fd = -1;
        }
    }

protected:
    int                 m_listen_fd;
    struct sockaddr_in  m_listen_sin;
    
    int                 m_dest_fd;
    struct sockaddr_in  m_dest_sin;
};
}

#endif
/*
//server

#include "udp_socket.h"

int main(int argc, char** argv) {  
  
    ff::UdpSocket usocket;
    usocket.bind("*", 6789);
    
    char message[256];
    while(1)  
    {  
        usocket.recv(message, sizeof(message)); 
        printf("Response from server:%s\n",message);  
        if(strncmp(message,"stop",4) == 0)//接受到的消息为 “stop”  
        {  
  
            printf("Sender has told me to end the connection\n");  
            break;  
        }  
    }  
  
    usocket.close();
    exit(0);  
  
    return (EXIT_SUCCESS);  
}

*/


/*
//client

#include "udp_socket.h"

#include <iostream>


int main(int argc, char** argv) {  
  
    ff::UdpSocket usocket;
    usocket.bind_dest("127.0.0.1", 6789);
    
    std::string data;
    while(data != "stop")  
    {
        cin >> data;
        usocket.sendto(data.c_str(), data.length() + 1);
        printf("send to server:%s\n", data.c_str());
    }  
  
    usocket.close();
    exit(0);  
  
    return (EXIT_SUCCESS);  
}

*/

