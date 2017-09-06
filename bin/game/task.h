#ifndef _FF_TASK_H_
#define _FF_TASK_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "game/entity.h"
#include "base/smart_ptr.h"

namespace ff
{

struct TaskConfig{
    TaskConfig():cfgid(0), taskType(0){}
    int64_t getPropNum(const std::string& key);
    std::string getPropStr(const std::string& key);

    int             cfgid;
    int             taskType;
    std::string     name;

    std::map<std::string, int64_t>      numProp;
    std::map<std::string, std::string>  strProp;
};
typedef SharedPtr<TaskConfig> TaskConfigPtr;


class TaskMgr
{
public:
    TaskConfigPtr getCfg(int cfgid) { return m_TaskCfgs[cfgid]; }

    bool init();
public:
    std::map<int/*cfgid*/, TaskConfigPtr>                               m_TaskCfgs;
    std::map<int/*taskType*/,  std::map<int/*cfgid*/, TaskConfigPtr> >  m_type2TaskCfg;
};

class TaskObj
{
public:
    TaskConfigPtr getCfg()          { return m_TaskConfig; }
    void setCfg(TaskConfigPtr c)    { m_TaskConfig = c;    }
    
    int getCfgId()                  {return m_TaskConfig->cfgid;    }
    int getTaskType()               {return m_TaskConfig->taskType; }
    const std::string& getCfgName() {return m_TaskConfig->name;     }
public:
    TaskConfigPtr       m_TaskConfig;
};
typedef SharedPtr<TaskObj> TaskObjPtr;

class TaskCtrl:public EntityField
{
public:
    TaskCtrl():m_nMaxNum(0){}
    virtual void onEvent(EntityPtr& entity, EntityEvent& event){}//!处理事件通知

    void addTask(TaskObjPtr Task);
    TaskObjPtr genTask(int cfgid);
    bool delTask(int cfgid);
    void clear() { m_allTasks.clear(); }
    
    TaskObjPtr getTask(int cfgid);
    int        getTask(int cfgid, std::vector<TaskObjPtr>& ret);
    TaskObjPtr getTask(const std::string& name);
    int        getTask(const std::string& name, std::vector<TaskObjPtr>& ret);
    
    TaskObjPtr getTaskByProp(const std::string& key, int64_t propVal);
    TaskObjPtr getTaskByProp(const std::string& key, const std::string& propVal);
    int       getTaskByProp(const std::string& key, int propVal, std::vector<TaskObjPtr>& ret);
    int       getTaskByProp(const std::string& key, const std::string& propVal, std::vector<TaskObjPtr>& ret);
    
    int curSize() const     { return m_allTasks.size(); }
    int maxNum()  const     { return m_nMaxNum;         }
    void setMaxNum(int n)   { m_nMaxNum  = n;           }
    int leftNum() const     { return m_nMaxNum - m_allTasks.size();}
    
    template<typename T>
    void foreach(T func){
        for (std::map<int, TaskObjPtr>::iterator it = m_allTasks.begin(); it != m_allTasks.end(); ++it){
            func(it->second);
        }
    }
public:
    int                                 m_nMaxNum;
    std::map<int, TaskObjPtr>      m_allTasks;
};

}

#endif
