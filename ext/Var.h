// Copyright 2010-2011 Google Inc.
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_ext_Var_h_
#define _crack_ext_Var_h_

#include <map>
#include <string>

namespace crack { namespace ext {

class Module;
class Type;

// opaque Var class.
class Var {
    friend class Module;
    friend class Type;

    private:
        Type *type;
        std::string name;
        size_t offset;
        
        Var(Type *type, const std::string &name, size_t offset = 0) :
            type(type),
            name(name),
            offset(offset) {
        }
};

typedef std::map<std::string, Var *> VarMap;
typedef std::vector<Var *> VarVec;

}} // namespace crack::ext

// macro to calculate the offset of an instance variable
#define CRACK_OFFSET(type, field) \
    reinterpret_cast<size_t>(&(reinterpret_cast<type *>(0))->field)

#endif
