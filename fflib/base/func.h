
#ifndef _FF_FUNC_H_
#define _FF_FUNC_H_

#include "base/smart_ptr.h"

namespace ff{

template<typename T>
struct FFFunction;
template<typename T>
class FuncBase;
template<typename FUNC, typename T>
class FuncImpl;

struct VoidVal{};
struct VoidRet{
    static VoidVal val(){
        VoidVal ret;
        return ret;
    }
};
template <typename T>
struct VoidUtil{
    typedef T RET_TYPE;
};
template <>
struct VoidUtil<void>{
    typedef VoidVal RET_TYPE;
};

template<typename R>
class FuncBase<R()>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call() = 0;
};
template<typename R, typename ARG1>
class FuncBase<R(ARG1)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1 arg1) = 0;
};
template<typename R, typename ARG1, typename ARG2>
class FuncBase<R(ARG1, ARG2)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3>
class FuncBase<R(ARG1, ARG2, ARG3)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4, ARG5) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) = 0;
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
public:
    virtual ~FuncBase(){}
    virtual typename VoidUtil<R>::RET_TYPE call(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) = 0;
};

template<typename R>
struct FFFunction<R()>{
    typedef FuncBase<R()>                    FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R()>(f);
    }
    Ret operator()(){
        return func->call();
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1>
struct FFFunction<R(ARG1)>{
    typedef FuncBase<R(ARG1)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1)>(f);
    }
    Ret operator()(ARG1 arg1){
        return func->call(arg1);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2>
struct FFFunction<R(ARG1, ARG2)>{
    typedef FuncBase<R(ARG1, ARG2)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2){
        return func->call(arg1, arg2);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3>
struct FFFunction<R(ARG1, ARG2, ARG3)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3){
        return func->call(arg1, arg2, arg3);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){
        return func->call(arg1, arg2, arg3, arg4);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4, ARG5)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4, ARG5)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){
        return func->call(arg1, arg2, arg3, arg4, arg5);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){
        return func->call(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7){
        return func->call(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8){
        return func->call(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};
template<typename R, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
struct FFFunction<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
    typedef FuncBase<R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>                FuncBaseDef;
    typedef typename VoidUtil<R>::RET_TYPE   Ret;
    SharedPtr<FuncBaseDef>                   func;
    
    template<typename FUNC>
    FFFunction(FUNC f){
        func = new FuncImpl<FUNC, R(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>(f);
    }
    Ret operator()(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9){
        return func->call(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    operator bool() const
    {
        return func;
    }
    FFFunction(){}
};

template<typename FUNC>
class FuncImpl<FUNC, void()>:public FuncBase<void()>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(){
        func();
        return VoidRet::val();
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1>
class FuncImpl<FUNC, void(ARG1)>:public FuncBase<void(ARG1)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1){
        func(arg1);
        return VoidRet::val();
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2>
class FuncImpl<FUNC, void(ARG1, ARG2)>:
        public FuncBase<void(ARG1, ARG2)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2){ 
        func(arg1, arg2);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3)>:
        public FuncBase<void(ARG1, ARG2, ARG3)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3){ 
        func(arg1, arg2, arg3);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ 
        func(arg1, arg2, arg3, arg4);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4, ARG5)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4, ARG5)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ 
        func(arg1, arg2, arg3, arg4, arg5);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ 
        func(arg1, arg2, arg3, arg4, arg5, arg6);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7){ 
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8){ 
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        return VoidRet::val(); 
    }
    FUNC    func;
};
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class FuncImpl<FUNC, void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>:
        public FuncBase<void(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual VoidVal call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9){ 
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
        return VoidRet::val();
    }
    FUNC    func;
};
//--------------下面有返回值的
template<typename FUNC, typename RET>
class FuncImpl<FUNC, RET()>:public FuncBase<RET()>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(){
        return func();
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1>
class FuncImpl<FUNC, RET(ARG1)>:public FuncBase<RET(ARG1)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1){
        return func(arg1);
        
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2>
class FuncImpl<FUNC, RET(ARG1, ARG2)>:
        public FuncBase<RET(ARG1, ARG2)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2){ 
        return func(arg1, arg2);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3)>:
        public FuncBase<RET(ARG1, ARG2, ARG3)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3){ 
        return func(arg1, arg2, arg3);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4){ 
        return func(arg1, arg2, arg3, arg4);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4, ARG5)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4, ARG5)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5){ 
        return func(arg1, arg2, arg3, arg4, arg5);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6){ 
        return func(arg1, arg2, arg3, arg4, arg5, arg6);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7){ 
        return func(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8){ 
        return func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
         
    }
    FUNC    func;
};
template<typename FUNC, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
         typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class FuncImpl<FUNC, RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>:
        public FuncBase<RET(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
public:
    FuncImpl(FUNC f):func(f){}
    virtual RET call(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9){ 
        return func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    FUNC    func;
};
}

#endif
