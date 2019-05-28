
//脚本

#include "./ffworker_cpp.h"
#include "base/perf_monitor.h"
#include "server/http_mgr.h"
using namespace ff;
using namespace std;


FFWorkerCpp::FFWorkerCpp():m_started(false)
{
}
FFWorkerCpp::~FFWorkerCpp()
{
}
int FFWorkerCpp::cppInit()
{
    DB_MGR.start();
    ArgHelper& arg_helper = Singleton<ArgHelper>::instance();
    string dbcfg;
    if (arg_helper.isEnableOption("-db")){
        dbcfg = arg_helper.getOptionValue("-db");
    }
    else{
        dbcfg = "sqlite://./h2test.db";
        LOGERROR((FFWORKER_CPP, "FFWorkerCpp::no db config use demo db:sqlite://./h2test.db"));
    }
    
    int nDbNum = DB_THREAD_NUM;
    if (dbcfg.find("sqlite://") != std::string::npos){
        nDbNum = 1;
    }
    for (int i = 0; i < nDbNum; ++i){
        if (0 == DB_MGR.connectDB(dbcfg, DB_DEFAULT_NAME)){
            LOGERROR((FFWORKER_CPP, "FFWorkerCpp::db connect failed"));
            return -1;
            break;
        }
    }

    int ret = -2;
    
    try{
        
        Mutex                    mutex;
        ConditionVar            cond(mutex);
        
        getRpc().getTaskQueue()->post(funcbind(&FFWorkerCpp::processInit, this, &mutex, &cond, &ret));
        LOGINFO((FFWORKER_CPP, "FFWorkerCpp::begin init py"));
        LockGuard lock(mutex);
        if (ret == -2){
            cond.wait();
        }
        LOGINFO((FFWORKER_CPP, "FFWorkerCpp::processInit return"));
        if (ret < 0)
        {
            this->close();
            return -1;
        }
    }
    catch(exception& e_)
    {
        return -1;
    }
    m_started = true;
    LOGTRACE((FFWORKER_CPP, "FFWorkerCpp::scriptInit end ok"));
    return ret;
}

//!!处理初始化逻辑
int FFWorkerCpp::processInit(Mutex* mutex, ConditionVar* var, int* ret)
{
    try{
        if (this->initModule()){
            *ret = 0;
        }
        else
            *ret = -1;
    }
    catch(exception& e_)
    {
        *ret = -1;
        LOGERROR((FFWORKER_CPP, "FFWorkerCpp::open failed er=<%s>", e_.what()));
    }
    LockGuard lock(*mutex);
    var->signal();
    LOGINFO((FFWORKER_CPP, "FFWorkerCpp::processInit end"));
    return 0;
}
int FFWorkerCpp::close()
{
    FFWorker::close();
    if (false == m_started)
        return 0;
    m_started = false;

    LOGINFO((FFWORKER_CPP, "FFWorkerCpp::close end"));
    return 0;
}

