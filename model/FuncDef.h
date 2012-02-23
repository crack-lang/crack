// Copyright 2009 Google Inc.

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
        enum Flags {
            noFlags =0,  // so we can specify this
            method = 1,  // function is a method (has a receiver)
            virtualized = 2, // function is virtual
            forward = 4,  // this is a forward declaration
            variadic = 8,  // this is a variadic function
            reverse = 16,  // for a binary op, reverse receiver and first arg
            abstract = 32,  // This is an abstract (pure virtual function)
            explicitFlags = 256  // these flags were set by an annotation
        } flags;
        
        // flag to tell us what to do about function arguments during matching.
        enum Convert {
            noConvert = 0,
            adapt = 1,          // only convert adaptive arguments
            adaptSecondary = 2, // convert adaptives except for the first 
                                // argument
            convert = 3         // convert all arguments
        };
        
        typedef std::vector<ArgDefPtr> ArgVec;
        ArgVec args;
        ArgDefPtr thisArg;
        TypeDefPtr returnType;
        NamespacePtr ns;

        // for a virtual function, this is the path to the base class 
        // where the vtable is defined
        TypeDef::AncestorPath pathToFirstDeclaration;

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
        
        virtual bool hasInstSlot();
        virtual bool isStatic() const;        
        virtual std::string getDisplayName() const;
        
        /**
         * Returns true if the function is an override of a virtual method
         * in an ancestor class.
         */
        bool isVirtualOverride() const;

        /**
         * Returns the "receiver type."  For a non-virtual function, this 
         * is simply the type that the function was declared in.  For a 
         * virtual function, it is the type of the base class in which the 
         * function was first declared.
         */
        TypeDef *getReceiverType() const;
        
        /**
         * Returns the "this" type of the function.  This is always either 
         * the receiver type or a specialization of it.
         */
        TypeDef *getThisType() const;

        virtual bool isConstant();
        
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
