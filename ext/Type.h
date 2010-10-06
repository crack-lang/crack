// Copyright 2010 Google Inc.

#ifndef _crack_ext_Type_h_
#define _crack_ext_Type_h_

namespace model {
    class TypeDef;
}

namespace crack { namespace ext {

class Func;
class Module;

class Type {
    friend class Func;
    friend class Module;

    private:
        model::TypeDef *typeDef;
        Type(model::TypeDef *typeDef) : typeDef(typeDef) {}
};
    
}} // namespace crack::ext

#endif
