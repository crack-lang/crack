// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>


namespace crack { namespace runtime {

#define CRK_PROC_STILL_RUNNING 1 << 8
#define CRK_PROC_KILLED        1 << 9
#define CRK_PROC_STOPPED       1 << 10
#define CRK_PROC_EXITED        1 << 11

int runChildProcess(const char **argv,
                    const char **env);

int waitProcess(int pid, int noHang);

void signalProcess(int pid, int sig);

} }
