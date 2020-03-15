using System;
using System.Collections.Generic;

namespace ff
{
    public class CfgTool
    {
        private static CfgTool gInstance = null;
        public static CfgTool Instance() {
            if (gInstance == null){
                gInstance = new CfgTool();
            }
            return gInstance;
        }
        protected Dictionary<string, string> m_dictKey2Val;
        public CfgTool()
        {
            m_dictKey2Val = new Dictionary<string, string>();
        }
        public bool InitCfg(string[] args)
        {
            for (int i = 0; i < args.Length; ++i)
            {
                string key = args[i];
                key = key.Trim();
                if (key == "")
                    continue;
                string val = "";
                if (i < args.Length - 1)
                {
                    val = args[i + 1];
                }
                val = val.Trim();
                if (key[0] == '-')
                {
                    key = key.Remove(0, 1);
                    if (val.Length > 0 && val[0] != '-')
                    {
                        i += 1;
                    }
                    m_dictKey2Val[key] = val;
                }
                else
                {
                    m_dictKey2Val[key] = "";
                }
            }
            string fileName = GetCfgVal("-f");
            if (fileName.Length > 0)
            {
                string[] lines = System.IO.File.ReadAllLines(fileName);
                foreach (string line in lines)
                {
                    if (line == "" || line[0] == '#')
                        continue;
                    string[] strList = line.Split("=");
                    if (strList.Length == 1)
                        m_dictKey2Val[strList[0].Trim()] = "";
                    else
                        m_dictKey2Val[strList[0].Trim()] = strList[1].Trim();
                }
            }
            return true;
        }
        public string GetCfgVal(string key, string defalutVal = "")
        {
            if (key.Length == 0)
                return defalutVal;
            if (key[0] == '-')
                key = key.Remove(0, 1);
            if (!m_dictKey2Val.ContainsKey(key))
                return defalutVal;
            return m_dictKey2Val[key];
        }
        public bool IsEnableCfg(string key)
        {
            if (key.Length == 0)
                return false;
            if (key[0] == '-')
                key = key.Remove(0, 1);
            if (m_dictKey2Val.ContainsKey(key))
                return true;
            return false;
        }
    }
}
