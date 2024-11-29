#ifndef __LOGMGR__
#define __LOGMGR__
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <functional>
#include "logger.h"
#include "common.h"

class LogMgr {
public:
    LogMgr();
    ~LogMgr();
    int exec();

private:
    static void signalchild_handler(int) { wait(NULL); }

private:
    int fdp[2] = {-1,-1};
    pid_t main_pid;
};

LogMgr::LogMgr() {
    main_pid = getpid();
}

LogMgr::~LogMgr() {
    
}

int LogMgr::exec() {
    pid_t log_pid;
    if(pipe(fdp) < 0) {
        perror("pipe");
        WARN() << "Cannot create pipe for log. Use stdout";
        Logger().setFD(fileno(stdout));
        goto L1;
    }

    Logger().setFD(fdp[1]);

    log_pid = fork();
    if (log_pid == -1) { 
        perror("fork");
        exit(1); 
    }

    if (log_pid > 0) {
        // parent
        close(fdp[0]);
        if(signal(SIGCHLD, LogMgr::signalchild_handler) == SIG_ERR) {
            printf("error signal()");
            return errno;
        }
        
        return 0;
    } 
    else {
        // child
        close(fdp[1]);
        int fdf = open("log", O_WRONLY | O_CREAT| O_APPEND, 0644);
        if(fdf == -1) {
            perror("open file");
            WARN() << "Cannot open file log. Use stdout.";
            fdf = fileno(stdout);
        }
        Logger().setFD(fdf);
        std::thread t([&]() {
            while (true)
            {
                sleep(1);
                if(getppid() != main_pid) {
                    
                    close(fdf);
                    exit(0);
                }
            }
            
        });
        char buff[BUFF_SIZE];
        while (true)
        {
            int bz = read(fdp[0], buff, BUFF_SIZE);
            if(bz) {
                buff[bz] = '\0';
                Logger().autoNewLine(false) << buff;
                std::cout << buff;
            }
        }
        if(t.joinable()) t.join();

        return 0;
    }
    L1:
    return 0;
}

#endif //__LOGMGR__