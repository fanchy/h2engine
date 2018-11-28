
#include "net/ctrlimpl/socket_ctrl_gate.h"
#include "net/net_stat.h"
#include "base/log.h"
#include "base/str_tool.h"
#include "net/codec.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/des.h>
#include <openssl/evp.h>  
#include <openssl/bio.h>  
#include <openssl/buffer.h>  
#include <openssl/md5.h>  

#include <stdio.h>
using namespace std;
using namespace ff;
#define FFNET "FFNET"

SocketCtrlGate::SocketCtrlGate(MsgHandlerPtr msg_handler_, NetStat* ns_):
    SocketCtrlCommon(msg_handler_),
    m_state(WAIT_FIRSTPKG),
    m_type(0),
    m_net_stat(ns_),
    m_last_update_tm(0),
    m_recv_pkg_num(0)
{
    
}

SocketCtrlGate::~SocketCtrlGate()
{
    
}

int SocketCtrlGate::handleOpen(SocketI* s_)
{
    m_last_update_tm = m_net_stat->getHeartBeat().tick();
    m_net_stat->getHeartBeat().add(s_);
    LOGTRACE((FFNET, "SocketCtrlGate::handleOpen ok"));
    return 0;
}

int SocketCtrlGate::handleRead(SocketI* s_, const char* buff, size_t len)
{
    LOGTRACE((FFNET, "SocketCtrlGate::handleRead begin len<%u>", len));
    if (WAIT_FIRSTPKG == m_state)
    {
        //!判断是不是websocket协议
        if (len >= 4)
        {
            string tmp;
            tmp.assign(buff, 4);
            if (tmp == "GET ")
            {
                m_state = WAIT_HANDSHAKE;
                return handleRead_websocket(s_, buff, len);
            }
        }
        //!判断是不是flash协议
        const char* cross_file_req = "<policy-file-request/>";
        size_t tmp = len > 22? 22: len;
        size_t i   = 0;
        for (; i < tmp; ++i)
        {
            if (buff[i] != cross_file_req[i])
            {
                break;
            }
        }
        if (i == tmp)
        {
            m_state = CROSSFILE_WAIT_END;
            LOGINFO((FFNET, "SocketCtrlGate::handleRead begin crossfile len<%u>", len));
            string ret = "<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\"/></cross-domain-policy>";
            ret.append("\0", 1);
            s_->asyncSend(ret);
            return 0;
        }
        m_state = WAIT_BINMSG_PKG;
    }
    
    if (WAIT_WEBSOCKET_PKG == m_state || WAIT_HANDSHAKE == m_state || WAIT_WEBSOCKET_NOMASK_PKG == m_state)
    {
        return handleRead_websocket(s_, buff, len);
    }
    
    if (WAIT_BINMSG_PKG != m_state)
    {
        return 0;
    }
    SocketCtrlCommon::handleRead(s_, buff, len);

    //! 判断消息包是否超过限制

    if (true == m_message.haveRecvHead(m_have_recv_size) && m_message.size() > (size_t)m_net_stat->getMaxPacketSize())
    {
        LOGERROR((FFNET, "SocketCtrlGate::handleRead end msg size<%u>,%d", m_message.size(), m_net_stat->getMaxPacketSize()));
        s_->close();
    }

    //! 更新心跳
    if (m_last_update_tm != m_net_stat->getHeartBeat().tick())
    {
        m_last_update_tm = m_net_stat->getHeartBeat().tick();
        m_net_stat->getHeartBeat().update(s_);
    }

    LOGTRACE((FFNET, "SocketCtrlGate::handleRead end msg size<%u>", m_message.size()));
    return 0;
}

int SocketCtrlGate::handleError(SocketI* s_)
{
    m_net_stat->getHeartBeat().del(s_);
    SocketCtrlCommon::handleError(s_);
    return 0;
}
typedef unsigned char uchar ;
string sha1_encode(const char *src)
{
    SHA_CTX c;
    uchar *dest = (uchar *)malloc((SHA_DIGEST_LENGTH + 1)*sizeof(uchar));
    memset(dest, 0, SHA_DIGEST_LENGTH + 1);
    if(!SHA1_Init(&c))
    {
        free(dest);
        return NULL;
    }
    SHA1_Update(&c, src, strlen((const char*)src));
    SHA1_Final(dest,&c);
    OPENSSL_cleanse(&c,sizeof(c));
    string ret;
    ret.assign((const char *)dest, SHA_DIGEST_LENGTH);
    free(dest);
    return ret;
}

static string base64_encode(const char * input, int length, bool with_new_line)  
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
    string ret = buff;
    ::free(buff);
    return ret;  
}  

static string buildpkg(const string& data, int opcode = 0x01)
{
    string ret = "  ";
    ret[0]  = 0;
    ret[1]  = 0;
    ret[0] |= 0x80;//!_fin
    ret[0] |= opcode;//0x01;//! opcode 1 text 2 binary

    if (data.length() < 126)
    {
        ret[1] = (uint8_t)(data.length());
        ret += data;
    }
    else if (data.length() < 65536)
    {
        ret[1] = 126;
        uint16_t extsize = data.length();
        extsize = htons(extsize);
        ret.append((const char*)(&extsize), sizeof(extsize));
        ret += data;
        LOGTRACE((FFNET, "buildpkg..........1"));
    }
    else
    {
        ret[1] = 127;
        uint64_t extsize = (uint64_t)data.length();
        extsize = hton64(extsize);
        ret.append((const char*)(&extsize), sizeof(extsize));


        ret += data;
        LOGTRACE((FFNET, "buildpkg..........ret extsize=%u,size=%u", extsize, ret.size()));
    }
    return ret;
}
static string ComputeWebSocketHandshakeSecurityHash09(string secWebSocketKey)
{
    string secWebSocketAccept = secWebSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    string sha1Hash    = sha1_encode(secWebSocketAccept.c_str());
    string ret         = base64_encode(sha1Hash.c_str(), sha1Hash.length(), false);

    return ret;
}

static uint32_t computeWebsokcetKeyVal(const string& val)
{
    uint32_t ret    = 0;
    uint32_t kongge = 0;
    string str_num;
    
    for (size_t i = 0; i < val.length(); ++i)
    {
        if (val[i] == ' ')
        {
            ++kongge;
        }
        else if (val[i] >= '0' and val[i] <= '9')
        {
            str_num += val[i];
        }
    }
    LOGTRACE((FFNET, "str_num=%s,kongge=%d", str_num, kongge));
    if (kongge > 0)
    {
        ret = uint32_t(::atoi(str_num.c_str()) / kongge);
    }
    ret = htonl(ret);
    
    return ret;
}

static string compute_md5(const char* s, int n)
{
    MD5_CTX c;
    unsigned char md5[17]={0};  

    MD5_Init(&c);  
    MD5_Update(&c, s, n);
    MD5_Final(md5,&c);
    string ret;
    ret.assign((const char*)md5, 16);
    return ret;
}

int SocketCtrlGate::handleRead_nomask_msg(SocketI* s_, const char* buff, size_t len)
{
    LOGTRACE((FFNET, "SocketCtrlGate::handleRead_nomask_msg begin len<%u>", len));
    
    uint32_t begin = 0;
    uint32_t m     = 0;
    for (m = 0; m < len; ++m)
    {
        if ((uint8_t)buff[m] == 0xFF)//! end frame
        {
            m_buff.append(buff + begin, m - begin);
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_nomask_msg buff=%s", m_buff));
            
            handle_parse_text_prot(s_, m_buff);
            ++m_recv_pkg_num;
            this->post_msg(s_);

            m_buff.clear();
            begin = m + 1;
        }
        else if ((uint8_t)buff[m] == 0x0)//! begin frame
        {
            begin = m + 1;
            continue;
        }
    }
    if (m > begin)
    {
        m_buff.append(buff + begin, m - begin);
    }
    return 0;
}


int SocketCtrlGate::checkPreSend(SocketI* sp_, string& buff, int flag)
{
    LOGTRACE((FFNET, "SocketCtrlGate::checkPreSend ...m_state=%d,m_recv_pkg_num=%d", int(m_state), m_recv_pkg_num));
    if (sp_->socket() < 0)
    {
        return -1;
    }
    
    if (WAIT_WEBSOCKET_PKG == m_state)
    {
        if (m_recv_pkg_num > 0)//!如果是handshake，不需要包装头
        {
            string tmp;
            tmp.assign(buff.c_str(), buff.size());
            int opcode = (m_type == 1)? 0x01: 0x02;
            string ret_msg = buildpkg(tmp, opcode);
            buff.assign(ret_msg.c_str(), ret_msg.size());
            LOGTRACE((FFNET, "SocketCtrlGate::checkPreSend mask"));
        }
    }
    else if (WAIT_WEBSOCKET_NOMASK_PKG == m_state)
    {
        if (m_recv_pkg_num > 0)//!如果是handshake，不需要包装头
        {
            string ret_msg = " ";
            ret_msg.append(buff.c_str(), buff.size());
            ret_msg += " ";
            ret_msg[0] = 0x0;
            ret_msg[ret_msg.size() - 1] = 0xFF;
            buff.assign(ret_msg.c_str(), ret_msg.size());
            LOGTRACE((FFNET, "SocketCtrlGate::checkPreSend nomask"));
        }
    }
    return 0;
}


int SocketCtrlGate::handleRead_websocket(SocketI* s_, const char* buff, size_t len)
{
    LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket begin len<%u>", len));
    if (WAIT_HANDSHAKE == m_state)
    {
        m_message.appendToBody(buff, len);
        const string& cur_head = m_message.getBody();
        if (cur_head.find("\r\n\r\n") < 0)
        {
            return 0;
        }

        map<string, string> param;
        vector<string> tmp_by_line;
        StrTool::split(cur_head, tmp_by_line, "\r\n");
        for (size_t i = 0; i < tmp_by_line.size(); ++i)
        {
            vector<string> tmp_val;
            StrTool::split(tmp_by_line[i], tmp_val, ":");
            //LOGTRACE((FFNET, "tmp_by_line=%s,tmp_val=%u,%s", tmp_by_line[i], tmp_val.size(), tmp_val[0]));
            if (tmp_val.size() == 2)
            {
                param[tmp_val[0]] = StrTool::trim(tmp_val[1]);
            }
            else if (tmp_val.size() == 3)
            {
                param[tmp_val[0]] = StrTool::trim(tmp_val[1]) + ":" + tmp_val[2];
            }
        }

        string argSec_WebSocket_Key = param["Sec-WebSocket-Key"];
        
        string handshake;
        if (argSec_WebSocket_Key.empty())
        {
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket end msg content<%s,%s>", param["Sec-WebSocket-Key1"], param["Sec-WebSocket-Key2"]));
            
            handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n";
            handshake += "Upgrade: WebSocket\r\n";
            handshake += "Connection: Upgrade\r\n";
            
            string str_origin = param["Origin"];
            if (str_origin.empty())
            {
                str_origin = "null";
            }
            handshake += string("Sec-WebSocket-Origin: ") + str_origin +"\r\n";
            
            string str_host = param["Host"];
            if (false == str_host.empty())
            {
                vector<string> tmp_path_arg;
                StrTool::split(tmp_by_line[0], tmp_path_arg, " ");
                string tmp_path = "/";
                if (tmp_path_arg.size() >= 2)
                {
                    tmp_path = tmp_path_arg[1];
                }
                
                handshake += string("Sec-WebSocket-Location: ws://") + param["Host"] + tmp_path + "\r\n\r\n";
            }

            uint32_t key1 = computeWebsokcetKeyVal(param["Sec-WebSocket-Key1"]);
            uint32_t key2 = computeWebsokcetKeyVal(param["Sec-WebSocket-Key2"]);
            
            string& key_ext   = tmp_by_line[tmp_by_line.size() - 1];
            if (key_ext.size() < 8)
            {
                LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket wait data key_ext[8byte]"));
                return 0;
            }

            char tmp_buff[16] = {0};
            
            memcpy(tmp_buff, (const char*)(&key1), sizeof(key1));
            memcpy(tmp_buff + sizeof(key1), (const char*)(&key2), sizeof(key2));
            memcpy(tmp_buff + sizeof(key1) + sizeof(key2), key_ext.c_str(), 8);
            
            handshake += compute_md5(tmp_buff, sizeof(tmp_buff));
            m_state = WAIT_WEBSOCKET_NOMASK_PKG;
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket end handshake size=%s", handshake.size()));
        }
        else
        {
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket end msg content<%s,%s>", m_message.getBody(), param["Sec-WebSocket-Key"]));
            string acceptKey = ComputeWebSocketHandshakeSecurityHash09(param["Sec-WebSocket-Key"]);

            handshake = "HTTP/1.1 101 Switching Protocols\r\n";
            handshake += "Upgrade: WebSocket\r\n";
            handshake += "Connection: Upgrade\r\n";
            handshake += "Sec-WebSocket-Accept: " + acceptKey;
            handshake += "\r\n\r\n";
            m_state = WAIT_WEBSOCKET_PKG;
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket end handshake=%s", handshake));
        }
        s_->asyncSend(handshake);
                
        m_message.clear();
        return 0;
    }
    if (WAIT_WEBSOCKET_NOMASK_PKG == m_state)
    {
        return handleRead_nomask_msg(s_, buff, len);
    }
    if (WAIT_WEBSOCKET_PKG == m_state)
    {
        return handleRead_mask_msg(s_, buff, len);;
    }
    
    LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket end msg"));
    return 0;
}
int SocketCtrlGate::handleRead_mask_msg(SocketI* s_, const char* buff, size_t len)
{
    m_buff.append(buff, len);

    while(true)
    {
        if (m_buff.size() < 2)
        {
            return 0;
        }
        //! 第一个字节
        //! 最高位用于描述消息是否结束,如果为1则该消息为消息尾部,如果为零则还有后续数据包;后面3位是用于扩展定义的,如果没有扩展约定的情况则必须为0.可以通过以下c#代码方式得到相应值
        bool iseof = (m_buff[0] >> 7) > 0? true: false;
        //! 最低4位用于描述消息类型,消息类型暂定有15种,其中有几种是预留设置.代码可以这样得到消息类型:
        int opcode = int(m_buff[0] & 0xF);
        //! 消息的第二个字节主要用一描述掩码和消息长度,最高位用0或1来描述是否有掩码处理,可以通过以下代码方式得到相应值
        bool mask  = (m_buff[1] & 0x80) == 0x80? true: false;
        //! 剩下的后面7位用来描述消息长度,由于7位最多只能描述127所以这个值会代表三种情况,
        //! 一种是消息内容少于126存储消息长度,如果消息长度少于UINT16的情况此值为126,当消息长度大于UINT16的情况下此值为127;
        //! 这两种情况的消息长度存储到紧随后面的byte[],分别是UINT16(2位byte)和UINT64(4位byte).可以通过以下c#代码方式得到相应值
        uint64_t pkgsize = (uint64_t)(m_buff[1] & 0x7F);
        uint8_t headsize = 2;
        uint8_t masksize = 0;
        
        LOGTRACE((FFNET, "SocketCtrlGate::handleRead_mask_msg dump iseof=%d,opcode=%d,mask=%d,pkgsize=%d..", iseof, opcode, mask, pkgsize));
        if (mask)
        {
            masksize = 4;
        }
        
        if (pkgsize == 126)
        {
            headsize = 4;
            if (m_buff.size() < headsize)
            {
                return 0;
            }
            uint16_t tmp_val = *((uint16_t*)(m_buff.c_str() + 2));
            pkgsize          = ntohs(tmp_val);
            if (m_buff.size() < headsize + masksize + pkgsize)
            {
                return 0;
            }
        }
        else if (pkgsize == 127)
        {
            headsize = 10;
            if (m_buff.size() < headsize)
            {
                return 0;
            }
            pkgsize = *((uint64_t*)(m_buff.c_str() + 2));
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket dump pkgsize=%d,headsize=%d,masksize=%d ..", (uint64_t)pkgsize, (int)headsize, (int)masksize));
            
            pkgsize =  ntoh64(pkgsize);
            long len = 0;
            int n = 1;
            for (int i = 7; i >= 0; i--)
            {
                len += (int)m_buff[2+i] * n;
                n *= 256;
            }
            //pkgsize = len;
            
            LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket dump pkgsize=%d,headsize=%d,masksize=%d ..", (uint64_t)pkgsize, (int)headsize, (int)masksize));
            
            if (m_buff.size() < headsize + masksize + pkgsize)
            {
                return 0;
            }
        }
        else
        {
            if (m_buff.size() < headsize + masksize + pkgsize)
            {
                return 0;
            }
            
        }
        
        //如果存在掩码的情况下获取4位掩码值:
        if (mask)
        {
            uint8_t* masking_key = ((uint8_t*)(m_buff.c_str() + headsize));
            for (uint32_t i = 0; i < pkgsize; i++)
            {
                m_buff[headsize + masksize + i] = (char)(m_buff[headsize + masksize + i] ^ masking_key[i % 4]);
            }
            //SocketCtrlCommon::handleRead(s_, m_buff.c_str() + headsize + sizeof(masking_key), pkgsize - sizeof(masking_key));
        }

        LOGTRACE((FFNET, "SocketCtrlGate::handleRead_websocket dump %d,%d,%d,m_buff=%u ..", (int)pkgsize, (int)headsize, (int)masksize, m_buff.size()));
        
        ++m_recv_pkg_num;
        if (2 == opcode)//!bin msg
        {
            SocketCtrlCommon::handleRead(s_, m_buff.c_str() + headsize + masksize, pkgsize);
            //s_->asyncSend(buildpkg(tmp));
        }
        else if (1 == opcode || 0 == opcode)
        {
            string tmp;
            tmp.assign(m_buff.c_str() + headsize + masksize, pkgsize);
            m_type = 1;
            handle_parse_text_prot(s_, tmp);
        }
        else if (8 == opcode)//!ask close
        {
            LOGTRACE((FFNET, "SocketCtrlGate::handle_parse_text_prot ask close"));
            s_->close();
            return 0;
        }
        else if (0x9 == opcode)//!ping http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10#section-4.5.2
        {
            if (WAIT_WEBSOCKET_PKG == m_state)
            {
                string ret_msg = buildpkg("", 0xA);
                s_->sendRaw(ret_msg);
                LOGTRACE((FFNET, "SocketCtrlGate::pong for ping"));
            }
        }

        m_buff.erase(0, int(headsize + masksize + pkgsize));
    }
    //! 更新心跳
    if (m_last_update_tm != m_net_stat->getHeartBeat().tick())
    {
        m_last_update_tm = m_net_stat->getHeartBeat().tick();
        m_net_stat->getHeartBeat().update(s_);
    }
    
	return 0;
}
int SocketCtrlGate::handle_parse_text_prot(SocketI* s_, const string& str_body_)
{
    LOGTRACE((FFNET, "SocketCtrlGate::handle_parse_text_prot str_body_=%s", str_body_));
    //！字符串协议 cmd:1,res:1
    string::size_type pos_end = str_body_.find("\n");
    if (pos_end != string::npos)
    {
        string tmp_opt = str_body_.substr(0, pos_end);
        
        vector<string> vec_cmd_opt;
        StrTool::split(tmp_opt, vec_cmd_opt, ",");
        
        for (size_t j = 0; j < vec_cmd_opt.size(); ++j)
        {
            vector<string> vec_cmd_k_v;
            StrTool::split(vec_cmd_opt[j], vec_cmd_k_v, ":");
            if (vec_cmd_k_v.size() == 2)
            {
                if (vec_cmd_k_v[0] == "cmd")
                {
                    m_message.getHead().cmd = ::atoi(vec_cmd_k_v[1].c_str());
                }
                else if (vec_cmd_k_v[0] == "res")
                {
                    m_message.getHead().flag = ::atoi(vec_cmd_k_v[1].c_str());
                }
            }
        }
        m_message.getHead().size = str_body_.size() - pos_end - 1;
        m_message.appendToBody(str_body_.c_str() + pos_end + 1, m_message.getHead().size);
    }
    else{
        m_message.getHead().size = str_body_.size();
        m_message.appendToBody(str_body_.c_str(), m_message.getHead().size);
    }
    //! 判断消息包是否超过限制
    if (m_message.getHead().size > (size_t)m_net_stat->getMaxPacketSize())
    {
        LOGERROR((FFNET, "SocketCtrlGate::handle_parse_text_prot exceed=%d:%d", m_message.getHead().size, m_net_stat->getMaxPacketSize()));
        s_->close();
        return 0;
    }
    this->post_msg(s_);
    return 0;
}

int SocketCtrlGate::get_type() const
{
    if (WAIT_WEBSOCKET_NOMASK_PKG == m_state || WAIT_WEBSOCKET_PKG == m_state)
    {
        return m_type;
    }
    return 0;
}

