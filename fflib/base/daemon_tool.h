#ifndef _WIN32
#ifndef _DAEMON_TOOL_H_
#define _DAEMON_TOOL_H_

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace ff
{
class DaemonTool
{
public:
    static void daemon()
    {
        daemon(NULL);
    }

    static void daemon(const char* uname_)
    {
        pid_t pid, sid;

        if ((pid = fork()) < 0)
        {
            printf("daemon fork failed:%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if(pid != 0)
        {
           exit(EXIT_SUCCESS);
        }

        char buffer[1024];
        if (getcwd(buffer, sizeof(buffer))){}

        if(chdir(buffer) == -1)
        {
            printf("chdir failed:%s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if(uname_ != NULL)
        {
            int uid = get_uid(uname_);
            if(uid == -1)
            {
                printf("get_uid failed:%s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if(setuid(uid) == -1)
            {
                printf("setuid failed:%s", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        sid = setsid();
        if (sid < 0)
        {
            printf("daemon setsid failed:%s", strerror(errno));
            exit (EXIT_FAILURE);
        }

        umask (022);

        for (int i = getdtablesize() - 1; i >= 0; --i)
        {
            (void)close(i);
        }

        int fd_dev = open("/dev/null", O_RDWR);
        if (dup(fd_dev)){}
        if (dup(fd_dev)){}
    }

    static int get_uid(const char* uname_)
    {
        struct passwd *user = NULL;
        user = getpwnam(uname_);
        if(user == NULL)
        {
                return -1;
        }
        return user->pw_uid;
    }
};

}
#endif

#endif

