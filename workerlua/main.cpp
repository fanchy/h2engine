#include <stdio.h>
#include "base/osdef.h"
#include "base/fftype.h"
#include "base/daemon_tool.h"
#include "base/arg_helper.h"
#include "base/str_tool.h"
#include "base/smart_ptr.h"

#include "base/log.h"
#include "base/signal_helper.h"
#include "base/daemon_tool.h"
#include "base/perf_monitor.h"
#include "base/os_tool.h"

#include "net/ffnet.h"
#include "rpc/ffrpc.h"
#include "rpc/ffbroker.h"
#include "server/ffworker.h"
#include "server/ffgate.h"
#include "server/http_mgr.h"

#include "./ffworker_lua.h"

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
      printf( "recv=%d please use Ctrl-C \n\n", (int)fdwCtrlType );
      return FALSE;
  }
}
static bool flagok = false;
#endif

int main(int argc, char* argv[])
{
    SignalHelper::bloack();
	ArgHelper& arg_helper = Singleton<ArgHelper>::instance();
	arg_helper.load(argc, argv);
    if (arg_helper.isEnableOption("-f"))
    {
        arg_helper.loadFromFile(arg_helper.getOptionValue("-f"));
    }
    string cfgfile = "h2.conf";
    if (arg_helper.isEnableOption("-f"))
    {
        cfgfile = arg_helper.getOptionValue("-f");
    }
    else if (OSTool::isFile("../h2.conf")){
        cfgfile = "../h2.conf";
    }
    arg_helper.loadFromFile(cfgfile);

    if (arg_helper.isEnableOption("-d"))
    {
    	#ifndef _WIN32
        DaemonTool::daemon();
        #endif
    }
    #ifdef _WIN32
    FFNet::instance().start();
    #endif

    //! ��������־�����shell����ǲ�ɫ�Σ���
    if (arg_helper.isEnableOption("-log_path"))
    {
        LOG.start(arg_helper);
    }
    else
    {
        LOG.start("-log_path ./log -log_filename log -log_class DB_MGR,GAME_LOG,BROKER,FFRPC,FFGATE,FFWORKER,FFWORKER_PYTHON,FFWORKER_LUA,FFWORKER_JS,FFNET,HHTP_MGR -log_print_screen true -log_print_file true -log_level 4");
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


    try
    {
        int worker_index = 0;
        if (arg_helper.isEnableOption("-worker_index"))
        {
            worker_index = ::atoi(arg_helper.getOptionValue("-worker_index").c_str());
        }

        Singleton<HttpMgr>::instance().start();

        std::string brokercfg = "tcp://127.0.0.1:43210";
        if (arg_helper.isEnableOption("-broker")){
            brokercfg = arg_helper.getOptionValue("-broker");
        }

        string entry = "main.lua";
        if (arg_helper.isEnableOption("-entry")){
            entry = arg_helper.getOptionValue("-entry");
            printf("use entry %s\n", entry.c_str());
        }
        else{
            if (OSTool::isFile("../main.lua")){
                entry = "../main.lua";
            }
            printf("use default entry %s, user -entry redirect entry script\n", entry.c_str());
        }

        if (Singleton<FFWorkerLua>::instance().open(brokercfg, worker_index))
        {
            printf("FFWorkerLua open error!\n");
            goto err_proc;
        }
        if (Singleton<FFWorkerLua>::instance().scriptInit(entry))
        {
            printf("FFWorkerLua scriptInit error!\n");
            goto err_proc;
        }
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

    Singleton<FFWorkerLua>::instance().close();
    PERF_MONITOR.stop();
    usleep(100);
    FFNet::instance().stop();
    usleep(200);

    Singleton<HttpMgr>::instance().stop();
#ifdef _WIN32
    if  (!flagok)
    {
    	Sleep(100);
	}
#endif

    return 0;
}

