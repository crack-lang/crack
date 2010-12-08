// Copyright (C) 2010 Conrad D. Steenberg
// Lincensed under LGPLv3

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include "ext/Module.h"
#include "ext/Func.h"

using namespace crack::ext;

// our exported functions
namespace crack { namespace runtime {


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
  FP_ILOGBNAN
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
                                  "FP_ILOGBNAN", NULL};

// Functions that take a single float argument
const char  *one_names[]={"sin", "cos", "tan", "sinh", "cosh", "tanh",
                             "asin", "acos", "atan", "asinh", "acosh", "atanh",
                             "exp", "exp2", "ilogb", "log", "log10", "log1p",
                             "log2", "cbrt", "abs","hypot",  "sqrt", "erf",
                             "erfc", "lgamma", "tgamma", "ceil", "floor",
                             "nearbyint", "rint", "round", "trunc", "expm1", NULL};

// Functions that take 2 float arguments
const char *two_names[]={"fmod", "remainder", "copysign", "nextafter",
                           "dim", "pow", NULL};


// Macros that take a single argument
const char *one_macro_names[]={"fpclassify", "isfinite", "isinf", "isnan",
                                  "isnormal", "sign", NULL};

float *one_funcs[]={ (float *)sinf,   (float *)cosf,
                      (float *)tanf,   (float *)sinhf,
                      (float *)coshf,  (float *)tanhf,
                      (float *)asinf,  (float *)acosf,
                      (float *)atanf,  (float *)asinhf,
                      (float *)acoshf, (float *)atanhf,
                      (float *)expf,   (float *)exp2f,
                      (float *)ilogbf, (float *)logf,
                      (float *)log10f, (float *)log1pf,
                      (float *)log2f,  (float *)cbrtf,
                      (float *)fabsf,  (float *)hypotf,
                      (float *)sqrtf,  (float *)erff,
                      (float *)erfcf,  (float *)lgammaf,
                      (float *)tgammaf,(float *)ceilf,
                      (float *)floor,  (float *)nearbyintf,
                      (float *)rintf,  (float *)roundf,
                      (float *)truncf, (float *)expm1f, NULL};

// Functions that take two arguments
// Some of these are already implemented by the compiler
float *two_funcs[]={(float *)fmod,     (float *)remainder,
                     (float *)copysign, (float *)nextafter,
                     (float *)fdim,     (float *)pow, NULL};


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


int *one_macros[]={(int *) crk_fpclassify, (int *) crk_isfinite,
                      (int *) crk_isinf, (int *) crk_isnan, (int *) crk_isnormal,
                      (int *) crk_signbit, NULL};


#if FLT_EVAL_METHOD==0
// double and float are distinct types
double *one_funcs_double[]={(double *)sin,  (double *)cos,
                              (double *)tan,   (double *)sinh,
                              (double *)cosh,  (double *)tanh,
                              (double *)asin,  (double *)acos,
                              (double *)atan,  (double *)asinh,
                              (double *)acosh, (double *)atanh,
                              (double *)exp,   (double *)exp2,
                              (double *)ilogb, (double *)log,
                              (double *)log10, (double *)log1p,
                              (double *)log2,  (double *)cbrt,
                              (double *)fabs,  (double *)hypot,
                              (double *)sqrt,  (double *)erf,
                              (double *)erfc,  (double *)lgamma,
                              (double *)tgamma,(double *)ceil,
                              (double *)floor, (double *)nearbyint,
                              (double *)rint,  (double *)round,
                              (double *)trunc, (double *)expm1, NULL};

double *two_funcs_double[]={(double *)fmod,     (double *)remainder,
                             (double *)copysign, (double *)nextafter,
                             (double *)fdim,     (double *)pow, NULL};

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

int *one_macros_double[]={(int *) crk_fpclassify_double, (int *) crk_isfinite_double,
                            (int *) crk_isinf_double, (int *) crk_isinf_double,
                            (int *) crk_isnan_double, (int *) crk_isnormal_double,
                            (int *) crk_signbit_double, NULL};



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
    Func *func = mod->addFunc(mod->getFloatType(), one_names[i], one_funcs[i]);
    func->addArg(mod->getFloatType(), "flt");
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), one_names[i], one_funcs_double[i]);
    funcd->addArg(mod->getFloat64Type(), "dbl");
#endif
  }

  for(i=0;one_macros[i];i++){
    Func *func = mod->addFunc(mod->getIntType(), one_macro_names[i], one_macros[i]);
    func->addArg(mod->getFloatType(), "flt");
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), one_macro_names[i], one_macros_double[i]);
    funcd->addArg(mod->getFloat64Type(), "dbl");
#endif
  }

  for(i=0;two_funcs[i];i++){
    Func *func = mod->addFunc(mod->getFloatType(), two_names[i], two_funcs[i]);
    func->addArg(mod->getFloatType(), "flt1");
    func->addArg(mod->getFloatType(), "flt2");
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    Func *funcd = mod->addFunc(mod->getFloat64Type(), two_names[i], two_funcs_double[i]);
    funcd->addArg(mod->getFloat64Type(), "dbl1");
    funcd->addArg(mod->getFloat64Type(), "dbl2");
#endif
  }

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
