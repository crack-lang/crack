// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BModuleDef.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include "spug/check.h"
#include "spug/stlutil.h"
#include "Consts.h"
#include "LLVMJitBuilder.h"

#include "model/EphemeralImportDef.h"

using namespace std;
using namespace model;
using namespace builder::mvll;
using namespace llvm;

BModuleDef::BModuleDef(const std::string &canonicalName,
                       model::Namespace *parent,
                       llvm::Module *rep0,
                       int repId
                       ) :
    ModuleDef(canonicalName, parent),
    cleanup(0),
    rep(rep0),
    repId(repId),
    importList() {
    SPUG_CHECK(repId != -1,
               "Module id of " << canonicalName << " initialized to -1."
               );
}

// We define this here so we don't have to includ Consts.h from BModuleDef.h
BModuleDef::~BModuleDef() {}

void BModuleDef::recordDependency(ModuleDef *other) {

    // quit if the dependency already is recorded
    for (VarDefVec::iterator depi = orderedForCache.begin();
         depi != orderedForCache.end();
         ++depi
         ) {
        EphemeralImportDef *modDef;
        if ((modDef = EphemeralImportDefPtr::rcast(*depi)) &&
             modDef->module == other
            )
            return;
    }

    EphemeralImportDefPtr def = new EphemeralImportDef(other);
    def->setOwner(this);
    orderedForCache.push_back(def.get());
}

void BModuleDef::clearRepFromConstants() {
    SPUG_FOR(vector<BStrConstPtr>, i, stringConstants) {
        (*i)->module = 0;
        (*i)->rep = 0;
    }
}

void BModuleDef::runMain(Builder &builder) {
    LLVMJitBuilder *llvmBuilder = LLVMJitBuilderPtr::cast(&builder);
    llvmBuilder->registerCleanups();
    llvm::ExecutionEngine *execEng = llvmBuilder->getExecEng();
    Function *func = rep->getFunction(name + ":main");
    SPUG_CHECK(func, "Function " << name << ":main" << " not defined.");
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    SPUG_CHECK(fptr, "no address for function " << string(func->getName()));
    fptr();

}
