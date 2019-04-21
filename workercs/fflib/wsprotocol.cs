using System;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace ff
{
    class WSProtocol
    {
        protected byte[] cacheRecvData;
        protected bool   bIsWebSocketConnection;
        protected Dictionary<string, string> dictParams;
        public List<byte[]> listSendPkg;//!需要发送的数据
        public List<byte[]> listRecvPkg;//!已经接收的
        byte[] dataFragmentation;//!缓存分片数据
        public WSProtocol()
        {
            cacheRecvData = new byte[0];
            bIsWebSocketConnection = true;
            dictParams = new Dictionary<string, string>();
            listSendPkg = new List<byte[]>();
            listRecvPkg = new List<byte[]>();
            dataFragmentation = new byte[0];
        }
        public bool IsWebSocketConnection()
        {
            return bIsWebSocketConnection;
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
            if (!IsWebSocketConnection())
            {
                return false;
            }
            cacheRecvData = Util.MergeArray(cacheRecvData, strNewData);
            if (dictParams.Count == 0)
            {
                string strRecvData = Util.Byte2String(cacheRecvData);
                if (strRecvData.Length < 3)
                {
                    return true;
                }
                if (strRecvData.Length >= 3 && strRecvData.StartsWith("GET") == false)
                {
                    bIsWebSocketConnection = false;
                    return false;
                }
                if (strRecvData.Contains("\r\n\r\n") == false)//!header data not end
                {
                    return true;
                }
                if (strRecvData.Contains("Upgrade: websocket") == false)
                {
                    bIsWebSocketConnection = false;
                    return false;
                }
                string[] strLines = strRecvData.Split("\r\n");
                foreach (var line in strLines)
                {
                    string[] strParams = line.Split(": ");
                    if (strParams.Length == 2)
                    {
                        dictParams[strParams[0]] = strParams[1];
                        Console.WriteLine(strParams[0] + "-->"+ strParams[1]);
                    }
                    else if (strParams.Length == 1 && strParams[0].Contains("GET"))
                    {
                        dictParams["PATH"] = strParams[0];
                    }
                }
                if (false == dictParams.ContainsKey("Sec-WebSocket-Key"))
                {
                    bIsWebSocketConnection = false;
                    return false;
                }
                string Sec_WebSocket_Key = dictParams["Sec-WebSocket-Key"];
                string strGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                byte[] dataToHash = System.Text.Encoding.UTF8.GetBytes(Sec_WebSocket_Key + strGUID);
                byte[] dataHashed = SHA1.Create().ComputeHash(dataToHash);
                string strHashBase64 = Convert.ToBase64String(dataHashed);

                string strSendData = string.Format("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {0}\r\n\r\n", strHashBase64);
                AddSendPkg(Util.String2Byte(strSendData));
                strRecvData = "";
                cacheRecvData = new byte[0];
                return true;
            }
            int nFIN = ((cacheRecvData[0] & 0x80) == 0x80)? 1: 0;
            int nOpcode = cacheRecvData[0] & 0x0F;
            int nMask = ((cacheRecvData[1] & 0x80) == 0x80) ? 1 : 0;
            int nPayload_length = cacheRecvData[1] & 0x7F;
            int nPlayLoadLenByteNum = 1;
            if (nPayload_length == 126)
            {
                nPlayLoadLenByteNum = 3;
                //flag = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(cacheRecvData, 2));
            }
            int nMaskingKeyByteNum = 4;
            byte[] aMasking_key = new byte[nMaskingKeyByteNum];
            Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum, aMasking_key, 0, 4);
            byte[] aPayload_data = new byte[nPayload_length];
            Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum, aPayload_data, 0, nPayload_length);
            int nLeftSize = cacheRecvData.Length - (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length);
            byte[] leftBytes = new byte[nLeftSize];
            if (nLeftSize > 0)
            {
                Array.Copy(cacheRecvData, 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length, leftBytes, 0, nLeftSize);
                cacheRecvData = leftBytes;
            }
            for (int i = 0; i < nPayload_length; i++)
            {
                aPayload_data[i] = (byte)(aPayload_data[i] ^ aMasking_key[i % 4]);
            }

            //string strPayLoad = Util.Byte2String(aPayload_data) + "---Gotit!";
            //FFLog.Trace(string.Format("nFIN={0},nOpcode={1},nMask={2},nPayload_length={3}，strPayLoad={4}", nFIN, nOpcode, nMask, nPayload_length, strPayLoad));
            if (8 == nOpcode)
            {
                AddSendPkg(BuildPkg(new byte[0], nOpcode));// close
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
                        dataFragmentation = Util.MergeArray(dataFragmentation, aPayload_data);
                        AddRecvPkg(dataFragmentation);
                        dataFragmentation = new byte[0];
                    }
                }
                else
                {
                    dataFragmentation = Util.MergeArray(dataFragmentation, aPayload_data);
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
    }
}
