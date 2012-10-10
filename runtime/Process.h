// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 


namespace crack { namespace runtime {

#define CRK_PROC_STILL_RUNNING 1 << 8
#define CRK_PROC_KILLED        1 << 9
#define CRK_PROC_STOPPED       1 << 10
#define CRK_PROC_EXITED        1 << 11

#define PIPE_STDIN             1
#define PIPE_STDOUT            1 << 1
#define PIPE_STDERR            1 << 2

typedef struct {
    int flags;
    int in;
    int out;
    int err;
} PipeDesc;

int runChildProcess(const char **argv,
                    const char **env,
                    PipeDesc *pd);

void closeProcess(PipeDesc *pd);

int waitProcess(int pid, int noHang);

void signalProcess(int pid, int sig);

} }
