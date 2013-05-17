// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 6/22/2012
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "util/md5.h"
md5_state_s *crk_new_md5_state() {
   md5_state_s *state = new md5_state_s;
   md5_init(state);
   return state;
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_runtime_md5_rinit() {
    return;
}

extern "C"
void crack_runtime_md5_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_md5 = mod->addType("md5", sizeof(md5_state_s));
    type_md5->finish();

    f = mod->addFunc(type_md5, "md5_init",
                     (void *)crk_new_md5_state
                     );

    f = mod->addFunc(type_void, "md5_append",
                     (void *)md5_append
                     );
       f->addArg(type_md5, "state");
       f->addArg(type_byteptr, "data");
       f->addArg(type_int, "nbytes");

    f = mod->addFunc(type_void, "md5_finish",
                     (void *)md5_finish
                     );
       f->addArg(type_md5, "state");
       f->addArg(type_byteptr, "digest");

}
