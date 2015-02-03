// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleMerger.h"

#include <iostream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace builder::mvll;
using namespace llvm;
using namespace std;

bool ModuleMerger::trace = false;
spug::Tracer ModuleMerger::tracer(
    "ModuleMerger",
    ModuleMerger::trace,
    "Merging of cyclic modules into a single module."
);

bool ModuleMerger::defined(GlobalValue *gval) {
    GlobalValue *targetGVal = 0;
    if (gval->hasName() && !gval->hasLocalLinkage())
        targetGVal = target->getNamedValue(gval->getName());

    // if it's already defined in the target, make sure it's in the value
    // map, too, then fall through to returning "true".
    if (targetGVal) {
        valueMap[gval] = targetGVal;
        return true;
    } else {
        return false;
    }
}

void ModuleMerger::copyGlobalAttrs(GlobalValue *dst, GlobalValue *src) {
    dst->copyAttributesFrom(src);
    dst->setAlignment(src->getAlignment());

    // copy the address if possible
    void *ptr;
    if (execEng && (ptr = execEng->getPointerToGlobalIfAvailable(src))) {
        if (trace)
            cerr << "copying pointer for " << src->getName().str() << endl;
        execEng->updateGlobalMapping(dst, ptr);
    }
}

void ModuleMerger::addGlobalDeclaration(GlobalVariable *gvar) {
    if (defined(gvar))
        return;

    if (trace)
        cerr << "Adding global definition for " << gvar->getName().str() <<
            " @" << gvar << endl;

    GlobalVariable *newGVar =
        new GlobalVariable(*target, gvar->getType()->getElementType(),
                           gvar->isConstant(),
                           gvar->getLinkage(),
                           0, // init
                           gvar->getName(),
                           0, // insert before
                           gvar->getThreadLocalMode(),
                           gvar->getType()->getAddressSpace()
                           );

    copyGlobalAttrs(newGVar, gvar);
    valueMap[gvar] = newGVar;
}

void ModuleMerger::addFunctionDeclaration(Function *func) {
    if (defined(func))
        return;

    if (trace)
        cerr << "adding function declaration for " << func->getName().str() <<
            " @" << func << endl;
    Function *newFunc = Function::Create(func->getFunctionType(),
                                         func->getLinkage(),
                                         func->getName(),
                                         target
                                         );
    copyGlobalAttrs(newFunc, func);
    valueMap[func] = newFunc;
}

void ModuleMerger::addInitializer(GlobalVariable *gvar) {
    GlobalVariable *dest = cast<GlobalVariable>(valueMap[gvar]);
    Constant *mapped = MapValue(gvar->getInitializer(), valueMap, RF_None);
    if (trace) {
        cerr << "Adding global initializer for " << gvar->getName().str() <<
            " @" << gvar << endl;
        cerr << "XXX var is:" << endl;
        dest->dump();
        cerr << "XXX type is:" << endl;
        dest->getType()->dump();
        PointerType *pt = dyn_cast<PointerType>(dest->getType());
        while (pt) {
            Type *et = pt->getElementType();
            cerr << "\n  XXX elem is:" << endl;
            et->dump();
            pt = dyn_cast<PointerType>(et);
        }
        cerr << "\nXXX initializer is:" << endl;
        mapped->dump();
    }

    dest->setInitializer(mapped);
}

void ModuleMerger::addFunctionBody(Function *func) {
    string errorMsg;

    if (trace)
        cerr << "adding function body for " << func->getName().str() <<
            " @" << func << endl;

    if (func->isDeclaration()) {
        if (!func->isMaterializable())
            return;
        else
            func->Materialize(&errorMsg);
    }

    Function *dest = cast<Function>(valueMap[func]);

    // We should already have an argument list, copy the names and add the
    // arguments to the value map.
    for (Function::arg_iterator arg = func->arg_begin(),
          destArg = dest->arg_begin();
         arg != func->arg_end();
         ++arg, ++destArg
         ) {
        destArg->setName(arg->getName());
        valueMap[arg] = destArg;
    }

    SmallVector<ReturnInst *, 8> returns;
    CloneFunctionInto(dest, func, valueMap, false, returns, "");
}

ModuleMerger::ModuleMerger(const string &name, int mergedModuleRepId,
                           ExecutionEngine *execEng
                           ) :
    execEng(execEng),
    mergedModuleRepId(mergedModuleRepId) {

    LLVMContext &lctx = getGlobalContext();
    target = new llvm::Module(name, lctx);
}

void ModuleMerger::merge(Module *module) {
    if (trace)
        cerr << ">>> running merge on " << module->getModuleIdentifier() <<
            endl;
    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i
         )
        addGlobalDeclaration(i);

    for (Module::iterator i = module->begin(); i != module->end(); ++i)
        addFunctionDeclaration(i);

    // update the global initializers.
    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i
         ) {
        if (i->hasInitializer())
            addInitializer(i);
    }

    // copy the function bodies.
    for (Module::iterator i = module->begin(); i != module->end(); ++i)
        addFunctionBody(i);
    if (trace)
        cerr << ">>> done with merge on " << module->getModuleIdentifier() <<
            endl;
}
