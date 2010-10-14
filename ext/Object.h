// Copyright 2010 Google Inc.

#ifndef _crack_ext_Object_h_
#define _crack_ext_Object_h_

namespace crack { namespace ext {

// stand-in for the crack.lang.Object base class.
class Object {
    private:
        void *__vtable;
    
    public:
        unsigned int refCount;
};
        

}} // namespace crack::ext

#endif

