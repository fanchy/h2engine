
#include "server/cmdmgr.h"
#include "server/entity.h"
#include "chat/chat.h"

using namespace ff;
using namespace std;

enum ChatCmdDef{
    CHAT_LOGIN       = 1001, //!演示用，随意定义
    CHAT_LOGOUT      = 1002,
    CHAT_BROADCAST   = 1003
};
static void handleLogin(EntityPtr entity, const string& msg){
    
}
static void handleLogout(EntityPtr entity, const string& msg){
    
}
static void handleChat(EntityPtr entity, const string& msg){
    
}

bool ChatModule::init(){
    CMD_MGR.reg(CHAT_LOGIN,            &handleLogin);
    CMD_MGR.reg(CHAT_LOGOUT,           &handleLogout);
    CMD_MGR.reg(CHAT_BROADCAST,        &handleChat);
    return true;
}

