#ifndef _ACCEPTOR_I_
#define _ACCEPTOR_I_

#include <string>


#include "netbase.h"

namespace ff {

class Acceptor: public Fd
{
public:
    virtual ~Acceptor(){}
    virtual int   open(const std::string& address_) = 0;

    int handleEpollWrite(){ return -1; }
};

}
#endif
