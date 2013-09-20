#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <lmdb.h>
typedef void * voidptr;
typedef char * byteptr;
typedef int Undef;
MDB_val *mdb_val_new() {
   return (MDB_val *)malloc(sizeof(MDB_val));
}
MDB_env *mdb_env_create_crk() {
    MDB_env *env;
    errno = mdb_env_create(&env);
    if (errno) return NULL;
    return env;
}
byteptr mdb_strerror_crk() {
    return mdb_strerror(errno);
}
unsigned int mdb_env_get_flags_crk(MDB_env *env) {
    unsigned int flags;
    errno = mdb_env_get_flags(env, &flags);
    if (errno) return 0;
    return flags;
}
MDB_txn *mdb_txn_begin_crk(MDB_env *env, MDB_txn *parent,
                            unsigned int flags)
{
    MDB_txn *txn;
    errno = mdb_txn_begin(env, parent, flags, &txn);
    if (errno) return NULL;
    return txn;
}
unsigned int mdb_dbi_open_crk(MDB_txn *txn, const char *name,
                           unsigned int flags)
{
    unsigned int dbi;
    errno = mdb_dbi_open(txn, name, flags, &dbi);
    if (errno) return 0;
    return dbi;
}
MDB_cursor *mdb_cursor_open_crk(MDB_txn *txn, unsigned int dbi) {
    MDB_cursor *cursor;
    errno = mdb_cursor_open(txn, dbi, &cursor);
    if (errno) return NULL;
    return cursor;
}
uint64_t mdb_cursor_count_crk(MDB_cursor *cursor) {
    size_t countp;
    errno = mdb_cursor_count(cursor, &countp);
    if (errno) return 0;
    return countp;
}
int mdb_reader_check_crk(MDB_env *env) {
    int num_cleared;
    errno = mdb_reader_check(env, &num_cleared);
    if (errno) return -1;
    return num_cleared;
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__lmdb_rinit() {
    return;
}

extern "C"
void crack_ext__lmdb_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_MDB_env = mod->addType("MDB_env", sizeof(Undef));
    type_MDB_env->finish();


    crack::ext::Type *type_MDB_cursor = mod->addType("MDB_cursor", sizeof(Undef));
    type_MDB_cursor->finish();


    crack::ext::Type *type_MDB_txn = mod->addType("MDB_txn", sizeof(Undef));
    type_MDB_txn->finish();


    crack::ext::Type *type_MDB_val = mod->addType("MDB_val", sizeof(MDB_val));
        type_MDB_val->addInstVar(type_uint64, "mv_size",
                                CRACK_OFFSET(MDB_val, mv_size));
        type_MDB_val->addInstVar(type_byteptr, "mv_data",
                                CRACK_OFFSET(MDB_val, mv_data));
    type_MDB_val->finish();


    crack::ext::Type *type_MDB_stat = mod->addType("MDB_stat", sizeof(MDB_stat));
        type_MDB_stat->addInstVar(type_uint, "ms_psize",
                                CRACK_OFFSET(MDB_stat, ms_psize));
        type_MDB_stat->addInstVar(type_uint, "ms_depth",
                                CRACK_OFFSET(MDB_stat, ms_depth));
        type_MDB_stat->addInstVar(type_uint64, "ms_branch_pages",
                                CRACK_OFFSET(MDB_stat, ms_branch_pages));
        type_MDB_stat->addInstVar(type_uint64, "ms_leaf_pages",
                                CRACK_OFFSET(MDB_stat, ms_leaf_pages));
        type_MDB_stat->addInstVar(type_uint64, "ms_overflow_pages",
                                CRACK_OFFSET(MDB_stat, ms_overflow_pages));
        type_MDB_stat->addInstVar(type_uint64, "ms_entries",
                                CRACK_OFFSET(MDB_stat, ms_entries));
    type_MDB_stat->finish();

    f = mod->addFunc(type_MDB_val, "mdb_val_new",
                     (void *)mdb_val_new
                     );

    f = mod->addFunc(type_MDB_env, "mdb_env_create",
                     (void *)mdb_env_create_crk
                     );

    f = mod->addFunc(type_void, "mdb_env_close",
                     (void *)mdb_env_close
                     );
       f->addArg(type_MDB_env, "env");

    f = mod->addFunc(type_int, "mdb_env_set_mapsize",
                     (void *)mdb_env_set_mapsize
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_uint64, "size");

    f = mod->addFunc(type_int, "mdb_env_set_maxdbs",
                     (void *)mdb_env_set_maxdbs
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_uint, "size");

    f = mod->addFunc(type_int, "mdb_env_open",
                     (void *)mdb_env_open
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_byteptr, "path");
       f->addArg(type_uint, "flags");
       f->addArg(type_int, "mode");

    f = mod->addFunc(type_byteptr, "mdb_strerror",
                     (void *)mdb_strerror
                     );
       f->addArg(type_int, "err");

    f = mod->addFunc(type_byteptr, "mdb_strerror",
                     (void *)mdb_strerror_crk
                     );

    f = mod->addFunc(type_int, "mdb_env_copy",
                     (void *)mdb_env_copy
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_byteptr, "path");

    f = mod->addFunc(type_int, "mdb_env_copyfd",
                     (void *)mdb_env_copyfd
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_int, "fd");

    f = mod->addFunc(type_int, "mdb_env_stat",
                     (void *)mdb_env_stat
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_MDB_stat, "stat");

    f = mod->addFunc(type_int, "mdb_env_sync",
                     (void *)mdb_env_sync
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_int, "force");

    f = mod->addFunc(type_int, "mdb_env_set_flags",
                     (void *)mdb_env_set_flags
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_uint, "flags");
       f->addArg(type_int, "onoff");

    f = mod->addFunc(type_uint, "mdb_env_get_flags",
                     (void *)mdb_env_get_flags_crk
                     );
       f->addArg(type_MDB_env, "env");

    f = mod->addFunc(type_MDB_txn, "mdb_txn_begin",
                     (void *)mdb_txn_begin_crk
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_MDB_txn, "parent");
       f->addArg(type_uint, "flags");

    f = mod->addFunc(type_MDB_env, "mdb_txn_env",
                     (void *)mdb_txn_env
                     );
       f->addArg(type_MDB_txn, "txn");

    f = mod->addFunc(type_int, "mdb_txn_commit",
                     (void *)mdb_txn_commit
                     );
       f->addArg(type_MDB_txn, "txn");

    f = mod->addFunc(type_void, "mdb_txn_abort",
                     (void *)mdb_txn_abort
                     );
       f->addArg(type_MDB_txn, "txn");

    f = mod->addFunc(type_void, "mdb_txn_reset",
                     (void *)mdb_txn_reset
                     );
       f->addArg(type_MDB_txn, "txn");

    f = mod->addFunc(type_int, "mdb_txn_renew",
                     (void *)mdb_txn_renew
                     );
       f->addArg(type_MDB_txn, "txn");

    f = mod->addFunc(type_uint, "mdb_dbi_open",
                     (void *)mdb_dbi_open_crk
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_byteptr, "name");
       f->addArg(type_uint, "flags");

    f = mod->addFunc(type_void, "mdb_dbi_close",
                     (void *)mdb_dbi_close
                     );
       f->addArg(type_MDB_env, "env");
       f->addArg(type_uint, "dbi");

    f = mod->addFunc(type_int, "mdb_drop",
                     (void *)mdb_drop
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_uint, "dbi");
       f->addArg(type_int, "del");

    f = mod->addFunc(type_int, "mdb_get",
                     (void *)mdb_get
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_uint, "dbi");
       f->addArg(type_MDB_val, "key");
       f->addArg(type_MDB_val, "data");

    f = mod->addFunc(type_int, "mdb_put",
                     (void *)mdb_put
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_uint, "dbi");
       f->addArg(type_MDB_val, "key");
       f->addArg(type_MDB_val, "data");
       f->addArg(type_uint, "flags");

    f = mod->addFunc(type_int, "mdb_del",
                     (void *)mdb_del
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_uint, "dbi");
       f->addArg(type_MDB_val, "key");
       f->addArg(type_MDB_val, "data");

    f = mod->addFunc(type_MDB_cursor, "mdb_cursor_open",
                     (void *)mdb_cursor_open_crk
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_uint, "dbi");

    f = mod->addFunc(type_void, "mdb_cursor_close",
                     (void *)mdb_cursor_close
                     );
       f->addArg(type_MDB_cursor, "cursor");

    f = mod->addFunc(type_int, "mdb_cursor_renew",
                     (void *)mdb_cursor_renew
                     );
       f->addArg(type_MDB_txn, "txn");
       f->addArg(type_MDB_cursor, "cursor");

    f = mod->addFunc(type_MDB_txn, "mdb_cursor_txn",
                     (void *)mdb_cursor_txn
                     );
       f->addArg(type_MDB_cursor, "cursor");

    f = mod->addFunc(type_uint, "mdb_cursor_dbi",
                     (void *)mdb_cursor_dbi
                     );
       f->addArg(type_MDB_cursor, "cursor");

    f = mod->addFunc(type_int, "mdb_cursor_get",
                     (void *)mdb_cursor_get
                     );
       f->addArg(type_MDB_cursor, "cursor");
       f->addArg(type_MDB_val, "key");
       f->addArg(type_MDB_val, "data");
       f->addArg(type_int, "op");

    f = mod->addFunc(type_int, "mdb_cursor_put",
                     (void *)mdb_cursor_put
                     );
       f->addArg(type_MDB_cursor, "cursor");
       f->addArg(type_MDB_val, "key");
       f->addArg(type_MDB_val, "data");
       f->addArg(type_uint, "flags");

    f = mod->addFunc(type_int, "mdb_cursor_del",
                     (void *)mdb_cursor_del
                     );
       f->addArg(type_MDB_cursor, "cursor");
       f->addArg(type_uint, "flags");

    f = mod->addFunc(type_uint64, "mdb_cursor_count",
                     (void *)mdb_cursor_count_crk
                     );
       f->addArg(type_MDB_cursor, "cursor");

    f = mod->addFunc(type_int, "mdb_reader_check",
                     (void *)mdb_reader_check_crk
                     );
       f->addArg(type_MDB_env, "env");


    mod->addConstant(type_int, "MDB_VERSION_MAJOR",
                     static_cast<int>(MDB_VERSION_MAJOR)
                     );

    mod->addConstant(type_int, "MDB_VERSION_MINOR",
                     static_cast<int>(MDB_VERSION_MINOR)
                     );

    mod->addConstant(type_int, "MDB_VERSION_PATCH",
                     static_cast<int>(MDB_VERSION_PATCH)
                     );

    mod->addConstant(type_int, "MDB_FIXEDMAP",
                     static_cast<int>(MDB_FIXEDMAP)
                     );

    mod->addConstant(type_int, "MDB_NOSUBDIR",
                     static_cast<int>(MDB_NOSUBDIR)
                     );

    mod->addConstant(type_int, "MDB_NOSYNC",
                     static_cast<int>(MDB_NOSYNC)
                     );

    mod->addConstant(type_int, "MDB_RDONLY",
                     static_cast<int>(MDB_RDONLY)
                     );

    mod->addConstant(type_int, "MDB_NOMETASYNC",
                     static_cast<int>(MDB_NOMETASYNC)
                     );

    mod->addConstant(type_int, "MDB_WRITEMAP",
                     static_cast<int>(MDB_WRITEMAP)
                     );

    mod->addConstant(type_int, "MDB_MAPASYNC",
                     static_cast<int>(MDB_MAPASYNC)
                     );

    mod->addConstant(type_int, "MDB_NOTLS",
                     static_cast<int>(MDB_NOTLS)
                     );

    mod->addConstant(type_int, "MDB_REVERSEKEY",
                     static_cast<int>(MDB_REVERSEKEY)
                     );

    mod->addConstant(type_int, "MDB_DUPSORT",
                     static_cast<int>(MDB_DUPSORT)
                     );

    mod->addConstant(type_int, "MDB_INTEGERKEY",
                     static_cast<int>(MDB_INTEGERKEY)
                     );

    mod->addConstant(type_int, "MDB_DUPFIXED",
                     static_cast<int>(MDB_DUPFIXED)
                     );

    mod->addConstant(type_int, "MDB_INTEGERDUP",
                     static_cast<int>(MDB_INTEGERDUP)
                     );

    mod->addConstant(type_int, "MDB_REVERSEDUP",
                     static_cast<int>(MDB_REVERSEDUP)
                     );

    mod->addConstant(type_int, "MDB_CREATE",
                     static_cast<int>(MDB_CREATE)
                     );

    mod->addConstant(type_int, "MDB_NOOVERWRITE",
                     static_cast<int>(MDB_NOOVERWRITE)
                     );

    mod->addConstant(type_int, "MDB_NODUPDATA",
                     static_cast<int>(MDB_NODUPDATA)
                     );

    mod->addConstant(type_int, "MDB_CURRENT",
                     static_cast<int>(MDB_CURRENT)
                     );

    mod->addConstant(type_int, "MDB_RESERVE",
                     static_cast<int>(MDB_RESERVE)
                     );

    mod->addConstant(type_int, "MDB_APPEND",
                     static_cast<int>(MDB_APPEND)
                     );

    mod->addConstant(type_int, "MDB_APPENDDUP",
                     static_cast<int>(MDB_APPENDDUP)
                     );

    mod->addConstant(type_int, "MDB_MULTIPLE",
                     static_cast<int>(MDB_MULTIPLE)
                     );

    mod->addConstant(type_int, "MDB_SUCCESS",
                     static_cast<int>(MDB_SUCCESS)
                     );

    mod->addConstant(type_int, "MDB_KEYEXIST",
                     static_cast<int>(MDB_KEYEXIST)
                     );

    mod->addConstant(type_int, "MDB_NOTFOUND",
                     static_cast<int>(MDB_NOTFOUND)
                     );

    mod->addConstant(type_int, "MDB_PAGE_NOTFOUND",
                     static_cast<int>(MDB_PAGE_NOTFOUND)
                     );

    mod->addConstant(type_int, "MDB_CORRUPTED",
                     static_cast<int>(MDB_CORRUPTED)
                     );

    mod->addConstant(type_int, "MDB_PANIC",
                     static_cast<int>(MDB_PANIC)
                     );

    mod->addConstant(type_int, "MDB_VERSION_MISMATCH",
                     static_cast<int>(MDB_VERSION_MISMATCH)
                     );

    mod->addConstant(type_int, "MDB_INVALID",
                     static_cast<int>(MDB_INVALID)
                     );

    mod->addConstant(type_int, "MDB_MAP_FULL",
                     static_cast<int>(MDB_MAP_FULL)
                     );

    mod->addConstant(type_int, "MDB_DBS_FULL",
                     static_cast<int>(MDB_DBS_FULL)
                     );

    mod->addConstant(type_int, "MDB_READERS_FULL",
                     static_cast<int>(MDB_READERS_FULL)
                     );

    mod->addConstant(type_int, "MDB_TLS_FULL",
                     static_cast<int>(MDB_TLS_FULL)
                     );

    mod->addConstant(type_int, "MDB_TXN_FULL",
                     static_cast<int>(MDB_TXN_FULL)
                     );

    mod->addConstant(type_int, "MDB_CURSOR_FULL",
                     static_cast<int>(MDB_CURSOR_FULL)
                     );

    mod->addConstant(type_int, "MDB_PAGE_FULL",
                     static_cast<int>(MDB_PAGE_FULL)
                     );

    mod->addConstant(type_int, "MDB_MAP_RESIZED",
                     static_cast<int>(MDB_MAP_RESIZED)
                     );

    mod->addConstant(type_int, "MDB_INCOMPATIBLE",
                     static_cast<int>(MDB_INCOMPATIBLE)
                     );

    mod->addConstant(type_int, "MDB_BAD_RSLOT",
                     static_cast<int>(MDB_BAD_RSLOT)
                     );

    mod->addConstant(type_int, "MDB_BAD_TXN",
                     static_cast<int>(MDB_BAD_TXN)
                     );

    mod->addConstant(type_int, "MDB_LAST_ERRCODE",
                     static_cast<int>(MDB_LAST_ERRCODE)
                     );

    mod->addConstant(type_int, "MDB_BAD_VALSIZE",
                     static_cast<int>(MDB_BAD_VALSIZE)
                     );

    mod->addConstant(type_int, "MDB_FIRST",
                     static_cast<int>(MDB_FIRST)
                     );

    mod->addConstant(type_int, "MDB_FIRST_DUP",
                     static_cast<int>(MDB_FIRST_DUP)
                     );

    mod->addConstant(type_int, "MDB_GET_BOTH",
                     static_cast<int>(MDB_GET_BOTH)
                     );

    mod->addConstant(type_int, "MDB_GET_BOTH_RANGE",
                     static_cast<int>(MDB_GET_BOTH_RANGE)
                     );

    mod->addConstant(type_int, "MDB_GET_CURRENT",
                     static_cast<int>(MDB_GET_CURRENT)
                     );

    mod->addConstant(type_int, "MDB_GET_MULTIPLE",
                     static_cast<int>(MDB_GET_MULTIPLE)
                     );

    mod->addConstant(type_int, "MDB_LAST",
                     static_cast<int>(MDB_LAST)
                     );

    mod->addConstant(type_int, "MDB_LAST_DUP",
                     static_cast<int>(MDB_LAST_DUP)
                     );

    mod->addConstant(type_int, "MDB_NEXT",
                     static_cast<int>(MDB_NEXT)
                     );

    mod->addConstant(type_int, "MDB_NEXT_DUP",
                     static_cast<int>(MDB_NEXT_DUP)
                     );

    mod->addConstant(type_int, "MDB_NEXT_MULTIPLE",
                     static_cast<int>(MDB_NEXT_MULTIPLE)
                     );

    mod->addConstant(type_int, "MDB_NEXT_NODUP",
                     static_cast<int>(MDB_NEXT_NODUP)
                     );

    mod->addConstant(type_int, "MDB_PREV",
                     static_cast<int>(MDB_PREV)
                     );

    mod->addConstant(type_int, "MDB_PREV_DUP",
                     static_cast<int>(MDB_PREV_DUP)
                     );

    mod->addConstant(type_int, "MDB_PREV_NODUP",
                     static_cast<int>(MDB_PREV_NODUP)
                     );

    mod->addConstant(type_int, "MDB_SET",
                     static_cast<int>(MDB_SET)
                     );

    mod->addConstant(type_int, "MDB_SET_KEY",
                     static_cast<int>(MDB_SET_KEY)
                     );

    mod->addConstant(type_int, "MDB_SET_RANGE",
                     static_cast<int>(MDB_SET_RANGE)
                     );
}
