// Copyright 2009 Google Inc.

#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include "model/ResultExpr.h"
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(Expr);
class Namespace;
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDefImpl);

SPUG_RCPTR(VarDef);

// Variable definition.  All names in a context (including functions and 
// types) are derived from VarDef's.
class VarDef : public virtual spug::RCBase {

    protected:
        Namespace *owner;
        mutable std::string fullName; // a cache, built in getFullName

    public:
        TypeDefPtr type;
        std::string name;
        VarDefImplPtr impl;

        VarDef(TypeDef *type, const std::string &name);
        virtual ~VarDef();
        
        ResultExprPtr emitAssignment(Context &context, Expr *expr);
        
        /**
         * Returns true if the definition type requires a slot in the instance 
         * variable.
         */
        virtual bool hasInstSlot();
        
        /**
         * Returns true if the definition is class static.
         */
        virtual bool isStatic() const;

        /**
         * Returns the fully qualified name of the definition.
         */        
        std::string getFullName() const;
        
        /**
         * Set namespace owner
         */
        virtual void setOwner(Namespace *o) {
            owner = o;
            fullName.clear(); // must recache since owner changed
        }

        Namespace *getOwner(void) { return owner; }
        
        /**
         * Return true if the variable is unassignable.
         */
        virtual bool isConstant();

        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
        
        /**
         * Allow dumping from the debugger.
         */
        void dump() const;
};

inline std::ostream &operator <<(std::ostream &out, const VarDef &def) {
    def.dump(out);
    return out;
}

} // namespace model

#endif
