using System;

namespace ff
{
    public class Entity
    {
        public object GetFieldObject(string propName)
        {
            var v = this.GetType().GetProperty(propName);
            if (v != null)
            {
                return v.GetValue(this, null);
            }
            var v2 = this.GetType().GetField(propName);
            if (v2 == null)
                return null;
            if (v2.IsPublic == false)
                return false;
            return v2.GetValue(this);
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
        public Entity SetField(string propName, object newv)
        {
            var v = this.GetType().GetProperty(propName);
            if (v != null)
            {
                v.SetValue(this, newv);
                return this;
            }
            var v2 = this.GetType().GetField(propName);
            if (v2 == null)
                return this;
            if (v2.IsPublic == false)
                return this;
            v2.SetValue(this, newv);
            return this;
        }
    }
}
/*

    class Foo: Entity
    {
        public int a = 100;
        public int b = 200;
        public int c { get; set; }
        public Foo(): base()
        {
        }
    }
    
    Foo foo = new Foo();
    foo.c = 300;

    Console.WriteLine("foo.a={0},b={1},c{2}", foo.GetFieldAsInt("a"), foo.GetFieldAsInt("b"), foo.GetFieldAsInt("c"));
    foo.SetFieldObject("a", 1000);
    foo.SetFieldObject("b", 2000);
    foo.SetFieldObject("c", 3000);
    Console.WriteLine("foo.a={0},b={1},c{2}", foo.GetFieldAsInt("a"), foo.GetFieldAsInt("b"), foo.GetFieldAsInt("c"));

*/