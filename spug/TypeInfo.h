// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
#ifndef SPUG_TYPEINFO_H
#define SPUG_TYPEINFO_H

#include "RCPtr.h"
#include "RCBase.h"
#include <typeinfo>
#include <cxxabi.h>

namespace spug {

SPUG_RCPTR(TypeInfo);

/**
 * Wrapper around std::type_info.  Currently provides name demangling.
 */
class TypeInfo : public RCBase {

    private:

        // primitive type info
        const std::type_info &ti;
        mutable const char *realName;

        // copying verboten
        TypeInfo(const TypeInfo &other);
        void operator =(const TypeInfo &other);

        TypeInfo(const std::type_info &primInfo) : ti(primInfo), realName(0) {}
    public:

        /**
         * Returns the TypeInfo instance for a given instance.
         *
         * \todo keep a registry of types.
         */
        template <typename T>
        static TypeInfoPtr get(const T &inst) {
            return new TypeInfo(typeid(inst));
        }

        /** Returns the demangled class name.  */
        const char *getName() const {
            int status;
            if (!realName)
                realName = abi::__cxa_demangle(ti.name(), 0, 0, &status);
            return realName;
        }
};

} // namespace spug

#endif
