// Copyright 2010 Google Inc.

#ifndef _model_ModuleDef_h_
#define _model_ModuleDef_h_

#include <vector>
#include "Namespace.h"
#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);

SPUG_RCPTR(ModuleDef);

/**
 * A module.
 * The context of a module is the parent module.
 */
class ModuleDef : public VarDef, public Namespace {
    public:
        // the parent namespace.  This should be the root namespace where 
        // builtins are stored.
        NamespacePtr parent;

        ModuleDef(const std::string &name, Namespace *parent);

        /**
         * Close the module, executing it.
         */
        void close(Context &context);
        
        /**
         * Call the module destructor - cleans up all global variables.
         */
        virtual void callDestructor() = 0;
        
        virtual bool hasInstSlot();

        /**
         * Set namespace owner, and set our namespace name
         */
        virtual void setOwner(Namespace *o) {
            owner = o;
            canonicalName = o->getName()+"."+name;
        }

        virtual NamespacePtr getParent(unsigned index);
};

} // namespace model

#endif
