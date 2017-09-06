#ifndef _ACCEPTOR_I_
#define _ACCEPTOR_I_

#include <string>


#include "netbase.h"

namespace ff {

class acceptor_i: public Fd
{
public:
    virtual ~acceptor_i(){}
    virtual int   open(const std::string& address_) = 0;

    int handleEpollWrite(){ return -1; }
};

}
#endif
