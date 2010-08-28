// Copyright 2010 Google Inc.

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>

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
        crk_SO_REUSEADDR;
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

} // extern "C"