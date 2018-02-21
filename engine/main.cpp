#include <stdio.h>

#include "base/fftype.h"
#include "base/daemon_tool.h"
#include "base/arg_helper.h"
#include "base/str_tool.h"
#include "base/smart_ptr.h"

#include "base/log.h"
#include "base/signal_helper.h"
#include "base/daemon_tool.h"
#include "base/performance_daemon.h"

#include "rpc/ffrpc.h"
#include "rpc/ffbroker.h"
#include "server/ffgate.h"
#include "server/shared_mem.h"


using namespace ff;
using namespace std;

#ifdef _WIN32
static bool g_flagwait = true;
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT:
	case SIGTERM:
	case 2: 
      printf( "Ctrl-C event\n\n" );
      g_flagwait = false;
      return( TRUE );
    default: 
      printf( "recv=%d please use Ctrl-C \n\n", fdwCtrlType );
      return FALSE; 
  } 
} 
static bool flagok = false;
#endif

int main(int argc, char* argv[])
{
   
	ArgHelper arg_helper(argc, argv);
    if (arg_helper.isEnableOption("-f"))
    {
        arg_helper.loadFromFile(arg_helper.getOptionValue("-f"));
    }

    if (arg_helper.isEnableOption("-d"))
    {
    	#ifndef _WIN32
        DaemonTool::daemon();
        #endif
    }
    #ifdef _WIN32
    Singleton<NetFactory::NetData>::instance().start();
    #endif
    
    //! 美丽的日志组件，shell输出是彩色滴！！
    if (arg_helper.isEnableOption("-log_path"))
    {
        LOG.start(arg_helper);
    }
    else
    {
        LOG.start("-log_path ./log -log_filename log -log_class DB_MGR,XX,BROKER,FFRPC,FFGATE,FFWORKER,FFWORKER_PYTHON,FFWORKER_LUA,FFWORKER_JS,FFNET,HHTP_MGR -log_print_screen true -log_print_file true -log_level 4");
    }
    std::string perf_path = "./perf";
    long perf_timeout = 10*60;//! second
    if (arg_helper.isEnableOption("-perf_path"))
    {
        perf_path = arg_helper.getOptionValue("-perf_path");
    }
    if (arg_helper.isEnableOption("-perf_timeout"))
    {
        perf_timeout = ::atoi(arg_helper.getOptionValue("-perf_timeout").c_str());
    }
    if (PERF_MONITOR.start(perf_path, perf_timeout))
    {
        return -1;
    }
    
    FFBroker              ffbroker;
    FFGate ffgate;
    try
    {
        string gate_listen = "tcp://*:44000";
        //! 启动broker，负责网络相关的操作，如消息转发，节点注册，重连等
        if (arg_helper.isEnableOption("-gate_listen"))
        {
            gate_listen = arg_helper.getOptionValue("-gate_listen");
        }
        std::string brokercfg = "tcp://127.0.0.1:43210";
        if (ffbroker.open(brokercfg))
        {
            printf("broker open failed\n");
            goto err_proc;
        }
        int port = ffbroker.get_port();
        printf("broker open ok\n");
        if (Singleton<SharedSyncmemMgr>::instance().init_master(port)){
            printf("shared mem open failed\n");
            goto err_proc;
        }
        
        if (ffgate.open(brokercfg, gate_listen))
        {
            printf("gate open error!\n");
            goto err_proc;
        }
        printf("gate open ok\n");
    }
    catch(exception& e_)
    {
        printf("exception=%s\n", e_.what());
        return -1;
    }

#ifndef _WIN32
    SignalHelper::wait();
#else
    if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) 
	  { 
	    printf( "\nserver is running.\n" ); 
	    printf( "--  Ctrl+C exit\n" ); 
		while(g_flagwait){
			usleep(1000*100);
		}
	  }
	  else 
	  {
	    printf( "\nERROR: Could not set control handler"); 
	    return 1;
	  }
	flagok = true;  
#endif
err_proc:
    ffgate.close();
    PERF_MONITOR.stop();
    usleep(100);
    NetFactory::stop();
    usleep(200);
    
    Singleton<SharedSyncmemMgr>::instance().cleanup();

#ifdef _WIN32
    if  (!flagok)
    {
    	sleep(10);
	}
#endif

    return 0;
}

