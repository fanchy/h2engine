#include "server/shared_mem.h"
using namespace ff;
using namespace std;


SharedSyncmemMgr::~SharedSyncmemMgr()
{
    wroker_shared_data_t* pshared_mem = get_self_sharedmem();
    if (pshared_mem)
    {
        pshared_mem->pid = 0;
    }
    
    if (m_master_shm != 0){       
        ::shmdt(m_master_shm);
        m_worker_shmid = -1;
        m_master_shm = NULL;
        m_worker_shm = NULL;
    }
}

int SharedSyncmemMgr::init_master(int shared_key)
{
    printf("sharedmem m_worker_num=%d\n", m_worker_num);
    m_worker_shmid = ::shmget((key_t)shared_key, sizeof(MasterSharedData) + sizeof(struct wroker_shared_data_t)*m_worker_num, 0666);  
    bool clean_oldlock = false;
    if (m_worker_shmid != -1)//!存在
    {
        printf("sharedmem exist reuse\n");
        clean_oldlock = true;
    }
    else
    {
        m_worker_shmid = ::shmget((key_t)shared_key, sizeof(MasterSharedData) + sizeof(struct wroker_shared_data_t)*m_worker_num, 0666|IPC_CREAT);  
        if(m_worker_shmid == -1)
        {
            if (EINVAL == errno)//!size 变大了,需要先删除老的
            {
                printf("sharedmem realloc\n");
                int oldid = ::shmget((key_t)shared_key,  1, 0666); 
                if (oldid != - 1){
                    ::shmctl(oldid, IPC_RMID, NULL);
                    m_worker_shmid = ::shmget((key_t)shared_key, sizeof(MasterSharedData) + sizeof(struct wroker_shared_data_t)*m_worker_num, 0666|IPC_CREAT);  
                }
            }
            if(m_worker_shmid == -1)
            {
                perror("shmget failed\n");
                return -1;
            }
        }
    }

    void* p = shmat(m_worker_shmid, 0, 0);
    m_master_shm   = (MasterSharedData*)p;
    m_worker_shm   = (wroker_shared_data_t*)((const char*)p + sizeof(MasterSharedData));

    //!销毁老的锁
    if (clean_oldlock){
        m_master_shm->cleanup();
    }
    m_master_shm->init();

    for (int i = 0; i < m_worker_num; ++i)
    {
        wroker_shared_data_t* p = m_worker_shm+i;
        
        if (clean_oldlock){//!销毁老的锁
            p->cleanup();
        }
        p->init();
    }
    return 0;
}

int SharedSyncmemMgr::init_worker(int shared_key, int work_index_, TaskQueue* tq ){
    m_worker_shmid = shmget((key_t)shared_key, sizeof(MasterSharedData) + sizeof(struct wroker_shared_data_t)*m_worker_num, 0666);  
    if(m_worker_shmid == -1)
    {
        printf("shmget failed!\n");
        return -1;
    }

    void* p = shmat(m_worker_shmid, 0, 0);
    m_master_shm   = (MasterSharedData*)p;
    m_worker_shm   = (wroker_shared_data_t*)((const char*)p + sizeof(MasterSharedData));
    
    m_worker_index = work_index_;
    
    wroker_shared_data_t* workdata = get_self_sharedmem();
    workdata->pid = ::getpid();
    
    m_thread.create_thread(TaskBinder::gen(&SharedSyncmemMgr::workerRun, this, tq), 1);
    return 0;
}

int SharedSyncmemMgr::processCmdQueue()
{
    vector<SyncCmdData> tmplist;
    LockGuard guard(m_mutex);
    if (m_sync_cmd_queue.empty()){
        return 0;
    }
    tmplist = m_sync_cmd_queue;
    m_sync_cmd_queue.clear();
    
    for (unsigned int i = 0; i < tmplist.size(); ++i)
    {
        SyncCmdData& cmddata = tmplist[i];
        //printf("process queue update %lu, %lu, %s\n", cmddata.proptype, cmddata.propkey, cmddata.body.c_str());
        if (m_notify_func)
        {
            m_notify_func(cmddata.cmd, cmddata.body);
        }
    }
    
    
    return 0;
}

int SharedSyncmemMgr::workerRun(TaskQueue* tq){
    wroker_shared_data_t* pshared_mem = get_self_sharedmem();

    while (m_running && pshared_mem)
    {
        printf("SharedSyncmemMgr::workerRun.......\n");
        if (false == pshared_mem->waitsignal())//!no signal
        {
            sleep(1);
            continue;
        }
        if (false == m_running)
        {
            break;
        }
        if (pshared_mem->cmd != 0)
        {
            SyncCmdData cmddata;
            cmddata.cmd = pshared_mem->cmd;
            cmddata.body.assign(pshared_mem->body, pshared_mem->len);
            {
                LockGuard guard(m_mutex);
                m_sync_cmd_queue.push_back(cmddata);
            }
            if (tq){
                tq->post(TaskBinder::gen(&SharedSyncmemMgr::processCmdQueue, this));
            }
            pshared_mem->cmd = 0;
            pshared_mem->len = 0;
        }
    }
    //printf("SharedSyncmemMgr::workerRun exit\n");
    pshared_mem->pid = 0;
    return 0;
}
bool SharedSyncmemMgr::syncSharedData(int32_t cmd, const std::string& data){

    for (int i = 0; i < m_worker_num; ++i)
    {
        if (i != m_worker_index)
        {
            //printf("SharedSyncmemMgr::syncSharedData i =%d\n", i);
            wroker_shared_data_t* workdata = m_worker_shm + i;
            if (workdata->pid != 0)
            {
                //printf("sync data to worker[%d]%s\n", i, data.c_str());
                workdata->cmd      = cmd;
                workdata->len      = data.size();
                ::memcpy(workdata->body, data.c_str(), ::min(sizeof(workdata->body), data.size()));
                
                workdata->locksignal();//!发信号通知
                //printf("sync data to worker[%d]%s end ok\n", i, data.c_str());
            }
        }
    }

    bool ok = true;
    //!数据复制完毕后，检测对方进程是否都已经拷贝数据完毕
    for (int m = 0; m < 100; ++m)//!最多等待10毫秒
    {
        ok = true;
        for (int i = 0; i < m_worker_num; ++i)
        {
            if (i != m_worker_index)
            {
                wroker_shared_data_t* workdata = m_worker_shm + i;
                if (workdata->cmd != 0)//!还没有复制完毕数据
                {
                    if (i > 10){
                        printf("wait worker[%d] copy data\n", i);
                    }
                    ok = false;
                }
            }
        }
        if (ok)
        {
            break;
        }
        else{
            ::usleep(100-m);
        }
    }
    if (!ok){
        printf("worker don't copy data\n");
    }
    else{
        //printf("worker copy data ok\n");
    }
    return true;
}
bool SharedSyncmemMgr::writeLockGuard()
{
    m_lock_guard = new writelock_gurard_t(m_master_shm);
    processCmdQueue();//!立即同步数据
    return true;
}
bool SharedSyncmemMgr::writeLockEnd(){
    m_lock_guard.reset();
    //printf("SharedSyncmemMgr::writeLockEnd end...............\n");
    return true;
}
void SharedSyncmemMgr::cleanup(){
    m_running = false;
    wroker_shared_data_t* pshared_mem = get_self_sharedmem();
    if (pshared_mem)
    {
        //printf("SharedSyncmemMgr::cleanup....\n");
        pshared_mem->locksignal();//!发信号，对方进程会自动读取数据
        m_thread.join();
        //printf("SharedSyncmemMgr::cleanup....end ok\n");
    }
}

bool MasterSharedData::timelock(int sec)
{
    struct timespec tout;
    ::clock_gettime(CLOCK_REALTIME, &tout);
    tout.tv_sec += sec;
    int err = ::pthread_mutex_timedlock(&master_lock, &tout);
    if (err != 0)//!lock failed
    {
        return false;
    }
    return true;
}
bool MasterSharedData::unlock()
{
    if (::pthread_mutex_unlock(&master_lock))
    {
        return false;
    }
    return true;
}

bool MasterSharedData::init()
{
    ::pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    ::pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    
    ::pthread_mutex_init(&(master_lock), &mutexAttr);
    
    this->pidusing = 0;
    return true;
}
void MasterSharedData::cleanup()
{
    ::pthread_mutex_destroy(&(master_lock));
}

bool wroker_shared_data_t::locksignal()
{
    /*
    struct timespec tout;
    clock_gettime(CLOCK_REALTIME, &tout);
    tout.tv_sec += 1;
    ::pthread_mutex_timedlock(&wroker_lock, &tout);
    ::pthread_cond_signal(&(worker_cond));
    ::pthread_mutex_unlock(&wroker_lock);
    */
    ::sem_post(&(this->sem));
    return true;
}
bool wroker_shared_data_t::waitsignal()
{
    /*
    struct timespec tout;
    clock_gettime(CLOCK_REALTIME, &tout);
    tout.tv_sec += 1;
    ::pthread_mutex_timedlock(&wroker_lock, &tout);//int n = pthread_mutex_lock(&wroker_lock);//
    //printf("%d %d %d %d\n", n, EINVAL, EAGAIN, EDEADLK);
    perror("wroker_shared_data_t::waitsignal....\n");
    int ret = ::pthread_cond_wait(&worker_cond, &wroker_lock);
    perror("wroker_shared_data_t::waitsignal....\n");
    ::pthread_mutex_unlock(&wroker_lock);
    */
    int ret = ::sem_wait(&this->sem);
    return ret == 0;
}
bool wroker_shared_data_t::init()
{
    /*
    ::pthread_mutexattr_t mutexAttr;
    ::pthread_mutexattr_init(&mutexAttr);
    ::pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    
    ::pthread_condattr_t condattr;
    ::pthread_condattr_init(&condattr);
    ::pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
    ::pthread_mutex_init(&(wroker_lock), &mutexAttr);
    ::pthread_cond_init(&worker_cond, NULL);
    */
    ::sem_init(&(this->sem), 1, 0);
    return true;
}
void wroker_shared_data_t::cleanup()
{
    /*
    ::pthread_mutex_destroy(&(wroker_lock));
    ::pthread_cond_destroy(&worker_cond);
    */
    ::sem_destroy(&(this->sem));
}
writelock_gurard_t::writelock_gurard_t(MasterSharedData* master):m_master_data(master), pid(0){
    m_master_data->timelock();
    pid = ::getpid();
    m_master_data->pidusing = pid;
    //printf("writelock_gurard_t...............\n");
}
writelock_gurard_t::~writelock_gurard_t(){
    if (pid == m_master_data->pidusing)
    {
        m_master_data->unlock();
        m_master_data->pidusing = 0;
    }
    //printf("~writelock_gurard_t end...............\n");
}
 /*
    SharedSyncmemMgr mgr;
    mgr.init_master(43210, 11);
    
    printf("init_master end\n");
    SharedSyncmemMgr mgr2;
    mgr2.init_worker(43210, 0);
    
    SharedSyncmemMgr mgr3;
    mgr3.init_worker(43210, 1);
    
    sleep(1);
    mgr3.allProp[1].allData[2].nData = 2;
    {
        sem_gurard_t guard(mgr3.get_master_sem());
        mgr3.sync_data(1, 2);
    }
    sleep(1);
    
    mgr3.allProp[1].allData[3].fData = 2.2;
    mgr3.allProp[1].allData[4].strData = "abc";
    prop_data_t::prop_data_ptr_t p = new prop_data_t();
    mgr3.allProp[1].allData[5].listData.push_back(p);
    
    prop_data_t::prop_data_ptr_t p2 = new prop_data_t();
    mgr3.allProp[2].allData[5].mapData[2222] = p2;
    mgr3.allProp[1].allData.erase(3);
    {
        sem_gurard_t guard(mgr3.get_master_sem());
        mgr3.sync_data(1, 3).sync_data(1, 4).sync_data(1, 5).sync_data(2, 5);
    }
    sleep(1);
    
    mgr2.processCmdQueue();
    
    mgr2.dump();
    mgr3.dump();
    
    printf("ok\n");
    
    mgr2.cleanup();
    return 0;
    */