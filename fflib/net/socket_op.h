#ifndef _SOCKET_OP_H_
#define _SOCKET_OP_H_

#include "base/osdef.h"
#include "base/fftype.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#else
#include <windows.h>
#endif

#include <string>

#define  RECV_BUFFER_SIZE 8192
namespace ff {

struct SocketOp
{
    static int set_nonblock(Socketfd fd_)
    {
    	#ifdef WIN32
		    DWORD nMode = 1;
		    int nRes = ioctlsocket(fd_, FIONBIO, &nMode);
		    if (nRes == SOCKET_ERROR) {
		        return -1;
		    }
		#else
	        int flags;
	        flags = fcntl(fd_, F_GETFL, 0);
	        if ((flags = fcntl(fd_, F_SETFL, flags | O_NONBLOCK)) < 0)
	        {
	            return -1;
	        }
    	#endif
        return 0;
    }
    static std::string getpeername(Socketfd sockfd)
    {
        std::string ret;
        struct sockaddr_in sa;
        socklen_t len = sizeof(sa);
        if(!::getpeername(sockfd, (struct sockaddr *)&sa, &len))
        {
            ret = ::inet_ntoa(sa.sin_addr);
        }
        return ret;
    }

    static int set_no_delay(Socketfd sockfd, bool flag_ = true)
    {
        int on = flag_? 1: 0;
        return ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,(const char*)&on,sizeof(on));
    }

	static void close(Socketfd sockfd)
	{
		#ifdef _WIN32
		::closesocket(sockfd);
		#else
		::close(sockfd);
		#endif
	}
	static int readAll(Socketfd fd, std::string& retData)//!success ret>=0 fail -1
	{
        int nread = 0;
        char recv_buffer[RECV_BUFFER_SIZE] = {0};
        do
        {
#ifdef _WIN32
            nread = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
            if (nread == SOCKET_ERROR || nread == 0)
            {
                if  (WSAEWOULDBLOCK == WSAGetLastError()){
                    return (int)retData.size();
                }
                return -1;
            }
#else
            nread = read(fd, recv_buffer, sizeof(recv_buffer));
            if (nread == 0)
            {
                return -1;
            }
            else if (nread < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EWOULDBLOCK)
                {
                    return (int)retData.size();
                }
                else
                {
                    return -1;
                }
            }
#endif // _WIN32
            else
            {
                retData.append(recv_buffer, nread);
            }
        }while(nread == RECV_BUFFER_SIZE);

        return (int)retData.size();

        return 0;
    }
    static int sendAll(Socketfd fd, const std::string& retData)//!success ret>=0 fail -1
	{
        int nwritten = 0;
        while(nwritten < (int)retData.size())
        {
            int nret = 0;
            #ifndef linux
            if((nret = send(fd, retData.c_str() + nwritten, (int)retData.size() - nwritten, 0)) <= 0)
            #else
            if((nret = send(fd, retData.c_str() + nwritten, (int)retData.size() - nwritten, MSG_NOSIGNAL)) <= 0)
            #endif
            {
                if (EINTR == errno)
                {
                    continue;
                }
                else if (EWOULDBLOCK == errno)
                {
                    return nwritten;
                }
                else
                {
                    return -1;
                }
            }

            nwritten += nret;
        }
        return nwritten;
    }
    static int acceptSocket(Socketfd fdListen)
	{
	    Socketfd newfd = -1;

    	#ifdef _WIN32
        	newfd = accept(fdListen, NULL, NULL);
    	    if (newfd == INVALID_SOCKET) {
    	        wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
    	        return 0;
    	    }
    	#else
            struct sockaddr_storage addr;
            socklen_t addrlen = sizeof(addr);
            if ((newfd = ::accept(fdListen, (struct sockaddr *)&addr, &addrlen)) == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return 0;
                }
                else if (errno == EINTR || errno == EMFILE || errno == ECONNABORTED || errno == ENFILE ||
                            errno == EPERM || errno == ENOBUFS || errno == ENOMEM)
                {
                    return 0;
                }
                perror("accept other error");
                return -1;
            }
        #endif
        return newfd;
	}
};

}

#endif
