// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Process.h"
#include "ext/Module.h"
#include "ext/Func.h"
#include "ext/Type.h"

#include <vector>

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

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
        perror("waitpid");
        return -1;
    }
// END UNIX

}

void signalProcess(int pid, int sig) {

// UNIX
    kill(pid, sig);
// END UNIX

}

void closeProcess(PipeDesc *pd) {

    if (pd->in != -1 && close(pd->in) == -1)
            perror("close pipe failed: stdin");

    if (pd->out != -1 && close(pd->out) == -1)
            perror("close pipe failed: stdout");

    if (pd->err != -1 && close(pd->err) == -1)
            perror("close pipe failed: stderr");

}

int runChildProcess(const char **argv,
                    const char **env,
                    PipeDesc *pd
                    ) {

    assert(pd && "no PipeDesc passed");

// UNIX
    // set to unreadable initially
    pd->in = -1;
    pd->out = -1;
    pd->err = -1;

    int pipes[3][2]; // 0,1,2 (in,out,err) x 0,1 (read,write)

    // create pipes
    // XXX check pd to see which pipes we should make, if any
    for (int i = 0; i < 3; i++) {
        if (pd->flags & (1 << i) && pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return -1;
        }
    }

    pid_t p = fork();

    switch (p) {
    case -1:
        perror("fork failed");

        // clean the pipes
        for (int i = 0; i < 3; ++i)
            for (int j = 1; i < 2; ++j)
                if (pipes[i][j] != -1)
                    close(pipes[i][j]);

        return -1;
    case 0:
        // child
        if (pd->flags & 1) {
            if (close(pipes[0][1]) == -1) { // child stdin write close
                perror("close - child 0");
                return -1;
            }
            if (pipes[0][0] != STDIN_FILENO) {
                dup2(pipes[0][0], STDIN_FILENO);
                close(pipes[0][0]);
            }
        }
        if (pd->flags & 2) {
            if (close(pipes[1][0]) == -1) { // child stdout read close
                perror("close - child 1");
                return -1;
            }
            if (pipes[1][1] != STDOUT_FILENO) {
                dup2(pipes[1][1], STDOUT_FILENO);
                close(pipes[1][1]);
            }
        }
        if (pd->flags & 4) {
            if (close(pipes[2][0]) == -1) { // child stderr read close
                perror("close - child 2");
                return -1;
            }
            if (pipes[2][1] != STDERR_FILENO) {
                dup2(pipes[2][1], STDERR_FILENO);
                close(pipes[2][1]);
            }
        }
        // XXX close all handles > 2?

        if (env) {
            execve(argv[0],
                   const_cast<char* const*>(argv),
                   const_cast<char* const*>(env));
        }
        else {
            execvp(argv[0], const_cast<char* const*>(argv));
        }

        // if we get here, exec failed
        _exit(-1);

    default:
        // parent

        // parent stdin read close
        if (pd->flags & 1) {
            if (close(pipes[0][0]) == -1) {
                perror("close - parent 0");
                return -1;
            }
            // return pipes[0][1] as writeable fd (to child stdin)
            pd->in = pipes[0][1];
        }
        // parent stdout write close
        if (pd->flags & 2) {
                if (close(pipes[1][1]) == -1) {
                perror("close - parent 1");
                return -1;
            }
            // return pipes[1][0] as readable fd (from child stdout)
            pd->out = pipes[1][0];
        }
        // parent stderr write close
        if (pd->flags & 4) {
            if (close(pipes[2][1]) == -1) {
                perror("close - parent 2");
                return -1;
            }
            // return pipes[2][0] as readable fd (from child stderr)
            pd->err = pipes[2][0];
        }

        return p;
    }

// END UNIX

}


}} // namespace crack::runtime
