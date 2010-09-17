// Copyright 2010 Google Inc.

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <errno.h>
#include <signal.h>

#include <iostream>
using namespace std;

// mirrors the _Constants class in crack.net
struct _Constants {
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

static _Constants constants = {
    AF_UNIX,
    AF_LOCAL,
    AF_INET,
    AF_INET6,
    AF_IPX,
    AF_NETLINK,
    AF_X25,
    AF_AX25,
    AF_ATMPVC,
    AF_APPLETALK,
    AF_PACKET,
    SOCK_STREAM,
    SOCK_DGRAM,
    SOCK_SEQPACKET,
    SOCK_RAW,
    SOCK_RDM,
    SOCK_PACKET,
    SOCK_NONBLOCK,
    SOCK_CLOEXEC,
    SOL_SOCKET,
    SO_REUSEADDR,
    POLLIN,
    POLLOUT,
    POLLPRI,
    POLLERR,
    POLLHUP,
    POLLNVAL,
    INADDR_ANY
};

struct crk_Object {
    void *vtable;
    unsigned int refCount;
};

struct SockAddrIn {
    struct crk_Object baseObject;
    uint32_t addr;
    unsigned int port;
};

struct TimeVal {
    struct crk_Object baseObject;
    int32_t secs, nsecs;
};

struct PollFD {
    struct crk_Object baseObject;
    int fd, events, revents;
    void *pollable;
};

static void set_sockaddr_in(struct sockaddr_in *sa, SockAddrIn *addr) {
    sa->sin_family = AF_INET;
    sa->sin_port = htons(addr->port);
    sa->sin_addr.s_addr = htonl(addr->addr);
}

// our exported functions
extern "C" {

_Constants *_crack_get_constants() { return &constants; }

uint32_t _crack_make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (a << 24) | (b << 16) | (c << 8) | d;
}

int _crack_connect(int s, SockAddrIn *addr) {
    sockaddr_in sa;
    set_sockaddr_in(&sa, addr);
    return connect(s, (sockaddr *)&sa, sizeof(sa));
}

int _crack_bind(int s, SockAddrIn *addr) {
    sockaddr_in sa;
    set_sockaddr_in(&sa, addr);
    return bind(s, (sockaddr *)&sa, sizeof(sa));
}

int _crack_accept(int s, SockAddrIn *addr) {
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    socklen_t saSize = sizeof(sa);
    int newSock = accept(s, (sockaddr *)&sa, &saSize);
    if (newSock != -1) {
        addr->port = ntohs(sa.sin_port);
        addr->addr = ntohl(sa.sin_addr.s_addr);
    }

    return newSock;
}

int _crack_setsockopt_int(int fd, int level, int optname, int val) {
    return setsockopt(fd, level, optname, &val, sizeof(val));
}

pollfd *_crack_pollset_create(unsigned int size) {
    return (pollfd *)calloc(size, sizeof(struct pollfd));
}

void _crack_pollset_copy(struct pollfd *dst, struct pollfd *src, 
                         unsigned int size
                         ) {
    memcpy(dst, src, size * sizeof(struct pollfd));
}

void _crack_pollset_destroy(struct pollfd *pollset) {
    free(pollset);
}

void _crack_pollset_set(struct pollfd *set, unsigned int index,
                        int fd,
                        int events,
                        int revents
                        ) {
    struct pollfd &elem = set[index];
    elem.fd = fd;
    elem.events = events;
    elem.revents = revents;
}

void _crack_pollset_get(struct pollfd *set, unsigned int index,
                        PollFD *outputEntry
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
int _crack_pollset_next(struct pollfd *set, unsigned int size,
                        unsigned int index,
                        PollFD *outputEntry
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

int _crack_poll(struct pollfd *fds, unsigned int nfds, TimeVal *tv,
                sigset_t *sigmask
                ) {
    if (tv) {
        struct timespec ts = {tv->secs, tv->nsecs};
        return ppoll(fds, nfds, &ts, sigmask);
    } else {
        return ppoll(fds, nfds, 0, sigmask);
    }
}

sigset_t *_crack_sigset_create() {
    return (sigset_t *)malloc(sizeof(sigset_t));
}

void _crack_sigset_destroy(sigset_t *sigmask) {
    free(sigmask);
}

int _crack_sigset_empty(sigset_t *sigmask) {
    return sigemptyset(sigmask);
}

int _crack_sigset_fill(sigset_t *sigmask) {
    return sigfillset(sigmask);
}

int _crack_sigset_add(sigset_t *sigmask, int signum) {
    return sigaddset(sigmask, signum);
}

int _crack_sigset_del(sigset_t *sigmask, int signum) {
    return sigdelset(sigmask, signum);
}

int _crack_sigset_has(sigset_t *sigmask, int signum) {
    return sigismember(sigmask, signum);
}

} // extern "C"
