// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_BExtFuncDef_h_
#define _builder_llvm_BExtFuncDef_h_

namespace builder {
    class Builder;
};

namespace builder { namespace mvll {

// An external function definition.
class BExtFuncDef : public BFuncDef {
    private:
        void *addr;

    public:
        BExtFuncDef(FuncDef::Flags flags, const std::string &name,
                    size_t argCount,
                    void *addr
                    ) :
            BFuncDef(flags, name, argCount),
            addr(addr) {
        }

        virtual void *getFuncAddr(Builder &builder) {
            return addr;
        }

        virtual void *getExtFuncAddr() const { return addr; }
};

}} // namespace builder::mvll

#endif

