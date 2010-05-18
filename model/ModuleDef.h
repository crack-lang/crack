// Copyright 2010 Google Inc.

#ifndef _model_ModuleDef_h_
#define _model_ModuleDef_h_

#include <vector>
#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);

SPUG_RCPTR(ModuleDef);

/**
 * A module.
 * The context of a module is the parent module.
 */
class ModuleDef : public VarDef {
    public:
        // the module's context (VarDef::context is the parent module context, 
        // but not a parent of moduleContext - module scope does not 
        // delegate to parent scope).
        ContextPtr moduleContext;

        ModuleDef(const std::string &name, Context *moduleContext);

        /**
         * Resolve a symbol from the module.
         */
        VarDefPtr lookUp(const std::string &name);
        
        /**
         * Close the module, executing it.
         */
        void close(bool optimize);
        
        /**
         * Call the module destructor - cleans up all global variables.
         */
        virtual void callDestructor() = 0;
        
        virtual bool hasInstSlot();
};

} // namespace model

#endif
