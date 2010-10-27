// Copyright 2010 Google Inc.
// XXX do we really need 3 reference counting mechanisms???

#ifndef _crack_ext_RCObj_h
#define _crack_ext_RCObj_h

namespace crack { namespace ext {

class RCObj {
    protected:
        unsigned int refCount;

    public:
        virtual ~RCObj();

        /**
         * Increments the reference count of the object.
         */
        void bind();
        
        /**
         * Decrements the reference count of the object, deleting it if it 
         * drops to zero.
         */
        void release();
};

}}

#endif


