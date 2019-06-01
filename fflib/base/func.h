
#ifndef _FUNC_H_
#define _FUNC_H_

#include <stdio.h>
#include <stdexcept>
#include <string>

namespace ff {

template<typename T>
class Function;

template <typename T>struct RetTraits{ typedef T RET_TYPE; };
template <> struct RetTraits<void> { typedef int RET_TYPE;};
template <typename CLASS_TYPE>
struct ObjPtrGetter 
{ 
	static CLASS_TYPE* getPtr(CLASS_TYPE* p) { return p; }
	template <typename T>
	static CLASS_TYPE* getPtr(T p) { return p.get(); }
};
template <typename T> struct RefTraits			{	typedef T REAL_TYPE;	};
template <typename T> struct RefTraits<const T&>{	typedef T REAL_TYPE;	};
template <typename T> struct RefTraits<T&>		{	typedef T REAL_TYPE;	};

template <typename RET>                                 
struct CallUtil                                         
{                                                       
    template <typename FUNC>                          
    static RET call(FUNC f){ return f(); }          
    template <typename CLASS_TYPE, typename FUNC>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p){ return (p->*(f))(); }          
    template <typename FUNC, typename ARG1>                          
    static RET call(FUNC f, ARG1 arg1){ return f(arg1); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1){ return (p->*(f))(arg1); }          
    template <typename FUNC, typename ARG1, typename ARG2>                          
    static RET call(FUNC f, ARG1 arg1, ARG2 arg2){ return f(arg1, arg2); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2){ return (p->*(f))(arg1, arg2); }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3>                          
    static RET call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3){ return f(arg1, arg2, arg3); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3){ return (p->*(f))(arg1, arg2, arg3); }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                          
    static RET call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ return f(arg1, arg2, arg3, arg4); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ return (p->*(f))(arg1, arg2, arg3, arg4); }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                          
    static RET call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ return f(arg1, arg2, arg3, arg4, arg5); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ return (p->*(f))(arg1, arg2, arg3, arg4, arg5); }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                          
    static RET call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ return f(arg1, arg2, arg3, arg4, arg5, arg6); }          
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                          
    static RET callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ return (p->*(f))(arg1, arg2, arg3, arg4, arg5, arg6); }          
};                                                          
template <>                                                 
struct CallUtil<void>                                       
{                                                           
    template <typename FUNC>                          
    static int call(FUNC f){ f(); return 0;}        
    template <typename CLASS_TYPE, typename FUNC>                          
    static int callMethod(FUNC f, CLASS_TYPE* p){ (p->*(f))();return 0; }          
    template <typename FUNC, typename ARG1>                          
    static int call(FUNC f, ARG1 arg1){ f(arg1); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1){ (p->*(f))(arg1);return 0; }          
    template <typename FUNC, typename ARG1, typename ARG2>                          
    static int call(FUNC f, ARG1 arg1, ARG2 arg2){ f(arg1, arg2); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2){ (p->*(f))(arg1, arg2);return 0; }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3>                          
    static int call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3){ f(arg1, arg2, arg3); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3){ (p->*(f))(arg1, arg2, arg3);return 0; }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                          
    static int call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ f(arg1, arg2, arg3, arg4); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ (p->*(f))(arg1, arg2, arg3, arg4);return 0; }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                          
    static int call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ f(arg1, arg2, arg3, arg4, arg5); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ (p->*(f))(arg1, arg2, arg3, arg4, arg5);return 0; }          
    template <typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                          
    static int call(FUNC f, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ f(arg1, arg2, arg3, arg4, arg5, arg6); return 0;}        
    template <typename CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                          
    static int callMethod(FUNC f, CLASS_TYPE* p, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ (p->*(f))(arg1, arg2, arg3, arg4, arg5, arg6);return 0; }          
};                                                          
template<typename RET>                                                               
class IFuncArg0                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg0() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call() = 0;                            
    virtual IFuncArg0<RET>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET>                                                   
class ImplFuncArg0 : public IFuncArg0<RET>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg0(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call() {                                       
        return CallUtil<RET>::call(func);                                            
    }                                                                                  
    virtual IFuncArg0<RET>* copy() {                                                
        return new ImplFuncArg0<T, RET>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET>                                                               
class Function<RET()>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET()>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET()>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg0<T, RET>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()()                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call();                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg0<RET>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1>                                                               
class IFuncArg1                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg1() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1) = 0;                            
    virtual IFuncArg1<RET, ARG1>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1>                                                   
class ImplFuncArg1 : public IFuncArg1<RET, ARG1>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg1(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1) {                                       
        return CallUtil<RET>::call(func, arg1);                                            
    }                                                                                  
    virtual IFuncArg1<RET, ARG1>* copy() {                                                
        return new ImplFuncArg1<T, RET, ARG1>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1>                                                               
class Function<RET(ARG1)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg1<T, RET, ARG1>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg1<RET, ARG1>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1, typename ARG2>                                                               
class IFuncArg2                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg2() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2) = 0;                            
    virtual IFuncArg2<RET, ARG1, ARG2>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1, typename ARG2>                                                   
class ImplFuncArg2 : public IFuncArg2<RET, ARG1, ARG2>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg2(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2) {                                       
        return CallUtil<RET>::call(func, arg1, arg2);                                            
    }                                                                                  
    virtual IFuncArg2<RET, ARG1, ARG2>* copy() {                                                
        return new ImplFuncArg2<T, RET, ARG1, ARG2>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1, typename ARG2>                                                               
class Function<RET(ARG1, ARG2)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1, ARG2)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1, ARG2)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg2<T, RET, ARG1, ARG2>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1, arg2);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg2<RET, ARG1, ARG2>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3>                                                               
class IFuncArg3                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg3() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3) = 0;                            
    virtual IFuncArg3<RET, ARG1, ARG2, ARG3>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1, typename ARG2, typename ARG3>                                                   
class ImplFuncArg3 : public IFuncArg3<RET, ARG1, ARG2, ARG3>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg3(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3) {                                       
        return CallUtil<RET>::call(func, arg1, arg2, arg3);                                            
    }                                                                                  
    virtual IFuncArg3<RET, ARG1, ARG2, ARG3>* copy() {                                                
        return new ImplFuncArg3<T, RET, ARG1, ARG2, ARG3>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3>                                                               
class Function<RET(ARG1, ARG2, ARG3)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1, ARG2, ARG3)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1, ARG2, ARG3)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg3<T, RET, ARG1, ARG2, ARG3>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1, arg2, arg3);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg3<RET, ARG1, ARG2, ARG3>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                                                               
class IFuncArg4                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg4() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) = 0;                            
    virtual IFuncArg4<RET, ARG1, ARG2, ARG3, ARG4>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                                                   
class ImplFuncArg4 : public IFuncArg4<RET, ARG1, ARG2, ARG3, ARG4>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg4(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) {                                       
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                                            
    }                                                                                  
    virtual IFuncArg4<RET, ARG1, ARG2, ARG3, ARG4>* copy() {                                                
        return new ImplFuncArg4<T, RET, ARG1, ARG2, ARG3, ARG4>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                                                               
class Function<RET(ARG1, ARG2, ARG3, ARG4)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1, ARG2, ARG3, ARG4)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1, ARG2, ARG3, ARG4)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg4<T, RET, ARG1, ARG2, ARG3, ARG4>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1, arg2, arg3, arg4);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg4<RET, ARG1, ARG2, ARG3, ARG4>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                                                               
class IFuncArg5                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg5() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) = 0;                            
    virtual IFuncArg5<RET, ARG1, ARG2, ARG3, ARG4, ARG5>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                                                   
class ImplFuncArg5 : public IFuncArg5<RET, ARG1, ARG2, ARG3, ARG4, ARG5>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg5(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) {                                       
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                                            
    }                                                                                  
    virtual IFuncArg5<RET, ARG1, ARG2, ARG3, ARG4, ARG5>* copy() {                                                
        return new ImplFuncArg5<T, RET, ARG1, ARG2, ARG3, ARG4, ARG5>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                                                               
class Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg5<T, RET, ARG1, ARG2, ARG3, ARG4, ARG5>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1, arg2, arg3, arg4, arg5);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg5<RET, ARG1, ARG2, ARG3, ARG4, ARG5>* pFunc;                                                          
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                                                               
class IFuncArg6                                                                       
{                                                                                      
public:                                                                                
    virtual ~IFuncArg6() {}                                                           
    virtual typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) = 0;                            
    virtual IFuncArg6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>* copy() = 0;                                             
};                                                                                     
template<typename T, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                                                   
class ImplFuncArg6 : public IFuncArg6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>                                         
{                                                                                      
public:                                                                                
    ImplFuncArg6(const T& t) : func(t) {}                                             
    typename RetTraits<RET>::RET_TYPE call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) {                                       
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                                            
    }                                                                                  
    virtual IFuncArg6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>* copy() {                                                
        return new ImplFuncArg6<T, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>(func);                                      
    }                                                                                  
    T func;                                                                            
};                                                                                     
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                                                               
class Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>                                                                
{                                                                                      
public:                                                                                
    Function(): pFunc(NULL) {                                                          
    }                                                                                  
    template<typename T>                                                               
    Function(const T& t): pFunc(NULL) {                                               
        this->assign(t);                                                               
    }                                                                                  
    ~Function() {                                                                      
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    Function(const Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>& t): pFunc(NULL) {                                             
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    void operator=(const Function<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>& t) {                                       
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        if (t.pFunc) {                                                                 
            pFunc = t.pFunc->copy();                                                   
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void operator=(const T& t) {                                                       
        this->assign(t);                                                               
    }                                                                                  
    void assign(long t) {                                                              
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
    }                                                                                  
    template<typename T>                                                               
    void assign(const T& t) {                                                          
        if (pFunc) {                                                                   
            delete  pFunc;                                                             
            pFunc = NULL;                                                              
        }                                                                              
        pFunc = new ImplFuncArg6<T, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>(t);                                        
    }                                                                                  
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)                                                                 
    {                                                                                  
        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      
        return pFunc->call(arg1, arg2, arg3, arg4, arg5, arg6);                                                        
    }                                                                                  
    operator bool() const                                                              
    {                                                                                  
        return NULL != pFunc;                                                          
    }                                                                                  
protected:                                                                             
    IFuncArg6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>* pFunc;                                                          
};                                                                                     
template <typename RET>                             
struct FunctorTmp0_0                                              
{                                                      
    FunctorTmp0_0(RET(*f)()): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func);                               
    }                                                  
    RET(*func)();                                    
};                                                     
template <typename RET, typename ARG1>                             
struct FunctorTmp1_0                                              
{                                                      
    FunctorTmp1_0(RET(*f)(ARG1)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1);                               
    }                                                  
    RET(*func)(ARG1);                                    
};                                                     
template <typename RET, typename ARG1>                             
struct FunctorTmp1_1                                              
{                                                      
    FunctorTmp1_1(RET(*f)(ARG1), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1);                               
    }                                                  
    RET(*func)(ARG1);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2>                             
struct FunctorTmp2_0                                              
{                                                      
    FunctorTmp2_0(RET(*f)(ARG1, ARG2)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2);                               
    }                                                  
    RET(*func)(ARG1, ARG2);                                    
};                                                     
template <typename RET, typename ARG1, typename ARG2>                             
struct FunctorTmp2_1                                              
{                                                      
    FunctorTmp2_1(RET(*f)(ARG1, ARG2), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2);                               
    }                                                  
    RET(*func)(ARG1, ARG2);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2>                             
struct FunctorTmp2_2                                              
{                                                      
    FunctorTmp2_2(RET(*f)(ARG1, ARG2), ARG1 a1, ARG2 a2): func(f) , arg1(a1), arg2(a2) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2);                               
    }                                                  
    RET(*func)(ARG1, ARG2);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                             
struct FunctorTmp3_0                                              
{                                                      
    FunctorTmp3_0(RET(*f)(ARG1, ARG2, ARG3)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3);                                    
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                             
struct FunctorTmp3_1                                              
{                                                      
    FunctorTmp3_1(RET(*f)(ARG1, ARG2, ARG3), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                             
struct FunctorTmp3_2                                              
{                                                      
    FunctorTmp3_2(RET(*f)(ARG1, ARG2, ARG3), ARG1 a1, ARG2 a2): func(f) , arg1(a1), arg2(a2) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                             
struct FunctorTmp3_3                                              
{                                                      
    FunctorTmp3_3(RET(*f)(ARG1, ARG2, ARG3), ARG1 a1, ARG2 a2, ARG3 a3): func(f) , arg1(a1), arg2(a2), arg3(a3) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                             
struct FunctorTmp4_0                                              
{                                                      
    FunctorTmp4_0(RET(*f)(ARG1, ARG2, ARG3, ARG4)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4);                                    
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                             
struct FunctorTmp4_1                                              
{                                                      
    FunctorTmp4_1(RET(*f)(ARG1, ARG2, ARG3, ARG4), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                             
struct FunctorTmp4_2                                              
{                                                      
    FunctorTmp4_2(RET(*f)(ARG1, ARG2, ARG3, ARG4), ARG1 a1, ARG2 a2): func(f) , arg1(a1), arg2(a2) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                             
struct FunctorTmp4_3                                              
{                                                      
    FunctorTmp4_3(RET(*f)(ARG1, ARG2, ARG3, ARG4), ARG1 a1, ARG2 a2, ARG3 a3): func(f) , arg1(a1), arg2(a2), arg3(a3) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                             
struct FunctorTmp4_4                                              
{                                                      
    FunctorTmp4_4(RET(*f)(ARG1, ARG2, ARG3, ARG4), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_0                                              
{                                                      
    FunctorTmp5_0(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_1                                              
{                                                      
    FunctorTmp5_1(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_2                                              
{                                                      
    FunctorTmp5_2(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), ARG1 a1, ARG2 a2): func(f) , arg1(a1), arg2(a2) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4, ARG5 arg5)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_3                                              
{                                                      
    FunctorTmp5_3(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), ARG1 a1, ARG2 a2, ARG3 a3): func(f) , arg1(a1), arg2(a2), arg3(a3) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4, ARG5 arg5)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_4                                              
{                                                      
    FunctorTmp5_4(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG5 arg5)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                             
struct FunctorTmp5_5                                              
{                                                      
    FunctorTmp5_5(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_0                                              
{                                                      
    FunctorTmp6_0(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)): func(f)  {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_1                                              
{                                                      
    FunctorTmp6_1(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1): func(f) , arg1(a1) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_2                                              
{                                                      
    FunctorTmp6_2(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1, ARG2 a2): func(f) , arg1(a1), arg2(a2) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_3                                              
{                                                      
    FunctorTmp6_3(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1, ARG2 a2, ARG3 a3): func(f) , arg1(a1), arg2(a2), arg3(a3) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4, ARG5 arg5, ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_4                                              
{                                                      
    FunctorTmp6_4(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG5 arg5, ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_5                                              
{                                                      
    FunctorTmp6_5(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5) {}                    
    typename RetTraits<RET>::RET_TYPE operator()(ARG6 arg6)                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
};                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                             
struct FunctorTmp6_6                                              
{                                                      
    FunctorTmp6_6(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5, ARG6 a6): func(f) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6) {}                    
    typename RetTraits<RET>::RET_TYPE operator()()                                 
    {                                                  
        return CallUtil<RET>::call(func, arg1, arg2, arg3, arg4, arg5, arg6);                               
    }                                                  
    RET(*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                                    
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
    typename RefTraits<ARG6>::REAL_TYPE arg6;
};                                                     
template <typename RET>                            
FunctorTmp0_0<RET> funcbind(RET(*f)())         
{                                                     
    FunctorTmp0_0<RET> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1>                            
FunctorTmp1_0<RET, ARG1> funcbind(RET(*f)(ARG1))         
{                                                     
    FunctorTmp1_0<RET, ARG1> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1>                            
FunctorTmp1_1<RET, ARG1> funcbind(RET(*f)(ARG1), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp1_1<RET, ARG1> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2>                            
FunctorTmp2_0<RET, ARG1, ARG2> funcbind(RET(*f)(ARG1, ARG2))         
{                                                     
    FunctorTmp2_0<RET, ARG1, ARG2> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2>                            
FunctorTmp2_1<RET, ARG1, ARG2> funcbind(RET(*f)(ARG1, ARG2), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp2_1<RET, ARG1, ARG2> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2>                            
FunctorTmp2_2<RET, ARG1, ARG2> funcbind(RET(*f)(ARG1, ARG2), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    FunctorTmp2_2<RET, ARG1, ARG2> ret(f, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                            
FunctorTmp3_0<RET, ARG1, ARG2, ARG3> funcbind(RET(*f)(ARG1, ARG2, ARG3))         
{                                                     
    FunctorTmp3_0<RET, ARG1, ARG2, ARG3> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                            
FunctorTmp3_1<RET, ARG1, ARG2, ARG3> funcbind(RET(*f)(ARG1, ARG2, ARG3), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp3_1<RET, ARG1, ARG2, ARG3> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                            
FunctorTmp3_2<RET, ARG1, ARG2, ARG3> funcbind(RET(*f)(ARG1, ARG2, ARG3), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    FunctorTmp3_2<RET, ARG1, ARG2, ARG3> ret(f, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3>                            
FunctorTmp3_3<RET, ARG1, ARG2, ARG3> funcbind(RET(*f)(ARG1, ARG2, ARG3), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    FunctorTmp3_3<RET, ARG1, ARG2, ARG3> ret(f, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
FunctorTmp4_0<RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4))         
{                                                     
    FunctorTmp4_0<RET, ARG1, ARG2, ARG3, ARG4> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
FunctorTmp4_1<RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp4_1<RET, ARG1, ARG2, ARG3, ARG4> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
FunctorTmp4_2<RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    FunctorTmp4_2<RET, ARG1, ARG2, ARG3, ARG4> ret(f, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
FunctorTmp4_3<RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    FunctorTmp4_3<RET, ARG1, ARG2, ARG3, ARG4> ret(f, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
FunctorTmp4_4<RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    FunctorTmp4_4<RET, ARG1, ARG2, ARG3, ARG4> ret(f, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_0<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5))         
{                                                     
    FunctorTmp5_0<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_1<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp5_1<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_2<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    FunctorTmp5_2<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_3<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    FunctorTmp5_3<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_4<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    FunctorTmp5_4<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
FunctorTmp5_5<RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5)         
{                                                     
    FunctorTmp5_5<RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, arg1, arg2, arg3, arg4, arg5);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_0<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6))         
{                                                     
    FunctorTmp6_0<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_1<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    FunctorTmp6_1<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_2<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    FunctorTmp6_2<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_3<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    FunctorTmp6_3<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_4<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    FunctorTmp6_4<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_5<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5)         
{                                                     
    FunctorTmp6_5<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1, arg2, arg3, arg4, arg5);                  
    return ret;                                       
}                                                     
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
FunctorTmp6_6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5, typename RefTraits<ARG6>::REAL_TYPE arg6)         
{                                                     
    FunctorTmp6_6<RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, arg1, arg2, arg3, arg4, arg5, arg6);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET>         
struct ClassFunctorTmp0_0                                              
{                                                      
    ClassFunctorTmp0_0(RET(CLASS_TYPE::*f)(), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr));            
    }                                                  
    RET(CLASS_TYPE::*func)();                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1>         
struct ClassFunctorTmp1_0                                              
{                                                      
    ClassFunctorTmp1_0(RET(CLASS_TYPE::*f)(ARG1), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1>         
struct ClassFunctorTmp1_1                                              
{                                                      
    ClassFunctorTmp1_1(RET(CLASS_TYPE::*f)(ARG1), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>         
struct ClassFunctorTmp2_0                                              
{                                                      
    ClassFunctorTmp2_0(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>         
struct ClassFunctorTmp2_1                                              
{                                                      
    ClassFunctorTmp2_1(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>         
struct ClassFunctorTmp2_2                                              
{                                                      
    ClassFunctorTmp2_2(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p, ARG1 a1, ARG2 a2): func(f), ptr(p) , arg1(a1), arg2(a2) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>         
struct ClassFunctorTmp3_0                                              
{                                                      
    ClassFunctorTmp3_0(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>         
struct ClassFunctorTmp3_1                                              
{                                                      
    ClassFunctorTmp3_1(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>         
struct ClassFunctorTmp3_2                                              
{                                                      
    ClassFunctorTmp3_2(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, ARG1 a1, ARG2 a2): func(f), ptr(p) , arg1(a1), arg2(a2) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>         
struct ClassFunctorTmp3_3                                              
{                                                      
    ClassFunctorTmp3_3(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>         
struct ClassFunctorTmp4_0                                              
{                                                      
    ClassFunctorTmp4_0(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>         
struct ClassFunctorTmp4_1                                              
{                                                      
    ClassFunctorTmp4_1(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>         
struct ClassFunctorTmp4_2                                              
{                                                      
    ClassFunctorTmp4_2(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, ARG1 a1, ARG2 a2): func(f), ptr(p) , arg1(a1), arg2(a2) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>         
struct ClassFunctorTmp4_3                                              
{                                                      
    ClassFunctorTmp4_3(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>         
struct ClassFunctorTmp4_4                                              
{                                                      
    ClassFunctorTmp4_4(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_0                                              
{                                                      
    ClassFunctorTmp5_0(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_1                                              
{                                                      
    ClassFunctorTmp5_1(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_2                                              
{                                                      
    ClassFunctorTmp5_2(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, ARG1 a1, ARG2 a2): func(f), ptr(p) , arg1(a1), arg2(a2) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4, ARG5 arg5)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_3                                              
{                                                      
    ClassFunctorTmp5_3(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4, ARG5 arg5)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_4                                              
{                                                      
    ClassFunctorTmp5_4(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG5 arg5)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>         
struct ClassFunctorTmp5_5                                              
{                                                      
    ClassFunctorTmp5_5(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_0                                              
{                                                      
    ClassFunctorTmp6_0(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p): func(f), ptr(p)  {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_1                                              
{                                                      
    ClassFunctorTmp6_1(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1): func(f), ptr(p) , arg1(a1) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_2                                              
{                                                      
    ClassFunctorTmp6_2(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1, ARG2 a2): func(f), ptr(p) , arg1(a1), arg2(a2) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_3                                              
{                                                      
    ClassFunctorTmp6_3(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG4 arg4, ARG5 arg5, ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_4                                              
{                                                      
    ClassFunctorTmp6_4(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG5 arg5, ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_5                                              
{                                                      
    ClassFunctorTmp6_5(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5) {}       
    typename RetTraits<RET>::RET_TYPE operator()(ARG6 arg6)   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>         
struct ClassFunctorTmp6_6                                              
{                                                      
    ClassFunctorTmp6_6(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5, ARG6 a6): func(f), ptr(p) , arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6) {}       
    typename RetTraits<RET>::RET_TYPE operator()()   
    {                                                  
        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr), arg1, arg2, arg3, arg4, arg5, arg6);            
    }                                                  
    RET(CLASS_TYPE::*func)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);                        
    OBJ_PTR ptr;                                       
    typename RefTraits<ARG1>::REAL_TYPE arg1;
    typename RefTraits<ARG2>::REAL_TYPE arg2;
    typename RefTraits<ARG3>::REAL_TYPE arg3;
    typename RefTraits<ARG4>::REAL_TYPE arg4;
    typename RefTraits<ARG5>::REAL_TYPE arg5;
    typename RefTraits<ARG6>::REAL_TYPE arg6;
};                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET>                            
ClassFunctorTmp0_0<CLASS_TYPE, OBJ_PTR, RET> funcbind(RET(CLASS_TYPE::*f)(), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp0_0<CLASS_TYPE, OBJ_PTR, RET> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1>                            
ClassFunctorTmp1_0<CLASS_TYPE, OBJ_PTR, RET, ARG1> funcbind(RET(CLASS_TYPE::*f)(ARG1), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp1_0<CLASS_TYPE, OBJ_PTR, RET, ARG1> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1>                            
ClassFunctorTmp1_1<CLASS_TYPE, OBJ_PTR, RET, ARG1> funcbind(RET(CLASS_TYPE::*f)(ARG1), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp1_1<CLASS_TYPE, OBJ_PTR, RET, ARG1> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>                            
ClassFunctorTmp2_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp2_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>                            
ClassFunctorTmp2_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp2_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2>                            
ClassFunctorTmp2_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    ClassFunctorTmp2_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2> ret(f, p, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>                            
ClassFunctorTmp3_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp3_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>                            
ClassFunctorTmp3_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp3_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>                            
ClassFunctorTmp3_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    ClassFunctorTmp3_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> ret(f, p, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3>                            
ClassFunctorTmp3_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    ClassFunctorTmp3_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3> ret(f, p, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
ClassFunctorTmp4_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp4_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
ClassFunctorTmp4_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp4_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
ClassFunctorTmp4_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    ClassFunctorTmp4_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> ret(f, p, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
ClassFunctorTmp4_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    ClassFunctorTmp4_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> ret(f, p, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>                            
ClassFunctorTmp4_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    ClassFunctorTmp4_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4> ret(f, p, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp5_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp5_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    ClassFunctorTmp5_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    ClassFunctorTmp5_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    ClassFunctorTmp5_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>                            
ClassFunctorTmp5_5<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5)         
{                                                     
    ClassFunctorTmp5_5<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5> ret(f, p, arg1, arg2, arg3, arg4, arg5);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p)         
{                                                     
    ClassFunctorTmp6_0<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1)         
{                                                     
    ClassFunctorTmp6_1<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2)         
{                                                     
    ClassFunctorTmp6_2<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1, arg2);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3)         
{                                                     
    ClassFunctorTmp6_3<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1, arg2, arg3);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4)         
{                                                     
    ClassFunctorTmp6_4<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1, arg2, arg3, arg4);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_5<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5)         
{                                                     
    ClassFunctorTmp6_5<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1, arg2, arg3, arg4, arg5);                  
    return ret;                                       
}                                                     
template <typename CLASS_TYPE, typename OBJ_PTR, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>                            
ClassFunctorTmp6_6<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> funcbind(RET(CLASS_TYPE::*f)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6), OBJ_PTR p, typename RefTraits<ARG1>::REAL_TYPE arg1, typename RefTraits<ARG2>::REAL_TYPE arg2, typename RefTraits<ARG3>::REAL_TYPE arg3, typename RefTraits<ARG4>::REAL_TYPE arg4, typename RefTraits<ARG5>::REAL_TYPE arg5, typename RefTraits<ARG6>::REAL_TYPE arg6)         
{                                                     
    ClassFunctorTmp6_6<CLASS_TYPE, OBJ_PTR, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> ret(f, p, arg1, arg2, arg3, arg4, arg5, arg6);                  
    return ret;                                       
}                                                     

//!end namespace ff
}
#endif

