// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BModuleDef.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include "spug/check.h"
#include "LLVMJitBuilder.h"

#include "model/EphemeralImportDef.h"

using namespace std;
using namespace model;
using namespace builder::mvll;
using namespace llvm;

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


void BModuleDef::runMain(Builder &builder) {
    llvm::ExecutionEngine *execEng =
        LLVMBuilderPtr::cast(&builder)->getExecEng();
    Function *func = rep->getFunction(name + ":main");
    SPUG_CHECK(func, "Function " << name << ":main" << " not defined.");
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    SPUG_CHECK(fptr, "no address for function " << string(func->getName()));
    fptr();

}
