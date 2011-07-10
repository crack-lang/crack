// Copyright 2011 Google Inc

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
                      BJitModuleDef *owner
                      ) :
            BModuleDef(canonicalName, parent, rep0),
            owner(owner) {
        }

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
                std::cout << "deferring " << name << " to close of " <<
                    owner->name << std::endl;
                owner->subModules.push_back(closer);
            } else {
                recursiveClose(context, builder);
            }
        }
};

}} // end namespace builder::vmll

#endif
