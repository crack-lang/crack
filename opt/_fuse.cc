#define FUSE_USE_VERSION 26
#include <fuse.h>
struct DoubleTimeSpec {
    timespec first, second;
};
typedef struct statvfs StatVFS;
struct Undef {};
void crk_fuse_operations_init(struct fuse_operations *ops) {
}
int crk_fuse_main(int argc, char *argv[],
                  const struct fuse_operations *op,
                  void *user_data
                  ) {
    return fuse_main(argc, argv, op, user_data);
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__fuse_rinit() {
    return;
}

extern "C"
void crack_ext__fuse_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
    mod->inject(std::string("import crack.runtime Stat;"));
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
    crack::ext::Type *type_Stat = mod->getType("Stat");

    crack::ext::Type *type_FuseFileInfo = mod->addType("FuseFileInfo", sizeof(fuse_file_info));
        type_FuseFileInfo->addInstVar(type_int, "flags",
                                CRACK_OFFSET(fuse_file_info, flags));
        type_FuseFileInfo->addInstVar(type_uint64, "fh_old",
                                CRACK_OFFSET(fuse_file_info, fh_old));
        type_FuseFileInfo->addInstVar(type_int, "writepage",
                                CRACK_OFFSET(fuse_file_info, writepage));
        type_FuseFileInfo->addInstVar(type_uint64, "fh",
                                CRACK_OFFSET(fuse_file_info, fh));
        type_FuseFileInfo->addInstVar(type_uint64, "lock_owner",
                                CRACK_OFFSET(fuse_file_info, lock_owner));
    type_FuseFileInfo->finish();


    crack::ext::Type *type_StatVFS = mod->addType("StatVFS", sizeof(StatVFS));
        type_StatVFS->addInstVar(type_uint64, "f_bsize",
                                CRACK_OFFSET(StatVFS, f_bsize));
        type_StatVFS->addInstVar(type_uint64, "f_frsize",
                                CRACK_OFFSET(StatVFS, f_frsize));
        type_StatVFS->addInstVar(type_uint, "f_blocks",
                                CRACK_OFFSET(StatVFS, f_blocks));
        type_StatVFS->addInstVar(type_uint, "f_bfree",
                                CRACK_OFFSET(StatVFS, f_bfree));
        type_StatVFS->addInstVar(type_uint, "f_bavail",
                                CRACK_OFFSET(StatVFS, f_bavail));
        type_StatVFS->addInstVar(type_uint, "f_files",
                                CRACK_OFFSET(StatVFS, f_files));
        type_StatVFS->addInstVar(type_uint, "f_ffree",
                                CRACK_OFFSET(StatVFS, f_ffree));
        type_StatVFS->addInstVar(type_uint, "f_favail",
                                CRACK_OFFSET(StatVFS, f_favail));
        type_StatVFS->addInstVar(type_uint64, "f_flag",
                                CRACK_OFFSET(StatVFS, f_flag));
        type_StatVFS->addInstVar(type_uint64, "f_namemax",
                                CRACK_OFFSET(StatVFS, f_namemax));
    type_StatVFS->finish();


    crack::ext::Type *type_FuseConnInfo = mod->addType("FuseConnInfo", sizeof(fuse_conn_info));
    type_FuseConnInfo->finish();


    crack::ext::Type *type_DoubleTimeSpec = mod->addType("DoubleTimeSpec", sizeof(DoubleTimeSpec));
    type_DoubleTimeSpec->finish();


    crack::ext::Type *type_FusePollHandle = mod->addType("FusePollHandle", sizeof(Undef));
    type_FusePollHandle->finish();


    crack::ext::Type *type_FuseBufVec = mod->addType("FuseBufVec", sizeof(fuse_bufvec));
    type_FuseBufVec->finish();


    crack::ext::Type *type_Flock = mod->addType("Flock", sizeof(flock));
        type_Flock->addInstVar(type_uint16, "l_type",
                                CRACK_OFFSET(flock, l_type));
        type_Flock->addInstVar(type_uint16, "l_whence",
                                CRACK_OFFSET(flock, l_whence));
        type_Flock->addInstVar(type_uintz, "l_start",
                                CRACK_OFFSET(flock, l_start));
        type_Flock->addInstVar(type_uintz, "l_len",
                                CRACK_OFFSET(flock, l_len));
        type_Flock->addInstVar(type_uintz, "l_pid",
                                CRACK_OFFSET(flock, l_pid));
    type_Flock->finish();


    crack::ext::Type *function = mod->getType("function");

    crack::ext::Type *function_pint_c_sbyteptr_c_sStat_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_Stat;
        function_pint_c_sbyteptr_c_sStat_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_c_sintz_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        params[3] = type_intz;
        function_pint_c_sbyteptr_c_sbyteptr_c_sintz_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sint_c_sint_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_int;
        params[3] = type_int;
        function_pint_c_sbyteptr_c_sint_c_sint_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sint_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_int;
        function_pint_c_sbyteptr_c_sint_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_int;
        params[1] = type_byteptr;
        function_pint_c_sbyteptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        function_pint_c_sbyteptr_c_sbyteptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_suintz_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_uintz;
        function_pint_c_sbyteptr_c_suintz_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_c_suintz_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(6);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        params[3] = type_uintz;
        params[4] = type_uintz;
        params[5] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sbyteptr_c_suintz_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sStatVFS_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_StatVFS;
        function_pint_c_sbyteptr_c_sStatVFS_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sint_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_int;
        params[3] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sint_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_c_sint_q;
    {
        std::vector<crack::ext::Type *> params(6);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        params[3] = type_byteptr;
        params[4] = type_uintz;
        params[5] = type_int;
        function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_c_sint_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_q;
    {
        std::vector<crack::ext::Type *> params(5);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        params[3] = type_byteptr;
        params[4] = type_uintz;
        function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sbyteptr_c_suintz_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_byteptr;
        params[3] = type_uintz;
        function_pint_c_sbyteptr_c_sbyteptr_c_suintz_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q;
    {
        std::vector<crack::ext::Type *> params(5);
        params[0] = type_int;
        params[1] = type_voidptr;
        params[2] = type_byteptr;
        params[3] = type_Stat;
        params[4] = type_uintz;
        function_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_svoidptr_c_sfunction_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(6);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_voidptr;
        params[3] = function_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q;
        params[4] = type_uintz;
        params[5] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_svoidptr_c_sfunction_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pvoidptr_c_sFuseConnInfo_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_voidptr;
        params[1] = type_FuseConnInfo;
        function_pvoidptr_c_sFuseConnInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pvoid_c_svoidptr_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_void;
        params[1] = type_voidptr;
        function_pvoid_c_svoidptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_uintz;
        params[3] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sStat_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_Stat;
        params[3] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sStat_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_c_sFlock_q;
    {
        std::vector<crack::ext::Type *> params(5);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_FuseFileInfo;
        params[3] = type_int;
        params[4] = type_Flock;
        function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_c_sFlock_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sDoubleTimeSpec_q;
    {
        std::vector<crack::ext::Type *> params(3);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_DoubleTimeSpec;
        function_pint_c_sbyteptr_c_sDoubleTimeSpec_q = function->getSpecialization(params);
    }

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_puint64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint64;
        array_puint64_q = array->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_suintz_c_sarray_puint64_q_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_uintz;
        params[3] = array_puint64_q;
        function_pint_c_sbyteptr_c_suintz_c_sarray_puint64_q_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sint_c_svoidptr_c_sFuseFileInfo_c_suint_c_svoidptr_q;
    {
        std::vector<crack::ext::Type *> params(7);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_int;
        params[3] = type_voidptr;
        params[4] = type_FuseFileInfo;
        params[5] = type_uint;
        params[6] = type_voidptr;
        function_pint_c_sbyteptr_c_sint_c_svoidptr_c_sFuseFileInfo_c_suint_c_svoidptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *array_puint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint;
        array_puint_q = array->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sFuseFileInfo_c_sFusePollHandle_c_sarray_puint_q_q;
    {
        std::vector<crack::ext::Type *> params(5);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_FuseFileInfo;
        params[3] = type_FusePollHandle;
        params[4] = array_puint_q;
        function_pint_c_sbyteptr_c_sFuseFileInfo_c_sFusePollHandle_c_sarray_puint_q_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sFuseBufVec_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(5);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_FuseBufVec;
        params[3] = type_uintz;
        params[4] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sFuseBufVec_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *array_pFuseBufVec_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_FuseBufVec;
        array_pFuseBufVec_q = array->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sarray_pFuseBufVec_q_c_suintz_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(6);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = array_pFuseBufVec_q;
        params[3] = type_uintz;
        params[4] = type_uintz;
        params[5] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sarray_pFuseBufVec_q_c_suintz_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_FuseFileInfo;
        params[3] = type_int;
        function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_q = function->getSpecialization(params);
    }

    crack::ext::Type *function_pint_c_sbyteptr_c_sint_c_suintz_c_suintz_c_sFuseFileInfo_q;
    {
        std::vector<crack::ext::Type *> params(6);
        params[0] = type_int;
        params[1] = type_byteptr;
        params[2] = type_int;
        params[3] = type_uintz;
        params[4] = type_uintz;
        params[5] = type_FuseFileInfo;
        function_pint_c_sbyteptr_c_sint_c_suintz_c_suintz_c_sFuseFileInfo_q = function->getSpecialization(params);
    }

    crack::ext::Type *type_FuseOperations = mod->addType("FuseOperations", sizeof(fuse_operations));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sStat_q, "getattr",
                                CRACK_OFFSET(fuse_operations, getattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_sintz_q, "readlink",
                                CRACK_OFFSET(fuse_operations, readlink));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_sint_q, "mknod",
                                CRACK_OFFSET(fuse_operations, mknod));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_q, "mkdir",
                                CRACK_OFFSET(fuse_operations, mkdir));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_q, "unlink",
                                CRACK_OFFSET(fuse_operations, unlink));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_q, "rmdir",
                                CRACK_OFFSET(fuse_operations, rmdir));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_q, "symlink",
                                CRACK_OFFSET(fuse_operations, symlink));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_q, "rename",
                                CRACK_OFFSET(fuse_operations, rename));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_q, "link",
                                CRACK_OFFSET(fuse_operations, link));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_q, "chmod",
                                CRACK_OFFSET(fuse_operations, chmod));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_sint_q, "chown",
                                CRACK_OFFSET(fuse_operations, chown));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_suintz_q, "truncate",
                                CRACK_OFFSET(fuse_operations, truncate));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_q, "open",
                                CRACK_OFFSET(fuse_operations, open));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_suintz_c_suintz_c_sFuseFileInfo_q, "read",
                                CRACK_OFFSET(fuse_operations, read));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_suintz_c_suintz_c_sFuseFileInfo_q, "write",
                                CRACK_OFFSET(fuse_operations, write));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sStatVFS_q, "statfs",
                                CRACK_OFFSET(fuse_operations, statfs));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_q, "flush",
                                CRACK_OFFSET(fuse_operations, flush));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_q, "release",
                                CRACK_OFFSET(fuse_operations, release));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_sFuseFileInfo_q, "fsync",
                                CRACK_OFFSET(fuse_operations, fsync));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_c_sint_q, "setxattr",
                                CRACK_OFFSET(fuse_operations, setxattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_sbyteptr_c_suintz_q, "getxattr",
                                CRACK_OFFSET(fuse_operations, getxattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_c_suintz_q, "listxattr",
                                CRACK_OFFSET(fuse_operations, listxattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sbyteptr_q, "removexattr",
                                CRACK_OFFSET(fuse_operations, removexattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_q, "opendir",
                                CRACK_OFFSET(fuse_operations, opendir));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_svoidptr_c_sfunction_pint_c_svoidptr_c_sbyteptr_c_sStat_c_suintz_q_c_suintz_c_sFuseFileInfo_q, "readdir",
                                CRACK_OFFSET(fuse_operations, readdir));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_q, "releasedir",
                                CRACK_OFFSET(fuse_operations, releasedir));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_sFuseFileInfo_q, "fsyncdir",
                                CRACK_OFFSET(fuse_operations, fsyncdir));
        type_FuseOperations->addInstVar(function_pvoidptr_c_sFuseConnInfo_q, "init",
                                CRACK_OFFSET(fuse_operations, init));
        type_FuseOperations->addInstVar(function_pvoid_c_svoidptr_q, "destroy",
                                CRACK_OFFSET(fuse_operations, destroy));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_q, "access",
                                CRACK_OFFSET(fuse_operations, access));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_sFuseFileInfo_q, "create",
                                CRACK_OFFSET(fuse_operations, create));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_suintz_c_sFuseFileInfo_q, "ftruncate",
                                CRACK_OFFSET(fuse_operations, ftruncate));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sStat_c_sFuseFileInfo_q, "fgetattr",
                                CRACK_OFFSET(fuse_operations, fgetattr));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_c_sFlock_q, "lock",
                                CRACK_OFFSET(fuse_operations, lock));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sDoubleTimeSpec_q, "utimens",
                                CRACK_OFFSET(fuse_operations, utimens));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_suintz_c_sarray_puint64_q_q, "bmap",
                                CRACK_OFFSET(fuse_operations, bmap));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_svoidptr_c_sFuseFileInfo_c_suint_c_svoidptr_q, "ioctl",
                                CRACK_OFFSET(fuse_operations, ioctl));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_c_sFusePollHandle_c_sarray_puint_q_q, "poll",
                                CRACK_OFFSET(fuse_operations, poll));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseBufVec_c_suintz_c_sFuseFileInfo_q, "write_buf",
                                CRACK_OFFSET(fuse_operations, write_buf));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sarray_pFuseBufVec_q_c_suintz_c_suintz_c_sFuseFileInfo_q, "read_buf",
                                CRACK_OFFSET(fuse_operations, read_buf));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sFuseFileInfo_c_sint_q, "flock",
                                CRACK_OFFSET(fuse_operations, flock));
        type_FuseOperations->addInstVar(function_pint_c_sbyteptr_c_sint_c_suintz_c_suintz_c_sFuseFileInfo_q, "fallocate",
                                CRACK_OFFSET(fuse_operations, fallocate));
        f = type_FuseOperations->addConstructor("oper init",
                            (void *)crk_fuse_operations_init
                        );

    type_FuseOperations->finish();


    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_FuseContext = mod->addType("FuseContext", sizeof(fuse_context));
        type_FuseContext->addInstVar(type_int, "uid",
                                CRACK_OFFSET(fuse_context, uid));
        type_FuseContext->addInstVar(type_int, "gid",
                                CRACK_OFFSET(fuse_context, gid));
        type_FuseContext->addInstVar(type_int, "pid",
                                CRACK_OFFSET(fuse_context, pid));
        type_FuseContext->addInstVar(type_voidptr, "private_data",
                                CRACK_OFFSET(fuse_context, private_data));
        type_FuseContext->addInstVar(type_int, "umask",
                                CRACK_OFFSET(fuse_context, umask));
    type_FuseContext->finish();

    f = mod->addFunc(type_int, "main",
                     (void *)crk_fuse_main
                     );
       f->addArg(type_int, "argc");
       f->addArg(array_pbyteptr_q, "argv");
       f->addArg(type_FuseOperations, "ops");
       f->addArg(type_voidptr, "userData");

    f = mod->addFunc(type_FuseContext, "get_context",
                     (void *)fuse_get_context
                     );

}
