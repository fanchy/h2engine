using System;
using System.Collections.Generic;
using IronPython.Hosting;
using Microsoft.Scripting.Hosting;


namespace ff
{
    public class PyScript
    {
        private static PyScript gInstance = null;
        public static PyScript Instance() {
            if (gInstance == null){
                gInstance = new PyScript();
            }
            return gInstance;
        }
        private ScriptEngine engine;
        private ScriptScope scope;
        private dynamic m_h2call;
        public PyScript()
        {
            engine = Python.CreateEngine();
            scope = engine.CreateScope();
            AddPath("script");
            string initCode = @"
import sys
class h2module:
    def setmodule(self, modname, fieldname, obj):
        if not sys.modules.get(modname):
            sys.modules[modname] = h2module()
        mod = __import__(modname)
        setattr(mod, fieldname, obj)
sys.modules['h2ext'] = h2module()
import h2ext

FuncCache={}

#import StringIO
#import traceback
def h2call(funcname, args):
    #try:
    global FuncCache;f = FuncCache.get(funcname);
    if f==None:
        mod2func = funcname.split('.')
        mod = __import__(mod2func[0])
        f = getattr(mod, mod2func[1])
        if not f:
            raise 'h2exception:' + '%s not found'%(funcname)
        FuncCache[funcname] = f
    return f(*args)
    #except Exception:
        #fp = StringIO.StringIO()
        #traceback.print_exc()#traceback.print_exc(file = fp)
        #msg = ''#fp.getvalue()
        #tracebackdata = 'h2exception:' + funcname + ' ' + msg
        #print(tracebackdata)
        #return tracebackdata
    #return None
            ";
            EvalStr(initCode);
            m_h2call = scope.GetVariable("h2call");

        }
        public ScriptScope GetScriptScope() { return scope; }
        public object EvalStr(string code)
        {
            try
            {
                ScriptSource callCode = engine.CreateScriptSourceFromString(code, Microsoft.Scripting.SourceCodeKind.AutoDetect);
                var result = callCode.Execute(scope);
                
                return result;
            }
            catch (Exception e)
            {
                FFLog.Error("PyScript.EvalStr:" + code + "->" + e.Message);
            }
            return "";
        }
        public object CallPythonFunc(string nameModAndFunc, object[] argsObj)
        {
            object ret = null;
            Int64 nBeginUs = DateTime.Now.Ticks / 10;
            try
            {
                ret = m_h2call(nameModAndFunc, argsObj);
            }
            catch (Exception e)
            {
                var strArgs = string.Join(",", argsObj);
                FFLog.Error(string.Format("PyScript.CallPython:{0}({1})exception:{2}", nameModAndFunc, strArgs, e.Message));
            }
            PerfMonitor.Instance().AddPerf(string.Format("py={0}", nameModAndFunc), DateTime.Now.Ticks / 10 - nBeginUs);
            return ret;
        }
        public object CallPython(string nameModAndFunc, object arg1)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1});
        }
        public object CallPython(string nameModAndFunc, object arg1, object arg2)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1, arg2});
        }
        public object CallPython(string nameModAndFunc, object arg1, object arg2, object arg3)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1, arg2, arg3});
        }
        public object CallPython(string nameModAndFunc, object arg1, object arg2, object arg3, object arg4)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1, arg2, arg3, arg4});
        }
        public object CallPython(string nameModAndFunc, object arg1, object arg2, object arg3, object arg4, object arg5)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1, arg2, arg3, arg4, arg5});
        }
        public object CallPython(string nameModAndFunc, object arg1, object arg2, object arg3, object arg4, object arg5, object arg6)
        {
            return CallPythonFunc(nameModAndFunc, new object[]{arg1, arg2, arg3, arg4, arg5, arg6});
        }
        public bool AddPath(string path)
        {
            var paths = engine.GetSearchPaths();
            List<string> lstPath = new List<string>();
            lstPath.AddRange(paths);
            lstPath.Add(path);
            engine.SetSearchPaths(lstPath.ToArray());
            return true;
        }
        public PyScript RegToModule(string name, object v, string toModule = "h2ext")
        {
            CallPython("h2ext.setmodule", toModule, name, v);
            return this;
        }
    }

}
/*

PyScript.Instance().AddPath("script");

public class Student
{
    public Student()
    {
        Name = "FOO";
    }
    public int Age { get; set; }
    public string Name { get; set; }
    public override string ToString()
    {
        return string.Format("{0} is {1} years old", this.Name, this.Age);
    }
}
public delegate Student StudentNew();
StudentNew f = delegate ()
{
    return new Student();
};
PyScript.Instance().RegToModule("Student", f);
Console.WriteLine("callret:"+ PyScript.Instance().CallPython("test.foo", 11,22).ToString());
Console.WriteLine("callret2:"+ PyScript.Instance().CallPython("test.foo", 21,22).ToString());
*/