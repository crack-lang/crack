#include <pcre.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__pcre_rinit() {
    return;
}

extern "C"
void crack_ext__pcre_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_PCRE = mod->addType("PCRE", sizeof(int));
    type_PCRE->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_PCRE, "pcre_compile2",
                     (void *)pcre_compile2
                     );
       f->addArg(type_byteptr, "pattern");
       f->addArg(type_int, "options");
       f->addArg(array_pint_q, "errorCode");
       f->addArg(array_pbyteptr_q, "errorText");
       f->addArg(array_pint_q, "errorOffset");
       f->addArg(type_byteptr, "tablePtr");

    f = mod->addFunc(type_int, "pcre_exec",
                     (void *)pcre_exec
                     );
       f->addArg(type_PCRE, "pcre");
       f->addArg(type_voidptr, "extra");
       f->addArg(type_byteptr, "subject");
       f->addArg(type_uint, "subjectSize");
       f->addArg(type_int, "startOffset");
       f->addArg(type_int, "options");
       f->addArg(array_pint_q, "outputVec");
       f->addArg(type_uint, "outputVecSize");

    f = mod->addFunc(type_void, "pcre_fullinfo",
                     (void *)pcre_fullinfo
                     );
       f->addArg(type_PCRE, "pcre");
       f->addArg(type_voidptr, "extra");
       f->addArg(type_int, "param");
       f->addArg(array_pint_q, "result");

    f = mod->addFunc(type_int, "pcre_get_stringnumber",
                     (void *)pcre_get_stringnumber
                     );
       f->addArg(type_PCRE, "pcre");
       f->addArg(type_byteptr, "name");


    mod->addConstant(type_int, "PCRE_ANCHORED",
                     static_cast<int>(PCRE_ANCHORED)
                     );

    mod->addConstant(type_int, "PCRE_DOTALL",
                     static_cast<int>(PCRE_DOTALL)
                     );

    mod->addConstant(type_int, "PCRE_AUTO_CALLOUT",
                     static_cast<int>(PCRE_AUTO_CALLOUT)
                     );

    mod->addConstant(type_int, "PCRE_CASELESS",
                     static_cast<int>(PCRE_CASELESS)
                     );

    mod->addConstant(type_int, "PCRE_DOLLAR_ENDONLY",
                     static_cast<int>(PCRE_DOLLAR_ENDONLY)
                     );

    mod->addConstant(type_int, "PCRE_DOTALL",
                     static_cast<int>(PCRE_DOTALL)
                     );

    mod->addConstant(type_int, "PCRE_EXTENDED",
                     static_cast<int>(PCRE_EXTENDED)
                     );

    mod->addConstant(type_int, "PCRE_EXTRA",
                     static_cast<int>(PCRE_EXTRA)
                     );

    mod->addConstant(type_int, "PCRE_FIRSTLINE",
                     static_cast<int>(PCRE_FIRSTLINE)
                     );

    mod->addConstant(type_int, "PCRE_MULTILINE",
                     static_cast<int>(PCRE_MULTILINE)
                     );

    mod->addConstant(type_int, "PCRE_NO_AUTO_CAPTURE",
                     static_cast<int>(PCRE_NO_AUTO_CAPTURE)
                     );

    mod->addConstant(type_int, "PCRE_UNGREEDY",
                     static_cast<int>(PCRE_UNGREEDY)
                     );

    mod->addConstant(type_int, "PCRE_UTF8",
                     static_cast<int>(PCRE_UTF8)
                     );

    mod->addConstant(type_int, "PCRE_NO_UTF8_CHECK",
                     static_cast<int>(PCRE_NO_UTF8_CHECK)
                     );
}
