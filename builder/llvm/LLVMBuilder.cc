// Copyright 2010-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LLVMBuilder.h"

#include "ArrayTypeDef.h"
#include "BBuilderContextData.h"
#include "BBranchPoint.h"
#include "BCleanupFrame.h"
#include "BFieldRef.h"
#include "BFuncDef.h"
#include "BFuncPtr.h"
#include "BModuleDef.h"
#include "BResultExpr.h"
#include "BTypeDef.h"
#include "Cacher.h"
#include "Consts.h"
#include "ExceptionCleanupExpr.h"
#include "FuncBuilder.h"
#include "FunctionTypeDef.h"
#include "Incompletes.h"
#include "LLVMValueExpr.h"
#include "Ops.h"
#include "PlaceholderInstruction.h"
#include "Utils.h"
#include "VarDefs.h"
#include "VTableBuilder.h"
#include "DebugInfo.h"

#include "parser/Parser.h"
#include "parser/ParseError.h"

#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/Dwarf.h>
#include <llvm/IR/Intrinsics.h>

#include <spug/Exception.h>
#include <spug/StringFmt.h>

#include <model/AllocExpr.h>
#include <model/AssignExpr.h>
#include <model/CompositeNamespace.h>
#include <model/Construct.h>
#include <model/GlobalNamespace.h>
#include <model/InstVarDef.h>
#include <model/LocalNamespace.h>
#include <model/NullConst.h>
#include <model/OverloadDef.h>
#include <model/StubDef.h>
#include <model/TernaryExpr.h>
#include "util/CacheFiles.h"

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;
using namespace crack::util;

typedef model::FuncCall::ExprVec ExprVec;

int LLVMBuilder::argc = 1;

namespace {
    char *tempArgv[] = {const_cast<char *>("undefined")};
}
char **LLVMBuilder::argv = tempArgv;

extern "C" {

    char **__getArgv() {
        return LLVMBuilder::argv;
    }

    int __getArgc() {
        return LLVMBuilder::argc;
    }

}


namespace {

    // emit all cleanups from this context to outerContext (non-inclusive)
    void emitCleanupsTo(Context &context, Context &outerContext) {

        // unless we've reached our stop, emit for all parent contexts
        if (&outerContext != &context) {

            // close all cleanups in thie context
            closeAllCleanupsStatic(context);
            emitCleanupsTo(*context.parent, outerContext);
        }
    }

    BasicBlock *emitUnwindFrameCleanups(BCleanupFrame *frame,
                                        BasicBlock *next
                                        ) {
        if (frame->parent)
            next = emitUnwindFrameCleanups(
                BCleanupFramePtr::rcast(frame->parent),
                next
            );
        return frame->emitUnwindCleanups(next);
    }

    Value *getExceptionObjectField(Context &context, IRBuilder<> &builder,
                                   int index
                                   ) {
        VarDefPtr exStr = context.ns->lookUp(":exStruct");
        BHeapVarDefImplPtr exStrImpl = BHeapVarDefImplPtr::arcast(exStr->impl);
        return builder.CreateLoad(
            builder.CreateConstGEP2_32(exStrImpl->rep, 0, index)
        );
    }


    Value *getExceptionObjectValue(Context &context, IRBuilder<> &builder) {
        return getExceptionObjectField(context, builder, 0);
    }

    Value *getExceptionSelectorValue(Context &context, IRBuilder<> &builder) {
        return getExceptionObjectField(context, builder, 1);
    }

    BasicBlock *emitUnwindCleanups(Context &context, Context &outerContext,
                                   BasicBlock *finalBlock
                                   ) {

        // unless we've reached our stop, emit for all parent contexts
        if (&outerContext != &context) {

            // emit the cleanups in the parent block
            BasicBlock *next =
                emitUnwindCleanups(*context.parent, outerContext, finalBlock);

            // close all cleanups in thie context
            BCleanupFrame *frame =
                BCleanupFramePtr::rcast(context.cleanupFrame);
            return emitUnwindFrameCleanups(frame, next);
        } else {
            return finalBlock;
        }
    }

    // Prepares a function "func" to act as an override for "override"
    unsigned wrapOverride(TypeDef *classType, BFuncDef *overriden,
                          FuncBuilder &funcBuilder
                          ) {
        // find the path to the overriden's class
        BTypeDef *overridenClass = BTypeDefPtr::acast(overriden->getOwner());

        // the type of the receiver is that of its first declaration
        funcBuilder.setReceiverType(
            BTypeDefPtr::arcast(overriden->receiverType)
        );

        funcBuilder.funcDef->vtableSlot = overriden->vtableSlot;
        return overriden->vtableSlot;
    }

    void finishClassType(Context &context, BTypeDef *classType) {
        // for the kinds of things we're about to do, we need a global block
        // for functions to restore to, and for that we need a function and
        // module.
        LLVMContext &lctx = getGlobalContext();
        LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
        // builder.module should already exist from .builtin module
        assert(builder.module);
        vector<Type *> argTypes;
        FunctionType *voidFuncNoArgs =
            FunctionType::get(Type::getVoidTy(lctx), argTypes, false);
        Function *func = Function::Create(voidFuncNoArgs,
                                          Function::ExternalLinkage,
                                          "__builtin_init__",
                                          builder.module
                                          );
        func->setCallingConv(llvm::CallingConv::C);
        builder.builder.SetInsertPoint(BasicBlock::Create(lctx,
                                                          "__builtin_init__",
                                                          builder.func
                                                          )
                                       );

        // add "Class"
        int lineNum = __LINE__ + 1;
        string temp("    byteptr name;\n"
                    "    uint numBases;\n"
                    "    array[Class] bases;\n"
                    "    array[intz] offsets;\n"
                    "    bool isSubclass(Class other) {\n"
                    "        if (this is other)\n"
                    "            return (1==1);\n"
                    "        uint i;\n"
                    "        while (i < numBases) {\n"
                    "            if (bases[i].isSubclass(other))\n"
                    "                return (1==1);\n"
                    "            i = i + uint(1);\n"
                    "        }\n"
                    "        return (1==0);\n"
                    "    }\n"
                    "    intz getInstOffset(Class other) {\n"
                    "        if (this is other)\n"
                    "            return 0;\n"
                    "        uint i;\n"
                    "        for (int i = 0; i < numBases; ++i) {\n"
                    "            off := bases[i].getInstOffset(other);\n"
                    "            if (off >= 0)\n"
                    "                return offsets[i] + off;\n"
                    "        }\n"
                    "        return -1;\n"
                    "    }\n"
                    "}\n"
                    );

        // create the class context
        ContextPtr classCtx =
            context.createSubContext(Context::instance, classType);

        CompositeNamespacePtr ns = new CompositeNamespace(classType,
                                                          context.ns.get()
                                                          );
        ContextPtr lexicalContext =
            classCtx->createSubContext(Context::composite, ns.get());
        BBuilderContextData *bdata =
            BBuilderContextData::get(lexicalContext.get());

        istringstream src(temp);
        try {
            parser::Toker toker(src, "<builtin>", lineNum);
            parser::Parser p(toker, lexicalContext.get());
            p.parseClassBody();
        } catch (parser::ParseError &ex) {
            std::cerr << ex << endl;
            assert(false);
        }

        // let the "end class" emitter handle the rest of this.
        context.builder.emitEndClass(*classCtx);

        // close off the block.
        builder.builder.CreateRetVoid();
    }

    void fixMeta(Context &context, BTypeDef *type) {
        SPUG_CHECK(type->parents.empty(),
                   "Meta-class fixup can only be applied to types without "
                    "a base class (trying to fixMeta on " <<
                    type->getFullName() << ")"
                   );
        BTypeDefPtr metaType;
        BGlobalVarDefImplPtr classImpl;
        type->type = metaType = createMetaClass(context, type->name);
        context.construct->registerDef(metaType.get());
        metaType->meta = type;
        createClassImpl(context, BTypeDefPtr::acast(type));
        type->createEmptyOffsetsInitializer(context);
    }

    void addExplicitTruncate(Context &context, BTypeDef *sourceType,
                             BTypeDef *targetType
                             ) {
        FuncDefPtr func =
            new GeneralOpDef<TruncOpCall>(targetType, FuncDef::noFlags,
                                          "oper new",
                                          1
                                          );
        func->args[0] = new ArgDef(sourceType, "val");
        context.addDef(func.get(), targetType);
    }

    void addNopNew(Context &context, BTypeDef *type) {
        FuncDefPtr func =
            new GeneralOpDef<NoOpCall>(type, FuncDef::noFlags,
                                       "oper new",
                                       1
                                       );
        func->args[0] = new ArgDef(type, "val");
        context.addDef(func.get(), type);
    }

    template <typename opType>
    void addExplicitFPTruncate(Context &context, BTypeDef *sourceType,
                               BTypeDef *targetType
                               ) {
        FuncDefPtr func =
            new GeneralOpDef<opType>(targetType, FuncDef::noFlags,
                                          "oper new",
                                          1
                                          );
        func->args[0] = new ArgDef(sourceType, "val");
        context.addDef(func.get(), targetType);
    }

    BTypeDef *createIntPrimType(Context &context, Type *llvmType,
                                const char *name
                                ) {
        BTypeDefPtr btype = new BTypeDef(context.construct->classType.get(),
                                         name,
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createIntConst(context, 0, btype.get());
        context.addDef(new BoolOpDef(context.construct->boolType.get(),
                                     "oper to .builtin.bool"
                                     ),
                       btype.get()
                       );

        // if you remove this, for the love of god, change the return type so
        // we don't leak the pointer.
        context.addDef(btype.get());

        // construct an empty offsets variable initializer.
        btype->createEmptyOffsetsInitializer(context);

        return btype.get();
    }

    BTypeDef *createFloatPrimType(Context &context, Type *llvmType,
                                  const char *name
                                  ) {
        BTypeDefPtr btype = new BTypeDef(context.construct->classType.get(),
                                         name,
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createFloatConst(context, 0.0, btype.get());
        context.addDef(new FBoolOpDef(context.construct->boolType.get(),
                                      "oper to .builtin.bool"
                                      ),
                       btype.get()
                       );

        // if you remove this, for the love of god, change the return type so
        // we don't leak the pointer.
        context.addDef(btype.get());

        // construct an empty offsets variable initializer.
        btype->createEmptyOffsetsInitializer(context);

        return btype.get();
    }

    BBuilderContextData::CatchDataPtr getContextCatchData(Context &context) {
        ContextPtr catchContext = context.getCatch();
        if (!catchContext->toplevel) {
            BBuilderContextDataPtr bdata =
                BBuilderContextDataPtr::rcast(catchContext->builderData);
            return bdata->getCatchData();
        } else {
            return 0;
        }
    }
} // anon namespace

void LLVMBuilder::emitFunctionCleanups(Context &context) {

    // close all cleanups in this context.
    closeAllCleanupsStatic(context);

    // recurse up through the parents.
    if (!context.toplevel && context.parent->scope == Context::local)
        emitFunctionCleanups(*context.parent);
}

void LLVMBuilder::createLLVMModule(const string &name) {
    LLVMContext &lctx = getGlobalContext();
    module = new llvm::Module(name, lctx);
}

void LLVMBuilder::finishModule(Context &context, ModuleDef *moduleDef) {
    LLVMContext &lctx = getGlobalContext();

    // get the "initialized" flag
    GlobalVariable *moduleInit = module->getNamedGlobal(moduleDef->name +
                                                         ":initialized"
                                                        );

    // if there was a top-level throw, we could already have a terminator.
    // Generate the code to set the init flag and a return instruction if not.
    if (!builder.GetInsertBlock()->getTerminator()) {

        if (moduleDef->fromExtension) {
            // if this is an extension, we create a runtime initialize call
            // this allows the extension to initialize, but also ensures the
            // extension will be linked since ld requires at least one call into it
            // XXX real mangle? see Construct::loadSharedLib
            string name = moduleDef->getFullName();
            for (int i=0; i < name.size(); ++i) {
                if (name[i] == '.')
                    name[i] = '_';
            }
            Constant *initFunc =
                module->getOrInsertFunction(name + "_rinit",
                                            Type::getVoidTy(getGlobalContext()),
                                            NULL
                                            );
            Function *f = llvm::cast<llvm::Function>(initFunc);
            builder.CreateCall(f);
        }

        // at the end of the code for the module, set the "initialized" flag.
        builder.CreateStore(Constant::getIntegerValue(
                                Type::getInt1Ty(lctx),APInt(1,1,false)
                             ),
                            moduleInit
                            );

        builder.CreateRetVoid();
    }

    // since the cleanups have to be emitted against the module context, clear
    // the unwind blocks so we generate them for the del function.
    clearCachedCleanups(context);

    // emit the cleanup function for this module
    // we will emit calls to these (for all modules) during run() in the finalir
    string cleanupFuncName = moduleDef->name + ":cleanup";
    llvm::Constant *c =
        module->getOrInsertFunction(cleanupFuncName,
                                    Type::getVoidTy(lctx), NULL);
    Function *initFunc = func;
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    // create a new exStruct variable for this context in the standard start
    // block.
    createFuncStartBlocks(cleanupFuncName);
    createSpecialVar(context.ns.get(), getExStructType(), ":exStruct");

    // if the initialization flag is not set, branch to return
    BasicBlock *cleanupBlock = BasicBlock::Create(lctx, ":cleanup", func),
               *retBlock = BasicBlock::Create(lctx, "done", func);
    Value *initVal = builder.CreateLoad(moduleInit);
    builder.CreateCondBr(initVal, cleanupBlock, retBlock);

    builder.SetInsertPoint(cleanupBlock);

    closeAllCleanupsStatic(context);
    builder.CreateBr(retBlock);

    builder.SetInsertPoint(retBlock);
    builder.CreateRetVoid();

    // restore the main function.
    func = initFunc;
}


void LLVMBuilder::initializeMethodInfo(Context &context, FuncDef::Flags flags,
                                       FuncDef *existing,
                                       BTypeDef *&classType,
                                       FuncBuilder &funcBuilder
                                       ) {
    if (!classType) {
        ContextPtr classCtx = context.getClassContext();
        assert(classCtx && "method is not nested in a class context.");
        classType = BTypeDefPtr::arcast(classCtx->ns);
    }

    // create the vtable slot for a virtual function
    if (flags & FuncDef::virtualized) {
        // use the original's slot if this is an override.
        if (existing) {
            funcBuilder.funcDef->vtableSlot =
                wrapOverride(classType, BFuncDefPtr::acast(existing),
                             funcBuilder
                             );
        } else {
            funcBuilder.funcDef->vtableSlot = classType->nextVTableSlot++;
            funcBuilder.setReceiverType(classType);
        }
    } else {
        funcBuilder.setReceiverType(classType);
    }
}

// this is confusingly named - it returns a block suitable for use in the
// "unwind" clause of an invoke, which is actually a landing pad (elsewhere
// we're using "unwind" to refer to the "resume" block)
BasicBlock *LLVMBuilder::getUnwindBlock(Context &context) {
    // get the catch-level context and unwind block
    ContextPtr outerContext = context.getCatch();
    BBranchpointPtr bpos =
        BBranchpointPtr::rcast(outerContext->getCatchBranchpoint());

    BasicBlock *final;
    BBuilderContextData::CatchDataPtr cdata;
    if (bpos) {
        final = bpos->block;

        // get the "catch data" for the context so we can correctly store
        // placeholders instructions for the selector calls.
        BBuilderContextData *outerBData =
            BBuilderContextData::get(outerContext.get());;
        cdata = outerBData->getCatchData();
    } else {
        // no catch clause.  Find or create the unwind clause for the function
        // to continue the unwind.
        ContextPtr funcCtx = context.getToplevel();
        BBuilderContextData *bdata =
            BBuilderContextDataPtr::arcast(funcCtx->builderData);
        BasicBlock *unwindBlock = bdata->getUnwindBlock(func);

        final = unwindBlock;

        // move the outer context back one level so we get cleanups for the
        // function scope.
        outerContext = outerContext->getParent();
    }

    BasicBlock *cleanups = emitUnwindCleanups(context, *outerContext, final);
    BCleanupFrame *firstCleanupFrame =
        BCleanupFramePtr::rcast(context.cleanupFrame);

    return firstCleanupFrame->getLandingPad(cleanups, cdata.get());
}

Function *LLVMBuilder::getExceptionPersonalityFunc() {
    if (!exceptionPersonalityFunc) {
        LLVMContext &lctx = getGlobalContext();
        vector<Type *> args(5);;
        args[0] = Type::getInt32Ty(lctx);
        args[1] = args[0];
        args[2] = Type::getInt64Ty(lctx);
        args[3] = Type::getInt8Ty(lctx)->getPointerTo();
        args[4] = args[3];
        FunctionType *epType = FunctionType::get(Type::getVoidTy(lctx), args,
                                                false
                                                );

        Constant *ep =
            module->getOrInsertFunction("__CrackExceptionPersonality", epType);
        exceptionPersonalityFunc = cast<Function>(ep);
    }
    return exceptionPersonalityFunc;
}

void LLVMBuilder::clearCachedCleanups(Context &context) {
    BCleanupFrame *frame = BCleanupFramePtr::rcast(context.cleanupFrame);
    if (frame)
        frame->clearCachedCleanups();
    BBuilderContextData *bdata = BBuilderContextData::get(&context);
    bdata->unwindBlock = 0;
}

void LLVMBuilder::narrow(TypeDef *curType, TypeDef *ancestor) {
    // quick short-circuit to deal with the trivial case
    if (curType == ancestor)
        return;

    assert(curType->isDerivedFrom(ancestor));

    BTypeDef *bcurType = BTypeDefPtr::acast(curType);
    BTypeDef *bancestor = BTypeDefPtr::acast(ancestor);
    if (curType->complete) {
        lastValue = IncompleteNarrower::emitGEP(builder, bcurType, bancestor,
                                                lastValue
                                                );
    } else {
        // create a placeholder instruction
        PlaceholderInstruction *placeholder =
            new IncompleteNarrower(lastValue, bcurType, bancestor,
                                   builder.GetInsertBlock()
                                   );
        lastValue = placeholder;

        // store it
        bcurType->addPlaceholder(placeholder);
    }
}

Function *LLVMBuilder::getModFunc(BFuncDef *funcDef) {
    string name = funcDef->symbolName.size() ? funcDef->symbolName :
                                               funcDef->getUniqueId();
    Function *func = module->getFunction(name);
    if (!func) {
        // Not found, create a new one.  Since we're setting the name from the
        // name of the funcRep, and we know nothing of that name already
        // exists in the module, we don't have to apply the symbol name from
        // the funcDef to it, it should already have the symbol name.  (in
        // practice, applying the symbol name was causing LLVM name mangling
        // to occur in ways that broke things).
        BFuncDef *bfuncDef = BFuncDefPtr::acast(funcDef);
        func = Function::Create(bfuncDef->getLLVMFuncType(),
                                Function::ExternalLinkage,
                                name,
                                module
                                );

        // If this function is external, add the address.
        void *addr = bfuncDef->getExtFuncAddr();
        if (addr)
            addGlobalFuncMapping(func, addr);
    }
    return func;
}

GlobalVariable *LLVMBuilder::getModVar(BGlobalVarDefImpl *varDefImpl) {

    string name = varDefImpl->getName();
    GlobalVariable *global = module->getGlobalVariable(name);
    if (!global) {
        // extract the raw type
        Type *type = varDefImpl->getLLVMType()->getElementType();

        global =
            new GlobalVariable(*module, type, varDefImpl->isConstant(),
                               GlobalValue::ExternalLinkage,
                               0, // initializer: null for externs
                               name
                               );
    }

    return global;
}

BTypeDefPtr LLVMBuilder::getFuncType(Context &context,
                                     FuncDef *funcDef,
                                     llvm::Type *llvmFuncType
                                     ) {

    // create a new type object and store it
    TypeDefPtr function = context.construct->functionType.get();

    if (!function) {
        // there is no function in this context XXX this should create a
        // deferred entry.
        BTypeDefPtr crkFuncType = new BTypeDef(context.construct->classType.get(),
                                               "",
                                               llvmFuncType
                                               );

        // Give it an "oper to .builtin.voidptr" method.
        context.addDef(
                    new VoidPtrOpDef(context.construct->voidptrType.get()),
                    crkFuncType.get()
                    );

        return crkFuncType.get();
    }

    // we have function, specialize based on return and argument types
    TypeDef::TypeVecObjPtr args = new TypeDef::TypeVecObj();

    // push return
    args->push_back(funcDef->returnType);

    // if there is a receiver, push that
    TypeDefPtr rcvrType = funcDef->receiverType;
    if (rcvrType)
        args->push_back(rcvrType.get());

    // now args
    for (FuncDef::ArgVec::iterator arg = funcDef->args.begin();
         arg != funcDef->args.end();
         ++arg
         ) {
        args->push_back((*arg)->type.get());
    }

    BTypeDefPtr specFuncType =
        BTypeDefPtr::arcast(function->getSpecialization(context, args.get()));

    specFuncType->defaultInitializer = new NullConst(specFuncType.get());

    return specFuncType.get();

}

BHeapVarDefImplPtr LLVMBuilder::createLocalVar(BTypeDef *tp,
                                               Value *&var,
                                               const string &name,
                                               const parser::Location *loc,
                                               Value *initVal
                                               ) {
    // insert an alloca into the first block of the function - we
    // define all of our allocas up front because if we do them in
    // loops they eat the stack.

    // if the last instruction is terminal, we need to insert before it
    BasicBlock::iterator i = funcBlock->end();
    if (i != funcBlock->begin() && !(--i)->isTerminator())
        // otherwise insert after it.
        ++i;

    IRBuilder<> b(funcBlock, i);
    var = b.CreateAlloca(tp->rep, 0);
    var->setName(name);

    if (debugInfo)
        debugInfo->declareLocal(tp,
                                var,
                                b.GetInsertBlock(),
                                loc);

    if (initVal) {
        Instruction *i = b.CreateStore(initVal, var);
        if (debugInfo)
            debugInfo->addDebugLoc(i, loc);
    }

    return new BHeapVarDefImpl(var);
}

void LLVMBuilder::emitExceptionCleanup(Context &context) {
    ExprPtr cleanup =
        new ExceptionCleanupExpr(context.construct->voidType.get());

    // we don't need to close this cleanup frame, these cleanups get generated
    // at the close of the context.
    context.createCleanupFrame();
    context.cleanupFrame->addCleanup(cleanup.get());
}

LLVMBuilder::LLVMBuilder(LLVMBuilder *root) :
    debugInfo(0),
    rootBuilder(root),
    nextModuleId(0),
    module(0),
    builder(getGlobalContext()),
    func(0),
    llvmVoidPtrType(root ? root->llvmVoidPtrType : 0),
    lastValue(0),
    intzLLVM(root ? root->intzLLVM : 0),
    exceptionPersonalityFunc(0),
    unwindResumeFunc(0) {

    if (root)
        options = root->options;
}

ResultExprPtr LLVMBuilder::emitFuncCall(Context &context, FuncCall *funcCall) {

    // get the LLVM arg list from the receiver and the argument expressions
    vector<Value*> valueArgs;

    // either a normal function call or a function pointer call
    BFuncDef *funcDef = BFuncDefPtr::rcast(funcCall->func);
    BFuncPtr *funcPtr;
    if (!funcDef) {
        funcPtr = BFuncPtrPtr::rcast(funcCall->func);
        assert(funcPtr && "no funcDef or funcPtr");
    }

    // if there's a receiver, use it as the first argument.
    Value *receiver;

    if (funcCall->receiver) {
        assert(funcDef && "funcPtr instead of funcDef");
        funcCall->receiver->emit(context)->handleTransient(context);
        narrow(funcCall->receiver->type.get(), funcDef->receiverType.get());
        receiver = lastValue;
        valueArgs.push_back(receiver);
    } else {
        receiver = 0;
    }

    // emit the arguments
    FuncCall::ExprVec &vals = funcCall->args;
    FuncDef::ArgVec::iterator argIter = funcCall->func->args.begin();
    for (ExprVec::const_iterator valIter = vals.begin(); valIter < vals.end();
         ++valIter, ++argIter
         ) {
        (*valIter)->emit(context)->handleTransient(context);
        narrow((*valIter)->type.get(), (*argIter)->type.get());
        valueArgs.push_back(lastValue);
    }


    // if we're already emitting cleanups for an unwind, both the normal
    // destination block and the cleanup block are the same.
    BasicBlock *followingBlock = 0, *cleanupBlock;
    getInvokeBlocks(context, followingBlock, cleanupBlock);

    if (funcCall->virtualized) {
        assert(funcDef && "funcPtr instead of funcDef");
        lastValue = IncompleteVirtualFunc::emitCall(context, funcDef,
                                                    receiver,
                                                    valueArgs,
                                                    followingBlock,
                                                    cleanupBlock
                                                    );
    }
    else {
        // for a normal funcdef, get the Function*
        // for a function pointer, we invoke the pointer directly
        Value *callee = (funcDef) ? funcDef->getRep(*this) : funcPtr->rep;
        lastValue =
            builder.CreateInvoke(callee, followingBlock,
                                 cleanupBlock,
                                 valueArgs
                                 );

    }

    // continue emitting code into the new following block.
    if (followingBlock != cleanupBlock)
        builder.SetInsertPoint(followingBlock);

    return new BResultExpr(funcCall, lastValue);
}

ResultExprPtr LLVMBuilder::emitStrConst(Context &context, StrConst *val) {
    BStrConst *bval = BStrConstPtr::cast(val);
    // if the global string hasn't been defined yet in this module, create it
    if (!bval->rep || bval->module != module) {
        // we have to do this the hard way because strings may contain
        // embedded nulls (IRBuilder.CreateGlobalStringPtr expects a
        // null-terminated string)
        LLVMContext &llvmContext = getGlobalContext();
        Constant *llvmVal =
            ConstantDataArray::getString(llvmContext, val->val, true);
        GlobalVariable *gvar = new GlobalVariable(*module,
                                                  llvmVal->getType(),
                                                  true, // is constant
                                                  GlobalValue::InternalLinkage,
                                                  llvmVal,
                                                 "str:"+
                                                  module->getModuleIdentifier());

        Value *zero = ConstantInt::get(Type::getInt32Ty(llvmContext), 0);
        Value *args[] = { zero, zero };
        bval->rep = builder.CreateInBoundsGEP(gvar,
                                              ArrayRef<Value *>(args, 2)
                                              );
        bval->module = module;
        moduleDef->stringConstants.push_back(bval);
    }
    lastValue = bval->rep;
    return new BResultExpr(val, lastValue);
}

ResultExprPtr LLVMBuilder::emitIntConst(Context &context, IntConst *val) {
    lastValue = dynamic_cast<const BIntConst *>(val)->rep;
    return new BResultExpr(val, lastValue);
}

ResultExprPtr LLVMBuilder::emitFloatConst(Context &context, FloatConst *val) {
    lastValue = dynamic_cast<const BFloatConst *>(val)->rep;
    return new BResultExpr(val, lastValue);
}

ResultExprPtr LLVMBuilder::emitNull(Context &context,
                                    NullConst *nullExpr
                                    ) {
    BTypeDef *btype = BTypeDefPtr::arcast(nullExpr->type);
    lastValue = Constant::getNullValue(btype->rep);

    return new BResultExpr(nullExpr, lastValue);
}

ResultExprPtr LLVMBuilder::emitAlloc(Context &context, AllocExpr *allocExpr,
                                     Expr *countExpr
                                     ) {
    // XXX need to be able to do this for an incomplete class when we
    // allow user defined oper new.
    BTypeDef *btype = BTypeDefPtr::arcast(allocExpr->type);
    PointerType *tp = cast<PointerType>(btype->rep);

    // XXX mega-hack, clear the contents of the allocated memory (this is to
    // get around the temporary lack of automatic member initialization)

    // calculate the size of instances of the type
    BTypeDef *uintzType = BTypeDefPtr::arcast(context.construct->uintzType);
    Value *size = IncompleteSizeOf::emitSizeOf(context, btype,
                                               uintzType->rep);

    // if a count expression was supplied, emit it.  Otherwise, count is a
    // constant 1
    Value *countVal;
    if (countExpr) {
        countExpr->emit(context)->handleTransient(context);
        countVal = lastValue;
    } else {
        countVal = ConstantInt::get(uintzType->rep, 1);
    }

    // construct a call to the "calloc" function
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);
    vector<Value *> callocArgs(2);
    callocArgs[0] = countVal;
    callocArgs[1] = size;
    Value *result = builder.CreateCall(callocFunc, callocArgs);
    lastValue = builder.CreateBitCast(result, tp);

    return new BResultExpr(allocExpr, lastValue);
}

void LLVMBuilder::emitTest(Context &context, Expr *expr) {
    expr->emit(context);
    BTypeDef *exprType = BTypeDefPtr::arcast(expr->type);
    lastValue =
        builder.CreateICmpNE(lastValue,
                             Constant::getNullValue(exprType->rep)
                             );
}

BranchpointPtr LLVMBuilder::emitIf(Context &context, Expr *cond) {
    return labeledIf(context, cond, "true", "false", true);
}

BranchpointPtr LLVMBuilder::labeledIf(Context &context, Expr *cond,
                                      const char* tLabel,
                                      const char* fLabel,
                                      bool condInCleanupFrame
                                      ) {

    // create blocks for the true and false conditions
    LLVMContext &lctx = getGlobalContext();
    BasicBlock *trueBlock = BasicBlock::Create(lctx, tLabel, func);

    BBranchpointPtr result = new BBranchpoint(
        BasicBlock::Create(lctx, fLabel, func)
    );

    if (condInCleanupFrame) context.createCleanupFrame();
    cond->emitCond(context);
    Value *condVal = lastValue; // condition value
    if (condInCleanupFrame) context.closeCleanupFrame();
    result->block2 = builder.GetInsertBlock(); // condition block
    lastValue = condVal;
    builder.CreateCondBr(lastValue, trueBlock, result->block);

    // repoint to the new ("if true") block
    builder.SetInsertPoint(trueBlock);
    return result;
}

BranchpointPtr LLVMBuilder::emitElse(model::Context &context,
                                     model::Branchpoint *pos,
                                     bool terminal
                                     ) {
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);

    // create a block to come after the else and jump to it from the current
    // "if true" block.
    BasicBlock *falseBlock = bpos->block;
    bpos->block = 0;
    if (!terminal) {
        bpos->block = BasicBlock::Create(getGlobalContext(), "cond_end", func);
        builder.CreateBr(bpos->block);
    }

    // new block is the "false" condition
    builder.SetInsertPoint(falseBlock);
    return pos;
}

void LLVMBuilder::emitEndIf(Context &context,
                            Branchpoint *pos,
                            bool terminal
                            ) {
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);

    // branch from the current block to the next block
    if (!terminal) {
        if (!bpos->block)
            bpos->block =
                BasicBlock::Create(getGlobalContext(), "cond_end", func);
        builder.CreateBr(bpos->block);

    }

    // if we ended up with any non-terminal paths our of the if, the new
    // block is the next block
    if (bpos->block)
        builder.SetInsertPoint(bpos->block);
}

TernaryExprPtr LLVMBuilder::createTernary(model::Context &context,
                                          model::Expr *cond,
                                          model::Expr *trueVal,
                                          model::Expr *falseVal,
                                          model::TypeDef *type
                                          ) {
    return new TernaryExpr(cond, trueVal, falseVal, type);
}

ResultExprPtr LLVMBuilder::emitTernary(Context &context, TernaryExpr *expr) {

    // condition on first arg
    BranchpointPtr pos = labeledIf(context, expr->cond.get(), "tern_T",
                                   "tern_F",
                                   false
                                   );
    BBranchpoint *bpos = BBranchpointPtr::arcast(pos);
    Value *condVal = lastValue; // arg[0] condition value
    BasicBlock *falseBlock = expr->falseVal ? bpos->block : 0; // false block
    BasicBlock *oBlock = bpos->block2; // condition block

    // now pointing to true block, save it for phi
    BasicBlock *trueBlock = builder.GetInsertBlock();

    // create the block after the expression (use the "false" block if there
    // is no false value)
    LLVMContext &lctx = getGlobalContext();
    BasicBlock *postBlock =
        expr->falseVal ? BasicBlock::Create(lctx, "after_tern", func) :
                         bpos->block;

    // emit the true expression in its own cleanup frame
    context.createCleanupFrame();
    ResultExprPtr tempResult = expr->trueVal->emit(context);
    narrow(expr->trueVal->type.get(), expr->type.get());
    Value *trueVal = lastValue;

    // if the false expression is productive and this one isn't, make it
    // productive
    if (expr->falseVal && expr->falseVal->isProductive() &&
        !expr->trueVal->isProductive()
        )
        tempResult->handleAssignment(context);
    context.closeCleanupFrame();

    // branch to the end
    builder.CreateBr(postBlock);

    // pick up changes to the block
    trueBlock = builder.GetInsertBlock();

    // emit the false expression
    Value *falseVal;
    if (expr->falseVal) {
        builder.SetInsertPoint(falseBlock);
        context.createCleanupFrame();
        tempResult = expr->falseVal->emit(context);
        narrow(expr->falseVal->type.get(), expr->type.get());
        falseVal = lastValue;

        // if the true expression was productive, and this one isn't, make it
        // productive
        if (expr->trueVal->isProductive() && !expr->falseVal->isProductive())
            tempResult->handleAssignment(context);
        context.closeCleanupFrame();
        builder.CreateBr(postBlock);
        falseBlock = builder.GetInsertBlock();
    }

    // emit the phi
    builder.SetInsertPoint(postBlock);
    if (expr->falseVal) {
        PHINode *p = builder.CreatePHI(
            BTypeDefPtr::arcast(expr->type)->rep, 2,
            "tern_R"
        );
        p->addIncoming(trueVal, trueBlock);
        p->addIncoming(falseVal, falseBlock);
        lastValue = p;
    }

    return new BResultExpr(expr, lastValue);
}

BranchpointPtr LLVMBuilder::emitBeginWhile(Context &context,
                                           Expr *cond,
                                           bool gotPostBlock
                                           ) {
    LLVMContext &lctx = getGlobalContext();
    BBranchpointPtr bpos = new BBranchpoint(BasicBlock::Create(lctx,
                                                               "while_end",
                                                               func
                                                               )
                                            );
    bpos->context = &context;

    BasicBlock *whileCond =
        BasicBlock::Create(lctx, "while_cond", func);

    // if there is a post-loop block, make it block2 (which gets branched to
    // at the end of the body and from continue) and make the the condition
    // block3.
    if (gotPostBlock) {
        bpos->block2 = BasicBlock::Create(lctx, "while_post", func);
        bpos->block3 = whileCond;
    } else {
        // no post-loop: block2 is the condition
        bpos->block2 = whileCond;
    }

    BasicBlock *whileBody = BasicBlock::Create(lctx, "while_body", func);
    builder.CreateBr(whileCond);
    builder.SetInsertPoint(whileCond);

    // XXX see notes above on a conditional type.
    context.createCleanupFrame();
    cond->emitCond(context);
    Value *condVal = lastValue;
    context.closeCleanupFrame();
    lastValue = condVal;
    builder.CreateCondBr(lastValue, whileBody, bpos->block);

    // begin generating code in the while body
    builder.SetInsertPoint(whileBody);

    return bpos;
}

void LLVMBuilder::emitEndWhile(Context &context, Branchpoint *pos,
                               bool isTerminal
                               ) {
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);

    // emit the branch back to the conditional
    if (!isTerminal)
        // if there's a post-block, jump to the conditional
        if (bpos->block3)
            builder.CreateBr(bpos->block3);
        else
            builder.CreateBr(bpos->block2);

    // new code goes to the following block
    builder.SetInsertPoint(bpos->block);
}

void LLVMBuilder::emitPostLoop(model::Context &context,
                               model::Branchpoint *pos,
                               bool isTerminal
                               ) {
    // block2 should be the post-loop code, block3 should be the condition
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);
    assert(bpos->block3 && "attempted to emit undeclared post-loop");

    if (!isTerminal)
        // branch from the end of the body to the post-loop
        builder.CreateBr(bpos->block2);

    // set the new block to the post-loop
    builder.SetInsertPoint(bpos->block2);
}

void LLVMBuilder::emitBreak(Context &context, Branchpoint *branch) {
    BBranchpoint *bpos = BBranchpointPtr::acast(branch);
    emitCleanupsTo(context, *bpos->context);
    builder.CreateBr(bpos->block);
}

void LLVMBuilder::emitContinue(Context &context, Branchpoint *branch) {
    BBranchpoint *bpos = BBranchpointPtr::acast(branch);
    emitCleanupsTo(context, *bpos->context);
    builder.CreateBr(bpos->block2);
}

void LLVMBuilder::createSpecialVar(Namespace *ns,
                                   TypeDef *type,
                                   const string &name
                                   ) {
    Value *ptr;
    BTypeDef *tp = BTypeDefPtr::cast(type);
    VarDefPtr varDef = new VarDef(tp, name);
    varDef->impl = createLocalVar(tp, ptr, name);
    ns->addDef(varDef.get());
}

void LLVMBuilder::createFuncStartBlocks(const std::string &name) {
    // create the "function block" (the first block in the function, will be
    // used to hold all local variable allocations)
    funcBlock = BasicBlock::Create(getGlobalContext(), name, func);
    builder.SetInsertPoint(funcBlock);

    // since the function block can get appended to arbitrarily, create a
    // first block where it is safe for us to emit terminating instructions
    BasicBlock *firstBlock = BasicBlock::Create(getGlobalContext(), "l",
                                                func
                                                );
    builder.CreateBr(firstBlock);
    builder.SetInsertPoint(firstBlock);
}

void LLVMBuilder::beginModuleMain(const string &moduleFullName) {
    LLVMContext &lctx = getGlobalContext();

    // start out like any other function.
    createFuncStartBlocks(moduleFullName + ":init");

    // insert point is now at the begining of the :main function for
    // this module. this will run the top level code for the module
    // however, we consult a module level global to ensure that we only
    // run the top level code once, since this module may be imported from
    // more than one place
    GlobalVariable *moduleInit =
        new GlobalVariable(*module,
                           Type::getInt1Ty(lctx),
                           false, // constant
                           GlobalValue::InternalLinkage,
                           Constant::getIntegerValue(
                               Type::getInt1Ty(lctx),APInt(1,0,false)
                           ),
                           moduleFullName + ":initialized"
                           );

    // emit code that checks the global and returns immediately if it
    // has been set to 1
    BasicBlock *alreadyInitBlock = BasicBlock::Create(lctx, "alreadyInit", func);
    BasicBlock *mainInsert = BasicBlock::Create(lctx, "topLevel", func);
    Value* currentInitVal = builder.CreateLoad(moduleInit);
    builder.CreateCondBr(currentInitVal, alreadyInitBlock, mainInsert);

    // already init, return
    builder.SetInsertPoint(alreadyInitBlock);
    builder.CreateRetVoid();

    // branch to the actual first block of the function
    builder.SetInsertPoint(mainInsert);
    BasicBlock *temp = BasicBlock::Create(lctx, "moduleBody", func);
    builder.CreateBr(temp);
    builder.SetInsertPoint(temp);
}

namespace {
    /**
    * Create aliases of the runtime function set (defined in
    * createModuleCommon()) in the slave module.
    */
    void aliasRuntimeFuncsInSlave(ModuleDef *slave, ModuleDef *master) {
        slave->addAlias(master->lookUp("__getArgv").get());
        slave->addAlias(master->lookUp("__getArgc").get());
        slave->addAlias(master->lookUp("__CrackThrow").get());
        slave->addAlias(master->lookUp("__CrackGetException").get());
        slave->addAlias(master->lookUp("__CrackBadCast").get());
        slave->addAlias(master->lookUp("__CrackCleanupException").get());
        slave->addAlias(master->lookUp("__CrackExceptionFrame").get());
    }
}

ModuleDefPtr LLVMBuilder::createModule(Context &context,
                                       const string &name,
                                       const string &path,
                                       ModuleDef *owner
                                       ) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    if (!owner)
        createLLVMModule(name);
    else
        module = BModuleDefPtr::cast(owner)->rep;

    if (options->debugMode)
        debugInfo = new DebugInfo(module,
                                  name,
                                  path,
                                  context.builder.options.get()
                                  );

    // create a context data object
    BBuilderContextData::get(&context);

    llvm::Constant *c =
        module->getOrInsertFunction(name + ":main", Type::getVoidTy(lctx),
                                    NULL
                                    );
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    beginModuleMain(name);

    // create the exception structure for the module main function
    createSpecialVar(context.ns.get(), getExStructType(), ":exStruct");

    moduleDef = innerCreateModule(context, name, owner);
    moduleDef->sourcePath = getSourcePath(path);

    if (owner) {
        // If the module is a slave, register it as such and create simple
        // aliases for all of the runtime functions that we would normally
        // have to create.
        owner->getMaster()->addSlave(moduleDef.get());
        aliasRuntimeFuncsInSlave(moduleDef.get(), owner);
        callocFunc = module->getFunction("calloc");
    } else {
        // Create externs for all of the runtime functions.
        createModuleCommon(context);
    }

    return moduleDef;
}

void LLVMBuilder::getInvokeBlocks(Context &context,
                                  BasicBlock *&followingBlock,
                                  BasicBlock *&cleanupBlock
                                  ) {
    followingBlock = 0;
    BBuilderContextData *bdata;
    if (context.emittingCleanups &&
        (bdata = BBuilderContextDataPtr::rcast(context.builderData))
        ) {
        followingBlock = bdata->nextCleanupBlock;
        BBuilderContextData::CatchDataPtr cdata =
            getContextCatchData(context);
        if (followingBlock)
            cleanupBlock = createLandingPad(context, followingBlock,
                                            cdata.get()
                                            );
    }

    // otherwise, create a new normal destinatation block and get the cleanup
    // block.
    if (!followingBlock) {
        followingBlock = BasicBlock::Create(getGlobalContext(), "l",
                                            this->func
                                            );
        cleanupBlock = getUnwindBlock(context);
    }
}

int LLVMBuilder::getNextModuleId() {
    if (rootBuilder) {
        return rootBuilder->getNextModuleId();
    } else {
        return nextModuleId++;
    }
}

Function *LLVMBuilder::getUnwindResumeFunc() {
    if (!unwindResumeFunc) {
        LLVMContext &lctx = getGlobalContext();
        Constant *c = module->getOrInsertFunction("_Unwind_Resume",
                                                  Type::getVoidTy(lctx),
                                                  Type::getInt8PtrTy(lctx),
                                                  NULL
                                                  );
        unwindResumeFunc = cast<Function>(c);
    }
    return unwindResumeFunc;
}

BModuleDef *LLVMBuilder::instantiateModule(Context &context,
                                           const string &name,
                                           Module *module
                                           ) {
    return new BModuleDef(name, context.ns.get(), module,
                          getNextModuleId()
                          );
}

void LLVMBuilder::emitExceptionCleanupExpr(Context &context) {
    Value *exObjVal = getExceptionObjectValue(context, builder);
    Function *cleanupExceptionFunc =
        module->getFunction("__CrackCleanupException");
    BasicBlock *followingBlock, *cleanupBlock;
    getInvokeBlocks(context, followingBlock, cleanupBlock);
    builder.CreateInvoke(cleanupExceptionFunc, followingBlock, cleanupBlock,
                         exObjVal
                         );
    if (followingBlock != cleanupBlock)
        builder.SetInsertPoint(followingBlock);
}

BasicBlock *LLVMBuilder::createLandingPad(
    Context &context,
    BasicBlock *next,
    BBuilderContextData::CatchData *cdata
) {
    BasicBlock *lp;

    // look up the exception structure variable and get its type
    VarDefPtr exStructVar = context.ns->lookUp(":exStruct");
    Type *exStructType = BTypeDefPtr::arcast(exStructVar->type)->rep;

    lp = BasicBlock::Create(getGlobalContext(), "lp", func);
    IRBuilder<> b(lp);
    Type *i8PtrType = b.getInt8PtrTy();

    // get the personality func
    Value *personality = getExceptionPersonalityFunc();

    // create a catch selector or a cleanup selector
    Value *exStruct;
    if (cdata) {
        // We're in a try/catch.  create the incomplete selector function
        // call (class impls will get filled in later)
        IncompleteCatchSelector *sel  =
            new IncompleteCatchSelector(exStructType, personality,
                                        b.GetInsertBlock()
                                        );
        cdata->selectors.push_back(sel);
        exStruct = sel;
    } else {
        // cleanups only
        LandingPadInst *lpi =
            b.CreateLandingPad(exStructType, personality, 0);
        lpi->setCleanup(true);
        exStruct = lpi;
    }

    // get the function-wide exception structure and store the landingpad
    // result in it.
    BMemVarDefImpl *exStructImpl =
        BMemVarDefImplPtr::rcast(exStructVar->impl);
    b.CreateStore(exStruct, exStructImpl->getRep(*this));

    b.CreateBr(next);

    return lp;
}

bool LLVMBuilder::suppressCleanups() {
    // only ever want to do this if the last instruction is unreachable.
    BasicBlock *block = builder.GetInsertBlock();
    BasicBlock::iterator i = block->end();
    return i != block->begin() &&
           (--i)->getOpcode() == Instruction::Unreachable;
}


BranchpointPtr LLVMBuilder::emitBeginTry(model::Context &context) {
    BasicBlock *catchSwitch = BasicBlock::Create(getGlobalContext(),
                                                 "catch_switch",
                                                 func
                                                 );
    BBranchpointPtr bpos = new BBranchpoint(catchSwitch);
    return bpos;
}

ExprPtr LLVMBuilder::emitCatch(Context &context,
                               Branchpoint *branchpoint,
                               TypeDef *catchType,
                               bool terminal
                               ) {
    BBranchpoint *bpos = BBranchpointPtr::cast(branchpoint);

    // get the catch data
    BBuilderContextData *bdata =
        BBuilderContextData::get(&context);
    BBuilderContextData::CatchDataPtr cdata = bdata->getCatchData();

    // if this is the first catch block (as indicated by the lack of a
    // "post-try" block in block2), create the post-try and create a branch to
    // it in the current block.
    if (!bpos->block2) {
        bpos->block2 = BasicBlock::Create(getGlobalContext(), "after_try",
                                          func
                                          );
        if (!terminal)
            builder.CreateBr(bpos->block2);

        // get the cleanup blocks for the contexts in the function outside of
        // the catch
        ContextPtr outsideFunction = context.getToplevel()->getParent();
        BasicBlock *funcUnwindBlock = bdata->getUnwindBlock(func);
        BasicBlock *outerCleanups = emitUnwindCleanups(*context.getParent(),
                                                       *outsideFunction,
                                                       funcUnwindBlock
                                                       );

        // generate a switch instruction based on the value of
        // the selector in :exStruct, we'll fill it in with values later.
        builder.SetInsertPoint(bpos->block);
        Value *selVal = getExceptionSelectorValue(context, builder);
        cdata->switchInst =
            builder.CreateSwitch(selVal, outerCleanups, 3);

    // if the last catch block was not terminal, branch to after_try
    } else if (!terminal) {
        builder.CreateBr(bpos->block2);
    }

    // create a catch block and make it the new insert point.
    BasicBlock *catchBlock = BasicBlock::Create(getGlobalContext(), "catch",
                                                func
                                                );
    builder.SetInsertPoint(catchBlock);

    // store the type and the catch block for later fixup
    BTypeDef *btype = BTypeDefPtr::cast(catchType);
    fixClassInstRep(btype);
    cdata->catches.push_back(
        BBuilderContextData::CatchBranch(btype, catchBlock)
    );

    // record it if the last block was non-terminal
    if (!terminal)
        cdata->nonTerminal = true;

    // emit an expression to get the exception object
    Value *exObjVal = getExceptionObjectValue(context, builder);
    Function *getExFunc = module->getFunction("__CrackGetException");
    vector<Value *> parms(1);
    parms[0] = exObjVal;
    lastValue = builder.CreateCall(getExFunc, parms);
    lastValue = builder.CreateBitCast(lastValue, btype->rep);
    return new LLVMValueExpr(catchType, lastValue);
}

void LLVMBuilder::emitEndTry(model::Context &context,
                             Branchpoint *branchpoint,
                             bool terminal
                             ) {
    // get the catch-data
    BBuilderContextData *bdata = BBuilderContextData::get(&context);
    BBuilderContextData::CatchDataPtr cdata = bdata->getCatchData();

    BasicBlock *nextBlock = BBranchpointPtr::cast(branchpoint)->block2;
    if (!terminal) {
        builder.CreateBr(nextBlock);
        cdata->nonTerminal = true;
    }

    if (!cdata->nonTerminal) {
        // all blocks are terminal - delete the after_try block
        nextBlock->eraseFromParent();
    } else {
        // emit subsequent code into the new block
        builder.SetInsertPoint(nextBlock);
    }

    // if this is a nested try/catch block, add it to the catch data for its
    // parent.
    ContextPtr outer = context.getParent()->getCatch();
    if (!outer->toplevel) {
        // this isn't a toplevel, therefore it is a try/catch statement
        BBuilderContextData::CatchDataPtr enclosingCData =
            BBuilderContextData::get(outer.get())->getCatchData();
        enclosingCData->nested.push_back(cdata);

    } else {
        cdata->fixAllSelectors(moduleDef.get());
    }
}

void LLVMBuilder::emitThrow(Context &context, Expr *expr) {
    Function *throwFunc = module->getFunction("__CrackThrow");
    context.createCleanupFrame();
    expr->emit(context);
    narrow(expr->type.get(), context.construct->vtableBaseType.get());
    BasicBlock *unwindBlock = getUnwindBlock(context),
               *unreachableBlock = BasicBlock::Create(getGlobalContext(),
                                                      "unreachable",
                                                      func
                                                      );
    builder.CreateInvoke(throwFunc, unreachableBlock, unwindBlock, lastValue);
    builder.SetInsertPoint(unreachableBlock);
    builder.CreateUnreachable();
    // XXX think I actually want to discard the cleanup frame
    context.closeCleanupFrame();
}

FuncDefPtr LLVMBuilder::createFuncForward(Context &context,
                                          FuncDef::Flags flags,
                                          const string &name,
                                          TypeDef *returnType,
                                          const vector<ArgDefPtr> &args,
                                          FuncDef *override
                                          ) {
    assert(flags & FuncDef::forward || flags & FuncDef::abstract);

    // create the function
    FuncBuilder f(context, flags, BTypeDefPtr::cast(returnType), name,
                  args.size()
                  );
    f.setArgs(args);

    BTypeDef *classType = 0;
    if (flags & FuncDef::method)
        initializeMethodInfo(context, flags, override, classType, f);
    f.finish(false);
    return f.funcDef;
}

namespace {
    // create a BTypeDef with all of the built-in methods.
    BTypeDefPtr createTypeDef(Context &context, const string &name,
                              BTypeDef *metaType,
                              Type *llvmType,
                              unsigned int nextVTableSlot
                              ) {
        BTypeDefPtr type = new BTypeDef(metaType, name,
                                        PointerType::getUnqual(llvmType),
                                        true,
                                        nextVTableSlot
                                        );

        // tie the meta-class to the class
        metaType->meta = type.get();

        // create the unsafeCast() function.
        context.addDef(new UnsafeCastDef(type.get()), metaType);

        // create function to convert to voidptr
        context.addDef(new VoidPtrOpDef(context.construct->voidptrType.get()),
                       type.get()
                       );

        // make the class default to initializing to null
        type->defaultInitializer = new NullConst(type.get());

        return type;
    }
}

BTypeDefPtr LLVMBuilder::createClass(Context &context, const string &name,
                                     unsigned int nextVTableSlot
                                     ) {
    BTypeDefPtr type;
    BTypeDefPtr metaType = createMetaClass(context, name);

    string canonicalName = context.parent->ns->getNamespaceName() +
                           "." + name;

    StructType *curType;

    // we first check to see if a structure with this canonical name exists
    // if it does, we use it. if it doesn't we create it, and the body and
    // name get set in emitEndClass
    curType = getLLVMType(canonicalName);
    if (!curType) {
        curType = StructType::create(getGlobalContext());
        curType->setName(canonicalName);
        putLLVMType(canonicalName, curType);
    }
    return createTypeDef(context, name, metaType.get(), curType,
                         nextVTableSlot
                         );
}

TypeDefPtr LLVMBuilder::createClassForward(Context &context,
                                           const string &name
                                           ) {
    TypeDefPtr result = createClass(context, name, 0);
    result->forward = true;
    return result;
}

FuncDefPtr LLVMBuilder::emitBeginFunc(Context &context,
                                      FuncDef::Flags flags,
                                      const string &name,
                                      TypeDef *returnType,
                                      const vector<ArgDefPtr> &args,
                                      FuncDef *existing
                                      ) {
    // store the current function and block in the context
    BBuilderContextData *contextData =
        BBuilderContextData::get(&context);
    contextData->func = func;
    contextData->block = builder.GetInsertBlock();

    // if we didn't get a forward declaration, create the function.
    BFuncDefPtr funcDef;
    BTypeDef *classType = 0;
    const vector<ArgDefPtr> *realArgs;
    if (!existing || !(existing->flags & FuncDef::forward)) {

        // create the function
        FuncBuilder f(context, flags, BTypeDefPtr::cast(returnType), name,
                      args.size()
                      );
        f.setArgs(args);

        // see if this is a method, if so store the class type as the receiver type
        if (flags & FuncDef::method) {
            initializeMethodInfo(context, flags, existing, classType, f);
            if (debugInfo)
                debugInfo->emitFunctionDef(SPUG_FSTR(classType->getFullName() <<
                                                    "::" <<
                                                    name
                                                    ),
                                           context.getLocation()
                                           );
        } else if (debugInfo) {
            debugInfo->emitFunctionDef(name, context.getLocation());
            debugInfo->emitLexicalBlock(context.getLocation());
        }

        f.finish(false);
        funcDef = f.funcDef;
        realArgs = &args;
    } else {
        // 'existing' is a forward definition, fill it in.
        funcDef = BFuncDefPtr::acast(existing);
        classType = BTypeDefPtr::cast(funcDef->getOwner());
        funcDef->flags =
            static_cast<FuncDef::Flags>(
                funcDef->flags &
                 static_cast<FuncDef::Flags>(~FuncDef::forward)
            );
        if (debugInfo) {
            debugInfo->emitFunctionDef(funcDef->getFullName(),
                                       context.getLocation()
                                       );
            debugInfo->emitLexicalBlock(context.getLocation());
        }
        realArgs = &existing->args;
    }

    func = funcDef->getFuncRep(*this);

    createFuncStartBlocks(name);

    if (flags & FuncDef::virtualized) {
        // emit code to convert from the first declaration base class
        // instance to the method's class instance.
        ArgDefPtr thisVar = funcDef->thisArg;
        BArgVarDefImpl *thisImpl = BArgVarDefImplPtr::arcast(thisVar->impl);
        Value *inst = thisImpl->rep;
        Context *classCtx = context.getClassContext().get();
        BTypeDefPtr receiverType = funcDef->receiverType;
        Value *thisRep =
            IncompleteSpecialize::emitSpecialize(context,
                                                 classType,
                                                 inst,
                                                 receiverType.get()
                                                 );
        // lookup the "this" variable, and replace its rep
//        VarDefPtr thisVar = context.ns->lookUp("this");
//        BArgVarDefImpl *thisImpl = BArgVarDefImplPtr::arcast(thisVar->impl);
        thisImpl->rep = thisRep;
    }

    // promote all of the arguments to local variables.
    const vector<ArgDefPtr> &a = *realArgs;
    for (int i = 0; i < args.size(); ++i)
        a[i]->impl =
            BArgVarDefImplPtr::arcast(a[i]->impl)->promote(*this, a[i].get());

    // create a variable to hold the exception info from the landing pad
    createSpecialVar(context.ns.get(), getExStructType(), ":exStruct");

    return funcDef;
}

void LLVMBuilder::emitEndFunc(model::Context &context,
                              FuncDef *funcDef) {
    // in certain conditions, (multiple terminating branches) we can end up
    // with an empty block.  If so, remove.
    BasicBlock *block = builder.GetInsertBlock();
    if (block->begin() == block->end())
        block->eraseFromParent();

    // restore the block and function
    BBuilderContextData *contextData =
        BBuilderContextDataPtr::rcast(context.builderData);
    func = contextData->func;
    funcBlock = func ? &func->front() : 0;
    builder.SetInsertPoint(contextData->block);
}

FuncDefPtr LLVMBuilder::createExternFunc(Context &context,
                                         FuncDef::Flags flags,
                                         const string &name,
                                         TypeDef *returnType,
                                         TypeDef *receiverType,
                                         const vector<ArgDefPtr> &args,
                                         void *cfunc,
                                         const char *symbolName
                                         ) {
    return createExternFuncCommon(context, flags, name, returnType,
                                  receiverType,
                                  args,
                                  cfunc,
                                  symbolName
                                  );
}

FuncDefPtr LLVMBuilder::createExternFuncCommon(Context &context,
                                               FuncDef::Flags flags,
                                               const string &name,
                                               TypeDef *returnType,
                                               TypeDef *receiverType,
                                               const vector<ArgDefPtr> &args,
                                               void *cfunc,
                                               const char *symbolName
                                               ) {

    // XXX only needed for linker?
    // if a symbol name wasn't given, we look it up from the dynamic library
    string symName(symbolName?symbolName:"");

    if (symName.empty()) {
        Dl_info dinfo;
        int rdl = dladdr(cfunc, &dinfo);
        if (!rdl || !dinfo.dli_sname) {
            throw spug::Exception(SPUG_FSTR("unable to locate symbol for "
                                            "extern function: " << name));
        }
        symName = dinfo.dli_sname;
    }

    ContextPtr funcCtx =
        context.createSubContext(Context::local, new
                                 LocalNamespace(context.ns.get(), name)
                                 );
    FuncBuilder f(*funcCtx, flags, BTypeDefPtr::cast(returnType),
                  name,
                  args.size(),
                  Function::ExternalLinkage,
                  cfunc
                  );

    if (!symName.empty())
        f.setSymbolName(symName);

    // if we've got a receiver, add it to the func builder and store a "this"
    // variable.
    if (receiverType) {
        // see if there is an existing method to override
        FuncDefPtr existing;
        TypeDef *existingOwner = 0;
        OverloadDefPtr ovld = context.lookUp(name);
        if (ovld) {
            existing = ovld->getSigMatch(args);
            if (existing) {
                existingOwner = TypeDefPtr::cast(existing->getOwner());

                // ignore it if it's not a method from a base class.
                if (!existingOwner ||
                     !receiverType->isDerivedFrom(existingOwner) ||
                     !(existing->flags & FuncDef::method)
                    )
                    existing = 0;
            }
        }

        // if we're overriding a virtual in a base class, change the receiver
        // type to that of the virtual method
        if (existing) {
            assert(existing->flags & FuncDef::virtualized);
            receiverType = existingOwner;
        }

        f.setReceiverType(BTypeDefPtr::acast(receiverType));
        ArgDefPtr thisDef =
            funcCtx->builder.createArgDef(receiverType, "this");
        funcCtx->addDef(thisDef.get());
    }

    f.setArgs(args);
    f.finish(false);
    primFuncs[f.funcDef->getFuncRep(*this)] = cfunc;
    return f.funcDef;
}

namespace {
    void createOperClassFunc(Context &context,
                             BTypeDef *objClass
                             ) {

        // build a local context to hold the "this"
        Context localCtx(context.builder, Context::local, &context,
                         new LocalNamespace(objClass, objClass->name),
                         context.compileNS.get()
                         );
        localCtx.incref();
        localCtx.addDef(new ArgDef(objClass, "this"));

        BTypeDef *classType =
            BTypeDefPtr::arcast(context.construct->classType);
        FuncBuilder funcBuilder(localCtx,
                                FuncDef::method | FuncDef::virtualized,
                                classType,
                                "oper class",
                                0
                                );

        // ensure canonicalized symbol names for link.  We have to construct
        // this name because at the point where we get called the class hasn't
        // yet been assigned a context.
        funcBuilder.setSymbolName(
            SPUG_FSTR(context.parent->ns->getNamespaceName() << '.' <<
                      objClass->name <<
                      ".oper class()"
                      )
        );

        // if this is an override, do the wrapping.
        FuncDefPtr override = context.lookUpNoArgs("oper class", true,
                                                   objClass
                                                   );
        if (override) {
            wrapOverride(objClass, BFuncDefPtr::arcast(override), funcBuilder);
        } else {
            // everything must have an override except for VTableBase::oper
            // class.
            assert(objClass == context.construct->vtableBaseType);
            funcBuilder.funcDef->vtableSlot = objClass->nextVTableSlot++;
            funcBuilder.setReceiverType(objClass);
        }

        funcBuilder.finish(false);
        context.addDef(funcBuilder.funcDef.get(), objClass);

        LLVMBuilder &lb = dynamic_cast<LLVMBuilder &>(context.builder);
        BasicBlock *block = BasicBlock::Create(
            getGlobalContext(),
            "oper class",
            funcBuilder.funcDef->getFuncRep(lb)
        );

        // body of the function: load the class instance global variable
        IRBuilder<> builder(block);
        BGlobalVarDefImpl *impl =
            BGlobalVarDefImplPtr::arcast(objClass->impl);
        Value *val = builder.CreateLoad(impl->getRep(lb));

        // extract the Class instance from it and return it.
        val = builder.CreateConstGEP2_32(val, 0, 0);
        builder.CreateRet(val);
    }
}

TypeDefPtr LLVMBuilder::emitBeginClass(Context &context,
                                       const string &name,
                                       const vector<TypeDefPtr> &bases,
                                       TypeDef *forwardDef
                                       ) {
    assert(!context.builderData);
    BBuilderContextData *bdata =
        BBuilderContextData::get(&context);

    // find the first base class with a vtable
    BTypeDef *baseWithVTable = 0;
    for (vector<TypeDefPtr>::const_iterator iter = bases.begin();
        iter != bases.end();
        ++iter
        ) {
        BTypeDef *base = BTypeDefPtr::rcast(*iter);
        if (base->hasVTable) {
            baseWithVTable = base;
            break;
        }
    }

    BTypeDefPtr type;
    if (!forwardDef) {
        type = createClass(context, name,
                           baseWithVTable ?
                                baseWithVTable->nextVTableSlot : 0
                           );
    } else {
        type = BTypeDefPtr::acast(forwardDef);
        type->nextVTableSlot =
            baseWithVTable ? baseWithVTable->nextVTableSlot : 0;
        type->forward = false;
    }

    // add all of the base classes to the type
    for (vector<TypeDefPtr>::const_iterator iter = bases.begin();
         iter != bases.end();
         ++iter
         ) {
        BTypeDef *base = BTypeDefPtr::rcast(*iter);
        type->addBaseClass(base);
    }

    // create the class implementation.
    createClassImpl(context, type.get());

    // make the type the namespace of the context
    context.ns = type;

    // create the "oper class" function - currently returns voidptr, but
    // that's good enough for now.
    if (baseWithVTable)
        createOperClassFunc(context, type.get());

    return type.get();
}

void LLVMBuilder::emitEndClass(Context &context) {
    // build a vector of the base classes and instance variables
    vector<Type *> members;

    // first the base classes
    BTypeDef *type = BTypeDefPtr::arcast(context.ns);
    for (TypeDef::TypeVec::iterator baseIter = type->parents.begin();
         baseIter != type->parents.end();
         ++baseIter
         ) {
        BTypeDef *typeDef = BTypeDefPtr::arcast(*baseIter);
        members.push_back(cast<PointerType>(typeDef->rep)->getElementType());
    }

    for (TypeDef::VarDefMap::iterator iter = type->beginDefs();
        iter != type->endDefs();
        ++iter
        ) {
        BFuncDef *funcDef;

        // see if the variable needs an instance slot
        if (iter->second->hasInstSlot()) {
            BInstVarDefImpl *impl =
                BInstVarDefImplPtr::rcast(iter->second->impl);

            // resize the set of members if the new guy doesn't fit
            if (impl->index >= members.size())
                members.resize(impl->index + 1, 0);

            // get the underlying type object, add it to the vector
            BTypeDef *typeDef = BTypeDefPtr::rcast(iter->second->type);
            members[impl->index] = typeDef->rep;
        }
    }

    // if instances of the type require padding, add a character array.
    if (type->padding)
        members.push_back(ArrayType::get(builder.getInt8Ty(), type->padding));

    // verify that all of the members have been assigned
    for (vector<Type *>::iterator iter = members.begin();
         iter != members.end();
         ++iter
         )
        assert(*iter);

    // refine the type to the actual type of the structure.

    // we also check and reuse an existing type of the same canonical name
    // if it exists in this llvm context. this happens in caching scenarios
    string canonicalName = context.parent->ns->getNamespaceName() +
                           "." + type->name;

    // extract the struct type out of the existing pointer type
    const PointerType *ptrType =
        cast<PointerType>(type->rep);
    StructType *curType = cast<StructType>(ptrType->getElementType());

    // set the body (if we haven't already: this can happen if we're reusing
    // an existing LLVM type object)
    if (curType->isOpaque())
        curType->setBody(members);

    // verify that all of the base classes are complete (because we can only
    // inherit from an incomplete base class in the case of a nested derived
    // class, there can be only one incomplete base class)
    TypeDefPtr incompleteBase;
    for (TypeDef::TypeVec::iterator iter = type->parents.begin();
         iter != type->parents.end();
         ++iter
         ) {
        if (!(*iter)->complete) {
            assert(!incompleteBase);
            incompleteBase = *iter;
        }
    }

    // if we have an incomplete base, we have to defer placeholder instruction
    // resolution to the incomplete base class
    if (incompleteBase)
        BTypeDefPtr::arcast(incompleteBase)->addDependent(type, &context);
    else
        type->fixIncompletes(context);
}

void LLVMBuilder::emitReturn(model::Context &context,
                             model::Expr *expr) {

    if (expr) {
        ResultExprPtr resultExpr = expr->emit(context);
        narrow(expr->type.get(), context.returnType.get());
        Value *retVal = lastValue;

        // XXX there's an opportunity for an optimization here, if we return a
        // local variable, we should omit the cleanup of that local variable
        // and the bind of the assignment.
        resultExpr->handleAssignment(context);
        emitFunctionCleanups(context);

        builder.CreateRet(retVal);
    } else {
        emitFunctionCleanups(context);
        builder.CreateRetVoid();
    }
}

VarDefPtr LLVMBuilder::emitVarDef(Context &context, TypeDef *type,
                                  const string &name,
                                  Expr *initializer,
                                  bool staticScope
                                  ) {
    // XXX use InternalLinkage for variables starting with _ (I think that
    // might work)

    // reveal our type object
    BTypeDef *tp = BTypeDefPtr::cast(type);

    // get the definition context
    ContextPtr defCtx = context.getDefContext();

    // do initialization (unless we're in instance scope - instance variables
    // get initialized in the constructors)
    if (defCtx->scope != Context::instance) {
        ResultExprPtr result;
        if (initializer) {
            result = initializer->emit(context);
            narrow(initializer->type.get(), type);
        } else {
            // assuming that we don't need to narrow a default initializer.
            result = type->defaultInitializer->emit(context);
        }

        // handle the assignment, then restore the original last value (since
        // assignment handling can modify that.
        Value *tmp = lastValue;
        result->handleAssignment(context);
        lastValue = tmp;
    }

    Value *var = 0;
    BMemVarDefImplPtr varDefImpl;
    switch (defCtx->scope) {

        case Context::instance:
            // class statics share the same context as instance variables:
            // they are distinguished from instance variables by their
            // declaration and are equivalent to module scoped globals in the
            // way they are emitted, so if the staticScope flag is set we want
            // to fall through to module scope
            if (!staticScope) {
                // first, we need to determine the index of the new field.
                BTypeDef *btype = BTypeDefPtr::arcast(defCtx->ns);
                unsigned idx = btype->fieldCount++;

                // instance variables are unlike the other stored types - we
                // use the InstVarDef class to preserve the initializer and a
                // different kind of implementation object.
                VarDefPtr varDef =
                    new InstVarDef(type, name,
                                   initializer ? initializer :
                                                 type->defaultInitializer.get()
                                   );
                varDef->impl = new BInstVarDefImpl(idx);
                return varDef;
            }

        case Context::module: {
            GlobalVariable *gvar;
            var = gvar =
                new GlobalVariable(*module, tp->rep, false, // isConstant
                                   GlobalValue::ExternalLinkage,

                                   // initializer - this needs to be
                                   // provided or the global will be
                                   // treated as an extern.
                                   Constant::getNullValue(tp->rep),
                                   module->getModuleIdentifier()+"."+name
                                   );
            varDefImpl = new BGlobalVarDefImpl(gvar, moduleDef->repId);
            break;
        }

        case Context::local: {
            varDefImpl = createLocalVar(tp, var, name, &context.getLocation());
            break;
        }

        default:
            assert(false && "invalid context value!");
    }

    // allocate the variable and assign it
    lastValue = builder.CreateStore(lastValue, var);
    if (debugInfo)
        debugInfo->addDebugLoc(cast<Instruction>(lastValue),
                               &context.getLocation()
                               );

    // create the definition object.
    VarDefPtr varDef = new VarDef(type, name);
    varDef->impl = varDefImpl;
    return varDef;
}


VarDefPtr LLVMBuilder::createOffsetField(model::Context &context,
                                         model::TypeDef *type,
                                         const std::string &name,
                                         size_t offset
                                         ) {
    VarDefPtr varDef = new VarDef(type, name);
    varDef->impl = new BOffsetFieldDefImpl(offset);
    return varDef;
}

CleanupFramePtr LLVMBuilder::createCleanupFrame(Context &context) {
    return new BCleanupFrame(&context);
}

void LLVMBuilder::closeAllCleanups(Context &context) {
    closeAllCleanupsStatic(context);
}

model::StrConstPtr LLVMBuilder::createStrConst(model::Context &context,
                                               const std::string &val) {
    return new BStrConst(context.construct->byteptrType.get(), val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, int64_t val,
                                        TypeDef *typeDef
                                        ) {
    // XXX probably need to consider the simplest type that the constant can
    // fit into (compatibility rules will allow us to coerce it into another
    // type)
    return new BIntConst(typeDef ? BTypeDefPtr::acast(typeDef) :
                          BTypeDefPtr::acast(IntConst::selectType(context,
                                                                  val
                                                                  )
                                              ),
                         val
                         );
}
// in this case, we know we have a constant big enough to require an unsigned
// 64 bit int
IntConstPtr LLVMBuilder::createUIntConst(model::Context &context, uint64_t val,
                                         TypeDef *typeDef
                                         ) {
    return new BIntConst(typeDef ? BTypeDefPtr::acast(typeDef) :
                         BTypeDefPtr::acast(
                             context.construct->uint64Type.get()),
                         val
                         );
}

FloatConstPtr LLVMBuilder::createFloatConst(model::Context &context, double val,
                                        TypeDef *typeDef
                                        ) {
    // XXX probably need to consider the simplest type that the constant can
    // fit into (compatibility rules will allow us to coerce it into another
    // type)
    return new BFloatConst(typeDef ? BTypeDefPtr::acast(typeDef) :
                           BTypeDefPtr::arcast(context.construct->floatType),
                           val
                           );
}

model::FuncCallPtr LLVMBuilder::createFuncCall(FuncDef *func,
                                               bool squashVirtual
                                               ) {
    // try to create a BinCmp
    OpDef *specialOp = OpDefPtr::cast(func);
    if (specialOp) {
        // if this is a bin op, let it create the call
        return specialOp->createFuncCall();
    } else {
        // normal function call
        return new FuncCall(func, squashVirtual);
    }
}

ArgDefPtr LLVMBuilder::createArgDef(TypeDef *type,
                                    const string &name
                                    ) {
    // we don't create BBuilderVarDefData for these yet - we will back-fill
    // the builder data when we create the function object.
    ArgDefPtr argDef = new ArgDef(type, name);
    return argDef;
}

VarRefPtr LLVMBuilder::createVarRef(VarDef *varDef) {
    return new VarRef(varDef);
}

VarRefPtr LLVMBuilder::createFieldRef(Expr *aggregate,
                                      VarDef *varDef
                                      ) {
    return new BFieldRef(aggregate, varDef);
}

ResultExprPtr LLVMBuilder::emitFieldAssign(Context &context,
                                           Expr *aggregate,
                                           AssignExpr *assign
                                           ) {
    aggregate->emit(context);

    // narrow to the type of the ancestor that owns the field.
    BTypeDef *typeDef = BTypeDefPtr::acast(assign->var->getOwner());
    narrow(aggregate->type.get(), typeDef);
    Value *aggregateRep = lastValue;

    // emit the value last, lastValue after this needs to be the expression so
    // we can chain assignments.
    ResultExprPtr resultExpr = assign->value->emit(context);

    // record the result as being bound to a variable.
    Value *temp = lastValue;
    resultExpr->handleAssignment(context);
    lastValue = temp;

    // narrow the value to the type of the variable (we do some funky checking
    // here because of constants, which can present a different type that they
    // match)
    if (assign->value->type != assign->var->type &&
        assign->value->type->isDerivedFrom(assign->var->type.get()))
        narrow(assign->value->type.get(), assign->var->type.get());

    // emit the field assignment or a placeholder
    BFieldDefImplPtr impl = BFieldDefImplPtr::rcast(assign->var->impl);
    if (typeDef->complete) {
        impl->emitFieldAssign(builder, aggregateRep, lastValue);
    } else {
        // create a placeholder instruction
        PlaceholderInstruction *placeholder =
            new IncompleteInstVarAssign(aggregateRep->getType(),
                                        aggregateRep,
                                        impl.get(),
                                        lastValue,
                                        builder.GetInsertBlock()
                                        );

        // store it
        typeDef->addPlaceholder(placeholder);
    }

    lastValue = temp;
    return new BResultExpr(assign, temp);
}

Builder::CacheFilePtr LLVMBuilder::getCacheFile(Context &context,
                                                const std::string &canonicalName
                                                ) {
    LLVMCacheFilePtr cacheFile = new LLVMCacheFile(context, options.get(),
                                                   canonicalName
                                                   );
    if (cacheFile->getCacheFile())
        return cacheFile;
    else
        return 0;
}

VarDefPtr LLVMBuilder::materializeVar(Context &context, const string &name,
                                      TypeDef *type,
                                      int instSlot
                                      ) {
    ostringstream tmp;
    tmp << context.ns->getNamespaceName() << "." << name;
    const string &fullName = tmp.str();
    VarDefPtr result = new VarDef(type, name);
    if (instSlot >= 0) {
        result->impl = new BInstVarDefImpl(instSlot);
    } else {
        GlobalVariable *gvar = module->getGlobalVariable(fullName);
        SPUG_CHECK(gvar,
                   "Global variable " << fullName << " of type " <<
                    type->getFullName() << " not found in module " <<
                    module->getModuleIdentifier()
                );
        result->impl = new BGlobalVarDefImpl(gvar, moduleDef->repId);
    }
    return result;
}

ArgDefPtr LLVMBuilder::materializeArg(Context &context, const string &name,
                                      TypeDef *type
                                      ) {
    return new ArgDef(type, name);
}

TypeDefPtr LLVMBuilder::materializeType(Context &context, const string &name,
                                        const string &namespaceName) {
    ostringstream tmp;
    tmp << namespaceName << "." << name;
    const string &fullName = tmp.str();
    Type *llvmType = module->getTypeByName(fullName);
    SPUG_CHECK(llvmType,
               "Type " << fullName << " not found in module " <<
                module->getModuleIdentifier()
               );

    // This is kind of a mess.  Normally createMetaClass is passed the class
    // context.  But this isn't the case during deserialization.  During
    // deserialization we get the class' enclosing context (and that also
    // requires some special handling for slave modules).  So we pass in the
    // enclosing context's namespace to force createMetaClass() to make it the
    // owner of the new meta-class.
    BTypeDefPtr metaType = createMetaClass(context, name, context.ns.get());
    BTypeDefPtr result =
        createTypeDef(context, name, metaType.get(), llvmType, 0);

    // get the class body instance global variable.
    GlobalVariable *gvar = module->getGlobalVariable(fullName);
    SPUG_CHECK(gvar,
               "Unable to load global variable " << fullName <<
                " from module " << module->getModuleIdentifier()
               );
    result->impl = new BGlobalVarDefImpl(gvar, moduleDef->repId);
    result->complete = true;
    result->setClassInst(module->getGlobalVariable(fullName + ":body"));

    return result;
}


FuncDefPtr LLVMBuilder::materializeFunc(Context &context, FuncDef::Flags flags,
                                        const string &name,
                                        TypeDef *returnType,
                                        const ArgVec &args
                                        ) {
    BFuncDefPtr result = new BFuncDef(flags, name, args.size());
    result->args = args;
    result->returnType = returnType;

    if (!(flags & FuncDef::abstract)) {
        string fullName = result->getUniqueId(context.ns.get());
        Function *func = module->getFunction(fullName);
        SPUG_CHECK(func,
                "Function " << fullName << " not found in module " <<
                    module->getModuleIdentifier()
                );
        result->setRep(func, moduleDef->repId);
    } else {
        // Create a null constant.  First we have to construct a type.
        vector<Type *> argTypes;
        argTypes.reserve(args.size());
        for (ArgVec::const_iterator iter = args.begin();
             iter != args.end();
             ++iter
             )
            argTypes.push_back(BTypeDefPtr::arcast((*iter)->type)->rep);

        FunctionType *funcType =
            FunctionType::get(BTypeDefPtr::acast(returnType)->rep,
                              argTypes,
                              false
                              );

        result->setRep(Constant::getNullValue(funcType->getPointerTo()),
                       moduleDef->repId
                       );
    }
    return result;
}

void LLVMBuilder::cacheModule(Context &context, ModuleDef *module,
                              const std::string &uniquifier
                              ) {
    string errors;
    string path = getCacheFilePath(options.get(),
                                   *context.construct,
                                   module->getNamespaceName(),
                                   "bc"
                                   );
    tool_output_file out((path + uniquifier).c_str(), errors, 0);
    if (errors.size())
        throw spug::Exception(errors);

    // encode main function location in bitcode metadata
    Module *mod = BModuleDefPtr::cast(module)->rep;
    vector<Value *> dList;
    NamedMDNode *node;

    // Cacher metadata is mostly obsolete, but we still rely on it for shared
    // library imports.
    Cacher cacher(context, options.get(), BModuleDefPtr::cast(module));
    cacher.writeMetadata();

    // fos needs to destruct before we can "keep()"
    {
        formatted_raw_ostream fos(out.os());
        WriteBitcodeToFile(mod, fos);
    }

    out.keep();
}

void LLVMBuilder::finishCachedModule(Context &context, ModuleDef *module,
                                     const string &uniquifier,
                                     bool retain
                                     ) {
    string finalPath = getCacheFilePath(options.get(),
                                        *context.construct,
                                        module->getNamespaceName(),
                                        "bc"
                                        );
    string tempPath = finalPath + uniquifier;
    if (retain)
        move(tempPath, finalPath);
    else
        unlink(tempPath.c_str());
}

ModuleDefPtr LLVMBuilder::registerPrimFuncs(model::Context &context) {

    assert(!context.getParent()->getParent() && "parent context must be root");
    assert(!module);

    if (options->statsMode)
        context.construct->stats->setState(ConstructStats::builtin);

    createLLVMModule(".builtin");
    BModuleDefPtr builtinMod = instantiateModule(context, ".builtin", module);
    moduleDef = builtinMod;

    // Replace the context's namespace, it's going to become the builtin
    // module.
    context.ns = builtinMod;

    if (options->debugMode)
        debugInfo = new DebugInfo(module,
                                  ".builtin",
                                  "",
                                  options.get()
                                  );

    if (options->statsMode)
        context.construct->stats->setModule(builtinMod.get());

    Construct *gd = context.construct;
    LLVMContext &lctx = getGlobalContext();

    // create the basic types

    BTypeDef *classType;
    StructType *classTypeRep = getLLVMType(".builtin.Class");
    if (!classTypeRep) {
        classTypeRep = StructType::create(lctx);
        classTypeRep->setName(".builtin.Class");
        putLLVMType(".builtin.Class", classTypeRep);
    }
    Type *classTypePtrRep = PointerType::getUnqual(classTypeRep);
    gd->classType = classType = new BTypeDef(0, "Class", classTypePtrRep);
    classType->type = classType;
    classType->meta = classType;
    classType->defaultInitializer = new NullConst(classType);
    context.addDef(classType);

    // some tools for creating meta-classes
    BTypeDefPtr metaType;           // storage for meta-types

    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef(context.construct->classType.get(),
                                           "void",
                                           Type::getVoidTy(lctx)
                                           );
    context.addDef(voidType);
    deferMetaClass.push_back(voidType);

    BTypeDef *voidptrType;
    llvmVoidPtrType = Type::getInt8Ty(lctx)->getPointerTo();
    gd->voidptrType = voidptrType =
        new BTypeDef(context.construct->classType.get(), "voidptr",
                     llvmVoidPtrType
                     );
    voidptrType->defaultInitializer = new NullConst(voidptrType);
    context.addDef(voidptrType);
    deferMetaClass.push_back(voidptrType);

    // now that we've got a voidptr type, give the class object a cast to it.
    context.addDef(new VoidPtrOpDef(voidptrType), classType);

    llvm::Type *llvmBytePtrType =
        PointerType::getUnqual(Type::getInt8Ty(lctx));
    BTypeDef *byteptrType;
    gd->byteptrType = byteptrType = new BTypeDef(context.construct->classType.get(),
                                                 "byteptr",
                                                 llvmBytePtrType
                                                 );
    byteptrType->defaultInitializer = new NullConst(byteptrType);
    byteptrType->complete = true;
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        byteptrType
    );
    deferMetaClass.push_back(byteptrType);
    context.addDef(byteptrType);

    Type *llvmBoolType = IntegerType::getInt1Ty(lctx);
    BTypeDef *boolType;
    gd->boolType = boolType = new BTypeDef(context.construct->classType.get(),
                                           "bool",
                                           llvmBoolType
                                           );
    gd->boolType->defaultInitializer = new BIntConst(boolType, (int64_t)0);
    context.addDef(boolType);
    deferMetaClass.push_back(boolType);
    if (debugInfo) {
        debugInfo->createBasicType(boolType, 8, dwarf::DW_ATE_boolean);
    }

    // we don't create the intz type until later, but we have to cheat and
    // get a sneak preview because this type is needed to create the offsets
    // tables.
    bool ptrIs32Bit = sizeof(void *) == 4;
    if (ptrIs32Bit) {
        intzLLVM = Type::getInt32Ty(lctx);
    } else {
        assert(sizeof(void *) == 8);
        intzLLVM = Type::getInt64Ty(lctx);
    }

    BTypeDef *byteType = createIntPrimType(context, Type::getInt8Ty(lctx),
                                           "byte"
                                           );
    gd->byteType = byteType;
    deferMetaClass.push_back(byteType);
    if (debugInfo) {
        debugInfo->createBasicType(byteType, 8, dwarf::DW_ATE_unsigned_char);
    }

    BTypeDef *int16Type = createIntPrimType(context, Type::getInt16Ty(lctx),
                                            "int16"
                                            );
    gd->int16Type = int16Type;
    deferMetaClass.push_back(int16Type);
    if (debugInfo) {
        debugInfo->createBasicType(int16Type, 16, dwarf::DW_ATE_signed);
    }

    BTypeDef *int32Type = createIntPrimType(context, Type::getInt32Ty(lctx),
                                            "int32"
                                            );
    gd->int32Type = int32Type;
    deferMetaClass.push_back(int32Type);
    if (debugInfo) {
        debugInfo->createBasicType(int32Type, 32, dwarf::DW_ATE_signed);
    }

    BTypeDef *int64Type = createIntPrimType(context, Type::getInt64Ty(lctx),
                                            "int64"
                                            );
    gd->int64Type = int64Type;
    deferMetaClass.push_back(int64Type);
    if (debugInfo) {
        debugInfo->createBasicType(int64Type, 64, dwarf::DW_ATE_signed);
    }

    BTypeDef *uint16Type = createIntPrimType(context, Type::getInt16Ty(lctx),
                                            "uint16"
                                            );
    gd->uint16Type = uint16Type;
    deferMetaClass.push_back(uint16Type);
    if (debugInfo) {
        debugInfo->createBasicType(uint16Type, 16, dwarf::DW_ATE_unsigned);
    }

    BTypeDef *uint32Type = createIntPrimType(context, Type::getInt32Ty(lctx),
                                            "uint32"
                                            );
    gd->uint32Type = uint32Type;
    deferMetaClass.push_back(uint32Type);
    if (debugInfo) {
        debugInfo->createBasicType(uint32Type, 32, dwarf::DW_ATE_unsigned);
    }

    BTypeDef *uint64Type = createIntPrimType(context, Type::getInt64Ty(lctx),
                                            "uint64"
                                            );
    gd->uint64Type = uint64Type;
    deferMetaClass.push_back(uint64Type);
    if (debugInfo) {
        debugInfo->createBasicType(uint64Type, 64, dwarf::DW_ATE_unsigned);
    }

    BTypeDef *float32Type = createFloatPrimType(context, Type::getFloatTy(lctx),
                                            "float32"
                                            );
    gd->float32Type = float32Type;
    deferMetaClass.push_back(float32Type);
    if (debugInfo) {
        debugInfo->createBasicType(float32Type, 32, dwarf::DW_ATE_float);
    }

    BTypeDef *float64Type = createFloatPrimType(context,
                                                Type::getDoubleTy(lctx),
                                                "float64"
                                                );
    gd->float64Type = float64Type;
    deferMetaClass.push_back(float64Type);
    if (debugInfo) {
        debugInfo->createBasicType(float64Type, 64, dwarf::DW_ATE_float);
    }

    // PDNTs
    BTypeDef *intType, *uintType, *floatType, *intzType, *uintzType,
        *atomicType;
    bool intIs32Bit, floatIs32Bit;
    if (sizeof(int) == 4) {
        intIs32Bit = true;
        intType = createIntPrimType(context, Type::getInt32Ty(lctx), "int");
        uintType = createIntPrimType(context, Type::getInt32Ty(lctx), "uint");
        if (debugInfo) {
            debugInfo->createBasicType(intType, 32, dwarf::DW_ATE_signed);
            debugInfo->createBasicType(uintType, 32, dwarf::DW_ATE_unsigned);
        }
    } else {
        assert(sizeof(int) == 8);
        intIs32Bit = false;
        intType = createIntPrimType(context, Type::getInt64Ty(lctx), "int");
        uintType = createIntPrimType(context, Type::getInt64Ty(lctx), "uint");
        if (debugInfo) {
            debugInfo->createBasicType(intType, 64, dwarf::DW_ATE_signed);
            debugInfo->createBasicType(uintType, 64, dwarf::DW_ATE_unsigned);
        }
    }
    gd->intType = intType;
    gd->uintType = uintType;
    gd->intSize = intIs32Bit ? 32 : 64;
    deferMetaClass.push_back(intType);
    deferMetaClass.push_back(uintType);

    intzType = createIntPrimType(context, intzLLVM, "intz");
    uintzType = createIntPrimType(context, intzLLVM, "uintz");
    atomicType = createIntPrimType(context, intzLLVM, "atomic_int");
    if (debugInfo) {
        debugInfo->createBasicType(intzType, ptrIs32Bit ? 32 : 64,
                                   dwarf::DW_ATE_signed
                                   );
        debugInfo->createBasicType(uintzType, ptrIs32Bit ? 32 : 64,
                                   dwarf::DW_ATE_unsigned
                                   );
        debugInfo->createBasicType(atomicType, ptrIs32Bit ? 32 : 64,
                                   dwarf::DW_ATE_signed
                                   );
    }

    gd->intzType = intzType;
    gd->uintzType = uintzType;
    gd->intzSize = ptrIs32Bit ? 32 : 64;
    deferMetaClass.push_back(intzType);
    deferMetaClass.push_back(uintzType);
    deferMetaClass.push_back(atomicType);

    if (sizeof(float) == 4) {
        floatIs32Bit = true;
        floatType = createFloatPrimType(context, Type::getFloatTy(lctx),
                                        "float"
                                        );
        if (debugInfo) {
            debugInfo->createBasicType(floatType, 32, dwarf::DW_ATE_float);
        }
    } else {
        floatIs32Bit = false;
        assert(sizeof(float) == 8);

        floatType = createFloatPrimType(context, Type::getDoubleTy(lctx),
                                        "float"
                                        );
        if (debugInfo) {
            debugInfo->createBasicType(floatType, 64, dwarf::DW_ATE_float);
        }
    }
    gd->floatType = floatType;
    deferMetaClass.push_back(floatType);

    // conversion from voidptr to integer
    FuncDefPtr funcDef =
        new GeneralOpDef<PtrToIntOpCall>(uintzType, FuncDef::noFlags,
                                         "oper new",
                                         1
                                         );
    funcDef->args[0] = new ArgDef(voidptrType, "val");
    context.addDef(funcDef.get(), uintzType);

    // the definition order of global binary operations is significant, when
    // there is no exact match and we need to attempt conversions, we want to
    // check the higher precision types first.

    // create integer operations
#define INTOPS(type, signed, shift, ns) \
    context.addDef(new AddOpDef(type, 0, ns), ns);                                \
    context.addDef(new SubOpDef(type, 0, ns), ns);                                \
    context.addDef(new MulOpDef(type, 0, ns), ns);                                \
    context.addDef(new signed##DivOpDef(type, 0, ns), ns);                        \
    context.addDef(new signed##RemOpDef(type, 0, ns), ns);                        \
    context.addDef(new ICmpEQOpDef(type, boolType, ns), ns);                      \
    context.addDef(new ICmpNEOpDef(type, boolType, ns), ns);                      \
    context.addDef(new ICmp##signed##GTOpDef(type, boolType, ns), ns);                     \
    context.addDef(new ICmp##signed##LTOpDef(type, boolType, ns), ns);                     \
    context.addDef(new ICmp##signed##GEOpDef(type, boolType, ns), ns);                     \
    context.addDef(new ICmp##signed##LEOpDef(type, boolType, ns), ns);                     \
    context.addDef(new NegOpDef(type, "oper -", ns), ns);                         \
    context.addDef(new BitNotOpDef(type, "oper ~", ns), ns);                      \
    context.addDef(new OrOpDef(type, 0, ns), ns);                                 \
    context.addDef(new AndOpDef(type, 0, ns), ns);                                \
    context.addDef(new XorOpDef(type, 0, ns), ns);                                \
    context.addDef(new ShlOpDef(type, 0, ns), ns);                                \
    context.addDef(new shift##ShrOpDef(type, 0, ns), ns);

    INTOPS(byteType, U, L, 0)
    INTOPS(int16Type, S, A, 0)
    INTOPS(uint16Type, U, L, 0)
    INTOPS(int32Type, S, A, 0)
    INTOPS(uint32Type, U, L, 0)
    INTOPS(int64Type, S, A, 0)
    INTOPS(uint64Type, U, L, 0)

    // float operations
#define FLOPS(type, ns) \
    context.addDef(new FAddOpDef(type, 0, ns), ns);                           \
    context.addDef(new FSubOpDef(type, 0, ns), ns);                           \
    context.addDef(new FMulOpDef(type, 0, ns), ns);                           \
    context.addDef(new FDivOpDef(type, 0, ns), ns);                           \
    context.addDef(new FRemOpDef(type, 0, ns), ns);                           \
    context.addDef(new FCmpOEQOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FCmpONEOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FCmpOGTOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FCmpOLTOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FCmpOGEOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FCmpOLEOpDef(type, boolType, ns), ns);                 \
    context.addDef(new FNegOpDef(type, "oper -", ns), ns);

    FLOPS(float32Type, 0)
    FLOPS(float64Type, 0)

// Reverse integer operations
#define REVINTOPS(type, signed, shift) \
    context.addDef(new AddROpDef(type, 0, true, true), type);               \
    context.addDef(new SubROpDef(type, 0, true, true), type);               \
    context.addDef(new MulROpDef(type, 0, true, true), type);               \
    context.addDef(new signed##DivROpDef(type, 0, true, true), type);       \
    context.addDef(new signed##RemROpDef(type, 0, true, true), type);       \
    context.addDef(new ICmpEQROpDef(type, boolType, true, true), type);     \
    context.addDef(new ICmpNEROpDef(type, boolType, true, true), type);     \
    context.addDef(new ICmpSGTROpDef(type, boolType, true, true), type);    \
    context.addDef(new ICmpSLTROpDef(type, boolType, true, true), type);    \
    context.addDef(new ICmpSGEROpDef(type, boolType, true, true), type);    \
    context.addDef(new ICmpSLEROpDef(type, boolType, true, true), type);    \
    context.addDef(new OrROpDef(type, 0, true, true), type);                \
    context.addDef(new AndROpDef(type, 0, true, true), type);               \
    context.addDef(new XorROpDef(type, 0, true, true), type);               \
    context.addDef(new ShlROpDef(type, 0, true, true), type);               \
    context.addDef(new shift##ShrROpDef(type, 0, true, true), type);

// reverse floating point operations
#define REVFLOPS(type) \
    context.addDef(new FAddROpDef(type, 0, true, true), type);               \
    context.addDef(new FSubROpDef(type, 0, true, true), type);               \
    context.addDef(new FMulROpDef(type, 0, true, true), type);               \
    context.addDef(new FDivROpDef(type, 0, true, true), type);               \
    context.addDef(new FRemROpDef(type, 0, true, true), type);               \
    context.addDef(new FCmpOEQROpDef(type, boolType, true, true), type);     \
    context.addDef(new FCmpONEROpDef(type, boolType, true, true), type);     \
    context.addDef(new FCmpOGTROpDef(type, boolType, true, true), type);     \
    context.addDef(new FCmpOLTROpDef(type, boolType, true, true), type);     \
    context.addDef(new FCmpOGEROpDef(type, boolType, true, true), type);     \
    context.addDef(new FCmpOLEROpDef(type, boolType, true, true), type);

    // PDNT operations need to be methods so that we try to resolve them with
    // type conversions prior to attempting the general methods and _only if_
    // one of the arguments is a PDNT.  We also define reverse operations for
    // them for the same reason.
    INTOPS(intType, S, A, intType)
    REVINTOPS(intType, S, A)
    INTOPS(uintType, U, L, uintType)
    REVINTOPS(uintType, U, L)
    INTOPS(intzType, S, A, intzType)
    REVINTOPS(intzType, S, A)
    INTOPS(uintzType, U, L, uintzType)
    REVINTOPS(intType, U, L)
    FLOPS(floatType, floatType)
    REVFLOPS(floatType)

    // atomic types
    context.addDef(new AtomicAddOpDef(intzType, 0, true), atomicType);
    context.addDef(new AtomicSubOpDef(intzType, 0, true), atomicType);
    context.addDef(new AtomicLoadOpDef(intzType, "oper to .builtin.intz"),
                   atomicType
                   );
    if (ptrIs32Bit == intIs32Bit) {
        context.addDef(new AtomicLoadOpDef(intType, "oper to .builtin.int"),
                    atomicType
                    );
        context.addDef(new AtomicLoadOpDef(uintType, "oper to .builtin.uint"),
                    atomicType
                    );
    } else if (!ptrIs32Bit && intIs32Bit) {
        context.addDef(new AtomicLoadTruncOpDef(intType,
                                                "oper to .builtin.int"
                                                ),
                       atomicType
                       );
        context.addDef(new AtomicLoadTruncOpDef(uintType,
                                                "oper to .builtin.uint"
                                                ),
                       atomicType
                       );
    } else {
        SPUG_CHECK(false,
                   "This platform has 32-bit integers but 64 bit pointers, "
                    "which the crack executor can't deal with.  We need to "
                    "implement a ZExt atomic load operation to accomodate it."
                   );
    }

    // boolean logic
    context.addDef(new LogicAndOpDef(boolType, boolType));
    context.addDef(new LogicOrOpDef(boolType, boolType));

    // implicit conversions (no loss of precision)
    context.addDef(new ZExtOpDef(int16Type, "oper to .builtin.int16"), byteType);
    context.addDef(new ZExtOpDef(int32Type, "oper to .builtin.int32"), byteType);
    context.addDef(new ZExtOpDef(int64Type, "oper to .builtin.int64"), byteType);
    context.addDef(new ZExtOpDef(uint16Type, "oper to .builtin.uint16"), byteType);
    context.addDef(new ZExtOpDef(uint32Type, "oper to .builtin.uint32"), byteType);
    context.addDef(new ZExtOpDef(uint64Type, "oper to .builtin.uint64"), byteType);

    context.addDef(new ZExtOpDef(int32Type, "oper to .builtin.int32"), int16Type);
    context.addDef(new ZExtOpDef(int64Type, "oper to .builtin.int64"), int16Type);
    context.addDef(new ZExtOpDef(int32Type, "oper to .builtin.int32"), uint16Type);
    context.addDef(new ZExtOpDef(int64Type, "oper to .builtin.int64"), uint16Type);
    context.addDef(new ZExtOpDef(uint32Type, "oper to .builtin.uint32"), uint16Type);
    context.addDef(new ZExtOpDef(uint64Type, "oper to .builtin.uint64"), uint16Type);

    context.addDef(new UIToFPOpDef(float32Type, "oper to .builtin.float32"), byteType);
    context.addDef(new UIToFPOpDef(float64Type, "oper to .builtin.float64"), byteType);

    context.addDef(new SIToFPOpDef(float32Type, "oper to .builtin.float32"), int16Type);
    context.addDef(new SIToFPOpDef(float64Type, "oper to .builtin.float64"), int16Type);
    context.addDef(new UIToFPOpDef(float32Type, "oper to .builtin.float32"), uint16Type);
    context.addDef(new UIToFPOpDef(float64Type, "oper to .builtin.float64"), uint16Type);

    context.addDef(new SExtOpDef(int64Type, "oper to .builtin.int64"), int32Type);
    context.addDef(new SIToFPOpDef(float64Type, "oper to .builtin.float64"), int32Type);
    context.addDef(new ZExtOpDef(uint64Type, "oper to .builtin.uint64"), uint32Type);
    context.addDef(new ZExtOpDef(int64Type, "oper to .builtin.int64"), uint32Type);
    context.addDef(new UIToFPOpDef(float64Type, "oper to .builtin.float64"), uint32Type);
    context.addDef(new FPExtOpDef(float64Type, "oper to .builtin.float64"), float32Type);

    // implicit conversions from UNTs to PDNTs
    context.addDef(new ZExtOpDef(intType, "oper to .builtin.int"), byteType);
    context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), byteType);
    context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), byteType);
    context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), byteType);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), byteType);

    context.addDef(new ZExtOpDef(intType, "oper to .builtin.int"), uint16Type);
    context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), uint16Type);
    context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), uint16Type);
    context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), uint16Type);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uint16Type);

    context.addDef(new ZExtOpDef(intType, "oper to .builtin.int"), int16Type);
    context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), int16Type);
    context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), int16Type);
    context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), int16Type);
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), int16Type);


    if (intIs32Bit) {
        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), int32Type);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), int32Type);
        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), uint32Type);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), uint32Type);
        context.addDef(new TruncOpDef(intType, "oper to .builtin.int"), int64Type);
        context.addDef(new TruncOpDef(uintType, "oper to .builtin.uint"), int64Type);
        context.addDef(new TruncOpDef(intType, "oper to .builtin.int"), uint64Type);
        context.addDef(new TruncOpDef(uintType, "oper to .builtin.uint"), uint64Type);
    } else {
        context.addDef(new SExtOpDef(intType, "oper to .builtin.int"), int32Type);
        context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), int32Type);
        context.addDef(new ZExtOpDef(intType, "oper to .builtin.int"), uint32Type);
        context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), uint32Type);
        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), int64Type);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), int64Type);
        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), uint64Type);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), uint64Type);
    }
    if (ptrIs32Bit) {
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), int32Type);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), int32Type);
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), uint32Type);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), uint32Type);
        context.addDef(new TruncOpDef(intzType, "oper to .builtin.intz"), int64Type);
        context.addDef(new TruncOpDef(uintzType, "oper to .builtin.uintz"), int64Type);
        context.addDef(new TruncOpDef(intzType, "oper to .builtin.intz"), uint64Type);
        context.addDef(new TruncOpDef(uintzType, "oper to .builtin.uintz"), uint64Type);
    } else {
        context.addDef(new SExtOpDef(intzType, "oper to .builtin.intz"), int32Type);
        context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), int32Type);
        context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), uint32Type);
        context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), uint32Type);
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), int64Type);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), int64Type);
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), uint64Type);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), uint64Type);
    }
    if (floatIs32Bit) {
        context.addDef(new NoOpDef(floatType, "oper to .builtin.float"), float32Type);
        context.addDef(new FPTruncOpDef(floatType, "oper to .builtin.float"),
                       float64Type
                       );
        context.addDef(new FPExtOpDef(float64Type, "oper to .builtin.float64"),
                       floatType
                       );
    } else {
        context.addDef(new FPExtOpDef(floatType, "oper to .builtin.float"),
                       float32Type
                       );
        context.addDef(new NoOpDef(floatType, "oper to .builtin.float"), float64Type);
        context.addDef(new NoOpDef(float64Type, "oper to .builtin.float64"), floatType);
    }
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), int16Type);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uint16Type);
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), int32Type);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uint32Type);
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), int64Type);
    context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), uint64Type);
    context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), uint64Type);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uint64Type);
    context.addDef(new FPToSIOpDef(intType, "oper to .builtin.int"), float32Type);
    context.addDef(new FPToUIOpDef(uintType, "oper to .builtin.uint"), float32Type);
    context.addDef(new FPToSIOpDef(intzType, "oper to .builtin.intz"), float32Type);
    context.addDef(new FPToUIOpDef(uintzType, "oper to .builtin.uintz"), float32Type);
    context.addDef(new FPToSIOpDef(intType, "oper to .builtin.int"), float64Type);
    context.addDef(new FPToUIOpDef(uintType, "oper to .builtin.uint"), float64Type);
    context.addDef(new FPToSIOpDef(intzType, "oper to .builtin.intz"), float64Type);
    context.addDef(new FPToUIOpDef(uintzType, "oper to .builtin.uintz"), float64Type);

    // implicit conversion from PDNTs to UNTs
    if (intIs32Bit) {
        context.addDef(new SExtOpDef(int64Type, "oper to .builtin.int64"), intType);
        context.addDef(new ZExtOpDef(uint64Type, "oper to .builtin.uint64"), uintType);
    } else {
        context.addDef(new NoOpDef(int64Type, "oper to .builtin.int64"), intType);
        context.addDef(new NoOpDef(uint64Type, "oper to .builtin.uint64"), uintType);
    }
    if (ptrIs32Bit) {
        context.addDef(new SExtOpDef(int64Type, "oper to .builtin.int64"), intzType);
        context.addDef(new ZExtOpDef(uint64Type, "oper to .builtin.uint64"), uintzType);
    } else {
        context.addDef(new NoOpDef(int64Type, "oper to .builtin.int64"), intzType);
        context.addDef(new NoOpDef(uint64Type, "oper to .builtin.uint64"), uintzType);
    }
    if (floatIs32Bit)
        context.addDef(new FPExtOpDef(float64Type, "oper to .builtin.float64"),
                       floatType
                       );
    else
        context.addDef(new NoOpDef(float64Type, "oper to .builtin.float64"), floatType);
    context.addDef(new UIToFPOpDef(float32Type, "oper to .builtin.float32"), uintType);
    context.addDef(new SIToFPOpDef(float32Type, "oper to .builtin.float32"), intType);
    context.addDef(new UIToFPOpDef(float64Type, "oper to .builtin.float64"), uintType);
    context.addDef(new SIToFPOpDef(float64Type, "oper to .builtin.float64"), intType);
    context.addDef(new UIToFPOpDef(float32Type, "oper to .builtin.float32"), uintzType);
    context.addDef(new SIToFPOpDef(float32Type, "oper to .builtin.float32"), intzType);
    context.addDef(new UIToFPOpDef(float64Type, "oper to .builtin.float64"), uintzType);
    context.addDef(new SIToFPOpDef(float64Type, "oper to .builtin.float64"), intzType);

    // implicit conversion from PDNTs to other PDNTs
    context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), intType);
    context.addDef(new NoOpDef(intType, "oper to .builtin.int"), uintType);
    context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), intzType);
    context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), uintzType);

    if (intIs32Bit == ptrIs32Bit) {
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), intType);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), intType);
        context.addDef(new NoOpDef(intzType, "oper to .builtin.intz"), uintType);
        context.addDef(new NoOpDef(uintzType, "oper to .builtin.uintz"), uintType);

        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), intzType);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), intzType);
        context.addDef(new NoOpDef(intType, "oper to .builtin.int"), uintzType);
        context.addDef(new NoOpDef(uintType, "oper to .builtin.uint"), uintzType);
    } else if (intIs32Bit) {
        context.addDef(new SExtOpDef(intzType, "oper to .builtin.intz"), intType);
        context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), intType);
        context.addDef(new ZExtOpDef(intzType, "oper to .builtin.intz"), uintType);
        context.addDef(new ZExtOpDef(uintzType, "oper to .builtin.uintz"), uintType);

        context.addDef(new TruncOpDef(intType, "oper to .builtin.int"), intzType);
        context.addDef(new TruncOpDef(uintType, "oper to .builtin.uint"), intzType);
        context.addDef(new TruncOpDef(intType, "oper to .builtin.int"), uintzType);
        context.addDef(new TruncOpDef(uintType, "oper to .builtin.uint"), uintzType);
    } else if (ptrIs32Bit) {
        // integer is wider than a pointer?  Not very likely, but just in
        // case...
        context.addDef(new TruncOpDef(intzType, "oper to .builtin.intz"), intType);
        context.addDef(new TruncOpDef(uintzType, "oper to .builtin.uintz"), intType);
        context.addDef(new TruncOpDef(intzType, "oper to .builtin.intz"), uintType);
        context.addDef(new TruncOpDef(uintzType, "oper to .builtin.uintz"), uintType);

        context.addDef(new SExtOpDef(intType, "oper to .builtin.int"), intzType);
        context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), intzType);
        context.addDef(new ZExtOpDef(intType, "oper to .builtin.int"), uintzType);
        context.addDef(new ZExtOpDef(uintType, "oper to .builtin.uint"), uintzType);
    }
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), intType);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uintType);
    context.addDef(new SIToFPOpDef(floatType, "oper to .builtin.float"), intzType);
    context.addDef(new UIToFPOpDef(floatType, "oper to .builtin.float"), uintzType);
    context.addDef(new FPToUIOpDef(intType, "oper to .builtin.int"), floatType);
    context.addDef(new FPToUIOpDef(uintType, "oper to .builtin.uint"), floatType);
    context.addDef(new FPToUIOpDef(intzType, "oper to .builtin.intz"), floatType);
    context.addDef(new FPToUIOpDef(uintzType, "oper to .builtin.uintz"), floatType);

    // add the increment and decrement operators
    context.addDef(new PreIncrIntOpDef(byteType, "oper ++x"), byteType);
    context.addDef(new PreIncrIntOpDef(int16Type, "oper ++x"), int16Type);
    context.addDef(new PreIncrIntOpDef(uint16Type, "oper ++x"), uint16Type);
    context.addDef(new PreIncrIntOpDef(int32Type, "oper ++x"), int32Type);
    context.addDef(new PreIncrIntOpDef(uint32Type, "oper ++x"), uint32Type);
    context.addDef(new PreIncrIntOpDef(int64Type, "oper ++x"), int64Type);
    context.addDef(new PreIncrIntOpDef(uint64Type, "oper ++x"), uint64Type);
    context.addDef(new PreIncrIntOpDef(intType, "oper ++x"), intType);
    context.addDef(new PreIncrIntOpDef(uintType, "oper ++x"), uintType);
    context.addDef(new PreIncrIntOpDef(intzType, "oper ++x"), intzType);
    context.addDef(new PreIncrIntOpDef(uintzType, "oper ++x"), uintzType);
    context.addDef(new PreIncrPtrOpDef(byteptrType, "oper ++x"), byteptrType);

    context.addDef(new PreDecrIntOpDef(byteType, "oper --x"), byteType);
    context.addDef(new PreDecrIntOpDef(int16Type, "oper --x"), int16Type);
    context.addDef(new PreDecrIntOpDef(uint16Type, "oper --x"), uint16Type);
    context.addDef(new PreDecrIntOpDef(int32Type, "oper --x"), int32Type);
    context.addDef(new PreDecrIntOpDef(uint32Type, "oper --x"), uint32Type);
    context.addDef(new PreDecrIntOpDef(int64Type, "oper --x"), int64Type);
    context.addDef(new PreDecrIntOpDef(uint64Type, "oper --x"), uint64Type);
    context.addDef(new PreDecrIntOpDef(intType, "oper --x"), intType);
    context.addDef(new PreDecrIntOpDef(uintType, "oper --x"), uintType);
    context.addDef(new PreDecrIntOpDef(intzType, "oper --x"), intzType);
    context.addDef(new PreDecrIntOpDef(uintzType, "oper --x"), uintzType);
    context.addDef(new PreDecrPtrOpDef(byteptrType, "oper --x"), byteptrType);

    context.addDef(new PostIncrIntOpDef(byteType, "oper x++"), byteType);
    context.addDef(new PostIncrIntOpDef(int16Type, "oper x++"), int16Type);
    context.addDef(new PostIncrIntOpDef(uint16Type, "oper x++"), uint16Type);
    context.addDef(new PostIncrIntOpDef(int32Type, "oper x++"), int32Type);
    context.addDef(new PostIncrIntOpDef(uint32Type, "oper x++"), uint32Type);
    context.addDef(new PostIncrIntOpDef(int64Type, "oper x++"), int64Type);
    context.addDef(new PostIncrIntOpDef(uint64Type, "oper x++"), uint64Type);
    context.addDef(new PostIncrIntOpDef(intType, "oper x++"), intType);
    context.addDef(new PostIncrIntOpDef(uintType, "oper x++"), uintType);
    context.addDef(new PostIncrIntOpDef(intzType, "oper x++"), intzType);
    context.addDef(new PostIncrIntOpDef(uintzType, "oper x++"), uintzType);
    context.addDef(new PostIncrPtrOpDef(byteptrType, "oper x++"), byteptrType);

    context.addDef(new PostDecrIntOpDef(byteType, "oper x--"), byteType);
    context.addDef(new PostDecrIntOpDef(int16Type, "oper x--"), int16Type);
    context.addDef(new PostDecrIntOpDef(uint16Type, "oper x--"), uint16Type);
    context.addDef(new PostDecrIntOpDef(int32Type, "oper x--"), int32Type);
    context.addDef(new PostDecrIntOpDef(uint32Type, "oper x--"), uint32Type);
    context.addDef(new PostDecrIntOpDef(int64Type, "oper x--"), int64Type);
    context.addDef(new PostDecrIntOpDef(uint64Type, "oper x--"), uint64Type);
    context.addDef(new PostDecrIntOpDef(intType, "oper x--"), intType);
    context.addDef(new PostDecrIntOpDef(uintType, "oper x--"), uintType);
    context.addDef(new PostDecrIntOpDef(intzType, "oper x--"), intzType);
    context.addDef(new PostDecrIntOpDef(uintzType, "oper x--"), uintzType);
    context.addDef(new PostDecrPtrOpDef(byteptrType, "oper x--"), byteptrType);

    // explicit no-op construction
    addNopNew(context, int64Type);
    addNopNew(context, uint64Type);
    addNopNew(context, int32Type);
    addNopNew(context, uint32Type);
    addNopNew(context, int16Type);
    addNopNew(context, uint16Type);
    addNopNew(context, byteType);
    addNopNew(context, float32Type);
    addNopNew(context, float64Type);
    addNopNew(context, intType);
    addNopNew(context, uintType);
    addNopNew(context, intzType);
    addNopNew(context, uintzType);
    addNopNew(context, floatType);

    // explicit (loss of precision)
    addExplicitTruncate(context, int64Type, uint64Type);
    addExplicitTruncate(context, int64Type, int32Type);
    addExplicitTruncate(context, int64Type, uint32Type);
    addExplicitTruncate(context, int64Type, int16Type);
    addExplicitTruncate(context, int64Type, uint16Type);
    addExplicitTruncate(context, int64Type, byteType);

    addExplicitTruncate(context, uint64Type, int64Type);
    addExplicitTruncate(context, uint64Type, int32Type);
    addExplicitTruncate(context, uint64Type, uint32Type);
    addExplicitTruncate(context, uint64Type, int16Type);
    addExplicitTruncate(context, uint64Type, uint16Type);
    addExplicitTruncate(context, uint64Type, byteType);

    addExplicitTruncate(context, int32Type, byteType);
    addExplicitTruncate(context, int32Type, uint16Type);
    addExplicitTruncate(context, int32Type, int16Type);
    addExplicitTruncate(context, int32Type, uint32Type);

    addExplicitTruncate(context, uint32Type, byteType);
    addExplicitTruncate(context, uint32Type, int16Type);
    addExplicitTruncate(context, uint32Type, uint16Type);
    addExplicitTruncate(context, uint32Type, int32Type);

    addExplicitTruncate(context, int16Type, byteType);
    addExplicitTruncate(context, int16Type, uint16Type);

    addExplicitTruncate(context, uint16Type, byteType);
    addExplicitTruncate(context, uint16Type, int16Type);

    addExplicitTruncate(context, intType, int16Type);
    addExplicitTruncate(context, intType, uint16Type);
    addExplicitTruncate(context, intType, int32Type);
    addExplicitTruncate(context, intType, uint32Type);
    addExplicitTruncate(context, intType, byteType);

    addExplicitTruncate(context, uintType, int16Type);
    addExplicitTruncate(context, uintType, uint16Type);
    addExplicitTruncate(context, uintType, int32Type);
    addExplicitTruncate(context, uintType, uint32Type);
    addExplicitTruncate(context, uintType, byteType);

    addExplicitFPTruncate<FPTruncOpCall>(context, float64Type, float32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(context, float32Type, int16Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, uint16Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float32Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float32Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, uint64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(context, float64Type, int16Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, uint16Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float64Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float64Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, uint64Type);

    addExplicitFPTruncate<SIToFPOpCall>(context, int64Type, float32Type);
    addExplicitFPTruncate<SIToFPOpCall>(context, uint64Type, float32Type);
    addExplicitFPTruncate<SIToFPOpCall>(context, int64Type, float64Type);
    addExplicitFPTruncate<SIToFPOpCall>(context, uint64Type, float64Type);

    // create the array generic
    TypeDefPtr arrayType = new ArrayTypeDef(context.construct->classType.get(),
                                            "array",
                                            0
                                            );
    gd->arrayType = arrayType;
    context.addDef(arrayType.get());
    deferMetaClass.push_back(arrayType);

    // create the raw function type
    // "oper call" methods are added as this type is specialized during parse
    TypeDefPtr functionType = new FunctionTypeDef(
                                            context.construct->classType.get(),
                                            "function",
                                            0
                                            );
    context.addDef(functionType.get());
    gd->functionType = functionType;
    deferMetaClass.push_back(functionType);

    {
        // add the exception structure type (this is required for creating any
        // kind of function)
        vector<Type *> elems(2);
        elems[0] = builder.getInt8PtrTy();
        elems[1] = builder.getInt32Ty();
        exStructType = new BTypeDef(0, ":ExStruct",
                                    StructType::get(lctx, elems),
                                    false
                                    );
        context.addDef(exStructType.get());
        deferMetaClass.push_back(exStructType);
    }

    // now that we have byteptr and array and all of the integer types, we can
    // initialize the body of Class (the meta-type) and create an
    // implementation object for it.
    context.addDef(new IsOpDef(classType, boolType));
    finishClassType(context, classType);
    createClassImpl(context, classType);

    // back fill the meta-class for the types defined so far.  We create a
    // bogus context for them because functions called by fixMeta() expect to
    // run on a class context.
    BTypeDefPtr bogusType = new BTypeDef(metaType.get(), "BogusType", 0);
    ContextPtr classCtx = context.createSubContext(Context::instance,
                                                   bogusType.get()
                                                   );
    for (int i = 0; i < deferMetaClass.size(); ++i)
        fixMeta(*classCtx, deferMetaClass[i].get());
    deferMetaClass.clear();

    // create OverloadDef's type
    metaType = createMetaClass(context, "Overload");
    BTypeDefPtr overloadDef = new BTypeDef(metaType.get(), "Overload", 0);
    metaType->meta = overloadDef.get();
    createClassImpl(context, overloadDef.get());

    // Give it a context and an "oper to .builtin.voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        overloadDef.get()
    );
    context.addDef(overloadDef.get());
    overloadDef->createEmptyOffsetsInitializer(context);

    // create an empty structure type and its pointer for VTableBase
    // Actual type is {}** (another layer of pointer indirection) because
    // classes need to be pointer types.
    vector<Type *> members;
    string vtableTypeName = ".builtin.VTableBase";
    StructType *vtableType = getLLVMType(vtableTypeName);
    if (!vtableType) {
        vtableType = StructType::create(getGlobalContext(), members,
                                        vtableTypeName
                                        );
        putLLVMType(vtableTypeName, vtableType);
    }
    Type *vtablePtrType = PointerType::getUnqual(vtableType);
    metaType = createMetaClass(context, "VTableBase");
    BTypeDef *vtableBaseType;
    gd->vtableBaseType = vtableBaseType =
        new BTypeDef(metaType.get(), "VTableBase",
                     PointerType::getUnqual(vtablePtrType),
                     true
                     );
    vtableBaseType->hasVTable = true;
    vtableBaseType->defaultInitializer = new NullConst(vtableBaseType);
    createClassImpl(context, vtableBaseType);
    metaType->meta = vtableBaseType;
    context.addDef(vtableBaseType);
    context.construct->registerDef(vtableBaseType);
    createOperClassFunc(context, vtableBaseType);

    // Add an "oper to .builtin.voidptr" method.
    context.addDef(new VoidPtrOpDef(voidptrType), vtableBaseType);

    // build VTableBase's vtable
    VTableBuilder vtableBuilder(this, vtableBaseType);
    vtableBaseType->createAllVTables(vtableBuilder, ".vtable.VTableBase",
                                     vtableBaseType
                                     );
    vtableBuilder.emit(vtableBaseType);

    // finally, mark the class as complete
    vtableBaseType->complete = true;
    vtableBaseType->fixIncompletes(context);

    // pointer equality check (to allow checking for None)
    context.addDef(new IsOpDef(voidptrType, boolType));
    context.addDef(new IsOpDef(byteptrType, boolType));

    // boolean not
    context.addDef(new BitNotOpDef(boolType, "oper !"));

    // byteptr array indexing
    addArrayMethods(context, byteptrType, byteType);

    // bind the module to the execution engine
    engineBindModule(builtinMod.get());
    engineFinishModule(context, builtinMod.get());

    if (options->statsMode) {
        context.construct->stats->setState(ConstructStats::start);
        context.construct->stats->setModule(NULL);
    }

    if (debugInfo)
        delete debugInfo;

    moduleDef = 0;
    return builtinMod;
}

void LLVMBuilder::initialize(Context &context) {
    createLLVMModule(".root");
    moduleDef = instantiateModule(context, ".root", module);
}

std::string LLVMBuilder::getSourcePath(const std::string &path) {
    char *rp = realpath(path.c_str(), NULL);
    string result;
    if (!rp) {
        result = path;
    }
    else {
        result = rp;
        free(rp);
    }
    return result;

}

void LLVMBuilder::initializeImport(model::ModuleDef *imported,
                                   const ImportedDefVec &symbols
                                   ) {

    BModuleDef *importedMod = dynamic_cast<BModuleDef*>(imported);

    assert(importedMod && "importedMod was not a BModuleDef");
    assert(importedMod->getFullName().find('[') == -1);

    string importedMainName = imported->name + ":main";
    Constant *fc =
        module->getOrInsertFunction(importedMainName,
                                    Type::getVoidTy(getGlobalContext()),
                                    NULL
                                    );
    Function *f = llvm::cast<llvm::Function>(fc);
    builder.CreateCall(f);
}

void LLVMBuilder::createModuleCommon(Context &context) {

    // name some structs in this module
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    BTypeDef *vtableBaseType = BTypeDefPtr::arcast(
                                  context.construct->vtableBaseType);

    // all of the "extern" primitive functions have to be created in each of
    // the modules - we can not directly reference across modules.

    //BTypeDef *int32Type = BTypeDefPtr::arcast(context.construct->int32Type);
    //BTypeDef *int64Type = BTypeDefPtr::arcast(context.construct->int64Type);
    //BTypeDef *uint64Type = BTypeDefPtr::arcast(context.construct->uint64Type);
    BTypeDef *intType = BTypeDefPtr::arcast(context.construct->intType);
    BTypeDef *uintzType = BTypeDefPtr::arcast(context.construct->uintzType);
    BTypeDef *voidType = BTypeDefPtr::arcast(context.construct->voidType);
    //BTypeDef *float32Type = BTypeDefPtr::arcast(context.construct->float32Type);
    BTypeDef *byteptrType =
        BTypeDefPtr::arcast(context.construct->byteptrType);
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);

    // create "void *calloc(uint size)"
    {
        FuncBuilder f(context, FuncDef::builtin, voidptrType, "calloc", 2);
        f.addArg("size", uintzType);
        f.addArg("size", uintzType);
        f.setSymbolName("calloc");
        f.finish();
        callocFunc = f.funcDef->getFuncRep(*this);
    }

    // create "array[byteptr] __getArgv()"
    {
        TypeDefPtr array = context.ns->lookUp("array");
        assert(array.get() && "array not defined in context");
        TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
        types->push_back(byteptrType);
        TypeDefPtr arrayOfByteptr =
            array->getSpecialization(context, types.get());
        FuncBuilder f(context, FuncDef::builtin,
                      BTypeDefPtr::arcast(arrayOfByteptr),
                      "__getArgv",
                      0
                      );
        f.setSymbolName("__getArgv");
        f.finish();
    }

    // create "int __getArgc()"
    {
        FuncBuilder f(context, FuncDef::builtin, intType, "__getArgc", 0);
        f.setSymbolName("__getArgc");
        f.finish();
    }

    // create "__CrackThrow(VTableBase)"
    {
        FuncBuilder f(context, FuncDef::builtin, voidType, "__CrackThrow", 1);
        f.addArg("exception", vtableBaseType);
        f.setSymbolName("__CrackThrow");
        f.finish();
    }

    // create "__CrackGetException(voidptr)"
    {
        FuncBuilder f(context, FuncDef::builtin, voidptrType,
                      "__CrackGetException",
                      1
                      );
        f.addArg("exceptionObject", byteptrType);
        f.setSymbolName("__CrackGetException");
        f.finish();
    }

    // create "__CrackBadCast(Class a, Class b)"
    {
        FuncBuilder f(context, FuncDef::builtin, voidType,
                      "__CrackBadCast",
                      2
                      );
        f.addArg("curType", classType);
        f.addArg("newType", classType);
        f.setSymbolName("__CrackBadCast");
        f.finish();
    }

    // create "__CrackCleanupException(voidptr exceptionObject)"
    {
        FuncBuilder f(context, FuncDef::builtin, voidType,
                      "__CrackCleanupException",
                      1
                      );
        f.addArg("exceptionObject", voidptrType);
        f.setSymbolName("__CrackCleanupException");
        f.finish();
    }

    // create "__CrackExceptionFrame()"
    {
        FuncBuilder f(context, FuncDef::builtin, voidType,
                      "__CrackExceptionFrame",
                      0
                      );
        f.setSymbolName("__CrackExceptionFrame");
        f.finish();
    }
}

void *LLVMBuilder::loadSharedLibrary(const std::string &name) {
    // leak the handle so the library stays mapped for the life of the process.
    void *handle = dlopen(name.c_str(), RTLD_LAZY|RTLD_GLOBAL);
    if (!handle)
        throw spug::Exception(dlerror());
    return handle;
}

void LLVMBuilder::importSharedLibrary(const string &name,
                                      const ImportedDefVec &symbols,
                                      Context &context,
                                      Namespace *ns
                                      ) {
    // see if we've loaded this shared library before.
    SharedLibDefPtr shlibMod;
    void *handle;
    {
        SharedLibMap &libs = getSharedLibs();
        SharedLibMap::iterator i = libs.find(name);
        if (i != libs.end()) {
            shlibMod = i->second;
            handle = shlibMod->handle;
        } else {
            handle = loadSharedLibrary(name);
            shlibMod = new SharedLibDef(name, handle);
            libs.insert(make_pair(name, shlibMod));
        }
    }

    for (ImportedDefVec::const_iterator iter = symbols.begin();
         iter != symbols.end();
         ++iter
         ) {

        void *sym = dlsym(handle, iter->source.c_str());
        if (!sym)
            throw spug::Exception(dlerror());
        recordShlibSym(iter->source);

        // save for caching (when called from parser). when called from Cacher,
        // we don't save (and don't need to since we're already cached)
        if (moduleDef)
            moduleDef->shlibImportList[name] = symbols;

        // store a stub for the symbol
        StubDefPtr stub = new StubDef(context.construct->voidType.get(),
                                      iter->local,
                                      sym
                                      );
        shlibMod->addDef(stub.get());
        if (ns)
            ns->addAlias(stub.get());
    }
}

void LLVMBuilder::registerImportedDef(Context &context, VarDef *varDef) {
    // no-op for LLVM builder.
}

void LLVMBuilder::setArgv(int newArgc, char **newArgv) {
    argc = newArgc;
    argv = newArgv;
}

void LLVMBuilder::emitMemVarRef(Context &context, Value *val) {
    lastValue = builder.CreateLoad(val);
}

void LLVMBuilder::emitArgVarRef(Context &context, Value *val) {
    lastValue = val;
}

void LLVMBuilder::emitVTableInit(Context &context, TypeDef *typeDef) {
    BTypeDef *btype = BTypeDefPtr::cast(typeDef);
    BTypeDef *vtableBaseType =
        BTypeDefPtr::arcast(context.construct->vtableBaseType);
    PlaceholderInstruction *vtableInit =
        new IncompleteVTableInit(btype, lastValue, vtableBaseType,
                                 builder.GetInsertBlock()
                                 );
    // store it
    btype->addPlaceholder(vtableInit);
}

LLVMBuilder::TypeMap LLVMBuilder::llvmTypes;

StructType *LLVMBuilder::getLLVMType(const string &canonicalName) {
    TypeMap::iterator iter = llvmTypes.find(canonicalName);
    if (iter == llvmTypes.end())
        return 0;
    return iter->second;
}

void LLVMBuilder::putLLVMType(const string &canonicalName,
                              StructType *type
                              ) {
    pair<TypeMap::iterator, bool> result =
        llvmTypes.insert(pair<string, StructType *>(canonicalName, type));
    SPUG_CHECK(result.second,
               "Attempting to redefine LLVM type " << canonicalName
               );
}
