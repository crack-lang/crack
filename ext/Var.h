// Copyright 2010 Google Inc.

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
        
        Var(Type *type, const std::string &name) :
            type(type),
            name(name) {
        }
};

typedef std::map<std::string, Var *> VarMap;
typedef std::vector<Var *> VarVec;

}} // namespace crack::ext

#endif