// Copyright 2009 Google Inc.

#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <vector>
#include <map>
#include <spug/RCPtr.h>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);
SPUG_RCPTR(Expr);
class Initializers;
SPUG_RCPTR(FuncDef);
SPUG_RCPTR(VarDef);

SPUG_RCPTR(TypeDef);

// a type.
class TypeDef : public VarDef {
    public:
        class TypeVec;

    protected:
        TypeDef *findSpecialization(TypeVec *types);

    public:

        SPUG_RCPTR(TypeVec);
        class TypeVec : public spug::RCBase, public std::vector<TypeDefPtr> {};
        
        // a vector of types that can be used as a key
        struct TypeVecKey {
            TypeVecPtr vec;
            TypeVecKey(TypeVec *vec) : vec(vec) {}
            bool operator <(const TypeVecKey &other) const {
                if (vec == other.vec)
                    return false;
                else
                    return *vec < *other.vec;
            }
            
            bool equals(const TypeVec *other) const {
                return *vec == *other;
            }
        };
        typedef std::map<TypeVecKey, TypeDefPtr> SpecializationCache;
        
        // defined for a generic type.  Stores the cache of all 
        // specializations for the type.
        SpecializationCache *generic;
        
        // the type's context - contains all of the method/attribute 
        // definitions for the type.
        ContextPtr context;
        
        // the default initializer expression (XXX I'm not sure that we want 
        // to keep this, for now it's expedient to be able to do variable 
        // initialization without the whole "oper new" business)
        ExprPtr defaultInitializer;
        
        // if true, the type is a pointer type (points to a structure)
        bool pointer;
        
        // if true, the type has a vtable (and is derived from vtable_base)
        bool hasVTable;
        
        // if the type is a meta type, "meta" is the type that it is the 
        // meta-type of.
        TypeDef *meta;
        
        // if true, the initializers for the type have been emitted and it is 
        // now illegal to add instance variables.
        bool initializersEmitted;
        
        TypeDef(TypeDef *metaType, const std::string &name, 
                bool pointer = false
                ) :
            VarDef(metaType, name),
            generic(0),
            pointer(pointer),
            hasVTable(false),
            meta(0),
            initializersEmitted(false) {
        }
        
        ~TypeDef() { if (generic) delete generic; }

        /**
         * Overrides VarDef::hasInstSlot() to return false (nested classes 
         * don't need an instance slot).
         */
        virtual bool hasInstSlot();
        
        /**
         * Returns true if the function name is the name of a method that is 
         * implicitly final (non-virtual).
         */
        static bool isImplicitFinal(const std::string &name);
        
        /**
         * Returns true if the type is derived from "other."
         */
        bool isDerivedFrom(const TypeDef *other) const;

        /** Emit a variable definition for the type. */
        VarDefPtr emitVarDef(Context &container, const std::string &name,
                             Expr *initializer
                             );
        
        /** 
         * Returns true if "other" satisfies the type - in other words, if 
         * "other" either equals "this" or is a subclass of "this".
         */
        bool matches(const TypeDef &other) const;
        
        /**
         * Create the default initializer.
         */
        FuncDefPtr createDefaultInit();
        
        /**
         * Create the default destructor for the type.
         */
        void createDefaultDestructor();

        /**
         * Create a "new" function to wrap the specified "init" function.
         */
        void createNewFunc(FuncDef *initFunc);
        
        /**
         * Return a function to convert to the specified type, if such a 
         * function exists.
         */
        virtual FuncDefPtr getConverter(const TypeDef &other);

        /**
         * Fill in everything that's missing from the class.
         */
        void rectify();
        
        /**
         * Returns true if 'type' is a parent.
         */
        bool isParent(TypeDef *type);
        
        struct AncestorReference {
            unsigned index;
            TypeDefPtr ancestor;
        };
        
        typedef std::vector<AncestorReference> AncestorPath;
        
        /**
         * Finds the path to the specified ancesetor.
         * Returns true if the ancestor was found, false if not.
         */
        bool getPathToAncestor(const TypeDef &ancestor, AncestorPath &path,
                               unsigned depth = 0
                               );
        
        /**
         * Emit all of the initializers for the type (base classes and fields) 
         * and amrk the type as initilalized so we can't go introducing new 
         * members.
         */
        void emitInitializers(Context &context, Initializers *inits);
        
        /**
         * Add the destructor cleanups for the type to the cleanup frame for 
         * the context.
         */
        void addDestructorCleanups(Context &context);

        /**
         * Returns a new specialization for the specified types, creating it 
         * if necessary.
         */
        virtual TypeDef *getSpecialization(Context &context, TypeVec *types);
        
        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;

};

} // namespace model


#endif
