/*===========================================================================*\
     
    RCBase.h - reference counted object base class

    Copyright (C) 2005 Michael A. Muller
 
    This file is part of spug++.
 
    spug++ is free software: you can redistribute it and/or modify it under the 
    terms of the GNU Lesser General Public License as published by the Free 
    Software Foundation, either version 3 of the License, or (at your option) 
    any later version.
 
    spug++ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public License 
    along with spug++.  If not, see <http://www.gnu.org/licenses/>.
 
\*===========================================================================*/

#ifndef SPUG_RCBASE_H
#define SPUG_RCBASE_H

namespace spug {

/** 
 * Reference counting base class.  This class is not thread-safe.
 */
class RCBase {

    private:
        int refCount;

    public:
        RCBase() : refCount(0) {}
        virtual ~RCBase() {}

        /** increment the reference count */
        void incref() { ++refCount; }

        /** decrement the reference count */
        void decref() { if (!--refCount) delete this; }

        /** return the reference count */
        int refcnt() const { return refCount; }
};

}

#endif
