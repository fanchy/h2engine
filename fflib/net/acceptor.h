#ifndef _ACCEPTOR_I_
#define _ACCEPTOR_I_

#include <string>


namespace ff {

class Acceptor
{
public:
    virtual ~Acceptor(){}
    virtual int   open(const std::string& address_) = 0;
    virtual void close()= 0;
};

}
#endif
