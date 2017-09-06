#include "base/thread.h"
using namespace std;
using namespace ff;

void* Thread::thread_func(void* p_)
{
    Task* t = (Task*)p_;
    t->run();
    delete  t;
    return 0;
}

int Thread::create_thread(Task func, int num)
{
    for (int i = 0; i < num; ++i)
    {
        pthread_t ntid;
        Task* t = new Task(func);
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
