// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 6/22/2012
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef __APPLE__
#include <rpc/types.h>
#include <stdint.h>
#endif
#include <rpc/xdr.h>

#define INT_SIZE sizeof(int)

    XDR *crk_xdrmem_create(char *buffer, unsigned int numbytes, int op){
               XDR *xdrs = new XDR;
               xdrmem_create(xdrs, buffer, numbytes, (enum xdr_op)op);
               return xdrs;
            }

    void crk_xdr_destroy(XDR * xdrs){
        xdr_destroy(xdrs);
        delete xdrs;
    }

    bool xdr_error;
    unsigned int xdr_size;

    bool crk_xdr_error(){
        return xdr_error;
    }

    unsigned int crk_xdr_size(){
        return xdr_size;
    }

    // Returns the end of the buffer
    unsigned int crk_xdr_getpos(XDR *xdrs){
        return xdr_getpos(xdrs);
    } 

    // Sets the position to be read from next in the buffer/stream
    void crk_xdr_setpos(XDR *xdrs, unsigned int pos){
        xdr_setpos(xdrs, pos);
    } 


#define scalar_op(tpe, func_tpe, crk_tpe) \
    bool crk_xdr_encode_##crk_tpe(XDR *xdrs, tpe value){ \
        return (bool)xdr_##func_tpe(xdrs, &value); \
    } \
\
    tpe crk_xdr_decode_##crk_tpe(XDR *xdrs){ \
        tpe value; \
        xdr_error = xdr_##func_tpe(xdrs, &value); \
        return value; \
    }

    scalar_op(int, int, int)
    scalar_op(unsigned int, u_int, uint)
    scalar_op(int32_t, int32_t, int32)
#ifndef __APPLE__
    scalar_op(uint32_t, uint32_t, uint32)
#endif
    scalar_op(int64_t, int64_t, int64)
#ifndef __APPLE__
    scalar_op(uint64_t, uint64_t, uint64)
#endif
    scalar_op(float, float, float32)
    scalar_op(double, double, float64)

    // Encode a bool
    bool crk_xdr_encode_bool(XDR *xdrs, bool value){
        bool_t val = value;
        return xdr_bool(xdrs, &val);
    }

    // Decode a bool
    bool crk_xdr_decode_bool(XDR *xdrs){
        bool_t value;
        xdr_error = xdr_bool(xdrs, &value);
        return (bool)value;
    }

    // Encode a string
    bool crk_xdr_encode_bytes(XDR *xdrs, char *buf, unsigned int numbytes){
        return (bool)xdr_bytes(xdrs, &buf, &numbytes, numbytes);
    }

    // Decode a string
    unsigned int crk_xdr_decode_bytes(XDR *xdrs, char *buf, unsigned int numbytes){
        xdr_error = xdr_bytes(xdrs, &buf, &numbytes, numbytes);
        return numbytes;
    }

#define array_op(tpe, func_tpe, crk_tpe) \
    bool crk_xdr_encode_array_##crk_tpe(XDR *xdrs, tpe *value, \
            unsigned int count, unsigned int max) { \
        return (bool) xdr_array(xdrs,  (char **)&value, &count, max, \
                                sizeof(tpe), (xdrproc_t)xdr_##func_tpe); \
    } \
\
    unsigned int crk_xdr_decode_array_##crk_tpe(XDR *xdrs, tpe *value, \
                                                  unsigned int max) { \
        unsigned int count; \
        xdr_error = xdr_array(xdrs, (char **)&value, &count, max, \
                              sizeof(tpe), (xdrproc_t)xdr_##func_tpe); \
        return count; \
    }

    array_op(int, int, int)
    array_op(unsigned int, u_int, uint)
    array_op(int32_t, int32_t, int32)
#ifndef __APPLE__
    array_op(uint32_t, uint32_t, uint32)
#endif
    array_op(int64_t, int64_t, int64)
#ifndef __APPLE__
    array_op(uint64_t, uint64_t, uint64)
#endif
    array_op(float, float, float32)
    array_op(double, double, float64)
    array_op(bool_t, bool, bool)

    

#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_runtime_xdr_rinit() {
    return;
}

extern "C"
void crack_runtime_xdr_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_xdr = mod->addType("xdr", sizeof(XDR));
    type_xdr->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint;
        array_puint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint32_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int32;
        array_pint32_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint32_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint32;
        array_puint32_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int64;
        array_pint64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint64;
        array_puint64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pfloat32_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float32;
        array_pfloat32_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pfloat64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float64;
        array_pfloat64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pbool_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_bool;
        array_pbool_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_xdr, "xdrmem_create",
                     (void *)crk_xdrmem_create
                     );
       f->addArg(type_byteptr, "buffer");
       f->addArg(type_uint, "numbytes");
       f->addArg(type_int, "op");

    f = mod->addFunc(type_void, "xdr_destroy",
                     (void *)crk_xdr_destroy
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_uint, "xdr_getpos",
                     (void *)crk_xdr_getpos
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_uint, "xdr_setpos",
                     (void *)crk_xdr_setpos
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_uint, "pos");

    f = mod->addFunc(type_bool, "xdr_encode_int",
                     (void *)crk_xdr_encode_int
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_int, "value");

    f = mod->addFunc(type_int, "xdr_decode_int",
                     (void *)crk_xdr_decode_int
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_bool, "xdr_encode_uint",
                     (void *)crk_xdr_encode_uint
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_uint, "value");

    f = mod->addFunc(type_uint, "xdr_decode_uint",
                     (void *)crk_xdr_decode_uint
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_bool, "xdr_encode_int32",
                     (void *)crk_xdr_encode_int32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_int32, "value");

    f = mod->addFunc(type_int32, "xdr_decode_int32",
                     (void *)crk_xdr_decode_int32
                     );
       f->addArg(type_xdr, "xdrs");

#ifndef __APPLE__
    f = mod->addFunc(type_bool, "xdr_encode_uint32",
                     (void *)crk_xdr_encode_uint32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_uint32, "value");

    f = mod->addFunc(type_uint32, "xdr_decode_uint32",
                     (void *)crk_xdr_decode_uint32
                     );
       f->addArg(type_xdr, "xdrs");
#endif

    f = mod->addFunc(type_bool, "xdr_encode_int64",
                     (void *)crk_xdr_encode_int64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_int64, "value");

    f = mod->addFunc(type_int64, "xdr_decode_int64",
                     (void *)crk_xdr_decode_int64
                     );
       f->addArg(type_xdr, "xdrs");

#ifndef __APPLE__
    f = mod->addFunc(type_bool, "xdr_encode_uint64",
                     (void *)crk_xdr_encode_uint64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_uint64, "value");

    f = mod->addFunc(type_uint64, "xdr_decode_uint64",
                     (void *)crk_xdr_decode_uint64
                     );
       f->addArg(type_xdr, "xdrs");
#endif

    f = mod->addFunc(type_bool, "xdr_encode_float32",
                     (void *)crk_xdr_encode_float32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_float32, "value");

    f = mod->addFunc(type_float32, "xdr_decode_float32",
                     (void *)crk_xdr_decode_float32
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_bool, "xdr_encode_float64",
                     (void *)crk_xdr_encode_float64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_float64, "value");

    f = mod->addFunc(type_float64, "xdr_decode_float64",
                     (void *)crk_xdr_decode_float64
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_bool, "xdr_encode_bool",
                     (void *)crk_xdr_encode_bool
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_bool, "value");

    f = mod->addFunc(type_bool, "xdr_decode_bool",
                     (void *)crk_xdr_decode_bool
                     );
       f->addArg(type_xdr, "xdrs");

    f = mod->addFunc(type_bool, "xdr_encode_bytes",
                     (void *)crk_xdr_encode_bytes
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_byteptr, "buf");
       f->addArg(type_uint, "numbytes");

    f = mod->addFunc(type_uint, "xdr_decode_bytes",
                     (void *)crk_xdr_decode_bytes
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(type_byteptr, "buf");
       f->addArg(type_uint, "numbytes");

    f = mod->addFunc(type_bool, "xdr_encode_array_int",
                     (void *)crk_xdr_encode_array_int
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_int",
                     (void *)crk_xdr_decode_array_int
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint_q, "buf");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_bool, "xdr_encode_array_uint",
                     (void *)crk_xdr_encode_array_uint
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_uint",
                     (void *)crk_xdr_decode_array_uint
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint_q, "buf");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_bool, "xdr_encode_array_int32",
                     (void *)crk_xdr_encode_array_int32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint32_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_int32",
                     (void *)crk_xdr_decode_array_int32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint32_q, "buf");
       f->addArg(type_uint, "max");

#ifndef __APPLE__
    f = mod->addFunc(type_bool, "xdr_encode_array_uint32",
                     (void *)crk_xdr_encode_array_uint32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint32_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_uint32",
                     (void *)crk_xdr_decode_array_uint32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint32_q, "buf");
       f->addArg(type_uint, "max");
#endif

    f = mod->addFunc(type_bool, "xdr_encode_array_int64",
                     (void *)crk_xdr_encode_array_int64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint64_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_int64",
                     (void *)crk_xdr_decode_array_int64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pint64_q, "buf");
       f->addArg(type_uint, "max");

#ifndef __APPLE__
    f = mod->addFunc(type_bool, "xdr_encode_array_uint64",
                     (void *)crk_xdr_encode_array_uint64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint64_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_uint64",
                     (void *)crk_xdr_decode_array_uint64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_puint64_q, "buf");
       f->addArg(type_uint, "max");
#endif

    f = mod->addFunc(type_bool, "xdr_encode_array_float32",
                     (void *)crk_xdr_encode_array_float32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pfloat32_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_float32",
                     (void *)crk_xdr_decode_array_float32
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pfloat32_q, "buf");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_bool, "xdr_encode_array_float64",
                     (void *)crk_xdr_encode_array_float64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pfloat64_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_float64",
                     (void *)crk_xdr_decode_array_float64
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pfloat64_q, "buf");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_bool, "xdr_encode_array_bool",
                     (void *)crk_xdr_encode_array_bool
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pbool_q, "buf");
       f->addArg(type_uint, "count");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_uint, "xdr_decode_array_bool",
                     (void *)crk_xdr_decode_array_bool
                     );
       f->addArg(type_xdr, "xdrs");
       f->addArg(array_pbool_q, "buf");
       f->addArg(type_uint, "max");

    f = mod->addFunc(type_bool, "xdr_error",
                     (void *)crk_xdr_error
                     );


    mod->addConstant(type_int, "XDR_ENCODE",
                     static_cast<int>(XDR_ENCODE)
                     );

    mod->addConstant(type_int, "XDR_DECODE",
                     static_cast<int>(XDR_DECODE)
                     );

    mod->addConstant(type_int, "XDR_FREE",
                     static_cast<int>(XDR_FREE)
                     );

    mod->addConstant(type_int, "INT_SIZE",
                     static_cast<int>(INT_SIZE)
                     );
}
