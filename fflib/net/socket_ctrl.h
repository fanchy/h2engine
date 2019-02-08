#ifndef _SOCKET_CTRL_I_
#define _SOCKET_CTRL_I_

#include <string>


namespace ff {

class SocketI;

class SocketCtrlI
{
public:
    virtual ~SocketCtrlI(){}

    virtual int handleOpen(SocketI*)                                    {return 0;}
    virtual int checkPreSend(SocketI*, std::string& buff, int flag)         {return 0;}

    virtual int handleError(SocketI*)                                    = 0;
    virtual int handleRead(SocketI*, const char* buff, size_t len)       = 0;
    virtual int handleWriteCompleted(SocketI*)                         {return 0;}

    virtual int get_type() const { return 0; }
};

}

#endif
