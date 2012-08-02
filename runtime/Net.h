// Copyright 2010-2012 Google Inc.
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_runtime_Net_h_
#define _crack_runtime_Net_h_

#include <stdint.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>
#include <sys/un.h>
#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif

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

    static uint32_t crack_htonl(uint32_t val);
    static uint32_t crack_ntohl(uint32_t val);
    static uint16_t crack_htons(uint16_t val);
    static uint16_t crack_ntohs(uint16_t val);
};

struct SockAddrUn : public SockAddr {
    char path[UNIX_PATH_MAX];

    static void init(SockAddrUn *inst, const char *path, size_t size);
    static const char *getPath(SockAddrUn *inst);
};

struct TimeVal {
    int32_t secs, nsecs;

    static void init(TimeVal *inst, int32_t secs0, int32_t nsecs0);
    static void setToNow(TimeVal *inst, void *tz);
};

struct PollEvt {
    int fd, events, revents;
};

Constants *getConstants();
uint32_t makeIPV4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
int connect(int s, SockAddr *addr);
int bind(int s, SockAddr *addr);
int accept(int s, SockAddr *addr);
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
void PollSet_delete(struct pollfd *set, unsigned int size, unsigned int index);
int PollSet_poll(struct pollfd *fds, unsigned int nfds, TimeVal *tv,
                 sigset_t *sigmask
                 );


addrinfo *AddrInfo_create(const char *host, const char *service,
                          addrinfo *hints
                          );
void AddrInfo_free(addrinfo *info);
sockaddr_in *AddrInfo_getInAddr(addrinfo *ai);

struct PipeAddr {
    int32_t flags, readfd, writefd;
    static void init1(PipeAddr *pipe, int32_t flags);
    static void init2(PipeAddr *pipe, int32_t flags, int32_t readfd, int32_t writefd);
};

// Two variants of fcntl
int crk_fcntl1(int fd, int cmd);
int crk_fcntl2(int fd, int cmd, int64_t arg);

}} // namespace crack::runtime

#endif
