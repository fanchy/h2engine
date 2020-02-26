using System;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Collections.Generic;
using pb = global::Google.Protobuf;

using System.Linq;

namespace ff
{
    class Util
    {
        public static byte[] bytesPBBuffer = new byte[4096];
        public static byte[] MergeArray(byte[] array1, byte[] array2)
        {
            byte[] ret = new byte[array1.Length + array2.Length];
            Array.Copy(array1, 0, ret, 0, array1.Length);
            Array.Copy(array2, 0, ret, array1.Length, array2.Length);
            return ret;
        }
        public static byte[] MergeArray(byte[][] listByteArray)
        {
            int totalLen = 0;
            foreach (var each in listByteArray)
            {
                totalLen += each.Length;
            }
            byte[] ret = new byte[totalLen];
            totalLen = 0;
            foreach (var each in listByteArray)
            {
                each.CopyTo(ret, totalLen);
                totalLen += each.Length;
            }
            return ret;
        }
        public static byte[] String2Byte(string data)
        {
            return System.Text.UTF8Encoding.Default.GetBytes(data);
        }
        public static string Byte2String(byte[] data)
        {
            return System.Text.UTF8Encoding.Default.GetString(data);
        }
        public static string Byte2String(byte[] data, int index, long count)
        {
            return System.Text.UTF8Encoding.Default.GetString(data, index, (int)count);
        }
        public static byte[] Pb2Byte(pb::IMessage retMsg)
        {
            pb::CodedOutputStream cos = new pb::CodedOutputStream(bytesPBBuffer);
            retMsg.WriteTo(cos);
            byte[] ret = new byte[cos.Position];
            Array.Copy(bytesPBBuffer, 0, ret, 0, cos.Position);

            return ret;
        }
        public static T Byte2Pb<T>(byte[] data) where T : pb::IMessage, new()
        {
            T retMsg = new T();
            retMsg.MergeFrom(new pb::CodedInputStream(data));
            return retMsg;
        }
        public static int Distance(int x1, int y1, int x2, int y2)
        {
            return Math.Max(Math.Abs(x1 - x2), Math.Abs(y1 - y2));
        }
        public static long GetNowTimeMs()
        {
            System.DateTime currentTime = DateTime.Now;
            long ret = ((Int64)currentTime.Ticks) / 10000;
            return ret;
        }
        public static string InitClassByNames(string[] names)
        {
            string nspace = "ff";

            var q = from t in System.Reflection.Assembly.GetExecutingAssembly().GetTypes()
                where t.IsClass && t.Namespace == nspace
                select t;
            q.ToList().ForEach(t => {
                if (Array.IndexOf(names, t.Name) >= 0)
                {
                    object[] paraNone = new object[]{};
                    System.Reflection.MethodInfo method = t.GetMethod("Instance");
                    if (method != null){
                        Console.WriteLine(t.Name + ":" + method);
                        var ret = method.Invoke(null, paraNone);
                        System.Reflection.MethodInfo initMethod = t.GetMethod("Init");
                        if (initMethod != null)
                        {
                            initMethod.Invoke(ret, paraNone);
                        }
                    }
                }
            });
            return "";
        }
    }
}
