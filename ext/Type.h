// Copyright 2010 Google Inc.

#ifndef _crack_ext_Type_h_
#define _crack_ext_Type_h_

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

    public:
        typedef std::vector<Func *> FuncVec;

    private:        
        typedef std::vector<Type *> TypeVec;
        
        // Impl holds everything that we need to create a new type
        struct Impl {
            std::string name;
            model::Context *context;
            TypeVec bases;
            FuncVec funcs;
            VarMap instVars;
            VarVec instVarVec;
            size_t instSize;
            
            Impl(const std::string &name, 
                 model::Context *context,
                 size_t instSize
                 ) :
                name(name),
                context(context),
                instSize(instSize) {
            }
            
            ~Impl();
        };


        Type(Module *module, model::TypeDef *typeDef) : 
            module(module),
            typeDef(typeDef), 
            impl(0),
            finished(false) {
        }

        Type(Module *module, const std::string &name, 
             model::Context *context,
             size_t instSize
             ) : 
            typeDef(0),
            module(module),
            impl(new Impl(name, context, instSize)),
            finished(false) {
        }

        // verify that the type has been initilized (has a non-null impl)
        void checkInitialized();

        // verify that the type has been "finished" (presumably before using 
        // it).
        void checkFinished();

    protected:

        bool finished;
        Module *module;
        Impl *impl;
        
        Type(Module *module, const std::string &name, 
             model::Context *context, 
             size_t instSize,
             model::TypeDef *typeDef) : 
            typeDef(typeDef),
            module(module),
            impl(new Impl(name, context, instSize)),
            finished(false) {
        }
    
        ~Type();

        bool isFinished() const;
        void setClasses(Func *f, model::TypeDef *base, model::TypeDef *wrapper,
                        model::Context *context);
        
    public:
        
        // this is public, it sucks.  Don't use it.
        model::TypeDef *typeDef;
        
        /**
         * Add a new base class.
         * The new base must not already be in the class' ancestry.
         */
        void addBase(Type *base);

        /**
         * Add an instance variable.
         */
        void addInstVar(Type *type, const std::string &name, size_t offset);
        
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
         * Add a new method to the class and returns it.
         * @return the new method.
         * @param returnType the method's return type
         * @param name the method name
         * @param body the function body
         */
        Func *addMethod(Type *returnType, const std::string &name,
                        const std::string& body = std::string()
                        );

        /**
         * Add a new constructor.
         * If 'name' and 'funcPtr' are not null, the should be the name and 
         * function pointer of a function with args as those to be added to 
         * the constructor.  This function will be called in the body of the 
         * constructor with the constructor's arguments.
         * Default initializers will be called prior to the function.
         */
        Func *addConstructor(const char *name = 0, void *funcPtr = 0);

        /**
         * Add a new constructor.
         * This function will be called in the body of the
         * constructor with the constructor's arguments.
         * Default initializers will be called prior to the function.
         */
        Func *addConstructor(const std::string& body);

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
         * Add a new static method to the class and returns it.  Static
         * methods have no implicit "this" parameter.
         * @return the new method.
         * @param returnType the method's return type
         * @param name the method name
         * @param body the function body
         */
        Func *addStaticMethod(Type *returnType, const std::string &name,
                              const std::string& body = std::string()
                              );
        
        /**
         * Returns a specialization of the type for the given parameters.
         */
        Type *getSpecialization(const std::vector<Type *> &params);
        
        /**
         * Mark the new type as "finished"
         */
        virtual void finish();
};
    
}} // namespace crack::ext

#endif
