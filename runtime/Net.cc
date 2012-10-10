// Copyright 2010-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Arno Rehn <arno@arnorehn.de>
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Net.h"
#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <arpa/inet.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <errno.h>
#include <signal.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include <iostream>

using namespace std;

namespace {
    static void set_sockaddr_in(struct sockaddr_in *sa, 
                                crack::runtime::SockAddrIn *addr
                                ) {
        sa->sin_family = AF_INET;
        sa->sin_port = htons(addr->port);
        sa->sin_addr.s_addr = htonl(addr->addr);
    }

    static void set_sockaddr_un(struct sockaddr_un *sa,
                                crack::runtime::SockAddrUn *addr
                                ) {
        sa->sun_family = AF_UNIX;
        strcpy(sa->sun_path, addr->path);
    }

    typedef union {
        sockaddr_in in;
        sockaddr_un un;
    } AddrUnion;

    static socklen_t set_sockaddr(AddrUnion &addrs,
                                  crack::runtime::SockAddr *addr
                                  ) {
        socklen_t sz;
        if (addr->family == AF_INET) {
            set_sockaddr_in(&addrs.in,
                            static_cast<crack::runtime::SockAddrIn *>(addr));
            sz = sizeof(addrs.in);
        } else {
            set_sockaddr_un(&addrs.un,
                            static_cast<crack::runtime::SockAddrUn *>(addr));
            sz = sizeof(addrs.un);
        }

        return sz;
    }

}

// our exported functions
namespace crack { namespace runtime {

void SockAddrIn::init1(SockAddrIn *inst, uint8_t a, uint8_t b, uint8_t c, 
                       uint8_t d, 
                       unsigned int port0
                       ) {
    inst->family = AF_INET;
    inst->addr = makeIPV4(a, b, c, d);
    inst->port = port0;
}

void SockAddrIn::init2(SockAddrIn *inst, uint32_t addr0, unsigned int port0) {
    inst->family = AF_INET;
    inst->addr = addr0;
    inst->port = port0;
}

uint32_t SockAddrIn::crack_htonl(uint32_t val) {
    return htonl(val);
}

uint32_t SockAddrIn::crack_ntohl(uint32_t val) {
    return ntohl(val);
}

uint16_t SockAddrIn::crack_htons(uint16_t val) {
    return htons(val);
}

uint16_t SockAddrIn::crack_ntohs(uint16_t val) {
    return ntohs(val);
}

namespace {
    inline size_t min(size_t a, size_t b) {
        return (a < b) ? a : b;
    }
}

void SockAddrUn::init(SockAddrUn *inst, const char *path, size_t size) {
    inst->family = AF_UNIX;
    size = min(size, UNIX_PATH_MAX - 1);
    strncpy(inst->path, path, size);
    inst->path[size] = 0;
}

const char *SockAddrUn::getPath(SockAddrUn *inst) {
    return inst->path;
}

void TimeVal::init(TimeVal *inst, int32_t secs0, int32_t nsecs0) {
    inst->secs = secs0;
    inst->nsecs = nsecs0;
}

void TimeVal::setToNow(TimeVal *inst, void *tz) {
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)tz);
    inst->secs = tv.tv_sec;
    // TimeVal uses naoseconcs, gettimeofday uses microseconds.
    inst->nsecs = tv.tv_usec * 1000;
}

uint32_t makeIPV4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (a << 24) | (b << 16) | (c << 8) | d;
}

int connect(int s, SockAddr *addr) {
    AddrUnion addrs;
    socklen_t sz = set_sockaddr(addrs, addr);
    return ::connect(s, (sockaddr *)&addrs, sz);
}

int bind(int s, SockAddr *addr) {
    AddrUnion addrs;
    socklen_t sz = set_sockaddr(addrs, addr);
    return ::bind(s, (sockaddr *)&addrs, sz);
}

int accept(int s, SockAddr *addr) {
    AddrUnion addrs;
    addrs.in.sin_family = addr->family;
    socklen_t saSize = sizeof(addrs);
    int newSock = ::accept(s, (sockaddr *)&addrs, &saSize);
    if (newSock != -1) {
        switch (addrs.in.sin_family) {
            case AF_INET: {
                SockAddrIn *in = static_cast<SockAddrIn *>(addr);
                in->port = ntohs(addrs.in.sin_port);
                in->addr = ntohl(addrs.in.sin_addr.s_addr);
                break;
            }
            case AF_UNIX: {
                SockAddrUn *un = static_cast<SockAddrUn *>(addr);
                assert(un->family == AF_UNIX);
                strcpy(un->path, addrs.un.sun_path);
                break;
            }
        }
    }

    return newSock;
}

int setsockopt_int(int fd, int level, int optname, int val) {
    return setsockopt(fd, level, optname, &val, sizeof(val));
}

pollfd *PollSet_create(unsigned int size) {
    return (pollfd *)calloc(size, sizeof(struct pollfd));
}

void PollSet_copy(struct pollfd *dst, struct pollfd *src, unsigned int size) {
    memcpy(dst, src, size * sizeof(struct pollfd));
}

void PollSet_destroy(struct pollfd *pollset) {
    free(pollset);
}

void PollSet_set(struct pollfd *set, unsigned int index, int fd, int events, 
                 int revents
                 ) {
    struct pollfd &elem = set[index];
    elem.fd = fd;
    elem.events = events;
    elem.revents = revents;
}

void PollSet_get(struct pollfd *set, unsigned int index,
                 PollEvt *outputEntry
                 ) {
    struct pollfd &elem = set[index];
    outputEntry->fd = elem.fd;
    outputEntry->events = elem.fd;
    outputEntry->revents = elem.fd;
}

// find the next poll entry that has an event in revents whose index is >= 
// index.  Makes it easy to iterate over the pollset.  Returns the index of 
// the item found, stores the entry info in outputEntry, -1 if no item was 
// found.
int PollSet_next(struct pollfd *set, unsigned int size, unsigned int index, 
                 PollEvt *outputEntry
                 ) {
    for (; index < size; ++index) {
        pollfd *elem = &set[index];
        if (elem->revents) {
            outputEntry->fd = elem->fd;
            outputEntry->events = elem->events;
            outputEntry->revents = elem->revents;
            return index;
        }
    }
    
    // not found.
    return -1;
}

void PollSet_delete(struct pollfd *set, unsigned int size, unsigned int index) {
    memmove(set + index, set + index + 1, 
            (size - index - 1) * sizeof(struct pollfd)
            );
}

int PollSet_poll(struct pollfd *fds, unsigned int nfds, TimeVal *tv,
                 sigset_t *sigmask
                 ) {
#ifdef HAVE_PPOLL
    if (tv) {
        struct timespec ts = {tv->secs, tv->nsecs};
        return ppoll(fds, nfds, &ts, sigmask);
    } else {
        return ppoll(fds, nfds, 0, sigmask);
    }
#else
    assert(0 && "XXX no ppoll");
#endif
}

sigset_t *SigSet_create() {
    return (sigset_t *)malloc(sizeof(sigset_t));
}

void SigSet_destroy(sigset_t *sigmask) {
    free(sigmask);
}

int SigSet_empty(sigset_t *sigmask) {
    return sigemptyset(sigmask);
}

int SigSet_fill(sigset_t *sigmask) {
    return sigfillset(sigmask);
}

int SigSet_add(sigset_t *sigmask, int signum) {
    return sigaddset(sigmask, signum);
}

int SigSet_del(sigset_t *sigmask, int signum) {
    return sigdelset(sigmask, signum);
}

int SigSet_has(sigset_t *sigmask, int signum) {
    return sigismember(sigmask, signum);
}

addrinfo *AddrInfo_create(const char *host, const char *service,
                          addrinfo *hints
                          ) {
    addrinfo *result;
    if (getaddrinfo(host, service, hints, &result))
        return 0;
    else
        return result;
}

sockaddr_in *AddrInfo_getInAddr(addrinfo *ai) {
    if (ai->ai_family == AF_INET)
        return (sockaddr_in *)ai->ai_addr;
    else
        return 0;
}

void PipeAddr::init1(PipeAddr *pipeAddr, int32_t flags) {
    int pipefd[2] = {-1, -1};
    int errors = pipe(pipefd);

    if (errors == 0) {
        pipeAddr->flags=flags;
        pipeAddr->readfd = int32_t(pipefd[0]);
        pipeAddr->writefd = int32_t(pipefd[1]);
        if (fcntl(pipefd[0], F_SETFL, flags)!=-1) return;
        fcntl(pipefd[1], F_SETFL, flags);
    }
}

void PipeAddr::init2(PipeAddr *pipe, int32_t flags, int32_t readfd, int32_t writefd) {
    pipe->flags = flags;
    pipe->readfd = readfd;
    pipe->writefd = writefd;
}

int crk_fcntl1(int fd, int cmd){
    return fcntl(fd, cmd);
}

int crk_fcntl2(int fd, int cmd, int64_t arg){
    return fcntl(fd, cmd, (long)arg);
}

}} // namespace crack::runtime

