// Copyright (C) 2010 Conrad D. Steenberg
// Lincensed under LGPLv3

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include "ext/Module.h"
#include "ext/Func.h"

using namespace crack::ext;


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

typedef struct _one_name_struct{
   const char *funcname;
   arg_values *argname;
} one_name_struct;

typedef struct _two_name_struct{
   const char *funcname;
   arg_values *argname1;
   arg_values *argname2;
} two_name_struct;


// -----------------------------------------------------------------------------
const double constants[] = {
  HUGE_VAL,
  INFINITY,
  NAN,
  FP_INFINITE,
  FP_NAN,
  FP_NORMAL,
  FP_SUBNORMAL,
  FP_ZERO,
  FP_ILOGB0,
  FP_ILOGBNAN,
  FE_ALL_EXCEPT,
  FE_INVALID,
  FE_DIVBYZERO,
  FE_OVERFLOW,
  FE_UNDERFLOW
};

const char  *constant_names[]={ "HUGE_VAL",
                                  "INFINITY",
                                  "NAN",
                                  "FP_INFINITE",
                                  "FP_NAN",
                                  "FP_NORMAL",
                                  "FP_SUBNORMAL",
                                  "FP_ZERO",
                                  "FP_ILOGB0",
                                  "FP_ILOGBNAN", 
                                  "ALL_EXCEPT",
                                  "INVALID",
                                  "DIVBYZERO",
                                  "OVERFLOW",
                                  "UNDERFLOW",
                                  ,NULL};

// Functions that take a single float argument
const one_name_struct  *one_names[]={{"sin", ANGLE}, {"cos", ANGLE},
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
                                         { "rint", VALUE}, {"round", VALUE}, 
                                         {"trunc", VALUE}, {"expm1", VALUE}, NULL};

// Functions that take 2 float arguments
const two_name_struct *two_names[]={{"fmod", ENUMERATOR, DIVISOR}, 
                                       {"remainder", ENUMERATOR, DIVISOR}, 
                                       {"copysign", VALUE, SIGN},
                                       {"nextafter", VALUE, DIRECTION},
                                       {"hypot", X, Y}, 
                                       {"dim", VALUE, DIRECTION},
                                       {"pow", VALUE, POWER}, NULL};


// Macros that take a single argument
const char *one_macro_names[]={"fpclassify", "isfinite", "isinf", "isnan",
                                  "isnormal", "sign", "ilogb", NULL};


OneFuncFloat *one_funcs[]={ sinf,   cosf,
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
                              truncf, expm1f, NULL};

// Functions that take two arguments
// Some of these are already implemented by the compiler

TwoFuncFloat *two_funcs[]= {fmodf,     remainderf,
                              copysignf, nextafterf, hypotf,
                              fdimf,     powf, NULL};


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
  return isinf(x);
}

int crk_isnormal(float x){
  return isinf(x);
}

int crk_signbit(float x){
  return isinf(x);
}

OneMacroFuncFloat *one_macros[]={crk_fpclassify, crk_isfinite,
                                    crk_isinf, crk_isnan, 
                                    crk_isnormal,
                                    crk_signbit, ilogbf, NULL};


#if FLT_EVAL_METHOD==0
OneFuncDouble *one_funcs_double[]={  sin,   cos,
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
                                       trunc, expm1, NULL};

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
  return isinf(x);
}

int crk_isnormal_double(double x){
  return isinf(x);
}

int crk_signbit_double(double x){
  return isinf(x);
}

OneMacroFuncDouble *one_macros_double[]={ crk_fpclassify_double, crk_isfinite_double,
                                             crk_isinf_double, crk_isinf_double,
                                             crk_isnan_double, crk_isnormal_double,
                                             crk_signbit_double, ilogb, NULL};

#endif

u_int64_t crk_gettimeofday(void){
  struct timeval crk_timeval;
  gettimeofday(&crk_timeval, NULL);
  return (unsigned long)(crk_timeval.tv_sec*1000000 + crk_timeval.tv_usec); // Time in usecs
}

//------------------------------------------------------------------------------
void math_init(Module *mod) {
  int i;

  // Add constants to module - not supported yet
  //~ for (i=0; constant_names[i];i++){
    //~ mod->addInstVar(intType, constant_names[i])
  //~ }

  for(i=0;one_funcs[i];i++){
    Func *func = mod->addFunc(mod->getFloatType(), one_names[i], (void *) one_funcs[i].funcname);
      func->addArg(mod->getFloatType(), one_funcs[i].argname);
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), one_names[i], (void *) one_funcs[i].funcname);
    funcd->addArg(mod->getFloat64Type(), one_funcs[i].argname);
#endif
  }

  for(i=0;one_macros[i];i++){
    Func *func = mod->addFunc(mod->getIntType(), one_macro_names[i], (void *) one_macros[i]);
    func->addArg(mod->getFloatType(), "value");
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), one_macro_names[i], (void *) one_macros_double[i]);
    funcd->addArg(mod->getFloat64Type(), "value");
#endif
  }

  for(i=0;two_funcs[i];i++){
    Func *func = mod->addFunc(mod->getFloatType(), two_names[i].funcname, (void *) two_funcs[i]);
    func->addArg(mod->getFloatType(), two_names[i].argname1);
    func->addArg(mod->getFloatType(), two_names[i].argname2);
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), two_names[i].funcname, (void *) two_funcs_double[i]);
    funcd->addArg(mod->getFloat64Type(), two_names[i].argname1);
    funcd->addArg(mod->getFloat64Type(), two_names[i].argname2);
#endif
  }

  // Math error handling
  Func *funcd = mod->addFunc(mod->getIntType(), "clearexcept", (void *) feclearexcept);
  funcd->addArg(mod->getIntType(), "errors");
  Func *funcd = mod->addFunc(mod->getIntType(), "testexcept", (void *) fetestexcept);
  funcd->addArg(mod->getIntType(), "errors");


  // Add constants known at compile time
  for(i=0;constant_names[i];i++){
    mod->addConstant(mod->getFloatType(), constant_names[i], constants[i]);
  }

  // Some utility functions
  // atoi
  Func *atoi_func = mod->addFunc(mod->getIntType(), "atoi", (int *)atoi);
  atoi_func->addArg(mod->getByteptrType(), "str");

  // strtof
  Func *strtof_func = mod->addFunc(mod->getFloatType(), "atof", (int *)atof);
  strtof_func->addArg(mod->getByteptrType(), "str");

  // atof like strtof, but no error checking
  Func *atof_func = mod->addFunc(mod->getFloatType(), "strtof", (int *)strtof);
  atof_func->addArg(mod->getByteptrType(), "str");

  // gettimofday wrapper
  Func* time_func = mod->addFunc(mod->getUint64Type(), "usecs", (u_int64_t *)crk_gettimeofday);
}


}} // namespace crack::runtime
