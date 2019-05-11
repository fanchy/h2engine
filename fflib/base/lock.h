#ifndef _LOCK_H_
#define _LOCK_H_

#include<pthread.h>

namespace ff {

class Mutex
{
public:
    Mutex();
    ~Mutex();
    bool lock();
    //bool time_lock(int us_);
    bool unlock();
    bool try_lock();

    pthread_mutex_t& get_mutex() {return m_mutex;}

private:
    pthread_mutex_t              m_mutex;
};

class RWMutex
{
public:
    RWMutex();
    ~RWMutex();
    bool rd_lock();
    bool wr_lock();
    bool unlock();
private:
    pthread_rwlock_t m_rwlock;
};

class ConditionVar
{
public:
    ConditionVar(Mutex& mutex_);
    ~ConditionVar();
    bool wait();
    //bool time_wait(int us_);
    bool signal();
    bool broadcast();

private:
    Mutex&                         m_mutex;
    pthread_cond_t                m_cond;
};

class LockGuard
{
public:
    LockGuard(Mutex& mutex_):
        m_flag(false),
        m_mutex(mutex_)
    {
        m_mutex.lock();
        m_flag = true;
    }
    ~LockGuard()
    {
        if (is_locked())
        {
            unlock();
        }
    }

    bool unlock()
    {
        m_flag = false;
        return m_mutex.unlock();
    }

    bool is_locked() const { return m_flag; }
private:
    bool               m_flag;
    Mutex&      m_mutex;
};

class RdLockGuard
{
public:
    RdLockGuard(RWMutex& mutex_):
        m_flag(false),
        m_mutex(mutex_)
    {
        m_mutex.rd_lock();
        m_flag = true;
    }
    ~RdLockGuard()
    {
        if (is_locked())
        {
            unlock();
        }
    }

    bool unlock()
    {
        m_flag = false;
        return m_mutex.unlock();
    }

    bool is_locked() const { return m_flag; }
private:
    bool                    m_flag;
    RWMutex&      m_mutex;
};

class WrLockGuard
{
public:
    WrLockGuard(RWMutex& mutex_):
        m_flag(false),
        m_mutex(mutex_)
    {
        m_mutex.wr_lock();
        m_flag = true;
    }
    ~WrLockGuard()
    {
        if (is_locked())
        {
            unlock();
        }
    }

    bool unlock()
    {
        m_flag = false;
        return m_mutex.unlock();
    }

    bool is_locked() const { return m_flag; }
private:
    bool                    m_flag;
    RWMutex&      m_mutex;
};

#ifdef linux
class SpinLock
{
public:
    SpinLock();
    ~SpinLock();
    void lock();
    void unlock();
    bool try_lock();
private:
    pthread_spinlock_t spinlock;
};

#else
typedef Mutex SpinLock ;
#endif

class spin_LockGuard
{
public:
    spin_LockGuard(SpinLock& lock_);
    ~spin_LockGuard();

private:
    SpinLock& lock;
};
}
#endif

