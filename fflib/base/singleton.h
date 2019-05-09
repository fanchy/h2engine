#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

template<class T>
class Singleton
{
public:
    static T& instance()
    {
	pthread_once(&m_ponce, &Singleton::init);
        return *m_instance;
    }

    static T* instance_ptr()
    {
        return m_instance;
    }
private:
    static void init()
    {
        m_instance = new T();
        atexit(destroy);
    }
    static void destroy()
    {
        //printf("%s begin\n", __PRETTY_FUNCTION__);
        if(m_instance)
        {
            delete m_instance;
            m_instance = 0;
        }

        //printf("%s end\n", __PRETTY_FUNCTION__);
    }

private:
    static T* volatile m_instance;
    static pthread_once_t m_ponce;
};

template<class T>
T* volatile Singleton<T>::m_instance = NULL;

template<typename T>
pthread_once_t Singleton<T>::m_ponce = PTHREAD_ONCE_INIT;

#endif

