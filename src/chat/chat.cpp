
#include "server/cmd_util.h"
#include "server/entity.h"
#include "server/ffworker.h"
#include "server/db_mgr.h"
#include "chat/chat.h"
#include "common/common_def.h"

using namespace ff;
using namespace std;

enum ChatCmdDef{
    CHAT_C_LOGIN       = 101, //!演示用，随意定义
    CHAT_C_BROADCAST   = 102,
    CHAT_S_BROADCAST   = 102
};

class ChatCtrl:public EntityField
{
public:
    ChatCtrl():nChatTimes(0){}
    int   nChatTimes;
};

struct ChatLoginDbFunctor{
    void operator()(QueryDBResult& result){
        if (entity->getSession() == 0){
            //!异步载入数据的过程中，user可能下线
            return;
        }
        char buff[256] = {0};
        if (result.dataResult.empty()){//! 数据库里没有数据，创建一条数据
            snprintf(buff, sizeof(buff), "insert into chattest (UID, CHAT_TIMES) values ('%lu', '0')", uid);
            DB_MGR.asyncQueryModId(uid, buff);
        }
        else{
            entity->get<ChatCtrl>()->nChatTimes = ::atoi(result.dataResult[0][0].c_str());
        }
        EntityPtr old = ENTITY_MGR.get(ENTITY_PLAYER, uid);
        if (old){//!重登录，踢掉原来的
            old->sessionClose();
            ENTITY_MGR.del(ENTITY_PLAYER, uid);
        }
        entity->setUid(uid);
        entity->setType(ENTITY_PLAYER);
        ENTITY_MGR.add(entity);
        
        
        snprintf(buff, sizeof(buff), "user[%lu]进入了聊天室！", entity->getUid());
        FFWORKER.gateBroadcastMsg(CHAT_S_BROADCAST, buff);//!这个是gate广播也就是全服广播
    }
    userid_t    uid;
    EntityPtr   entity;
};
static void handleLogin(EntityPtr entity, const string& msg){//!处理登录，简单示例，用字符串协议
    userid_t uid = ::atoi(msg.c_str());
    if (uid == 0){
        entity->sendMsg(CHAT_S_BROADCAST, "非法操作，请先使用101协议登录，参数为uid(非0)！");
        return;
    }
    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "select CHAT_TIMES from chattest where UID = '%lu'", uid);
    ChatLoginDbFunctor dbFunc;
    dbFunc.uid = uid;
    dbFunc.entity = entity;
    DB_MGR.asyncQueryModId(uid, sql, dbFunc);
    
}
static void handleLogout(EntityPtr entity, const string& msg){
    //!清除缓存
    if (entity->getUid() == 0){
        return;
    }
    char buff[256] = {0};
    snprintf(buff, sizeof(buff), "update chattest set CHAT_TIMES = '%d' where UID = '%lu'", 
                                 entity->get<ChatCtrl>()->nChatTimes, entity->getUid());
    DB_MGR.asyncQueryModId(entity->getUid(), buff);
    
    snprintf(buff, sizeof(buff), "user[%lu]离开了聊天室！", entity->getUid());
    FFWORKER.gateBroadcastMsg(CHAT_S_BROADCAST, buff);//!这个是gate广播也就是全服广播
    ENTITY_MGR.del(ENTITY_PLAYER, entity->getUid());
}
struct ChatFunctor{
    bool operator()(EntityPtr e){
        e->sendMsg(CHAT_S_BROADCAST, destData);
        return true;
    }
    string destData;
};
static void handleChat(EntityPtr entity, const string& msg){
    //!简单示例，广播给所有人
    char buff[256] = {0};
    entity->get<ChatCtrl>()->nChatTimes += 1;
    snprintf(buff, sizeof(buff), "user[%lu]说:%s 发言总次数[%d]", entity->getUid(), msg.c_str(), entity->get<ChatCtrl>()->nChatTimes);
    ChatFunctor func;
    func.destData = buff;
    ENTITY_MGR.foreach(ENTITY_PLAYER, func);//!这里遍历每一个entity，也就是本worker上的所有用户,这个是示例，不如gateBroadcastMsg效率高
}
static bool cmdValidCheckFunctor(EntityPtr entity, uint16_t cmd, const string& msg){
    if (cmd == CHAT_C_LOGIN){//!登录消息只能发一次，uid被赋值就表示登录过了
        if (entity->getUid()){
            entity->sendMsg(CHAT_S_BROADCAST, "非法操作，你已经登录过了！请使用102协议说话");
            return false;
        }
    }
    else{
        if (entity->getUid() == 0){//!其他消息必须是登录后才会发，否则忽略 
            entity->sendMsg(CHAT_S_BROADCAST, "非法操作，请先使用101协议登录，参数为uid(非0)！");
            return false;
        }
    }
    return true;
}

bool ChatModule::init(){
    CMD_MGR.setCmdHookFunc(cmdValidCheckFunctor);
    CMD_MGR.reg(CHAT_C_LOGIN,            &handleLogin);
    CMD_MGR.reg(LOGOUT_CMD,              &handleLogout);
    CMD_MGR.reg(CHAT_C_BROADCAST,        &handleChat);

    //!一般而言，初始化的时候需要创建表，读取配置等
    string sql = "create table IF NOT EXISTS chattest (UID integer, CHAT_TIMES interger);";
    DB_MGR.query(sql);
    return true;
}

