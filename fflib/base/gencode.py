# -*- coding:utf-8 -*-
MAX_NUM = 6
def genCallUtil():
    code = ''
    code += 'template <typename RET>                                 \n'
    code += 'struct CallUtil                                         \n'
    code += '{                                                       \n'
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表
        funcArgTypeAndname= ''
        funcArgVarNamesList= ''
        for argNumIndex in range(1, maxArgNum + 1):
            if argNumIndex > 1:
                funcArgVarNamesList+= ', '
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += ', ARG%d'%(argNumIndex)
            funcArgTypeAndname+= ', ARG%d arg%d'%(argNumIndex, argNumIndex)
            funcArgVarNamesList+= 'arg%d'%(argNumIndex)
            
        code += '    template <typename FUNC%s>                          \n'%(typeNameAndArgList)
        code += '    static RET call(FUNC f%s){ return f(%s); }          \n'%(funcArgTypeAndname, funcArgVarNamesList)
        code += '    template <typename CLASS_TYPE, typename FUNC%s>                          \n'%(typeNameAndArgList)
        code += '    static RET callMethod(FUNC f, CLASS_TYPE* p%s){ return (p->*(f))(%s); }          \n'%(funcArgTypeAndname, funcArgVarNamesList)
    code += '};                                                          \n'
    
    code += 'template <>                                                 \n'
    code += 'struct CallUtil<void>                                       \n'
    code += '{                                                           \n'
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表
        funcArgTypeAndname= ''
        funcArgVarNamesList= ''
        for argNumIndex in range(1, maxArgNum + 1):
            if argNumIndex > 1:
                funcArgVarNamesList+= ', '
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += ', ARG%d'%(argNumIndex)
            funcArgTypeAndname+= ', ARG%d arg%d'%(argNumIndex, argNumIndex)
            funcArgVarNamesList+= 'arg%d'%(argNumIndex)
            
        code += '    template <typename FUNC%s>                          \n'%(typeNameAndArgList)
        code += '    static int call(FUNC f%s){ f(%s); return 0;}        \n'%(funcArgTypeAndname, funcArgVarNamesList)
        code += '    template <typename CLASS_TYPE, typename FUNC%s>                          \n'%(typeNameAndArgList)
        code += '    static int callMethod(FUNC f, CLASS_TYPE* p%s){ (p->*(f))(%s);return 0; }          \n'%(funcArgTypeAndname, funcArgVarNamesList)
    code += '};                                                          \n'
    return code        
def genFunctionCode():
    code = ''
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表
        funcArgTypeAndname= ''
        funcArgVarNamesList= ''
        for argNumIndex in range(1, maxArgNum + 1):
            if argNumIndex > 1:
                funcArgTypeAndname+= ', '
                funcArgVarNamesList+= ', '
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += ', ARG%d'%(argNumIndex)
            funcArgTypeAndname+= 'ARG%d arg%d'%(argNumIndex, argNumIndex)
            funcArgVarNamesList+= 'arg%d'%(argNumIndex)
        callerArgsList = ''
        if funcArgVarNamesList:
            callerArgsList = ', ' + funcArgVarNamesList
        code += 'template<typename RET%s>                                                               \n'%(typeNameAndArgList)
        code += 'class IFuncArg%d                                                                       \n'%(maxArgNum)
        code += '{                                                                                      \n'
        code += 'public:                                                                                \n'
        code += '    virtual ~IFuncArg%d() {}                                                           \n'%(maxArgNum)
        code += '    virtual typename RetTraits<RET>::RET_TYPE call(%s) = 0;                            \n'%(funcArgTypeAndname)
        code += '    virtual IFuncArg%d<RET%s>* copy() = 0;                                             \n'%(maxArgNum, funcSrcTypeArgList)
        code += '};                                                                                     \n'
        code += 'template<typename T, typename RET%s>                                                   \n'%(typeNameAndArgList)
        code += 'class ImplFuncArg%d : public IFuncArg%d<RET%s>                                         \n'%(maxArgNum, maxArgNum, funcSrcTypeArgList)
        code += '{                                                                                      \n'
        code += 'public:                                                                                \n'
        code += '    ImplFuncArg%d(const T& t) : func(t) {}                                             \n'%(maxArgNum)
        code += '    typename RetTraits<RET>::RET_TYPE call(%s) {                                       \n'%(funcArgTypeAndname)
        code += '        return CallUtil<RET>::call(func%s);                                            \n'%(callerArgsList)
        code += '    }                                                                                  \n'
        code += '    virtual IFuncArg%d<RET%s>* copy() {                                                \n'%(maxArgNum, funcSrcTypeArgList)
        code += '        return new ImplFuncArg%d<T, RET%s>(func);                                      \n'%(maxArgNum, funcSrcTypeArgList)
        code += '    }                                                                                  \n'
        code += '    T func;                                                                            \n'
        code += '};                                                                                     \n'
        code += 'template<typename RET%s>                                                               \n'%(typeNameAndArgList)
        code += 'class Function<RET(%s)>                                                                \n'%(funcSrcTypeArgList[2:])
        code += '{                                                                                      \n'
        code += 'public:                                                                                \n'
        code += '    Function(): pFunc(NULL) {                                                          \n'
        code += '    }                                                                                  \n'
        code += '    template<typename T>                                                               \n'
        code += '    Function(const T& t): pFunc(NULL) {                                               \n'
        code += '        this->assign(t);                                                               \n'
        code += '    }                                                                                  \n'
        code += '    ~Function() {                                                                      \n'
        code += '        if (pFunc) {                                                                   \n'
        code += '            delete  pFunc;                                                             \n'
        code += '            pFunc = NULL;                                                              \n'
        code += '        }                                                                              \n'
        code += '    }                                                                                  \n'
        code += '    Function(const Function<RET(%s)>& t): pFunc(NULL) {                                             \n'%(funcSrcTypeArgList[2:])
        code += '        if (pFunc) {                                                                   \n'
        code += '            delete  pFunc;                                                             \n'
        code += '            pFunc = NULL;                                                              \n'
        code += '        }                                                                              \n'
        code += '        if (t.pFunc) {                                                                 \n'
        code += '            pFunc = t.pFunc->copy();                                                   \n'
        code += '        }                                                                              \n'
        code += '    }                                                                                  \n'
        code += '    void operator=(const Function<RET(%s)>& t) {                                       \n'%(funcSrcTypeArgList[2:])
        code += '        if (pFunc) {                                                                   \n'
        code += '            delete  pFunc;                                                             \n'
        code += '            pFunc = NULL;                                                              \n'
        code += '        }                                                                              \n'
        code += '        if (t.pFunc) {                                                                 \n'
        code += '            pFunc = t.pFunc->copy();                                                   \n'
        code += '        }                                                                              \n'
        code += '    }                                                                                  \n'
        code += '    template<typename T>                                                               \n'
        code += '    void operator=(const T& t) {                                                       \n'
        code += '        this->assign(t);                                                               \n'
        code += '    }                                                                                  \n'
        code += '    template<typename T>                                                               \n'
        code += '    void assign(const T& t) {                                                          \n'
        code += '        if (pFunc) {                                                                   \n'
        code += '            delete  pFunc;                                                             \n'
        code += '            pFunc = NULL;                                                              \n'
        code += '        }                                                                              \n'
        code += '        pFunc = new ImplFuncArg%d<T, RET%s>(t);                                        \n'%(maxArgNum, funcSrcTypeArgList)
        code += '    }                                                                                  \n'
        code += '    typename RetTraits<RET>::RET_TYPE operator()(%s)                                                                 \n'%(funcArgTypeAndname)
        code += '    {                                                                                  \n'
        code += '        if (!pFunc) throw std::runtime_error("null functiorn canott be invoked");      \n'
        code += '        return pFunc->call(%s);                                                        \n'%(funcArgVarNamesList)
        code += '    }                                                                                  \n'
        code += '    operator bool() const                                                              \n'
        code += '    {                                                                                  \n'
        code += '        return NULL != pFunc;                                                          \n'
        code += '    }                                                                                  \n'
        code += 'protected:                                                                             \n'
        code += '    IFuncArg%d<RET%s>* pFunc;                                                          \n'%(maxArgNum, funcSrcTypeArgList)
        code += '};                                                                                     \n'
        
    return code
def genFunctorTmp():
    code = ''
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表
        funcArgVarNamesList= ''
        for argNumIndex in range(1, maxArgNum + 1):
            if argNumIndex > 1:
                funcSrcTypeArgList += ', '
                #funcArgVarNamesList+= ', '
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += 'ARG%d'%(argNumIndex)
            funcArgVarNamesList+= ', arg%d'%(argNumIndex)
        funcCtorArgNum = 0#构造函数的参数数量
        for funcCtorArgNum in range(maxArgNum+1):
            funcName = 'FunctorTmp%d_%d'%(maxArgNum, funcCtorArgNum)
            callArgsList = ''
            ctorArgsList = ''
            ctorArgsInitList = ''
            memberVarDeclairList = ''
            for i in range(funcCtorArgNum):
                ctorInitArgIndex = i+1
                ctorArgsList += ', ARG%d a%d'%(ctorInitArgIndex, ctorInitArgIndex)
                ctorArgsInitList+= ', arg%d(a%d)'%(ctorInitArgIndex, ctorInitArgIndex)
                memberVarDeclairList += '    typename RefTraits<ARG%d>::REAL_TYPE arg%d;\n'%(ctorInitArgIndex, ctorInitArgIndex)
            for callArgIndex in range(funcCtorArgNum+1, maxArgNum + 1):
                if callArgsList:
                    callArgsList += ', '
                callArgsList += 'ARG%d arg%d'%(callArgIndex, callArgIndex)
            code += 'template <typename RET%s>                             \n'%(typeNameAndArgList)
            code += 'struct %s                                              \n'%(funcName)
            code += '{                                                      \n'
            code += '    %s(RET(*f)(%s)%s): func(f) %s {}                    \n'%(funcName, funcSrcTypeArgList, ctorArgsList, ctorArgsInitList)
            code += '    typename RetTraits<RET>::RET_TYPE operator()(%s)                                 \n'%(callArgsList)
            code += '    {                                                  \n'
            code += '        return CallUtil<RET>::call(func%s);                               \n'%(funcArgVarNamesList)
            code += '    }                                                  \n'
            code += '    RET(*func)(%s);                                    \n'%(funcSrcTypeArgList)
            code += memberVarDeclairList
            code += '};                                                     \n'
    return code
def genFunctorBind():
    code = ''
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表

        for argNumIndex in range(1, maxArgNum + 1):
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += ', ARG%d'%(argNumIndex)
        funcCtorArgNum = 0#构造函数的参数数量
        for funcCtorArgNum in range(maxArgNum+1):
            funcNameRet = 'FunctorTmp%d_%d'%(maxArgNum, funcCtorArgNum)
            ctorArgsList = ''
            ctorArgsInitList = ''
            for i in range(funcCtorArgNum):
                ctorInitArgIndex = i+1
                ctorArgsList += ', typename RefTraits<ARG%d>::REAL_TYPE arg%d'%(ctorInitArgIndex, ctorInitArgIndex)
                ctorArgsInitList+= ', arg%d'%(ctorInitArgIndex)

            code += 'template <typename RET%s>                            \n'%(typeNameAndArgList)
            code += '%s<RET%s> funcbind(RET(*f)(%s)%s)         \n'%(funcNameRet, funcSrcTypeArgList, funcSrcTypeArgList[2:], ctorArgsList)
            code += '{                                                     \n'
            code += '    %s<RET%s> ret(f%s);                  \n'%(funcNameRet, funcSrcTypeArgList, ctorArgsInitList)
            code += '    return ret;                                       \n'
            code += '}                                                     \n'
    return code
def genClassFunctorTmp():
    code = ''
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表
        funcArgVarNamesList= ''
        for argNumIndex in range(1, maxArgNum + 1):
            if argNumIndex > 1:
                funcSrcTypeArgList += ', '
                #funcArgVarNamesList+= ', '
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += 'ARG%d'%(argNumIndex)
            funcArgVarNamesList+= ', arg%d'%(argNumIndex)
        funcCtorArgNum = 0#构造函数的参数数量
        for funcCtorArgNum in range(maxArgNum+1):
            funcName = 'ClassFunctorTmp%d_%d'%(maxArgNum, funcCtorArgNum)
            callArgsList = ''
            ctorArgsList = ''
            ctorArgsInitList = ''
            memberVarDeclairList = ''
            for i in range(funcCtorArgNum):
                ctorInitArgIndex = i+1
                ctorArgsList += ', ARG%d a%d'%(ctorInitArgIndex, ctorInitArgIndex)
                ctorArgsInitList+= ', arg%d(a%d)'%(ctorInitArgIndex, ctorInitArgIndex)
                memberVarDeclairList += '    typename RefTraits<ARG%d>::REAL_TYPE arg%d;\n'%(ctorInitArgIndex, ctorInitArgIndex)
            for callArgIndex in range(funcCtorArgNum+1, maxArgNum + 1):
                if callArgsList:
                    callArgsList += ', '
                callArgsList += 'ARG%d arg%d'%(callArgIndex, callArgIndex)
            code += 'template <typename CLASS_TYPE, typename OBJ_PTR, typename RET%s>         \n'%(typeNameAndArgList)
            code += 'struct %s                                              \n'%(funcName)
            code += '{                                                      \n'
            code += '    %s(RET(CLASS_TYPE::*f)(%s), OBJ_PTR p%s): func(f), ptr(p) %s {}       \n'%(funcName, funcSrcTypeArgList, ctorArgsList, ctorArgsInitList)
            code += '    typename RetTraits<RET>::RET_TYPE operator()(%s)   \n'%(callArgsList)
            code += '    {                                                  \n'
            code += '        return CallUtil<RET>::callMethod(func, ObjPtrGetter<CLASS_TYPE>::getPtr(ptr)%s);            \n'%(funcArgVarNamesList)
            code += '    }                                                  \n'
            
            code += '    RET(CLASS_TYPE::*func)(%s);                        \n'%(funcSrcTypeArgList)
            code += '    OBJ_PTR ptr;                                       \n'
            code += memberVarDeclairList
            code += '};                                                     \n'
    return code
def genClassFunctorBind():
    code = ''
    for maxArgNum in range(0, MAX_NUM+1):
        typeNameAndArgList = ''
        funcSrcTypeArgList = ''#函数原型的模板参数列表

        for argNumIndex in range(1, maxArgNum + 1):
            typeNameAndArgList += ', typename ARG%d'%(argNumIndex)
            funcSrcTypeArgList += ', ARG%d'%(argNumIndex)
        funcCtorArgNum = 0#构造函数的参数数量
        for funcCtorArgNum in range(maxArgNum+1):
            funcNameRet = 'ClassFunctorTmp%d_%d'%(maxArgNum, funcCtorArgNum)
            ctorArgsList = ''
            ctorArgsInitList = ''
            for i in range(funcCtorArgNum):
                ctorInitArgIndex = i+1
                ctorArgsList += ', typename RefTraits<ARG%d>::REAL_TYPE arg%d'%(ctorInitArgIndex, ctorInitArgIndex)
                ctorArgsInitList+= ', arg%d'%(ctorInitArgIndex)

            code += 'template <typename CLASS_TYPE, typename OBJ_PTR, typename RET%s>                            \n'%(typeNameAndArgList)
            code += '%s<CLASS_TYPE, OBJ_PTR, RET%s> funcbind(RET(CLASS_TYPE::*f)(%s), OBJ_PTR p%s)         \n'%(funcNameRet, funcSrcTypeArgList, funcSrcTypeArgList[2:], ctorArgsList)
            code += '{                                                     \n'
            code += '    %s<CLASS_TYPE, OBJ_PTR, RET%s> ret(f, p%s);                  \n'%(funcNameRet, funcSrcTypeArgList, ctorArgsInitList)
            code += '    return ret;                                       \n'
            code += '}                                                     \n'
    return code
headercode = """
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

"""
footercode = """
//!end namespace ff
}
#endif

"""
callUtil = genCallUtil()
#print(callUtil)
functionCode = genFunctionCode()
#print(functionCode)
funcCode = genFunctorTmp()
#print(funcCode)
bindCode = genFunctorBind()
#print(bindCode)
classFuncCode = genClassFunctorTmp()
classBindCode = genClassFunctorBind()
print(classBindCode)
cppCode = headercode + callUtil + functionCode + funcCode + bindCode + classFuncCode + classBindCode + footercode 
f = open("./func.h", "w")
f.write(cppCode)
f.close()


