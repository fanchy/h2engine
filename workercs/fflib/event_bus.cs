
using System;
using System.Collections.Generic;


namespace ff
{
    public class EventBus
    {
        private static EventBus gInstance = null;
        public static EventBus Instance() {
            if (gInstance == null){
                gInstance = new EventBus();
            }
            return gInstance;
        }
        public delegate void EventHandler(object e);
        public delegate void EventHandlerCommon<EVENT_TYPE>(EVENT_TYPE e);//泛型委托
        protected Dictionary<string, EventHandler> m_dictName2Func;
        EventBus()
        {
            m_dictName2Func = new Dictionary<string, EventHandler>();
        }
        public void FireEvent(object e)
        {
            Type t = e.GetType();
            if (m_dictName2Func.ContainsKey(t.Name) == false)
            {
                return;
            }
            m_dictName2Func[t.Name](e);

        }
        public EventBus ListenEvent<T>(EventHandlerCommon<T> func)
        {
            Type t = typeof(T);
            EventHandler eh = (object oe)=>{
                T e = (T)oe;
                func(e);
            };
            if (m_dictName2Func.ContainsKey(t.Name) == false)
            {
                m_dictName2Func[t.Name] = eh;
            }
            else {
                m_dictName2Func[t.Name] += eh;
            }
            return this;
        }
    }
}
/*
    public class EventFoo{
        public int a = 10;
    }

    EventBus.Instance().ListenEvent((EventFoo f)=>{
        Console.WriteLine("listen 1......{0}", f.a);
    });
    EventBus.Instance().ListenEvent((EventFoo f)=>{
        Console.WriteLine("listen 2......{0}", f.a);
    });
    EventFoo ef = new EventFoo();
    ef.a  = 20;
    EventBus.Instance().FireEvent(ef);
*/
