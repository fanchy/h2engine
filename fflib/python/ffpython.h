#ifndef  _FFPYTHON_H_
#define _FFPYTHON_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>

#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdexcept>
namespace ff
{
#ifndef PyString_Check 
    #define PYTHON_3
    #define PyString_Check PyUnicode_Check
    #define PyString_AsString(pvalue_)  PyUnicode_AsUTF8(pvalue_)
    #define PyString_AsStringAndSize(pvalue_, pDestPtr, nLen)  *pDestPtr = PyUnicode_AsUTF8AndSize(pvalue_, nLen)
    #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
    #define PyString_FromString PyUnicode_FromString
#endif
#ifdef _WIN32
#define  SAFE_SPRINTF   _snprintf_s
#define PTR_NUMBER int64_t
#else
#define  SAFE_SPRINTF   snprintf
#define PTR_NUMBER int64_t
#endif
#define  PYCTOR int (*)
template<typename T>
struct ScriptCppOps;

template<typename T> struct CheckType { static bool IsScriptObject() { return false; } };
template<> struct CheckType<PyObject*> { static bool IsScriptObject() { return true; } };
struct ScriptObjRefGuard
{
    ScriptObjRefGuard(bool b, PyObject* v) :bNoDec(b),value(v){}
    ~ScriptObjRefGuard() {
        if (bNoDec == false)
            Py_XDECREF(value);
        value = NULL;
        //printf("ref dec!!!!\n");
    }
    bool bNoDec;
    PyObject* value;
};

template<typename T> struct RetTraitUtil { typedef T RET_TYPE; };
template<> struct RetTraitUtil<void> { typedef bool RET_TYPE; };
template <typename T> struct InitValueTrait { static T value() { return T(); } };
template <> struct InitValueTrait<std::string> { static const char* value() { return ""; } };
template <> struct InitValueTrait<const char*> { static const char* value() { return ""; } };

#define  RET_V typename RetTraitUtil<RET>::RET_TYPE

class ScriptIterface
{
public:
    ScriptIterface(int n = 0):pobjArg(NULL), nMinArgs(n){}
    virtual ~ScriptIterface() {}
    PyObject* arg(size_t i) {
        if (i <= tmpArgs.size())
            return tmpArgs[i - 1];
        return  Py_None;
    }
    void clearTmpArg() {
        pobjArg = NULL;
        tmpArgs.clear();
    }
    virtual PyObject* handleRun() { Py_RETURN_NONE; }
    virtual void* handleNew() { return NULL; }
    virtual void handleDel() { }
public:
    void*                   pobjArg;
    int                     nMinArgs;//!最少参数数量
    std::string             strName;
    std::vector<PyObject*>  tmpArgs;
};
template<typename T>
class ScriptFuncImpl;
template<typename CTOR>
class ScriptClassImpl;
template<typename T>
class ScriptMethodImpl;
template<typename T>
class ScriptFieldImpl;
class ScriptRawImpl :public ScriptIterface {
public:
    typedef PyObject* (*FuncType)(std::vector<PyObject*>&); FuncType func;
    ScriptRawImpl(FuncType f) :func(f) {}
    virtual PyObject* handleRun() { 
        return func(tmpArgs);
    }
};

enum FFScriptTypeDef {
    E_STATIC_FUNC  = 0,
    E_CLASS_NEW    = 1,
    E_CLASS_DEL    = 2,
    E_CLASS_METHOD = 3,
    E_CLASS_FIELD  = 4
};

class FFPython
{
public:
	FFPython();
	~FFPython();
    void addPath(const std::string& path);
	void runCode(const std::string& code);
    bool reload(const std::string& pyMod);
    bool load(const std::string& pyMod);
    
    FFPython& reg(ScriptIterface* pObj, const std::string& name, 
        int nOps, std::string nameClass = "", std::string nameInherit = "");
    template<typename T>
    FFPython& regFunc(T func, const std::string& name) {
        return reg(new ScriptFuncImpl<T>(func), name, E_STATIC_FUNC);
    }
    template<typename CTOR>
    FFPython& regClass(const std::string& nameClass, std::string nameInherit = "") {
        ScriptCppOps<typename ScriptClassImpl<CTOR>::ClassTypeReal*>::BindClassName = nameClass;
        return reg(new ScriptClassImpl<CTOR>(), nameClass, E_CLASS_NEW, nameClass, nameInherit);
    }
    template<typename T>
    FFPython& regMethod(T func, const std::string& name, std::string nameClass = "") {
        return reg(new ScriptMethodImpl<T>(func), name, E_CLASS_METHOD, nameClass);
    }
    template<typename T>
    FFPython& regField(T func, const std::string& name) {
        return reg(new ScriptFieldImpl<T>(func), name, E_CLASS_FIELD);
    }
    FFPython& regFunc(ScriptRawImpl::FuncType func, const std::string& name) {
        return reg(new ScriptRawImpl(func), name, E_STATIC_FUNC);
    }

    PyObject* callFuncByObj(PyObject* pFunc, std::vector<PyObject*>& objArgs);
    PyObject* callFunc(const std::string& modName, const std::string& funcName, std::vector<PyObject*>& objArgs);
    template<typename RET>
    bool callFuncByObj(PyObject* pFunc, std::vector<PyObject*>& objArgs, RET* pRet)
    {
        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFuncByObj(pFunc, objArgs));
        if (pRet) {
            *pRet = ScriptCppOps<RET_V>::scriptToCpp(retObj.value, *pRet);
        }
        return getErrMsg().empty();
    }
    template<typename RET>
    RET_V callFuncByObjRet(PyObject* pFunc, std::vector<PyObject*>& objArgs)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        callFuncByObj(pFunc, objArgs, &ret);
        return getErrMsg().empty();
    }
    template<typename RET>
    RET_V callMethodByObjRet(PyObject* pObj, const std::string& nameFunc, std::vector<PyObject*>& objArgs)
    {
        PyObject* pFunc = PyObject_GetAttrString(pObj, nameFunc.c_str());
        if (!pFunc) {
            Py_RETURN_NONE;
        }
        RET_V ret = InitValueTrait<RET_V>::value();
        callFuncByObj(pFunc, objArgs, &ret);
        Py_XDECREF(pFunc);
        return getErrMsg().empty();
    }
    template<typename RET>
    bool callFunc(const std::string& modName, const std::string& funcName, std::vector<PyObject*>& objArgs, RET* pRet)
    {
        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, objArgs));
        if (pRet) {
            *pRet = ScriptCppOps<RET_V>::scriptToCpp(retObj.value, *pRet);
        }
        return getErrMsg().empty();
    }
    //! 调用python函数，最多支持9个参数
    template<typename RET>
    RET_V call(const std::string& modName, const std::string& funcName
        )
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1>
    RET_V call(const std::string& modName, const std::string& funcName
        , const ARG1& arg1)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2>
    RET_V call(const std::string& modName, const std::string& funcName
        , const ARG1& arg1, const ARG2& arg2)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3>
    RET_V call(const std::string& modName, const std::string& funcName
        , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    RET_V call(const std::string& modName, const std::string& funcName
        , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
        , typename ARG5>
        RET_V call(const std::string& modName, const std::string& funcName
            , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4
            , const ARG5& arg5)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));
        args.push_back(ScriptCppOps<ARG5>::scriptFromCpp(arg5));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
        , typename ARG5, typename ARG6>
        RET_V call(const std::string& modName, const std::string& funcName
            , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4
            , const ARG5& arg5, const ARG6& arg6)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));
        args.push_back(ScriptCppOps<ARG5>::scriptFromCpp(arg5));
        args.push_back(ScriptCppOps<ARG6>::scriptFromCpp(arg6));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
        , typename ARG5, typename ARG6, typename ARG7>
        RET_V call(const std::string& modName, const std::string& funcName
            , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4
            , const ARG5& arg5, const ARG6& arg6, const ARG7& arg7)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));
        args.push_back(ScriptCppOps<ARG5>::scriptFromCpp(arg5));
        args.push_back(ScriptCppOps<ARG6>::scriptFromCpp(arg6));
        args.push_back(ScriptCppOps<ARG7>::scriptFromCpp(arg7));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
        , typename ARG5, typename ARG6, typename ARG7, typename ARG8>
        RET_V call(const std::string& modName, const std::string& funcName
            , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4
            , const ARG5& arg5, const ARG6& arg6, const ARG7& arg7, const ARG8& arg8)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));
        args.push_back(ScriptCppOps<ARG5>::scriptFromCpp(arg5));
        args.push_back(ScriptCppOps<ARG6>::scriptFromCpp(arg6));
        args.push_back(ScriptCppOps<ARG7>::scriptFromCpp(arg7));
        args.push_back(ScriptCppOps<ARG8>::scriptFromCpp(arg8));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
        , typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
        RET_V call(const std::string& modName, const std::string& funcName
            , const ARG1& arg1, const ARG2& arg2, const ARG3& arg3, const ARG4& arg4
            , const ARG5& arg5, const ARG6& arg6, const ARG7& arg7, const ARG8& arg8, const ARG9& arg9)
    {
        RET_V ret = InitValueTrait<RET_V>::value();
        std::vector<PyObject*>& args = allocArgList();
        args.push_back(ScriptCppOps<ARG1>::scriptFromCpp(arg1));
        args.push_back(ScriptCppOps<ARG2>::scriptFromCpp(arg2));
        args.push_back(ScriptCppOps<ARG3>::scriptFromCpp(arg3));
        args.push_back(ScriptCppOps<ARG4>::scriptFromCpp(arg4));
        args.push_back(ScriptCppOps<ARG5>::scriptFromCpp(arg5));
        args.push_back(ScriptCppOps<ARG6>::scriptFromCpp(arg6));
        args.push_back(ScriptCppOps<ARG7>::scriptFromCpp(arg7));
        args.push_back(ScriptCppOps<ARG8>::scriptFromCpp(arg8));
        args.push_back(ScriptCppOps<ARG9>::scriptFromCpp(arg9));

        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), callFunc(modName, funcName, args));
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
public:
    PyObject* getScriptVarByObj(PyObject* pyMod, const std::string& strVarName);
    PyObject* getScriptVar(const std::string& strMod, const std::string& strVarName);
    template<typename RET>
    RET_V getVar(PyObject* pyMod, const std::string& strVarName) {
        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), getScriptVarByObj(pyMod, strVarName));
        RET_V ret = InitValueTrait<RET_V>::value();
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename RET>
    RET_V getVar(const std::string& strMod, const std::string& strVarName) {
        ScriptObjRefGuard retObj(CheckType<RET_V>::IsScriptObject(), getScriptVar(strMod, strVarName));
        RET_V ret = InitValueTrait<RET_V>::value();
        ScriptCppOps<RET_V>::scriptToCpp(retObj.value, ret);
        return ret;
    }
    template<typename T>
    bool setVar(const std::string& strMod, const std::string& strVarName, const T& val)
    {
        PyObject* pName = NULL, * pModule = NULL;
        std::string err_msg;

        pName = PyString_FromString(strMod.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule){
            traceback(m_strErr);
            return false;
        }

        PyObject* pval = ScriptCppOps<T>::scriptFromCpp(val);;
        int ret = PyObject_SetAttrString(pModule, strVarName.c_str(), pval);
        Py_DECREF(pModule);
        return true;
    }
    const std::string& getErrMsg() { return m_strErr; }
    std::vector<PyObject*>& allocArgList() { m_listArgs.clear(); return m_listArgs; }
    static ScriptIterface* getRegFuncByID(size_t i) {
        if (i < m_regFuncs.size()) {
            return m_regFuncs[i];
        }
        return NULL;
    }
    int traceback(std::string& ret_);
    void cacheObj(PyObject* b) { m_listArgs.push_back(b); }
private:
    std::string                             m_strErr;
    std::vector<PyObject*>                  m_listArgs;
    static std::vector<ScriptIterface*>     m_regFuncs;
    std::string                             m_curRegClassName;
    std::vector<PyObject*>                  m_listGlobalCache;
public:
    static std::map<void*, ScriptIterface*> m_allocObjs;
    static PyObject*                        pyobjBuildTmpObj;
};
template<>
struct ScriptCppOps<int8_t> {
    static PyObject* scriptFromCpp(int8_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, int8_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (int8_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (int8_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (int8_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<uint8_t> {
    static PyObject* scriptFromCpp(uint8_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, uint8_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (uint8_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (uint8_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (uint8_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<int16_t> {
    static PyObject* scriptFromCpp(int16_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, int16_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (int16_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (int16_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (int16_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<uint16_t> {
    static PyObject* scriptFromCpp(uint16_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, uint16_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (uint16_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (uint16_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (uint16_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<int32_t> {
    static PyObject* scriptFromCpp(int32_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, int32_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (int32_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (int32_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (int32_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<uint32_t> {
    static PyObject* scriptFromCpp(uint32_t n) { return PyLong_FromLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, uint32_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (uint32_t)PyLong_AsLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (uint32_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (uint32_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<int64_t> {
    static PyObject* scriptFromCpp(int64_t n) { return PyLong_FromLongLong(n); }
    static bool scriptToCpp(PyObject* pvalue, int64_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (int64_t)PyLong_AsLongLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (int64_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (int64_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<>
struct ScriptCppOps<uint64_t> {
    static PyObject* scriptFromCpp(uint64_t n) { return PyLong_FromLongLong(long(n)); }
    static bool scriptToCpp(PyObject* pvalue, uint64_t& ret) {
        if (PyLong_Check(pvalue)) { ret = (uint64_t)PyLong_AsLongLong(pvalue); }
        else if (PyBool_Check(pvalue)) { ret = (Py_False == pvalue ? 0 : 1); }
        else if (PyFloat_Check(pvalue)) { ret = (uint64_t)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (uint64_t)atol(PyString_AsString(pvalue)); }
        else { ret = 0; return false; }
        return true;
    }
};
template<> struct ScriptCppOps<float> {
    static PyObject* scriptFromCpp(float n) { return PyFloat_FromDouble(float(n)); }
    static bool scriptToCpp(PyObject* pvalue, float& ret) {
        if (PyLong_Check(pvalue)) { ret = (float)PyLong_AsLong(pvalue); }
        else if (PyFloat_Check(pvalue)) { ret = (float)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (float)atof(PyString_AsString(pvalue)); }
        else { ret = 0.0; return false; }
        return true;
    }
};
template<> struct ScriptCppOps<double> {
    static PyObject* scriptFromCpp(double n) { return PyFloat_FromDouble(double(n)); }
    static bool scriptToCpp(PyObject* pvalue, double& ret) {
        if (PyLong_Check(pvalue)) { ret = (double)PyLong_AsLong(pvalue); }
        else if (PyFloat_Check(pvalue)) { ret = (double)PyFloat_AsDouble(pvalue); }
        else if (PyString_Check(pvalue)) { ret = (double)atof(PyString_AsString(pvalue)); }
        else { ret = 0.0; return false; }
        return true;
    }
};
template<> struct ScriptCppOps<bool> {
    static PyObject* scriptFromCpp(bool n) { if (n) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
    static bool scriptToCpp(PyObject* pvalue, bool& ret) {
        if (Py_False == pvalue || Py_None == pvalue) { ret = false; }
        else { ret = true; }
        return true;
    }
};
template<> struct ScriptCppOps<std::string> {
    static PyObject* scriptFromCpp(const std::string& n) { return PyUnicode_FromStringAndSize(n.c_str(), n.size()); }
    static bool scriptToCpp(PyObject* pvalue, std::string& ret) {
        if (PyLong_Check(pvalue)) { 
            char buff[64] = { 0 };
            SAFE_SPRINTF(buff, sizeof(buff), "%ld", PyLong_AsLong(pvalue));
            ret = buff;
        }
        else if (PyFloat_Check(pvalue)) { 
            char buff[64] = { 0 };
            SAFE_SPRINTF(buff, sizeof(buff), "%g", PyFloat_AsDouble(pvalue));
            ret = buff;
        }
        else if (PyString_Check(pvalue)) {
            Py_ssize_t size = 0;
#ifdef PYTHON_3
            const char* pstr = NULL;
#else
            char* pstr = NULL;
#endif
            PyString_AsStringAndSize(pvalue, (&pstr), &size);
            if (pstr)
                ret.assign(pstr, size);
        }
        else { ret.clear(); return false; }
        return true;
    }
};
template<> struct ScriptCppOps<const char*> {
    static PyObject* scriptFromCpp(const char* n) { return PyUnicode_FromString(n); }
    static bool scriptToCpp(PyObject* pvalue, const char*& ret) {
        if (PyString_Check(pvalue)) {
            ret = PyString_AsString(pvalue);
        }
        else { ret = "";  return false; }
        return true;
    }
};
template<> struct ScriptCppOps<void*> {
    static PyObject* scriptFromCpp(void* n) { return PyLong_FromVoidPtr(n); }
    static bool scriptToCpp(PyObject* pvalue, void*& ret) {
        ret = PyLong_AsVoidPtr(pvalue);
        return true;
    }
};
template<int size> struct ScriptCppOps<char[size]> {
    static PyObject* scriptFromCpp(const char* n) { return PyUnicode_FromString(n); }
};
template<> struct ScriptCppOps<PyObject*> {
    static PyObject* scriptFromCpp(PyObject* n) { return n; }
    static bool scriptToCpp(PyObject* pvalue, PyObject*& ret) {
        ret = pvalue;
        return true;
    }
};
template<typename T> struct ScriptCppOps<T*> {
    static std::string BindClassName;
    static PyObject* scriptFromCpp(T* n) {
        if (!FFPython::pyobjBuildTmpObj) {
            Py_RETURN_NONE;
        }
        PyObject* pFunc = FFPython::pyobjBuildTmpObj;
        PyObject* pValue = NULL;
        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject* pArgs = PyTuple_New(2);
            PyTuple_SetItem(pArgs, 0, ScriptCppOps<std::string>::scriptFromCpp(BindClassName));
            PyTuple_SetItem(pArgs, 1, PyLong_FromVoidPtr(n));
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
        }
        if (!pValue) {
            PyErr_Print();
            Py_RETURN_NONE;
        }
        return pValue;
    }
    static bool scriptToCpp(PyObject* pyobjVal, T*& ret) {
        PyObject* pValue = PyObject_GetAttrString(pyobjVal, "_cppInterObj_");
        if (pValue) {
            ret = (T*)PyLong_AsVoidPtr(pValue);
            Py_DECREF(pValue);
        }
        return true;
    }
};
template<typename T> std::string ScriptCppOps<T*>::BindClassName;
template<typename T>
struct ScriptCppOps<std::vector<T> >
{
    static PyObject* scriptFromCpp(const std::vector<T>& argVal)
    {
        PyObject* ret = PyList_New(argVal.size());
        for (size_t i = 0; i < argVal.size(); ++i)
        {
            PyList_SetItem(ret, i, ScriptCppOps<T>::scriptFromCpp(argVal[i]));
        }
        return ret;
    }
    static bool scriptToCpp(PyObject* pyobjVal, std::vector<T>& retVal)
    {
        retVal.clear();
        if (true == PyTuple_Check(pyobjVal))
        {
            Py_ssize_t n = PyTuple_Size(pyobjVal);
            for (Py_ssize_t i = 0; i < n; ++i)
            {
                retVal.push_back(T());
                ScriptCppOps<T>::scriptToCpp(PyTuple_GetItem(pyobjVal, i), retVal.back());
            }
        }
        else if (true == PyList_Check(pyobjVal))
        {
            Py_ssize_t n = PyList_Size(pyobjVal);
            for (Py_ssize_t i = 0; i < n; ++i)
            {
                retVal.push_back(T());
                ScriptCppOps<T>::scriptToCpp(PyList_GetItem(pyobjVal, i), retVal.back());
            }
        }
        else
            return false;
        return true;
    }
};
template<typename T>
struct ScriptCppOps<std::list<T> >
{
    static PyObject* scriptFromCpp(const std::list<T>& argVal)
    {
        PyObject* ret = PyList_New(argVal.size());
        int i = 0;
        for (typename std::list<T>::const_iterator it = argVal.begin(); it != argVal.end(); ++it)
        {
            PyList_SetItem(ret, i++, ScriptCppOps<T>::scriptFromCpp(*it));
        }
        return ret;
    }
    static bool scriptToCpp(PyObject* pyobjVal, std::list<T>& retVal)
    {
        retVal.clear();
        if (true == PyTuple_Check(pyobjVal))
        {
            Py_ssize_t n = PyTuple_Size(pyobjVal);
            for (Py_ssize_t i = 0; i < n; ++i)
            {
                retVal.push_back(T());
                ScriptCppOps<T>::scriptToCpp(PyTuple_GetItem(pyobjVal, i), retVal.back());
            }
        }
        else if (true == PyList_Check(pyobjVal))
        {
            Py_ssize_t n = PyList_Size(pyobjVal);
            for (Py_ssize_t i = 0; i < n; ++i)
            {
                retVal.push_back(T());
                ScriptCppOps<T>::scriptToCpp(PyList_GetItem(pyobjVal, i), retVal.back());
            }
        }
        else
            return false;
        return true;
    }
};
template<typename T>
struct ScriptCppOps<std::set<T> >
{
    static PyObject* scriptFromCpp(const std::set<T>& argVal)
    {
        PyObject* ret = PySet_New(NULL);
        for (typename std::set<T>::const_iterator it = argVal.begin(); it != argVal.end(); ++it)
        {
            PyObject* v = ScriptCppOps<T>::scriptFromCpp(*it);
            PySet_Add(ret, v);
            Py_DECREF(v);
        }
        return ret;
    }
    static bool scriptToCpp(PyObject* pyobjVal, std::set<T>& retVal)
    {
        retVal.clear();
        PyObject* iter = PyObject_GetIter(pyobjVal);
        PyObject* item = NULL;
        while (NULL != iter && NULL != (item = PyIter_Next(iter)))
        {
            T tmp();
            if (ScriptCppOps<T>::scriptToCpp(item, tmp))
            {
                Py_DECREF(item);
                Py_DECREF(iter);
                return false;
            }
            retVal.insert(tmp);
            Py_DECREF(item);
        }
        if (iter)
        {
            Py_DECREF(iter);
        }
        return true;
    }
};
template<typename T, typename R>
struct ScriptCppOps<std::map<T, R> >
{
    static PyObject* scriptFromCpp(const std::map<T, R>& argVal)
    {
        PyObject* ret = PyDict_New();
        for (typename std::map<T, R>::const_iterator it = argVal.begin(); it != argVal.end(); ++it)
        {
            PyObject* k = ScriptCppOps<T>::scriptFromCpp(it->first);
            PyObject* v = ScriptCppOps<R>::scriptFromCpp(it->second);
            PyDict_SetItem(ret, k, v);
            Py_DECREF(k);
            Py_DECREF(v);
        }
        return ret;
    }
    static bool scriptToCpp(PyObject* pyobjVal, std::map<T, R>& retVal)
    {
        retVal.clear();
        if (true == PyDict_Check(pyobjVal))
        {
            PyObject* key = NULL, * value = NULL;
            Py_ssize_t pos = 0;

            while (PyDict_Next(pyobjVal, &pos, &key, &value))
            {
                T arg1 = InitValueTrait<T>::value();
                R arg2 = InitValueTrait<R>::value();
                ScriptCppOps<T>::scriptToCpp(key, arg1);
                ScriptCppOps<R>::scriptToCpp(value, arg2);
                retVal[arg1] = arg2;
            }
        }
        return true;
    }
};
template <typename T> struct ScriptRefTraits { typedef T RealType; };
template <typename T> struct ScriptRefTraits<const T> { typedef T RealType; };
template <typename T> struct ScriptRefTraits<const T&> { typedef T RealType; };
template <typename T> struct ScriptRefTraits<T&> { typedef T RealType; };
//!带返回值函数开始
template <typename RET >
class ScriptFuncImpl<RET(*)()> : public ScriptIterface {
public:
    typedef RET(*FuncType)(); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(0), func(f) {}
    virtual PyObject* handleRun() {
        return ScriptCppOps<RET>::scriptFromCpp(func());
    }
};
template <typename RET, typename ARG1>
class ScriptFuncImpl<RET(*)(ARG1)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(1), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1));
    }
};
template <typename RET, typename ARG1, typename ARG2>
class ScriptFuncImpl<RET(*)(ARG1, ARG2)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(2), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(3), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(4), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4, ARG5)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(5), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4, arg5));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(6), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4, arg5, arg6));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(7), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(8), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class ScriptFuncImpl<RET(*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)> : public ScriptIterface {
public:
    typedef RET(*FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(9), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        typename ScriptRefTraits<ARG9>::RealType arg9 = InitValueTrait<typename ScriptRefTraits<ARG9>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        ScriptCppOps<typename ScriptRefTraits<ARG9>::RealType>::scriptToCpp(arg(9), arg9);
        return ScriptCppOps<RET>::scriptFromCpp(func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
    }
};
//!带返回值函数结束
//!void函数开始
template <>
class ScriptFuncImpl<void(*)()> : public ScriptIterface{
public:
    typedef void(*FuncType)(); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(0), func(f) {}
    virtual PyObject* handleRun() {
        func();
        Py_RETURN_NONE;
    }
};
template < typename ARG1>
class ScriptFuncImpl<void(*)(ARG1)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(1), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        func(arg1);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2>
class ScriptFuncImpl<void(*)(ARG1,ARG2)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(2), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        func(arg1,arg2);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(3), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        func(arg1,arg2,arg3);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(4), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        func(arg1,arg2,arg3,arg4);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4,ARG5)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4,ARG5); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(5), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        func(arg1,arg2,arg3,arg4,arg5);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(6), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        func(arg1,arg2,arg3,arg4,arg5,arg6);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(7), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        func(arg1,arg2,arg3,arg4,arg5,arg6,arg7);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(8), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        func(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
        Py_RETURN_NONE;
    }
};
template < typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class ScriptFuncImpl<void(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9)> : public ScriptIterface{
public:
    typedef void(*FuncType)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9); FuncType func;
    ScriptFuncImpl(FuncType f) :ScriptIterface(9), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        typename ScriptRefTraits<ARG9>::RealType arg9 = InitValueTrait<typename ScriptRefTraits<ARG9>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        ScriptCppOps<typename ScriptRefTraits<ARG9>::RealType>::scriptToCpp(arg(9), arg9);
        func(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9);
        Py_RETURN_NONE;
    }
};
//!void 构造函数结束
//!构造函数
template<typename CLASS_TYPE>
class ScriptClassImpl<CLASS_TYPE()> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(0) {}
    virtual void* handleNew() {
        return new CLASS_TYPE();
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1>
class ScriptClassImpl<CLASS_TYPE(ARG1)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(1) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        return new CLASS_TYPE(arg1);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2>
class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(2) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        return new CLASS_TYPE(arg1, arg2);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3>
class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(3) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        return new CLASS_TYPE(arg1, arg2, arg3);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(4) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5>
    class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4, ARG5)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(5) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4, arg5);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6>
    class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(6) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7>
    class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(7) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(8) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
template<typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4
    , typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    class ScriptClassImpl<CLASS_TYPE(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)> :public ScriptIterface
{
public:
    typedef CLASS_TYPE ClassTypeReal;
    ScriptClassImpl() :ScriptIterface(9) {}
    virtual void* handleNew() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        typename ScriptRefTraits<ARG9>::RealType arg9 = InitValueTrait<typename ScriptRefTraits<ARG9>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        ScriptCppOps<typename ScriptRefTraits<ARG9>::RealType>::scriptToCpp(arg(9), arg9);
        return new CLASS_TYPE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    virtual void handleDel() {
        delete (CLASS_TYPE*)(this->pobjArg);
    }
};
//!构造函数结束
//!带返回值类方法开始
template <typename CLASS_TYPE, typename RET >
class ScriptMethodImpl<RET(CLASS_TYPE::*)()> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(0), func(f) {}
    virtual PyObject* handleRun() {

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)());
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(1), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(2), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(3), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(4), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(5), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(6), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(7), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(8), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    }
};
template <typename CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class ScriptMethodImpl<RET(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(9), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        typename ScriptRefTraits<ARG9>::RealType arg9 = InitValueTrait<typename ScriptRefTraits<ARG9>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        ScriptCppOps<typename ScriptRefTraits<ARG9>::RealType>::scriptToCpp(arg(9), arg9);

        return ScriptCppOps<RET>::scriptFromCpp(((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
    }
};
//!带返回值类方法结束
template <typename CLASS_TYPE>
class ScriptMethodImpl<void(CLASS_TYPE::*)()> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(0), func(f) {}
    virtual PyObject* handleRun() {

        ((CLASS_TYPE*)pobjArg->*func)();
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(1), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);

        ((CLASS_TYPE*)pobjArg->*func)(arg1);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(2), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(3), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(4), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(5), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(6), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(7), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(8), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        Py_RETURN_NONE;
    }
};
template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
class ScriptMethodImpl<void(CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)> : public ScriptIterface {
public:
    typedef void(CLASS_TYPE::* FuncType)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9); FuncType func;
    ScriptMethodImpl(FuncType f) :ScriptIterface(9), func(f) {}
    virtual PyObject* handleRun() {
        typename ScriptRefTraits<ARG1>::RealType arg1 = InitValueTrait<typename ScriptRefTraits<ARG1>::RealType>::value();
        typename ScriptRefTraits<ARG2>::RealType arg2 = InitValueTrait<typename ScriptRefTraits<ARG2>::RealType>::value();
        typename ScriptRefTraits<ARG3>::RealType arg3 = InitValueTrait<typename ScriptRefTraits<ARG3>::RealType>::value();
        typename ScriptRefTraits<ARG4>::RealType arg4 = InitValueTrait<typename ScriptRefTraits<ARG4>::RealType>::value();
        typename ScriptRefTraits<ARG5>::RealType arg5 = InitValueTrait<typename ScriptRefTraits<ARG5>::RealType>::value();
        typename ScriptRefTraits<ARG6>::RealType arg6 = InitValueTrait<typename ScriptRefTraits<ARG6>::RealType>::value();
        typename ScriptRefTraits<ARG7>::RealType arg7 = InitValueTrait<typename ScriptRefTraits<ARG7>::RealType>::value();
        typename ScriptRefTraits<ARG8>::RealType arg8 = InitValueTrait<typename ScriptRefTraits<ARG8>::RealType>::value();
        typename ScriptRefTraits<ARG9>::RealType arg9 = InitValueTrait<typename ScriptRefTraits<ARG9>::RealType>::value();
        ScriptCppOps<typename ScriptRefTraits<ARG1>::RealType>::scriptToCpp(arg(1), arg1);
        ScriptCppOps<typename ScriptRefTraits<ARG2>::RealType>::scriptToCpp(arg(2), arg2);
        ScriptCppOps<typename ScriptRefTraits<ARG3>::RealType>::scriptToCpp(arg(3), arg3);
        ScriptCppOps<typename ScriptRefTraits<ARG4>::RealType>::scriptToCpp(arg(4), arg4);
        ScriptCppOps<typename ScriptRefTraits<ARG5>::RealType>::scriptToCpp(arg(5), arg5);
        ScriptCppOps<typename ScriptRefTraits<ARG6>::RealType>::scriptToCpp(arg(6), arg6);
        ScriptCppOps<typename ScriptRefTraits<ARG7>::RealType>::scriptToCpp(arg(7), arg7);
        ScriptCppOps<typename ScriptRefTraits<ARG8>::RealType>::scriptToCpp(arg(8), arg8);
        ScriptCppOps<typename ScriptRefTraits<ARG9>::RealType>::scriptToCpp(arg(9), arg9);

        ((CLASS_TYPE*)pobjArg->*func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
        Py_RETURN_NONE;
    }
};
//!void类方法结束
//!类字段开始
template <typename CLASS_TYPE, typename RET>
class ScriptFieldImpl<RET(CLASS_TYPE::*)> : public ScriptIterface {
public:
    typedef RET(CLASS_TYPE::* FieldType); FieldType field;
    ScriptFieldImpl(FieldType f) :ScriptIterface(0), field(f) {}
    virtual PyObject* handleRun() {
        if (tmpArgs.empty()) //!沒有参数就是取值，有参数就是赋值
            return ScriptCppOps<RET>::scriptFromCpp((CLASS_TYPE*)pobjArg->*field);
        ScriptCppOps<RET>::scriptToCpp(arg(1), (CLASS_TYPE*)pobjArg->*field);
        Py_RETURN_NONE;
    }
};
//!类字段结束
}//end namespace ff
#endif // ! _FFPYTHON_H_


