// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Process.h"
#include "ext/Module.h"
#include "ext/Func.h"
#include "ext/Type.h"

#include <vector>

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h>

using namespace crack::ext;

namespace crack { namespace runtime {

// returns the exit status if exited, or the signal that killed or stopped
// the process in the most sig byte with a bit flag set in 9 or 10 depending
// on how it was signaled, or bit 8 set for "still running"
int waitProcess(int pid, int noHang) {

// UNIX
    int status, retPid;
    retPid = waitpid(pid, &status, (noHang)?WNOHANG:0);
    if (noHang && retPid == 0)
        return CRK_PROC_STILL_RUNNING;
    if (WIFEXITED(status)) {
        return CRK_PROC_EXITED | WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)) {
        return CRK_PROC_KILLED | WTERMSIG(status);
    }
    else if (WIFSTOPPED(status)) {
        return CRK_PROC_STOPPED | WSTOPSIG(status);
    }
    else {
        // ??
        std::cerr << "unknown?\n";
        return -1;
    }
// END UNIX

}

void signalProcess(int pid, int sig) {

// UNIX
    kill(pid, sig);
// END UNIX

}

int runChildProcess(const char **argv,
                    const char **env) {

// UNIX
    pid_t p = fork();

    switch (p) {
    case -1:
        perror("fork failed");
        exit(1);
    case 0:
        // child
        if (env) {
            execve(argv[0],
                   const_cast<char* const*>(argv),
                   const_cast<char* const*>(env));
        }
        else {
            execvp(argv[0], const_cast<char* const*>(argv));
        }
        // XXX handle pipes
        exit(1);
    default:
        // parent
        return p;
    }

// END UNIX

}


}} // namespace crack::runtime
