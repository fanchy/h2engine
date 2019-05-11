
#ifndef _SIGNAL_HELPER_H_
#define _SIGNAL_HELPER_H_

#ifndef _WIN32
#include <signal.h>

#include <map>
#include <string>


class SignalHelper
{
public:
    static int bloack()
    {
        sigset_t mask_sig;

        sigfillset(&mask_sig);
        return ::pthread_sigmask(SIG_BLOCK, &mask_sig, NULL);
    }
    static int wait(std::string option_ = "")
    {
        sigset_t mask_sig;
        sigemptyset(&mask_sig);
        sigaddset(&mask_sig, SIGINT);
        sigaddset(&mask_sig, SIGQUIT);
        sigaddset(&mask_sig, SIGTERM);

        int sig_num = 0;
        pthread_sigmask(SIG_BLOCK, &mask_sig, 0);

        ::sigwait(&mask_sig, &sig_num);
        return sig_num;
    }
};
#else
class SignalHelper
{
public:
    static int bloack() {return 0;}
    static int wait(std::string option_ = ""){return 0;}
};
#endif

#endif
