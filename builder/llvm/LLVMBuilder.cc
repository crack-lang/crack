// Copyright 2009-2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMBuilder.h"

#include "ArrayTypeDef.h"
#include "BBuilderContextData.h"
#include "BBranchPoint.h"
#include "BCleanupFrame.h"
#include "BFieldRef.h"
#include "BFuncDef.h"
#include "BModuleDef.h"
#include "BResultExpr.h"
#include "BTypeDef.h"
#include "Consts.h"
#include "ExceptionCleanupExpr.h"
#include "FuncBuilder.h"
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

#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Intrinsics.h>

#include <spug/Exception.h>
#include <spug/StringFmt.h>

#include <model/AllocExpr.h>
#include <model/AssignExpr.h>
#include <model/CompositeNamespace.h>
#include <model/Construct.h>
#include <model/InstVarDef.h>
#include <model/LocalNamespace.h>
#include <model/NullConst.h>
#include <model/OverloadDef.h>
#include <model/StubDef.h>
#include <model/TernaryExpr.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;

typedef model::FuncCall::ExprVec ExprVec;

// XXX find a way to remove this? see Incompletes.cc
const Type *llvmIntType = 0;

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
        classType->getPathToAncestor(
            *overridenClass, 
            funcBuilder.funcDef->pathToFirstDeclaration
        );
        
        // augment it with the path from the overriden to its first 
        // declaration.
        funcBuilder.funcDef->pathToFirstDeclaration.insert(
            funcBuilder.funcDef->pathToFirstDeclaration.end(),
            overriden->pathToFirstDeclaration.begin(),
            overriden->pathToFirstDeclaration.end()
        );

        // the type of the receiver is that of its first declaration                
        BTypeDef *receiverClass = 
            BTypeDefPtr::acast(overriden->getReceiverType());
        funcBuilder.setReceiverType(receiverClass);

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
        vector<const Type *> argTypes;
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
        BBuilderContextData *bdata;
        lexicalContext->builderData = bdata = new BBuilderContextData();

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

    void fixMeta(Context &context, TypeDef *type) {
        BTypeDefPtr metaType;
        BGlobalVarDefImplPtr classImpl;
        type->type = metaType = createMetaClass(context, type->getFullName());
        metaType->meta = type;
        createClassImpl(context, BTypeDefPtr::acast(type));
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

    BTypeDef *createIntPrimType(Context &context, const Type *llvmType,
                                const char *name
                                ) {
        BTypeDefPtr btype = new BTypeDef(context.construct->classType.get(),
                                         name,
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createIntConst(context, 0, btype.get());
        context.addDef(new BoolOpDef(context.construct->boolType.get(),
                                     "toBool"
                                     ),
                       btype.get()
                       );

        // if you remove this, for the love of god, change the return type so
        // we don't leak the pointer.
        context.addDef(btype.get());
        return btype.get();
    }

    BTypeDef *createFloatPrimType(Context &context, const Type *llvmType,
                                  const char *name
                                  ) {
        BTypeDefPtr btype = new BTypeDef(context.construct->classType.get(),
                                         name,
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createFloatConst(context, 0.0, btype.get());
        context.addDef(new FBoolOpDef(context.construct->boolType.get(),
                                      "toBool"
                                      ),
                       btype.get()
                       );

        // if you remove this, for the love of god, change the return type so
        // we don't leak the pointer.
        context.addDef(btype.get());
        return btype.get();
    }

    Value *getExceptionObjectValue(Context &context, IRBuilder<> &builder) {
        VarDefPtr exObj = context.ns->lookUp(":exceptionObject");
        BHeapVarDefImplPtr exObjImpl = BHeapVarDefImplPtr::rcast(exObj->impl);
        return builder.CreateLoad(exObjImpl->rep);
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
    getDeclaration(module, Intrinsic::eh_selector);
    getDeclaration(module, Intrinsic::eh_exception);
    
    // our exception personality function
    vector<const Type *> args(5);;
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

void LLVMBuilder::initializeMethodInfo(Context &context, FuncDef::Flags flags,
                                       FuncDef *existing,
                                       BTypeDef *&classType,
                                       FuncBuilder &funcBuilder
                                       ) {
    ContextPtr classCtx = context.getClassContext();
    assert(classCtx && "method is not nested in a class context.");
    classType = BTypeDefPtr::arcast(classCtx->ns);

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

Function *LLVMBuilder::getModFunc(FuncDef *funcDef) {
    ModFuncMap::iterator iter = moduleFuncs.find(funcDef);
    if (iter == moduleFuncs.end()) {
        // not found, create a new one and map it to the existing function 
        // pointer
        BFuncDef *bfuncDef = BFuncDefPtr::acast(funcDef);
        Function *func = Function::Create(bfuncDef->rep->getFunctionType(),
                                          Function::ExternalLinkage,
                                          bfuncDef->rep->getName(),
                                          module
                                          );

        // possibly do a global mapping (delegated to specific builder impl.)
        addGlobalFuncMapping(func, bfuncDef->rep);

        // low level symbol name
        if (!bfuncDef->symbolName.empty())
            func->setName(bfuncDef->symbolName);

        // cache it in the map
        moduleFuncs[bfuncDef] = func;
        return func;
    } else {
        return iter->second;
    }
}

GlobalVariable *LLVMBuilder::getModVar(model::VarDefImpl *varDefImpl) {
    ModVarMap::iterator iter = moduleVars.find(varDefImpl);
    if (iter == moduleVars.end()) {
        BGlobalVarDefImpl *bvar = BGlobalVarDefImplPtr::acast(varDefImpl);

        // extract the raw type
        const Type *type = bvar->rep->getType()->getElementType();

        GlobalVariable *global =
            new GlobalVariable(*module, type, bvar->rep->isConstant(),
                               GlobalValue::ExternalLinkage,
                               0, // initializer: null for externs
                               bvar->rep->getName()
                               );


        // possibly do a global mapping (delegated to specific builder impl.)
        addGlobalVarMapping(global, bvar->rep);

        moduleVars[varDefImpl] = global;
        return global;
    } else {
        return iter->second;
    }
}

TypeDef *LLVMBuilder::getFuncType(Context &context,
                                  const llvm::Type *llvmFuncType
                                  ) {

    // see if we've already got it
    FuncTypeMap::const_iterator iter = funcTypes.find(llvmFuncType);
    if (iter != funcTypes.end())
        return TypeDefPtr::rcast(iter->second);

    // nope.  create a new type object and store it
    BTypeDefPtr crkFuncType = new BTypeDef(context.construct->classType.get(),
                                           "",
                                           llvmFuncType
                                           );
    funcTypes[llvmFuncType] = crkFuncType;
    
    // Give it an "oper to voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        crkFuncType.get()
    );
    
    return crkFuncType.get();
}

BHeapVarDefImplPtr LLVMBuilder::createLocalVar(BTypeDef *tp, Value *&var,
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
    if (initVal)
        b.CreateStore(initVal, var);
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

LLVMBuilder::LLVMBuilder() :
    debugInfo(0),
    module(0),
    builder(getGlobalContext()),
    func(0),
    lastValue(0) {

}

ResultExprPtr LLVMBuilder::emitFuncCall(Context &context, FuncCall *funcCall) {

    BFuncDef *func = BFuncDefPtr::arcast(funcCall->func);

    // get the LLVM arg list from the receiver and the argument expressions
    vector<Value*> valueArgs;
    
    // if there's a receiver, use it as the first argument.
    Value *receiver;
    BFuncDef *funcDef = BFuncDefPtr::arcast(funcCall->func);
    if (funcCall->receiver) {
        funcCall->receiver->emit(context)->handleTransient(context);
        narrow(funcCall->receiver->type.get(), funcDef->getReceiverType());
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

    if (funcCall->virtualized)
        lastValue = IncompleteVirtualFunc::emitCall(context, funcDef, 
                                                    receiver,
                                                    valueArgs,
                                                    followingBlock,
                                                    cleanupBlock
                                                    );
    else {
        lastValue =
            builder.CreateInvoke(funcDef->getRep(*this), followingBlock, 
                                 cleanupBlock,
                                 valueArgs.begin(), 
                                 valueArgs.end()
                                 );

    }

    // continue emitting code into the new following block.
    if (followingBlock != cleanupBlock)
        builder.SetInsertPoint(followingBlock);

    return new BResultExpr(funcCall, lastValue);
}

ResultExprPtr LLVMBuilder::emitStrConst(Context &context, StrConst *val) {
    BStrConst *bval = BStrConstPtr::cast(val);
    // if the global string hasn't been defined yet, create it
    if (!bval->rep) {
        // we have to do this the hard way because strings may contain 
        // embedded nulls (IRBuilder.CreateGlobalStringPtr expects a 
        // null-terminated string)
        LLVMContext &llvmContext = getGlobalContext();
        Constant *llvmVal =
            ConstantArray::get(llvmContext, val->val, true);
        GlobalVariable *gvar = new GlobalVariable(*module,
                                                  llvmVal->getType(),
                                                  true, // is constant
                                                  GlobalValue::InternalLinkage,
                                                  llvmVal,
                                                 "str:"+
                                                  module->getModuleIdentifier(),
                                                  0,
                                                  false);
        
        Value *zero = ConstantInt::get(Type::getInt32Ty(llvmContext), 0);
        Value *args[] = { zero, zero };
        bval->rep = builder.CreateInBoundsGEP(gvar, args, args + 2);
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
    const PointerType *tp = cast<const PointerType>(btype->rep);
    
    // XXX mega-hack, clear the contents of the allocated memory (this is to 
    // get around the temporary lack of automatic member initialization)
    
    // calculate the size of instances of the type
    Value *null = Constant::getNullValue(tp);
    assert(llvmIntType && "integer type has not been initialized");
    Value *startPos = builder.CreatePtrToInt(null, llvmIntType);
    Value *endPos = 
        builder.CreatePtrToInt(
            builder.CreateConstGEP1_32(null, 1),
            llvmIntType
            );
    Value *size = builder.CreateSub(endPos, startPos);
    
    // if a count expression was supplied, emit it.  Otherwise, count is a 
    // constant 1
    Value *countVal;
    if (countExpr) {
        countExpr->emit(context)->handleTransient(context);
        countVal = lastValue;
    } else {
        countVal = ConstantInt::get(llvmIntType, 1);
    }
    
    // construct a call to the "calloc" function
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);
    vector<Value *> callocArgs(2);
    callocArgs[0] = countVal;
    callocArgs[1] = size;
    Value *result = builder.CreateCall(callocFunc, callocArgs.begin(), 
                                       callocArgs.end()
                                       );
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
    return labeledIf(context, cond, "true", "false");
}

BranchpointPtr LLVMBuilder::labeledIf(Context &context, Expr *cond,
                                      const char* tLabel,
                                      const char* fLabel) {

    // create blocks for the true and false conditions
    LLVMContext &lctx = getGlobalContext();
    BasicBlock *trueBlock = BasicBlock::Create(lctx, tLabel, func);

    BBranchpointPtr result = new BBranchpoint(
        BasicBlock::Create(lctx, fLabel, func)
    );

    context.createCleanupFrame();
    cond->emitCond(context);
    Value *condVal = lastValue; // condition value
    context.closeCleanupFrame();
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
                                   "tern_F"
                                   );
    BBranchpoint *bpos = BBranchpointPtr::arcast(pos);
    Value *condVal = lastValue; // arg[0] condition value
    BasicBlock *falseBlock = bpos->block; // false block
    BasicBlock *oBlock = bpos->block2; // condition block

    // now pointing to true block, save it for phi
    BasicBlock *trueBlock = builder.GetInsertBlock();
    
    // create the block after the expression
    LLVMContext &lctx = getGlobalContext();
    BasicBlock *postBlock = BasicBlock::Create(lctx, "after_tern", func);
    
    // emit the true expression in its own cleanup frame
    context.createCleanupFrame();
    expr->trueVal->emit(context);
    narrow(expr->trueVal->type.get(), expr->type.get());
    Value *trueVal = lastValue;
    context.closeCleanupFrame();
    
    // branch to the end
    builder.CreateBr(postBlock);
    
    // pick up changes to the block
    trueBlock = builder.GetInsertBlock();
    
    // emit the false expression 
    builder.SetInsertPoint(falseBlock);
    context.createCleanupFrame();
    expr->falseVal->emit(context);
    narrow(expr->falseVal->type.get(), expr->type.get());
    Value *falseVal = lastValue;
    context.closeCleanupFrame();
    builder.CreateBr(postBlock);
    falseBlock = builder.GetInsertBlock();

    // emit the phi
    builder.SetInsertPoint(postBlock);
    PHINode *p = builder.CreatePHI(
        BTypeDefPtr::arcast(expr->type)->rep,
        "tern_R"
    );
    p->addIncoming(trueVal, trueBlock);
    p->addIncoming(falseVal, falseBlock);
    lastValue = p;
    
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

void LLVMBuilder::createSpecialVar(Namespace *ns, TypeDef *type, 
                                   const string &name
                                   ) {
    Value *ptr;
    BTypeDef *tp = BTypeDefPtr::cast(type);
    VarDefPtr varDef = new VarDef(tp, name);
    varDef->impl = createLocalVar(tp, ptr);
    ptr->setName(name);
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

void LLVMBuilder::getInvokeBlocks(Context &context, 
                                  BasicBlock *&followingBlock,
                                  BasicBlock *&cleanupBlock
                                  ) {
    followingBlock = 0;
    BBuilderContextData *bdata;
    if (context.emittingCleanups &&
        (bdata = BBuilderContextDataPtr::rcast(context.builderData))
        )
        followingBlock = cleanupBlock = bdata->nextCleanupBlock;

    // otherwise, create a new normal destinatation block and get the cleanup 
    // block.
    if (!followingBlock) {
        followingBlock = BasicBlock::Create(getGlobalContext(), "l", 
                                            this->func
                                            );
        cleanupBlock = getUnwindBlock(context);
    }
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

bool LLVMBuilder::suppressCleanups() {
    // only ever want to do this if the last instruction is unreachable.
    BasicBlock *block = builder.GetInsertBlock();
    BasicBlock::iterator i = block->end();
    return i != block->begin() &&
           (--i)->getOpcode() == Instruction::Unreachable;
}
        

BranchpointPtr LLVMBuilder::emitBeginTry(model::Context &context) {
    // make sure we have the special exception variables installed in the 
    // context.
    if (!context.ns->lookUp(":exceptionSelector")) {
        createSpecialVar(context.ns.get(), context.construct->int32Type.get(), 
                         ":exceptionSelector"
                         );
        createSpecialVar(context.ns.get(), context.construct->voidptrType.get(), 
                         ":exceptionObject"
                         );
    }

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
        // :exceptionSelector, we'll fill it in with values later.
        builder.SetInsertPoint(bpos->block);
        VarDefPtr sel = context.ns->lookUp(":exceptionSelector");
        BHeapVarDefImplPtr selImpl = BHeapVarDefImplPtr::rcast(sel->impl);
        Value *selVal = builder.CreateLoad(selImpl->rep);
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
    lastValue = builder.CreateCall(getExFunc, parms.begin(), parms.end());
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
        cdata->fixAllSelectors(module);
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
    assert(flags & FuncDef::forward);

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

BTypeDefPtr LLVMBuilder::createClass(Context &context, const string &name,
                                     unsigned int nextVTableSlot
                                     ) {
    BTypeDefPtr type;
    TypeDef::TypeVec bases;
    BTypeDefPtr metaType = createMetaClass(context, name);
    module->addTypeName("struct.meta." + name, metaType->rep);

    const Type *opaque = OpaqueType::get(getGlobalContext());
    type = new BTypeDef(metaType.get(), name,
                        PointerType::getUnqual(opaque),
                        true,
                        nextVTableSlot
                        );

    // tie the meta-class to the class
    metaType->meta = type.get();
    
    // create the unsafeCast() function.
    context.addDef(new UnsafeCastDef(type.get()), metaType.get());
    
    // create function to convert to voidptr
    context.addDef(new VoidPtrOpDef(context.construct->voidptrType.get()), 
                   type.get());

    // make the class default to initializing to null
    type->defaultInitializer = new NullConst(type.get());

    return type;
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
    BBuilderContextData *contextData;
    context.builderData = contextData = new BBuilderContextData();
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

    func = funcDef->rep;
    
    createFuncStartBlocks(name);
    
    if (flags & FuncDef::virtualized) {
        // emit code to convert from the first declaration base class 
        // instance to the method's class instance.
        ArgDefPtr thisVar = funcDef->thisArg;
        BArgVarDefImpl *thisImpl = BArgVarDefImplPtr::arcast(thisVar->impl);
        Value *inst = thisImpl->rep;
        Context *classCtx = context.getClassContext().get();
        Value *thisRep =
            IncompleteSpecialize::emitSpecialize(context,
                                                 classType,
                                                 inst,
                                                 funcDef->pathToFirstDeclaration
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
                  args.size()
                  );

    if (!symName.empty())
        f.setSymbolName(symName);

    // if we've got a receiver, add it to the func builder and store a "this"
    // variable.
    if (receiverType) {
        f.setReceiverType(BTypeDefPtr::acast(receiverType));
        ArgDefPtr thisDef = 
            funcCtx->builder.createArgDef(receiverType, "this");
        funcCtx->addDef(thisDef.get());
    }

    f.setArgs(args);
    f.finish(false);
    primFuncs[f.funcDef->rep] = cfunc;
    return f.funcDef;
}

namespace {
    void createOperClassFunc(Context &context,
                             BTypeDef *objClass,
                             BTypeDef *metaClass
                             ) {

        // build a local context to hold the "this"
        Context localCtx(context.builder, Context::local, &context,
                         new LocalNamespace(objClass, objClass->name),
                         context.compileNS.get()
                         );
        localCtx.addDef(new ArgDef(objClass, "this"));

        FuncBuilder funcBuilder(localCtx,
                                FuncDef::method | FuncDef::virtualized,
                                metaClass,
                                "oper class",
                                0
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

        BasicBlock *block = BasicBlock::Create(getGlobalContext(),
                                               "oper class",
                                               funcBuilder.funcDef->rep
                                               );
        
        // body of the function: load the global variable and return it.
        IRBuilder<> builder(block);
        BGlobalVarDefImpl *impl = 
            BGlobalVarDefImplPtr::arcast(objClass->impl);
        Value *val = builder.CreateLoad(impl->rep);
        builder.CreateRet(val);
    }
}

TypeDefPtr LLVMBuilder::emitBeginClass(Context &context,
                                       const string &name,
                                       const vector<TypeDefPtr> &bases,
                                       TypeDef *forwardDef
                                       ) {
    assert(!context.builderData);
    BBuilderContextData *bdata;
    context.builderData = bdata = new BBuilderContextData();

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
        createOperClassFunc(context, type.get(), 
                            BTypeDefPtr::arcast(type->type)
                            );

    return type.get();
}
        
void LLVMBuilder::emitEndClass(Context &context) {
    // build a vector of the base classes and instance variables
    vector<const Type *> members;
    
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
    
    // verify that all of the members have been assigned
    for (vector<const Type *>::iterator iter = members.begin();
         iter != members.end();
         ++iter
         )
        assert(*iter);
    
    // refine the type to the actual type of the structure.
    
    // extract the opaque type out of the pointer type.
    const PointerType *ptrType =
        cast<PointerType>(type->rep);
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type*>(ptrType->getElementType()));
    
    // create the actual type (store it in a type holder so that if the new 
    // type gets replaced, we'll get the new type and not drop the old one)
    PATypeHolder newType(StructType::get(getGlobalContext(), members));
    module->addTypeName("struct." + type->name, newType);
    
    // refine the type and store the new pointer type (the existing pointer 
    // to opaque type may not end up getting changed)
    curType->refineAbstractTypeTo(newType);
    type->rep = PointerType::getUnqual(newType);

    // construct the vtable if necessary
    if (type->hasVTable) {
        VTableBuilder vtableBuilder(
            this,
            BTypeDefPtr::arcast(context.construct->vtableBaseType),
            module
        );
        type->createAllVTables(
            vtableBuilder, 
            ".vtable." + type->name,
            BTypeDefPtr::arcast(context.construct->vtableBaseType)
        );
        vtableBuilder.emit(type);
    }

    // fix-up all of the placeholder instructions
    for (vector<PlaceholderInstruction *>::iterator iter = 
            type->placeholders.begin();
         iter != type->placeholders.end();
         ++iter
         )
        (*iter)->fix();
    type->placeholders.clear();
    type->complete = true;
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
    
    // get the defintion context
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
            varDefImpl = new BGlobalVarDefImpl(gvar);
            break;
        }

        case Context::local: {
            varDefImpl = createLocalVar(tp, var);
            break;
        }
        
        default:
            assert(false && "invalid context value!");
    }
    
    // allocate the variable and assign it
    lastValue = builder.CreateStore(lastValue, var);
    
    // create the definition object.
    VarDefPtr varDef = new VarDef(type, name);
    varDef->impl = varDefImpl;
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
                          BTypeDefPtr::arcast(context.construct->float32Type),
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

    unsigned index = BInstVarDefImplPtr::rcast(assign->var->impl)->index;
    // if the variable is part of a complete context, just do the store.  
    // Otherwise create a fixup.
    if (typeDef->complete) {
        Value *fieldRef = builder.CreateStructGEP(aggregateRep, index);
        builder.CreateStore(lastValue, fieldRef);
    } else {
        // create a placeholder instruction
        PlaceholderInstruction *placeholder =
            new IncompleteInstVarAssign(aggregateRep->getType(),
                                        aggregateRep,
                                        index,
                                        lastValue,
                                        builder.GetInsertBlock()
                                        );

        // store it
        typeDef->addPlaceholder(placeholder);
    }

    lastValue = temp;
    return new BResultExpr(assign, temp);
}

ModuleDefPtr LLVMBuilder::registerPrimFuncs(model::Context &context) {

    assert(!context.getParent()->getParent() && "parent context must be root");
    assert(!module);

    createLLVMModule(".builtin");
    BModuleDef *bMod = new BModuleDef(".builtin", context.ns.get(), module);

    Construct *gd = context.construct;
    LLVMContext &lctx = getGlobalContext();

    // create the basic types
    
    BTypeDef *classType;
    Type *classTypeRep = OpaqueType::get(lctx);
    Type *classTypePtrRep = PointerType::getUnqual(classTypeRep);
    gd->classType = classType = new BTypeDef(0, "Class", classTypePtrRep);
    classType->type = classType;
    classType->meta = classType;
    context.addDef(classType);

    // some tools for creating meta-classes
    BTypeDefPtr metaType;           // storage for meta-types
    
    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef(context.construct->classType.get(), 
                                           "void",
                                           Type::getVoidTy(lctx)
                                           );
    context.addDef(voidType);

    BTypeDef *voidptrType;
    llvmVoidPtrType = Type::getInt8Ty(lctx)->getPointerTo();
        PointerType::getUnqual(OpaqueType::get(getGlobalContext()));
    gd->voidptrType = voidptrType = new BTypeDef(context.construct->classType.get(), 
                                                 "voidptr",
                                                 llvmVoidPtrType
                                                 );
    voidptrType->defaultInitializer = new NullConst(voidptrType);
    context.addDef(voidptrType);

    // now that we've got a voidptr type, give the class object a cast to it.
    context.addDef(new VoidPtrOpDef(voidptrType), classType);
    
    llvm::Type *llvmBytePtrType = 
        PointerType::getUnqual(Type::getInt8Ty(lctx));
    BTypeDef *byteptrType;
    gd->byteptrType = byteptrType = new BTypeDef(context.construct->classType.get(), 
                                                 "byteptr",
                                                 llvmBytePtrType
                                                 );
    byteptrType->defaultInitializer = createStrConst(context, "");
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        byteptrType
    );
    FuncDefPtr funcDef =
        new GeneralOpDef<UnsafeCastCall>(byteptrType, FuncDef::noFlags,
                                         "oper new",
                                         1
                                         );
    funcDef->args[0] = new ArgDef(voidptrType, "val");
    context.addDef(funcDef.get(), byteptrType);
    context.addDef(byteptrType);
    
    const Type *llvmBoolType = IntegerType::getInt1Ty(lctx);
    BTypeDef *boolType;
    gd->boolType = boolType = new BTypeDef(context.construct->classType.get(), 
                                           "bool",
                                           llvmBoolType
                                           );
    gd->boolType->defaultInitializer = new BIntConst(boolType, (int64_t)0);
    context.addDef(boolType);
    
    BTypeDef *byteType = createIntPrimType(context, Type::getInt8Ty(lctx),
                                           "byte"
                                           );
    gd->byteType = byteType;

    BTypeDef *int32Type = createIntPrimType(context, Type::getInt32Ty(lctx),
                                            "int32"
                                            );
    gd->int32Type = int32Type;

    BTypeDef *int64Type = createIntPrimType(context, Type::getInt64Ty(lctx),
                                            "int64"
                                            );
    gd->int64Type = int64Type;
    
    BTypeDef *uint32Type = createIntPrimType(context, Type::getInt32Ty(lctx),
                                            "uint32"
                                            );
    gd->uint32Type = uint32Type;

    BTypeDef *uint64Type = createIntPrimType(context, Type::getInt64Ty(lctx),
                                            "uint64"
                                            );
    gd->uint64Type = uint64Type;

    BTypeDef *float32Type = createFloatPrimType(context, Type::getFloatTy(lctx),
                                            "float32"
                                            );
    gd->float32Type = float32Type;

    BTypeDef *float64Type = createFloatPrimType(context, Type::getDoubleTy(lctx),
                                            "float64"
                                            );
    gd->float64Type = float64Type;

    // XXX bad assumptions about sizeof
    if (sizeof(int) == 4) {
        context.ns->addAlias("int", int32Type);
        context.ns->addAlias("uint", uint32Type);
        context.ns->addAlias("float", float32Type);
        gd->uintType = uint32Type;
        gd->intType = int32Type;
        gd->floatType = float32Type;
        llvmIntType = int32Type->rep;
    } else {
        assert(sizeof(int) == 8);
        context.ns->addAlias("int", int64Type);
        context.ns->addAlias("uint", uint64Type);
        context.ns->addAlias("float", float64Type);
        gd->uintType = uint64Type;
        gd->intType = int64Type;
        gd->floatType = float64Type;
        llvmIntType = int64Type->rep;
    }

    // conversion from voidptr to integer
    funcDef =
        new GeneralOpDef<PtrToIntOpCall>(uint64Type, FuncDef::noFlags,
                                         "oper new",
                                         1
                                         );
    funcDef->args[0] = new ArgDef(voidptrType, "val");
    context.addDef(funcDef.get(), uint64Type);
    
    // create integer operations
    context.addDef(new AddOpDef(byteType));
    context.addDef(new SubOpDef(byteType));
    context.addDef(new MulOpDef(byteType));
    context.addDef(new SDivOpDef(byteType));
    context.addDef(new SRemOpDef(byteType));
    context.addDef(new ICmpEQOpDef(byteType, boolType));
    context.addDef(new ICmpNEOpDef(byteType, boolType));
    context.addDef(new ICmpSGTOpDef(byteType, boolType));
    context.addDef(new ICmpSLTOpDef(byteType, boolType));
    context.addDef(new ICmpSGEOpDef(byteType, boolType));
    context.addDef(new ICmpSLEOpDef(byteType, boolType));
    context.addDef(new NegOpDef(byteType, "oper -"));
    context.addDef(new BitNotOpDef(byteType, "oper ~"));
    context.addDef(new OrOpDef(byteType));
    context.addDef(new AndOpDef(byteType));
    context.addDef(new XorOpDef(byteType));
    context.addDef(new ShlOpDef(byteType));
    context.addDef(new LShrOpDef(byteType));

    context.addDef(new AddOpDef(uint32Type));
    context.addDef(new SubOpDef(uint32Type));
    context.addDef(new MulOpDef(uint32Type));
    context.addDef(new UDivOpDef(uint32Type));
    context.addDef(new URemOpDef(uint32Type));
    context.addDef(new ICmpEQOpDef(uint32Type, boolType));
    context.addDef(new ICmpNEOpDef(uint32Type, boolType));
    context.addDef(new ICmpUGTOpDef(uint32Type, boolType));
    context.addDef(new ICmpULTOpDef(uint32Type, boolType));
    context.addDef(new ICmpUGEOpDef(uint32Type, boolType));
    context.addDef(new ICmpULEOpDef(uint32Type, boolType));
    context.addDef(new NegOpDef(uint32Type, "oper -"));
    context.addDef(new BitNotOpDef(uint32Type, "oper ~"));
    context.addDef(new OrOpDef(uint32Type));
    context.addDef(new AndOpDef(uint32Type));
    context.addDef(new XorOpDef(uint32Type));
    context.addDef(new ShlOpDef(uint32Type));
    context.addDef(new LShrOpDef(uint32Type));

    context.addDef(new AddOpDef(int32Type));
    context.addDef(new SubOpDef(int32Type));
    context.addDef(new MulOpDef(int32Type));
    context.addDef(new SDivOpDef(int32Type));
    context.addDef(new SRemOpDef(int32Type));
    context.addDef(new ICmpEQOpDef(int32Type, boolType));
    context.addDef(new ICmpNEOpDef(int32Type, boolType));
    context.addDef(new ICmpSGTOpDef(int32Type, boolType));
    context.addDef(new ICmpSLTOpDef(int32Type, boolType));
    context.addDef(new ICmpSGEOpDef(int32Type, boolType));
    context.addDef(new ICmpSLEOpDef(int32Type, boolType));
    context.addDef(new NegOpDef(int32Type, "oper -"));
    context.addDef(new BitNotOpDef(int32Type, "oper ~"));
    context.addDef(new OrOpDef(int32Type));
    context.addDef(new AndOpDef(int32Type));
    context.addDef(new XorOpDef(int32Type));
    context.addDef(new ShlOpDef(int32Type));
    context.addDef(new AShrOpDef(int32Type));

    context.addDef(new AddOpDef(uint64Type));
    context.addDef(new SubOpDef(uint64Type));
    context.addDef(new MulOpDef(uint64Type));
    context.addDef(new UDivOpDef(uint64Type));
    context.addDef(new URemOpDef(uint64Type));
    context.addDef(new ICmpEQOpDef(uint64Type, boolType));
    context.addDef(new ICmpNEOpDef(uint64Type, boolType));
    context.addDef(new ICmpUGTOpDef(uint64Type, boolType));
    context.addDef(new ICmpULTOpDef(uint64Type, boolType));
    context.addDef(new ICmpUGEOpDef(uint64Type, boolType));
    context.addDef(new ICmpULEOpDef(uint64Type, boolType));
    context.addDef(new NegOpDef(uint64Type, "oper -"));
    context.addDef(new BitNotOpDef(uint64Type, "oper ~"));
    context.addDef(new OrOpDef(uint64Type));
    context.addDef(new AndOpDef(uint64Type));
    context.addDef(new XorOpDef(uint64Type));
    context.addDef(new ShlOpDef(uint64Type));
    context.addDef(new LShrOpDef(uint64Type));

    context.addDef(new AddOpDef(int64Type));
    context.addDef(new SubOpDef(int64Type));
    context.addDef(new MulOpDef(int64Type));
    context.addDef(new SDivOpDef(int64Type));
    context.addDef(new SRemOpDef(int64Type));
    context.addDef(new ICmpEQOpDef(int64Type, boolType));
    context.addDef(new ICmpNEOpDef(int64Type, boolType));
    context.addDef(new ICmpSGTOpDef(int64Type, boolType));
    context.addDef(new ICmpSLTOpDef(int64Type, boolType));
    context.addDef(new ICmpSGEOpDef(int64Type, boolType));
    context.addDef(new ICmpSLEOpDef(int64Type, boolType));
    context.addDef(new NegOpDef(int64Type, "oper -"));
    context.addDef(new BitNotOpDef(int64Type, "oper ~"));
    context.addDef(new OrOpDef(int64Type));
    context.addDef(new AndOpDef(int64Type));
    context.addDef(new XorOpDef(int64Type));
    context.addDef(new ShlOpDef(int64Type));
    context.addDef(new AShrOpDef(int64Type));

    // float operations
    context.addDef(new FAddOpDef(float32Type));
    context.addDef(new FSubOpDef(float32Type));
    context.addDef(new FMulOpDef(float32Type));
    context.addDef(new FDivOpDef(float32Type));
    context.addDef(new FRemOpDef(float32Type));
    context.addDef(new FCmpOEQOpDef(float32Type, boolType));
    context.addDef(new FCmpONEOpDef(float32Type, boolType));
    context.addDef(new FCmpOGTOpDef(float32Type, boolType));
    context.addDef(new FCmpOLTOpDef(float32Type, boolType));
    context.addDef(new FCmpOGEOpDef(float32Type, boolType));
    context.addDef(new FCmpOLEOpDef(float32Type, boolType));
    context.addDef(new FNegOpDef(float32Type, "oper -"));

    context.addDef(new FAddOpDef(float64Type));
    context.addDef(new FSubOpDef(float64Type));
    context.addDef(new FMulOpDef(float64Type));
    context.addDef(new FDivOpDef(float64Type));
    context.addDef(new FRemOpDef(float64Type));
    context.addDef(new FCmpOEQOpDef(float64Type, boolType));
    context.addDef(new FCmpONEOpDef(float64Type, boolType));
    context.addDef(new FCmpOGTOpDef(float64Type, boolType));
    context.addDef(new FCmpOLTOpDef(float64Type, boolType));
    context.addDef(new FCmpOGEOpDef(float64Type, boolType));
    context.addDef(new FCmpOLEOpDef(float64Type, boolType));
    context.addDef(new FNegOpDef(float64Type, "oper -"));

    // boolean logic
    context.addDef(new LogicAndOpDef(boolType, boolType));
    context.addDef(new LogicOrOpDef(boolType, boolType));
    
    // implicit conversions (no loss of precision)
    context.addDef(new ZExtOpDef(int32Type, "oper to int32"), byteType);
    context.addDef(new ZExtOpDef(int64Type, "oper to int64"), byteType);
    context.addDef(new ZExtOpDef(uint32Type, "oper to uint32"), byteType);
    context.addDef(new ZExtOpDef(uint64Type, "oper to uint64"), byteType);
    context.addDef(new UIToFPOpDef(float32Type, "oper to float32"), byteType);
    context.addDef(new UIToFPOpDef(float64Type, "oper to float64"), byteType);
    context.addDef(new SExtOpDef(int64Type, "oper to int64"), int32Type);
    context.addDef(new ZExtOpDef(uint64Type, "oper to uint64"), int32Type);
    context.addDef(new SIToFPOpDef(float64Type, "oper to float64"), int32Type);
    context.addDef(new ZExtOpDef(uint64Type, "oper to uint64"), uint32Type);
    context.addDef(new ZExtOpDef(int64Type, "oper to int64"), uint32Type);
    context.addDef(new UIToFPOpDef(float64Type, "oper to float64"), uint32Type);
    context.addDef(new FPExtOpDef(float64Type, "oper to float64"), float32Type);

    // add the increment and decrement operators
    context.addDef(new PreIncrIntOpDef(byteType, "oper ++x"), byteType);
    context.addDef(new PreIncrIntOpDef(int32Type, "oper ++x"), int32Type);
    context.addDef(new PreIncrIntOpDef(uint32Type, "oper ++x"), uint32Type);
    context.addDef(new PreIncrIntOpDef(int64Type, "oper ++x"), int64Type);
    context.addDef(new PreIncrIntOpDef(uint64Type, "oper ++x"), uint64Type);
    context.addDef(new PreDecrIntOpDef(byteType, "oper --x"), byteType);
    context.addDef(new PreDecrIntOpDef(int32Type, "oper --x"), int32Type);
    context.addDef(new PreDecrIntOpDef(uint32Type, "oper --x"), uint32Type);
    context.addDef(new PreDecrIntOpDef(int64Type, "oper --x"), int64Type);
    context.addDef(new PreDecrIntOpDef(uint64Type, "oper --x"), uint64Type);
    context.addDef(new PostIncrIntOpDef(byteType, "oper x++"), byteType);
    context.addDef(new PostIncrIntOpDef(int32Type, "oper x++"), int32Type);
    context.addDef(new PostIncrIntOpDef(uint32Type, "oper x++"), uint32Type);
    context.addDef(new PostIncrIntOpDef(int64Type, "oper x++"), int64Type);
    context.addDef(new PostIncrIntOpDef(uint64Type, "oper x++"), uint64Type);
    context.addDef(new PostDecrIntOpDef(byteType, "oper x--"), byteType);
    context.addDef(new PostDecrIntOpDef(int32Type, "oper x--"), int32Type);
    context.addDef(new PostDecrIntOpDef(uint32Type, "oper x--"), uint32Type);
    context.addDef(new PostDecrIntOpDef(int64Type, "oper x--"), int64Type);
    context.addDef(new PostDecrIntOpDef(uint64Type, "oper x--"), uint64Type);

    // explicit no-op construction
    addNopNew(context, int64Type);
    addNopNew(context, uint64Type);
    addNopNew(context, int32Type);
    addNopNew(context, uint32Type);
    addNopNew(context, byteType);
    addNopNew(context, float32Type);
    addNopNew(context, float64Type);

    // explicit (loss of precision)
    addExplicitTruncate(context, int64Type, uint64Type);
    addExplicitTruncate(context, int64Type, int32Type);
    addExplicitTruncate(context, int64Type, uint32Type);
    addExplicitTruncate(context, int64Type, byteType);
    addExplicitTruncate(context, uint64Type, int64Type);
    addExplicitTruncate(context, uint64Type, int32Type);
    addExplicitTruncate(context, uint64Type, uint32Type);
    addExplicitTruncate(context, uint64Type, byteType);
    addExplicitTruncate(context, int32Type, byteType);
    addExplicitTruncate(context, int32Type, uint32Type);
    addExplicitTruncate(context, int32Type, uint32Type);
    addExplicitTruncate(context, uint32Type, byteType);
    addExplicitTruncate(context, uint32Type, int32Type);

    addExplicitFPTruncate<FPTruncOpCall>(context, float64Type, float32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(context, float32Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float32Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float32Type, uint64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(context, float64Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(context, float64Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(context, float64Type, uint64Type);

    // create the array generic
    TypeDefPtr arrayType = new ArrayTypeDef(context.construct->classType.get(),
                                            "array", 
                                            0
                                            );
    context.addDef(arrayType.get());

    // now that we have byteptr and array and all of the integer types, we can
    // initialize the body of Class.
    context.addDef(new IsOpDef(classType, boolType));

    finishClassType(context, classType);
    
    // back-fill meta class and impls for the existing primitives
    fixMeta(context, voidType);
    fixMeta(context, voidptrType);
    fixMeta(context, boolType);
    fixMeta(context, byteType);
    fixMeta(context, int32Type);
    fixMeta(context, int64Type);
    fixMeta(context, uint32Type);
    fixMeta(context, uint64Type);
    fixMeta(context, arrayType.get());

    // create OverloadDef's type
    metaType = createMetaClass(context, "Overload");
    BTypeDefPtr overloadDef = new BTypeDef(metaType.get(), "Overload", 0);
    metaType->meta = overloadDef.get();
    createClassImpl(context, overloadDef.get());
        
    // Give it a context and an "oper to voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        overloadDef.get()
    );
    
    // create an empty structure type and its pointer for VTableBase 
    // Actual type is {}** (another layer of pointer indirection) because 
    // classes need to be pointer types.
    vector<const Type *> members;
    Type *vtableType = StructType::get(getGlobalContext(), members);
    Type *vtablePtrType = PointerType::getUnqual(vtableType);
    metaType = createMetaClass(context, "VTableBase");
    BTypeDef *vtableBaseType;
    gd->vtableBaseType = vtableBaseType =
        new BTypeDef(metaType.get(), "VTableBase",
                     PointerType::getUnqual(vtablePtrType),
                     true
                     );
    vtableBaseType->hasVTable = true;
    createClassImpl(context, vtableBaseType);
    metaType->meta = vtableBaseType;
    context.addDef(vtableBaseType);
    createOperClassFunc(context, vtableBaseType, metaType.get());

    // build VTableBase's vtable
    VTableBuilder vtableBuilder(this, vtableBaseType, module);
    vtableBaseType->createAllVTables(vtableBuilder, ".vtable.VTableBase", 
                                     vtableBaseType
                                     );
    vtableBuilder.emit(vtableBaseType);

    // pointer equality check (to allow checking for None)
    context.addDef(new IsOpDef(voidptrType, boolType));
    context.addDef(new IsOpDef(byteptrType, boolType));
    
    // boolean not
    context.addDef(new BitNotOpDef(boolType, "oper !"));
    
    // byteptr array indexing
    addArrayMethods(context, byteptrType, byteType);    

    // bind the module to the execution engine
    engineBindModule(bMod);
    engineFinishModule(bMod);

    return bMod;

}

void LLVMBuilder::createModuleCommon(Context &context) {

    // name some structs in this module
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    module->addTypeName(".struct.Class", classType->rep);
    BTypeDef *vtableBaseType = BTypeDefPtr::arcast(
                                  context.construct->vtableBaseType);
    module->addTypeName(".struct.vtableBase", vtableBaseType->rep);

    // all of the "extern" primitive functions have to be created in each of
    // the modules - we can not directly reference across modules.

    //BTypeDef *int32Type = BTypeDefPtr::arcast(context.construct->int32Type);
    //BTypeDef *int64Type = BTypeDefPtr::arcast(context.construct->int64Type);
    //BTypeDef *uint64Type = BTypeDefPtr::arcast(context.construct->uint64Type);
    BTypeDef *intType = BTypeDefPtr::arcast(context.construct->intType);
    BTypeDef *voidType = BTypeDefPtr::arcast(context.construct->int32Type);
    //BTypeDef *float32Type = BTypeDefPtr::arcast(context.construct->float32Type);
    //BTypeDef *byteptrType =
    //    BTypeDefPtr::arcast(context.construct->byteptrType);
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);

    // create "void *calloc(uint size)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidptrType, "calloc", 2);
        f.addArg("size", intType);
        f.addArg("size", intType);
        f.setSymbolName("calloc");
        f.finish();
        callocFunc = f.funcDef->getRep(*this);
    }

    // create "array[byteptr] __getArgv()"
    {
        TypeDefPtr array = context.ns->lookUp("array");
        assert(array.get() && "array not defined in context");
        TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
        types->push_back(context.construct->byteptrType.get());
        TypeDefPtr arrayOfByteptr =
            array->getSpecialization(context, types.get());
        FuncBuilder f(context, FuncDef::noFlags,
                      BTypeDefPtr::arcast(arrayOfByteptr),
                      "__getArgv",
                      0
                      );
        f.setSymbolName("__getArgv");
        f.finish();
    }

    // create "int __getArgc()"
    {
        FuncBuilder f(context, FuncDef::noFlags, intType, "__getArgc", 0);
        f.setSymbolName("__getArgc");
        f.finish();
    }

    // create "__CrackThrow(VTableBase)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "__CrackThrow", 1);
        f.addArg("exception", vtableBaseType);
        f.setSymbolName("__CrackThrow");
        f.finish();
    }

    // create "__CrackGetException(voidptr)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidptrType,
                      "__CrackGetException",
                      1
                      );
        f.addArg("exceptionObject", voidptrType);
        f.setSymbolName("__CrackGetException");
        f.finish();
    }

    // create "__CrackBadCast(Class a, Class b)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType,
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
        FuncBuilder f(context, FuncDef::noFlags, voidType,
                      "__CrackCleanupException",
                      1
                      );
        f.addArg("exceptionObject", voidptrType);
        f.setSymbolName("__CrackCleanupException");
        f.finish();
    }
    
    // create "__CrackExceptionFrame()"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType,
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
                                    const vector<string> &symbols,
                                    Context &context,
                                    Namespace *ns
                                    ) {
    void *handle = loadSharedLibrary(name);
    for (vector<string>::const_iterator iter = symbols.begin();
         iter != symbols.end();
         ++iter
         ) {
        void *sym = dlsym(handle, iter->c_str());
        if (!sym)
            throw spug::Exception(dlerror());

        // store a stub for the symbol        
        ns->addDef(new StubDef(context.construct->voidType.get(), 
                               *iter,
                               sym
                               )
                   );
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

