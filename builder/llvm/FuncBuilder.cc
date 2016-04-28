// Copyright 2010-2012 Google Inc.
// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Arno Rehn <arno@arnorehn.de>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "FuncBuilder.h"

#include "BExtFuncDef.h"
#include "BFuncDef.h"
#include "VarDefs.h"
#include "model/ResultExpr.h"

#include <llvm/IR/LLVMContext.h>

using namespace llvm;
using namespace model;
using namespace std;
using namespace builder::mvll;

FuncBuilder::FuncBuilder(Context &context, FuncDef::Flags flags,
                         BTypeDef *returnType,
                         const string &name,
                         size_t argCount,
                         Function::LinkageTypes linkage,
                         void *addr
                         ) :
    context(context),
    returnType(returnType),
    funcDef(addr ? new BExtFuncDef(flags, name, argCount, addr) :
                   new BFuncDef(flags, name, argCount)
            ),
    linkage(linkage),
    argIndex(0) {

    funcDef->returnType = returnType;
    funcDef->ns = context.ns;
}

void FuncBuilder::finish(bool storeDef) {
    size_t argCount = funcDef->args.size();
    assert(argIndex == argCount);
    vector<Type *> llvmArgs(argCount +
                                  (receiverType ? 1 : 0)
                                  );

    // create the array of LLVM arguments
    int i = 0;
    if (receiverType)
        llvmArgs[i++] = receiverType->rep;
    for (vector<ArgDefPtr>::iterator iter = funcDef->args.begin();
         iter != funcDef->args.end();
         ++iter, ++i
         )
        llvmArgs[i] = BTypeDefPtr::rcast((*iter)->type)->rep;

    // register the function with LLVM
    Type *rawRetType = returnType->rep ? returnType->rep :
                                         Type::getVoidTy(getGlobalContext());
    FunctionType *llvmFuncType =
            FunctionType::get(rawRetType, llvmArgs,
                              funcDef->flags & FuncDef::variadic);
    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    if (!(funcDef->flags & FuncDef::abstract)) {
        ContextPtr defCtx = context.getParent()->getDefContext();
        Function *func = Function::Create(llvmFuncType,
                                          linkage,
                                          funcDef->getUniqueId(defCtx->ns.get()),
                                          builder.module
                                          );
        func->setCallingConv(llvm::CallingConv::C);
        if (!funcDef->symbolName.empty())
            func->setName(funcDef->symbolName);
        string fname = func->getName();

        // back-fill builder data and set arg names
        Function::arg_iterator llvmArg = func->arg_begin();
        vector<ArgDefPtr>::const_iterator crackArg =
                funcDef->args.begin();
        if (receiverType) {
            llvmArg->setName("this");

            // add the implementation to the "this" var
            receiver = context.ns->lookUp("this");
            assert(receiver &&
                   "missing 'this' variable in the context of a "
                   "function with a receiver"
                   );
            funcDef->thisArg = receiver;
            receiver->impl = new BArgVarDefImpl(llvmArg);
            ++llvmArg;
        }
        for (; llvmArg != func->arg_end(); ++llvmArg, ++crackArg) {
            llvmArg->setName((*crackArg)->name);

            // need the address of the value here because it is going
            // to be used in a "load" context.
            (*crackArg)->impl = new BArgVarDefImpl(llvmArg);
        }

        // create an implementation object to return the function
        // pointer
        funcDef->impl = new BConstDefImpl(funcDef.get(), func);

        funcDef->setRep(func, builder.moduleDef->repId);
    } else {
        // Create a null constant of the type of the function
        Constant *rep = Constant::getNullValue(llvmFuncType->getPointerTo());
        SPUG_CHECK(rep,
                   "Unable to create null value for type of " <<
                    *funcDef
                   );
        funcDef->setRep(rep, builder.moduleDef->repId);
    }

    // get or create the type registered for the function
    BTypeDefPtr crkFuncType = builder.getFuncType(context, funcDef.get(),
                                                  llvmFuncType
                                                  );
    funcDef->type = crkFuncType;

    if (storeDef)
        context.addDef(funcDef.get());
}

void FuncBuilder::addArg(const char *name, TypeDef *type) {
    assert(argIndex <= funcDef->args.size());
    funcDef->args[argIndex++] = new ArgDef(type, name);
}

void FuncBuilder::setArgs(const vector<ArgDefPtr> &args) {
    assert(argIndex == 0 && args.size() == funcDef->args.size());
    argIndex = args.size();
    funcDef->args = args;
}

