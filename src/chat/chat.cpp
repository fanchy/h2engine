
#include "common/cmdmgr.h"
#include "common/entity.h"
#include "chat/chat.h"

using namespace ff;
using namespace std;

enum ChatCmdDef{
    LOGIN  = 1001, //!演示用，随意定义
    LOGOUT = 1002,
    CHAT   = 1003
};
static void handleLogin(EntityPtr entity, const string& msg){
    
}
static void handleLogout(EntityPtr entity, const string& msg){
    
}
static void handleChat(EntityPtr entity, const string& msg){
    
}

bool ChatModule::init(){
    CMD_MGR.reg(ChatCmdDef.LOGIN,       &handleLogin);
    CMD_MGR.reg(ChatCmdDef.LOGOUT,      &handleLogout);
    CMD_MGR.reg(ChatCmdDef.CHAT,        &handleChat);
    return true;
}

