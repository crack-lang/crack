
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "Dir.h"
#include "Util.h"
#include "Net.h"
#include "Math.h"
#include "Exceptions.h"
using namespace crack::ext;

extern "C" void crack_runtime_init(Module *mod) {
    Type *byteptrType = mod->getByteptrType();
    Type *intType = mod->getIntType();
    Type *uint32Type = mod->getUint32Type();
    Type *int32Type = mod->getInt32Type();
    Type *uintType = mod->getUintType();
    Type *byteType = mod->getByteType();
    Type *voidType = mod->getVoidType();
    Type *voidptrType = mod->getVoidptrType();

    Type *cdentType = mod->addType("DirEntry");
    cdentType->addInstVar(byteptrType, "name");
    cdentType->addInstVar(intType, "type");
    cdentType->finish();

    Type *cdType = mod->addType("Dir");
    cdType->finish();
    
    Func *f = mod->addFunc(cdType, "opendir", (void *)crack::runtime::opendir);
    f->addArg(byteptrType, "name");
    
    f = mod->addFunc(cdentType, "getDirEntry", 
                     (void *)crack::runtime::getDirEntry
                     );
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "closedir", (void *)crack::runtime::closedir);
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "readdir", (void *)crack::runtime::readdir);
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "fnmatch", (void *)crack::runtime::fnmatch);
    f->addArg(byteptrType, "pattern");
    f->addArg(byteptrType, "string");
    
    f = mod->addFunc(byteptrType, "basename", (void *)basename, "basename");
    f->addArg(byteptrType, "path");
    
    f = mod->addFunc(byteptrType, "dirname", (void *)dirname, "dirname");
    f->addArg(byteptrType, "path");
    
    f = mod->addFunc(byteptrType, "c_strerror",
                     (void *)crack::runtime::strerror
                     );
    
    f = mod->addFunc(mod->getVoidType(), "float_str", 
                     (void *)crack::runtime::float_str
                     );
    f->addArg(mod->getFloat64Type(), "val");
    f->addArg(byteptrType, "buf");
    f->addArg(mod->getUintType(), "size");

    f = mod->addFunc(uintType, "rand", 
                     (void *)crack::runtime::rand
                     );
    f->addArg(mod->getUintType(), "low");
    f->addArg(mod->getUintType(), "high");

    f = mod->addFunc(byteptrType, "puts",
                     (void *)crack::runtime::puts
                     );
    f->addArg(mod->getByteptrType(), "str");

    f = mod->addFunc(byteptrType, "__die",
                     (void *)crack::runtime::__die
                     );
    f->addArg(mod->getByteptrType(), "message");

    f = mod->addFunc(byteptrType, "printfloat",
                     (void *)crack::runtime::printfloat
                     );
    f->addArg(mod->getFloatType(), "val");

    f = mod->addFunc(byteptrType, "printint",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getIntType(), "val");

    f = mod->addFunc(byteptrType, "printint64",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getInt64Type(), "val");

    f = mod->addFunc(byteptrType, "printuint64",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getUint64Type(), "val");

    // normal file open and close.

    f = mod->addFunc(intType, "open", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "open", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "flags");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "creat", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    mod->addConstant(intType, "O_RDONLY", O_RDONLY);
    mod->addConstant(intType, "O_WRONLY", O_WRONLY);
    mod->addConstant(intType, "O_RDWR", O_RDWR);
    mod->addConstant(intType, "O_APPEND", O_APPEND);
    mod->addConstant(intType, "O_ASYNC", O_ASYNC);
    mod->addConstant(intType, "O_CREAT", O_CREAT);

    // Net
    
    Type *netConstantsType = mod->addType("Constants");
    netConstantsType->addInstVar(intType, "AF_UNIX");
    netConstantsType->addInstVar(intType, "AF_LOCAL");
    netConstantsType->addInstVar(intType, "AF_INET");
    netConstantsType->addInstVar(intType, "AF_INET6");
    netConstantsType->addInstVar(intType, "AF_IPX");
    netConstantsType->addInstVar(intType, "AF_NETLINK");
    netConstantsType->addInstVar(intType, "AF_X25");
    netConstantsType->addInstVar(intType, "AF_AX25");
    netConstantsType->addInstVar(intType, "AF_ATMPVC");
    netConstantsType->addInstVar(intType, "AF_APPLETALK");
    netConstantsType->addInstVar(intType, "AF_PACKET");
    netConstantsType->addInstVar(intType, "SOCK_STREAM");
    netConstantsType->addInstVar(intType, "SOCK_DGRAM");
    netConstantsType->addInstVar(intType, "SOCK_SEQPACKET");
    netConstantsType->addInstVar(intType, "SOCK_RAW");
    netConstantsType->addInstVar(intType, "SOCK_RDM");
    netConstantsType->addInstVar(intType, "SOCK_PACKET");
    netConstantsType->addInstVar(intType, "SOCK_NONBLOCK");
    netConstantsType->addInstVar(intType, "SOCK_CLOEXEC");
    netConstantsType->addInstVar(intType, "SOL_SOCKET");
    netConstantsType->addInstVar(intType, "SO_REUSEADDR");
    netConstantsType->addInstVar(intType, "POLLIN");
    netConstantsType->addInstVar(intType, "POLLOUT");
    netConstantsType->addInstVar(intType, "POLLPRI");
    netConstantsType->addInstVar(intType, "POLLERR");
    netConstantsType->addInstVar(intType, "POLLHUP");
    netConstantsType->addInstVar(intType, "POLLNVAL");
    netConstantsType->addInstVar(uint32Type, "INADDR_ANY");
    netConstantsType->addStaticMethod(netConstantsType, "oper new",
                                      (void *)crack::runtime::getConstants);
    netConstantsType->finish();
    
    f = mod->addFunc(uint32Type, "makeIPV4", 
                     (void*)crack::runtime::makeIPV4);
    f->addArg(byteType, "a");
    f->addArg(byteType, "b");
    f->addArg(byteType, "c");
    f->addArg(byteType, "d");

    // begin SockAddr
    Type *sockAddrType = mod->addType("SockAddr");
    sockAddrType->addConstructor();
    sockAddrType->addInstVar(intType, "family");
    sockAddrType->finish();
    // end SockAddr

    // begin SockAddrIn    
    Type *sockAddrInType = mod->addType("SockAddrIn");
    sockAddrInType->addBase(sockAddrType);
    sockAddrInType->addInstVar(uint32Type, "addr");
    sockAddrInType->addInstVar(uintType, "port");

    f = sockAddrInType->addConstructor(
        "init",
        (void *)&crack::runtime::SockAddrIn::init1
    );
    f->addArg(byteType, "a");
    f->addArg(byteType, "b");
    f->addArg(byteType, "c");
    f->addArg(byteType, "d");
    f->addArg(intType, "port");

    f = sockAddrInType->addConstructor(
        "init",
        (void *)&crack::runtime::SockAddrIn::init2
    );
    f->addArg(uint32Type, "addr");
    f->addArg(uintType, "port");
    
    sockAddrInType->finish();
    // end SockAddrIn
    
    f = mod->addFunc(intType, "connect", (void *)crack::runtime::connect);
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "bind",
                     (void *)crack::runtime::bind
                     );
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "accept",
                     (void *)crack::runtime::accept
                     );
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "setsockopt",
                     (void *)crack::runtime::setsockopt_int
                     );
    f->addArg(intType, "s");
    f->addArg(intType, "level");
    f->addArg(intType, "optname");
    f->addArg(intType, "val");

    // begin PollEvt
    Type *pollEventType = mod->addType("PollEvt");
    pollEventType->addInstVar(intType, "fd");
    pollEventType->addInstVar(intType, "events");
    pollEventType->addInstVar(intType, "revents");
    pollEventType->addConstructor();
    pollEventType->finish();
    // end PollEvent

    // begin TimeVal
    Type *timeValType = mod->addType("TimeVal");
    timeValType->addInstVar(int32Type, "secs");
    timeValType->addInstVar(int32Type, "nsecs");

    f = timeValType->addConstructor("init", 
                                    (void *)&crack::runtime::TimeVal::init
                                    );
    f->addArg(int32Type, "secs");
    f->addArg(int32Type, "nsecs");
    
    f = timeValType->addMethod(voidType, "setToNow",
                               (void *)gettimeofday
                               );
    f->addArg(voidptrType, "tz");

    timeValType->finish();
    // end TimeVal
    
    // begin SigSet
    Type *sigSetType = mod->addType("SigSet");
    f = sigSetType->addStaticMethod(sigSetType, "oper new",
                                    (void *)crack::runtime::SigSet_create
                                    );
    
    f = sigSetType->addMethod(voidType, "destroy",
                              (void *)crack::runtime::SigSet_destroy
                              );

    f = sigSetType->addMethod(voidType, "empty",
                              (void *)crack::runtime::SigSet_empty
                              );
    
    f = sigSetType->addMethod(voidType, "fill",
                              (void *)crack::runtime::SigSet_fill
                              );

    f = sigSetType->addMethod(voidType, "add",
                              (void *)crack::runtime::SigSet_add
                              );
    f->addArg(intType, "signum");

    f = sigSetType->addMethod(voidType, "del",
                              (void *)crack::runtime::SigSet_del
                              );
    f->addArg(intType, "signum");

    f = sigSetType->addMethod(voidType, "has",
                              (void *)crack::runtime::SigSet_has
                              );
    f->addArg(intType, "signum");

    sigSetType->finish();
    // end SigSet

    // begin PollSet
    Type *pollSetType = mod->addType("PollSet");
    f = pollSetType->addStaticMethod(pollSetType, "oper new",
                                     (void *)&crack::runtime::PollSet_create
                                     );
    f->addArg(uintType, "size");
    
    f = pollSetType->addMethod(voidType, "copy",
                               (void *)crack::runtime::PollSet_copy
                               );
    f->addArg(pollSetType, "src");
    f->addArg(uintType, "size");
    
    f = pollSetType->addMethod(voidType, "destroy",
                               (void *)crack::runtime::PollSet_destroy
                               );
    f = pollSetType->addMethod(voidType, "set",
                               (void *)crack::runtime::PollSet_set
                               );
    f->addArg(uintType, "index");
    f->addArg(intType, "fd");
    f->addArg(intType, "events");
    f->addArg(intType, "revents");

    f = pollSetType->addMethod(voidType, "get",
                               (void *)crack::runtime::PollSet_get
                               );
    f->addArg(uintType, "index");
    f->addArg(pollEventType, "event");

    f = pollSetType->addMethod(intType, "next",
                               (void *)crack::runtime::PollSet_next
                               );
    f->addArg(uintType, "size");
    f->addArg(uintType, "index");
    f->addArg(pollEventType, "outputEntry");
    
    f = pollSetType->addMethod(intType, "poll",
                               (void *)crack::runtime::PollSet_poll
                               );
    f->addArg(uintType, "nfds");
    f->addArg(timeValType, "tv");
    f->addArg(sigSetType, "sigmask");

    pollSetType->finish();
    // end PollSet

    // misc C functions
    f = mod->addFunc(intType, "close", (void *)close, "close");
    f->addArg(intType, "fd");
    
    f = mod->addFunc(intType, "socket", (void *)socket, "socket");
    f->addArg(intType, "domain");
    f->addArg(intType, "type");
    f->addArg(intType, "protocol");
    
    f = mod->addFunc(intType, "listen", (void *)listen, "listen");
    f->addArg(intType, "fd");
    f->addArg(intType, "backlog");
    
    f = mod->addFunc(intType, "send", (void *)send, "send");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "size");
    f->addArg(intType, "flags");
    
    f = mod->addFunc(intType, "recv", (void *)recv, "recv");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "size");
    f->addArg(intType, "flags");

    f = mod->addFunc(voidType, "abort", (void *)abort, "abort");

    f = mod->addFunc(voidType, "free", (void *)free, "free");
    f->addArg(voidptrType, "size");
    
    f = mod->addFunc(voidType, "strcpy", (void *)strcpy, "strcpy");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");

    f = mod->addFunc(uintType, "strlen", (void *)strlen, "strlen");
    f->addArg(byteptrType, "str");

    f = mod->addFunc(byteptrType, "malloc", (void *)malloc, "malloc");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(byteptrType, "memcpy", (void *)memcpy, "memcpy");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(intType, "memcmp", (void *)memcmp, "memcmp");
    f->addArg(byteptrType, "a");
    f->addArg(byteptrType, "b");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(byteptrType, "memmove", (void *)memmove, "memmove");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");
    f->addArg(uintType, "size");
    
    Type *cFileType = mod->addType("CFile");
    cFileType->finish();
    
    f = mod->addFunc(cFileType, "fopen", (void *)fopen, "fopen");
    f->addArg(byteptrType, "path");
    f->addArg(byteptrType, "mode");
    
    f = mod->addFunc(intType, "fclose", (void *)fclose, "close");
    f->addArg(cFileType, "fp");
    
    f = mod->addFunc(intType, "fileno", (void *)fileno, "fileno");
    f->addArg(cFileType, "fp");

    f = mod->addFunc(intType, "read", (void *)read, "read");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "count");

    f = mod->addFunc(intType, "write", (void *)write, "write");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "count");

    f = mod->addFunc(voidType, "exit", (void *)exit, "exit");
    f->addArg(intType, "status");

    // Add math functions
    crack::runtime::math_init(mod);
    
    // add exception functions
    mod->addConstant(intType, "EXCEPTION_MATCH_FUNC", 
                     crack::runtime::exceptionMatchFuncHook
                     );
    f = mod->addFunc(voidType, "registerHook", 
                     (void *)crack::runtime::registerHook
                     );
    f->addArg(intType, "hookId");
    f->addArg(voidptrType, "hook");
}
