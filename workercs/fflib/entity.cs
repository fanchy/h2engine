using System;
using System.Collections.Generic;

namespace ff
{
    public delegate object FiedlGetter(string propName);
    public delegate void FiedlSetter(string propName, object v);
    public class FieldGetterSetter
    {
        public FiedlGetter funcGetter;
        public FiedlSetter funcSetter;
    }
    enum EVarType
    {
        VarInt = 0,
        VarStr = 1,
        VarMap = 2,
    };
    public class Key2ValCfg
    {
        public int nID   = 0;
        public int nType = 0;
        public Dictionary<string, string> key2val = new Dictionary<string, string>();
    }
    public class VarTypeCfg
    {
        private static VarTypeCfg gInstance = null;
        public static VarTypeCfg Instance()
        {
            if (gInstance == null)
            {
                gInstance = new VarTypeCfg();
            }
            return gInstance;
        }
        public Dictionary<string, Key2ValCfg> key2cfg = new Dictionary<string, Key2ValCfg>();
        public int GetIDByKey(string key)
        {
            if (!key2cfg.ContainsKey(key))
            {
                return 0;
            }
            return key2cfg[key].nID;
        }
        public Key2ValCfg GetCfgByKey(string key)
        {
            if (!key2cfg.ContainsKey(key))
            {
                return null;
            }
            return key2cfg[key];
        }
        public void AddCfg(string key, Key2ValCfg cfg) 
        {
            key2cfg[key] = cfg;
        }
    }
    public class Entity
    {
        protected VarData m_varData = new VarData()
        {
            IntData = new Dictionary<int, long>(),
            StrData = new Dictionary<int, string>(),
            MapData = new Dictionary<int, Dictionary<int, long>>()
        };
        protected Dictionary<string, FieldGetterSetter> m_dictName2Func = new Dictionary<string, FieldGetterSetter>();
        public FieldGetterSetter GenVarGetterSetter(string propName)
        {
            FieldGetterSetter objField = null;
            var cfg = VarTypeCfg.Instance().GetCfgByKey(propName);
            if (cfg == null)
            {
                return objField;
            }
            int nId = cfg.nID;
            if (cfg.nType == (int)EVarType.VarInt)
            {
                objField = new FieldGetterSetter()
                {
                    funcGetter = (string name) =>
                    {
                        if (m_varData.IntData.ContainsKey(nId))
                        {
                            return m_varData.IntData[nId];
                        }
                        return null;
                    },
                    funcSetter = (string name, object newv) =>
                    {
                        m_varData.IntData[nId] = Convert.ToInt64(newv);
                    }
                };
            }
            else if (cfg.nType == (int)EVarType.VarInt)
            {
                objField = new FieldGetterSetter()
                {
                    funcGetter = (string name) =>
                    {
                        if (m_varData.StrData.ContainsKey(nId))
                        {
                            return m_varData.StrData[nId];
                        }
                        return null;
                    },
                    funcSetter = (string name, object newv) =>
                    {
                        m_varData.StrData[nId] = (string)newv;
                    }
                };
            }
            else if (cfg.nType == (int)EVarType.VarMap)
            {
                objField = new FieldGetterSetter()
                {
                    funcGetter = (string name) =>
                    {
                        if (m_varData.MapData.ContainsKey(nId))
                        {
                            return m_varData.MapData[nId];
                        }
                        return null;
                    },
                    funcSetter = (string name, object newv) =>
                    {
                        m_varData.MapData[nId] = (Dictionary<int, long>)newv;
                    }
                };
            }
            return objField;
        }
        public FieldGetterSetter GenGetterSetter(string propName)
        {
            FieldGetterSetter objField = null;
            {
                var v = this.GetType().GetProperty(propName);
                if (v != null)
                {
                    objField = new FieldGetterSetter()
                    {
                        funcGetter = (string name) =>
                        {
                            return v.GetValue(this, null);
                        },
                        funcSetter = (string name, object newv) =>
                        {
                            v.SetValue(this, newv);
                        }
                    };
                    m_dictName2Func[propName] = objField;
                    return objField;
                }
            }
            {
                var v = this.GetType().GetField(propName);
                if (v != null)
                {
                    objField = new FieldGetterSetter()
                    {
                        funcGetter = (string name) =>
                        {
                            return v.GetValue(this);
                        },
                        funcSetter = (string name, object newv) =>
                        {
                            v.SetValue(this, newv);
                        }
                    };
                    m_dictName2Func[propName] = objField;
                    return objField;
                }
            }
            objField = GenVarGetterSetter(propName);
            if (objField != null)
            {
                m_dictName2Func[propName] = objField;
                return objField;
            }
            return objField;
        }
        public object GetFieldObject(string propName)
        {
            FieldGetterSetter objField = null;
            if (!m_dictName2Func.ContainsKey(propName))
            {
                objField = GenGetterSetter(propName);
                if (objField == null)
                    return null;
            }
            else
            {
                objField = m_dictName2Func[propName];
            }
            return objField.funcGetter(propName);
        }
        public Entity SetField(string propName, object newv)
        {
            FieldGetterSetter objField = null;
            if (!m_dictName2Func.ContainsKey(propName))
            {
                objField = GenGetterSetter(propName);
                if (objField == null)
                    return null;
            }
            else
            {
                objField = m_dictName2Func[propName];
            }

            objField.funcSetter(propName, newv);
            return this;
        }
        public T GetField<T>(string propName) where T : new()
        {
            object v = GetFieldObject(propName);
            if (v is T)
            {
                return (T)v;
            }
            return new T();
        }
        public Int64 GetFieldAsInt(string propName)
        {
            object v = GetFieldObject(propName);
            if (v is int)
            {
                return (int)v;
            }
            else if (v is Int64)
            {
                return (Int64)v;
            }
            else if (v is Byte)
            {
                return (Byte)v;
            }
            else if (v is short)
            {
                return (short)v;
            }
            return 0;
        }
        public Double GetFieldAsDouble(string propName)
        {
            object v = GetFieldObject(propName);
            if (v is double)
            {
                double dv = (double)v;
                return (Int64)dv;
            }
            else if (v is float)
            {
                float dv = (float)v;
                return (Int64)dv;
            }
            return 0.0;
        }
        public string GetFieldAsString(string propName)
        {
            object v = GetFieldObject(propName);
            if (v is string)
            {
                return (string)v;
            }
            return "";
        }
        public byte[] EncodeVarData()
        {
            return FFNet.EncodeMsg(m_varData);
        }
        public VarData GetVarData()
        {
            return m_varData;
        }
    }
}
/*

            VarTypeCfg.Instance().AddCfg("varS", new Key2ValCfg() { nID = 1 });
            Foo foo = new Foo();
            foo.c = 300;

            Console.WriteLine("foo.a={0},b={1},c={2}", foo.GetFieldAsInt("a"), foo.GetFieldAsInt("b"), foo.GetFieldAsInt("c"));
            foo.SetField("a", 1000);
            foo.SetField("b", 2000);
            foo.SetField("c", 3000);
            foo.SetField("varS", 4000);
            Console.WriteLine("foo.a={0},b={1},c={2},varwS={3}", 
                foo.GetFieldAsInt("a"), foo.GetFieldAsInt("b"), foo.GetFieldAsInt("c"), foo.GetFieldAsInt("varS"));

*/
