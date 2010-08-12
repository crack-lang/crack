/*===========================================================================*\
     
    Copyright (C) 2006 Michael A. Muller
 
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

#ifndef SPUG_EXCEPTION_H
#define SPUG_EXCEPTION_H

#include <exception>
#include <string>
#include <iostream>
#include "TypeInfo.h"

namespace spug {

/**
 * Simple exception base class that allows exceptions to be thrown with an
 * informative message.
 */
class Exception : public std::exception {

    protected:
        std::string msg;

    public:
        Exception() {}
        Exception(const char *msg) : msg(msg) {}
        Exception(const std::string &msg) : msg(msg) {}

        ~Exception() throw () {}

        virtual const char *getClassName() const { 
            return TypeInfo::get(*this)->getName(); 
        }

        /**
         * Returns the user supplied message string.
         */
        virtual std::string getMessage() const {
            return msg;
        }

        friend std::ostream &
            operator <<(std::ostream &out, const Exception &err);
};

inline std::ostream &operator <<(std::ostream &out, const Exception &err) {
    out << err.getClassName() << ": " << err.getMessage();
    return out;
}

// some macros to make it extremely easy to define derived exceptions

// Defines an exception class derived from an arbitrary base class
#define SPUG_DERIVED_EXCEPTION(cls, base) \
    class cls : public base { \
        public: \
            cls() {} \
            cls(const char *msg) : base(msg) {} \
            cls(const std::string &msg) : base(msg) {} \
            virtual const char *getClassName() const { return #cls; } \
    };

// defines an exception class derived from spug::Exception
#define SPUG_EXCEPTION(cls) SPUG_DERIVED_EXCEPTION(cls, spug::Exception)

}

#endif

