/*===========================================================================*\
     
    RCPtr.h - reference counted pointer template.

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

#ifndef SPUG_RCPTR_H
#define SPUG_RCPTR_H

#include <iostream>
#include <typeinfo>
#include <exception>
#include <assert.h>

namespace spug {

/**
 * Another intrusive reference-counted pointer class.  This class is all about
 * convenience - users are free to access the underlying pointer if they want
 * to.
 *
 * This is meant to be used to manage an instance of RCBase.  It can be used
 * with any class that implements incref() and decref().
 */
template <class T>
class RCPtr {
    private:
        // The raw pointer
        T *obj;

    public:

        /** Copy constructor. */
        RCPtr(const RCPtr<T> &other) : obj(other.obj) {
            if (obj) obj->incref();
        }

        /** Constructs a RCPtr from a T*. */
        RCPtr(T *obj0) : obj(obj0) {
            if (obj) obj->incref();
        }

        /** Construct from a derived class RCPtr */
        template <class U>
        RCPtr(const RCPtr<U> &other) : obj(0) {
            if (other.get()) {
                obj = dynamic_cast<T *>(other.get());
                if (!obj)
                    throw std::bad_cast();
                else
                    obj->incref();
            }
        }


        /** Constructs a RCPtr initialized to NULL. */
        RCPtr() : obj(0) {}

        ~RCPtr() {
            if (obj) obj->decref();
        }

        /** Copies another *RCPtrBase* to the receiver. */
        RCPtr<T> &operator =(const RCPtr<T> &other) {
            *this = other.obj;
            return *this;
        }

        /** Assigns a T* to the receiver. */
        RCPtr<T> &operator =(T *obj0) {
            // increment the new object, release the existing object.  The 
            // order is important, as the old object could reference the new 
            // one.
            if (obj0) obj0->incref();
            if (obj) obj->decref();

            // link to the new one
            obj = obj0;
            return *this;
        }


        /**
         * Convenience function, equivalent to dynamic_cast<T>(other);
         */
        template <class U>
        static T *cast(U *other) {
            return dynamic_cast<T *>(other);
        }

        /**
         * Like "cast()" but assert that the object is of the correct type.
         * Null values will also fail.
         */
        template <class U>
        static T *acast(U *other) {
            T *result = dynamic_cast<T *>(other);
            assert(result);
            return result;
        }

        /**
         * Convenience function, equivalent to dynamic_cast<T>(other.get());
         */
        template <class U>
        static T *rcast(const RCPtr<U> &other) {
            return dynamic_cast<T *>(other.get());
        }

        /**
         * Like "rcast()" but assert that the object is of the correct type.
         * Null values will also fail.
         */
        template <class U>
        static T *arcast(const RCPtr<U> &other) {
            T *result = dynamic_cast<T *>(other.get());
            assert(result);
            return result;
        }

        /**
         * Used to invoke a member function on the receiver's *ManagedObject*
         */
        T *operator ->() const {
            return obj;
        }

        /** Used to deal directly with the receiver's member object. */
        T &operator *() const {
            return *obj;
        }

        /** allows us to easily check for NULL in a conditional statement. */
        operator int() const {
            return (obj != NULL);
        }

        /**
            Allows us to compare the pointer value of two Managed Object
            Pointers.
         */
        template <class U>
        int operator ==(const RCPtr<U> &other) const {
            return obj == other.get();
        }

        template <class U>
        int operator !=(const RCPtr<U> &other) const {
            return obj != other.get();
        }

        int operator !=(const void *ptr) const {
            return obj != ptr;
        }

        /**
            Allows us to compare the pointer value to any kind of pointer.
            Basically, this exists to permit comparison to NULL.
         */
        int operator ==(const void *ptr) const {
            return (void*)obj == ptr;
        }

        /**
            Allows us to compare the pointer value to any kind of pointer.
            Basically, this exists to permit comparison to NULL.
         */
        friend int operator ==(const void *ptr1, const RCPtr<T> &ptr2) {
            return (void*)ptr2.obj == ptr1;
        }

        /**
            Allows us to compare the pointer value to any kind of pointer.
            Basically, this exists to permit comparison to NULL.
         */
        int operator ==(int ptr) const {
            return (int)obj == ptr;
        }

        /**
            Allows us to compare the pointer value to any kind of pointer.
            Basically, this exists to permit comparison to NULL.
         */
        friend int operator ==(int ptr1, const RCPtr<T> &ptr2) {
            return (int)ptr2.obj == ptr1;
        }

        /**
         * Returns the underlying raw pointer.
         */
        T *get() const {
            return obj;
        }

    };

} // namespace spug

/**
  macro to easily define RCPtr instances, complete with a forward
  declaration.
*/
#define SPUG_RCPTR(cls) class cls; typedef spug::RCPtr<cls> cls##Ptr;

#endif

