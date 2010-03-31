// Copyright 2009 Google Inc.

#ifndef _model_OverloadDef_h_
#define _model_OverloadDef_h_

#include <list>

#include "FuncDef.h"

namespace model {

SPUG_RCPTR(Context);
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);

SPUG_RCPTR(OverloadDef);

/** An overloaded function. */
class OverloadDef : public VarDef {
    public:
        typedef std::list<FuncDefPtr> FuncList;
        struct Parent {
            mutable OverloadDefPtr overload;
            ContextPtr context;
            
            Parent(Context *context) : context(context) {}
            OverloadDef *getOverload(const OverloadDef *owner) const;
        };
        typedef std::vector<Parent> ParentVec;

    private:
        FuncList funcs;
        ParentVec parents;

        /**
         * Sets the impl and the type object from the function.  To be called 
         * for the first function added as a hack to keep function-as-objects 
         * working.
         */
        void setImpl(FuncDef *func);
        
        /**
         * Flatten the overload definition into a single list where each 
         * signature is represented only once.
         * 
         * @param funcs output list of functions to add to.
         */
        void flatten(FuncList &funcs) const;

    public:

        OverloadDef(const std::string &name) :
            // XXX need function types, but they'll probably be assigned after 
            // the fact.
            VarDef(0, name) {
        }
       
        /**
         * Returns the overload matching the given args, null if one does not 
         * exist
         * 
         * @param context the context used in case conversion expressions need 
         *        to be constructed.
         * @param args the argument list.  This may be modified if 'convert' 
         *        is true and there are conversions to be applied.
         * @param convert if true, attempt to convert arguments.
         */
        FuncDef *getMatch(Context &context, std::vector<ExprPtr> &args,
                          bool convert
                          );
 
        /**
         * Returns the overload matching the given args. This does the full 
         * resolution pass, first attempting a resolution without any 
         * conversions and then applying conversions.  As such, it will modify
         * "args" if there are conversions to be applied.
         */
        FuncDef *getMatch(Context &context, std::vector<ExprPtr> &args) {
            FuncDef *result = getMatch(context, args, false);
            if (!result)
                result = getMatch(context, args, true);
            return result;
        }
        
        /**
         * Returns the overload with the matching signature if there is one, 
         * NULL if not.
         */
        FuncDef *getSigMatch(const FuncDef::ArgVec &args);
        
        /** 
         * Returns true if the overload includeds a signature for the 
         * specified argument list.
         */
        bool matches(const FuncDef::ArgVec &args) {
            return getSigMatch(args) ? true : false;
        }
        
        /**
         * Adds the function to the overload set.  The function will be 
         * inserted after all other overloads from the context but before 
         * overloads from the parent context.
         */
        void addFunc(FuncDef *func);
        
        /**
         * Adds the parent context to the overload set.  Lookups will be 
         * delgated to parents in the order provided.
         */
        void addParent(Context *context);
        
        /**
         * Iterate over the funcs local to this context - do not iterate over 
         * the functions in the parent overloads.
         */
        /** @{ */
        FuncList::iterator beginTopFuncs() { return funcs.begin(); }
        FuncList::iterator endTopFuncs() { return funcs.end(); }
        /** @} */
        
        bool hasInstSlot();
        
        /**
         * Returns true if the overload consists of only one function.
         */
        bool isSingleFunction() const;
        
        /**
         * Make sure we have an implementation object, create one if we don't.
         */
        void createImpl();

        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
};

} // namespace model

#endif
