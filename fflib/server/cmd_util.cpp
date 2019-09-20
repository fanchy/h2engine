#include "server/cmd_util.h"
#include "server/ffworker.h"
using namespace ff;
using namespace std;

static void handleSessionCmd(SessionReqEvent& e){
    CmdHandlerPtr cmdHandler = CMD_MGR.getCmdHandler(e.cmd);
    if (cmdHandler){
        e.isDone = true;

        if (CMD_MGR.m_funcHook && CMD_MGR.m_funcHook(e.entity, e.cmd, e.data) == false){
            return;
        }
        cmdHandler->handleCmd(e.entity, e.cmd, e.data);
    }
}
bool CmdModule::init(){
    
    EVENT_BUS_LISTEN(&handleSessionCmd);
    return true;
}

/*


static void testStaticFuncStr(EntityPtr, const string& ret_msg){
    
}
static void testStaticFunc(EntityPtr, RegisterToBroker::out_t& ret_msg){
    
}
struct TestObj{
    void testStaticFunc(EntityPtr, const std::string&){
        
    }
    void testStaticFunc2(EntityPtr, RegisterToBroker::out_t& ret_msg){
        
    }
};
    CMD_MGR.reg(100, &testStaticFuncStr);
    CMD_MGR.reg(101, &testStaticFunc);
    TestObj obj;
    CMD_MGR.reg(201, &TestObj::testStaticFunc, &obj);
    CMD_MGR.reg(202, &TestObj::testStaticFunc2, &obj);
*/