// Copyright 2010 Google Inc.

#ifndef _crack_ext_Type_h_
#define _crack_ext_Type_h_

#include <map>
#include <string>
#include <vector>

#include "Var.h"

namespace model {
    class Context;
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
        
        typedef std::vector<Type *> TypeVec;
        typedef std::map<std::string, Func *> FuncMap;
        
        // Impl holds everything that we need to create a new type
        struct Impl {
            std::string name;
            model::Context *context;
            TypeVec bases;
            FuncMap funcs;
            VarMap instVars;
            
            Impl(const std::string &name, model::Context *context) :
                name(name),
                context(context) {
            }
            
            ~Impl();
        };

        Impl *impl;

        Type(model::TypeDef *typeDef) : typeDef(typeDef), impl(0) {}
        Type(const std::string &name, model::Context *context) : 
            typeDef(0),
            impl(new Impl(name, context)) {
        }
        ~Type();

        // verify that the type has been "finished" (presumably before using 
        // it).
        void checkFinished();
    
    public:
        
        /**
         * Add a new base class.
         * The new base must not already be in the class' ancestry.
         */
        void addBase(Type *base);

        /**
         * Add an instance variable.
         */
        void addInstVar(Type *type, const std::string &name);
        
        /**
         * Add a new method to the class and returns it.
         * @return the new method.
         * @param returnType the method's return type
         * @param name the method name
         * @param funcPtr the C function that implements the method.  The 
         *        first parameter of this function should be an instance of 
         *        the type.
         */
        Func *addMethod(Type *returnType, const std::string &name,
                        void *funcPtr
                        );
        
        /**
         * Add a new static method to the class and returns it.  Static 
         * methods have no implicit "this" parameter.
         * @return the new method.
         * @param returnType the method's return type
         * @param name the method name
         * @param funcPtr the C function that implements the method.  The 
         *        first parameter of this function should be an instance of 
         *        the type.
         */
        Func *addStaticMethod(Type *returnType, const std::string &name,
                              void *funcPtr
                              );
        
        /**
         * Mark the new type as "finished"
         */
        void finish();
};
    
}} // namespace crack::ext

#endif
