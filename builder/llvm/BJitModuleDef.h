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
                
                void close(ModuleVec &modules) {
                    moduleDef->recursiveClose(modules, *context, builder.get());
                }
        };
        
        // modules to close during our own closing.
        std::vector<CloserPtr> subModules;

        BJitModuleDefPtr owner;

        BJitModuleDef(const std::string &canonicalName,
                      model::Namespace *parent,
                      llvm::Module *rep0,
                      BJitModuleDef *owner
                      ) :
            BModuleDef(canonicalName, parent, rep0),
            owner(owner) {
        }

        // Recursively close all of the modules and flatten the tree into 
        // 'modules'.
        void recursiveClose(ModuleVec &modules, model::Context &context, 
                            LLVMJitBuilder *builder
                            ) {
            // closing for real - close all of my sub-modules
            for (int i = 0; i < subModules.size(); ++i)
                subModules[i]->close(modules);
            
            // and do the real close
            builder->innerCloseModule(context, this);
            modules.push_back(this);
        }
        
        void closeOrDefer(model::Context &context, LLVMJitBuilder *builder) {
            // if we've got an owner, defer our close until his close
            if (owner) {
                CloserPtr closer = new Closer(&context, this, builder);
                owner->subModules.push_back(closer);
            } else {
                ModuleVec modules;
                recursiveClose(modules, context, builder);
                builder->mergeAndRegister(modules);
            }
        }
        
        /** 
         * Does a recursive close on the owned modules but not on this one.  
         * This is to allow us to close dependencies on a module that is 
         * itself cached and doesn't require a close.
         */
        void closeOwned(LLVMJitBuilder *builder) {
            // don't do anything if we don't have any sub-modules.
            if (!subModules.size())
                return;

            ModuleVec modules;
            
            // close the subs
            for (int i = 0; i < subModules.size(); ++i)
                subModules[i]->close(modules);
            
            // add this module and then merge the set
            modules.push_back(this);
            builder->mergeAndRegister(modules);
        }
        
        // Override onDeserialized so we can close owned modules imported 
        // from a serialized module.
        virtual void onDeserialized(model::Context &context) {
            BModuleDef::onDeserialized(context);
            LLVMJitBuilder *builder = 
                &dynamic_cast<builder::mvll::LLVMJitBuilder &>(
                    context.builder
                );
            closeOwned(builder);
        }
};

}} // end namespace builder::vmll

#endif
