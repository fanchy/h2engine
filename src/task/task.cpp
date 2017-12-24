#include <ctype.h>

#include "task/task.h"
#include "server/db_mgr.h"
#include "common/game_event.h"
#include "server/ffworker.h"

using namespace ff;
using namespace std;

int64_t TaskConfig::getPropNum(const std::string& key){
    std::map<std::string, int64_t>::iterator it = numProp.find(key);
    if (it != numProp.end()){
        return it->second;
    }
    return 0;
}
static bool isNumberStr(const string& s){
    if (s.empty()){
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i){
        if (!::isdigit(s[i])){
            return false;
        }
    }
    return false;
}

string TaskConfig::getPropStr(const std::string& key){
    std::map<std::string, std::string>::iterator it = strProp.find(key);
    if (it != strProp.end()){
        return it->second;
    }
    return "";
}

bool TaskMgr::init(){
    vector<vector<string> > retdata; 
    vector<string> col;
    string errinfo;
    int affectedRows = 0;
    
    string sql = "select * from Taskcfg";
    DB_MGR_OBJ.syncQueryDBGroupMod("cfgDB", 0, sql, retdata, col, errinfo, affectedRows);
    
    if (retdata.empty()){
        return false;
    }
    
    m_TaskCfgs.clear();
    m_type2TaskCfg.clear();
    
    for (size_t i = 0; i < retdata.size(); ++i){
        vector<string>& row = retdata[i];
        TaskConfigPtr cfgTask = new TaskConfig();
        
        for (size_t j = 0; j < row.size(); ++j){
            const string& keyname = col[j];
            if (isNumberStr(row[j])){
                cfgTask->numProp[keyname] = ::atol(row[j].c_str());
            }
            else{
                cfgTask->strProp[keyname] = row[j];
            }
        }
        
        cfgTask->cfgid = int(cfgTask->getPropNum("cfgid"));
        cfgTask->taskLine = int(cfgTask->getPropNum("taskLine"));
        
        m_TaskCfgs[cfgTask->cfgid] = cfgTask;
        m_type2TaskCfg[cfgTask->taskLine][cfgTask->cfgid] = cfgTask;
    }
    return true;
}

TaskObjPtr TaskCtrl::getTask(int cfgid){
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.find(cfgid);
    if (it != m_allTasks.end()){
        return it->second;
    }
    return NULL;
}

void TaskCtrl::addTask(TaskObjPtr Task){
    m_allTasks[Task->taskCfg->cfgid] = Task;
}

TaskObjPtr TaskCtrl::genTask(int cfgid){
    TaskConfigPtr cfg = Singleton<TaskMgr>::instance().getCfg(cfgid);
    if (!cfg){
        return NULL;
    }
    
    TaskObjPtr Task = new TaskObj();
    Task->taskCfg = cfg;
    this->addTask(Task);
    
    return Task;
}

bool TaskCtrl::delTask(int cfgid){
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.find(cfgid);
    if (it != m_allTasks.end()){
        m_allTasks.erase(it);
        return true;
    }
    return false;
}

bool TaskCtrl::loadFromDB(const std::vector<std::string>& filedNames, const std::vector<std::vector<std::string> >& fieldDatas){
    for (size_t i = 0; i < fieldDatas.size(); ++i){
        const std::vector<std::string>& row = fieldDatas[i];
        int cfgid  = ::atoi(row[0].c_str());
        TaskObjPtr task = genTask(cfgid);
        if (!task){
            continue;
        }
        task->status   = ::atoi(row[1].c_str());
        task->value    = ::atoi(row[2].c_str());
        task->tmUpdate = ::atoi(row[1].c_str());
        m_allTasks[task->taskCfg->cfgid] = task;
    }
    return true;
}
static void handleEntityDataLoadBegin(EntityDataLoadBegin& e){
    char sql[512] = {0};
    snprintf(sql, sizeof(sql), "select cfgid,status,value,updatetime from task where uid = '%lu'", e.entity->getUid());
    e.moduleLoadSql[e.entity->get<TaskCtrl>()->getFieldName()].push_back(sql);
}
static void handleEntityDataLoadEnd(EntityDataLoadEnd& e){
    std::vector<EntityDataLoadEnd::SqlResult>& sqlResult = e.moduleDatas[e.entity->get<TaskCtrl>()->getFieldName()];
    if (false == sqlResult.empty()){
        e.entity->get<TaskCtrl>()->loadFromDB(sqlResult[0].fieldNames, sqlResult[0].fieldDatas);
    }
    else{
        std::vector<std::string> tmp1;
        std::vector<std::vector<std::string> > tmp2;
    }
}

static bool initEnvir(){
    EVENT_BUS_LISTEN(&handleEntityDataLoadBegin);
    EVENT_BUS_LISTEN(&handleEntityDataLoadEnd);
    return true;
}
WORKER_AT_SETUP(initEnvir);
