#ifndef _FF_FFWORKER_CPP_H_
#define _FF_FFWORKER_CPP_H_

//#include <assert.h>
//#include <string>


#include "base/log.h"
#include "server/db_mgr.h"
#include "server/fftask_processor.h"
#include "server/ffworker.h"

class ffpython_t;
namespace ff
{
#define FFWORKER_CPP "FFWORKER_CPP"

class FFWorkerCpp: public FFWorker, task_processor_i
{
public:
    FFWorkerCpp();
    ~FFWorkerCpp();

    int                     close();
    int                     processInit(Mutex* mutex, ConditionVar* var, int* ret);
    int                     cppInit();

public:
    bool                    m_started;
};

}

#endif
