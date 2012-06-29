// Copyright 2010 Conrad D. Steenberg <conrad.steenberg@gmail.com>

#include <math.h>
#include <errno.h>
#include <fenv.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <string>
#include <dlfcn.h>
#include "ext/Module.h"
#include "ext/Func.h"

using namespace crack::ext;
using namespace std;

// This version of the code does not wrap the math functions, but needs to specify
// their symbol names in addFunc
// Conversely symbols defined in a non-secondary library must _not_ be specified

#ifndef WRAP_SECONDARY_SYMBOLS
namespace crack { namespace runtime {

// Argument name definitions
enum arg_values{ VALUE, ANGLE, ENUMERATOR, DIVISOR, POWER, SIGN, X, Y, DIRECTION};
const char *arg_names[]={"value", "angle", "enumerator", "divisor", "power", 
                          "sign", "x", "y", "direction"};

// Function type definitions
typedef float (OneFuncFloat)(float);
typedef float (TwoFuncFloat)(float, float);
typedef int (OneMacroFuncFloat)(float);

#if FLT_EVAL_METHOD==0
// double and float are distinct types
typedef double (OneFuncDouble)(double);
typedef double (TwoFuncDouble)(double, double);
typedef int (OneMacroFuncDouble)(double);
#endif

// Structs to hold function and constant info
typedef struct _one_name_struct{
   const char *funcname;
   const char *cname;
   arg_values argname;
} one_name_struct;

typedef struct _two_name_struct{
   const char *funcname;
   arg_values argname1;
   arg_values argname2;
} two_name_struct;

typedef struct _double_constant_struct{
   const char *name;
   const double value;
} double_constant_struct;

typedef struct _int_constant_struct{
   const char *name;
   const int value;
} int_constant_struct;


// -----------------------------------------------------------------------------
const double_constant_struct double_constants[] = {
                                    {"HUGE_VAL",      HUGE_VAL},
                                    {"INFINITY",      INFINITY},
                                    {"NAN",           NAN},
                                    {NULL, 0}
                                 };


const int_constant_struct int_constants[]={{"ERANGE",        ERANGE}, 
                                               {"EINVAL",        EINVAL}, 
                                               {"ENOMEM",        ENOMEM}, 
                                               {"FP_INFINITE",   FP_INFINITE},
                                               {"FP_NAN",        FP_NAN},
                                               {"FP_NORMAL",     FP_NORMAL},
                                               {"FP_SUBNORMAL",  FP_SUBNORMAL},
                                               {"FP_ZERO",       FP_ZERO},
                                               {"FP_ILOGB0",     FP_ILOGB0},
                                               {"FP_ILOGBNAN",   FP_ILOGBNAN},
                                               {"ALL_EXCEPT",    FE_ALL_EXCEPT},
                                               {"INVALID",       FE_INVALID},
                                               {"DIVBYZERO",     FE_DIVBYZERO},
                                               {"OVERFLOW",      FE_OVERFLOW},
                                               {"UNDERFLOW",     FE_UNDERFLOW},
                                               {NULL, 0}};
// -----------------------------------------------------------------------------
// Functions that take a single float argument
const one_name_struct  one_names[]={ 
                              {"sin", "sin", ANGLE}, {"cos", "cos", ANGLE},
                              {"tan", "tan", ANGLE}, {"sinh", "sinh", ANGLE}, 
                              {"cosh", "cosh", ANGLE}, {"tanh", "tanh", ANGLE},
                              {"asin", "asin", VALUE}, {"acos", "acos", VALUE}, 
                              {"atan", "atan", VALUE}, {"asinh", "asinh", VALUE},
                              {"acosh", "acosh", VALUE}, {"atanh", "atanh", VALUE},
                              {"exp", "exp", VALUE}, {"exp2", "exp2", VALUE},
                              {"log", "log", VALUE}, {"abs", "fabs", VALUE},
                              {"log10", "log10", VALUE}, {"log1p", "log1p", VALUE},
                              {"log2", "log2", VALUE}, {"cbrt", "cbrt", VALUE},
                              {"sqrt", "sqrt", VALUE}, {"erf", "erf", VALUE},
                              {"erfc", "erfc", VALUE}, {"lgamma", "lgamma", VALUE},
                              {"tgamma", "tgamma", VALUE}, {"ceil", "ceil", VALUE},
                              {"floor", "floor", VALUE}, {"nearbyint", "nearbyint", VALUE},
                              {"rint", "rint", VALUE}, {"round", "round", VALUE}, 
                              {"trunc", "trunc", VALUE}, {"expm1", "expm1", VALUE},
                              {NULL, NULL, VALUE}
                           };

// Functions that take 2 float arguments
const two_name_struct   two_names[]={
                              {"fmod", ENUMERATOR, DIVISOR}, 
                              {"remainder", ENUMERATOR, DIVISOR}, 
                              {"copysign", VALUE, SIGN},
                              {"nextafter", VALUE, DIRECTION},
                              {"hypot", X, Y}, 
                              {"fdim", VALUE, DIRECTION},
                              {"pow", VALUE, POWER}, {NULL, VALUE, VALUE}
                           };


// Macros that take a single argument
const char *one_macro_names[]={"fpclassify", "isfinite", "isinf", "isnan",
                                  "isnormal", "sign", "ilogb", NULL};


OneFuncFloat *one_funcs[]= { sinf,   cosf,
                               tanf,   sinhf,
                               coshf,  tanhf,
                               asinf,  acosf,
                               atanf,  asinhf,
                               acoshf, atanhf,
                               expf,   exp2f,
                               logf,   fabsf,
                               log10f, log1pf,
                               log2f,  cbrtf,
                               sqrtf,  erff,
                               erfcf,  lgammaf,
                               tgammaf,ceilf,
                               floorf,  nearbyintf,
                               rintf,  roundf,
                               truncf, expm1f, NULL
                              };

// Functions that take two arguments
// Some of these are already implemented by the compiler

TwoFuncFloat *two_funcs[]=  { fmodf,     remainderf,
                                copysignf, nextafterf, hypotf,
                                fdimf,     powf, NULL
                              };


// Bindings for macros that take one argument
int crk_fpclassify(float x){
  return fpclassify(x);
}

int crk_isfinite(float x){
  return isfinite(x);
}

int crk_isinf(float x){
  return isinf(x);
}

int crk_isnan(float x){
  return isnan(x);
}

int crk_isnormal(float x){
  return isnormal(x);
}

int crk_signbit(float x){
  return signbit(x);
}

OneMacroFuncFloat *one_macros[]={crk_fpclassify, crk_isfinite,
                                    crk_isinf, crk_isnan, 
                                    crk_isnormal,
                                    crk_signbit, ilogbf, NULL};

// -----------------------------------------------------------------------------
#if FLT_EVAL_METHOD==0
OneFuncDouble *one_funcs_double[]= { sin,   cos,
                                       tan,   sinh,
                                       cosh,  tanh,
                                       asin,  acos,
                                       atan,  asinh,
                                       acosh, atanh,
                                       exp,   exp2,
                                       fabs,  log,
                                       log10, log1p,
                                       log2,  cbrt,
                                       sqrt,  erf,
                                       erfc,  lgamma,
                                       tgamma,ceil,
                                       floor, nearbyint,
                                       rint,  round,
                                       trunc, expm1, NULL
                                     };

TwoFuncDouble *two_funcs_double[]={  fmod,     remainder,
                                       copysign, nextafter, hypot,
                                       fdim,     pow, NULL};

// Bindings for macros that take one argument
int crk_fpclassify_double(double x){
  return fpclassify(x);
}

int crk_isfinite_double(double x){
  return isfinite(x);
}

int crk_isinf_double(double x){
  return isinf(x);
}

int crk_isnan_double(double x){
  return isnan(x);
}

int crk_isnormal_double(double x){
  return isnormal(x);
}

int crk_signbit_double(double x){
  return signbit(x);
}

int crk_get_errno(){
  return errno;
}

void crk_set_errno(int value){
  errno=value;
}

int crk_strtoi(char *s){
   errno = 0;
   return (int)strtol(s, (char**)NULL, 0);
}

float crk_strtof(char *s){
   errno = 0;
   return (float)strtof(s, (char**)NULL);
}

float crk_strtod(char *s){
   errno = 0;
   return (double)strtod(s, (char**)NULL);
}


OneMacroFuncDouble *one_macros_double[]={ crk_fpclassify_double, crk_isfinite_double,
                                             crk_isinf_double, crk_isinf_double,
                                             crk_isnan_double, crk_isnormal_double,
                                             crk_signbit_double, ilogb, NULL
                                           };

#endif

int64_t crk_gettimeofday(void){
  struct timeval crk_timeval;
  gettimeofday(&crk_timeval, NULL);
  return (long)(crk_timeval.tv_sec*1000000 + crk_timeval.tv_usec); // Time in usecs
}

//------------------------------------------------------------------------------
void math_init(Module *mod) {
  int i, j, numtypes=3;
  char buffer[100];
  char symbol_buffer[100];
  char postfixes[][5] = {"", "f", "32", "64", ""};
  char symbol_postfixes[][5] = {"f", "f", "f", "", ""};
  Type *functypes[] = {mod->getFloatType(), mod->getFloatType(),
                       mod->getFloat32Type(), mod->getFloat64Type(),
                       mod->getFloat64Type()};
  Func *func, *funcd;

#if FLT_EVAL_METHOD==0
  numtypes=3;
#endif

  // One argument functions
  for(i=0;one_funcs[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, one_names[i].funcname);
      strcat(buffer, postfixes[j]);

      strcpy(symbol_buffer, one_names[i].cname);
      strcat(symbol_buffer, symbol_postfixes[j]);

      func = mod->addFunc(functypes[j], buffer,
        (void *) one_funcs[i], symbol_buffer);
      func->addArg(functypes[j], arg_names[one_names[i].argname]);
    }

#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_names[i].funcname);
      strcat(buffer, postfixes[j]);

      strcpy(symbol_buffer, one_names[i].cname);
      strcat(symbol_buffer, symbol_postfixes[j]);

      funcd = mod->addFunc(functypes[j], buffer,
        (void *) one_funcs_double[i], symbol_buffer);
      funcd->addArg(functypes[j], arg_names[one_names[i].argname]);
    }
#endif
  }

  for(i=0;one_macros[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);

      func = mod->addFunc(mod->getIntType(), buffer,
        (void *) one_macros[i]);
      func->addArg(functypes[j], "value");
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);

      funcd = mod->addFunc(mod->getIntType(), one_macro_names[i],
        (void *) one_macros_double[i]);
      funcd->addArg(functypes[j], "value");
    }
#endif
  }

   // Two argument functions
  for(i=0;two_funcs[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      strcpy(symbol_buffer, one_names[i].funcname);
      strcat(symbol_buffer, symbol_postfixes[j]);

      func = mod->addFunc(functypes[j], buffer,
            (void *) two_funcs[i], symbol_buffer);
      func->addArg(functypes[j], arg_names[two_names[i].argname1]);
      func->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      strcpy(symbol_buffer, one_names[i].funcname);
      strcat(symbol_buffer, symbol_postfixes[j]);

      funcd = mod->addFunc(functypes[j], two_names[i].funcname,
        (void *) two_funcs_double[i], symbol_buffer);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname1]);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#endif
  }

  // Add constants to module
  for (i=0; double_constants[i].name;i++){
    mod->addConstant(mod->getFloat64Type(), double_constants[i].name,
      double_constants[i].value);
  }

  for (i=0; int_constants[i].name;i++){
    mod->addConstant(mod->getIntType(), int_constants[i].name,
      int_constants[i].value);
  }

  // Math error handling
  func = mod->addFunc(mod->getIntType(), "clearexcept", (void *) feclearexcept);
  func->addArg(mod->getIntType(), "errors");
  func = mod->addFunc(mod->getIntType(), "testexcept", (void *) fetestexcept);
  func->addArg(mod->getIntType(), "errors");

  // Some utility functions
  // Get and set errno
  Func *get_errno_func = mod->addFunc(mod->getIntType(), "errno", (void *)crk_get_errno);

  Func *set_errno_func = mod->addFunc(mod->getVoidType(), "setErrno", (void *)crk_set_errno);
  set_errno_func->addArg(mod->getIntType(), "value");

  // atoi
  Func *atoi_func = mod->addFunc(mod->getIntType(), "atoi", (void *)atoi);
  atoi_func->addArg(mod->getByteptrType(), "str");

  // strtoi
  Func *strtoi_func = mod->addFunc(mod->getIntType(), "strtoi", (void *)crk_strtoi);
  strtoi_func->addArg(mod->getByteptrType(), "str");

  // strtof
  Func *strtof_func = mod->addFunc(mod->getFloatType(), "strtof", (void *)crk_strtof);
  strtof_func->addArg(mod->getByteptrType(), "str");

  // atof like strtof, but no error checking
  Func *atof_func = mod->addFunc(mod->getFloatType(), "atof", (void *)atof);
  atof_func->addArg(mod->getByteptrType(), "str");

  // strtod
  Func *strtod_func = mod->addFunc(mod->getFloat64Type(), "strtod", (void *)crk_strtod);
  strtod_func->addArg(mod->getByteptrType(), "str");

  // gettimofday wrapper
  Func* time_func = mod->addFunc(mod->getInt64Type(), "usecs", (void *)crk_gettimeofday);

}


}} // namespace crack::runtime

#else
// Version of the code that wraps all the math functions and do not specify symbol
// names anywhere
namespace crack { namespace runtime {

// Argument name definitions

enum arg_values{ VALUE, ANGLE, ENUMERATOR, DIVISOR, POWER, SIGN, X, Y, DIRECTION};
const char *arg_names[]={"value", "angle", "enumerator", "divisor", "power", 
                          "sign", "x", "y", "direction"};

// Function type definitions
typedef float (OneFuncFloat)(float);
typedef float (TwoFuncFloat)(float, float);
typedef int (OneMacroFuncFloat)(float);

#if FLT_EVAL_METHOD==0
// double and float are distinct types
typedef double (OneFuncDouble)(double);
typedef double (TwoFuncDouble)(double, double);
typedef int (OneMacroFuncDouble)(double);
#endif

// Structs to hold function and constant info
typedef struct _one_name_struct{
   const char *funcname;
   arg_values argname;
} one_name_struct;

typedef struct _two_name_struct{
   const char *funcname;
   arg_values argname1;
   arg_values argname2;
} two_name_struct;

typedef struct _double_constant_struct{
   const char *name;
   const double value;
} double_constant_struct;

typedef struct _int_constant_struct{
   const char *name;
   const int value;
} int_constant_struct;


// -----------------------------------------------------------------------------
const double_constant_struct double_constants[] = {
                                    {"HUGE_VAL",      HUGE_VAL},
                                    {"INFINITY",      INFINITY},
                                    {"NAN",           NAN},
                                    {NULL, 0}
                                 };


const int_constant_struct int_constants[]={{"ERANGE",        ERANGE}, 
                                               {"EINVAL",        EINVAL}, 
                                               {"ENOMEM",        ENOMEM}, 
                                               {"FP_INFINITE",   FP_INFINITE},
                                               {"FP_NAN",        FP_NAN},
                                               {"FP_NORMAL",     FP_NORMAL},
                                               {"FP_SUBNORMAL",  FP_SUBNORMAL},
                                               {"FP_ZERO",       FP_ZERO},
                                               {"FP_ILOGB0",     FP_ILOGB0},
                                               {"FP_ILOGBNAN",   FP_ILOGBNAN},
                                               {"ALL_EXCEPT",    FE_ALL_EXCEPT},
                                               {"INVALID",       FE_INVALID},
                                               {"DIVBYZERO",     FE_DIVBYZERO},
                                               {"OVERFLOW",      FE_OVERFLOW},
                                               {"UNDERFLOW",     FE_UNDERFLOW},
                                               {NULL, 0}};
// -----------------------------------------------------------------------------
// Functions that take a single float argument
const one_name_struct  one_names[]={ 
                              {"sin", ANGLE}, {"cos", ANGLE},
                              {"tan", ANGLE}, {"sinh", ANGLE}, 
                              {"cosh", ANGLE}, {"tanh", ANGLE},
                              {"asin", VALUE}, {"acos", VALUE}, 
                              {"atan", VALUE}, {"asinh", VALUE},
                              {"acosh", VALUE}, {"atanh", VALUE},
                              {"exp", VALUE}, {"exp2", VALUE},
                              {"log", VALUE}, {"abs", VALUE},
                              {"log10", VALUE}, {"log1p", VALUE},
                              {"log2", VALUE}, {"cbrt", VALUE},
                              {"sqrt", VALUE}, {"erf", VALUE},
                              {"erfc", VALUE}, {"lgamma", VALUE},
                              {"tgamma", VALUE}, {"ceil", VALUE},
                              {"floor", VALUE}, {"nearbyint", VALUE},
                              {"rint", VALUE}, {"round", VALUE}, 
                              {"trunc", VALUE}, {"expm1", VALUE},
                              {NULL, VALUE}
                           };

// Functions that take 2 float arguments
const two_name_struct   two_names[]={
                              {"fmod", ENUMERATOR, DIVISOR}, 
                              {"remainder", ENUMERATOR, DIVISOR}, 
                              {"copysign", VALUE, SIGN},
                              {"nextafter", VALUE, DIRECTION},
                              {"hypot", X, Y}, 
                              {"fdim", VALUE, DIRECTION},
                              {"pow", VALUE, POWER}, {NULL, VALUE, VALUE}
                           };


// Macros that take a single argument
const char *one_macro_names[]={"fpclassify", "isfinite", "isinf", "isnan",
                                  "isnormal", "sign", "ilogb", NULL};

#define one_funcs_impl_float(fname) \
  float crack_##fname(float arg) { return fname(arg);}

one_funcs_impl_float(sinf)
one_funcs_impl_float(cosf)
one_funcs_impl_float(tanf)
one_funcs_impl_float(sinhf)
one_funcs_impl_float(coshf)
one_funcs_impl_float(tanhf)
one_funcs_impl_float(asinf)
one_funcs_impl_float(acosf)
one_funcs_impl_float(atanf)
one_funcs_impl_float(asinhf)
one_funcs_impl_float(acoshf)
one_funcs_impl_float(atanhf)
one_funcs_impl_float(expf)
one_funcs_impl_float(exp2f)
one_funcs_impl_float(logf)
one_funcs_impl_float(fabsf)
one_funcs_impl_float(log10f)
one_funcs_impl_float(log1pf)
one_funcs_impl_float(log2f)
one_funcs_impl_float(cbrtf)
one_funcs_impl_float(sqrtf)
one_funcs_impl_float(erff)
one_funcs_impl_float(erfcf)
one_funcs_impl_float(lgammaf)
one_funcs_impl_float(tgammaf)

one_funcs_impl_float(ceilf)
one_funcs_impl_float(floorf)
one_funcs_impl_float(nearbyintf)
one_funcs_impl_float(rintf)
one_funcs_impl_float(roundf)
one_funcs_impl_float(truncf)
one_funcs_impl_float(expm1f)


OneFuncFloat *one_funcs[]= { crack_sinf,  crack_cosf,
                               crack_tanf,  crack_sinhf,
                               crack_coshf,  crack_tanhf,
                               crack_asinf,  crack_acosf,
                               crack_atanf,  crack_asinhf,
                               crack_acoshf,  crack_atanhf,
                               crack_expf,  crack_exp2f,
                               crack_logf,  crack_fabsf,
                               crack_log10f,  crack_log1pf,
                               crack_log2f,  crack_cbrtf,
                               crack_sqrtf,  crack_erff,
                               crack_erfcf,  crack_lgammaf,
                               crack_tgammaf, crack_ceilf,
                               crack_floorf,  crack_nearbyintf,
                               crack_rintf,  crack_roundf,
                               crack_truncf, crack_expm1f, NULL
                              };

// Functions that take two arguments
// Some of these are already implemented by the compiler

TwoFuncFloat *two_funcs[]=  { fmodf,     remainderf,
                                copysignf, nextafterf, hypotf,
                                fdimf,     powf, NULL
                              };

// Bindings for macros that take one argument
int crk_fpclassify(float x){
  return fpclassify(x);
}

int crk_isfinite(float x){
  return isfinite(x);
}

int crk_isinf(float x){
  return isinf(x);
}

int crk_isnan(float x){
  return isnan(x);
}

int crk_isnormal(float x){
  return isnormal(x);
}

int crk_signbit(float x){
  return signbit(x);
}

OneMacroFuncFloat *one_macros[]={crk_fpclassify, crk_isfinite,
                                    crk_isinf, crk_isnan, 
                                    crk_isnormal,
                                    crk_signbit, ilogbf, NULL};

// -----------------------------------------------------------------------------
#if FLT_EVAL_METHOD==0

#define one_funcs_impl(fname) \
  double crack_##fname(double arg) { return fname(arg);}

#define two_funcs_impl(fname) \
  double crack_##fname(double arg1, double arg2) { return fname(arg1, arg2);}


one_funcs_impl(sin)
one_funcs_impl(cos)
one_funcs_impl(tan)

one_funcs_impl(sinh)
one_funcs_impl(cosh)
one_funcs_impl(tanh)

one_funcs_impl(asin)
one_funcs_impl(acos)
one_funcs_impl(atan)

one_funcs_impl(asinh)
one_funcs_impl(acosh)
one_funcs_impl(atanh)

one_funcs_impl(exp)
one_funcs_impl(exp2)
one_funcs_impl(fabs)
one_funcs_impl(log)

one_funcs_impl(log10)
one_funcs_impl(log1p)
one_funcs_impl(log2)

one_funcs_impl(cbrt)
one_funcs_impl(sqrt)
one_funcs_impl(erf)
one_funcs_impl(erfc)

one_funcs_impl(lgamma)
one_funcs_impl(tgamma)

one_funcs_impl(ceil)
one_funcs_impl(floor)
one_funcs_impl(nearbyint)

one_funcs_impl(rint)
one_funcs_impl(round)
one_funcs_impl(trunc)
one_funcs_impl(expm1)

OneFuncDouble *one_funcs_double[]= { crack_sin,  crack_cos,
                                       crack_tan,  crack_sinh,
                                       crack_cosh,  crack_tanh,
                                       crack_asin,  crack_acos,
                                       crack_atan,  crack_asinh,
                                       crack_acosh,  crack_atanh,
                                       crack_exp,   crack_exp2,
                                       crack_fabs,  crack_log,
                                       crack_log10, crack_log1p,
                                       crack_log2,  crack_cbrt,
                                       crack_sqrt,  crack_erf,
                                       crack_erfc,  crack_lgamma,
                                       crack_tgamma,  crack_ceil,
                                       crack_floor,  crack_nearbyint,
                                       crack_rint,  crack_round,
                                       crack_trunc, crack_expm1, NULL
                                     };

two_funcs_impl(fmod)
two_funcs_impl(remainder)
two_funcs_impl(copysign)
two_funcs_impl(nextafter)
two_funcs_impl(hypot)
two_funcs_impl(fdim)
two_funcs_impl(pow)

TwoFuncDouble *two_funcs_double[]={  crack_fmod,  crack_remainder,
                                       crack_copysign,  crack_nextafter,
                                       crack_hypot,  crack_fdim, crack_pow, NULL};

// Bindings for macros that take one argument
int crk_fpclassify_double(double x){
  return fpclassify(x);
}

int crk_isfinite_double(double x){
  return isfinite(x);
}

int crk_isinf_double(double x){
  return isinf(x);
}

int crk_isnan_double(double x){
  return isnan(x);
}

int crk_isnormal_double(double x){
  return isnormal(x);
}

int crk_signbit_double(double x){
  return signbit(x);
}

int crk_get_errno(){
  return errno;
}

void crk_set_errno(int value){
  errno=value;
}

int crk_strtoi(char *s){
   errno = 0;
   return (int)strtol(s, (char**)NULL, 0);
}

float crk_strtof(char *s){
   errno = 0;
   return (float)strtof(s, (char**)NULL);
}

float crk_strtod(char *s){
   errno = 0;
   return (double)strtod(s, (char**)NULL);
}


OneMacroFuncDouble *one_macros_double[]={ crk_fpclassify_double, crk_isfinite_double,
                                             crk_isinf_double, crk_isinf_double,
                                             crk_isnan_double, crk_isnormal_double,
                                             crk_signbit_double, ilogb, NULL
                                           };

#endif

int64_t crk_gettimeofday(void){
  struct timeval crk_timeval;
  gettimeofday(&crk_timeval, NULL);
  return (int64_t)(crk_timeval.tv_sec*1000000 + crk_timeval.tv_usec); // Time in usecs
}

//------------------------------------------------------------------------------
void math_init(Module *mod) {
  int i, j, numtypes=4;
  char buffer[100];
  char symbol_buffer[100];
  char postfixes[][5] = {"", "f", "32", "64", ""};
  char symbol_postfixes[][5] = {"f", "f", "f", "", ""};
  Type *functypes[] = {mod->getFloatType(), mod->getFloatType(),
                       mod->getFloat32Type(), mod->getFloat64Type(),
                       mod->getFloat64Type()};
  Func *func, *funcd;

#if FLT_EVAL_METHOD==0
  numtypes=3;
#endif

  // One argument functions
  for(i=0;one_funcs[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, one_names[i].funcname);
      strcat(buffer, postfixes[j]);

      func = mod->addFunc(functypes[j], buffer,
        (void *) one_funcs[i]);
      func->addArg(functypes[j], arg_names[one_names[i].argname]);
    }

#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_names[i].funcname);
      strcat(buffer, postfixes[j]);

      funcd = mod->addFunc(functypes[j], buffer,
        (void *) one_funcs_double[i]);
      funcd->addArg(functypes[j], arg_names[one_names[i].argname]);
    }
#endif
  }

  for(i=0;one_macros[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);

      func = mod->addFunc(mod->getIntType(), buffer,
        (void *) one_macros[i]);
      func->addArg(functypes[j], "value");
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);

      funcd = mod->addFunc(mod->getIntType(), one_macro_names[i],
        (void *) one_macros_double[i]);
      funcd->addArg(functypes[j], "value");
    }
#endif
  }

   // Two argument functions
  for(i=0;two_funcs[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      func = mod->addFunc(functypes[j], buffer,
            (void *) two_funcs[i]);
      func->addArg(functypes[j], arg_names[two_names[i].argname1]);
      func->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      funcd = mod->addFunc(functypes[j], two_names[i].funcname,
        (void *) two_funcs_double[i]);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname1]);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#endif
  }

  // Add constants to module
  for (i=0; double_constants[i].name;i++){
    mod->addConstant(mod->getFloat64Type(), double_constants[i].name,
      double_constants[i].value);
  }

  for (i=0; int_constants[i].name;i++){
    mod->addConstant(mod->getIntType(), int_constants[i].name,
      int_constants[i].value);
  }

  // Math error handling
  func = mod->addFunc(mod->getIntType(), "clearexcept", (void *) feclearexcept);
  func->addArg(mod->getIntType(), "errors");
  func = mod->addFunc(mod->getIntType(), "testexcept", (void *) fetestexcept);
  func->addArg(mod->getIntType(), "errors");

  // Some utility functions
  // Get and set errno
  Func *get_errno_func = mod->addFunc(mod->getIntType(), "errno", (void *)crk_get_errno);

  Func *set_errno_func = mod->addFunc(mod->getVoidType(), "setErrno", (void *)crk_set_errno);
  set_errno_func->addArg(mod->getIntType(), "value");

  // atoi
  Func *atoi_func = mod->addFunc(mod->getIntType(), "atoi", (void *)atoi);
  atoi_func->addArg(mod->getByteptrType(), "str");

  // strtoi
  Func *strtoi_func = mod->addFunc(mod->getIntType(), "strtoi", (void *)crk_strtoi);
  strtoi_func->addArg(mod->getByteptrType(), "str");

  // strtof
  Func *strtof_func = mod->addFunc(mod->getFloatType(), "strtof", (void *)crk_strtof);
  strtof_func->addArg(mod->getByteptrType(), "str");

  // atof like strtof, but no error checking
  Func *atof_func = mod->addFunc(mod->getFloatType(), "atof", (void *)atof);
  atof_func->addArg(mod->getByteptrType(), "str");

  // strtod
  Func *strtod_func = mod->addFunc(mod->getFloat64Type(), "strtod", (void *)crk_strtod);
  strtod_func->addArg(mod->getByteptrType(), "str");

  // gettimofday wrapper
  Func* time_func = mod->addFunc(mod->getInt64Type(), "usecs", (void *)crk_gettimeofday);

}


}} // namespace crack::runtime
#endif
