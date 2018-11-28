#ifndef __FFPYTHON_H_
#define __FFPYTHON_H_
#ifdef _WIN32
#define WIN_GCC
#endif
#ifdef WIN_GCC
#include <C:\Python27\include\Python.h>
#include <C:\Python27\include\structmember.h>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#else
#include <Python.h>
#include <structmember.h>
#endif

#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdexcept>
using namespace std;
#ifdef WIN_GCC
#define  SAFE_SPRINTF   snprintf
#elif _WIN32
#define  SAFE_SPRINTF   _snprintf_s
#else
#define  SAFE_SPRINTF   snprintf
#endif

template <typename T>
struct init_value_traits_t;

template <typename T>
struct init_value_traits_t
{
    inline static T value(){ return T(); }
};

template <typename T>
struct init_value_traits_t<const T*>
{
    inline static T* value(){ return NULL; }
};
template <>
struct init_value_traits_t<PyObject*>
{
    inline static PyObject* value(){ return NULL; }
};
template <typename T>
struct init_value_traits_t<const T&>
{
    inline static T value(){ return T(); }
};

template <>
struct init_value_traits_t<std::string>
{
    inline static const char* value(){ return ""; }
};

template <>
struct init_value_traits_t<const std::string&>
{
    inline static const char* value(){ return ""; }
};
//! 获取python异常信息
struct pyops_t
{
    static int traceback(string& ret_)
    {
        PyObject* err = PyErr_Occurred();

        if (err != NULL) {
            PyObject *ptype = NULL, *pvalue = NULL, *ptraceback = NULL;
            PyObject *pyth_module = NULL, *pyth_func = NULL;
            
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue)
            {
                if (true == PyList_Check(pvalue))
                {
                    int n = PyList_Size(pvalue);
                    for (int i = 0; i < n; ++i)
                    {
                        PyObject *pystr = PyObject_Str(PyList_GetItem(pvalue, i));
                        ret_ += PyString_AsString(pystr);
                        ret_ += "\n";
                        Py_DECREF(pystr);
                    }
                }
                if (true == PyTuple_Check(pvalue))
                {
                    int n = PyTuple_Size(pvalue);
                    for (int i = 0; i < n; ++i)
                    {
                        PyObject* tmp_str = PyTuple_GetItem(pvalue, i);
                        if (true == PyTuple_Check(tmp_str))
                        {
                            int m = PyTuple_Size(tmp_str);
                            for (int j = 0; j < m; ++j)
                            {
                                PyObject *pystr = PyObject_Str(PyTuple_GetItem(tmp_str, j));
                                ret_ += PyString_AsString(pystr);
                                ret_ += ",";
                                Py_DECREF(pystr);
                            }
                        }
                        else
                        {
                            PyObject *pystr = PyObject_Str(tmp_str);
                            ret_ += PyString_AsString(pystr);
                            Py_DECREF(pystr);
                        }
                        ret_ += "\n";
                    }
                }
                else
                {
                    PyObject *pystr = PyObject_Str(pvalue);
                    if (pystr)
                    {
                        ret_ += PyString_AsString(pystr);
                        ret_ += "\n";
                        Py_DECREF(pystr);
                    }
                }
            }
            
            /* See if we can get a full traceback */
            PyObject *module_name = PyString_FromString("traceback");
            pyth_module = PyImport_Import(module_name);
            Py_DECREF(module_name);

            if (pyth_module && ptype && pvalue && ptraceback)
            {
                pyth_func = PyObject_GetAttrString(pyth_module, "format_exception");
                if (pyth_func && PyCallable_Check(pyth_func)) {
                    PyObject *pyth_val = PyObject_CallFunctionObjArgs(pyth_func, ptype, pvalue, ptraceback, NULL);
                    if (pyth_val && true == PyList_Check(pyth_val))
                    {
                        int n = PyList_Size(pyth_val);
                        for (int i = 0; i < n; ++i)
                        {
                            PyObject* tmp_str = PyList_GetItem(pyth_val, i);
                            PyObject *pystr = PyObject_Str(tmp_str);
                            if (pystr)
                            {
                                ret_ += PyString_AsString(pystr);
      
                                Py_DECREF(pystr);
                            }
                            ret_ += "\n";
                        }
                    }
                    Py_XDECREF(pyth_val);
                }
            }
            Py_XDECREF(pyth_func);
            Py_XDECREF(pyth_module);
            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
            PyErr_Clear();
            return 0;
        }

        return -1;
    }

};
struct cpp_void_t{};

//! 用于抽取类型、类型对应的引用
template<typename T>
struct type_ref_traits_t;

//! 用于python 可选参数
template<typename T>
struct pyoption_t
{
        typedef typename type_ref_traits_t<T>::value_t value_t;
        pyoption_t():m_set_flag(false){}
        bool is_set() const { return m_set_flag;}
        void set()                        { m_set_flag = true;}
        value_t&       value()    { return m_value;}
        const value_t& value() const{ return m_value;}
        
        const value_t& value(const value_t& default_)
        {
            if (is_set())
                return m_value;
            else
                return default_;
        }
        bool        m_set_flag;
        value_t m_value;
};
//! 用于判断是否是可选参数
template<typename T>
struct pyoption_traits_t;

//! pytype_traits_t 封装 PyLong_FromLong 相关的操作，用于为调用python生成参数
template<typename T>
struct pytype_traits_t;

//! 用于调用python函数，生成tuple类型的python函数参数的工具类
struct pycall_arg_t
{
    pycall_arg_t(int arg_num):
        arg_index(0),
        pargs_tuple(PyTuple_New(arg_num))
    {}
    ~pycall_arg_t()
    {
        release();
    }
    PyObject * get_args() const
    {
        return pargs_tuple;
    }
    template<typename T>
    pycall_arg_t& add(const T& val_)
    {
        if (arg_index < PyTuple_Size(pargs_tuple))
        {
            PyObject* tmp_arg = pytype_traits_t<T>::pyobj_from_cppobj(val_);
            PyTuple_SetItem(pargs_tuple, arg_index, tmp_arg);
            ++arg_index;
        }
        return *this;
    }
    void release()
    {
        if (pargs_tuple)
        {
            Py_DECREF(pargs_tuple);
            pargs_tuple = NULL;
        }
    }
    int         arg_index;
    PyObject *  pargs_tuple;
};

//! 用于调用python函数，获取返回值的工具类
class pytype_tool_t
{
public:
    virtual ~pytype_tool_t(){};
    virtual int parse_value(PyObject *pvalue_) = 0;
    virtual const char* return_type() {return "";}
};

//! 用于调用python函数，获取返回值的工具泛型类
template<typename T>
class pytype_tool_impl_t;
//! 封装调用python函数的C API
struct pycall_t
{
    static int call_func(PyObject *pModule, const string& mod_name_, const string& func_name_,
                         pycall_arg_t& pyarg_, pytype_tool_t& pyret_, string& err_)
    
    //! 封装调用python函数
    {
        PyObject *pFunc = PyObject_GetAttrString(pModule, func_name_.c_str());
        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = pyarg_.get_args();
            PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
            pyarg_.release();//! 等价于Py_DECREF(pArgs);

            if (pValue != NULL) {
                if (pyret_.parse_value(pValue))
                {
                    err_ += "value returned is not ";
                    err_ += pyret_.return_type();
                    err_ += string(" ") + func_name_  + " in " + mod_name_;
                }
                Py_DECREF(pValue);
            }
        }
        else
        {
            err_ += "Cannot find function ";
            err_ += func_name_ + " in " + mod_name_ + ",";
        }

        Py_XDECREF(pFunc);
        if (PyErr_Occurred())
        {
            pyops_t::traceback(err_);
            return 0;
        }
        return 0;
    }

    static int call_func_obj(PyObject *pFunc, pycall_arg_t& pyarg_, pytype_tool_t& pyret_, string& err_)
    {
        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = pyarg_.get_args();
            PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
            pyarg_.release();//! 等价于Py_DECREF(pArgs);

            if (pValue != NULL) {
                if (pyret_.parse_value(pValue))
                {
                    err_ += "value returned is not ";
                    err_ += pyret_.return_type();
                }
                Py_DECREF(pValue);
            }
        }
        else
        {
            err_ += "invalid function";
        }

        if (PyErr_Occurred())
        {
            pyops_t::traceback(err_);
            return 0;
        }
        return 0;
    }
    template<typename T>
    static const T& call(const string& mod_name_, const string& func_name_, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret);
    template<typename T>
    static const T& call_obj_method(PyObject *pObj, const string& func_name_, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret);
    template<typename T>
    static const T& call_lambda(PyObject *pFunc, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret);
};
//! 用于扩展python的工具类，用来解析参数
struct pyext_tool_t;


template<typename T>
struct pyext_return_tool_t;

//! 用于扩展python，生成pyobject类型的返回值给python
template <typename T>
struct pyext_func_traits_t;

//! 用于扩展python，traits出注册给python的函数接口
#ifndef PYCTOR
#define  PYCTOR int (*)
#endif
//! 表示void类型，由于void类型不能return，用void_ignore_t适配
template<typename T>
struct void_ignore_t;

template<typename T>
struct void_ignore_t
{
    typedef T value_t;
};

template<>
struct void_ignore_t<void>
{
    typedef cpp_void_t value_t;
};

#define  RET_V typename void_ignore_t<RET>::value_t

//! 记录各个基类和子类的相互关系
struct cpp_to_pyclass_reg_info_t
{
    struct inherit_info_t
    {
        inherit_info_t():pytype_def(NULL){}
        PyTypeObject* pytype_def;
        string inherit_name;
        set<PyTypeObject*> all_child;
    };
    typedef map<string, inherit_info_t> inherit_info_map_t;
    static inherit_info_map_t& get_all_info()
    {
        static inherit_info_map_t inherit_info;
        return inherit_info;
    }

    static void add(const string& child_, const string& base_, PyTypeObject* def_)
    {
        inherit_info_t tmp;
        tmp.inherit_name = base_;
        tmp.pytype_def = def_;
        get_all_info()[child_] = tmp;
        get_all_info()[base_].all_child.insert(def_);
    }
    static bool is_instance(PyObject* pysrc, const string& class_)
    {
        inherit_info_map_t& inherit_info = get_all_info();
        inherit_info_t& tmp = inherit_info[class_];
        if (tmp.pytype_def && PyObject_TypeCheck(pysrc, tmp.pytype_def))
        {
            return true;
        }
        for (set<PyTypeObject*>::iterator it = tmp.all_child.begin(); it != tmp.all_child.end(); ++it)
        {
            if (*it && PyObject_TypeCheck(pysrc, *it))
            {
                return true;
            }
        }

        return false;
    }
    
};


//! 记录C++ class 对应到python中的名称、参数信息等,全局
struct static_pytype_info_t
{
    string class_name;
    string mod_name;
    int    total_args_num;
    PyTypeObject* pytype_def;
};

//! 工具类，用于生成分配python class的接口，包括分配、释放
template<typename T>
struct pyclass_base_info_t
{
    struct obj_data_t
    {
        obj_data_t():obj(NULL){}

        PyObject_HEAD;
        T* obj;
        bool forbid_release;
        void disable_auto_release(){ forbid_release = true; }
        void release()
        {
            if (obj)
            {
                delete obj;
                obj = NULL;
            }
        }
    };

    static void free_obj(obj_data_t* self)
    {
        if  (false == self->forbid_release && self->obj)
        {
            self->release();
        }
        self->ob_type->tp_free((PyObject*)self);
    }

    static PyObject *alloc_obj(PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
        obj_data_t *self = (obj_data_t *)type->tp_alloc(type, 0);
        return (PyObject *)self;
    }
        static PyObject *release(PyTypeObject *type, PyObject *args)
    {
        obj_data_t *self = (obj_data_t *)type;
                self->release();
                Py_RETURN_TRUE;
    }
    static static_pytype_info_t pytype_info;
};
template<typename T>
static_pytype_info_t pyclass_base_info_t<T>::pytype_info;

//! 方便生成pyclass 初始化函数
template <typename CLASS_TYPE, typename CTOR>
struct pyclass_ctor_tool_t;

//! used to gen method of py class
template<typename T>
struct pyclass_method_gen_t;

//! 防止出现指针为NULL调用出错
#define  NULL_PTR_GUARD(X) if (NULL == X) {PyErr_SetString(PyExc_TypeError, "obj data ptr NULL");return NULL;}

//! 用于生成python 的getter和setter接口，适配于c++ class的成员变量
template <typename CLASS_TYPE, typename RET>
struct pyclass_member_func_gen_t
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    typedef RET CLASS_TYPE::* property_ptr_t;

    static PyObject *getter_func(obj_data_t *self, void *closure)
    {
        property_ptr_t property_ptr;
        ::memcpy(&property_ptr, &closure, sizeof(closure));
        NULL_PTR_GUARD(self->obj);
        CLASS_TYPE* p = self->obj;
        return pytype_traits_t<RET>::pyobj_from_cppobj(p->*property_ptr);
    }
    static int setter_func(obj_data_t *self, PyObject *value, void *closure)
    {
        property_ptr_t property_ptr;
        ::memcpy(&property_ptr, &closure, sizeof(closure));
        CLASS_TYPE* p = self->obj;

        return pytype_traits_t<RET>::pyobj_to_cppobj(value, p->*property_ptr);
    }
};

//! 用于C++ 注册class的工具类，会记录class对应的名称、成员方法、成员变量
class pyclass_regigster_tool_t
{
public:
    struct method_info_t
    {
        PyCFunction func;
        string func_name;
        string func_real_name;
        string doc;
        string doc_real;
        int  args_num;
        int  option_args_num;
        long func_addr;
    };
    struct property_info_t
    {
        void*   ptr;
        string  property_name;
        string  doc;
        getter  getter_func;
        setter  setter_func;
    };
    virtual ~pyclass_regigster_tool_t(){}

    typedef PyObject *(*pyobj_alloc_t)(PyTypeObject*, PyObject*, PyObject*);

    string                class_name;
    string                class_real_name;
    string                class_name_with_mod;
    string                class_reel_name_with_mod;
    string                inherit_name;
    int         type_size;
    string      doc;
    int         args_num;
    int         option_args_num;
    destructor  dector;
    initproc    init;
    pyobj_alloc_t ctor;

    //!  member functions
    PyCFunction                      delete_func;
    vector<method_info_t>        methods_info;
    //! property 
    vector<property_info_t>        propertys_info;
    //! for init module
    PyTypeObject                        pytype_def;
    //! method
    vector<PyMethodDef>                pymethod_def;
    //! property
    vector<PyGetSetDef>     pyproperty_def;

    //! 静态类型需要全局记录该类型被注册成神马python 类型
    static_pytype_info_t*   static_pytype_info;

    template<typename FUNC>
    pyclass_regigster_tool_t& reg(FUNC f_, const string& func_name_, string doc_ = "")
    {
        method_info_t tmp;
        tmp.func_name = func_name_;
        tmp.func_real_name = func_name_ + "_internal_";
        tmp.doc       = doc_;
        tmp.doc_real  = "internal use";
        tmp.func      = (PyCFunction)pyclass_method_gen_t<FUNC>::pymethod;
        tmp.args_num = pyclass_method_gen_t<FUNC>::args_num();
        tmp.option_args_num = pyclass_method_gen_t<FUNC>::option_args_num();
        ::memcpy(&tmp.func_addr, &f_, sizeof(f_));
        methods_info.push_back(tmp);
        return *this;
    }
    template<typename CLASS_TYPE,typename RET>
    pyclass_regigster_tool_t& reg_property(RET CLASS_TYPE::* member_, const string& member_name_, string doc_ = "")
    {
        property_info_t tmp;
        ::memcpy(&tmp.ptr, &member_, sizeof(member_));
        tmp.property_name = member_name_;
        tmp.doc = doc_;
        tmp.getter_func = (getter)pyclass_member_func_gen_t<CLASS_TYPE, RET>::getter_func;
        tmp.setter_func = (setter)pyclass_member_func_gen_t<CLASS_TYPE, RET>::setter_func;
        propertys_info.push_back(tmp);
        return *this;
    }
};


class ffpython_t
{
    struct reg_info_t
    {
        reg_info_t():args_num(0),option_args_num(0),func_addr(0), nargflag(false){}
        int  args_num;
        int  option_args_num;
        long func_addr;
        PyCFunction func;
        string func_name;
        string func_impl_name;
        string doc;
        string doc_impl;
        bool nargflag;
    };

public:
    ffpython_t()
    {
        if (!Py_IsInitialized())
            Py_Initialize();
    }
    ~ffpython_t()
    {
        clear_cache_pyobject();
    }
    static int init_py()
    {
        Py_Initialize();
        return 0;
    }
    static int final_py()
    {
        Py_Finalize();
        return 0;
    }
    static int add_path(const string& path_)
    {
        char buff[1024];
        SAFE_SPRINTF(buff, sizeof(buff), "import sys\nif '%s' not in sys.path:\n\tsys.path.append('%s')\n", path_.c_str(), path_.c_str());
        PyRun_SimpleString(buff);
        return 0;
    }
    static int run_string(const string& py_)
    {
        PyRun_SimpleString(py_.c_str());
        return 0;
    }
    static int reload(const string& py_name_)
    {
        PyObject *pName = NULL, *pModule = NULL;
        string err_msg;

        pName   = PyString_FromString(py_name_.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
            return -1;
        }

        PyObject *pNewMod = PyImport_ReloadModule(pModule);
        Py_DECREF(pModule);
        if (NULL == pNewMod)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
            return -1;
        }
        Py_DECREF(pNewMod);   
        return 0;
    }
    static int load(const string& py_name_)
    {
        PyObject *pName = NULL, *pModule = NULL;
        string err_msg;

        pName   = PyString_FromString(py_name_.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
            return -1;
        }

        Py_DECREF(pModule);
        return 0;
    }
    //! 注册static function，
    template<typename T>
    ffpython_t& reg(T func_, const string& func_name_, string doc_ = "", bool nargflag = false)
    {
        reg_info_t tmp;
        tmp.args_num = pyext_func_traits_t<T>::args_num();
        tmp.option_args_num = pyext_func_traits_t<T>::option_args_num();
        tmp.func     = (PyCFunction)pyext_func_traits_t<T>::pyfunc;
        tmp.func_name= func_name_;
        tmp.func_impl_name = func_name_ + "_internal_";
        tmp.doc      = doc_;
        tmp.doc_impl = string("internal use, please call ") + func_name_;
        tmp.func_addr= (long)func_;
        
        if (nargflag){
            tmp.args_num = 1;
            tmp.nargflag = nargflag;
        }
        m_func_info.push_back(tmp);
        return *this;
    }

    //! 注册c++ class
    template<typename T, typename CTOR>
    pyclass_regigster_tool_t& reg_class(const string& class_name_, string doc_ = "", string inherit_name_ = "")
    {
        if (pyclass_base_info_t<T>::pytype_info.class_name.empty() == false)
            throw runtime_error("this type has been registed");

        pyclass_base_info_t<T>::pytype_info.class_name = class_name_;
        //pyclass_base_info_t<T>::pytype_info.mod_name   = m_mod_name;
        pyclass_base_info_t<T>::pytype_info.total_args_num = pyext_func_traits_t<CTOR>::args_num() + 
                                                                     pyext_func_traits_t<CTOR>::option_args_num();

        pyclass_regigster_tool_t tmp;
        tmp.class_name                = class_name_;
        tmp.class_real_name        = class_name_ + "_internal_";
        tmp.inherit_name        = inherit_name_;
        tmp.doc             = doc_;
        tmp.dector                        = (destructor)pyclass_base_info_t<T>::free_obj;
        tmp.init                        = (initproc)pyclass_ctor_tool_t<T, CTOR>::init_obj;
        tmp.ctor                        = pyclass_base_info_t<T>::alloc_obj;
        tmp.type_size                = sizeof(typename pyclass_base_info_t<T>::obj_data_t);
        tmp.args_num        = pyext_func_traits_t<CTOR>::args_num();
        tmp.option_args_num = pyext_func_traits_t<CTOR>::option_args_num();
        tmp.static_pytype_info = &(pyclass_base_info_t<T>::pytype_info);
        //! 注册析构函数,python若不调用析构函数,当对象被gc时自动调用
        tmp.delete_func = (PyCFunction)pyclass_base_info_t<T>::release;
            m_all_pyclass.push_back(tmp);

            return m_all_pyclass.back();
    }

    //! 将需要注册的函数、类型注册到python虚拟机
    int init(const string& mod_name_, string doc_ = "")
    {
        m_mod_name = mod_name_;
        m_mod_doc  = doc_;
        PyObject* m = init_method();
        init_pyclass(m);
        return 0;
    }

    //! 调用python函数，最多支持9个参数
    template<typename RET>
    RET_V call(const string& mod_name_, const string& func_)
    {
        pycall_arg_t args(0);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1)
    {
        pycall_arg_t args(1);
        args.add(a1);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2)
    {
        pycall_arg_t args(2);
        args.add(a1).add(a2);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3)
    {
        pycall_arg_t args(3);
        args.add(a1).add(a2).add(a3);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4)
    {
        pycall_arg_t args(4);
        args.add(a1).add(a2).add(a3).add(a4);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5)
    {
        pycall_arg_t args(5);
        args.add(a1).add(a2).add(a3).add(a4).add(a5);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6)
    {
        pycall_arg_t args(6);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6,const ARG7& a7)
    {
        pycall_arg_t args(7);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8)
    {
        pycall_arg_t args(8);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8, typename ARG9>
    RET_V call(const string& mod_name_, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
        const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8, const ARG9& a9)
    {
        pycall_arg_t args(9);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8).add(a9);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call<RET_V>(mod_name_, func_, args, pyret);
    }
    //!call python class method begin******************************************************************
    template<typename RET>
    RET_V obj_call(PyObject* pobj, const string& func_)
    {
        pycall_arg_t args(0);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1)
    {
        pycall_arg_t args(1);
        args.add(a1);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2)
    {
        pycall_arg_t args(2);
        args.add(a1).add(a2);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3)
    {
        pycall_arg_t args(3);
        args.add(a1).add(a2).add(a3);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4)
    {
        pycall_arg_t args(4);
        args.add(a1).add(a2).add(a3).add(a4);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5)
    {
        pycall_arg_t args(5);
        args.add(a1).add(a2).add(a3).add(a4).add(a5);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6)
    {
        pycall_arg_t args(6);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6,const ARG7& a7)
    {
        pycall_arg_t args(7);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8)
    {
        pycall_arg_t args(8);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8, typename ARG9>
    RET_V obj_call(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
        const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8, const ARG9& a9)
    {
        pycall_arg_t args(9);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8).add(a9);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_obj_method<RET_V>(pobj, func_, args, pyret);
    }
    //!call python class method end******************************************************************
    
    //!call python lambad function begin ############################################################
    template<typename RET>
    RET_V call_lambda(PyObject* pobj)
    {
        pycall_arg_t args(0);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1)
    {
        pycall_arg_t args(1);
        args.add(a1);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2)
    {
        pycall_arg_t args(2);
        args.add(a1).add(a2);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3)
    {
        pycall_arg_t args(3);
        args.add(a1).add(a2).add(a3);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4)
    {
        pycall_arg_t args(4);
        args.add(a1).add(a2).add(a3).add(a4);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5)
    {
        pycall_arg_t args(5);
        args.add(a1).add(a2).add(a3).add(a4).add(a5);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6)
    {
        pycall_arg_t args(6);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5,const ARG6& a6,const ARG7& a7)
    {
        pycall_arg_t args(7);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8>
    RET_V call_lambda(PyObject* pobj, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
                const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8)
    {
        pycall_arg_t args(8);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
             typename ARG8, typename ARG9>
    RET_V call_lambda(PyObject* pobj, const string& func_, const ARG1& a1, const ARG2& a2, const ARG3& a3, const ARG4& a4,
        const ARG5& a5, const ARG6& a6, const ARG7& a7, const ARG8& a8, const ARG9& a9)
    {
        pycall_arg_t args(9);
        args.add(a1).add(a2).add(a3).add(a4).add(a5).add(a6).add(a7).add(a8).add(a9);
        pytype_tool_impl_t<RET_V> pyret;
        return pycall_t::call_lambda<RET_V>(pobj, args, pyret);
    }
    //!call python lambad function ennd ############################################################
    template<typename RET>
    RET_V get_global_var(const string& mod_name_, const string& var_name_)
    {
        PyObject *pName = NULL, *pModule = NULL;
        string err_msg;

        pName   = PyString_FromString(mod_name_.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
        }

        pytype_tool_impl_t<RET_V> pyret;
        PyObject *pvalue = PyObject_GetAttrString(pModule, var_name_.c_str());
        Py_DECREF(pModule);

        if (!pvalue)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
        }

        if (pyret.parse_value(pvalue))
        {
            Py_XDECREF(pvalue);
            throw runtime_error("type invalid");
        }
        Py_XDECREF(pvalue);
        return pyret.get_value();
    }

    template<typename T>
    int set_global_var(const string& mod_name_, const string& var_name_, T val_)
    {
        PyObject *pName = NULL, *pModule = NULL;
        string err_msg;

        pName   = PyString_FromString(mod_name_.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule)
        {
            pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
        }

        PyObject* pval = pytype_traits_t<T>::pyobj_from_cppobj(val_);
        int ret = PyObject_SetAttrString(pModule, var_name_.c_str(), pval);
        Py_DECREF(pModule);
        Py_XDECREF(pval);
        return ret != -1? 0: -1;
    }
    template<typename RET>
    RET_V get_obj_attr(PyObject* pobj, const string& var_name_)
    {
    	pytype_tool_impl_t<RET_V> pyret;
    	
    	PyObject *pvalue = PyObject_GetAttrString(pobj, var_name_.c_str());
    	if (!pvalue){
    		string err_msg;
    		pyops_t::traceback(err_msg);
            throw runtime_error(err_msg.c_str());
		}
		if (pyret.parse_value(pvalue))
        {
            Py_XDECREF(pvalue);
            throw runtime_error("type invalid");
        }
    	Py_XDECREF(pvalue);
    	return pyret.get_value();
    }
    template<typename T>
    int set_obj_attr(PyObject* pobj, const string& var_name_, T val_)
    {
		PyObject* pval = pytype_traits_t<T>::pyobj_from_cppobj(val_);
        int ret = PyObject_SetAttrString(pobj, var_name_.c_str(), pval);
        Py_XDECREF(pval);
    	return ret;
    }
    
    void cache_pyobject(PyObject* pobj)
    {
        m_cache_pyobject.push_back(pobj);
    }
    void clear_cache_pyobject()
    {
        if (Py_IsInitialized())
        {
            for (size_t i = 0; i < m_cache_pyobject.size(); ++i)
            {
                Py_XDECREF(m_cache_pyobject[i]);
            }
            m_cache_pyobject.clear();
        }
    }
private:
    PyObject* init_method()
    {
        string mod_name_ = m_mod_name;
        string doc_      = m_mod_doc;

        if (m_pymethod_defs.empty())
        {
            m_pymethod_defs.reserve(m_func_info.size() + 1);

            for (size_t i = 0; i < m_func_info.size(); ++i)
            {
                PyMethodDef tmp = {m_func_info[i].func_impl_name.c_str(), m_func_info[i].func,
                    METH_VARARGS, m_func_info[i].doc_impl.c_str()};
                m_pymethod_defs.push_back(tmp);
            }
            PyMethodDef tmp = {NULL};
            m_pymethod_defs.push_back(tmp);
        }

        PyObject* m = Py_InitModule3(m_mod_name.c_str(), &(m_pymethod_defs.front()), doc_.c_str());

        for (size_t i = 0; i < m_func_info.size(); ++i)
        {
            string pystr_args;
            string pystr_args_only_name;
            for (int j = 0; j < m_func_info[i].args_num; ++j)
            {
                stringstream ss;
                if (pystr_args.empty())
                {
                    ss << "a" << (j+1);
                    pystr_args += ss.str();
                }
                else
                {
                    ss << ", a" << (j+1);
                    pystr_args += ss.str();
                }
            }
            pystr_args_only_name = pystr_args;
            for (int j = 0; j < m_func_info[i].option_args_num; ++j)
            {
                stringstream ss;
                if (pystr_args.empty())
                {
                    ss << "a" << (m_func_info[i].args_num + j+1);
                    string tmp =  ss.str();
                    pystr_args_only_name += tmp;
                    pystr_args += tmp + " = None";
                }
                else
                {
                    ss << ", a" << (m_func_info[i].args_num + j+1);
                    string tmp =  ss.str();
                    pystr_args_only_name += tmp;
                    pystr_args += tmp + " = None";
                }
            }
            if (!pystr_args_only_name.empty())
                pystr_args_only_name += ",";

            string strArgFlag;
            if (m_func_info[i].nargflag){
                strArgFlag = "*";
            }
            char buff[1024];
            SAFE_SPRINTF(buff, sizeof(buff), 
                            "_tmp_ff_ = None\nif '%s' in globals():\n\t_tmp_ff_ = globals()['%s']\n"
                "def %s(%s%s):\n"
                "\t'''%s'''\n"
                "\treturn %s.%s(%ld,(%s))\n"
                "import %s\n"
                "%s.%s = %s\n"
                "%s = None\n"
                            "if _tmp_ff_:\n\tglobals()['%s'] = _tmp_ff_\n_tmp_ff_ = None\n",
                            m_func_info[i].func_name.c_str(), m_func_info[i].func_name.c_str(), 
                m_func_info[i].func_name.c_str(), strArgFlag.c_str(), pystr_args.c_str(),
                m_func_info[i].doc.c_str(), 
                m_mod_name.c_str(), m_func_info[i].func_impl_name.c_str(), m_func_info[i].func_addr, pystr_args_only_name.c_str(),
                m_mod_name.c_str(),
                m_mod_name.c_str(), m_func_info[i].func_name.c_str(), m_func_info[i].func_name.c_str(),
                m_func_info[i].func_name.c_str(),
                            m_func_info[i].func_name.c_str()
                );

            //printf(buff);
            PyRun_SimpleString(buff);
        }

        return m;
    }
    int init_pyclass(PyObject* m)
    {
        for (size_t i = 0; i < m_all_pyclass.size(); ++i)
        {
            m_all_pyclass[i].static_pytype_info->mod_name = m_mod_name;
            if (false == m_all_pyclass[i].inherit_name.empty())//! 存在基类
            {
                pyclass_regigster_tool_t* inherit_class = get_pyclass_info_by_name(m_all_pyclass[i].inherit_name);
                assert(inherit_class && "base class must be registed");
                for (size_t n = 0; n < inherit_class->methods_info.size(); ++n)
                {
                    const string& method_name = inherit_class->methods_info[n].func_name;
                    if (false == is_method_exist(m_all_pyclass[i].methods_info, method_name))
                    {
                        m_all_pyclass[i].methods_info.push_back(inherit_class->methods_info[n]);
                    }
                }
                for (size_t n = 0; n < inherit_class->propertys_info.size(); ++n)
                {
                    const string& property_name = inherit_class->propertys_info[n].property_name;
                    if (false == is_property_exist(m_all_pyclass[i].propertys_info, property_name))
                    {
                        m_all_pyclass[i].propertys_info.push_back(inherit_class->propertys_info[n]);
                    }
                }
            }
            //! init class property
            for (size_t j = 0; j < m_all_pyclass[i].propertys_info.size(); ++j)
            {
                PyGetSetDef tmp = {(char*)m_all_pyclass[i].propertys_info[j].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[j].getter_func, 
                    m_all_pyclass[i].propertys_info[j].setter_func,
                    (char*)m_all_pyclass[i].propertys_info[j].doc.c_str(),
                    m_all_pyclass[i].propertys_info[j].ptr
                };
                m_all_pyclass[i].pyproperty_def.push_back(tmp);
            }
            PyGetSetDef tmp_property_def = {NULL};
            m_all_pyclass[i].pyproperty_def.push_back(tmp_property_def);
            //! init class method
            for (size_t j = 0; j < m_all_pyclass[i].methods_info.size(); ++j)
            {
                PyMethodDef tmp = {m_all_pyclass[i].methods_info[j].func_real_name.c_str(),
                    m_all_pyclass[i].methods_info[j].func, 
                    METH_VARARGS,
                    m_all_pyclass[i].methods_info[j].doc.c_str()
                };
                m_all_pyclass[i].pymethod_def.push_back(tmp);

            }
                     PyMethodDef tmp_del = {"delete",
                    m_all_pyclass[i].delete_func,
                    METH_VARARGS,
                    "delete obj"
                };
                m_all_pyclass[i].pymethod_def.push_back(tmp_del);
            PyMethodDef tmp_method_def = {NULL};
            m_all_pyclass[i].pymethod_def.push_back(tmp_method_def);

            m_all_pyclass[i].class_name_with_mod = m_mod_name + "." + m_all_pyclass[i].class_name;
            m_all_pyclass[i].class_reel_name_with_mod = m_mod_name + "." + m_all_pyclass[i].class_real_name;

            PyTypeObject tmp_pytype_def = 
            {
                PyObject_HEAD_INIT(NULL)
                0,                         /*ob_size*/
                m_all_pyclass[i].class_reel_name_with_mod.c_str(),             /*tp_name*/
                m_all_pyclass[i].type_size,             /*tp_size*/
                0,                         /*tp_itemsize*/
                (destructor)m_all_pyclass[i].dector, /*tp_dealloc*/
                0,                         /*tp_print*/
                0,                         /*tp_getattr*/
                0,                         /*tp_setattr*/
                0,                         /*tp_compare*/
                0,                         /*tp_repr*/
                0,                         /*tp_as_number*/
                0,                         /*tp_as_sequence*/
                0,                         /*tp_as_mapping*/
                0,                         /*tp_hash */
                0,                         /*tp_call*/
                0,                         /*tp_str*/
                0,                         /*tp_getattro*/
                0,                         /*tp_setattro*/
                0,                         /*tp_as_buffer*/
                Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
                m_all_pyclass[i].doc.c_str(),           /* tp_doc */
                0,                               /* tp_traverse */
                0,                               /* tp_clear */
                0,                               /* tp_richcompare */
                0,                               /* tp_weaklistoffset */
                0,                               /* tp_iter */
                0,                               /* tp_iternext */
                &(m_all_pyclass[i].pymethod_def.front()),//Noddy_methods,             /* tp_methods */
                0,//Noddy_members,             /* tp_members */
                &(m_all_pyclass[i].pyproperty_def.front()),//Noddy_getseters,           /* tp_getset */
                0,                         /* tp_base */
                0,                         /* tp_dict */
                0,                         /* tp_descr_get */
                0,                         /* tp_descr_set */
                0,                         /* tp_dictoffset */
                (initproc)m_all_pyclass[i].init,      /* tp_init */
                0,                         /* tp_alloc */
                m_all_pyclass[i].ctor,                 /* tp_new */
            };
            m_all_pyclass[i].pytype_def = tmp_pytype_def;
            m_all_pyclass[i].static_pytype_info->pytype_def = &m_all_pyclass[i].pytype_def;
            cpp_to_pyclass_reg_info_t::add(m_all_pyclass[i].class_name, m_all_pyclass[i].inherit_name, &m_all_pyclass[i].pytype_def);

            if (PyType_Ready(&m_all_pyclass[i].pytype_def) < 0)
                return -1;
            Py_INCREF((PyObject*)(&(m_all_pyclass[i].pytype_def)));
            PyModule_AddObject(m, m_all_pyclass[i].class_real_name.c_str(), (PyObject *)&m_all_pyclass[i].pytype_def);

            stringstream str_def_args;
            stringstream str_init_args;
            for (int a = 0; a < m_all_pyclass[i].args_num; ++a)
            {
                str_def_args << "a"<<(a+1)<<",";
                str_init_args << "a"<<(a+1)<<",";
            }
            for (int b = 0; b < m_all_pyclass[b].option_args_num; ++b)
            {
                str_def_args << "a"<<(m_all_pyclass[i].args_num+ b+1)<<" = None,";
                str_init_args << "a"<<(m_all_pyclass[i].args_num+ b+1)<<",";
            }

            char buff[1024];
            SAFE_SPRINTF(buff, sizeof(buff),
                            "_tmp_ff_ = None\nif '%s' in globals():\n\t_tmp_ff_ = globals()['%s']\n"
                "import %s\n"
                "class %s(object):\n"
                "\t'''%s'''\n"
                "\tdef __init__(self, %s assign_obj_ = 0):\n"//! 定义init函数
                "\t\t'''%s'''\n"
                "\t\tif True == isinstance(assign_obj_, %s):\n"
                "\t\t\tself.obj = assign_obj_\n"
                "\t\t\treturn\n"
                "\t\tself.obj = %s(0,(%s))\n",
                            m_all_pyclass[i].class_name.c_str(), m_all_pyclass[i].class_name.c_str(),
                m_mod_name.c_str(),
                m_all_pyclass[i].class_name.c_str(),
                m_all_pyclass[i].doc.c_str(),
                str_def_args.str().c_str(),
                "init class",
                m_all_pyclass[i].class_reel_name_with_mod.c_str(),
                m_all_pyclass[i].class_reel_name_with_mod.c_str(), str_init_args.str().c_str()
                );
            
            string gen_class_str = buff;
                    SAFE_SPRINTF(buff, sizeof(buff),
                "\tdef delete(self):\n"//! 定义init函数
                                    "\t\t'''delete obj'''\n"
                                    "\t\tself.obj.delete()\n");
                    gen_class_str += buff;
                    //! 增加析构函数
            //! 增加属性
            for (size_t c = 0; c < m_all_pyclass[i].propertys_info.size(); ++c)
            {
                SAFE_SPRINTF(buff, sizeof(buff), 
                    "\tdef get_%s(self):\n"
                    "\t\treturn self.obj.%s\n"
                    "\tdef set_%s(self, v):\n"
                    "\t\tself.obj.%s = v\n"
                    "\t@property\n"
                    "\tdef %s(self):\n"
                    "\t\treturn self.obj.%s\n"
                    "\t@%s.setter\n"
                    "\tdef %s(self, v):\n"
                    "\t\tself.obj.%s = v\n",
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str(),
                    m_all_pyclass[i].propertys_info[c].property_name.c_str()
                    );
                gen_class_str += buff;
            }

            for (size_t m = 0; m < m_all_pyclass[i].methods_info.size(); ++m)
            {
                string pystr_args;
                string pystr_args_only_name;
                for (int j = 0; j < m_all_pyclass[i].methods_info[m].args_num; ++j)
                {
                    stringstream ss;
                    if (pystr_args.empty())
                    {
                        ss << "a" << (j+1);
                        pystr_args += ss.str();
                    }
                    else
                    {
                        ss << ", a" << (j+1);
                        pystr_args += ss.str();
                    }
                }
                pystr_args_only_name = pystr_args;
                for (int j = 0; j < m_all_pyclass[i].methods_info[m].option_args_num; ++j)
                {
                    stringstream ss;
                    if (pystr_args.empty())
                    {
                        ss << "a" << (m_all_pyclass[i].methods_info[m].args_num + j+1);
                        string tmp =  ss.str();
                        pystr_args_only_name += tmp;
                        pystr_args += tmp + " = None";
                    }
                    else
                    {
                        ss << ", a" << (m_all_pyclass[i].methods_info[m].args_num + j+1);
                        string tmp =  ss.str();
                        pystr_args_only_name += tmp;
                        pystr_args += tmp + " = None";
                    }
                }
                if (!pystr_args_only_name.empty())
                    pystr_args_only_name += ",";

                SAFE_SPRINTF(buff, sizeof(buff), 
                    "\tdef %s(self,%s):\n"
                    "\t\t'''%s'''\n"
                    "\t\treturn self.obj.%s(%ld,(%s))\n"
                    ,m_all_pyclass[i].methods_info[m].func_name.c_str(), pystr_args.c_str(),
                    m_all_pyclass[i].methods_info[m].doc.c_str(),
                    m_all_pyclass[i].methods_info[m].func_real_name.c_str(), m_all_pyclass[i].methods_info[m].func_addr, pystr_args_only_name.c_str()
                    );
                gen_class_str += buff;
            }
            SAFE_SPRINTF(buff, sizeof(buff), 
                "%s.%s = %s\n"
                "%s = None\n"
                            "if _tmp_ff_:\n\tglobals()['%s'] = _tmp_ff_\n_tmp_ff_ = None\n",
                m_mod_name.c_str(), m_all_pyclass[i].class_name.c_str(), m_all_pyclass[i].class_name.c_str(),
                m_all_pyclass[i].class_name.c_str(),
                            m_all_pyclass[i].class_name.c_str()
                );
            gen_class_str += buff;
            //printf(gen_class_str.c_str());
            PyRun_SimpleString(gen_class_str.c_str());
        }
        return 0;
    }


    bool is_method_exist(const vector<pyclass_regigster_tool_t::method_info_t>& src_, const string& new_)
    {
        for (size_t i = 0; i < src_.size(); ++i)
        {
            if (new_ == src_[i].func_name)
            {
                return true;
            }
        }
        return false;
    }
    bool is_property_exist(const vector<pyclass_regigster_tool_t::property_info_t>& src_, const string& new_)
    {
        for (size_t i = 0; i < src_.size(); ++i)
        {
            if (new_ == src_[i].property_name)
            {
                return true;
            }
        }
        return false;
    }
    pyclass_regigster_tool_t* get_pyclass_info_by_name(const string& name_)
    {
        for (size_t i = 0; i < m_all_pyclass.size(); ++i)
        {
            if (m_all_pyclass[i].class_name == name_)
            {
                return &(m_all_pyclass[i]);
            }
        }
        return NULL;
    }


private:
    string                              m_mod_name;
    string                              m_mod_doc;
    vector<PyMethodDef>                 m_pymethod_defs;
    vector<reg_info_t>                  m_func_info;
    
    //! reg class
    vector<pyclass_regigster_tool_t>    m_all_pyclass;
    //! cache some pyobject for optimize
    vector<PyObject*>                   m_cache_pyobject;
};






template<typename T>
struct type_ref_traits_t
{
    typedef T        value_t;
    typedef T&        ref_t;
    value_t                value; 
};
template<typename T>
struct type_ref_traits_t<T&>
{
    typedef T        value_t;
    typedef T&        ref_t;
    value_t                value; 
};
template<typename T>
struct type_ref_traits_t<const T&>
{
    typedef T        value_t;
    typedef T&        ref_t;
    value_t                value;
};
//! 用于判断是否是可选参数
template<typename T>
struct pyoption_traits_t
{
    static int is() { return 0;}
};
template<typename T>
struct pyoption_traits_t<pyoption_t<T> >
{
    static int is() { return 1;}
};


//! pytype_traits_t 封装 PyLong_FromLong 相关的操作，用于为调用python生成参数

template<>//typename T>
struct pytype_traits_t<long>
{
    static PyObject* pyobj_from_cppobj(const long& val_)
    {
        return PyLong_FromLong(long(val_));
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, long& m_ret)
    {
        if (true == PyLong_Check(pvalue_))
        {
            m_ret = (long)PyLong_AsLong(pvalue_);
            return 0;
        }
        else if (true == PyInt_Check(pvalue_))
        {
            m_ret = (long)PyInt_AsLong(pvalue_);
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "long";}
};

#define  IMPL_INT_CODE(X) \
template<> \
struct pytype_traits_t<X> \
{ \
    static PyObject* pyobj_from_cppobj(const X& val_) \
    { \
        return PyInt_FromLong(long(val_)); \
    } \
    static int pyobj_to_cppobj(PyObject *pvalue_, X& m_ret) \
    { \
        if (true == PyLong_Check(pvalue_)) \
        { \
            m_ret = (X)PyLong_AsLong(pvalue_); \
            return 0; \
        } \
        else if (true == PyInt_Check(pvalue_)) \
        { \
            m_ret = (X)PyInt_AsLong(pvalue_); \
            return 0; \
        } \
        return -1; \
    } \
    static const char* get_typename() { return #X;} \
};

IMPL_INT_CODE(int)
IMPL_INT_CODE(unsigned int)
IMPL_INT_CODE(short)
IMPL_INT_CODE(unsigned short)
IMPL_INT_CODE(char)
IMPL_INT_CODE(unsigned char)

#ifdef _WIN32
IMPL_INT_CODE(unsigned long)
#ifdef WIN_GCC
IMPL_INT_CODE(long long)
IMPL_INT_CODE(unsigned long long)
#endif
#else
#ifndef __x86_64__
IMPL_INT_CODE(int64_t)
#endif
IMPL_INT_CODE(uint64_t)
#endif

template<typename T>
struct pytype_traits_t<const T*>
{
    static PyObject* pyobj_from_cppobj(const T* val_)
    {
        const string& mod_name = pyclass_base_info_t<T>::pytype_info.mod_name;
        const string& class_name = pyclass_base_info_t<T>::pytype_info.class_name;
        PyObject *pName = NULL, *pModule = NULL, *pValue = NULL;

        if (class_name.empty())
            return pytype_traits_t<long>::pyobj_from_cppobj(long(val_));

        pName   = PyString_FromString(mod_name.c_str());
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (NULL == pModule)
        {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Failed to load \"%s\"\n", mod_name.c_str());
            assert(NULL && "this can not be happened");
            return NULL;
        }
        PyObject *pyclass = PyObject_GetAttrString(pModule, class_name.c_str());
        if (pyclass && PyCallable_Check(pyclass)) {
            PyObject *pArgs = PyTuple_New(pyclass_base_info_t<T>::pytype_info.total_args_num+1);
            for (int i = 0; i< pyclass_base_info_t<T>::pytype_info.total_args_num; ++i)
            {
                PyTuple_SetItem(pArgs, i, pytype_traits_t<int>::pyobj_from_cppobj(0));
            }

            PyObject *palloc = pyclass_base_info_t<T>::alloc_obj(pyclass_base_info_t<T>::pytype_info.pytype_def, NULL, NULL);
            typename pyclass_base_info_t<T>::obj_data_t* pdest_obj = (typename pyclass_base_info_t<T>::obj_data_t*)palloc;
            //pdest_obj->obj = val_;
            ::memcpy(&pdest_obj->obj, &val_, sizeof(pdest_obj->obj));
            pdest_obj->disable_auto_release();
            PyTuple_SetItem(pArgs, pyclass_base_info_t<T>::pytype_info.total_args_num, palloc);
            pValue = PyObject_CallObject(pyclass, pArgs);
            Py_XDECREF(pArgs);
        }

        Py_XDECREF(pyclass);
        Py_DECREF(pModule);
        return pValue;
    }

    static int pyobj_to_cppobj(PyObject *pvalue_, T*& m_ret)
    {
        PyObject *pysrc = PyObject_GetAttrString(pvalue_, "obj");
        //!PyObject_TypeCheck(pysrc, pyclass_base_info_t<T>::pytype_info.pytype_def)) {
        if (NULL == pysrc || false == cpp_to_pyclass_reg_info_t::is_instance(pysrc, pyclass_base_info_t<T>::pytype_info.class_name))
        {
            Py_XDECREF(pysrc);
            return -1;
        }
        typename pyclass_base_info_t<T>::obj_data_t* pdest_obj = (typename pyclass_base_info_t<T>::obj_data_t*)pysrc;

        m_ret = pdest_obj->obj;
        Py_XDECREF(pysrc);
        return 0;
    }

    static const char* get_typename() { return pyclass_base_info_t<T>::pytype_info.class_name.c_str();}
};

template<typename T>
struct pytype_traits_t<T*>
{
    static PyObject* pyobj_from_cppobj(T* val_)
    {
        return pytype_traits_t<const T*>::pyobj_from_cppobj(val_);
    }
    static int pyobj_to_cppobj(PyObject *pvalue_,T*& m_ret)
    {
        return pytype_traits_t<const T*>::pyobj_to_cppobj(pvalue_, m_ret);
    }
    static const char* get_typename() { return pyclass_base_info_t<T>::pytype_info.class_name.c_str();}
};

template<>
struct pytype_traits_t<PyObject*>
{
    static PyObject* pyobj_from_cppobj(PyObject* val_)
    {
        if(val_ == NULL)
        {
            Py_RETURN_NONE;
            return NULL;
        }
        return val_;
    }
    
    static int pyobj_to_cppobj(PyObject *pvalue_, PyObject*& m_ret)
    {
        m_ret = pvalue_;
        return 0;
    }
    static const char* get_typename() { return "PyObject";}
};

template<>
struct pytype_traits_t<const char*>
{
    static PyObject* pyobj_from_cppobj(const char*& val_)
    {
        return PyString_FromString(val_);
    }
    /*
    static int pyobj_to_cppobj(PyObject *pvalue_, char*& m_ret)
    {
        if (true == PyString_Check(pvalue_))
        {
            m_ret = PyString_AsString(pvalue_);
            return 0;
        }
        return -1;
    }
    */
    static const char* get_typename() { return "string";}
};

template<>
struct pytype_traits_t<char*>
{
    static PyObject* pyobj_from_cppobj(const char*& val_)
    {
        return PyString_FromString(val_);
    }
    /*
    static int pyobj_to_cppobj(PyObject *pvalue_, char*& m_ret)
    {
        if (true == PyString_Check(pvalue_))
        {
            m_ret = PyString_AsString(pvalue_);
            return 0;
        }
        return -1;
    }
    */
    static const char* get_typename() { return "string";}
};

template<>
struct pytype_traits_t<bool>
{
    static PyObject* pyobj_from_cppobj(bool val_)
    {
        if (val_)
        {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, bool& m_ret)
    {
        if (Py_False ==  pvalue_|| Py_None == pvalue_)
        {
            m_ret = false;
        }
        else
        {
            m_ret = true;
        }
        return 0;
    }
    static const char* get_typename() { return "bool";}
};

template<typename T>
struct pytype_traits_t<pyoption_t<T> >
{
    static int pyobj_to_cppobj(PyObject *pvalue_, pyoption_t<T>& m_ret)
    {
        if (Py_None == pvalue_)
        {
            return 0;
        }
        else if (0 == pytype_traits_t<typename pyoption_t<T>::value_t>::pyobj_to_cppobj(pvalue_, m_ret.value()))
        {
            m_ret.set();
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "pyoption_t";}
};

template<>
struct pytype_traits_t<string>
{
    static PyObject* pyobj_from_cppobj(const string& val_)
    {
        return PyString_FromStringAndSize(val_.c_str(), val_.size());
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, string& m_ret)
    {
        if (true == PyString_Check(pvalue_))
        {
            char* pDest = NULL;
            Py_ssize_t  nLen    = 0;
            PyString_AsStringAndSize(pvalue_, &pDest, &nLen);
            
            m_ret.assign(pDest, nLen);
            return 0;
        }
        else if (true == PyUnicode_Check(pvalue_))
        {
            char* pDest = NULL;
            Py_ssize_t  nLen    = 0;
            PyString_AsStringAndSize(pvalue_, &pDest, &nLen);
            m_ret.assign(pDest, nLen);
            return 0;
            /*
#ifdef _WIN32
            PyObject* retStr = PyUnicode_AsEncodedString(pvalue_, "gbk", "");
#else
            PyObject* retStr = PyUnicode_AsUTF8String(pvalue_);
#endif
            if (retStr)
            {
                m_ret = PyString_AsString(retStr);
                Py_XDECREF(retStr);
                return 0;
            }
            */
        }
        return -1;
    }
    static const char* get_typename() { return "string";}
};

template<>
struct pytype_traits_t<float>
{
    static PyObject* pyobj_from_cppobj(float val_)
    {
        return PyFloat_FromDouble(double(val_));
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, float& m_ret)
    {
        if (true == PyFloat_Check(pvalue_))
        {
            m_ret = (float)PyFloat_AsDouble(pvalue_);
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "float";}
};

template<>
struct pytype_traits_t<double>
{
    static PyObject* pyobj_from_cppobj(double val_)
    {
        return PyFloat_FromDouble(val_);
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, double& m_ret)
    {
        if (true == PyFloat_Check(pvalue_))
        {
            m_ret = PyFloat_AsDouble(pvalue_);
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "double";}
};

template<typename T>
struct pytype_traits_t<vector<T> >
{
    static PyObject* pyobj_from_cppobj(const vector<T>& val_)
    {
        PyObject* ret = PyList_New(val_.size());
        for (size_t i = 0; i < val_.size(); ++i)
        {
            PyList_SetItem(ret, i, pytype_traits_t<T>::pyobj_from_cppobj(val_[i]));
        }
        return ret;
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, vector<T>& m_ret)
    {
        m_ret.clear();
        if (true == PyTuple_Check(pvalue_))
        {
            int n = PyTuple_Size(pvalue_);
            for (int i = 0; i < n; ++i)
            {
                pytype_tool_impl_t<T> ret_tool;
                if (ret_tool.parse_value(PyTuple_GetItem(pvalue_, i)))
                {
                    return -1;
                }
                m_ret.push_back(ret_tool.get_value());
            }
            return 0;
        }
        else if (true == PyList_Check(pvalue_))
        {
            int n = PyList_Size(pvalue_);
            for (int i = 0; i < n; ++i)
            {
                pytype_tool_impl_t<T> ret_tool;
                if (ret_tool.parse_value(PyList_GetItem(pvalue_, i)))
                {
                    return -1;
                }
                m_ret.push_back(ret_tool.get_value());
            }
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "vector";}
};
template<typename T>
struct pytype_traits_t<list<T> >
{
    static PyObject* pyobj_from_cppobj(const list<T>& val_)
    {
        size_t n = val_.size();
        PyObject* ret = PyList_New(n);
        int i = 0;
        for (typename list<T>::const_iterator it = val_.begin(); it != val_.end(); ++it)
        {
            PyList_SetItem(ret, i++, pytype_traits_t<T>::pyobj_from_cppobj(*it));
        }
        return ret;
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, list<T>& m_ret)
    {
        pytype_tool_impl_t<T> ret_tool;
        if (true == PyTuple_Check(pvalue_))
        {
            int n = PyTuple_Size(pvalue_);
            for (int i = 0; i < n; ++i)
            {
                pytype_tool_impl_t<T> ret_tool;
                if (ret_tool.parse_value(PyTuple_GetItem(pvalue_, i)))
                {
                    return -1;
                }
                m_ret.push_back(ret_tool.get_value());
            }
            return 0;
        }
        else if (true == PyList_Check(pvalue_))
        {
            int n = PyList_Size(pvalue_);
            for (int i = 0; i < n; ++i)
            {
                pytype_tool_impl_t<T> ret_tool;
                if (ret_tool.parse_value(PyList_GetItem(pvalue_, i)))
                {
                    return -1;
                }
                m_ret.push_back(ret_tool.get_value());
            }
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "list";}
};
template<typename T>
struct pytype_traits_t<set<T> >
{
    static PyObject* pyobj_from_cppobj(const set<T>& val_)
    {
        PyObject* ret = PySet_New(NULL);
        for (typename set<T>::const_iterator it = val_.begin(); it != val_.end(); ++it)
        {
            PyObject *v = pytype_traits_t<T>::pyobj_from_cppobj(*it);
            PySet_Add(ret, v);
            Py_DECREF(v);
        }
        return ret;
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, set<T>& m_ret)
    {
        m_ret.clear();
        pytype_tool_impl_t<T> ret_tool;
        PyObject *iter = PyObject_GetIter(pvalue_);
        PyObject *item = NULL;
        while (NULL != iter && NULL != (item = PyIter_Next(iter)))
        {
            T tmp();
            if (pytype_traits_t::pyobj_to_cppobj(item, tmp))
            {
                Py_DECREF(item);
                Py_DECREF(iter);
                return -1;
            }
            m_ret.insert(tmp);
            Py_DECREF(item);
        }
        if (iter)
        {
            Py_DECREF(iter);
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "set";}
};
template<typename T, typename R>
struct pytype_traits_t<map<T, R> >
{
    static PyObject* pyobj_from_cppobj(const map<T, R>& val_)
    {
        PyObject* ret = PyDict_New();
        for (typename map<T, R>::const_iterator it = val_.begin(); it != val_.end(); ++it)
        {
            PyObject *k = pytype_traits_t<T>::pyobj_from_cppobj(it->first);
            PyObject *v = pytype_traits_t<R>::pyobj_from_cppobj(it->second);
            PyDict_SetItem(ret, k, v);
            Py_DECREF(k);
            Py_DECREF(v);
        }
        return ret;
    }
    static int pyobj_to_cppobj(PyObject *pvalue_, map<T, R>& m_ret)
    {
        m_ret.clear();
        pytype_tool_impl_t<T> ret_tool_T;
        pytype_tool_impl_t<R> ret_tool_R;
        if (true == PyDict_Check(pvalue_))
        {
            PyObject *key = NULL, *value = NULL;
            Py_ssize_t pos = 0;

            while (PyDict_Next(pvalue_, &pos, &key, &value))
            {
                T tmp_key;
                R tmp_value;
                if (pytype_traits_t<T>::pyobj_to_cppobj(key, tmp_key) ||
                    pytype_traits_t<R>::pyobj_to_cppobj(value, tmp_value))
                {
                    return -1;
                }
                m_ret[tmp_key] = tmp_value;
            }
            return 0;
        }
        return -1;
    }
    static const char* get_typename() { return "map";}
};

//! 获取python函数的返回值,工具类
template<typename T>
class pytype_tool_impl_t: public pytype_tool_t
{
public:
    pytype_tool_impl_t():m_ret(){}

    virtual int parse_value(PyObject *pvalue_)
    {
        if (pytype_traits_t<T>::pyobj_to_cppobj(pvalue_, m_ret))
        {
            return -1;
        }
        return 0;
    }

    const T& get_value() const { return m_ret; }
    virtual const char* return_type() {return pytype_traits_t<T>::get_typename();}
private:
    T    m_ret;
};

template<>
class pytype_tool_impl_t<cpp_void_t>: public pytype_tool_t
{
public:
    pytype_tool_impl_t():m_ret(){}

    virtual int parse_value(PyObject *pvalue_)
    {
        return 0;
    }

    const cpp_void_t& get_value() const { return m_ret; }
    virtual const char* return_type() { return "void";}
private:
    cpp_void_t    m_ret;
};
template<typename T>
class pytype_tool_impl_t<const T*>: public pytype_tool_t
{
public:
    pytype_tool_impl_t():m_ret(){}

    virtual int parse_value(PyObject *pvalue_)
    {
        if (pytype_traits_t<T*>::pyobj_to_cppobj(pvalue_, m_ret))
        {
            return -1;
        }
        return 0;
    }

    T* get_value() const { return m_ret; }
private:
    T*    m_ret;
};

template<typename T>
const T& pycall_t::call(const string& mod_name_, const string& func_name_, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret)
{
    PyObject *pName = NULL, *pModule = NULL;
    string err_msg;

    pName   = PyString_FromString(mod_name_.c_str());
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (NULL == pModule)
    {
        pyops_t::traceback(err_msg);
        throw runtime_error(err_msg.c_str());
        return pyret.get_value();
    }

    call_func(pModule, mod_name_, func_name_, pyarg_, pyret, err_msg);
    Py_DECREF(pModule);

    if (!err_msg.empty())
    {
        throw runtime_error(err_msg.c_str());
    }
    return pyret.get_value();
}
template<typename T>
const T& pycall_t::call_obj_method(PyObject *pObj, const string& func_name_, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret)
{
    string err_msg;
    if (NULL == pObj)
    {
        pyops_t::traceback(err_msg);
        throw runtime_error(err_msg.c_str());
        return pyret.get_value();
    }

    static string mod_name_ = "NaN";
    call_func(pObj, mod_name_, func_name_, pyarg_, pyret, err_msg);

    if (!err_msg.empty())
    {
        throw runtime_error(err_msg.c_str());
    }
    return pyret.get_value();
}

template<typename T>
const T& pycall_t::call_lambda(PyObject *pFunc, pycall_arg_t& pyarg_, pytype_tool_impl_t<T>& pyret)
{
    string err_msg;
    if (NULL == pFunc)
    {
        err_msg = "can not call null PyObject";
        throw runtime_error(err_msg.c_str());
        return pyret.get_value();
    }

    call_func_obj(pFunc, pyarg_, pyret, err_msg);

    if (!err_msg.empty())
    {
        throw runtime_error(err_msg.c_str());
    }
    return pyret.get_value();
}

    
//! 用于扩展python的工具类，用来解析参数
struct pyext_tool_t
{
    pyext_tool_t(PyObject* args_):
        m_args(args_),
        m_arg_tuple(NULL),
        m_index(0),
        m_err(false),
        m_func_addr(0)
    {
        if (!PyArg_ParseTuple(args_, "lO", &m_func_addr, &m_arg_tuple)) {
            m_err = true;
            return;
        }
        if (NULL == m_arg_tuple || false == PyTuple_Check(m_arg_tuple))
        {
            PyErr_SetString(PyExc_TypeError, "arg type invalid(shoule func_name, args tuple)");
            m_err = true;
            return;
        }
        m_size = PyTuple_Size(m_arg_tuple);
    }

    template<typename T>
    pyext_tool_t& parse_arg(T& ret_arg_)
    {
        //typedef typename type_ref_traits_t<T>::value_t value_t;
        if (false == m_err)
        {
            if (m_index >= m_size)
            {
                stringstream ss;
                ss << "param num invalid, only["<< m_index + 1 <<"] provided";
                PyErr_SetString(PyExc_TypeError, ss.str().c_str());
                m_err = true;
                return *this;
            }

            pytype_tool_impl_t<T> ret_tool;
            if (ret_tool.parse_value(PyTuple_GetItem(m_arg_tuple, m_index)))
            {
                stringstream ss;
                ss << "param[" << m_index + 1 << "] type invalid, "<< pytype_traits_t<T>::get_typename() << " needed";
                PyErr_SetString(PyExc_TypeError, ss.str().c_str());
                m_err = true;
                return *this;
            }
            ++m_index;
            ret_arg_ = ret_tool.get_value();
        }
        return *this;
    }

    bool is_err() const { return m_err;}
    long get_func_addr() const { return m_func_addr;}

    template<typename FUNC>
    FUNC get_func_ptr() const 
    {
        FUNC f = NULL;
        ::memcpy(&f, &m_func_addr, sizeof(m_func_addr));
        return f;
    }
    PyObject* m_args;
    PyObject* m_arg_tuple;
    int       m_index;
    int       m_size;
    bool      m_err;//! 是否异常
    long      m_func_addr;
};


//! 用于扩展python，生成pyobject类型的返回值给python
template<typename T>
struct pyext_return_tool_t
{
   //! 用于静态方法
    template<typename F>
    static PyObject* route_call(F f)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f());
    }
    template<typename F, typename ARG1>
    static PyObject* route_call(F f, ARG1& a1)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value));
    }
    template<typename F, typename ARG1, typename ARG2>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value, a5.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7>
        static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8>
        static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value, a8.value));
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8, typename ARG9>
        static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8, ARG9& a9)
    {
        return pytype_traits_t<T>::pyobj_from_cppobj(f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value, a8.value, a9.value));
    }
    //! 用于成员方法
    template<typename O, typename F>
    static PyObject* route_method_call(O o, F f)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)());
    }
    template<typename O, typename F, typename ARG1>
    static PyObject* route_method_call(O o, F f, ARG1& a1)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value, a8.value));
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8, typename ARG9>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8, ARG9& a9)
    {
        NULL_PTR_GUARD(o);
        return pytype_traits_t<T>::pyobj_from_cppobj((o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value,
            a7.value, a8.value, a9.value));
    }
};

template<>
struct pyext_return_tool_t<void>
{
    template<typename F>
    static PyObject* route_call(F f)
    {
        f();
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1>
    static PyObject* route_call(F f, ARG1& a1)
    {
        f(a1.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2)
    {
        f(a1.value, a2.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3)
    {
        f(a1.value, a2.value, a3.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4)
    {
        f(a1.value, a2.value, a3.value, a4.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5)
    {
        f(a1.value, a2.value, a3.value, a4.value, a5.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6)
    {
        f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
    static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7)
    {
        f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
        typename ARG8>
        static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8)
    {
        f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value);
         Py_RETURN_NONE;
    }
    template<typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
        typename ARG8, typename ARG9>
        static PyObject* route_call(F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8, ARG9& a9)
    {
        f(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value, a9.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F>
    static PyObject* route_method_call(O o, F f)
    {
        (o->*f)();
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1>
    static PyObject* route_method_call(O o, F f, ARG1& a1)
    {
        (o->*f)(a1.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2)
    {
        (o->*f)(a1.value, a2.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3)
    {
        (o->*f)(a1.value, a2.value, a3.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value);
         Py_RETURN_NONE;
    }
    template<typename O, typename F, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
        typename ARG7, typename ARG8, typename ARG9>
        static PyObject* route_method_call(O o, F f, ARG1& a1, ARG2& a2, ARG3& a3, ARG4& a4, ARG5& a5, ARG6& a6, ARG7& a7, ARG8& a8,  ARG9& a9)
    {
        (o->*f)(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value, a9.value);
         Py_RETURN_NONE;
    }
};


//! 用于扩展python，traits出注册给python的函数接口
template <typename RET>
struct pyext_func_traits_t<RET (*)()>
{
    typedef RET (*func_t)();
    static int args_num() { return 0;}
    static int option_args_num() { return 0;}
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f);
    }
};

template <typename RET, typename ARG1>
struct pyext_func_traits_t<RET (*)(ARG1)>
{
    typedef RET (*func_t)(ARG1);
    static int args_num(){ return 1-option_args_num();}
    static int option_args_num()
    {
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        if (pyext_tool.parse_arg(a1.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1);
    }
};

template <typename RET, typename ARG1, typename ARG2>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2)>
{
    typedef RET (*func_t)(ARG1, ARG2);
    static int args_num() { return 2 - option_args_num();}
    static int option_args_num()
    {
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3);
    static int args_num() { return 3-option_args_num();}
    static int option_args_num() 
    { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3);
    }
};
template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4);
    static int args_num() { return 4-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5);
    static int args_num() { return 5-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4, a5);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
    static int args_num() { return 6-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4, a5, a6);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
    static int args_num() { return 7-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4, a5, a6, a7);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7, typename ARG8>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);
    static int args_num() { return 8-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4, a5, a6, a7, a8);
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7, typename ARG8, typename ARG9>
struct pyext_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
{
    typedef RET (*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);
    static int args_num() { return 9-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is() +
            pyoption_traits_t<typename type_ref_traits_t<ARG9>::value_t>::is();
    }
    static PyObject* pyfunc(PyObject* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = (func_t)pyext_tool.get_func_addr();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        type_ref_traits_t<ARG9> a9 = init_value_traits_t<type_ref_traits_t<ARG9> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).parse_arg(a9.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_call(f, a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
};

//! 方便生成pyclass 初始化函数
template <typename CLASS_TYPE>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)()>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;

        self->obj = new CLASS_TYPE();
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        if (pyext_tool.parse_arg(a1.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4,ARG5)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value, a5.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7,typename ARG8>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value);
        return 0;
    }
};

template <typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
typename ARG7,typename ARG8,typename ARG9>
struct pyclass_ctor_tool_t<CLASS_TYPE, int(*)(ARG1,ARG2,ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9)>
{
    typedef typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t obj_data_t;
    static int init_obj(obj_data_t *self, PyObject *args, PyObject *kwds)
    {
        if (self->obj) return 0;
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return -1;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        type_ref_traits_t<ARG9> a9 = init_value_traits_t<type_ref_traits_t<ARG9> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).parse_arg(a9.value).is_err())
        {
            return -1;
        }
        self->obj = new CLASS_TYPE(a1.value, a2.value, a3.value, a4.value, a5.value, a6.value, a7.value, a8.value, a9.value);
        return 0;
    }
};

template<typename RET, typename CLASS_TYPE>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)()>
{
    typedef RET (CLASS_TYPE::*func_t)();
    static int args_num() { return 0;}
    static int option_args_num() { return 0;}
    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1);
    static int args_num() { return 1-option_args_num();}
    static int option_args_num() { return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is();}

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        if (pyext_tool.parse_arg(a1.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2);
    static int args_num() { return 2-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2);;
    }
};


template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3);
    static int args_num() { return 3-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4);
    static int args_num() { return 4-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5);
    static int args_num() { return 5-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
    static int args_num() { return 6-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
    static int args_num() { return 7-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7, typename ARG8>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);
    static int args_num() { return 8-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7, a8);;
    }
};


template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7, typename ARG8, typename ARG9>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);
    static int args_num() { return 9-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG9>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        type_ref_traits_t<ARG9> a9 = init_value_traits_t<type_ref_traits_t<ARG9> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).parse_arg(a9.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7, a8, a9);;
    }
};

//! const类型成员函数---------------------------------------------------------------------------------------------

template<typename RET, typename CLASS_TYPE>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)() const>
{
    typedef RET (CLASS_TYPE::*func_t)() const;
    static int args_num() { return 0;}
    static int option_args_num() { return 0;}
    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1) const;
    static int args_num() { return 1-option_args_num();}
    static int option_args_num() { return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is();}

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        if (pyext_tool.parse_arg(a1.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2) const;
    static int args_num() { return 2-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2);;
    }
};


template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3) const;
    static int args_num() { return 3-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4) const;
    static int args_num() { return 4-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5) const;
    static int args_num() { return 5-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const;
    static int args_num() { return 6-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const;
    static int args_num() { return 7-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7);;
    }
};

template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7, typename ARG8>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) const;
    static int args_num() { return 8-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7, a8);;
    }
};


template<typename RET, typename CLASS_TYPE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
typename ARG6, typename ARG7, typename ARG8, typename ARG9>
struct pyclass_method_gen_t<RET (CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) const>
{
    typedef RET (CLASS_TYPE::*func_t)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) const;
    static int args_num() { return 9-option_args_num();}
    static int option_args_num() { 
        return pyoption_traits_t<typename type_ref_traits_t<ARG1>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG2>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG3>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG4>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG5>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG6>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG7>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG8>::value_t>::is()+
            pyoption_traits_t<typename type_ref_traits_t<ARG9>::value_t>::is();
    }

    static PyObject *pymethod(typename pyclass_base_info_t<CLASS_TYPE>::obj_data_t* self, PyObject* args)
    {
        pyext_tool_t pyext_tool(args);
        if (pyext_tool.is_err())
        {
            return NULL;
        }
        func_t f = pyext_tool.get_func_ptr<func_t>();
        if (0 == f)
        {
            PyErr_SetString(PyExc_TypeError, "func address must provided");
            return NULL;
        }
        type_ref_traits_t<ARG1> a1 = init_value_traits_t<type_ref_traits_t<ARG1> >::value();
        type_ref_traits_t<ARG2> a2 = init_value_traits_t<type_ref_traits_t<ARG2> >::value();
        type_ref_traits_t<ARG3> a3 = init_value_traits_t<type_ref_traits_t<ARG3> >::value();
        type_ref_traits_t<ARG4> a4 = init_value_traits_t<type_ref_traits_t<ARG4> >::value();
        type_ref_traits_t<ARG5> a5 = init_value_traits_t<type_ref_traits_t<ARG5> >::value();
        type_ref_traits_t<ARG6> a6 = init_value_traits_t<type_ref_traits_t<ARG6> >::value();
        type_ref_traits_t<ARG7> a7 = init_value_traits_t<type_ref_traits_t<ARG7> >::value();
        type_ref_traits_t<ARG8> a8 = init_value_traits_t<type_ref_traits_t<ARG8> >::value();
        type_ref_traits_t<ARG9> a9 = init_value_traits_t<type_ref_traits_t<ARG9> >::value();
        if (pyext_tool.parse_arg(a1.value).parse_arg(a2.value).parse_arg(a3.value).parse_arg(a4.value)
            .parse_arg(a5.value).parse_arg(a6.value).parse_arg(a7.value).parse_arg(a8.value).parse_arg(a9.value).is_err())
        {
            return NULL;
        }
        return pyext_return_tool_t<RET>::route_method_call(self->obj, f, a1, a2, a3, a4, a5, a6, a7, a8, a9);;
    }
};

#endif
