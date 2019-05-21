#ifndef _THREAD_H_
#define _THREAD_H_

#include<pthread.h>
#include <list>

#include "base/func.h"

namespace ff {

class Thread
{
    static void* thread_func(void* p_);

public:
    int create_thread(Function<void()> func, int num = 1);
    int join();

private:
    std::list<pthread_t> m_tid_list;
};

}

#endif
