#ifndef _WSPROTOCOL_H_
#define _WSPROTOCOL_H_

#include <string>
#include <vector>
#include <map>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>

#ifdef linux
#include <arpa/inet.h>
#endif
namespace ff {

#define ops_bswap_64(val) (((val) >> 56) |\
                    (((val) & 0x00ff000000000000ll) >> 40) |\
                    (((val) & 0x0000ff0000000000ll) >> 24) |\
                    (((val) & 0x000000ff00000000ll) >> 8)   |\
                    (((val) & 0x00000000ff000000ll) << 8)   |\
                    (((val) & 0x0000000000ff0000ll) << 24) |\
                    (((val) & 0x000000000000ff00ll) << 40) |\
                    (((val) << 56)))

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define ws_hton64(ll) (ops_bswap_64(ll))
    #define ws_ntoh64(ll) (ops_bswap_64(ll))
#else
    #define ws_hton64(ll) (ll)
    #define ws_ntoh64(ll) (ll)
#endif

class WSProtocol
{
public:
    WSProtocol():statusWebSocketConnection(0), bIsClose(false)
    {
    }
    bool isClose() const { return bIsClose;}
    bool isWebSocketConnection() const { return  statusWebSocketConnection == 1; }
    const std::map<std::string, std::string>& getParam() const { return dictParams; }
    const std::vector<std::string>& getSendPkg()
    {
        return listSendPkg;
    }
    void clearSendPkg()
    {
        listSendPkg.clear();
    }
    void addSendPkg(const std::string& data)
    {
        listSendPkg.push_back(data);
    }
    const std::vector<std::string>& getRecvPkg()
    {
        return listRecvPkg;
    }
    void clearRecvPkg()
    {
        listRecvPkg.clear();
    }
    void addRecvPkg(const std::string& data)
    {
        if (data.size() > 0)
        {
            listRecvPkg.push_back(data);
        }
    }
    int strSplit(const std::string& str, std::vector<std::string>& ret_, std::string sep = ",")
    {
        if (str.empty())
        {
            return 0;
        }

        std::string tmp;
        std::string::size_type pos_begin = str.find_first_not_of(sep);
        std::string::size_type comma_pos = 0;

        while (pos_begin != std::string::npos)
        {
            comma_pos = str.find(sep, pos_begin);
            if (comma_pos != std::string::npos)
            {
                tmp = str.substr(pos_begin, comma_pos - pos_begin);
                pos_begin = comma_pos + sep.length();
            }
            else
            {
                tmp = str.substr(pos_begin);
                pos_begin = comma_pos;
            }

            if (!tmp.empty())
            {
                ret_.push_back(tmp);
                tmp.clear();
            }
        }
        return 0;
    }
    bool handleRecv(const char* buff, size_t len)
    {
        if (statusWebSocketConnection == -1)
        {
            return false;
        }
        cacheRecvData.append(buff, len);
        do{
            if (dictParams.empty() == true)
            {
                std::string& strRecvData = cacheRecvData;
                if (strRecvData.size() >= 3)
                {
                    if (strRecvData.find("GET") == std::string::npos)
                    {
                        statusWebSocketConnection = -1;
                        return false;
                    }
                }
                else if (strRecvData.size() >= 2)
                {
                    if (strRecvData.find("GE") == std::string::npos)
                    {
                        statusWebSocketConnection = -1;
                        return false;
                    }
                }
                else
                {
                    if (strRecvData.find("G") == std::string::npos)
                    {
                        statusWebSocketConnection = -1;
                        return false;
                    }
                }
                statusWebSocketConnection = 1;
                if (strRecvData.find("\r\n\r\n") == std::string::npos)//!header data not end
                {
                    return true;
                }
                if (strRecvData.find("Upgrade: websocket") == std::string::npos)
                {
                    statusWebSocketConnection = -1;
                    return false;
                }
                std::vector<std::string> strLines;
                strSplit(strRecvData, strLines, "\r\n");
                for (size_t i = 0; i < strLines.size(); ++i)
                {
                    const std::string& line = strLines[i];
                    std::vector<std::string> strParams;
                    strSplit(line, strParams, ": ");
                    if (strParams.size() == 2)
                    {
                        dictParams[strParams[0]] = strParams[1];
                    }
                    else if (strParams.size() == 1 && strParams[0].find("GET") != std::string::npos)
                    {
                        dictParams["PATH"] = strParams[0];
                    }
                }
                if (dictParams.find("Sec-WebSocket-Key") != dictParams.end())
                {
                    const std::string& Sec_WebSocket_Key = dictParams["Sec-WebSocket-Key"];
                    std::string strGUID = Sec_WebSocket_Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                    std::string dataHashed = sha1Encode(strGUID);
                    std::string strHashBase64 = base64Encode(dataHashed.c_str(), dataHashed.length(), false);

                    char buff[512] = {0};
                    snprintf(buff, sizeof(buff), "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", strHashBase64.c_str());

                    addSendPkg(buff);
                }
                else if (dictParams.find("Sec-WebSocket-Key1") != dictParams.end())
                {
                    std::string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\n";

                    std::string str_origin = dictParams["Origin"];
                    if (str_origin.empty())
                    {
                        str_origin = "null";
                    }
                    handshake += std::string("Sec-WebSocket-Origin: ") + str_origin +"\r\n";

                    std::string str_host = dictParams["Host"];
                    if (false == str_host.empty())
                    {
                        std::vector<std::string> tmp_path_arg;
                        strSplit(strLines[0], tmp_path_arg, " ");
                        std::string tmp_path = "/";
                        if (tmp_path_arg.size() >= 2)
                        {
                            tmp_path = tmp_path_arg[1];
                        }

                        handshake += std::string("Sec-WebSocket-Location: ws://") + dictParams["Host"] + tmp_path + "\r\n\r\n";
                    }

                    uint32_t key1 = computeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key1"]);
                    uint32_t key2 = computeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key2"]);

                    std::string& key_ext   = strLines[strLines.size() - 1];
                    if (key_ext.size() < 8)
                    {
                        statusWebSocketConnection = -1;
                        return false;
                    }

                    char tmp_buff[16] = {0};
                    memcpy(tmp_buff, (const char*)(&key1), sizeof(key1));
                    memcpy(tmp_buff + sizeof(key1), (const char*)(&key2), sizeof(key2));
                    memcpy(tmp_buff + sizeof(key1) + sizeof(key2), key_ext.c_str(), 8);

                    handshake += computeMd5(tmp_buff, sizeof(tmp_buff));
                    addSendPkg(handshake);
                }
                else
                {
                    statusWebSocketConnection = -1;
                    return false;
                }
                cacheRecvData.clear();
                return true;
            }
            int nFIN = ((cacheRecvData[0] & 0x80) == 0x80)? 1: 0;

            int nOpcode = cacheRecvData[0] & 0x0F;
            //int nMask = ((cacheRecvData[1] & 0x80) == 0x80) ? 1 : 0; //!this must be 1
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
            if ((int)cacheRecvData.size() < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum))
            {
                return true;
            }
            if (nPayload_length == 126)
            {
                uint16_t tmpLen = 0;
                memcpy(&tmpLen, cacheRecvData.c_str() + 2, 2);
                nPayload_length = ntohs((int16_t)tmpLen);
            }
            else if (nPayload_length == 127)
            {
                int64_t tmpLen = 0;
                memcpy(&tmpLen, cacheRecvData.c_str() + 2, 8);
                nPayload_length = (int)ws_ntoh64(tmpLen);
            }

            if ((int)cacheRecvData.size() < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length))
            {
                return true;
            }
            std::string aMasking_key;
            aMasking_key.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum, nMaskingKeyByteNum);
            std::string aPayload_data;
            aPayload_data.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum, nPayload_length);
            int nLeftSize = cacheRecvData.size() - (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length);

            if (nLeftSize > 0)
            {
                std::string leftBytes;
                leftBytes.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length, nLeftSize);
                cacheRecvData = leftBytes;
            }
            else{
                cacheRecvData.clear();
            }
            for (int i = 0; i < nPayload_length; i++)
            {
                aPayload_data[i] = (char)(aPayload_data[i] ^ aMasking_key[i % nMaskingKeyByteNum]);
            }

            if (8 == nOpcode)
            {
                addSendPkg(buildPkg("", nOpcode));// close
                bIsClose = true;
            }
            else if (2 == nOpcode || 1 == nOpcode || 0 == nOpcode || 9 == nOpcode)
            {
                if (9 == nOpcode)//!ping
                {
                    addSendPkg(buildPkg("", 0xA));// pong
                }

                if (nFIN == 1)
                {
                    if (dataFragmentation.size() == 0)
                    {
                        addRecvPkg(aPayload_data);
                    }
                    else
                    {
                        dataFragmentation += aPayload_data;
                        addRecvPkg(dataFragmentation);
                        dataFragmentation.clear();
                    }
                }
                else
                {
                    dataFragmentation += aPayload_data;
                }
            }
        }while(cacheRecvData.size() > 0);
        return true;
    }
    std::string buildPkg(const std::string& dataBody, int opcode = 0x01)
    {
        int nBodyLenByteNum = 0;
        if (dataBody.size() >= 65536)
        {
            nBodyLenByteNum = 8;
        }
        else if (dataBody.size() >= 126)
        {
            nBodyLenByteNum = 2;
        }
        std::string ret = "  ";
        ret[0] = 0;
        ret[1] = 0;
        ret[0] |= 0x80;//!_fin
        ret[0] |= (char)opcode;//0x01;//! opcode 1 text 2 binary

        if (nBodyLenByteNum == 0)
        {
            ret[1] = (char)dataBody.size();
            ret += dataBody;
        }
        else if (nBodyLenByteNum == 2)
        {
            ret[1] = 126;

            uint16_t extsize = (uint16_t)dataBody.size();
            int16_t extsizeNet = htons((int16_t)extsize);
            ret.append((const char*)(&extsizeNet), sizeof(extsizeNet));
            ret += dataBody;
        }
        else
        {
            ret[1] = 127;
            //Array.Copy(dataBody, 0, ret, 10, dataBody.Length);

            size_t extsize = dataBody.size();
            int64_t extsizeNet = ws_hton64((int64_t)extsize);//System.Net.IPAddress.HostToNetworkOrder((long)extsize);
            ret.append((const char*)(&extsizeNet), sizeof(extsizeNet));
            ret += dataBody;
        }
        return ret;
    }

    std::string sha1Encode(const std::string& strSrc)
    {
        const char *src = strSrc.c_str();
        SHA_CTX c;
        unsigned char *dest = (unsigned char *)malloc((SHA_DIGEST_LENGTH + 1)*sizeof(unsigned char));
        memset(dest, 0, SHA_DIGEST_LENGTH + 1);
        if(!SHA1_Init(&c))
        {
            free(dest);
            return NULL;
        }
        SHA1_Update(&c, src, strlen((const char*)src));
        SHA1_Final(dest,&c);
        OPENSSL_cleanse(&c,sizeof(c));
        std::string ret;
        ret.assign((const char *)dest, SHA_DIGEST_LENGTH);
        free(dest);
        return ret;
    }

    std::string base64Encode(const char * input, int length, bool with_new_line)
    {
        BIO * bmem = NULL;
        BIO * b64 = NULL;
        BUF_MEM * bptr = NULL;

        b64 = BIO_new(BIO_f_base64());
        if(!with_new_line) {
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        }
        bmem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, bmem);
        BIO_write(b64, input, length);
        (void)BIO_flush(b64);
        BIO_get_mem_ptr(b64, &bptr);

        char * buff = (char *)::malloc(bptr->length + 1);
        memcpy(buff, bptr->data, bptr->length);
        buff[bptr->length] = 0;

        BIO_free_all(b64);
        std::string ret(buff);
        ::free(buff);
        return ret;
    }
    uint32_t computeWebsokcetKeyVal(const std::string& val)
    {
        uint32_t ret    = 0;
        uint32_t kongge = 0;
        std::string str_num;

        for (size_t i = 0; i < val.length(); ++i)
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
            ret = uint32_t(::atoi(str_num.c_str()) / kongge);
        }
        ret = htonl(ret);

        return ret;
    }

    std::string computeMd5(const char* s, int n)
    {
        MD5_CTX c;
        unsigned char md5[17]={0};

        MD5_Init(&c);
        MD5_Update(&c, s, n);
        MD5_Final(md5,&c);
        std::string ret;
        ret.assign((const char*)md5, 16);
        return ret;
    }
private:
    int   statusWebSocketConnection;
    bool  bIsClose;
    std::string cacheRecvData;
    std::map<std::string, std::string> dictParams;
    std::vector<std::string> listSendPkg;//!需要发送的数据
    std::vector<std::string> listRecvPkg;//!已经接收的
    std::string dataFragmentation;//!缓存分片数据
};

}

#endif
