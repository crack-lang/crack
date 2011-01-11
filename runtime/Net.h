// Copyright 2010 Google Inc.

#ifndef _crack_runtime_Net_h_
#define _crack_runtime_Net_h_

#include <stdint.h>
#include <signal.h>
#include <poll.h>
#include "ext/Object.h"
#include <iostream> // XXX remove

namespace crack { namespace runtime {

struct Constants {
    int crk_AF_UNIX,
        crk_AF_LOCAL,
        crk_AF_INET,
        crk_AF_INET6,
        crk_AF_IPX,
        crk_AF_NETLINK,
        crk_AF_X25,
        crk_AF_AX25,
        crk_AF_ATMPVC,
        crk_AF_APPLETALK,
        crk_AF_PACKET,
        crk_SOCK_STREAM,
        crk_SOCK_DGRAM,
        crk_SOCK_SEQPACKET,
        crk_SOCK_RAW,
        crk_SOCK_RDM,
        crk_SOCK_PACKET,
        crk_SOCK_NONBLOCK,
        crk_SOCK_CLOEXEC,
        crk_SOL_SOCKET,
        crk_SO_REUSEADDR,
        crk_POLLIN,
        crk_POLLOUT,
        crk_POLLPRI,
        crk_POLLERR,
        crk_POLLHUP,
        crk_POLLNVAL;
    uint32_t crk_INADDR_ANY;
};

struct SockAddr {
    int family;
};

struct SockAddrIn : public SockAddr {
    uint32_t addr;
    unsigned int port;

    static void init1(SockAddrIn *inst, uint8_t a, uint8_t b, uint8_t c, 
                      uint8_t d, 
                      unsigned int port
                      );

    static void init2(SockAddrIn *inst, uint32_t addr, unsigned int port);
};

struct TimeVal {
    int32_t secs, nsecs;

    static void init(TimeVal *inst, int32_t secs0, int32_t nsecs0);
};

struct PollEvt {
    int fd, events, revents;
};

Constants *getConstants();
uint32_t makeIPV4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
int connect(int s, SockAddrIn *addr);
int bind(int s, SockAddrIn *addr);
int accept(int s, SockAddrIn *addr);
int setsockopt_int(int fd, int level, int optname, int val);

sigset_t *SigSet_create();
void SigSet_destroy(sigset_t *sigmask);
int SigSet_empty(sigset_t *sigmask);
int SigSet_fill(sigset_t *sigmask);
int SigSet_add(sigset_t *sigmask, int signum);
int SigSet_del(sigset_t *sigmask, int signum);
int SigSet_has(sigset_t *sigmask, int signum);

pollfd *PollSet_create(unsigned int size);
void PollSet_copy(struct pollfd *dst, struct pollfd *src, unsigned int size);
void PollSet_destroy(struct pollfd *pollset);
void PollSet_set(struct pollfd *set, unsigned int index, int fd, int events, 
                 int revents
                 );
void PollSet_get(struct pollfd *set, unsigned int index,
                 PollEvt *outputEntry
                 );
int PollSet_next(struct pollfd *set, unsigned int size, unsigned int index, 
                 PollEvt *outputEntry
                 );
int PollSet_poll(struct pollfd *fds, unsigned int nfds, TimeVal *tv,
                 sigset_t *sigmask
                 );


}} // namespace crack::runtime

#endif
