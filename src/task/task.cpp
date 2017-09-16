#include <ctype.h>

#include "task/task.h"
#include "server/db_mgr.h"

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
        cfgTask->taskType = int(cfgTask->getPropNum("taskType"));
        cfgTask->name  = cfgTask->getPropStr("name");
        
        m_TaskCfgs[cfgTask->cfgid] = cfgTask;
        m_type2TaskCfg[cfgTask->taskType][cfgTask->cfgid] = cfgTask;
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

int TaskCtrl::getTask(int cfgid, std::vector<TaskObjPtr>& ret){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->cfgid == cfgid){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

TaskObjPtr TaskCtrl::getTask(const string& name){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->name == name){
            return it->second; 
        }
    }
    return NULL;
}
int TaskCtrl::getTask(const string& name, std::vector<TaskObjPtr>& ret){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->name == name){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

TaskObjPtr TaskCtrl::getTaskByProp(const std::string& key, int64_t propVal){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->getPropNum(key) == propVal){
            return it->second; 
        }
    }
    return NULL;
}
TaskObjPtr TaskCtrl::getTaskByProp(const std::string& key, const std::string& propVal){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->getPropStr(key) == propVal){
            return it->second; 
        }
    }
    return NULL;
}
int TaskCtrl::getTaskByProp(const std::string& key, int propVal, std::vector<TaskObjPtr>& ret){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->getPropNum(key) == propVal){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}
int TaskCtrl::getTaskByProp(const std::string& key, const std::string& propVal, std::vector<TaskObjPtr>& ret){
    for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
        if (it->second->getCfg()->getPropStr(key) == propVal){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

void TaskCtrl::addTask(TaskObjPtr Task){
    m_allTasks[Task->getCfgId()] = Task;
}

TaskObjPtr TaskCtrl::genTask(int cfgid){
    TaskConfigPtr cfg = Singleton<TaskMgr>::instance().getCfg(cfgid);
    if (!cfg){
        return NULL;
    }
    
    TaskObjPtr Task = new TaskObj();
    Task->setCfg(cfg);
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
