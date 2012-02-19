// Copyright (C) 2010 Conrad D. Steenberg
// Lincensed under LGPLv3

#include <math.h>
#include <errno.h>
#include <fenv.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string>
#include "ext/Module.h"
#include "ext/Func.h"

using namespace crack::ext;
using namespace std;


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
                              { "rint", VALUE}, {"round", VALUE}, 
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

u_int64_t crk_gettimeofday(void){
  struct timeval crk_timeval;
  gettimeofday(&crk_timeval, NULL);
  return (unsigned long)(crk_timeval.tv_sec*1000000 + crk_timeval.tv_usec); // Time in usecs
}

//------------------------------------------------------------------------------
void math_init(Module *mod) {
  int i, j, numtypes=4;
  char buffer[100];
  char postfixes[][4] = {"", "f", "32", "64", ""};
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
      func = mod->addFunc(functypes[j], buffer, (void *) one_funcs[i]);
      func->addArg(functypes[j], arg_names[one_names[i].argname]);
    }

#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_names[i].funcname);
      strcat(buffer, postfixes[j]);
      funcd = mod->addFunc(functypes[j], buffer, (void *) one_funcs_double[i]);
      funcd->addArg(functypes[j], arg_names[one_names[i].argname]);
    }
#endif
  }

  for(i=0;one_macros[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);
      func = mod->addFunc(mod->getIntType(), buffer, (void *) one_macros[i]);
      func->addArg(functypes[j], "value");
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, one_macro_names[i]);
      strcat(buffer, postfixes[j]);
      funcd = mod->addFunc(mod->getIntType(), one_macro_names[i], (void *) one_macros_double[i]);
      funcd->addArg(functypes[j], "value");
    }
#endif
  }

   // Two argument functions
  for(i=0;two_funcs[i];i++){
    for (j=0; j<numtypes; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      func = mod->addFunc(functypes[j], buffer, (void *) two_funcs[i]);
      func->addArg(functypes[j], arg_names[two_names[i].argname1]);
      func->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#if FLT_EVAL_METHOD==0
// double and float are distinct types
    for (j=numtypes; j<5; j++){
      strcpy(buffer, two_names[i].funcname);
      strcat(buffer, postfixes[j]);

      funcd = mod->addFunc(functypes[j], two_names[i].funcname, (void *) two_funcs_double[i]);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname1]);
      funcd->addArg(functypes[j], arg_names[two_names[i].argname2]);
    }
#endif
  }

  // Add constants to module
  for (i=0; double_constants[i].name;i++){
    mod->addConstant(mod->getFloat64Type(), double_constants[i].name, double_constants[i].value);
  }

  for (i=0; int_constants[i].name;i++){
    mod->addConstant(mod->getIntType(), int_constants[i].name, int_constants[i].value);
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
  Func* time_func = mod->addFunc(mod->getUint64Type(), "usecs", (void *)crk_gettimeofday);

}


}} // namespace crack::runtime
