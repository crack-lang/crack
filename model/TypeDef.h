// Copyright 2009-2012 Google Inc.
// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <vector>
#include <map>
#include <spug/RCPtr.h>

#include "ArgDef.h"
#include "VarDef.h"
#include "Namespace.h"

namespace model {

SPUG_RCPTR(Context);
class Deserializer;
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
class Generic;
class Initializers;
SPUG_RCPTR(VarDef);

SPUG_RCPTR(TypeDef);

// a type.
class TypeDef : public VarDef, public Namespace {
    public:
        class TypeVecObj;
        typedef std::vector<TypeDefPtr> TypeVec;

        bool isAbstract(FuncDef *func);
        bool hasAbstractFuncs(OverloadDef *overload,
                              std::vector<FuncDefPtr> *abstractFuncs
                              );
        /** 
         * Returns the specialized name for the type.
         */
        std::string getSpecializedName(TypeVecObj *types, bool fullName);


    protected:
        TypeDef *findSpecialization(TypeVecObj *types);
        virtual void storeDef(VarDef *def);
        TypeDef *extractInstantiation(ModuleDef *module, TypeVecObj *types);

    public:

        SPUG_RCPTR(TypeVecObj);
        class TypeVecObj : public spug::RCBase, 
                           public std::vector<TypeDefPtr> {
            public:
                TypeVecObj() {}
                TypeVecObj(const TypeVec &types) : TypeVec(types) {}
        };
        
        // a vector of types that can be used as a key
        struct TypeVecObjKey {
            TypeVecObjPtr vec;
            TypeVecObjKey(TypeVecObj *vec) : vec(vec) {}
            bool operator <(const TypeVecObjKey &other) const {
                if (vec == other.vec)
                    return false;
                else {
                    size_t mySize = vec->size(), otherSize = other.vec->size();
                    for (int i = 0; i < std::max(mySize, otherSize);
                         ++i
                         ) {
                        if (i >= mySize)
                            // other is greater
                            return true;
                        else if (i >= otherSize)
                            // this is greater
                            return false;
                        
                        TypeDef *myVal = vec->operator [](i).get(),
                                *otherVal = other.vec->operator [](i).get();
                        if (myVal < otherVal)
                            return true;
                        else if (otherVal < myVal)
                            return false;
                    }
                    return false;
                }
            }
            
            bool equals(const TypeVecObj *other) const {
                return *vec == *other;
            }
        };
        typedef std::map<TypeVecObjKey, TypeDefPtr> SpecializationCache;
        
        // the parent vector.
        TypeVec parents;
        
        // defined for a generic type.  Stores the cache of all 
        // specializations for the type.
        SpecializationCache *generic;
        Generic *genericInfo;
        
        // defined for a generic instantiation
        TypeVec genericParms;
        TypeDef *templateType;
        
        // the number of bytes of padding required by the type after the 
        // instance variables (this exists so we can define extension types, 
        // whose instances consist entirely of padding with no instance 
        // variables)
        unsigned padding;

        // the default initializer expression (XXX I'm not sure that we want 
        // to keep this, for now it's expedient to be able to do variable 
        // initialization without the whole "oper new" business)
        ExprPtr defaultInitializer;
        
        // if true, the type is a pointer type (points to a structure)
        bool pointer;
        
        // if true, the type has a vtable (and is derived from vtable_base)
        bool hasVTable;
        
        // True for specializations of the primitive generic types (e.g. 
        // array[int], function[void, float]...)
        bool primitiveGenericSpec;
                
        // if the type is a meta type, "meta" is the type that it is the 
        // meta-type of.
        TypeDef *meta;

        // true if the type has been completely defined (so that we can 
        // determine whether to emit references or placeholders for instance 
        // variable references and assignments)
        bool complete;
        
        // true if the type has been forward declared but has not yet been
        // defined.
        bool forward;
        
        // if true, the initializers for the type have been emitted and it is 
        // now illegal to add instance variables.
        bool initializersEmitted;
        
        // if true, this is an abstract class (contains abstract methods)
        bool abstract;
        
        enum Flags {
            noFlags = 0,
            abstractClass = 1,
            explicitFlags = 256  // these flags were set by an annotation
        };
        
        // if true, the user has created an explicit "oper new" for the class, 
        // so don't generate them for any more of the init methods.
        bool gotExplicitOperNew;
        
        TypeDef(TypeDef *metaType, const std::string &name, 
                bool pointer = false
                ) :
            VarDef(metaType, name),
            Namespace(name),
            genericInfo(0),
            generic(0),
            templateType(0),
            padding(0),
            pointer(pointer),
            hasVTable(false),
            primitiveGenericSpec(false),
            meta(0),
            complete(false),
            forward(false),
            initializersEmitted(false),
            abstract(false),
            gotExplicitOperNew(false) {
        }
        
        ~TypeDef() { if (generic) delete generic; }

        /** required implementation of Namespace::getModule() */
        virtual ModuleDefPtr getModule();
        
        virtual bool isHiddenScope();

        virtual VarDef *asVarDef();

        /** required implementation of Namespace::getParent() */
        virtual NamespacePtr getParent(unsigned i);

        virtual NamespacePtr getNamespaceOwner();

        virtual bool hasGenerics() const;
        
        virtual FuncDefPtr getFuncDef(Context &context,
                                      std::vector<ExprPtr> &args,
                                      bool allowOverrides = false
                                      ) const;
        
        /**
         * Returns the 'oper new' function for the class matching the given 
         * args or null if none exists.
         * This is equivalent to getFuncDef() except it does not produce an 
         * error if the function is not found.
         */
        FuncDefPtr getOperNew(Context &context,
                              std::vector<ExprPtr> &args
                              ) const;
        
        /**
         * Overrides VarDef::hasInstSlot() to return false (nested classes 
         * don't need an instance slot).
         */
        virtual bool hasInstSlot() const;
        
        /**
         * Returns true if the function name is the name of a method that is 
         * implicitly final (non-virtual).
         */
        static bool isImplicitFinal(const std::string &name);
        
        /**
         * Adds the type and all of its ancestors to the ancestor list.  In 
         * the process, verifies that the type can safely be added to the 
         * existing set of ancestors.  Aborts with an error if verification 
         * fails.
         * 
         * The current verifications are:
         * - that the type is not a primitive class.
         * - that neither the type nor any of its ancestors is already in 
         *   'ancestors' (this check is ignored for the VTableBase class).
         */ 
        void addToAncestors(Context &context, TypeVec &ancestors);
        
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
         * Create a default oper init function with the specified argument 
         * list.  This requires that all base classes have either a default 
         * constructor or a constructor exactly matching args.
         */
        FuncDefPtr createOperInit(Context &classContext, const ArgVec &args);

        /**
         * Create the default initializer.
         */
        FuncDefPtr createDefaultInit(Context &classContext);
        
        /**
         * Create the default destructor for the type.
         */
        void createDefaultDestructor(Context &classContext);

        /**
         * Create a "new" function to wrap the specified "init" function.
         */
        void createNewFunc(Context &classContext, FuncDef *initFunc);
        
        /**
         * Return a function to convert to the specified type, if such a 
         * function exists.
         */
        virtual FuncDefPtr getConverter(Context &context, 
                                        const TypeDef &other);

        /**
         * Create a function to cast to the type (should only be used on a 
         * type with a vtable)
         * @param outerContext this should be the context that the type was 
         *        defined in (it's used to find the module scoped __die() 
         *        function).
         * @param throws if true, emit the single argument version of the 
         *        function which throws an exception.  Otherwise emit the two 
         *        argument version which returns a default value provided by 
         *        the caller.
         */
        void createCast(Context &outerContext, bool throws);

        /**
         * Returns true if the class has any abstract functions.
         * @param abstractFuncs if provided, this is a vector to fill with the 
         *        abstract functions we've discovered.
         * @param ancestor (for internal use) if provided, this the ancestor 
         *                 to start searching from.
         */
        bool gotAbstractFuncs(std::vector<FuncDefPtr> *abstractFuncs = 0,
                              TypeDef *ancestor = 0
                              );

        /**
         * Alias definitions in all of the base meta-types in our meta-type.  
         * This should be done immediately after setting the base classes.
         */
        void aliasBaseMetaTypes();

        /**
         * Fill in everything that's missing from the class.
         */
        void rectify(Context &classContext);
        
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
        virtual TypeDefPtr getSpecialization(Context &context, 
                                             TypeVecObj *types
                                             );

        /**
         * Set namespace owner, and set our namespace name.
         */
        virtual void setOwner(Namespace *o) {

            owner = o;

            // set the TypeDef namespace canonical name based on owner
            canonicalName = (!o->getNamespaceName().empty())?
                                o->getNamespaceName()+"."+name :
                                name;
            fullName.clear();
        }

        virtual bool isConstant();
        
        /**
         * Fills 'deps' with the set of dependent classes - dependent classes 
         * are nested classes that are derived from this class.
         * Base class version does nothing, derived classes must implement.
         */
        virtual void getDependents(std::vector<TypeDefPtr> &deps);

        /**
         * Serialize the type definition.
         */
        void serializeDef(Serializer &serializer) const;

        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
        
        virtual bool needsReceiver() const;

        virtual bool isSerializable() const;
        virtual void addDependenciesTo(ModuleDef *mod, Set &added) const;
        virtual void serializeExtern(Serializer &serializer) const;
        virtual void serializeAlias(Serializer &serializer,
                                    const std::string &alias
                                    ) const;
        virtual void serialize(Serializer &serializer, bool writeKind,
                               const Namespace *ns
                               ) const;

        /** Serialize a type declaration. Returns the new object id. */
        void serializeDecl(Serializer &serializer, ModuleDef *master);

        /** 
         * Deserialize a type declaration.  Returns the next object id.
         */
        static void deserializeDecl(Deserializer &deser);

        /** Deserialize a reference to a type object. */
        static TypeDefPtr deserializeRef(Deserializer &deser, 
                                         const char *name = 0
                                         );

        /** Deserialize a type object. */
        static TypeDefPtr deserializeTypeDef(Deserializer &deser,
                                             const char *name = 0
                                             );

        /**
         * Do whatever is needed to reconstruct the VTable at the end of 
         * loading the class.
         */
        virtual void materializeVTable(Context &context) {}
};

} // namespace model


#endif
