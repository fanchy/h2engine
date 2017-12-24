#ifndef _FF_TASK_H_
#define _FF_TASK_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "common/entity.h"
#include "base/smart_ptr.h"

namespace ff
{

struct TaskConfig{
    TaskConfig():cfgid(0), taskLine(0), nextTask(0){}
    int64_t getPropNum(const std::string& key);
    std::string getPropStr(const std::string& key);

    int             cfgid;
    int             taskLine;//!任务线id
    int             nextTask;//!后续任务id

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

enum TaskStatusDef{
    TASK_GOT    = 0, //!任务已获取
    TASK_ACCEPTED,   //!任务已接取
    TASK_FINISHED,   //!任务已经完成
    TASK_END         //!任务已经结束
};
struct TaskObj
{
    TaskObj():status(0), value(0), tmUpdate(0){}

    TaskConfigPtr       taskCfg;
    int                 status;
    int                 value;
    time_t              tmUpdate;//!上次状态改变时间
};
typedef SharedPtr<TaskObj> TaskObjPtr;

class TaskCtrl:public EntityField
{
public:
    TaskCtrl():m_nMaxNum(10){}

    void addTask(TaskObjPtr Task);
    TaskObjPtr genTask(int cfgid);
    bool delTask(int cfgid);
    
    TaskObjPtr getTask(int cfgid);
    
    int curSize() const     { return m_allTasks.size(); }
    int leftNum() const     { return m_nMaxNum - m_allTasks.size();}
 
    bool loadFromDB(const std::vector<std::string>& filedNames, const std::vector<std::vector<std::string> >& fieldDatas);
public:
    int                            m_nMaxNum;
    std::map<int, TaskObjPtr>      m_allTasks;
};

}

#endif
