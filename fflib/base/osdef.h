
#ifndef _OS_DEF_H_
#define _OS_DEF_H_

#ifdef __MINGW32__
    #ifndef _WIN32
    #define _WIN32 1
    #endif // _WIN32
#endif // __MINGW32__

#ifdef _WIN32
    #include <winsock2.h>
    #define Socketfd SOCKET
#else
    #define Socketfd int
#endif // _WIN32

#endif
