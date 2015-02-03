// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_BJitModuleDef_h_
#define _builder_llvm_BJitModuleDef_h_

#include "BModuleDef.h"

namespace builder { namespace mvll {

SPUG_RCPTR(BJitModuleDef);

/**
 * Specialized Module object for the jit builder - includes an "owner", owned 
 * modules are closed as part of the owner's closing.  A module with an owner 
 * is a "sub-module" of the owner.
 * 
 * This mechanism is necessary for ephemeral modules, which can reference 
 * incomplete types from the modules they are spawned from.
 */
class BJitModuleDef : public BModuleDef {

    public:
        typedef std::vector<BJitModuleDefPtr> ModuleVec;
        
        // Closer stores all of the infromation necessary to close a 
        // sub-module.
        SPUG_RCPTR(Closer);
        class Closer : public spug::RCBase {
            private:
                model::ContextPtr context;
                BJitModuleDefPtr moduleDef;
                LLVMJitBuilderPtr builder;
            
            public:
                Closer(model::Context *context, 
                       BJitModuleDef *moduleDef, 
                       LLVMJitBuilder *builder
                       ) :
                    context(context),
                    moduleDef(moduleDef),
                    builder(builder) {
                }
                
                void close() {
                    moduleDef->recursiveClose(*context, builder.get());
                }
        };
        
        // modules to close during our own closing.
        std::vector<CloserPtr> subModules;

        BJitModuleDefPtr owner;

        BJitModuleDef(const std::string &canonicalName,
                      model::Namespace *parent,
                      llvm::Module *rep0,
                      int repId,
                      BJitModuleDef *owner
                      ) :
            BModuleDef(canonicalName, parent, rep0, repId),
            owner(owner) {
        }

        // Recursively close all of the modules and flatten the tree into 
        // 'modules'.
        void recursiveClose(model::Context &context, LLVMJitBuilder *builder) {
            // closing for real - close all of my sub-modules
            for (int i = 0; i < subModules.size(); ++i)
                subModules[i]->close();
            
            // and do the real close
            builder->innerCloseModule(context, this);
        }
        
        void closeOrDefer(model::Context &context, LLVMJitBuilder *builder) {
            // if we've got an owner, defer our close until his close
            if (owner) {
                CloserPtr closer = new Closer(&context, this, builder);
                owner->subModules.push_back(closer);
            } else {
                recursiveClose(context, builder);
            }
        }
};

}} // end namespace builder::vmll

#endif
