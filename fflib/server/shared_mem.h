
#ifndef _FF_SHARED_MEM_H_
#define _FF_SHARED_MEM_H_

#include <stdio.h>

#include "base/fftype.h"
#include "base/daemon_tool.h"
#include "base/arg_helper.h"
#include "base/str_tool.h"
#include "base/smart_ptr.h"

#include "base/log.h"
#include "base/signal_helper.h"
#include "base/daemon_tool.h"
#include "base/perf_monitor.h"

#include "rpc/ffrpc.h"
#include "rpc/ffbroker.h"
#include "server/shared_mem.h"

#include "rapidjson/document.h"     // rapidjson's DOM-style API                                                                                             
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/writer.h" // for stringify JSON
#include "rapidjson/filestream.h"   // wrapper of C stream for prettywriter as output
#include "rapidjson/stringbuffer.h"   // wrapper of C stream for prettywriter as output
#include "base/lock.h"
#include "base/event_bus.h"


#include <semaphore.h>
#ifndef _WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
namespace ff
{
#define SHARED_MEM_RESERVED_ID 0
#define MAX_WORKER_NUM  5
struct MasterSharedData
{
    bool timelock(int sec = 1);
    bool unlock();
    bool init();
    void cleanup();
    
    MasterSharedData():pidusing(0){}
    ::pthread_mutex_t     master_lock;//!写锁
    uint32_t              pidusing;   //!正在使用信号量的进程ID
};
struct wroker_shared_data_t
{
    wroker_shared_data_t():pid(0), cmd(0), len(0){
        memset(body, 0, sizeof(body));
    }
    bool locksignal();
    bool waitsignal();
    bool init();
    void cleanup();
    
    //::pthread_mutex_t     wroker_lock;//!条件变量锁
    //::pthread_cond_t      worker_cond;
    
    ::sem_t               sem;
    uint32_t    pid;
    int32_t     cmd;
    uint32_t    len;
    char body[1024*1024*5];
};

struct writelock_gurard_t
{
    writelock_gurard_t(MasterSharedData* master);
    ~writelock_gurard_t();
    MasterSharedData* m_master_data;
    uint32_t              pid;
};
typedef SharedPtr<writelock_gurard_t>   writelock_gurard_ptr_t;

class SharedSyncmemMgr{
public:
    typedef void (*notify_func_t)(int32_t , const std::string&);

    struct SyncCmdData{
        int32_t       cmd;
        std::string   body;
    };
    
    SharedSyncmemMgr():m_running(true), m_worker_num(MAX_WORKER_NUM), m_worker_index(-1), m_worker_shmid(-1), m_master_shm(NULL), m_worker_shm(NULL), m_notify_func(NULL){}
    ~SharedSyncmemMgr();
    int init_master(int shared_key);
    
    int init_worker(int shared_key, int work_index_, TaskQueue* tq = NULL);

    int processCmdQueue();
    
    wroker_shared_data_t* get_self_sharedmem(){
        if (m_worker_index == -1 || m_worker_shm == NULL)
            return NULL;
        return m_worker_shm + m_worker_index;
    }
    ::pthread_mutex_t* get_master_lock(){
        return &m_master_shm->master_lock;
    }
    
    int workerRun(TaskQueue* tq = NULL);
    bool syncSharedData(int32_t cmd, const std::string& data);
    void cleanup();

    
    void setNotifyFunc(notify_func_t f) { m_notify_func = f;}

    bool writeLockGuard();
    bool writeLockEnd();
    
public:
    int                                         m_running;
    int                                         m_worker_num;
    int                                         m_worker_index;
    int                                         m_worker_shmid;
    MasterSharedData*                           m_master_shm;
    wroker_shared_data_t*                       m_worker_shm;
  
    std::vector<SyncCmdData>                    m_sync_cmd_queue;
    Thread                                      m_thread;
    Mutex                                       m_mutex;
    
    notify_func_t                               m_notify_func;
    
    writelock_gurard_ptr_t                      m_lock_guard;
};
class OnSyncSharedData:public Event<OnSyncSharedData>
{
public:
    OnSyncSharedData(uint16_t c, const std::string& b)
        :cmd(c), body(b), isDone(false){}
    uint16_t            cmd;
    const std::string&  body;
    bool                isDone;
};

}

#endif

