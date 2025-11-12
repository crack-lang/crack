#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <iostream>

struct Undef {};

#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__pcre2_rinit() {
    return;
}

extern "C"
void crack_ext__pcre2_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_PCRECompileContext = mod->addType("PCRECompileContext", sizeof(Undef));
    type_PCRECompileContext->finish();


    crack::ext::Type *type_PCRE2MatchContext = mod->addType("PCRE2MatchContext", sizeof(Undef));
    type_PCRE2MatchContext->finish();


    crack::ext::Type *type_PCRE2Code = mod->addForwardType("PCRE2Code", sizeof(Undef));

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pintz_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_intz;
        array_pintz_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_PCRE2MatchData = mod->addType("PCRE2MatchData", sizeof(Undef));
f = type_PCRE2MatchData->addStaticMethod(
    type_PCRE2MatchData,
    "oper new",
    (void *)pcre2_match_data_create_from_pattern
);
    f->addArg(type_PCRE2Code,
              "code"
              );
    f->addArg(type_PCRE2MatchContext,
              "context"
              );

f = type_PCRE2MatchData->addMethod(
    type_PCRE2MatchData,
    "oper del",
    (void *)pcre2_match_data_free
);


    f = type_PCRE2MatchData->addMethod(
        type_uint32,
        "getOVectorCount",
        (void *)pcre2_get_ovector_count
    );


    f = type_PCRE2MatchData->addMethod(
        array_pintz_q,
        "getOVectorPointer",
        (void *)pcre2_get_ovector_pointer
    );

    type_PCRE2MatchData->finish();


    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    // Definition of forward type PCRE2Code -----------------------------------
f = type_PCRE2Code->addStaticMethod(
    type_PCRE2Code,
    "oper new",
    (void *)pcre2_compile
);
    f->addArg(type_byteptr,
              "pattern"
              );
    f->addArg(type_intz,
              "length"
              );
    f->addArg(type_uint32,
              "options"
              );
    f->addArg(array_pint_q,
              "errorCode"
              );
    f->addArg(array_pintz_q,
              "errorOffset"
              );
    f->addArg(type_PCRECompileContext,
              "context"
              );

f = type_PCRE2Code->addMethod(
    type_PCRE2Code,
    "oper del",
    (void *)pcre2_code_free
);


    f = type_PCRE2Code->addMethod(
        type_int,
        "match",
        (void *)pcre2_match
    );
    f->addArg(type_byteptr,
              "subject"
              );
    f->addArg(type_intz,
              "subjectLength"
              );
    f->addArg(type_intz,
              "startOffset"
              );
    f->addArg(type_uint32,
              "options"
              );
    f->addArg(type_PCRE2MatchData,
              "matchData"
              );
    f->addArg(type_PCRE2MatchContext,
              "mcontext"
              );


    f = type_PCRE2Code->addMethod(
        type_int,
        "substringNumberFromName",
        (void *)pcre2_substring_number_from_name
    );
    f->addArg(type_byteptr,
              "name"
              );

    type_PCRE2Code->finish();

    f = mod->addFunc(type_int, "pcre2_get_error_message",
                     (void *)pcre2_get_error_message
                     );
       f->addArg(type_int, "errorcode");
       f->addArg(type_byteptr, "buffer");
       f->addArg(type_intz, "bufflen");


    mod->addConstant(type_uint32, "PCRE2_ANCHORED",
                     static_cast<int>(PCRE2_ANCHORED)
                     );

    mod->addConstant(type_uint32, "PCRE2_ALLOW_EMPTY_CLASS",
                     static_cast<int>(PCRE2_ALLOW_EMPTY_CLASS)
                     );

    mod->addConstant(type_uint32, "PCRE2_ALT_BSUX",
                     static_cast<int>(PCRE2_ALT_BSUX)
                     );

    mod->addConstant(type_uint32, "PCRE2_ALT_CIRCUMFLEX",
                     static_cast<int>(PCRE2_ALT_CIRCUMFLEX)
                     );

    mod->addConstant(type_uint32, "PCRE2_ALT_EXTENDED_CLASS",
                     static_cast<int>(PCRE2_ALT_EXTENDED_CLASS)
                     );

    mod->addConstant(type_uint32, "PCRE2_ALT_VERBNAMES",
                     static_cast<int>(PCRE2_ALT_VERBNAMES)
                     );

    mod->addConstant(type_uint32, "PCRE2_AUTO_CALLOUT",
                     static_cast<int>(PCRE2_AUTO_CALLOUT)
                     );

    mod->addConstant(type_uint32, "PCRE2_CASELESS",
                     static_cast<int>(PCRE2_CASELESS)
                     );

    mod->addConstant(type_uint32, "PCRE2_DOLLAR_ENDONLY",
                     static_cast<int>(PCRE2_DOLLAR_ENDONLY)
                     );

    mod->addConstant(type_uint32, "PCRE2_DOTALL",
                     static_cast<int>(PCRE2_DOTALL)
                     );

    mod->addConstant(type_uint32, "PCRE2_DUPNAMES",
                     static_cast<int>(PCRE2_DUPNAMES)
                     );

    mod->addConstant(type_uint32, "PCRE2_ENDANCHORED",
                     static_cast<int>(PCRE2_ENDANCHORED)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTENDED",
                     static_cast<int>(PCRE2_EXTENDED)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTENDED_MORE",
                     static_cast<int>(PCRE2_EXTENDED_MORE)
                     );

    mod->addConstant(type_uint32, "PCRE2_FIRSTLINE",
                     static_cast<int>(PCRE2_FIRSTLINE)
                     );

    mod->addConstant(type_uint32, "PCRE2_LITERAL",
                     static_cast<int>(PCRE2_LITERAL)
                     );

    mod->addConstant(type_uint32, "PCRE2_MATCH_INVALID_UTF",
                     static_cast<int>(PCRE2_MATCH_INVALID_UTF)
                     );

    mod->addConstant(type_uint32, "PCRE2_MATCH_UNSET_BACKREF",
                     static_cast<int>(PCRE2_MATCH_UNSET_BACKREF)
                     );

    mod->addConstant(type_uint32, "PCRE2_MULTILINE",
                     static_cast<int>(PCRE2_MULTILINE)
                     );

    mod->addConstant(type_uint32, "PCRE2_NEVER_BACKSLASH_C",
                     static_cast<int>(PCRE2_NEVER_BACKSLASH_C)
                     );

    mod->addConstant(type_uint32, "PCRE2_NEVER_UCP",
                     static_cast<int>(PCRE2_NEVER_UCP)
                     );

    mod->addConstant(type_uint32, "PCRE2_NEVER_UTF",
                     static_cast<int>(PCRE2_NEVER_UTF)
                     );

    mod->addConstant(type_uint32, "PCRE2_NO_AUTO_CAPTURE",
                     static_cast<int>(PCRE2_NO_AUTO_CAPTURE)
                     );

    mod->addConstant(type_uint32, "PCRE2_NO_AUTO_POSSESS",
                     static_cast<int>(PCRE2_NO_AUTO_POSSESS)
                     );

    mod->addConstant(type_uint32, "PCRE2_NO_DOTSTAR_ANCHOR",
                     static_cast<int>(PCRE2_NO_DOTSTAR_ANCHOR)
                     );

    mod->addConstant(type_uint32, "PCRE2_NO_START_OPTIMIZE",
                     static_cast<int>(PCRE2_NO_START_OPTIMIZE)
                     );

    mod->addConstant(type_uint32, "PCRE2_NO_UTF_CHECK",
                     static_cast<int>(PCRE2_NO_UTF_CHECK)
                     );

    mod->addConstant(type_uint32, "PCRE2_UCP",
                     static_cast<int>(PCRE2_UCP)
                     );

    mod->addConstant(type_uint32, "PCRE2_UNGREEDY",
                     static_cast<int>(PCRE2_UNGREEDY)
                     );

    mod->addConstant(type_uint32, "PCRE2_USE_OFFSET_LIMIT",
                     static_cast<int>(PCRE2_USE_OFFSET_LIMIT)
                     );

    mod->addConstant(type_uint32, "PCRE2_UTF",
                     static_cast<int>(PCRE2_UTF)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK",
                     static_cast<int>(PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES",
                     static_cast<int>(PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ALT_BSUX",
                     static_cast<int>(PCRE2_EXTRA_ALT_BSUX)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ASCII_BSD",
                     static_cast<int>(PCRE2_EXTRA_ASCII_BSD)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ASCII_BSS",
                     static_cast<int>(PCRE2_EXTRA_ASCII_BSS)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ASCII_BSW",
                     static_cast<int>(PCRE2_EXTRA_ASCII_BSW)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ASCII_DIGIT",
                     static_cast<int>(PCRE2_EXTRA_ASCII_DIGIT)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ASCII_POSIX",
                     static_cast<int>(PCRE2_EXTRA_ASCII_POSIX)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL",
                     static_cast<int>(PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_CASELESS_RESTRICT",
                     static_cast<int>(PCRE2_EXTRA_CASELESS_RESTRICT)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_ESCAPED_CR_IS_LF",
                     static_cast<int>(PCRE2_EXTRA_ESCAPED_CR_IS_LF)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_MATCH_LINE",
                     static_cast<int>(PCRE2_EXTRA_MATCH_LINE)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_MATCH_WORD",
                     static_cast<int>(PCRE2_EXTRA_MATCH_WORD)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_NO_BS0",
                     static_cast<int>(PCRE2_EXTRA_NO_BS0)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_PYTHON_OCTAL",
                     static_cast<int>(PCRE2_EXTRA_PYTHON_OCTAL)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_NEVER_CALLOUT",
                     static_cast<int>(PCRE2_EXTRA_NEVER_CALLOUT)
                     );

    mod->addConstant(type_uint32, "PCRE2_EXTRA_TURKISH_CASING",
                     static_cast<int>(PCRE2_EXTRA_TURKISH_CASING)
                     );

    mod->addConstant(type_uint32, "PCRE2_ERROR_NOSUBSTRING",
                     static_cast<int>(PCRE2_ERROR_NOSUBSTRING)
                     );
}
