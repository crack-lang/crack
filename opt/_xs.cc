// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <xs/xs.h>

typedef void * voidptr;
typedef void * xs_socket_t;
struct xs_version_struct {
                int major, minor, patch;
            };
            typedef struct xs_version_struct xs_version_info;
xs_version_info *get_xs_version(){
                xs_version_info *v = new xs_version_info;
                xs_version(&(v->major), &(v->minor), &(v->patch));
                return v;
            }
int xs_setctxopt_int(voidptr ctx, int option, int optval) {
                return xs_setsockopt((voidptr)ctx, option, &optval, sizeof(int));
            }
xs_msg_t *crk_xs_msg_new() {
                xs_msg_t *msg = new xs_msg_t;
                return msg;
            }
int crk_xs_msg_init_data(xs_msg_t *msg, char *data, unsigned int size) {
                return xs_msg_init_data(msg, (voidptr)data, (size_t)size, NULL, NULL);
            }
int crk_xs_getmsgopt_int(xs_msg_t *msg, int option) {
                int optval;
                size_t optvallen;
                int status = xs_getmsgopt(msg, option, (voidptr)&optval, &optvallen);
                if (status == -1) return -1;
                return optval;
            }
int xs_setsockopt_int (voidptr sock, int option, int optval) {
                return xs_setsockopt(sock, option, (voidptr)&optval, sizeof(int));
            }
int xs_setsockopt_uint64 (voidptr sock, int option,
                                       uint64_t optval) {
                return xs_setsockopt(sock, option, (voidptr)&optval, sizeof(uint64_t));
            }
int xs_setsockopt_byteptr (voidptr sock, int option,
                char *optval, size_t optvallen) {
                return xs_setsockopt(sock, option, (voidptr)optval, optvallen);
            }
int xs_getsockopt_int(voidptr sock, int option) {
                int optval;
                size_t optvallen;
                int status = xs_getsockopt(sock, option, (voidptr)&optval, &optvallen);
                if (status == -1) return -1;
                return optval;
            }
uint64_t xs_getsockopt_uint64(voidptr sock, int option) {
                uint64_t optval;
                size_t optvallen;
                int status = xs_getsockopt(sock, option, (voidptr)&optval, &optvallen);
                if (status == -1) return -1; // This -1 is deliberate
                return optval;
            }
char *xs_getsockopt_byteptr(voidptr sock, int option) {
                char optval[255];
                size_t optvallen;
                int status = xs_getsockopt(sock, option, optval, &optvallen);
                if (status == -1) return NULL;
                char *valString = new char[optvallen];
                memcpy((voidptr)valString, (voidptr)optval, optvallen);
                valString[optvallen] = '\0'; // So that it's a CString
                return valString;
            }
int crk_xs_sock_recvmsg(xs_socket_t sock, xs_msg_t *msgc, int flags) {
                xs_msg_t msgr;
                xs_msg_t *msg = &msgr;
                xs_msg_init(msg);
                int numbytes = xs_recvmsg(sock, msg, flags);
                int i;
                char _buf[256];
                char *data = (char *)xs_msg_data(msg);
                memcpy(_buf, xs_msg_data(msg), numbytes<255 ? numbytes : 254);
                _buf[numbytes<255 ? numbytes : 255] = '\0';
                return numbytes;
             }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__xs_rinit() {
    return;
}

extern "C"
void crack_ext__xs_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_int16 = mod->getInt16Type();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint16 = mod->getUint16Type();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_intz = mod->getIntzType();
    crack::ext::Type *type_uintz = mod->getUintzType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_float = mod->getFloatType();

    crack::ext::Type *type_xs_version_info = mod->addType("xs_version_info", sizeof(xs_version_info));
        type_xs_version_info->addInstVar(type_int, "major",
                                CRACK_OFFSET(xs_version_info, major));
        type_xs_version_info->addInstVar(type_int, "minor",
                                CRACK_OFFSET(xs_version_info, minor));
        type_xs_version_info->addInstVar(type_int, "patch",
                                CRACK_OFFSET(xs_version_info, patch));
        f = type_xs_version_info->addConstructor("init",
                            (void *)get_xs_version
                        );

    type_xs_version_info->finish();


    crack::ext::Type *type_xs_context = mod->addType("xs_context", sizeof(voidptr));
    type_xs_context->finish();


    crack::ext::Type *type_xs_msg_t = mod->addType("xs_msg_t", sizeof(xs_msg_t));
        f = type_xs_msg_t->addConstructor("init",
                            (void *)crk_xs_msg_new
                        );


    f = type_xs_msg_t->addMethod(
        type_int, 
        "initEmpty",
        (void *)xs_msg_init
    );


    f = type_xs_msg_t->addMethod(
        type_int, 
        "initSize",
        (void *)xs_msg_init_size
    );
    f->addArg(type_uint, 
              "size"
              );


    f = type_xs_msg_t->addMethod(
        type_int, 
        "initData",
        (void *)crk_xs_msg_init_data
    );
    f->addArg(type_byteptr, 
              "data"
              );
    f->addArg(type_uint, 
              "size"
              );


    f = type_xs_msg_t->addMethod(
        type_int, 
        "close",
        (void *)xs_msg_close
    );


    f = type_xs_msg_t->addMethod(
        type_byteptr, 
        "data",
        (void *)xs_msg_data
    );


    f = type_xs_msg_t->addMethod(
        type_uint, 
        "size",
        (void *)xs_msg_size
    );


    f = type_xs_msg_t->addMethod(
        type_int, 
        "getOptInt",
        (void *)crk_xs_getmsgopt_int
    );
    f->addArg(type_int, 
              "option"
              );

    type_xs_msg_t->finish();


    crack::ext::Type *type_xs_socket_t = mod->addType("xs_socket_t", sizeof(voidptr));
    type_xs_socket_t->finish();


    crack::ext::Type *type_xs_pollitem_t = mod->addType("xs_pollitem_t", sizeof(xs_pollitem_t));
        type_xs_pollitem_t->addInstVar(type_xs_socket_t, "socket",
                                CRACK_OFFSET(xs_pollitem_t, socket));
        type_xs_pollitem_t->addInstVar(type_int, "fd",
                                CRACK_OFFSET(xs_pollitem_t, fd));
        type_xs_pollitem_t->addInstVar(type_int16, "events",
                                CRACK_OFFSET(xs_pollitem_t, events));
        type_xs_pollitem_t->addInstVar(type_int16, "revents",
                                CRACK_OFFSET(xs_pollitem_t, revents));
    type_xs_pollitem_t->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pxs__pollitem__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_xs_pollitem_t;
        array_pxs__pollitem__t_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_int, "xs_errno",
                     (void *)xs_errno
                     );

    f = mod->addFunc(type_byteptr, "xs_strerror",
                     (void *)xs_strerror
                     );
       f->addArg(type_int, "errno");

    f = mod->addFunc(type_xs_context, "xs_init",
                     (void *)xs_init
                     );

    f = mod->addFunc(type_int, "xs_term",
                     (void *)xs_term
                     );
       f->addArg(type_xs_context, "ctx");

    f = mod->addFunc(type_int, "xs_setctxopt",
                     (void *)xs_setctxopt
                     );
       f->addArg(type_xs_context, "ctx");
       f->addArg(type_int, "option");
       f->addArg(type_voidptr, "optval");
       f->addArg(type_uint, "optvallen");

    f = mod->addFunc(type_int, "xs_setctxopt_int",
                     (void *)xs_setctxopt_int
                     );
       f->addArg(type_xs_context, "ctx");
       f->addArg(type_int, "option");
       f->addArg(type_int, "optval");

    f = mod->addFunc(type_int, "xs_msg_move",
                     (void *)xs_msg_move
                     );
       f->addArg(type_xs_msg_t, "dest");
       f->addArg(type_xs_msg_t, "src");

    f = mod->addFunc(type_int, "xs_msg_copy",
                     (void *)xs_msg_copy
                     );
       f->addArg(type_xs_msg_t, "dest");
       f->addArg(type_xs_msg_t, "src");

    f = mod->addFunc(type_xs_socket_t, "xs_sock_new",
                     (void *)xs_socket
                     );
       f->addArg(type_xs_context, "ctx");
       f->addArg(type_int, "tpe");

    f = mod->addFunc(type_int, "xs_sock_close",
                     (void *)xs_close
                     );
       f->addArg(type_xs_socket_t, "sock");

    f = mod->addFunc(type_int, "xs_setsockopt",
                     (void *)xs_setsockopt
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");
       f->addArg(type_voidptr, "optval");
       f->addArg(type_uint, "optvallen");

    f = mod->addFunc(type_int, "xs_setsockopt_int",
                     (void *)xs_setsockopt_int
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");
       f->addArg(type_int, "optval");

    f = mod->addFunc(type_int, "xs_setsockopt_uint64",
                     (void *)xs_setsockopt_uint64
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");
       f->addArg(type_uint64, "optval");

    f = mod->addFunc(type_int, "xs_setsockopt_byteptr",
                     (void *)xs_setsockopt_byteptr
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");
       f->addArg(type_byteptr, "optval");
       f->addArg(type_uint, "optvallen");

    f = mod->addFunc(type_int, "xs_getsockopt_int",
                     (void *)xs_getsockopt_int
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");

    f = mod->addFunc(type_uint64, "xs_getsockopt_uint64",
                     (void *)xs_getsockopt_uint64
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");

    f = mod->addFunc(type_byteptr, "xs_getsockopt_byteptr",
                     (void *)xs_getsockopt_byteptr
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "option");

    f = mod->addFunc(type_int, "xs_sock_bind",
                     (void *)xs_bind
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_byteptr, "addr");

    f = mod->addFunc(type_int, "xs_sock_connect",
                     (void *)xs_connect
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_byteptr, "addr");

    f = mod->addFunc(type_int, "xs_sock_shutdown",
                     (void *)xs_shutdown
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_int, "how");

    f = mod->addFunc(type_int, "xs_sock_send",
                     (void *)xs_send
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_byteptr, "buf");
       f->addArg(type_uint, "len");
       f->addArg(type_int, "flags");

    f = mod->addFunc(type_int, "xs_sock_recv",
                     (void *)xs_recv
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_byteptr, "buf");
       f->addArg(type_uint, "len");
       f->addArg(type_int, "flags");

    f = mod->addFunc(type_int, "xs_sock_sendmsg",
                     (void *)xs_sendmsg
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_xs_msg_t, "msg");
       f->addArg(type_int, "flags");

    f = mod->addFunc(type_int, "xs_sock_recvmsg",
                     (void *)xs_recvmsg
                     );
       f->addArg(type_xs_socket_t, "sock");
       f->addArg(type_xs_msg_t, "msg");
       f->addArg(type_int, "flags");

    f = mod->addFunc(type_int, "xs_poll",
                     (void *)xs_poll
                     );
       f->addArg(array_pxs__pollitem__t_q, "items");
       f->addArg(type_int, "nitems");
       f->addArg(type_int, "timeout");


    mod->addConstant(type_int, "XS_VERSION_MAJOR",
                     static_cast<int>(XS_VERSION_MAJOR)
                     );

    mod->addConstant(type_int, "XS_VERSION_MINOR",
                     static_cast<int>(XS_VERSION_MINOR)
                     );

    mod->addConstant(type_int, "XS_VERSION_PATCH",
                     static_cast<int>(XS_VERSION_PATCH)
                     );

    mod->addConstant(type_int, "EFSM",
                     static_cast<int>((XS_HAUSNUMERO + 51))
                     );

    mod->addConstant(type_int, "ENOCOMPATPROTO",
                     static_cast<int>((XS_HAUSNUMERO + 52))
                     );

    mod->addConstant(type_int, "ETERM",
                     static_cast<int>((XS_HAUSNUMERO + 53))
                     );

    mod->addConstant(type_int, "EMTHREAD",
                     static_cast<int>((XS_HAUSNUMERO + 54))
                     );

    mod->addConstant(type_int, "XS_MAX_SOCKETS",
                     static_cast<int>(XS_MAX_SOCKETS)
                     );

    mod->addConstant(type_int, "XS_IO_THREADS",
                     static_cast<int>(XS_IO_THREADS)
                     );

    mod->addConstant(type_int, "XS_PLUGIN",
                     static_cast<int>(XS_PLUGIN)
                     );

    mod->addConstant(type_int, "XS_PAIR",
                     static_cast<int>(XS_PAIR)
                     );

    mod->addConstant(type_int, "XS_PUB",
                     static_cast<int>(XS_PUB)
                     );

    mod->addConstant(type_int, "XS_SUB",
                     static_cast<int>(XS_SUB)
                     );

    mod->addConstant(type_int, "XS_REQ",
                     static_cast<int>(XS_REQ)
                     );

    mod->addConstant(type_int, "XS_REP",
                     static_cast<int>(XS_REP)
                     );

    mod->addConstant(type_int, "XS_XREQ",
                     static_cast<int>(XS_XREQ)
                     );

    mod->addConstant(type_int, "XS_XREP",
                     static_cast<int>(XS_XREP)
                     );

    mod->addConstant(type_int, "XS_PULL",
                     static_cast<int>(XS_PULL)
                     );

    mod->addConstant(type_int, "XS_PUSH",
                     static_cast<int>(XS_PUSH)
                     );

    mod->addConstant(type_int, "XS_XPUB",
                     static_cast<int>(XS_XPUB)
                     );

    mod->addConstant(type_int, "XS_XSUB",
                     static_cast<int>(XS_XSUB)
                     );

    mod->addConstant(type_int, "XS_SURVEYOR",
                     static_cast<int>(XS_SURVEYOR)
                     );

    mod->addConstant(type_int, "XS_RESPONDENT",
                     static_cast<int>(XS_RESPONDENT)
                     );

    mod->addConstant(type_int, "XS_XSURVEYOR",
                     static_cast<int>(XS_XSURVEYOR)
                     );

    mod->addConstant(type_int, "XS_XRESPONDENT",
                     static_cast<int>(XS_XRESPONDENT)
                     );

    mod->addConstant(type_int, "XS_ROUTER",
                     static_cast<int>(XS_XREP)
                     );

    mod->addConstant(type_int, "XS_DEALER",
                     static_cast<int>(XS_XREQ)
                     );

    mod->addConstant(type_int, "XS_AFFINITY",
                     static_cast<int>(XS_AFFINITY)
                     );

    mod->addConstant(type_int, "XS_IDENTITY",
                     static_cast<int>(XS_IDENTITY)
                     );

    mod->addConstant(type_int, "XS_SUBSCRIBE",
                     static_cast<int>(XS_SUBSCRIBE)
                     );

    mod->addConstant(type_int, "XS_UNSUBSCRIBE",
                     static_cast<int>(XS_UNSUBSCRIBE)
                     );

    mod->addConstant(type_int, "XS_RATE",
                     static_cast<int>(XS_RATE)
                     );

    mod->addConstant(type_int, "XS_RECOVERY_IVL",
                     static_cast<int>(XS_RECOVERY_IVL)
                     );

    mod->addConstant(type_int, "XS_SNDBUF",
                     static_cast<int>(XS_SNDBUF)
                     );

    mod->addConstant(type_int, "XS_RCVBUF",
                     static_cast<int>(XS_RCVBUF)
                     );

    mod->addConstant(type_int, "XS_RCVMORE",
                     static_cast<int>(XS_RCVMORE)
                     );

    mod->addConstant(type_int, "XS_FD",
                     static_cast<int>(XS_FD)
                     );

    mod->addConstant(type_int, "XS_EVENTS",
                     static_cast<int>(XS_EVENTS)
                     );

    mod->addConstant(type_int, "XS_TYPE",
                     static_cast<int>(XS_TYPE)
                     );

    mod->addConstant(type_int, "XS_LINGER",
                     static_cast<int>(XS_LINGER)
                     );

    mod->addConstant(type_int, "XS_RECONNECT_IVL",
                     static_cast<int>(XS_RECONNECT_IVL)
                     );

    mod->addConstant(type_int, "XS_BACKLOG",
                     static_cast<int>(XS_BACKLOG)
                     );

    mod->addConstant(type_int, "XS_RECONNECT_IVL_MAX",
                     static_cast<int>(XS_RECONNECT_IVL_MAX)
                     );

    mod->addConstant(type_int, "XS_MAXMSGSIZE",
                     static_cast<int>(XS_MAXMSGSIZE)
                     );

    mod->addConstant(type_int, "XS_SNDHWM",
                     static_cast<int>(XS_SNDHWM)
                     );

    mod->addConstant(type_int, "XS_RCVHWM",
                     static_cast<int>(XS_RCVHWM)
                     );

    mod->addConstant(type_int, "XS_MULTICAST_HOPS",
                     static_cast<int>(XS_MULTICAST_HOPS)
                     );

    mod->addConstant(type_int, "XS_RCVTIMEO",
                     static_cast<int>(XS_RCVTIMEO)
                     );

    mod->addConstant(type_int, "XS_SNDTIMEO",
                     static_cast<int>(XS_SNDTIMEO)
                     );

    mod->addConstant(type_int, "XS_IPV4ONLY",
                     static_cast<int>(XS_IPV4ONLY)
                     );

    mod->addConstant(type_int, "XS_KEEPALIVE",
                     static_cast<int>(XS_KEEPALIVE)
                     );

    mod->addConstant(type_int, "XS_SURVEY_TIMEOUT",
                     static_cast<int>(XS_SURVEY_TIMEOUT)
                     );

    mod->addConstant(type_int, "XS_MORE",
                     static_cast<int>(XS_MORE)
                     );

    mod->addConstant(type_int, "XS_DONTWAIT",
                     static_cast<int>(XS_DONTWAIT)
                     );

    mod->addConstant(type_int, "XS_SNDMORE",
                     static_cast<int>(XS_SNDMORE)
                     );

    mod->addConstant(type_int, "XS_POLLIN",
                     static_cast<int>(XS_POLLIN)
                     );

    mod->addConstant(type_int, "XS_POLLOUT",
                     static_cast<int>(XS_POLLOUT)
                     );

    mod->addConstant(type_int, "XS_POLLERR",
                     static_cast<int>(XS_POLLERR)
                     );
}
