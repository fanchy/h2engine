#include "base/thread.h"
using namespace std;
using namespace ff;

struct TmpThreadData{
    TmpThreadData(const Function<void()>& f):func(f){}
    Function<void()> func;
};
void* Thread::thread_func(void* p_)
{
    TmpThreadData* t = (TmpThreadData*)p_;
    t->func();
    delete  t;
    return 0;
}

int Thread::create_thread(Function<void()> func, int num)
{
    for (int i = 0; i < num; ++i)
    {
        pthread_t ntid;
        TmpThreadData* t = new TmpThreadData(func);
        if (0 == ::pthread_create(&ntid, NULL, thread_func, t))
        {
            m_tid_list.push_back(ntid);
        }
    }
    return 0;
}

int Thread::join()
{
    list<pthread_t>::iterator it = m_tid_list.begin();
    for (; it != m_tid_list.end(); ++it)
    {
        pthread_join((*it), NULL);
    }
    m_tid_list.clear();
    return 0;
}
