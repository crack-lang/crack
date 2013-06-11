#include <stdlib.h>
#include <inttypes.h>
#include <float.h>
#include <netcdf.h>
typedef void * voidptr;
typedef char * byteptr;
typedef int Undef;
 static int crk_ncerr;
              static int crk_ncstatus;
            int nc_create_crk(byteptr path, int cmode) {
        int ncidp = 0;
        crk_ncstatus  = nc_create(path, cmode, &ncidp);
        if (!crk_ncstatus) return ncidp;
        else return -1;
    }
int nc_open_crk(const char *path, int cmode) {
        int ncidp = 0;
        crk_ncstatus  = nc_open(path, cmode, &ncidp);
        if (!crk_ncstatus) return ncidp;
        else return -1;
    }
byteptr nc_inq_path_crk(int ncid) {
        size_t path_len;
        byteptr path;
        crk_ncstatus = nc_inq_path(ncid, &path_len, NULL);
        path = (byteptr)malloc(path_len+1);
        if (!path) return NULL;
        crk_ncstatus = nc_inq_path(ncid, &path_len, path);
        path[path_len] =0;
        return path;
    }
int nc_def_dim_crk(int ncid, byteptr name, uint64_t len) {
        int idp = 0;
        crk_ncstatus  = nc_def_dim(ncid, name, len, &idp);
        if (!crk_ncstatus) return idp;
        else return -1;
    }
int nc_def_var_crk(int ncid, byteptr name, int xtype,
                     int ndims, int *dimidsp) {
        int varidp = 0;
        crk_ncstatus = nc_def_var(ncid, name, xtype, ndims, dimidsp, &varidp);
        if (!crk_ncstatus) return varidp;
        else return -1;
    }
int nc_get_status() {
        return crk_ncstatus;
    }int nc_inq_ndims_crk(int ncid) {
        int ndims;
        crk_ncstatus = nc_inq_ndims(ncid, &ndims);
        if (!crk_ncstatus) return ndims;
        else return -1;
    }
int *nc_inq_dimids_crk(int ncid, int ndims) {
        int nndims;
        int *dimids = (int *)malloc(ndims*sizeof(int));
        crk_ncstatus = nc_inq_dimids(ncid, &nndims, dimids, 0);
        if (crk_ncstatus != 0) {
            free(dimids);
            return NULL;
        }
        else return dimids;
    }
byteptr nc_inq_dimname_crk(int ncid, int dimid) {
        byteptr name = (byteptr)malloc(NC_MAX_NAME);
        crk_ncstatus = nc_inq_dimname(ncid, dimid, name);
        if (!crk_ncstatus) return name;
        free(name);
        return NULL;
    }
uint64_t nc_inq_dimlen_crk(int ncid, int dimid) {
        size_t len = 0;
        crk_ncstatus = nc_inq_dimlen(ncid, dimid, &len);
        if (!crk_ncstatus) return (int64_t)len;
        return 0;
    }
int nc_inq_nvars_crk(int ncid) {
        int nvars;
        crk_ncstatus = nc_inq_nvars(ncid, &nvars);
        if (!crk_ncstatus) return nvars;
        else return -1;
    }
int *nc_inq_varids_crk(int ncid, int nvars) {
        int nnvars;
        int *varids = (int *)malloc(nvars * sizeof(int));
        crk_ncstatus = nc_inq_varids(ncid, &nnvars, varids);
        if (!crk_ncstatus) return varids;
        else return varids;
    }
int nc_inq_varid_crk(int ncid, byteptr name) {
        int varid;
        crk_ncstatus = nc_inq_varid(ncid, name, &varid);
        if (!crk_ncstatus) return varid;
        else return -1;
    }
byteptr nc_inq_varname_crk(int ncid, int varid) {
        byteptr name = (byteptr)malloc(NC_MAX_NAME);
        crk_ncstatus = nc_inq_varname(ncid, varid, name);
        if (!crk_ncstatus) return name;
        free(name);
        return NULL;
    }
int nc_inq_varndims_crk(int ncid, int varid) {
        int ndims;
        crk_ncstatus = nc_inq_varndims(ncid, varid, &ndims);
        if (!crk_ncstatus) return ndims;
        else return -1;
    }
int *nc_inq_vardimid_crk(int ncid, int varid) {
        int ndims;
        int *dimids;
        crk_ncstatus = nc_inq_varndims(ncid, varid, &ndims);
        if (crk_ncstatus !=0) return NULL;
        dimids = (int *)malloc(ndims * sizeof(int));
        crk_ncstatus = nc_inq_vardimid(ncid, varid, dimids);
        if (crk_ncstatus !=0) return NULL;
        return dimids;
    }
int nc_inq_vartype_crk(int ncid, int varid) {
        int tpe = 0;
        crk_ncstatus = nc_inq_vartype(ncid, varid, &tpe);
        if (!crk_ncstatus) return tpe;
        return 0;
    }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__netcdf_rinit() {
    return;
}

extern "C"
void crack_ext__netcdf_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pfloat64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float64;
        array_pfloat64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint64;
        array_puint64_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_byteptr, "nc_inq_libvers",
                     (void *)nc_inq_libvers
                     );

    f = mod->addFunc(type_int, "nc_create",
                     (void *)nc_create_crk
                     );
       f->addArg(type_byteptr, "path");
       f->addArg(type_int, "cmode");

    f = mod->addFunc(type_int, "nc_open",
                     (void *)nc_open_crk
                     );
       f->addArg(type_byteptr, "path");
       f->addArg(type_int, "cmode");

    f = mod->addFunc(type_int, "nc_var_par_access",
                     (void *)nc_var_par_access
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(type_int, "par_access");

    f = mod->addFunc(type_byteptr, "nc_inq_path",
                     (void *)nc_inq_path_crk
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_redef",
                     (void *)nc_redef
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_enddef",
                     (void *)nc_enddef
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_sync",
                     (void *)nc_sync
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_abort",
                     (void *)nc_abort
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_close",
                     (void *)nc_close
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(type_int, "nc_def_dim",
                     (void *)nc_def_dim_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_byteptr, "name");
       f->addArg(type_uint64, "len");

    f = mod->addFunc(type_int, "nc_def_var",
                     (void *)nc_def_var_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int, "xtype");
       f->addArg(type_int, "ndims");
       f->addArg(array_pint_q, "dimidsp");

    f = mod->addFunc(type_int, "nc_put_var_double",
                     (void *)nc_put_var_double
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_pfloat64_q, "op");

    f = mod->addFunc(type_int, "nc_put_var_int",
                     (void *)nc_put_var_int
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_pint_q, "op");

    f = mod->addFunc(type_int, "nc_get_var_double",
                     (void *)nc_get_var_double
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_pfloat64_q, "op");

    f = mod->addFunc(type_int, "nc_get_var_int",
                     (void *)nc_get_var_int
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_pint_q, "op");

    f = mod->addFunc(type_int, "nc_put_var",
                     (void *)nc_put_var
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(type_voidptr, "op");

    f = mod->addFunc(type_int, "nc_get_var",
                     (void *)nc_get_var
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(type_voidptr, "op");

    f = mod->addFunc(type_int, "nc_put_vara_double",
                     (void *)nc_put_vara_double
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_puint64_q, "startp");
       f->addArg(array_puint64_q, "countp");
       f->addArg(array_pfloat64_q, "op");

    f = mod->addFunc(type_int, "nc_get_vara_double",
                     (void *)nc_get_vara_double
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_puint64_q, "startp");
       f->addArg(array_puint64_q, "countp");
       f->addArg(array_pfloat64_q, "op");

    f = mod->addFunc(type_int, "nc_put_vara_int",
                     (void *)nc_put_vara_int
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_puint64_q, "startp");
       f->addArg(array_puint64_q, "countp");
       f->addArg(array_pint_q, "op");

    f = mod->addFunc(type_int, "nc_get_vara_int",
                     (void *)nc_get_vara_int
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");
       f->addArg(array_puint64_q, "startp");
       f->addArg(array_puint64_q, "countp");
       f->addArg(array_pint_q, "op");

    f = mod->addFunc(type_byteptr, "nc_strerror",
                     (void *)nc_strerror
                     );
       f->addArg(type_int, "status");

    f = mod->addFunc(type_int, "nc_get_status",
                     (void *)nc_get_status
                     );

    f = mod->addFunc(type_int, "nc_inq_ndims",
                     (void *)nc_inq_ndims_crk
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(array_pint_q, "nc_inq_dimids",
                     (void *)nc_inq_dimids_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "ndims");

    f = mod->addFunc(type_byteptr, "nc_inq_dimname",
                     (void *)nc_inq_dimname_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "dimid");

    f = mod->addFunc(type_uint64, "nc_inq_dimlen",
                     (void *)nc_inq_dimlen_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "dimid");

    f = mod->addFunc(type_int, "nc_rename_dim",
                     (void *)nc_rename_dim
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "dimid");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_int, "nc_inq_nvars",
                     (void *)nc_inq_nvars_crk
                     );
       f->addArg(type_int, "ncid");

    f = mod->addFunc(array_pint_q, "nc_inq_varids",
                     (void *)nc_inq_varids_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "nvars");

    f = mod->addFunc(type_int, "nc_inq_varid",
                     (void *)nc_inq_varid_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_byteptr, "nc_inq_varname",
                     (void *)nc_inq_varname_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");

    f = mod->addFunc(type_int, "nc_inq_varndims",
                     (void *)nc_inq_varndims_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");

    f = mod->addFunc(array_pint_q, "nc_inq_vardimid",
                     (void *)nc_inq_vardimid_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");

    f = mod->addFunc(type_int, "nc_inq_vartype",
                     (void *)nc_inq_vartype_crk
                     );
       f->addArg(type_int, "ncid");
       f->addArg(type_int, "varid");


    mod->addConstant(type_int, "NC_NAT",
                     static_cast<int>(NC_NAT)
                     );

    mod->addConstant(type_int, "NC_BYTE",
                     static_cast<int>(NC_BYTE)
                     );

    mod->addConstant(type_int, "NC_CHAR",
                     static_cast<int>(NC_CHAR)
                     );

    mod->addConstant(type_int, "NC_SHORT",
                     static_cast<int>(NC_SHORT)
                     );

    mod->addConstant(type_int, "NC_INT",
                     static_cast<int>(NC_INT)
                     );

    mod->addConstant(type_int, "NC_LONG",
                     static_cast<int>(NC_LONG)
                     );

    mod->addConstant(type_int, "NC_FLOAT",
                     static_cast<int>(NC_FLOAT)
                     );

    mod->addConstant(type_int, "NC_DOUBLE",
                     static_cast<int>(NC_DOUBLE)
                     );

    mod->addConstant(type_int, "NC_UBYTE",
                     static_cast<int>(NC_UBYTE)
                     );

    mod->addConstant(type_int, "NC_USHORT",
                     static_cast<int>(NC_USHORT)
                     );

    mod->addConstant(type_int, "NC_UINT",
                     static_cast<int>(NC_UINT)
                     );

    mod->addConstant(type_int, "NC_INT64",
                     static_cast<int>(NC_INT64)
                     );

    mod->addConstant(type_int, "NC_UINT64",
                     static_cast<int>(NC_UINT64)
                     );

    mod->addConstant(type_int, "NC_STRING",
                     static_cast<int>(NC_STRING)
                     );

    mod->addConstant(type_int, "NC_MAX_ATOMIC_TYPE",
                     static_cast<int>(NC_MAX_ATOMIC_TYPE)
                     );

    mod->addConstant(type_int, "NC_VLEN",
                     static_cast<int>(NC_VLEN)
                     );

    mod->addConstant(type_int, "NC_OPAQUE",
                     static_cast<int>(NC_OPAQUE)
                     );

    mod->addConstant(type_int, "NC_ENUM",
                     static_cast<int>(NC_ENUM)
                     );

    mod->addConstant(type_int, "NC_COMPOUND",
                     static_cast<int>(NC_COMPOUND)
                     );

    mod->addConstant(type_int, "NC_FIRSTUSERTYPEID",
                     static_cast<int>(NC_FIRSTUSERTYPEID)
                     );

    mod->addConstant(type_int, "NC_FILL",
                     static_cast<int>(NC_FILL)
                     );

    mod->addConstant(type_int, "NC_NOFILL",
                     static_cast<int>(NC_NOFILL)
                     );

    mod->addConstant(type_int, "NC_NOWRITE",
                     static_cast<int>(NC_NOWRITE)
                     );

    mod->addConstant(type_int, "NC_WRITE",
                     static_cast<int>(NC_WRITE)
                     );

    mod->addConstant(type_int, "NC_CLOBBER",
                     static_cast<int>(NC_CLOBBER)
                     );

    mod->addConstant(type_int, "NC_NOCLOBBER",
                     static_cast<int>(NC_NOCLOBBER)
                     );

    mod->addConstant(type_int, "NC_64BIT_OFFSET",
                     static_cast<int>(NC_64BIT_OFFSET)
                     );

    mod->addConstant(type_int, "NC_NETCDF4",
                     static_cast<int>(NC_NETCDF4)
                     );

    mod->addConstant(type_int, "NC_CLASSIC_MODEL",
                     static_cast<int>(NC_CLASSIC_MODEL)
                     );

    mod->addConstant(type_int, "NC_SHARE",
                     static_cast<int>(NC_SHARE)
                     );

    mod->addConstant(type_int, "NC_MPIIO",
                     static_cast<int>(NC_MPIIO)
                     );

    mod->addConstant(type_int, "NC_MPIPOSIX",
                     static_cast<int>(NC_MPIPOSIX)
                     );

    mod->addConstant(type_int, "NC_PNETCDF",
                     static_cast<int>(NC_PNETCDF)
                     );

    mod->addConstant(type_int, "NC_LOCK",
                     static_cast<int>(NC_LOCK)
                     );

    mod->addConstant(type_int, "NC_FORMAT_CLASSIC",
                     static_cast<int>(NC_FORMAT_CLASSIC)
                     );

    mod->addConstant(type_int, "NC_FORMAT_64BIT",
                     static_cast<int>(NC_FORMAT_64BIT)
                     );

    mod->addConstant(type_int, "NC_FORMAT_NETCDF4",
                     static_cast<int>(NC_FORMAT_NETCDF4)
                     );

    mod->addConstant(type_int, "NC_FORMAT_NETCDF4_CLASSIC",
                     static_cast<int>(NC_FORMAT_NETCDF4_CLASSIC)
                     );

    mod->addConstant(type_int, "NC_SIZEHINT_DEFAULT",
                     static_cast<int>(NC_SIZEHINT_DEFAULT)
                     );

    mod->addConstant(type_int, "NC_UNLIMITED",
                     static_cast<int>(NC_UNLIMITED)
                     );

    mod->addConstant(type_int, "NC_GLOBAL",
                     static_cast<int>(NC_GLOBAL)
                     );

    mod->addConstant(type_int, "NC_MAX_DIMS",
                     static_cast<int>(NC_MAX_DIMS)
                     );

    mod->addConstant(type_int, "NC_MAX_ATTRS",
                     static_cast<int>(NC_MAX_ATTRS)
                     );

    mod->addConstant(type_int, "NC_MAX_VARS",
                     static_cast<int>(NC_MAX_VARS)
                     );

    mod->addConstant(type_int, "NC_MAX_NAME",
                     static_cast<int>(NC_MAX_NAME)
                     );

    mod->addConstant(type_int, "NC_MAX_VAR_DIMS",
                     static_cast<int>(NC_MAX_VAR_DIMS)
                     );

    mod->addConstant(type_int, "NC_ENDIAN_NATIVE",
                     static_cast<int>(NC_ENDIAN_NATIVE)
                     );

    mod->addConstant(type_int, "NC_ENDIAN_LITTLE",
                     static_cast<int>(NC_ENDIAN_LITTLE)
                     );

    mod->addConstant(type_int, "NC_ENDIAN_BIG",
                     static_cast<int>(NC_ENDIAN_BIG)
                     );

    mod->addConstant(type_int, "NC_CHUNKED",
                     static_cast<int>(NC_CHUNKED)
                     );

    mod->addConstant(type_int, "NC_CONTIGUOUS",
                     static_cast<int>(NC_CONTIGUOUS)
                     );

    mod->addConstant(type_int, "NC_NOCHECKSUM",
                     static_cast<int>(NC_NOCHECKSUM)
                     );

    mod->addConstant(type_int, "NC_FLETCHER32",
                     static_cast<int>(NC_FLETCHER32)
                     );

    mod->addConstant(type_int, "NC_NOSHUFFLE",
                     static_cast<int>(NC_NOSHUFFLE)
                     );

    mod->addConstant(type_int, "NC_SHUFFLE",
                     static_cast<int>(NC_SHUFFLE)
                     );

    mod->addConstant(type_int, "NC_NOERR",
                     static_cast<int>(NC_NOERR)
                     );

    mod->addConstant(type_int, "NC2_ERR",
                     static_cast<int>(NC2_ERR)
                     );

    mod->addConstant(type_int, "NC_EBADID",
                     static_cast<int>(NC_EBADID)
                     );

    mod->addConstant(type_int, "NC_ENFILE",
                     static_cast<int>(NC_ENFILE)
                     );

    mod->addConstant(type_int, "NC_EEXIST",
                     static_cast<int>(NC_EEXIST)
                     );

    mod->addConstant(type_int, "NC_EINVAL",
                     static_cast<int>(NC_EINVAL)
                     );

    mod->addConstant(type_int, "NC_EPERM",
                     static_cast<int>(NC_EPERM)
                     );

    mod->addConstant(type_int, "NC_ENOTINDEFINE",
                     static_cast<int>(NC_ENOTINDEFINE)
                     );

    mod->addConstant(type_int, "NC_EINDEFINE",
                     static_cast<int>(NC_EINDEFINE)
                     );

    mod->addConstant(type_int, "NC_EINVALCOORDS",
                     static_cast<int>(NC_EINVALCOORDS)
                     );

    mod->addConstant(type_int, "NC_EMAXDIMS",
                     static_cast<int>(NC_EMAXDIMS)
                     );

    mod->addConstant(type_int, "NC_ENAMEINUSE",
                     static_cast<int>(NC_ENAMEINUSE)
                     );

    mod->addConstant(type_int, "NC_ENOTATT",
                     static_cast<int>(NC_ENOTATT)
                     );

    mod->addConstant(type_int, "NC_EMAXATTS",
                     static_cast<int>(NC_EMAXATTS)
                     );

    mod->addConstant(type_int, "NC_EBADTYPE",
                     static_cast<int>(NC_EBADTYPE)
                     );

    mod->addConstant(type_int, "NC_EBADDIM",
                     static_cast<int>(NC_EBADDIM)
                     );

    mod->addConstant(type_int, "NC_EUNLIMPOS",
                     static_cast<int>(NC_EUNLIMPOS)
                     );

    mod->addConstant(type_int, "NC_EMAXVARS",
                     static_cast<int>(NC_EMAXVARS)
                     );

    mod->addConstant(type_int, "NC_ENOTVAR",
                     static_cast<int>(NC_ENOTVAR)
                     );

    mod->addConstant(type_int, "NC_EGLOBAL",
                     static_cast<int>(NC_EGLOBAL)
                     );

    mod->addConstant(type_int, "NC_ENOTNC",
                     static_cast<int>(NC_ENOTNC)
                     );

    mod->addConstant(type_int, "NC_ESTS",
                     static_cast<int>(NC_ESTS)
                     );

    mod->addConstant(type_int, "NC_EMAXNAME",
                     static_cast<int>(NC_EMAXNAME)
                     );

    mod->addConstant(type_int, "NC_EUNLIMIT",
                     static_cast<int>(NC_EUNLIMIT)
                     );

    mod->addConstant(type_int, "NC_ENORECVARS",
                     static_cast<int>(NC_ENORECVARS)
                     );

    mod->addConstant(type_int, "NC_ECHAR",
                     static_cast<int>(NC_ECHAR)
                     );

    mod->addConstant(type_int, "NC_EEDGE",
                     static_cast<int>(NC_EEDGE)
                     );

    mod->addConstant(type_int, "NC_ESTRIDE",
                     static_cast<int>(NC_ESTRIDE)
                     );

    mod->addConstant(type_int, "NC_EBADNAME",
                     static_cast<int>(NC_EBADNAME)
                     );

    mod->addConstant(type_int, "NC_ERANGE",
                     static_cast<int>(NC_ERANGE)
                     );

    mod->addConstant(type_int, "NC_ENOMEM",
                     static_cast<int>(NC_ENOMEM)
                     );

    mod->addConstant(type_int, "NC_EVARSIZE",
                     static_cast<int>(NC_EVARSIZE)
                     );

    mod->addConstant(type_int, "NC_EDIMSIZE",
                     static_cast<int>(NC_EDIMSIZE)
                     );

    mod->addConstant(type_int, "NC_ETRUNC",
                     static_cast<int>(NC_ETRUNC)
                     );

    mod->addConstant(type_int, "NC_EAXISTYPE",
                     static_cast<int>(NC_EAXISTYPE)
                     );

    mod->addConstant(type_int, "NC_EDAP",
                     static_cast<int>(NC_EDAP)
                     );

    mod->addConstant(type_int, "NC_ECURL",
                     static_cast<int>(NC_ECURL)
                     );

    mod->addConstant(type_int, "NC_EIO",
                     static_cast<int>(NC_EIO)
                     );

    mod->addConstant(type_int, "NC_ENODATA",
                     static_cast<int>(NC_ENODATA)
                     );

    mod->addConstant(type_int, "NC_EDAPSVC",
                     static_cast<int>(NC_EDAPSVC)
                     );

    mod->addConstant(type_int, "NC_EDAS",
                     static_cast<int>(NC_EDAS)
                     );

    mod->addConstant(type_int, "NC_EDDS",
                     static_cast<int>(NC_EDDS)
                     );

    mod->addConstant(type_int, "NC_EDATADDS",
                     static_cast<int>(NC_EDATADDS)
                     );

    mod->addConstant(type_int, "NC_EDAPURL",
                     static_cast<int>(NC_EDAPURL)
                     );

    mod->addConstant(type_int, "NC_EDAPCONSTRAINT",
                     static_cast<int>(NC_EDAPCONSTRAINT)
                     );

    mod->addConstant(type_int, "NC_ETRANSLATION",
                     static_cast<int>(NC_ETRANSLATION)
                     );

    mod->addConstant(type_int, "NC4_FIRST_ERROR",
                     static_cast<int>(NC4_FIRST_ERROR)
                     );

    mod->addConstant(type_int, "NC_EHDFERR",
                     static_cast<int>(NC_EHDFERR)
                     );

    mod->addConstant(type_int, "NC_ECANTREAD",
                     static_cast<int>(NC_ECANTREAD)
                     );

    mod->addConstant(type_int, "NC_ECANTWRITE",
                     static_cast<int>(NC_ECANTWRITE)
                     );

    mod->addConstant(type_int, "NC_ECANTCREATE",
                     static_cast<int>(NC_ECANTCREATE)
                     );

    mod->addConstant(type_int, "NC_EFILEMETA",
                     static_cast<int>(NC_EFILEMETA)
                     );

    mod->addConstant(type_int, "NC_EDIMMETA",
                     static_cast<int>(NC_EDIMMETA)
                     );

    mod->addConstant(type_int, "NC_EATTMETA",
                     static_cast<int>(NC_EATTMETA)
                     );

    mod->addConstant(type_int, "NC_EVARMETA",
                     static_cast<int>(NC_EVARMETA)
                     );

    mod->addConstant(type_int, "NC_ENOCOMPOUND",
                     static_cast<int>(NC_ENOCOMPOUND)
                     );

    mod->addConstant(type_int, "NC_EATTEXISTS",
                     static_cast<int>(NC_EATTEXISTS)
                     );

    mod->addConstant(type_int, "NC_ENOTNC4",
                     static_cast<int>(NC_ENOTNC4)
                     );

    mod->addConstant(type_int, "NC_ESTRICTNC3",
                     static_cast<int>(NC_ESTRICTNC3)
                     );

    mod->addConstant(type_int, "NC_ENOTNC3",
                     static_cast<int>(NC_ENOTNC3)
                     );

    mod->addConstant(type_int, "NC_ENOPAR",
                     static_cast<int>(NC_ENOPAR)
                     );

    mod->addConstant(type_int, "NC_EPARINIT",
                     static_cast<int>(NC_EPARINIT)
                     );

    mod->addConstant(type_int, "NC_EBADGRPID",
                     static_cast<int>(NC_EBADGRPID)
                     );

    mod->addConstant(type_int, "NC_EBADTYPID",
                     static_cast<int>(NC_EBADTYPID)
                     );

    mod->addConstant(type_int, "NC_ETYPDEFINED",
                     static_cast<int>(NC_ETYPDEFINED)
                     );

    mod->addConstant(type_int, "NC_EBADFIELD",
                     static_cast<int>(NC_EBADFIELD)
                     );

    mod->addConstant(type_int, "NC_EBADCLASS",
                     static_cast<int>(NC_EBADCLASS)
                     );

    mod->addConstant(type_int, "NC_EMAPTYPE",
                     static_cast<int>(NC_EMAPTYPE)
                     );

    mod->addConstant(type_int, "NC_ELATEFILL",
                     static_cast<int>(NC_ELATEFILL)
                     );

    mod->addConstant(type_int, "NC_ELATEDEF",
                     static_cast<int>(NC_ELATEDEF)
                     );

    mod->addConstant(type_int, "NC_EDIMSCALE",
                     static_cast<int>(NC_EDIMSCALE)
                     );

    mod->addConstant(type_int, "NC_ENOGRP",
                     static_cast<int>(NC_ENOGRP)
                     );

    mod->addConstant(type_int, "NC_ESTORAGE",
                     static_cast<int>(NC_ESTORAGE)
                     );

    mod->addConstant(type_int, "NC_EBADCHUNK",
                     static_cast<int>(NC_EBADCHUNK)
                     );

    mod->addConstant(type_int, "NC_ENOTBUILT",
                     static_cast<int>(NC_ENOTBUILT)
                     );

    mod->addConstant(type_int, "NC4_LAST_ERROR",
                     static_cast<int>(NC4_LAST_ERROR)
                     );
}
