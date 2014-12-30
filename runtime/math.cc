
#include <fenv.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

#define FUNC1(fname)     double crk_##fname(double arg) { return fname(arg); }

#define FUNC2(fname)     double crk_##fname(double arg1, double arg2) { return fname(arg1, arg2); }

FUNC1(sin)
FUNC1(cos)
FUNC1(tan)

FUNC1(sinh)
FUNC1(cosh)
FUNC1(tanh)

FUNC1(asin)
FUNC1(acos)
FUNC1(atan)

FUNC1(asinh)
FUNC1(acosh)
FUNC1(atanh)

FUNC1(sinf)
FUNC1(cosf)
FUNC1(tanf)

FUNC1(exp)
FUNC1(exp2)
FUNC1(fabs)
FUNC1(log)

FUNC1(log10)
FUNC1(log1p)
FUNC1(log2)

FUNC1(cbrt)
FUNC1(sqrt)
FUNC1(erf)
FUNC1(erfc)

FUNC1(lgamma)
FUNC1(tgamma)

FUNC1(ceil)
FUNC1(ceilf)
FUNC1(floor)
FUNC1(floorf)
FUNC1(nearbyint)
FUNC1(nearbyintf)

FUNC1(rint)
FUNC1(rintf)
FUNC1(round)
FUNC1(trunc)
FUNC1(expm1)

FUNC2(fmod)
FUNC2(remainder)
FUNC2(copysign)
FUNC2(nextafter)
FUNC2(hypot)
FUNC2(fdim)
FUNC2(pow)

int crk_fpclassify32(float x) {
    return fpclassify(x);
}

int crk_fpclassify64(double x) {
    return fpclassify(x);
}

int crk_isfinite32(float x) {
    return isfinite(x);
}

int crk_isfinite64(double x) {
    return isfinite(x);
}

int crk_isinf32(float x) {
    return isinf(x);
}

int crk_isinf64(double x) {
    return isinf(x);
}

int crk_isnan32(float x) {
    return isnan(x);
}

int crk_isnan64(double x) {
    return isnan(x);
}

int crk_isnormal32(float x) {
    return isnormal(x);
}

int crk_isnormal64(double x) {
    return isnormal(x);
}

int crk_signbit32(float x) {
    return signbit(x);
}

int crk_signbit64(double x) {
    return signbit(x);
}

int crk_get_errno() {
    return errno;
}

void crk_set_errno(int value) {
    errno = value;
}

float crk_strtof(char *s) {
   errno = 0;
   return (float)strtof(s, (char**)NULL);
}

float crk_strtod(char *s) {
    errno = 0;
    return (double)strtod(s, (char**)NULL);
}

int crk_strtoi(char *s) {
    errno = 0;
    return (int)strtol(s, (char**)NULL, 0);
}

int64_t crk_usecs(void) {
    struct timeval crk_timeval;
    gettimeofday(&crk_timeval, NULL);
    return (int64_t)(crk_timeval.tv_sec*1000000 + crk_timeval.tv_usec); // Time in usecs
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_runtime__math_rinit() {
    return;
}

extern "C"
void crack_runtime__math_cinit(crack::ext::Module *mod) {
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
    f = mod->addFunc(type_float64, "sin",
                     (void *)crk_sin
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "sin",
                     (void *)crk_sinf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "cos",
                     (void *)crk_cos
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "cos",
                     (void *)crk_cosf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "tan",
                     (void *)crk_tan
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "tan",
                     (void *)crk_tanf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "sinh",
                     (void *)crk_sinh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "sinh",
                     (void *)sinhf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "cosh",
                     (void *)crk_cosh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "cosh",
                     (void *)coshf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "tanh",
                     (void *)crk_tanh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "tanh",
                     (void *)tanhf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "asin",
                     (void *)asin
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "asin",
                     (void *)asinf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "acos",
                     (void *)acos
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "acos",
                     (void *)acosf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "atan",
                     (void *)crk_atan
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "atan",
                     (void *)atanf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "asinh",
                     (void *)crk_asinh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "asinh",
                     (void *)asinhf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "acosh",
                     (void *)crk_acosh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "acosh",
                     (void *)acoshf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "atanh",
                     (void *)crk_atanh
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "atanh",
                     (void *)atanhf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "exp",
                     (void *)crk_exp
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "exp",
                     (void *)expf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "exp2",
                     (void *)crk_exp2
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "exp2",
                     (void *)exp2f
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "log",
                     (void *)log
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "log",
                     (void *)logf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "abs",
                     (void *)crk_fabs
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "abs",
                     (void *)fabsf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "log10",
                     (void *)crk_log10
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "log10",
                     (void *)log10f
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "log1p",
                     (void *)crk_log1p
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "log1p",
                     (void *)log1pf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "log2",
                     (void *)crk_log2
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "log2",
                     (void *)log2f
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "cbrt",
                     (void *)crk_cbrt
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "cbrt",
                     (void *)cbrtf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "sqrt",
                     (void *)crk_sqrt
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "sqrt",
                     (void *)sqrtf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "erf",
                     (void *)crk_erf
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "erf",
                     (void *)erff
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "erfc",
                     (void *)crk_erfc
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "erfc",
                     (void *)erfcf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "lgamma",
                     (void *)crk_lgamma
                     );
       f->addArg(type_float64, "angle");

    f = mod->addFunc(type_float32, "lgamma",
                     (void *)lgammaf
                     );
       f->addArg(type_float32, "angle");

    f = mod->addFunc(type_float64, "tgamma",
                     (void *)crk_tgamma
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "tgamma",
                     (void *)tgammaf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "ceil",
                     (void *)crk_ceil
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "ceil",
                     (void *)crk_ceilf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "floor",
                     (void *)crk_floor
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "floor",
                     (void *)crk_floorf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "nearbyint",
                     (void *)crk_nearbyint
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "nearbyint",
                     (void *)crk_nearbyintf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "rint",
                     (void *)crk_rint
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "rint",
                     (void *)crk_rintf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "round",
                     (void *)crk_round
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "round",
                     (void *)roundf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "trunc",
                     (void *)crk_trunc
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "trunc",
                     (void *)truncf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "expm1",
                     (void *)crk_expm1
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "expm1",
                     (void *)expm1f
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "ilogb",
                     (void *)ilogb
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "ilogb",
                     (void *)ilogbf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "fpclassify",
                     (void *)crk_fpclassify32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "fpclassify",
                     (void *)crk_fpclassify64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_int, "isfinite",
                     (void *)crk_isfinite32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "isfinite",
                     (void *)crk_isfinite64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_int, "isinf",
                     (void *)crk_isinf32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "isinf",
                     (void *)crk_isinf64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_int, "isnan",
                     (void *)crk_isnan32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "isnan",
                     (void *)crk_isnan64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_int, "isnormal",
                     (void *)crk_isnormal32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "isnormal",
                     (void *)crk_isnormal64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_int, "sign",
                     (void *)crk_signbit32
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "sign",
                     (void *)crk_signbit64
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float64, "hypot",
                     (void *)hypot
                     );
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");

    f = mod->addFunc(type_float32, "hypot",
                     (void *)hypotf
                     );
       f->addArg(type_float32, "x");
       f->addArg(type_float32, "y");

    f = mod->addFunc(type_float64, "fmod",
                     (void *)crk_fmod
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "fmod",
                     (void *)fmodf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "remainder",
                     (void *)crk_remainder
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "remainder",
                     (void *)remainderf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "copysign",
                     (void *)crk_copysign
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "copysign",
                     (void *)copysignf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "nextafter",
                     (void *)crk_nextafter
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "nextafter",
                     (void *)nextafterf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "hypot",
                     (void *)crk_hypot
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "hypot",
                     (void *)hypotf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "fdim",
                     (void *)crk_fdim
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "fdim",
                     (void *)fdimf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_float64, "pow",
                     (void *)crk_pow
                     );
       f->addArg(type_float64, "val");

    f = mod->addFunc(type_float32, "pow",
                     (void *)powf
                     );
       f->addArg(type_float32, "val");

    f = mod->addFunc(type_int, "errno",
                     (void *)crk_get_errno
                     );

    f = mod->addFunc(type_void, "setErrno",
                     (void *)crk_set_errno
                     );
       f->addArg(type_int, "errno");

    f = mod->addFunc(type_int, "testexcept",
                     (void *)fetestexcept
                     );
       f->addArg(type_int, "errors");

    f = mod->addFunc(type_int, "clearexcept",
                     (void *)feclearexcept
                     );
       f->addArg(type_int, "errors");

    f = mod->addFunc(type_int, "atoi",
                     (void *)atoi
                     );
       f->addArg(type_byteptr, "str");

    f = mod->addFunc(type_float, "atof",
                     (void *)atof
                     );
       f->addArg(type_byteptr, "str");

    f = mod->addFunc(type_int, "strtoi",
                     (void *)crk_strtoi
                     );
       f->addArg(type_byteptr, "str");

    f = mod->addFunc(type_float32, "strtof",
                     (void *)crk_strtof
                     );
       f->addArg(type_byteptr, "str");

    f = mod->addFunc(type_float64, "strtod",
                     (void *)crk_strtod
                     );
       f->addArg(type_byteptr, "str");

    f = mod->addFunc(type_int64, "usecs",
                     (void *)crk_usecs
                     );


    mod->addConstant(type_float64, "HUGE_VAL",
                     static_cast<double>(HUGE_VAL)
                     );

    mod->addConstant(type_float64, "INFINITY",
                     static_cast<double>(INFINITY)
                     );

    mod->addConstant(type_float64, "NAN",
                     static_cast<double>(NAN)
                     );

    mod->addConstant(type_int, "ERANGE",
                     static_cast<int>(ERANGE)
                     );

    mod->addConstant(type_int, "EINVAL",
                     static_cast<int>(EINVAL)
                     );

    mod->addConstant(type_int, "ENOMEM",
                     static_cast<int>(ENOMEM)
                     );

    mod->addConstant(type_int, "FP_INFINITE",
                     static_cast<int>(FP_INFINITE)
                     );

    mod->addConstant(type_int, "FP_NAN",
                     static_cast<int>(FP_NAN)
                     );

    mod->addConstant(type_int, "FP_NORMAL",
                     static_cast<int>(FP_NORMAL)
                     );

    mod->addConstant(type_int, "FP_SUBNORMAL",
                     static_cast<int>(FP_SUBNORMAL)
                     );

    mod->addConstant(type_int, "FP_ZERO",
                     static_cast<int>(FP_ZERO)
                     );

    mod->addConstant(type_int, "FP_ILOGB0",
                     static_cast<int>(FP_ILOGB0)
                     );

    mod->addConstant(type_int, "FP_ILOGBNAN",
                     static_cast<int>(FP_ILOGBNAN)
                     );

    mod->addConstant(type_int, "ALL_EXCEPT",
                     static_cast<int>(FE_ALL_EXCEPT)
                     );

    mod->addConstant(type_int, "INVALID",
                     static_cast<int>(FE_INVALID)
                     );

    mod->addConstant(type_int, "DIVBYZERO",
                     static_cast<int>(FE_DIVBYZERO)
                     );

    mod->addConstant(type_int, "OVERFLOW",
                     static_cast<int>(FE_OVERFLOW)
                     );

    mod->addConstant(type_int, "UNDERFLOW",
                     static_cast<int>(FE_UNDERFLOW)
                     );
}
