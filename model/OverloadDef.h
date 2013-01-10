// Copyright 2009-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_OverloadDef_h_
#define _model_OverloadDef_h_

#include <list>

#include "FuncDef.h"

namespace model {

class Context;
class Deserializer;
SPUG_RCPTR(Expr);
SPUG_RCPTR(Namespace);
class Serializer;
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(OverloadDef);

/** 
 * An overloaded function. 
 * Overloads are not currently created by the builder, as they are presumed to 
 * be purely administrative entities with no representation in the backend.
 */
class OverloadDef : public VarDef {
    public:
        typedef std::list<FuncDefPtr> FuncList;
        typedef std::vector<OverloadDefPtr> ParentVec;

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
         * @param args the argument list.  This will be modified if 'convert' 
         *        is not "noConvert".
         * @param convertFlag defines whether and when conversions are done.
         * @param allowOverrides by default, this function will ignore 
         *        overrides of a virtual function in considering a match - 
         *        this is to provide the correct method resolution order.  
         *        Setting this flag to true causes it to return the first 
         *        matching function, regardless of whether it is an override.
         */
        FuncDef *getMatch(Context &context, std::vector<ExprPtr> &args,
                          FuncDef::Convert convertFlag,
                          bool allowOverrides
                          );
 
        /**
         * Returns the overload matching the given args. This does the full 
         * resolution pass, first attempting a resolution without any 
         * conversions and then applying conversions.  As such, it will modify
         * "args" if there are conversions to be applied.
         */
        FuncDef *getMatch(Context &context, std::vector<ExprPtr> &args,
                          bool allowOverrides
                          );
        
        /**
         * Returns the overload with the matching signature if there is one, 
         * NULL if not.
         * @param matchNames if true, require that the signator match the 
         *        names of the arg list.  If false, only require that the 
         *        types match.
         */
        FuncDef *getSigMatch(const ArgVec &args, 
                             bool matchNames = false);
        
        /**
         * Returns the overload with no arguments.  If 'acceptAlias' is false, 
         * it must belong to the same context as the overload in which it is 
         * defined.
         */
        FuncDef *getNoArgMatch(bool acceptAlias);
        
        /** 
         * Returns true if the overload includeds a signature for the 
         * specified argument list.
         */
        bool matches(const ArgVec &args) {
            return getSigMatch(args) ? true : false;
        }
        
        /**
         * Adds the function to the overload set.  The function will be 
         * inserted after all other overloads from the context but before 
         * overloads from the parent context.
         */
        void addFunc(FuncDef *func);
        
        /**
         * Adds the parent overload.  Lookups will be delgated to parents in 
         * the order provided.
         */
        void addParent(OverloadDef *paren);
        
        /** Returns true if 'parent' is a parent of the overload. */
        bool hasParent(OverloadDef *parent);
        
        /**
         * Create an alias overload - an alias overload is comprised of all 
         * of the functions in the overload and its ancestors flattened.  It 
         * has no ancestors of its own.
         */
        OverloadDefPtr createAlias();
        
        /**
         * Iterate over the funcs local to this context - do not iterate over 
         * the functions in the parent overloads.
         */
        /** @{ */
        FuncList::iterator beginTopFuncs() { return funcs.begin(); }
        FuncList::iterator endTopFuncs() { return funcs.end(); }
        /** @} */
        
        bool hasInstSlot();
        bool isStatic() const;
        virtual bool isSerializable(const Namespace *ns) const;
        
        /**
         * Returns true if the overload consists of only one function.
         */
        bool isSingleFunction() const;
        
        /**
         * Make sure we have an implementation object, create one if we don't.
         */
        void createImpl();
        
        virtual bool isConstant();

        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
        void display(std::ostream &out, const std::string &prefix = "") const;
        
        virtual void addDependenciesTo(const ModuleDef *mod, 
                                       ModuleDefMap &deps
                                       ) const;
        
        /**
         * Returns true if the overload includes any non-builtin functions 
         * (this is useful for determining if it needs to be serialized).
         */
        bool hasSerializableFuncs(const Namespace *ns) const;

        virtual void serialize(Serializer &serializer, bool writeKind,
                               const Namespace *ns
                               ) const;
        
        static OverloadDefPtr deserialize(Deserializer &deser);
};

} // namespace model

#endif
