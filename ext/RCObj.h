// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// XXX do we really need 3 reference counting mechanisms???

#ifndef _crack_ext_RCObj_h
#define _crack_ext_RCObj_h

namespace crack { namespace ext {

class RCObj {
    protected:
        unsigned int refCount;

    public:
        RCObj() : refCount(1) {}
        virtual ~RCObj();

        /**
         * Increments the reference count of the object.
         */
        void bind()  { if (this) ++refCount; }
        
        /**
         * Decrements the reference count of the object, deleting it if it 
         * drops to zero.
         */
        void release() { if (this && !--refCount) delete this; }
};

}}

#endif


