#include <ctype.h>
#include <algorithm>

#include "task/task.h"
#include "server/db_mgr.h"
#include "common/game_event.h"
#include "server/ffworker.h"
#include "base/log.h"
#include "base/time_tool.h"
#include "base/os_tool.h"

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
    string csvData;
    OSTool::readFile("config/task_config.csv", csvData);
    StrTool::loadCsvFromString(csvData, retdata);

    if (retdata.empty()){
        LOGWARN((GAME_LOG, "TaskMgr::init load none data"));
        return true;
    }
    
    allTaskCfg.clear();
    taskLine2Task.clear();
    map<int, vector<TaskConfigPtr> > tmpLine2task;
    vector<string>& col = retdata[0];
    for (size_t i = 1; i < retdata.size(); ++i){
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
    if (it == string::npos || it+strTriggerObject.size() > (cfgObject.size() - 1) || cfgObject[it+strTriggerObject.size()] != ','){
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
    
    DB_MGR.asyncQueryModId(this->getOwner()->getUid(), sql);

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
        
        DB_MGR.asyncQueryModId(this->getOwner()->getUid(), sql);
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
    
    DB_MGR.asyncQueryModId(this->getOwner()->getUid(), sql);
    TaskStatusChange eTask(this->getOwner(), task->taskCfg->cfgid, task->status);
    EVENT_BUS_FIRE(eTask);
    return true;
}
bool TaskCtrl::acceptTask(int cfgid){
    TaskObjPtr task = getTask(cfgid);
    if (!task || task->status != TASK_GOT){
        return false;
    }
    changeTaskStatus(task, TASK_ACCEPTED);
    return true;
}
bool TaskCtrl::endTask(int cfgid){
    TaskObjPtr task = getTask(cfgid);
    if (!task || task->status != TASK_FINISHED){
        return false;
    }
    changeTaskStatus(task, TASK_END);
    if (task->taskCfg->nextTask){
        TaskObjPtr nextTask = this->genTask(task->taskCfg->nextTask);
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
int TaskCtrl::triggerEvent(const std::string& triggerType, const std::string& triggerObject, int value){
    //!检查自己的任务有没有可以完成的
    std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin();
    for (; it != m_allTasks.end(); ++it){
        TaskObj& task = *(it->second);
        if (task.status != TASK_ACCEPTED){
            continue;
        }
        TaskConfig& taskCfg = *task.taskCfg;
        //LOGINFO((GAME_LOG, "TaskMgr::triggerEvent change task uid:%d,cfgid:%d,triggerObject:%s,value:%d", this->getOwner()->getUid(), task.taskCfg->cfgid, triggerObject, taskCfg.finishConditionObject));
        if (taskCfg.finishConditionType == triggerType && isCondititonObjectEqual(taskCfg.finishConditionObject, triggerObject)){
            task.value += value;
            if (task.value >= taskCfg.finishConditionValue){
                task.value = taskCfg.finishConditionValue;
                changeTaskStatus(it->second, TASK_FINISHED);
            }
            
            LOGINFO((GAME_LOG, "TaskMgr::triggerEvent change task uid:%d,cfgid:%d,status:%d,value:%d", this->getOwner()->getUid(), task.taskCfg->cfgid, task.status, task.value));
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
            vector<int>  ret;
            TaskObjPtr task = p->get<TaskCtrl>()->getTask(cfgid);
            if (task){
                ret.push_back(task->status);
                ret.push_back(task->value);
                ret.push_back(task->taskCfg->finishConditionValue);
            }
            return ret;
        }
        static vector<vector<int> > getTaskByStatus(EntityPtr p, int status){
            vector<vector<int> > ret;
            std::map<int, TaskObjPtr>&      m_allTasks = p->get<TaskCtrl>()->m_allTasks;
            for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
                TaskObjPtr task = it->second;
                if (task->status != status)
                    continue;
                vector<int> tmp;
                tmp.push_back(task->status);
                tmp.push_back(task->value);
                tmp.push_back(task->taskCfg->finishConditionValue);
                ret.push_back(tmp);
            }
            
            return ret;
        }
        static bool acceptTask(EntityPtr p, int cfgid){
            return p->get<TaskCtrl>()->acceptTask(cfgid);
        }
        static bool endTask(EntityPtr p, int cfgid){
            return p->get<TaskCtrl>()->endTask(cfgid);
        }
        static bool checkNewTask(EntityPtr p){
            return p->get<TaskCtrl>()->checkNewTask();
        }
        static void triggerEvent(EntityPtr e, const std::string& triggerType, const std::string& triggerObject, int value){
            e->get<TaskCtrl>()->triggerEvent(triggerType, triggerObject, value);
        }
        static map<string, string> getTaskCfg(int cfgid){
            map<string, string> ret;
            TaskConfigPtr taskCfg = TASK_MGR.getCfg(cfgid);
            if (taskCfg){
                ret.insert(taskCfg->allProp.begin(), taskCfg->allProp.end());
                ret["cfgid"] = StrTool::num2str(taskCfg->cfgid);
                ret["taskLine"] = StrTool::num2str(taskCfg->taskLine);
                ret["nextTask"] = StrTool::num2str(taskCfg->nextTask);
                ret["triggerPropertyType"] = StrTool::num2str(taskCfg->triggerPropertyType);
                ret["triggerPropertyValue"] = StrTool::num2str(taskCfg->triggerPropertyValue);
                ret["finishConditionType"] = StrTool::num2str(taskCfg->finishConditionType);
                ret["finishConditionObject"] = StrTool::num2str(taskCfg->finishConditionObject);
                ret["finishConditionValue"] = StrTool::num2str(taskCfg->finishConditionValue);
            }
            return ret;
        }
        static bool addTaskCfg(map<string, string>& args){
            TaskConfigPtr taskCfg = new TaskConfig();
        
            for (map<string, string>::iterator it = args.begin(); it != args.end(); ++it){
                string keyname = it->first;
                std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
                taskCfg->allProp[keyname] = it->second;
            }
            
            taskCfg->cfgid    = (int)taskCfg->getPropNum("cfgid");
            taskCfg->taskLine = (int)taskCfg->getPropNum("taskLine");
            taskCfg->nextTask = (int)taskCfg->getPropNum("nextTask");
            //LOGINFO((GAME_LOG, "task cfgid:%d, nextTask:%d", taskCfg->cfgid, taskCfg->nextTask));
            
            taskCfg->triggerPropertyType   = taskCfg->getPropStr("triggerPropertyType");
            taskCfg->triggerPropertyValue  = (int)taskCfg->getPropNum("triggerPropertyValue");
            taskCfg->finishConditionType   = taskCfg->getPropStr("finishConditionType");
            taskCfg->finishConditionObject = taskCfg->getPropStr("finishConditionObject");
            taskCfg->finishConditionValue  = (int)taskCfg->getPropNum("finishConditionValue");
            
            if (taskCfg->finishConditionObject.empty() == false &&
                taskCfg->finishConditionObject[taskCfg->finishConditionObject.size() - 1] != ','){
                taskCfg->finishConditionObject += ",";
            }
            TASK_MGR.allTaskCfg[taskCfg->cfgid] = taskCfg;
            return true;
        }
    };          
    SCRIPT_UTIL.reg("Task.genTask",            ScriptFunctor::genTask);
    SCRIPT_UTIL.reg("Task.delTask",            ScriptFunctor::delTask);
    SCRIPT_UTIL.reg("Task.changeTaskStatus",   ScriptFunctor::changeTaskStatus);
    SCRIPT_UTIL.reg("Task.getTask",            ScriptFunctor::getTask);
    SCRIPT_UTIL.reg("Task.getTaskByStatus",    ScriptFunctor::getTaskByStatus);
    SCRIPT_UTIL.reg("Task.acceptTask",         ScriptFunctor::acceptTask);
    SCRIPT_UTIL.reg("Task.endTask",            ScriptFunctor::endTask);
    SCRIPT_UTIL.reg("Task.checkNewTask",       ScriptFunctor::checkNewTask);
    SCRIPT_UTIL.reg("Task.triggerEvent",       ScriptFunctor::triggerEvent);
    SCRIPT_UTIL.reg("Task.getTaskCfg",         ScriptFunctor::getTaskCfg);
    SCRIPT_UTIL.reg("Task.addTaskCfg",         ScriptFunctor::addTaskCfg);
    
    //EntityPtr entity = NEW_ENTITY(1, 1);
    //SCRIPT_UTIL.callScript<void>("testTask", entity);
    return true;
}
WORKER_AT_SETUP(initEnvir);

