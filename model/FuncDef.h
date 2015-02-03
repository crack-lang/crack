// Copyright 2009-2012 Google Inc.
// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_FuncDef_h_
#define _model_FuncDef_h_

#include <vector>
#include "TypeDef.h"
#include "VarDef.h"
#include "ArgDef.h"

namespace builder {
    class Builder;
}

namespace model {

//SPUG_RCPTR(ArgDef);
SPUG_RCPTR(Expr);
SPUG_RCPTR(Namespace);

SPUG_RCPTR(FuncDef);

class FuncDef : public VarDef {
    public:
        // When changing flags, be mindful of the fact that these values are 
        // persisted - changing flag values invalidates all meta-data in the 
        // cache.
        enum Flags {
            noFlags =0,  // so we can specify this
            method = 1,  // function is a method (has a receiver)
            virtualized = 2, // function is virtual
            forward = 4,  // this is a forward declaration
            variadic = 8,  // this is a variadic function
            reverse = 16,  // for a binary op, reverse receiver and first arg
            abstract = 32,  // This is an abstract (pure virtual function)
            builtin = 64,   // Indicates a builtin function that should not be 
                            // serialized.  Either the function is just an 
                            // inlined sequence of instructions or it is a 
                            // function provided by the executor (a member of 
                            // the .builtin module).
            shlib = 128,    // Function loaded directly from a shared library.
            explicitFlags = 32768 // these flags were set by an annotation
        } flags;
        
        // flag to tell us what to do about function arguments during matching.
        enum Convert {
            noConvert = 0,
            adapt = 1,          // only convert adaptive arguments
            adaptSecondary = 2, // convert adaptives except for the first 
                                // argument
            convert = 3         // convert all arguments
        };
        
        // The persistable information in a function.  This allows us to 
        // share the common function deserialization code with shared library 
        // imports.
        struct Spec {
            TypeDefPtr returnType;
            Flags flags;
            ArgVec args;
            TypeDefPtr receiverType;
            unsigned vtableSlot;
            
            Spec() : flags(noFlags), vtableSlot(0) {}
            
            void deserialize(Deserializer &deser);
        };
            
        
        typedef std::vector<ArgDefPtr> ArgVec;
        ArgVec args;
        ArgDefPtr thisArg;
        TypeDefPtr returnType;
        NamespacePtr ns;

        // For a method, this is the type of the receiver.  Null for a 
        // non-method.
        TypeDefPtr receiverType;

        // for a virtual function, this is the vtable slot position.
        unsigned vtableSlot;

        FuncDef(Flags flags, const std::string &name, size_t argCount);
        
        /**
         * Returns true if 'args' matches the types of the functions 
         * arguments.
         * 
         * @param newValues the set of converted values.  This is only
         *        constructed if 'convertFlag' is something other than 
         *        "noConvert".
         * @param convertFlag defines how to do argument conversions.
         */
        virtual bool matches(Context &context, 
                             const std::vector<ExprPtr> &vals,
                             std::vector<ExprPtr> &newVals,
                             Convert convertFlag
                             );
        
        /**
         * Returns true if the arg list matches the functions args.
         */
        bool matches(const ArgVec &args);
        
        /**
         * Returns true if 'args' matches the functions arg names and types.
         */
        bool matchesWithNames(const ArgVec &args);
        
        /**
         * Returns true if the function can be overriden.
         */
        bool isOverridable() const;
        
        /**
         * Returns the method's offset in the VTable.
         */
        unsigned int getVTableOffset() const;

        virtual bool hasInstSlot() const;
        virtual bool isStatic() const;        
        virtual std::string getDisplayName() const;
        virtual bool isUsableFrom(const Context &context) const;
        virtual bool needsReceiver() const;
        virtual bool isSerializable() const;
        
        /**
         * The unique name of the function is its full name and its argument 
         * types.  'ns' can be provided in cases where the function has not 
         * yet been added to a namespace.
         */
        std::string getUniqueId(Namespace *ns = 0) const;
        
        /**
         * Returns true if the function is an override of a virtual method
         * in an ancestor class.
         */
        bool isVirtualOverride() const;

        /**
         * Returns the "this" type of the function.  This is always either 
         * the receiver type or a specialization of it.
         */
        TypeDef *getThisType() const;

        virtual bool isConstant();

        /**
         * Returns true if the function is an alias when referenced from the 
         * given overload.
         */        
        bool isAliasIn(const OverloadDef &overload) const;
        
        /**
         * Returns the address of the underlying compiled function, suitable 
         * for use when directly calling a function from C.
         * It is fair to assume that this will trigger an assertion failure if 
         * called on an incomplete function.
         */
        virtual void *getFuncAddr(builder::Builder &builder) = 0;
        
        /**
         * Returns a const-folded form of the function if there is one, null 
         * if not.
         */
        ExprPtr foldConstants(const std::vector<ExprPtr> &args) const;
        
        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
        void display(std::ostream &out, const std::string &prefix = "") const;

        /** Allow us to write the argument list. */
        static void dump(std::ostream &out, const ArgVec &args);
        static void display(std::ostream &out, const ArgVec &args);

        virtual void addDependenciesTo(ModuleDef *mod, Set &added) const;
        
        void serializeArgs(Serializer &serializer) const;
        static ArgVec deserializeArgs(Deserializer &deser);
        void serializeCommon(Serializer &serializer) const;

        /** Serialize a function.  Should not be used on an alias. */
        void serialize(Serializer &serializer) const;

        /** Serialize an aliased function. */        
        void serializeAlias(Serializer &serializer) const;

        // This should never be called, functions are never directly 
        // serialized.
        virtual void serialize(Serializer &serializer, bool writeKind,
                               const Namespace *ns
                               ) const;
        static FuncDefPtr deserialize(Deserializer &deser, 
                                      const std::string &funcName
                                      );
};

inline FuncDef::Flags operator |(FuncDef::Flags a, FuncDef::Flags b) {
    return static_cast<FuncDef::Flags>(static_cast<int>(a) | 
                                       static_cast<int>(b));
}

inline std::ostream &operator <<(std::ostream &out, 
                                 const FuncDef::ArgVec &args
                                 ) {
    FuncDef::display(out, args);
    return out;
}

} // namespace model

#endif
