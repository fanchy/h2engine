#ifndef _SOCKET_OP_H_
#define _SOCKET_OP_H_

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


namespace ff {

struct SocketOp
{
    static int set_nonblock(SOCKET_TYPE fd_)
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
    static std::string getpeername(SOCKET_TYPE sockfd)
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

    static int set_no_delay(SOCKET_TYPE sockfd, bool flag_ = true)
    {
        int on = flag_? 1: 0;
        return ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,(const char*)&on,sizeof(on));
    }

	static void close(SOCKET_TYPE sockfd)
	{
		#ifdef _WIN32
		::closesocket(sockfd);
		#else
		::close(sockfd);
		#endif
	}
};

}

#endif
