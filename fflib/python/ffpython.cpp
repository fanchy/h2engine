
#include "ffpython.h"
using namespace ff;

std::vector<ScriptIterface*> FFPython::m_regFuncs;
std::map<void*, ScriptIterface*> FFPython::m_allocObjs;
PyObject* FFPython::pyobjBuildTmpObj = NULL;
static PyObject* callExt(PyObject* self, PyObject* args)
{
	size_t idFunc = 0;
	int64_t addrObj = 0;
	int nOps = 0;
	int nAutoRelease = 0;
	PyObject* pyArgList = NULL;
	if (!PyArg_ParseTuple(args, "LiO|ii", &addrObj, &idFunc, &pyArgList, &nOps, &nAutoRelease) || pyArgList == NULL)
		return NULL;
	ScriptIterface* pfunc = FFPython::getRegFuncByID(idFunc);
	if (!pfunc) {
		Py_RETURN_NONE;
	}
	pfunc->clearTmpArg();//!支持递归
	pfunc->pobjArg = (void*)addrObj;//!如果是类方法，需要有类指针参数
	ScriptCppOps<std::vector<PyObject*> >::scriptToCpp(pyArgList, pfunc->tmpArgs);
	if (nOps != E_CLASS_DEL && pfunc->tmpArgs.size() < pfunc->nMinArgs)
	{
		char buff[256] = { 0 };
		SAFE_SPRINTF(buff, sizeof(buff), "args num error func:%s, expect:%d, given:%d", 
			pfunc->strName.c_str(), pfunc->nMinArgs, (int)pfunc->tmpArgs.size());
		PyErr_SetString(PyExc_TypeError, buff);
		pfunc->clearTmpArg();
		return NULL;
	}
	PyObject* ret = NULL;
	if (nOps == E_STATIC_FUNC) {
		ret = pfunc->handleRun();
	}
	else if (nOps == E_CLASS_NEW) {
		void* pObjNew = pfunc->handleNew();
		FFPython::m_allocObjs[pObjNew] = pfunc;
		ret = ScriptCppOps<void*>::scriptFromCpp(pObjNew);
	}
	else if (nOps == E_CLASS_DEL) {
		std::map<void*, ScriptIterface*>::iterator it = FFPython::m_allocObjs.find(pfunc->pobjArg);
		if (it != FFPython::m_allocObjs.end()) {
			FFPython::m_allocObjs.erase(it);
			pfunc->handleDel();
		}
	}
	else if (nOps == E_CLASS_METHOD || nOps == E_CLASS_FIELD) {
		if (nAutoRelease)//!如果是python创建的对象，验证一下指针有效性
		{
			std::map<void*, ScriptIterface*>::iterator it = FFPython::m_allocObjs.find(pfunc->pobjArg);
			if (it == FFPython::m_allocObjs.end()) {
				Py_RETURN_NONE;
			}
		}
		ret = pfunc->handleRun();
	}
	pfunc->clearTmpArg();
	if (nOps == 2)
		Py_RETURN_NONE;
	return ret;
}

static PyMethodDef EmbMethods[] = {
	{"callExt", callExt, METH_VARARGS, "ffpython internal func"},
	{NULL, NULL, 0, NULL}
};

FFPython::FFPython()
{
#ifdef PYTHON_3
	if (Py_IsInitialized()) {
		return;
	}
	struct PyInitTmpTool
	{
		static PyObject* PyInit_emb(void)
		{
			static PyModuleDef EmbModule = {
				PyModuleDef_HEAD_INIT, "ffpython", NULL, -1, EmbMethods,
				NULL, NULL, NULL, NULL
			};
			return PyModule_Create(&EmbModule);
		}
	};
	PyImport_AppendInittab("ffpython", &PyInitTmpTool::PyInit_emb);
	Py_Initialize();
#else
	Py_Initialize();
	Py_InitModule3("ffpython", EmbMethods, "");
#endif
	addPath("./");

	runCode("import ffpython");
	{
		const char* strCode = "\
def regFuncExt(idFunc, name):                   \n\
    import ffpython                             \n\
    callCppFunc = ffpython.callExt              \n\
    def funReal(*args):                         \n\
        return callCppFunc(0, idFunc, args, 0)  \n\
    setattr(ffpython, name, funReal)            \n\
    return True                                 \n\
ffpython.regFuncExt = regFuncExt				\n\
";
		runCode(strCode);
	}
	{
		const char* strCode = "\
def regMethodExt(idFunc, name, nameClass) :                                           \n\
	import ffpython																	  \n\
	classType = getattr(ffpython, nameClass, None)									  \n\
	if not classType:																  \n\
		return False																  \n\
	callCppFunc = ffpython.callExt												      \n\
	def funReal(self, *args):													      \n\
		return callCppFunc(self._cppInterObj_, idFunc, args, 0, self._autoRelease_)	  \n\
	setattr(classType, name, funReal)											      \n\
	return True																	      \n\
ffpython.regMethodExt = regMethodExt												  \n\
";
		runCode(strCode);
	}
	{
		const char* strCode = "\
def regFieldExt(nFuncID, fieldName, className):												\n\
	classType = getattr(ffpython, className, None)										    \n\
	callExt = ffpython.callExt																\n\
	def getFieldVal(self) :																	\n\
		return callExt(self._cppInterObj_, nFuncID, (), 4)									\n\
	def setFieldVal(self, value) :															\n\
		return callExt(self._cppInterObj_, nFuncID, (value,), 4)						    \n\
	setattr(classType, fieldName, property(getFieldVal, setFieldVal))#add property			\n\
	return																					\n\
ffpython.regFieldExt = regFieldExt															\n\
";
		runCode(strCode);
	}
	{
		const char* strCode = "\
def buildTmpObj(className, ptr) :               \n\
    srcType = getattr(ffpython, className, None)\n\
    if srcType :								\n\
        return srcType(cppTmpPtr = ptr)			\n\
    return None									\n\
ffpython.buildTmpObj = buildTmpObj				\n\
";
		runCode(strCode);
	}
	FFPython::pyobjBuildTmpObj = getScriptVar("ffpython", "buildTmpObj");
}
FFPython::~FFPython()
{
	Py_XDECREF(pyobjBuildTmpObj);
	pyobjBuildTmpObj = NULL;
	for (size_t i = 0; i < m_regFuncs.size(); ++i)
	{
		delete m_regFuncs[i];
	}
	m_regFuncs.clear();
	for (size_t i = 0; i < m_listGlobalCache.size(); ++i)
	{
		Py_XDECREF(m_listGlobalCache[i]);
	}
	m_listGlobalCache.clear();
	if (Py_IsInitialized())
		Py_Finalize();
}
void FFPython::addPath(const std::string& path)
{
	char buff[1024];
	SAFE_SPRINTF(buff, sizeof(buff), "import sys\nif '%s' not in sys.path:\n\tsys.path.append('%s')\n", path.c_str(), path.c_str());
	runCode(buff);
}
void FFPython::runCode(const std::string& code)
{
	PyRun_SimpleString(code.c_str());
	if (PyErr_Occurred()) {
		traceback(m_strErr);
	}
}
PyObject* FFPython::callFuncByObj(PyObject* pFunc, std::vector<PyObject*>& objArgs)
{
	PyObject* pValue = NULL;
	if (pFunc && PyCallable_Check(pFunc)) {
		PyObject* pArgs = PyTuple_New(objArgs.size());
		for (int i = 0; i < objArgs.size(); ++i) {
			PyTuple_SetItem(pArgs, i, objArgs[i]);
		}
		objArgs.clear();
		pValue = PyObject_CallObject(pFunc, pArgs);
		Py_DECREF(pArgs);
	}
	else {
		for (int i = 0; i < objArgs.size(); ++i) {
			Py_DECREF(objArgs[i]);
		}
		objArgs.clear();
	}
	if (PyErr_Occurred()) {
		traceback(m_strErr);
	}
	
	if (!pValue)
		Py_RETURN_FALSE;
	return pValue;
}
PyObject* FFPython::getScriptVarByObj(PyObject* pModule, const std::string& strVarName)
{
	PyObject* pValue = PyObject_GetAttrString(pModule, strVarName.c_str());
	if (!pValue) {
		Py_RETURN_NONE;
	}
	return pValue;
}
PyObject* FFPython::getScriptVar(const std::string& strMod, const std::string& strVarName)
{
	PyObject* pName = PyString_FromString(strMod.c_str());
	if (!pName)
		Py_RETURN_FALSE;
	PyObject* pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	if (!pModule) {
		Py_RETURN_NONE;
	}
	PyObject* pValue = getScriptVarByObj(pModule, strVarName);
	Py_DECREF(pModule);
	return pValue;
}
PyObject* FFPython::callFunc(const std::string& modName, const std::string& funcName, std::vector<PyObject*>& objArgs)
{
	PyObject* pFunc = getScriptVar(modName, funcName);
	PyObject* pValue = callFuncByObj(pFunc, objArgs);
	Py_XDECREF(pFunc);

	if (!pValue)
		Py_RETURN_FALSE;
	return pValue;
}
FFPython& FFPython::reg(ScriptIterface* pObj, const std::string& name, 
	int nOps, std::string nameClass, std::string nameInherit) {
	FFPython::m_regFuncs.push_back(pObj);
	FFPython::m_regFuncs.back()->strName = name;
	if (nOps == E_STATIC_FUNC) {
		call<void>("ffpython", "regFuncExt", FFPython::m_regFuncs.size() - 1, name);
	}
	else if (nOps == E_CLASS_NEW) {
		m_curRegClassName = nameClass;
		int idFunc = (int)FFPython::m_regFuncs.size() - 1;
		std::string strBaseInit;
		if (nameInherit.empty() == false) {
			nameInherit = std::string("(") + nameInherit + ")";
			strBaseInit = nameInherit + ".__init__(self,cppTmpPtr=1)";
		}
		char buff[1024*2];
		SAFE_SPRINTF(buff, sizeof(buff), "\
class %s%s:                                                          \n\
    def __init__(self, *args, **opt) :                               \n\
        %s															 \n\
        self._autoRelease_ = 0                                       \n\
        self._cppInterObj_ = opt.get('cppTmpPtr', 0)                 \n\
        if not self._cppInterObj_ :                                  \n\
            self._cppInterObj_ = ffpython.callExt(0, %d, args, 1)    \n\
            self._autoRelease_ = 1                                   \n\
    def __del__(self) :                                              \n\
        if self._autoRelease_ :                                      \n\
            ffpython.callExt(self._cppInterObj_, %d, (), 2)          \n\
    def __repr__(self):												 \n\
        return '<ffpython.%s object at 0x%%X>'%%(self._cppInterObj_) \n\
    def __str__(self):												 \n\
        return '<ffpython.%s object at 0x%%X>'%%(self._cppInterObj_) \n\
ffpython.%s = %s                                                     \n\
", nameClass.c_str(), nameInherit.c_str(), strBaseInit.c_str(), idFunc, idFunc,
	nameClass.c_str(), nameClass.c_str(), 
	nameClass.c_str(), nameClass.c_str());
		runCode(buff);
	}
	else if (nOps == E_CLASS_METHOD) {
		if (nameClass.empty())
			nameClass = m_curRegClassName;
		FFPython::m_regFuncs.back()->strName = nameClass + "." + name;
		call<void>("ffpython", "regMethodExt", FFPython::m_regFuncs.size() - 1, name, nameClass);
	}
	else if (nOps == E_CLASS_FIELD) {
		if (nameClass.empty())
			nameClass = m_curRegClassName;
		FFPython::m_regFuncs.back()->strName = nameClass + "." + name;
		call<void>("ffpython", "regFieldExt", FFPython::m_regFuncs.size() - 1, name, nameClass);
	}
	return *this;
}
int FFPython::traceback(std::string& ret_)
{
	ret_.clear();
	PyObject* err = PyErr_Occurred();
	if (!err)
	{
		return 0;
	}
	
	PyObject* ptype = NULL, * pvalue = NULL, * ptraceback = NULL;
	PyObject* pyth_module = NULL, * pyth_func = NULL;

	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	if (pvalue)
	{
		if (true == PyList_Check(pvalue))
		{
			int64_t n = PyList_Size(pvalue);
			for (int64_t i = 0; i < n; ++i)
			{
				PyObject* pystr = PyObject_Str(PyList_GetItem(pvalue, i));
				ret_ += PyString_AsString(pystr);
				ret_ += "\n";
				Py_DECREF(pystr);
			}
		}
		else if (true == PyTuple_Check(pvalue))
		{
			int64_t n = PyTuple_Size(pvalue);
			for (int64_t i = 0; i < n; ++i)
			{
				PyObject* tmp_str = PyTuple_GetItem(pvalue, i);
				if (true == PyTuple_Check(tmp_str))
				{
					int64_t m = PyTuple_Size(tmp_str);
					for (int64_t j = 0; j < m; ++j)
					{
						PyObject* pystr = PyObject_Str(PyTuple_GetItem(tmp_str, j));
						ret_ += PyString_AsString(pystr);
						ret_ += ",";
						Py_DECREF(pystr);
					}
				}
				else
				{
					PyObject* pystr = PyObject_Str(tmp_str);
					ret_ += PyString_AsString(pystr);
					Py_DECREF(pystr);
				}
				ret_ += "\n";
			}
		}
		else
		{
			PyObject* pystr = PyObject_Str(pvalue);
			if (pystr)
			{
				ret_ += PyString_AsString(pystr);
				ret_ += "\n";
				Py_DECREF(pystr);
			}
		}
	}

	/* See if we can get a full traceback */
	PyObject* module_name = PyString_FromString("traceback");
	pyth_module = PyImport_Import(module_name);
	Py_DECREF(module_name);

	if (pyth_module && ptype && pvalue && ptraceback)
	{
		pyth_func = PyObject_GetAttrString(pyth_module, "format_tb");
		if (pyth_func && PyCallable_Check(pyth_func)) {
			PyObject* pArgs = PyTuple_New(1);
			PyTuple_SetItem(pArgs, 0, ptraceback);
			PyObject* pyth_val = PyObject_CallObject(pyth_func, pArgs);
			if (pyth_val && true == PyList_Check(pyth_val))
			{
				int64_t n = PyList_Size(pyth_val);
				for (int64_t i = 0; i < n; ++i)
				{
					PyObject* tmp_str = PyList_GetItem(pyth_val, i);
					PyObject* pystr = PyObject_Str(tmp_str);
					if (pystr)
					{
						ret_ += PyString_AsString(pystr);

						Py_DECREF(pystr);
					}
					ret_ += "\n";
				}
			}
			else {
				PyErr_Print();
			}
			Py_XDECREF(pArgs);
			Py_XDECREF(pyth_val);
		}
	}
	Py_XDECREF(pyth_func);
	Py_XDECREF(pyth_module);
	Py_XDECREF(ptype);
	Py_XDECREF(pvalue);
	Py_XDECREF(ptraceback);
	PyErr_Clear();
	printf("ffpython traceback:%s\n", ret_.c_str());
	return 0;
}
bool FFPython::reload(const std::string& py_name_)
{
	PyObject* pName = NULL, * pModule = NULL;

	pName = PyString_FromString(py_name_.c_str());
	pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	if (NULL == pModule)
	{
		traceback(m_strErr);
		return false;
	}

	PyObject* pNewMod = PyImport_ReloadModule(pModule);
	Py_DECREF(pModule);
	if (NULL == pNewMod)
	{
		traceback(m_strErr);
		return false;
	}
	Py_DECREF(pNewMod);
	return 0;
}
bool FFPython::load(const std::string& py_name_)
{
	PyObject* pName = NULL, * pModule = NULL;

	pName = PyString_FromString(py_name_.c_str());
	pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	if (NULL == pModule)
	{
		traceback(m_strErr);
		return false;
	}

	Py_DECREF(pModule);
	return 0;
}
