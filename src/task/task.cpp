#include <ctype.h>
#include <algorithm>

#include "task/task.h"
#include "server/db_mgr.h"
#include "common/game_event.h"
#include "server/ffworker.h"
#include "base/log.h"
#include "base/time_tool.h"

using namespace ff;
using namespace std;

int64_t TaskConfig::getPropNum(const std::string& key){
    string v = getPropStr(key);
    if (v.empty() == false){
        return ::atol(v.c_str());
    }
    return 0;
}

string TaskConfig::getPropStr(const std::string& key){
    string keyname = key;
    std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
    std::map<std::string, std::string>::iterator it = allProp.find(keyname);
    if (it != allProp.end()){
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
    DB_MGR_OBJ.syncQueryDBGroupMod(CFG_DB, 0, sql, retdata, col, errinfo, affectedRows);
    
    if (errinfo.empty() == false)
    {
        LOGERROR((GAME_LOG, "TaskMgr::init failed:%s", errinfo));
        return false;
    }
    if (retdata.empty()){
        LOGWARN((GAME_LOG, "TaskMgr::init load none data"));
        return true;
    }
    
    allTaskCfg.clear();
    taskLine2Task.clear();
    map<int, vector<TaskConfigPtr> > tmpLine2task;
    for (size_t i = 0; i < retdata.size(); ++i){
        vector<string>& row = retdata[i];
        TaskConfigPtr taskCfg = new TaskConfig();
        
        for (size_t j = 0; j < row.size(); ++j){
            string& keyname = col[j];
            std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
            taskCfg->allProp[keyname] = row[j];
        }
        
        taskCfg->cfgid    = (int)taskCfg->getPropNum("cfgid");
        taskCfg->taskLine = (int)taskCfg->getPropNum("taskLine");
        taskCfg->nextTask = (int)taskCfg->getPropNum("nextTask");
        
        taskCfg->triggerPropertyType   = taskCfg->getPropStr("triggerPropertyType");
        taskCfg->triggerPropertyValue  = (int)taskCfg->getPropNum("triggerPropertyValue");
        taskCfg->finishConditionType   = taskCfg->getPropStr("finishConditionType");
        taskCfg->finishConditionObject = taskCfg->getPropStr("finishConditionObject");
        taskCfg->finishConditionValue  = (int)taskCfg->getPropNum("finishConditionValue");
        
        if (taskCfg->finishConditionObject.empty() == false &&
            taskCfg->finishConditionObject[taskCfg->finishConditionObject.size() - 1] != ','){
            taskCfg->finishConditionObject += ",";
        }
        allTaskCfg[taskCfg->cfgid] = taskCfg;
        tmpLine2task[taskCfg->taskLine].push_back(taskCfg);
    }
    for (map<int, vector<TaskConfigPtr> >::iterator it = tmpLine2task.begin(); it != tmpLine2task.end(); ++it){
        TaskConfigPtr firstTask = it->second[0];
        for (size_t i = 0; i < it->second.size(); ++i){
            TaskConfigPtr preTask;
            for (size_t j = 0; j < it->second.size(); ++j){
                if (it->second[j]->nextTask == firstTask->cfgid){
                    preTask = it->second[j];
                    break;
                }
            }
            if (!preTask){
                break;
            }
            firstTask = preTask;
        }
        taskLine2Task[it->first] = firstTask;
        LOGINFO((GAME_LOG, "TaskMgr::init load taskline %d->%d", it->first, firstTask->cfgid));
    }
    return true;
}
static  bool isCondititonObjectEqual(const string& cfgObject, const string& strTriggerObject){
    std::size_t it = cfgObject.find(strTriggerObject);
    if (it == string::npos || it >= (cfgObject.size() - 1) || strTriggerObject[it+1] != ','){
        return false;
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

TaskObjPtr TaskCtrl::genTask(int cfgid, int status){
    TaskConfigPtr cfg = TASK_MGR.getCfg(cfgid);
    if (!cfg){
        return NULL;
    }
    
    TaskObjPtr task = new TaskObj();
    task->taskCfg   = cfg;
    task->status    = status;
    task->tmUpdate  = ::time(NULL);
    this->addTask(task);
    
    char sql[512] = {0};
    snprintf(sql, sizeof(sql), "insert into task (`uid`, `cfgid`, `status`, `value`, `tmUpdate`) values (%lu, %d, %d, %d, '%s')",
                                this->getOwner()->getUid(), task->taskCfg->cfgid, task->status, task->value, TimeTool::formattm(task->tmUpdate).c_str());
    
    DB_MGR_OBJ.queryDBGroupMod(USER_DB, this->getOwner()->getUid(), sql);

    TaskStatusChange eTask(this->getOwner(), task->taskCfg->cfgid, task->status);
    EVENT_BUS_FIRE(eTask);
    return task;
}

bool TaskCtrl::delTask(int cfgid){
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.find(cfgid);
    if (it != m_allTasks.end()){
        m_allTasks.erase(it);
        char sql[512] = {0};
        snprintf(sql, sizeof(sql), "delete from task where `uid` = '%lu' and `cfgid` = '%d' ",
                                    this->getOwner()->getUid(), cfgid);
        
        DB_MGR_OBJ.queryDBGroupMod(USER_DB, this->getOwner()->getUid(), sql);
        return true;
    }
    return false;
}

bool TaskCtrl::changeTaskStatus(TaskObjPtr task, int status){
    if (!task)
        return false;
    task->status  = status;
    task->tmUpdate= ::time(NULL);
    char sql[512] = {0};
    snprintf(sql, sizeof(sql), "update task set status = %d, tmUpdate = '%s' where `uid` = '%lu' and `cfgid` = '%d' ",
                                status, TimeTool::formattm(task->tmUpdate).c_str(), this->getOwner()->getUid(), task->taskCfg->cfgid);
    
    DB_MGR_OBJ.queryDBGroupMod(USER_DB, this->getOwner()->getUid(), sql);
    TaskStatusChange eTask(this->getOwner(), task->taskCfg->cfgid, task->status);
    EVENT_BUS_FIRE(eTask);
    return true;
}
bool TaskCtrl::acceptTask(int cfgid){
    TaskObjPtr task = getTask(cfgid);
    if (!task || task->status != TASK_GOT){
        return false;
    }
    changeTaskStatus(task, TASK_FINISHED);
    return true;
}
bool TaskCtrl::finishTask(int cfgid){
    TaskObjPtr task = getTask(cfgid);
    if (!task || task->status != TASK_ACCEPTED){
        return false;
    }
    changeTaskStatus(task, TASK_FINISHED);
    if (task->taskCfg->nextTask){
        TaskObjPtr nextTask = this->getTask(task->taskCfg->nextTask);
        if (nextTask){
            this->delTask(cfgid);
        }
    }
    return true;
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
        task->tmUpdate = TimeTool::str2time(row[3]);
        m_allTasks[task->taskCfg->cfgid] = task;
    }
    
    return true;
}
bool TaskCtrl::checkNewTask(){
    //!检查每一条任务线，是否有新的任务线可以加入
    map<int/*taskLine*/,  TaskConfigPtr> allTaskLine = TASK_MGR.taskLine2Task;
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin();
    for (; it != m_allTasks.end(); ++it){
        allTaskLine.erase(it->second->taskCfg->cfgid);
    }
    map<int/*taskLine*/,  TaskConfigPtr>::iterator it2 = allTaskLine.begin();
    for (; it2 != TASK_MGR.taskLine2Task.end(); ++it2){
        //!检查属性是否满足, 如果满足增加任务
        TaskConfigPtr& taskCfg = it2->second;
        
        if (PROP_MGR.get(this->getOwner(), taskCfg->triggerPropertyType) >= taskCfg->triggerPropertyValue)
        {
            this->genTask(taskCfg->cfgid);
        }
    }
    return true;
}
int TaskCtrl::triggerEvent(const std::string& triggerType, const std::string& triggerObject, int value, std::vector<int>* changeTask){
    //!检查自己的任务有没有可以完成的
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin();
    for (; it != m_allTasks.end(); ++it){
        TaskObj& task = *(it->second);
        if (task.status != TASK_ACCEPTED){
            continue;
        }
        TaskConfig& taskCfg = *task.taskCfg;
        if (taskCfg.finishConditionType == triggerType && isCondititonObjectEqual(taskCfg.finishConditionObject, triggerObject)){
            task.value += value;
            if (task.value >= taskCfg.finishConditionValue){
                task.value = taskCfg.finishConditionValue;
                task.status = TASK_FINISHED;
            }
            if (changeTask){
                (*changeTask).push_back(taskCfg.cfgid);
            }
        }
    }
    return 0;
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
        e.entity->get<TaskCtrl>()->loadFromDB(tmp1, tmp2);
    }
}

static bool initEnvir(){
    EVENT_BUS_LISTEN(&handleEntityDataLoadBegin);
    EVENT_BUS_LISTEN(&handleEntityDataLoadEnd);

    //!注册操作任务的脚本接口
    if (false == TASK_MGR.init()){
        return false;
    }
    struct ScriptFunctor{
        static int genTask(EntityPtr p, int cfgid){
            if (p->get<TaskCtrl>()->genTask(cfgid)){
                return cfgid;
            }
            return 0;
        }
        static bool delTask(EntityPtr p, int cfgid){
            return p->get<TaskCtrl>()->delTask(cfgid);
        }
        static bool changeTaskStatus(EntityPtr p, int cfgid, int status){
            return p->get<TaskCtrl>()->changeTaskStatus(p->get<TaskCtrl>()->getTask(cfgid), status);
        }
        static vector<int> getTask(EntityPtr p, int cfgid){
            vector<int> ret;
            TaskObjPtr task = p->get<TaskCtrl>()->getTask(cfgid);
            if (task){
                ret.push_back(task->status);
                ret.push_back(task->value);
                ret.push_back(task->taskCfg->finishConditionValue);
            }
            return ret;
        }
        static bool acceptTask(EntityPtr p, int cfgid){
            return p->get<TaskCtrl>()->acceptTask(cfgid);
        }
        static bool finishTask(EntityPtr p, int cfgid){
            return p->get<TaskCtrl>()->finishTask(cfgid);
        }
        static bool checkNewTask(EntityPtr p){
            return p->get<TaskCtrl>()->checkNewTask();
        }
    };          
    SCRIPT_UTIL.reg("Task.genTask",            ScriptFunctor::genTask);
    SCRIPT_UTIL.reg("Task.delTask",            ScriptFunctor::delTask);
    SCRIPT_UTIL.reg("Task.changeTaskStatus",   ScriptFunctor::changeTaskStatus);
    SCRIPT_UTIL.reg("Task.getTask",            ScriptFunctor::getTask);
    SCRIPT_UTIL.reg("Task.acceptTask",         ScriptFunctor::acceptTask);
    SCRIPT_UTIL.reg("Task.finishTask",         ScriptFunctor::finishTask);
    SCRIPT_UTIL.reg("Task.checkNewTask",       ScriptFunctor::checkNewTask);
    
    return true;
}
WORKER_AT_SETUP(initEnvir);

