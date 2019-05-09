using System;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.IO;

namespace ff
{
    class WSProtocol
    {
        protected byte[] cacheRecvData;
        protected int   statusWebSocketConnection;
        protected Dictionary<string, string> dictParams;
        public List<byte[]> listSendPkg;//!需要发送的数据
        public List<byte[]> listRecvPkg;//!已经接收的
        byte[] dataFragmentation;//!缓存分片数据
        protected bool bIsClose;
        public WSProtocol()
        {
            cacheRecvData = new byte[0];
            statusWebSocketConnection = 0;
            dictParams = new Dictionary<string, string>();
            listSendPkg = new List<byte[]>();
            listRecvPkg = new List<byte[]>();
            dataFragmentation = new byte[0];
            bIsClose = false;
        }
        public bool IsClose()
        {
            return bIsClose;
        }
        public bool IsWebSocketConnection()
        {
            return statusWebSocketConnection == 1;
        }
        public Dictionary<string, string> GetParam()
        {
            return dictParams;
        }
        public List<byte[]> GetSendPkg()
        {
            return listSendPkg;
        }
        public void ClearSendPkg()
        {
            listSendPkg.Clear();
        }
        public void AddSendPkg(byte[] data)
        {
            listSendPkg.Add(data);
        }
        public List<byte[]> GetRecvPkg()
        {
            return listRecvPkg;
        }
        public void ClearRecvPkg()
        {
            listRecvPkg.Clear();
        }
        public void AddRecvPkg(byte[] data)
        {
            if (data.Length > 0)
            {
                listRecvPkg.Add(data);
            }            
        }
        public bool HandleRecv(byte[] strNewData)
        {
            if (statusWebSocketConnection == -1)
            {
                return false;
            }
            cacheRecvData = MergeArray(cacheRecvData, strNewData);
            if (dictParams.Count == 0)
            {
                string strRecvData = Byte2String(cacheRecvData);
                if (strRecvData.Length < 3)
                {
                    return true;
                }
                if (strRecvData.Length >= 3 && strRecvData.StartsWith("GET") == false)
                {
                    statusWebSocketConnection = -1;
                    return false;
                }
                if (strRecvData.Contains("\r\n\r\n") == false)//!header data not end
                {
                    return true;
                }
                if (strRecvData.Contains("Upgrade: websocket") == false)
                {
                    statusWebSocketConnection = -1;
                    return false;
                }
                string[] strLines = strRecvData.Split("\r\n");
                foreach (var line in strLines)
                {
                    string[] strParams = line.Split(": ");
                    if (strParams.Length == 2)
                    {
                        dictParams[strParams[0]] = strParams[1];
                    }
                    else if (strParams.Length == 1 && strParams[0].Contains("GET"))
                    {
                        dictParams["PATH"] = strParams[0];
                    }
                }
                if (true == dictParams.ContainsKey("Sec-WebSocket-Key"))
                {
                    string Sec_WebSocket_Key = dictParams["Sec-WebSocket-Key"];
                    string strGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                    byte[] dataToHash = System.Text.Encoding.UTF8.GetBytes(Sec_WebSocket_Key + strGUID);
                    byte[] dataHashed = SHA1.Create().ComputeHash(dataToHash);
                    string strHashBase64 = Convert.ToBase64String(dataHashed);

                    string strSendData = string.Format("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {0}\r\n\r\n", strHashBase64);
                    AddSendPkg(String2Byte(strSendData));
                    strRecvData = "";
                    cacheRecvData = new byte[0];
                    statusWebSocketConnection = 1;

                    return true;
                }
                else if (true == dictParams.ContainsKey("Sec-WebSocket-Key1"))
                {
                    string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\n";

                    string str_origin = dictParams["Origin"];
                    if (str_origin.Length == 0)
                    {
                        str_origin = "null";
                    }
                    handshake += "Sec-WebSocket-Origin: " + str_origin + "\r\n";

                    string str_host = dictParams["Host"];
                    if (str_host.Length > 0)
                    {
                        string[] tmp_path_arg = strLines[0].Split(" ");
                        string tmp_path = "/";
                        if (tmp_path_arg.Length >= 2)
                        {
                            tmp_path = tmp_path_arg[1];
                        }

                        handshake += "Sec-WebSocket-Location: ws://" + dictParams["Host"] + tmp_path + "\r\n\r\n";
                    }

                    UInt32 key1 = ComputeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key1"]);
                    UInt32 key2 = ComputeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key2"]);

                    string keyExt = strLines[strLines.Length - 1];
                    if (keyExt.Length < 8)
                    {
                        statusWebSocketConnection = -1;
                        return false;
                    }

                    byte[] tmpBuff = new byte[16];
                    byte[] key1Bytes = BitConverter.GetBytes(key1);
                    byte[] key2Bytes = BitConverter.GetBytes(key2);
                    byte[] keyExtBytes = String2Byte(keyExt);
                    Array.Copy(key1Bytes, 0, tmpBuff, 0, key1Bytes.Length);
                    Array.Copy(key2Bytes, 0, tmpBuff, key1Bytes.Length, key2Bytes.Length);
                    Array.Copy(keyExtBytes, 0, tmpBuff, key1Bytes.Length + key2Bytes.Length, keyExtBytes.Length);
                    handshake += ComputeMd5(tmpBuff);
                    AddSendPkg(String2Byte(handshake));
                }
                else
                {
                    statusWebSocketConnection = -1;
                    return false;
                }
            }
            int nFIN = ((cacheRecvData[0] & 0x80) == 0x80)? 1: 0;
            int nOpcode = cacheRecvData[0] & 0x0F;
            //int nMask = ((cacheRecvData[1] & 0x80) == 0x80) ? 1 : 0;
            int nPayload_length = cacheRecvData[1] & 0x7F;
            int nPlayLoadLenByteNum = 1;
            int nMaskingKeyByteNum = 4;
            if (nPayload_length == 126)
            {
                nPlayLoadLenByteNum = 3;
            }
            else if (nPayload_length == 127)
            {
                nPlayLoadLenByteNum = 9;
            }
            if (cacheRecvData.Length < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum))
            {
                return true;
            }
            if (nPayload_length == 126)
            {
                byte[] nPayload_length_Bytes = new byte[2];
                Array.Copy(cacheRecvData, 2, nPayload_length_Bytes, 0, nPayload_length_Bytes.Length);
                nPayload_length = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(nPayload_length_Bytes, 0));
            }
            else if (nPayload_length == 127)
            {
                byte[] nPayload_length_Bytes = new byte[8];
                Array.Copy(cacheRecvData, 2, nPayload_length_Bytes, 0, nPayload_length_Bytes.Length);
                nPayload_length = (int)System.Net.IPAddress.NetworkToHostOrder((long)BitConverter.ToInt64(nPayload_length_Bytes, 0));
            }
            if (cacheRecvData.Length < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length))
            {
                return true;
            }

            byte[] aMasking_key = new byte[nMaskingKeyByteNum];
            Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum, aMasking_key, 0, nMaskingKeyByteNum);
            byte[] aPayload_data = new byte[nPayload_length];
            Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum, aPayload_data, 0, nPayload_length);
            int nLeftSize = cacheRecvData.Length - (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length);
            byte[] leftBytes = new byte[nLeftSize];
            if (nLeftSize > 0)
            {
                Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length, leftBytes, 0, nLeftSize);
            }
            cacheRecvData = leftBytes;
            for (int i = 0; i < nPayload_length; i++)
            {
                aPayload_data[i] = (byte)(aPayload_data[i] ^ aMasking_key[i % 4]);
            }
            FFLog.Trace(string.Format("nOpcode={0},data={1}", nOpcode, aPayload_data.Length));
            if (8 == nOpcode)
            {
                AddSendPkg(BuildPkg(new byte[0], nOpcode));// close
                bIsClose = true;
            }
            else if (2 == nOpcode || 1 == nOpcode || 0 == nOpcode || 9 == nOpcode)
            {
                if (9 == nOpcode)//!ping
                {
                    AddSendPkg(BuildPkg(new byte[0], 0xA));// pong
                }

                if (nFIN == 1)
                {
                    if (dataFragmentation.Length == 0)
                    {
                        AddRecvPkg(aPayload_data);
                    }
                    else
                    {
                        dataFragmentation = MergeArray(dataFragmentation, aPayload_data);
                        AddRecvPkg(dataFragmentation);
                        dataFragmentation = new byte[0];
                    }
                }
                else
                {
                    dataFragmentation = MergeArray(dataFragmentation, aPayload_data);
                }
            }
            
            return true;
        }
        public byte[] BuildPkg(byte[] dataBody, int opcode = 0x01)
        {
            int nBodyLenByteNum = 0;
            if (dataBody.Length >= 65536)
            {
                nBodyLenByteNum = 8;
            }
            else if (dataBody.Length >= 126)
            {
                nBodyLenByteNum = 2;
            }
            byte[] ret = new byte[2 + nBodyLenByteNum + dataBody.Length];
            ret[0] = 0;
            ret[1] = 0;
            ret[0] |= 0x80;//!_fin
            ret[0] |= (byte)opcode;//0x01;//! opcode 1 text 2 binary

            if (nBodyLenByteNum == 0)
            {
                ret[1] = (byte)dataBody.Length;
                Array.Copy(dataBody, 0, ret, 2, dataBody.Length);
            }
            else if (nBodyLenByteNum == 2)
            {
                ret[1] = 126;

                UInt16 extsize = (UInt16)dataBody.Length;
                Int16 extsizeNet = System.Net.IPAddress.HostToNetworkOrder((Int16)extsize);
                byte[] lenArray = BitConverter.GetBytes(extsizeNet);
                Array.Copy(lenArray, 0, ret, 2, lenArray.Length);
                Array.Copy(dataBody, 0, ret, 4, dataBody.Length);
            }
            else
            {
                ret[1] = 127;
                Array.Copy(dataBody, 0, ret, 10, dataBody.Length);

                int extsize = dataBody.Length;
                Int64 extsizeNet = System.Net.IPAddress.HostToNetworkOrder((long)extsize);
                byte[] lenArray = BitConverter.GetBytes(extsizeNet);
                Array.Copy(lenArray, 0, ret, 2, lenArray.Length);
                Array.Copy(dataBody, 0, ret, 10, dataBody.Length);
            }
            return ret;
        }
        UInt32 ComputeWebsokcetKeyVal(string val)
        {
            UInt32 ret = 0;
            UInt32 kongge = 0;
            string str_num = "";

            for (int i = 0; i < val.Length; ++i)
            {
                if (val[i] == ' ')
                {
                    ++kongge;
                }
                else if (val[i] >= '0' && val[i] <= '9')
                {
                    str_num += val[i];
                }
            }
            if (kongge > 0)
            {
                ret = (UInt32)(Convert.ToUInt32(str_num) / kongge);
            }
            ret = (UInt32)System.Net.IPAddress.NetworkToHostOrder(ret);

            return ret;
        }
        string ComputeMd5(byte[] data)
        {
            MD5 md5 = new MD5CryptoServiceProvider();
            byte[] output = md5.ComputeHash(data);
            return Byte2String(output);
        }
        public static byte[] MergeArray(byte[] array1, byte[] array2)
        {
            byte[] ret = new byte[array1.Length + array2.Length];
            Array.Copy(array1, 0, ret, 0, array1.Length);
            Array.Copy(array2, 0, ret, array1.Length, array2.Length);
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
