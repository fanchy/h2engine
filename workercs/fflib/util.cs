using System;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Collections.Generic;

using pb = global::Google.Protobuf;
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
        public static  string Pb2String(pb::IMessage retMsg)
        {
            pb::CodedOutputStream cos = new pb::CodedOutputStream(bytesPBBuffer);
            retMsg.WriteTo(cos);
            string ret = Util.Byte2String(bytesPBBuffer, 0, cos.Position);
            return ret;
        }
        public static T String2Pb<T>(string data) where T : pb::IMessage, new()
        {
            T retMsg = new T();
            retMsg.MergeFrom(new pb::CodedInputStream(String2Byte(data)));
            return retMsg;
        }
    }
}
