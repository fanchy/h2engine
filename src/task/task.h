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
    TaskConfig():cfgid(0), taskLine(0), nextTask(0), triggerPropertyValue(0), finishConditionValue(0){}
    int64_t getPropNum(const std::string& key);
    std::string getPropStr(const std::string& key);

    int             cfgid;
    int             taskLine;//!任务线id
    int             nextTask;//!后续任务id

    std::string     triggerPropertyType;        //!触发任务属性类型 
    int             triggerPropertyValue;       //!触发任务属性目标值

    std::string     finishConditionType;        //!完成条件类型
    std::string     finishConditionObject;
    int             finishConditionValue;
    
    std::map<std::string, std::string>  allProp;
};
typedef SharedPtr<TaskConfig> TaskConfigPtr;


class TaskMgr
{
public:
    TaskConfigPtr getCfg(int cfgid) { return allTaskCfg[cfgid]; }

    bool init();
public:
    std::map<int/*cfgid*/, TaskConfigPtr>                               allTaskCfg;
    std::map<int/*taskLine*/,  TaskConfigPtr>                           taskLine2Task;
};
#define gTaskMgr Singleton<TaskMgr>::instance()

enum TaskStatusDef{
    TASK_GOT    = 0, //!任务已获取
    TASK_ACCEPTED,   //!任务已接取
    TASK_FINISHED,   //!任务已经完成
    TASK_END         //!任务已经结束
};
struct TaskObj
{
    TaskObj():status(TASK_GOT), value(0), tmUpdate(0){}

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

    void            addTask(TaskObjPtr Task);
    TaskObjPtr      genTask(int cfgid, int status = TASK_GOT);
    bool            delTask(int cfgid);
    bool            changeTaskStatus(TaskObjPtr task, int status);
    TaskObjPtr      getTask(int cfgid);
    bool            acceptTask(int cfgid);     //!接受任务
    bool            finishTask(int cfgid);     //!完成任务
    bool            checkNewTask();            //!检查是否有新任务
    
    int curSize() const     { return m_allTasks.size(); }
    int leftNum() const     { return m_nMaxNum - m_allTasks.size();}
 
    bool loadFromDB(const std::vector<std::string>& filedNames, const std::vector<std::vector<std::string> >& fieldDatas);
    int triggerEvent(const std::string& triggerType, const std::string& triggerObject, int value = 1,
                     std::vector<int>* changeTask = NULL);
public:
    int                            m_nMaxNum;
    std::map<int, TaskObjPtr>      m_allTasks;
};

}

#endif
