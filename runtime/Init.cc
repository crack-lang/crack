// Copyright 2010-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// Copyright 2012 Arno Rehn <arno@arnorehn.de>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "debug/DebugTools.h"
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "Dir.h"
#include "Util.h"
#include "Net.h"
#include "Exceptions.h"
#include "Process.h"
using namespace crack::ext;
using namespace crack::runtime;

extern "C" bool __CrackUncaughtException();
extern "C" void crack_runtime_time_cinit(crack::ext::Module *mod);
extern "C" void crack_runtime_md5_cinit(crack::ext::Module *mod);
extern "C" void crack_runtime_xdr_cinit(crack::ext::Module *mod);


// stat() appears to have some funny linkage issues in native mode so we wrap 
// it in a normal function.
extern "C" int crack_runtime_stat(const char *path, struct stat *buf) {
    return stat(path, buf);
}

// Forward declare the math module initializer.
extern "C" void crack_runtime__math_cinit(Module *mod);

extern "C"
void crack_runtime_rinit(void) {
    return;
}

// We have to wrap all variations of these because the C library takes 
// liberties doing name translation.

extern "C" int crk_creat(const char *pathname, mode_t mode) {
    return creat(pathname, mode);
}

extern "C" int crk_open(const char *pathname, mode_t mode) {
    return creat(pathname, mode);
}

extern "C" int crk_open2(const char *pathname, int flags, mode_t mode) {
    return open(pathname, flags, mode);
}

extern "C" void crack_runtime_cinit(Module *mod) {
    Type *byteptrType = mod->getByteptrType();
    Type *boolType = mod->getBoolType();
    Type *intType = mod->getIntType();
    Type *intzType = mod->getIntzType();
    Type *uintzType = mod->getUintzType();
    Type *uint16Type = mod->getUint16Type();
    Type *uint32Type = mod->getUint32Type();
    Type *int16Type = mod->getInt16Type();
    Type *int32Type = mod->getInt32Type();
    Type *int64Type = mod->getInt64Type();
    Type *uintType = mod->getUintType();
    Type *byteType = mod->getByteType();
    Type *voidType = mod->getVoidType();
    Type *voidptrType = mod->getVoidptrType();

    Type *baseArrayType = mod->getType("array");
    // array[byteptr]
    Type *byteptrArrayType;
    {
        std::vector<Type *> params(1);
        params[0] = byteptrType;
        byteptrArrayType = baseArrayType->getSpecialization(params);
    }
    byteptrArrayType->finish();

    Type *cdentType = mod->addType("DirEntry", 
                                   sizeof(crack::runtime::DirEntry)
                                   );
    cdentType->addInstVar(byteptrType, "name", 
                          CRACK_OFFSET(crack::runtime::DirEntry, name)
                          );
    cdentType->addInstVar(intType, "type", 
                          CRACK_OFFSET(crack::runtime::DirEntry, type)
                          );
    cdentType->finish();

    Type *cdType = mod->addType("Dir", sizeof(crack::runtime::Dir));
    // XXX should this be part of addType?
    cdType->addMethod(boolType, "oper to .builtin.bool", 
                      (void *)crack::runtime::Dir_toBool
                      );
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

    f = mod->addFunc(intType, "is_file", (void *)crack::runtime::is_file);
    f->addArg(byteptrType, "path");
    
    f = mod->addFunc(boolType, "fileExists",
                     (void *)crack::runtime::fileExists
                     );
    f->addArg(byteptrType, "path");
    
    Type *statType = mod->addType("Stat", sizeof(struct stat));
    statType->addInstVar(intType, "st_dev", CRACK_OFFSET(struct stat, st_dev));
    statType->addInstVar(intType, "st_ino", CRACK_OFFSET(struct stat, st_ino));
    statType->addInstVar(intType, "st_mode", CRACK_OFFSET(struct stat, st_mode));
    statType->addInstVar(intType, "st_nlink", 
                         CRACK_OFFSET(struct stat, st_nlink)
                         );
    statType->addInstVar(intType, "st_uid", CRACK_OFFSET(struct stat, st_uid));
    statType->addInstVar(intType, "st_gid", CRACK_OFFSET(struct stat, st_gid));
    statType->addInstVar(intType, "st_rdev", CRACK_OFFSET(struct stat, st_rdev));
    statType->addInstVar(intType, "st_size", CRACK_OFFSET(struct stat, st_size));
    statType->addInstVar(intType, "st_blksize", 
                         CRACK_OFFSET(struct stat, st_blksize)
                         );
    statType->addInstVar(intType, "st_blocks", 
                         CRACK_OFFSET(struct stat, st_blocks)
                         );
    statType->addInstVar(intType, "st_atime", 
                         CRACK_OFFSET(struct stat, st_atime)
                         );
    statType->addInstVar(intType, "st_mtime", 
                         CRACK_OFFSET(struct stat, st_mtime)
                         );
    statType->addInstVar(intType, "st_ctime", 
                         CRACK_OFFSET(struct stat, st_ctime)
                         );
    statType->addConstructor();
    statType->finish();
    
    mod->addConstant(intType, "S_IFMT", S_IFMT);
    mod->addConstant(intType, "S_IFSOCK", S_IFSOCK);
    mod->addConstant(intType, "S_IFLNK", S_IFLNK);
    mod->addConstant(intType, "S_IFREG", S_IFREG);
    mod->addConstant(intType, "S_IFBLK", S_IFBLK);
    mod->addConstant(intType, "S_IFDIR", S_IFDIR);
    mod->addConstant(intType, "S_IFCHR", S_IFCHR);
    mod->addConstant(intType, "S_IFIFO", S_IFIFO);
    mod->addConstant(intType, "S_ISUID", S_ISUID);
    mod->addConstant(intType, "S_ISGID", S_ISGID);
    mod->addConstant(intType, "S_ISVTX", S_ISVTX);
    mod->addConstant(intType, "S_IRWXU", S_IRWXU);
    mod->addConstant(intType, "S_IRUSR", S_IRUSR);
    mod->addConstant(intType, "S_IWUSR", S_IWUSR);
    mod->addConstant(intType, "S_IXUSR", S_IXUSR);
    mod->addConstant(intType, "S_IRWXG", S_IRWXG);
    mod->addConstant(intType, "S_IRGRP", S_IRGRP);
    mod->addConstant(intType, "S_IWGRP", S_IWGRP);
    mod->addConstant(intType, "S_IXGRP", S_IXGRP);
    mod->addConstant(intType, "S_IRWXO", S_IRWXO);
    mod->addConstant(intType, "S_IROTH", S_IROTH);
    mod->addConstant(intType, "S_IWOTH", S_IWOTH);
    mod->addConstant(intType, "S_IXOTH", S_IXOTH);


    f = mod->addFunc(intType, "stat", (void *)crack_runtime_stat, 
                     "crack_runtime_stat"
                     );
    f->addArg(byteptrType, "path");
    f->addArg(statType, "buf");

    f = mod->addFunc(intType, "fileRemove", (void *)remove);
    f->addArg(byteptrType, "path");

    f = mod->addFunc(intType, "rename", (void *)rename);
    f->addArg(byteptrType, "oldPath");
    f->addArg(byteptrType, "newPath");

    f = mod->addFunc(intType, "mkdir", (void *)mkdir);
    f->addArg(byteptrType, "path");
    f->addArg(intType, "mode");

    mod->addConstant(intType, "EACCES", EACCES);
    mod->addConstant(intType, "EBADF", EBADF);
    mod->addConstant(intType, "EEXIST", EEXIST);
    mod->addConstant(intType, "EFAULT", EFAULT);
    mod->addConstant(intType, "ELOOP", ELOOP);
    mod->addConstant(intType, "ENAMETOOLONG", ENAMETOOLONG);
    mod->addConstant(intType, "ENOENT", ENOENT);
    mod->addConstant(intType, "ENOMEM", ENOMEM);
    mod->addConstant(intType, "ENOTDIR", ENOTDIR);
    mod->addConstant(intType, "EOVERFLOW", EOVERFLOW);
    mod->addConstant(intType, "EXDEV", EXDEV);


    f = mod->addFunc(byteptrType, "c_strerror",
                     (void *)crack::runtime::strerror
                     );
    
    f = mod->addFunc(mod->getIntType(), "float_str", 
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
    
    f = mod->addFunc(intType, "random", (void *)random);

    f = mod->addFunc(voidType, "srandom", (void *)srandom);
    f->addArg(uintType, "seed");

    f = mod->addFunc(byteptrType, "initstate", (void *)initstate);
    f->addArg(uintType, "seed");
    f->addArg(byteptrType, "state");
    f->addArg(uintType, "n");

    f = mod->addFunc(byteptrType, "setstate", (void *)setstate);
    f->addArg(byteptrType, "state");

    f = mod->addFunc(intType, "puts",
                     (void *)crack::runtime::crk_puts
                     );
    f->addArg(mod->getByteptrType(), "str");
    f = mod->addFunc(intType, "putc",
                     (void *)crack::runtime::crk_putc
                     );
    f->addArg(mod->getByteType(), "byte");

    f = mod->addFunc(byteptrType, "__die",
                     (void *)crack::runtime::crk_die
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
                     (void *)crack::runtime::printint64
                     );
    f->addArg(mod->getInt64Type(), "val");

    f = mod->addFunc(byteptrType, "printuint64",
                     (void *)crack::runtime::printint64
                     );
    f->addArg(mod->getUint64Type(), "val");

    // normal file open and close.

    f = mod->addFunc(intType, "open", (void *)crk_open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "open", (void *)crk_open2);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "flags");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "creat", (void *)crk_creat);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "unlink", (void *)unlink);
    f->addArg(byteptrType, "pathname");

    f = mod->addFunc(byteptrType, "tempnam", (void *)tempnam);
    f->addArg(byteptrType, "prefix");
    f->addArg(byteptrType, "dir");

    mod->addConstant(intType, "O_RDONLY", O_RDONLY);
    mod->addConstant(intType, "O_WRONLY", O_WRONLY);
    mod->addConstant(intType, "O_RDWR", O_RDWR);
    mod->addConstant(intType, "O_APPEND", O_APPEND);
    mod->addConstant(intType, "O_ASYNC", O_ASYNC);
    mod->addConstant(intType, "O_CREAT", O_CREAT);
    mod->addConstant(intType, "O_EXCL", O_EXCL);
    mod->addConstant(intType, "O_TRUNC", O_TRUNC);
    mod->addConstant(intType, "O_NONBLOCK", O_NONBLOCK);

    f = mod->addFunc(byteptrType, "getcwd", (void *)getcwd);
    f->addArg(byteptrType, "buffer");
    f->addArg(uintzType, "size");

    mod->addConstant(uintzType, "PATH_MAX", PATH_MAX);

    f = mod->addFunc(intType, "chdir", (void *)chdir);
    f->addArg(byteptrType, "path");

    f = mod->addFunc(intType, "fcntl", (void *)crack::runtime::crk_fcntl1);
    f->addArg(intType, "fd");
    f->addArg(intType, "cmd");

    f = mod->addFunc(intType, "fcntl", (void *)crack::runtime::crk_fcntl2);
    f->addArg(intType, "fd");
    f->addArg(intType, "cmd");
    f->addArg(int64Type, "arg");

    // Constants for fcntl
    mod->addConstant(intType, "F_GETFD", F_GETFD);
    mod->addConstant(intType, "F_SETFD", F_SETFD);
    mod->addConstant(intType, "F_GETFL", F_GETFL);
    mod->addConstant(intType, "F_SETFL", F_SETFL);
    mod->addConstant(intType, "F_GETOWN", F_GETOWN);
    mod->addConstant(intType, "F_SETOWN", F_SETOWN);
#ifdef __linux__ 
    mod->addConstant(intType, "F_GETSIG", F_GETSIG);
    mod->addConstant(intType, "F_SETSIG", F_SETSIG);
    mod->addConstant(intType, "F_NOTIFY", F_NOTIFY);
#endif


    // Net

    mod->addConstant(intType, "AF_UNIX", AF_UNIX);
    mod->addConstant(intType, "AF_LOCAL", AF_LOCAL);
    mod->addConstant(intType, "AF_INET", AF_INET);
    mod->addConstant(intType, "AF_INET6", AF_INET6);
    mod->addConstant(intType, "AF_IPX", AF_IPX);
#ifdef __linux__
    mod->addConstant(intType, "AF_NETLINK", AF_NETLINK);
    mod->addConstant(intType, "AF_X25", AF_X25);
    mod->addConstant(intType, "AF_AX25", AF_AX25);
    mod->addConstant(intType, "AF_ATMPVC", AF_ATMPVC);
    mod->addConstant(intType, "AF_PACKET", AF_PACKET);
    mod->addConstant(intType, "SOCK_PACKET", SOCK_PACKET);
#endif
    mod->addConstant(intType, "AF_APPLETALK", AF_APPLETALK);
    mod->addConstant(intType, "SOCK_STREAM", SOCK_STREAM);
    mod->addConstant(intType, "SOCK_DGRAM", SOCK_DGRAM);
    mod->addConstant(intType, "SOCK_SEQPACKET", SOCK_SEQPACKET);
    mod->addConstant(intType, "SOCK_RAW", SOCK_RAW);
    mod->addConstant(intType, "SOCK_RDM", SOCK_RDM);

    mod->addConstant(intType, "SOL_SOCKET", SOL_SOCKET);
    mod->addConstant(intType, "SO_REUSEADDR", SO_REUSEADDR);
    mod->addConstant(intType, "POLLIN", POLLIN);
    mod->addConstant(intType, "POLLOUT", POLLOUT);
    mod->addConstant(intType, "POLLPRI", POLLPRI);
    mod->addConstant(intType, "POLLERR", POLLERR);
    mod->addConstant(intType, "POLLHUP", POLLHUP);
    mod->addConstant(intType, "POLLNVAL", POLLNVAL);
    mod->addConstant(uint32Type, "INADDR_ANY", static_cast<int>(INADDR_ANY));

    mod->addConstant(intType, "EAGAIN", EAGAIN);
    mod->addConstant(intType, "EWOULDBLOCK", EWOULDBLOCK);
    
    f = mod->addFunc(uint32Type, "makeIPV4", 
                     (void*)crack::runtime::makeIPV4);
    f->addArg(byteType, "a");
    f->addArg(byteType, "b");
    f->addArg(byteType, "c");
    f->addArg(byteType, "d");

    // begin SockAddr
    Type *sockAddrType = mod->addType("SockAddr", sizeof(SockAddr));
    sockAddrType->addConstructor();
    sockAddrType->addInstVar(intType, "family", 
                             CRACK_OFFSET(SockAddr, family)
                             );
    sockAddrType->finish();
    // end SockAddr

    // begin SockAddrIn    
    Type *sockAddrInType = mod->addType("SockAddrIn", 
                                        sizeof(SockAddrIn) - sizeof(SockAddr)
                                        );
    sockAddrInType->addBase(sockAddrType);
    sockAddrInType->addInstVar(uint32Type, "addr", 
                               CRACK_OFFSET(SockAddrIn, addr)
                               );
    sockAddrInType->addInstVar(uint16Type, "port",
                               CRACK_OFFSET(SockAddrIn, port)
                               );

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
    
    f = sockAddrInType->addStaticMethod(
        uint32Type, 
        "htonl",
        (void *)&crack::runtime::SockAddrIn::crack_htonl
    );
    f->addArg(uint32Type, "val");

    f = sockAddrInType->addStaticMethod(
        uint32Type, 
        "ntohl",
        (void *)&crack::runtime::SockAddrIn::crack_ntohl
    );
    f->addArg(uint32Type, "val");

    f = sockAddrInType->addStaticMethod(
        uintType, 
        "htons",
        (void *)&crack::runtime::SockAddrIn::crack_htons
    );
    f->addArg(uintType, "val");
    
    f = sockAddrInType->addStaticMethod(
        uintType,
        "ntohs",
        (void *)&crack::runtime::SockAddrIn::crack_ntohs
    );
    f->addArg(uintType, "val");
    
    sockAddrInType->finish();
    // end SockAddrIn
    
    // begin SockAddrIn
    Type *sockAddrUnType = mod->addType("SockAddrUn",
                                        sizeof(SockAddrUn) - sizeof(SockAddr)
                                        );
    sockAddrUnType->addBase(sockAddrType);

    f = sockAddrUnType->addConstructor(
        "init",
        (void *)&crack::runtime::SockAddrUn::init
    );
    f->addArg(byteptrType, "path");
    f->addArg(uintzType, "size");

    sockAddrUnType->addMethod(byteptrType, "getPath",
                              (void *)&crack::runtime::SockAddrUn::getPath
                              );

    sockAddrUnType->finish();
    // end SockAddrUn

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
    Type *pollEventType = mod->addType("PollEvt", 
                                       sizeof(crack::runtime::PollEvt));
    pollEventType->addInstVar(intType, "fd", 
                              CRACK_OFFSET(crack::runtime::PollEvt, fd)
                              );
    pollEventType->addInstVar(intType, "events", 
                              CRACK_OFFSET(crack::runtime::PollEvt, events)
                              );
    pollEventType->addInstVar(intType, "revents", 
                              CRACK_OFFSET(crack::runtime::PollEvt, revents)
                              );
    pollEventType->addConstructor();
    pollEventType->finish();
    // end PollEvent

    // begin TimeVal
    Type *timeValType = mod->addType("TimeVal", 
                                     sizeof(crack::runtime::TimeVal)
                                     );
    timeValType->addInstVar(int32Type, "secs", 
                            CRACK_OFFSET(crack::runtime::TimeVal, secs)
                            );
    timeValType->addInstVar(int32Type, "nsecs", 
                            CRACK_OFFSET(crack::runtime::TimeVal, nsecs)
                            );

    f = timeValType->addConstructor("init", 
                                    (void *)&crack::runtime::TimeVal::init
                                    );
    f->addArg(int32Type, "secs");
    f->addArg(int32Type, "nsecs");
    
    f = timeValType->addMethod(voidType, "setToNow",
                               (void *)&crack::runtime::TimeVal::setToNow
                               );
    f->addArg(voidptrType, "tz");

    timeValType->finish();
    // end TimeVal

    // sleep
    f = mod->addFunc(intType, "sleep", (void *)sleep, "sleep");
    f->addArg(uintType, "seconds"); 

    // begin Pipe
    Type *pipeType = mod->addType("PipeAddr", sizeof(crack::runtime::PipeAddr));
    pipeType->addInstVar(int32Type, "flags", 
                            CRACK_OFFSET(crack::runtime::PipeAddr, flags)
                            );
    pipeType->addInstVar(int32Type, "readfd", 
                            CRACK_OFFSET(crack::runtime::PipeAddr, readfd)
                            );
    pipeType->addInstVar(int32Type, "writefd", 
                            CRACK_OFFSET(crack::runtime::PipeAddr, writefd)
                            );

    f = pipeType->addConstructor("init",
                                    (void *)&crack::runtime::PipeAddr::init1
                                    );
    f->addArg(int32Type, "flags");

    f = pipeType->addConstructor("init",
                                    (void *)&crack::runtime::PipeAddr::init2
                                    );
    f->addArg(int32Type, "flags");
    f->addArg(int32Type, "readfd");
    f->addArg(int32Type, "writefd");
    pipeType->finish();

    // end Pipe
    
    // begin SigSet
    Type *sigSetType = mod->addType("SigSet", 0);
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
    Type *pollSetType = mod->addType("PollSet", 0);
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

    f = pollSetType->addMethod(intType, "delete",
                               (void *)crack::runtime::PollSet_delete
                               );
    f->addArg(uintType, "size");
    f->addArg(uintType, "index");
    
    f = pollSetType->addMethod(intType, "poll",
                               (void *)crack::runtime::PollSet_poll
                               );
    f->addArg(uintType, "nfds");
    f->addArg(timeValType, "tv");
    f->addArg(sigSetType, "sigmask");

    pollSetType->finish();
    // end PollSet

    // addrinfo
    Type *addrinfoType = mod->addType("AddrInfo", sizeof(addrinfo));
    f = addrinfoType->addStaticMethod(addrinfoType, "oper new", 
                                      (void *)&crack::runtime::AddrInfo_create
                                      );
    f->addArg(byteptrType, "node");
    f->addArg(byteptrType, "service");
    f->addArg(addrinfoType, "hints");
    
    f = addrinfoType->addMethod(voidType, "free",
                                (void *)freeaddrinfo
                                );
    f->addArg(addrinfoType, "addr");
    
    f = addrinfoType->addMethod(sockAddrInType, "getInAddr",
                                (void *)crack::runtime::AddrInfo_getInAddr
                                );
    
    addrinfoType->addInstVar(intType, "ai_flags",
                             CRACK_OFFSET(addrinfo, ai_flags)
                             );
    addrinfoType->addInstVar(intType, "ai_family",
                             CRACK_OFFSET(addrinfo, ai_family)
                             );
    addrinfoType->addInstVar(intType, "ai_socktype",
                             CRACK_OFFSET(addrinfo, ai_socktype)
                             );
    addrinfoType->addInstVar(intType, "ai_protocol",
                             CRACK_OFFSET(addrinfo, ai_protocol)
                             );
    addrinfoType->addInstVar(uintzType, "ai_addrlen",
                             CRACK_OFFSET(addrinfo, ai_addrlen)
                             );
    addrinfoType->addInstVar(sockAddrType, "ai_addr",
                             CRACK_OFFSET(addrinfo, ai_addr)
                             );
    addrinfoType->addInstVar(byteptrType, "ai_canonname",
                             CRACK_OFFSET(addrinfo, ai_canonname)
                             );
    addrinfoType->addInstVar(addrinfoType, "ai_next",
                             CRACK_OFFSET(addrinfo, ai_next)
                             );
    addrinfoType->finish();
    // end addrinfo

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

    f = mod->addFunc(byteptrType, "memset", (void *)memset, "memset");
    f->addArg(byteptrType, "dst");
    f->addArg(byteType, "c");
    f->addArg(uintType, "size");

    f = mod->addFunc(intType, "memcmp", (void *)memcmp, "memcmp");
    f->addArg(byteptrType, "a");
    f->addArg(byteptrType, "b");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(byteptrType, "memmove", (void *)memmove, "memmove");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");
    f->addArg(uintType, "size");
    
    Type *cFileType = mod->addType("CFile", 0);
    cFileType->finish();
    
    f = mod->addFunc(cFileType, "fopen", (void *)fopen, "fopen");
    f->addArg(byteptrType, "path");
    f->addArg(byteptrType, "mode");
    
    f = mod->addFunc(intType, "fclose", (void *)fclose, "fclose");
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

    f = mod->addFunc(intType, "utimes", (void *)crack::runtime::setUtimes);
    f->addArg(byteptrType, "path");
    f->addArg(int64Type, "atime");
    f->addArg(int64Type, "mtime");
    f->addArg(boolType, "now");

    f = mod->addFunc(voidType, "exit", (void *)exit, "exit");
    f->addArg(intType, "status");

    f = mod->addFunc(intType, "setNonBlocking",
                     (void *)crack::runtime::setNonBlocking);
    f->addArg(intType, "fd");
    f->addArg(boolType, "val");

    // mmap
    f = mod->addFunc(voidptrType, "mmap", (void *)mmap, "mmap");
    f->addArg(voidptrType, "start");
    f->addArg(uintzType, "length"); 
    f->addArg(intType, "prot");
    f->addArg(intType, "flags");
    f->addArg(intType, "fd");   
    f->addArg(uintzType, "offset");

    // munmap
    f = mod->addFunc(intType, "munmap", (void *)mmap, "munmap");
    f->addArg(voidptrType, "start");
    f->addArg(uintzType, "length"); 

    // mmap protection flags
    mod->addConstant(intType, "PROT_NONE", PROT_NONE);
    mod->addConstant(intType, "PROT_EXEC", PROT_EXEC);
    mod->addConstant(intType, "PROT_READ", PROT_READ);
    mod->addConstant(intType, "PROT_WRITE", PROT_WRITE);
    mod->addConstant(intType, "PROT_NONE", PROT_NONE);  

    // mmap mapping flags
    mod->addConstant(intType, "MAP_FIXED", MAP_FIXED);
    mod->addConstant(intType, "MAP_SHARED", MAP_SHARED);
    mod->addConstant(intType, "MAP_PRIVATE", MAP_PRIVATE);

    // Add math functions
    crack_runtime__math_cinit(mod);
    
    // Add time functions
    crack_runtime_time_cinit(mod);
    
    // Add md5 functions
    crack_runtime_md5_cinit(mod);

    // Add xdr functions
    crack_runtime_xdr_cinit(mod);
    
    // add exception functions
    mod->addConstant(intType, "EXCEPTION_MATCH_FUNC", 
                     crack::runtime::exceptionMatchFuncHook
                     );
    mod->addConstant(intType, "BAD_CAST_FUNC",
                     crack::runtime::badCastFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_RELEASE_FUNC",
                     crack::runtime::exceptionReleaseFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_PERSONALITY_FUNC",
                     crack::runtime::exceptionPersonalityFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_FRAME_FUNC",
                     crack::runtime::exceptionFrameFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_UNCAUGHT_FUNC",
                     crack::runtime::exceptionUncaughtFuncHook
                     );
    f = mod->addFunc(voidType, "registerHook", 
                     (void *)crack::runtime::registerHook
                     );
    f->addArg(intType, "hookId");
    f->addArg(voidptrType, "hook");
    
    // This shouldn't need to be registered, but as it stands, runtime just 
    // gets loaded like any other module in JIT mode and this is resolved at 
    // runtime.
    mod->addFunc(mod->getBoolType(), "__CrackUncaughtException",
                 (void *)__CrackUncaughtException
                 );

    // Process support
    mod->addConstant(intType, "SIGABRT", SIGABRT);
    mod->addConstant(intType, "SIGALRM", SIGALRM);
    mod->addConstant(intType, "SIGBUS" , SIGBUS);
    mod->addConstant(intType, "SIGCHLD", SIGCHLD);
#ifdef __linux__
    mod->addConstant(intType, "SIGCLD" , SIGCLD);
    mod->addConstant(intType, "SIGPOLL", SIGPOLL);
    mod->addConstant(intType, "SIGPWR" , SIGPWR);
#endif
    mod->addConstant(intType, "SIGCONT", SIGCONT);
    mod->addConstant(intType, "SIGFPE" , SIGFPE);
    mod->addConstant(intType, "SIGHUP" , SIGHUP);
    mod->addConstant(intType, "SIGILL" , SIGILL);
    mod->addConstant(intType, "SIGINT" , SIGINT);
    mod->addConstant(intType, "SIGIO"  , SIGIO);
    mod->addConstant(intType, "SIGIOT" , SIGIOT);
    mod->addConstant(intType, "SIGKILL", SIGKILL);
    mod->addConstant(intType, "SIGPIPE", SIGPIPE);
    mod->addConstant(intType, "SIGPROF", SIGPROF);
    mod->addConstant(intType, "SIGQUIT", SIGQUIT);
    mod->addConstant(intType, "SIGSEGV", SIGSEGV);
    mod->addConstant(intType, "SIGSTOP", SIGSTOP);
    mod->addConstant(intType, "SIGSYS" , SIGSYS);
    mod->addConstant(intType, "SIGTERM", SIGTERM);
    mod->addConstant(intType, "SIGTRAP", SIGTRAP);
    mod->addConstant(intType, "SIGTSTP", SIGTSTP);
    mod->addConstant(intType, "SIGTTIN", SIGTTIN);
    mod->addConstant(intType, "SIGURG" , SIGURG);
    mod->addConstant(intType, "SIGUSR1", SIGUSR1);
    mod->addConstant(intType, "SIGUSR2", SIGUSR2);
    mod->addConstant(intType, "SIGVTALRM", SIGVTALRM);
    mod->addConstant(intType, "SIGWINCH" , SIGWINCH);
    mod->addConstant(intType, "SIGXCPU", SIGXCPU);
    mod->addConstant(intType, "SIGXFSZ", SIGXFSZ);

    Type *cpipeType = mod->addType("PipeDesc", 
                                   sizeof(crack::runtime::PipeDesc)
                                   );
    cpipeType->addInstVar(intType, "flags", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, flags)
                          );
    cpipeType->addInstVar(intType, "stdin", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, in)
                          );
    cpipeType->addInstVar(intType, "stdout", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, out)
                          );
    cpipeType->addInstVar(intType, "stderr", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, err)
                          );
    cpipeType->addConstructor();
    cpipeType->finish();

    f = mod->addFunc(intType, "runChildProcess",
                     (void *)&crack::runtime::runChildProcess);
    f->addArg(byteptrArrayType, "argv");
    f->addArg(byteptrArrayType, "env");
    f->addArg(cpipeType, "pipes");

    f = mod->addFunc(intType, "closeProcess",
                     (void *)&crack::runtime::closeProcess);
    f->addArg(cpipeType, "pipes");

    f = mod->addFunc(intType, "waitProcess",
                     (void *)&crack::runtime::waitProcess);
    f->addArg(intType, "pid");
    f->addArg(intType, "noHang");

    f = mod->addFunc(voidType, "signalProcess",
                     (void *)&crack::runtime::signalProcess);
    f->addArg(intType, "pid");
    f->addArg(intType, "sig");

    f = mod->addFunc(byteptrType, "iconv",
                           (void*)&crack::runtime::crk_iconv);
    f->addArg(uintType, "targetCharSize");
    f->addArg(byteptrType, "to");
    f->addArg(byteptrType, "from");
    f->addArg(byteptrType, "string");
    f->addArg(uintType, "len");
    f->addArg(voidptrType, "convertedLen");

    // debug support - these are weird in that these functions actually reside 
    // in libCrackLang.
    f = mod->addFunc(voidType, "getLocation", 
                     (void *)&crack::debug::getLocation
                     );
    f->addArg(voidptrType, "address");
    f->addArg(byteptrArrayType, "info");
    f = mod->addFunc(voidType, "registerFuncTable",
                     (void *)&crack::debug::registerFuncTable
                     );
    f->addArg(byteptrArrayType, "funcTable");
    f = mod->addFunc(voidptrType, "getStackFrame",
                     (void *)&crack::debug::getStackFrame
                     );
}
