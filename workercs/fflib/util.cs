using System;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace ff
{
    class Util
    {
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
            byte[] ret = new byte[data.Length];
            for (int i = 0; i < data.Length; ++i)
            {
                ret[i] = (Byte)data[i];
            }
            return ret;
        }
        public static string Byte2String(byte[] data, int index = 0, int len = -1)
        {
            if (len == -1)
            {
                len = data.Length;
            }
            StringWriter sw = new StringWriter();
            for (int i = index; i < index + len && i < data.Length; ++i)
            {
                sw.Write((char)data[i]);
            }
            return sw.ToString();
        }
    }
}
