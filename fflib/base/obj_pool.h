#ifndef _OBJ_POOL_H_
#define _OBJ_POOL_H_

#ifdef linux
#include <malloc.h>
#endif
#include <string.h>
#include <list>


namespace ff {

template <typename T>
class FFObjPool
{
public:
    FFObjPool(){
    }
    virtual ~FFObjPool() {
        for (typename std::list<T*>::iterator it = m_free.begin(); it != m_free.end(); ++it){
            ::free(*it);
        }
        m_free.clear();
    }
    T* alloc(){
        if (false == m_free.empty()){
            T* pret = m_free.front();
            m_free.pop_front();
            pret = new (pret) T;
            return pret;
        }
        else{
            void* p = ::malloc(sizeof(T));
            T* pret = new (p) T;
            return pret;
        }
        return NULL;
    }
    void release(T* p){
        p->~T();
        ::memset((void*)p, 0, sizeof(T));
        m_free.push_back(p);
    }

protected:
    std::list<T*>    m_free;
};


}

#endif

//!
