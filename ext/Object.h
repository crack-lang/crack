// Copyright 2010 Google Inc.

#ifndef _crack_ext_Object_h_
#define _crack_ext_Object_h_

#include "crack_config.h"

namespace crack { namespace ext {

// stand-in for the crack.lang.Object base class.
class Object {
    private:
        void *__vtable;
    
    public:
        unsigned int refCount;
#if SIZEOF_VOID_P == 8
        unsigned int padding;  // hack to work around padding issues.
#endif
};
        

}} // namespace crack::ext

#endif

