// Copyright 2009 Google Inc.
                
#include "LLVMBuilder.h"

#include <dlfcn.h>

// LLVM includes
#include <stddef.h>
#include <stdlib.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT

#include <spug/Exception.h>
#include <spug/StringFmt.h>

#include <model/AllocExpr.h>
#include <model/AssignExpr.h>
#include <model/ArgDef.h>
#include <model/BuilderContextData.h>
#include <model/CleanupFrame.h>
#include <model/CompositeNamespace.h>
#include <model/VarDefImpl.h>
#include <model/InstVarDef.h>
#include <model/LocalNamespace.h>
#include <model/NullConst.h>
#include <model/OverloadDef.h>
#include <model/StubDef.h>
#include <model/VarDef.h>
#include <model/VarRef.h>

#include "BBuilderContextData.h"
#include "BBranchPoint.h"
#include "BCleanupFrame.h"
#include "BFieldRef.h"
#include "BFuncDef.h"
#include "BModuleDef.h"
#include "BResultExpr.h"
#include "BTypeDef.h"
#include "Consts.h"
#include "FuncBuilder.h"
#include "Incompletes.h"
#include "Ops.h"
#include "PlaceholderInstruction.h"
#include "VarDefs.h"
#include "VTableBuilder.h"

#include "parser/Parser.h"
#include "parser/ParseError.h"


using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;

typedef model::FuncCall::ExprVec ExprVec;

// XXX find a way to remove this? see Incompletes.cc
const Type *llvmIntType = 0;

namespace {
        
    void addArrayMethods(Context &context, TypeDef *arrayType, 
                         BTypeDef *elemType
                         ) {
        Context::GlobalData *gd = context.globalData;
        FuncDefPtr arrayGetItem = 
            new GeneralOpDef<ArrayGetItemCall>(elemType, FuncDef::method, 
                                               "oper []",
                                               1
                                               );
        arrayGetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
        arrayType->addDef(arrayGetItem.get());
    
        FuncDefPtr arraySetItem = 
            new GeneralOpDef<ArraySetItemCall>(elemType, FuncDef::method, 
                                               "oper []=",
                                               2
                                               );
        arraySetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
        arraySetItem->args[1] = new ArgDef(elemType, "value");
        arrayType->addDef(arraySetItem.get());
        
        FuncDefPtr arrayOffset =
            new GeneralOpDef<ArrayOffsetCall>(arrayType, FuncDef::noFlags, 
                                              "oper +",
                                              2
                                              );
        arrayOffset->args[0] = new ArgDef(arrayType, "base");
        arrayOffset->args[1] = new ArgDef(gd->uintType.get(), "offset");
        context.getDefContext()->ns->addDef(arrayOffset.get());
        
        FuncDefPtr arrayAlloc =
            new GeneralOpDef<ArrayAllocCall>(arrayType, FuncDef::noFlags,
                                             "oper new",
                                             1
                                             );
        arrayAlloc->args[0] = new ArgDef(gd->uintType.get(), "size");
        arrayType->addDef(arrayAlloc.get());
    }

    class ArrayTypeDef : public BTypeDef {
        public:
            ArrayTypeDef(TypeDef *metaType, const string &name,
                         const Type *rep
                         ) :
                BTypeDef(metaType, name, rep) {
                generic = new SpecializationCache();
            }
            
            // specializations of array types actually create a new type 
            // object.
            virtual TypeDef *getSpecialization(Context &context, 
                                               TypeVecObj *types
                                               ) {
                // see if it already exists
                TypeDef *spec = findSpecialization(types);
                if (spec)
                    return spec;
                
                // create it.
                
                assert(types->size() == 1);
                
                BTypeDef *parmType = BTypeDefPtr::rcast((*types)[0]);
                
                Type *llvmType = PointerType::getUnqual(parmType->rep);
                TypeDefPtr tempSpec = 
                    new BTypeDef(type.get(),
                                 SPUG_FSTR(name << "[" << parmType->name << 
                                            "]"
                                           ),
                                 llvmType
                                 );
                                  
                tempSpec->addDef(
                    new VoidPtrOpDef(context.globalData->voidPtrType.get())
                );
                
                // add all of the methods
                addArrayMethods(context, tempSpec.get(), parmType);
                (*generic)[types] = tempSpec;
                return tempSpec.get();
            }
    };

    void closeAllCleanupsStatic(Context &context) {
        BCleanupFrame* frame = BCleanupFramePtr::rcast(context.cleanupFrame);
        while (frame) {
            frame->close();
            frame = BCleanupFramePtr::rcast(frame->parent);
        }
    }

    // emit all cleanups from this context to that of the branchpoint.
    void emitCleanupsTo(Context &context, BBranchpoint *bpos) {
        
        // unless we've reached our stop, emit for all parent contexts
        if (!(bpos->context == &context)) {
    
            // close all cleanups in thie context
            closeAllCleanupsStatic(context);
            emitCleanupsTo(*context.parent, bpos);
        }
    }

    // Create a new meta-class.
    // context: enclosing context.
    // name: the original class name.
    // bases: the base classes.
    // classImpl: returned impl object for the original class.
    BTypeDefPtr createMetaClass(Context &context, 
                                const string &name,
                                vector<TypeDefPtr> bases,
                                BGlobalVarDefImplPtr &classImpl
                                ) {
        LLVMBuilder &llvmBuilder = 
            dynamic_cast<LLVMBuilder &>(context.builder);
        LLVMContext &lctx = getGlobalContext();
        
        BTypeDefPtr metaType = 
            new BTypeDef(context.globalData->classType.get(),
                         SPUG_FSTR("Class[" << name << "]"),
                         0,
                         true,
                         0
                         );
        BTypeDef *classType =
            BTypeDefPtr::arcast(context.globalData->classType);
        metaType->addBaseClass(classType);
        const PointerType *classPtrType = cast<PointerType>(classType->rep);
        const StructType *classStructType =
            cast<StructType>(classPtrType->getElementType());
        
        // Create a struct representation of the meta class.  This just has the 
        // Class class as its only field.
        vector<const Type *> fields(1);
        fields[0] = classStructType;
        const StructType *metaClassStructType = StructType::get(lctx, fields);
        const Type *metaClassPtrType =
            PointerType::getUnqual(metaClassStructType);
        metaType->rep = metaClassPtrType;
        metaType->complete = true;
        
        // create a global variable holding the class object.
        vector<Constant *> classStructVals(3);
    
        Constant *zero = ConstantInt::get(Type::getInt32Ty(lctx), 0);
        Constant *index00[] = { zero, zero }; 
        
        // name
        Constant *nameInit = ConstantArray::get(lctx, name, true);
        GlobalVariable *nameGVar =
            new GlobalVariable(*llvmBuilder.module,
                               nameInit->getType(),
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               nameInit,
                               name + ":name"
                               );
        classStructVals[0] =
            ConstantExpr::getGetElementPtr(nameGVar, index00, 2);
        
        // numBases
        const Type *uintType =
            BTypeDefPtr::arcast(context.globalData->uintType)->rep;
        classStructVals[1] = ConstantInt::get(uintType, bases.size());
        
        // bases
        vector<Constant *> basesVal(bases.size());
        for (int i = 0; i < bases.size(); ++i) {
            // get the pointer to the inner "Class" object of "Class[BaseName]"
            BGlobalVarDefImplPtr impl =
                BGlobalVarDefImplPtr::arcast(
                    BTypeDefPtr::arcast(bases[i])->impl
                );
            
            // extract the initializer from the rep (which is the global 
            // variable for the _pointer_ to the class) and then GEP our way 
            // into the base class (Class) instance.
            Constant *baseClassPtr = impl->rep->getInitializer();
            Constant *baseAsClass =
                ConstantExpr::getGetElementPtr(baseClassPtr, index00, 2);
            basesVal[i] = baseAsClass;
        }
        const ArrayType *baseArrayType =
            ArrayType::get(classType->rep, bases.size());
        Constant *baseArrayInit = ConstantArray::get(baseArrayType, basesVal);
        GlobalVariable *basesGVar =
            new GlobalVariable(*llvmBuilder.module,
                               baseArrayType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               baseArrayInit,
                               name + ":bases"
                               );
        classStructVals[2] = ConstantExpr::getGetElementPtr(basesGVar, index00,
                                                            2
                                                            );
        
        // build the instance of Class
        Constant *classStruct =
            ConstantStruct::get(classStructType, classStructVals);
        
        // the new meta class's structure is another structure with only 
        // Class as a member.
        vector<Constant *> metaClassStructVals(1);
        metaClassStructVals[0] = classStruct;
        Constant *classObjVal =
            ConstantStruct::get(metaClassStructType, metaClassStructVals);
    
        // Create the class global variable
        GlobalVariable *classInst = 
            new GlobalVariable(*llvmBuilder.module,
                               metaClassStructType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               classObjVal,
                               name + ":body"
                               );
        
        // create the pointer to the class instance
        GlobalVariable *classInstPtr =
            new GlobalVariable(*llvmBuilder.module,
                               metaClassPtrType,
                               true, // is constant
                               GlobalVariable::ExternalLinkage,
                               classInst,
                               name
                               );
    
        classImpl = new BGlobalVarDefImpl(classInstPtr);
        return metaType;
    }

    // Prepares a function "func" to act as an override for "override"
    unsigned wrapOverride(TypeDef *classType, BFuncDef *overriden, 
                          FuncBuilder &funcBuilder
                          ) {
        // find the path to the overriden's class
        BTypeDef *overridenClass = BTypeDefPtr::acast(overriden->owner);
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

} // anon namespace

void LLVMBuilder::emitFunctionCleanups(Context &context) {
    
    // close all cleanups in this context.
    closeAllCleanupsStatic(context);
    
    // recurse up through the parents.
    if (!context.toplevel && context.parent->scope == Context::local)
        emitFunctionCleanups(*context.parent);
}

ExecutionEngine *LLVMBuilder::bindModule(Module *mod) {
    if (execEng) {
        execEng->addModule(mod);
    } else {
        if (rootBuilder) 
            execEng = rootBuilder->bindModule(mod);
        else {
            llvm::JITEmitDebugInfo = true;
            // we have to specify all of the arguments for this so we can turn 
            // off "allocate globals with code."  In addition to being 
            // deprecated in the docs for this function, this option causes 
            // seg-faults when we allocate globals under certain conditions.
            execEng = ExecutionEngine::create(mod,
                                              false, // force interpreter
                                              0, // error string
                                              CodeGenOpt::Default, // opt lvl
                                              false // alloc globals with code
                                              );
        }
    }
    
    return execEng;
}

int LLVMBuilder::argc = 1;
namespace {
    char *tempArgv[] = {const_cast<char *>("undefined")};
}
char **LLVMBuilder::argv = tempArgv;

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
                                   block);
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
                                          bfuncDef->name,
                                          module
                                          );
        execEng->addGlobalMapping(func,
                                  execEng->getPointerToFunction(bfuncDef->rep)
                                  );
        
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


        // do the global mapping
        execEng->addGlobalMapping(global, 
                                  execEng->getPointerToGlobal(bvar->rep)
                                  );
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
    BTypeDefPtr crkFuncType = new BTypeDef(context.globalData->classType.get(), "", 
                                           llvmFuncType
                                           );
    funcTypes[llvmFuncType] = crkFuncType;
    
    // Give it an "oper to voidptr" method.
    crkFuncType->addDef(
        new VoidPtrOpDef(context.globalData->voidPtrType.get())
    );
    
    return crkFuncType.get();
}

LLVMBuilder::LLVMBuilder() :
    module(0),
    builder(getGlobalContext()),
    func(0),
    execEng(0),
    block(0),
    lastValue(0) {

    InitializeNativeTarget();
}

BuilderPtr LLVMBuilder::createChildBuilder() {
    LLVMBuilderPtr result = new LLVMBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    return result;
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
    
    if (funcCall->virtualized)
        lastValue = IncompleteVirtualFunc::emitCall(context, funcDef, 
                                                    receiver,
                                                    valueArgs
                                                    );
    else
        lastValue =
            builder.CreateCall(funcDef->getRep(*this), valueArgs.begin(), 
                               valueArgs.end()
                               );
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
                                                  "",
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
    PointerType *tp =
        cast<PointerType>(const_cast<Type *>(btype->rep));
    
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
    Function *callocFunc = module->getFunction("calloc");
    assert(callocFunc && "calloc function has not been defined");
    BTypeDef *voidPtrType =
        BTypeDefPtr::arcast(context.globalData->voidPtrType);
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
    result->block2 = block; // condition block
    Value *condVal = lastValue; // condition value
    context.closeCleanupFrame();
    lastValue = condVal;
    builder.CreateCondBr(lastValue, trueBlock, result->block);
    
    // repoint to the new ("if true") block
    builder.SetInsertPoint(block = trueBlock);
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
    builder.SetInsertPoint(block = falseBlock);
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
        builder.SetInsertPoint(block = bpos->block);
}

BranchpointPtr LLVMBuilder::emitBeginWhile(Context &context, 
                                           Expr *cond) {
    LLVMContext &lctx = getGlobalContext();
    BBranchpointPtr bpos = new BBranchpoint(BasicBlock::Create(lctx,
                                                               "while_end", 
                                                               func
                                                               )
                                            );
    bpos->context = &context;

    BasicBlock *whileCond = bpos->block2 =
        BasicBlock::Create(lctx, "while_cond", func);
    BasicBlock *whileBody = BasicBlock::Create(lctx, "while_body", func);
    builder.CreateBr(whileCond);
    builder.SetInsertPoint(block = whileCond);

    // XXX see notes above on a conditional type.
    context.createCleanupFrame();
    cond->emitCond(context);
    Value *condVal = lastValue;
    context.closeCleanupFrame();
    lastValue = condVal;
    builder.CreateCondBr(lastValue, whileBody, bpos->block);

    // begin generating code in the while body    
    builder.SetInsertPoint(block = whileBody);

    return bpos;
}

void LLVMBuilder::emitEndWhile(Context &context, Branchpoint *pos, 
                               bool isTerminal
                               ) {
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);

    // emit the branch back to conditional expression in the block
    if (!isTerminal)
        builder.CreateBr(bpos->block2);

    // new code goes to the following block
    builder.SetInsertPoint(block = bpos->block);
}

void LLVMBuilder::emitBreak(Context &context, Branchpoint *branch) {
    BBranchpoint *bpos = BBranchpointPtr::acast(branch);
    emitCleanupsTo(context, bpos);
    builder.CreateBr(bpos->block);
}

void LLVMBuilder::emitContinue(Context &context, Branchpoint *branch) {
    BBranchpoint *bpos = BBranchpointPtr::acast(branch);
    emitCleanupsTo(context, bpos);
    builder.CreateBr(bpos->block2);
}

FuncDefPtr LLVMBuilder::emitBeginFunc(Context &context,
                                      FuncDef::Flags flags,
                                      const string &name,
                                      TypeDef *returnType,
                                      const vector<ArgDefPtr> &args,
                                      FuncDef *override
                                      ) {
    // store the current function and block in the context
    BBuilderContextData *contextData;
    context.builderData = contextData = new BBuilderContextData();
    contextData->func = func;
    contextData->block = block;
    
    // create the function
    FuncBuilder f(context, flags, BTypeDefPtr::cast(returnType), name, 
                  args.size()
                  );
    f.setArgs(args);
    BFuncDef *funcDef = f.funcDef.get();
    
    // see if this is a method, if so store the class type as the receiver type
    unsigned vtableSlot = 0;
    BTypeDef *classType = 0;
    if (flags & FuncDef::method) {
        ContextPtr classCtx = context.getClassContext();
        assert(classCtx && "method is not nested in a class context.");
        classType = BTypeDefPtr::arcast(classCtx->ns);

        // create the vtable slot for a virtual function
        if (flags & FuncDef::virtualized)
            // use the original's slot if this is an override.
            if (override) {
                vtableSlot = wrapOverride(classType, 
                                          BFuncDefPtr::acast(override), 
                                          f
                                          );                
            } else {
                vtableSlot = classType->nextVTableSlot++;
                f.setReceiverType(classType);
            }
        else
            f.setReceiverType(classType);
    }

    f.finish(false);

    f.funcDef->vtableSlot = vtableSlot;
    func = f.funcDef->rep;
    block = BasicBlock::Create(getGlobalContext(), name, func);
    builder.SetInsertPoint(block);
    
    if (flags & FuncDef::virtualized) {
        // emit code to convert from the first declaration base class 
        // instance to the method's class instance.
        Value *inst = 
            dynamic_cast<BArgVarDefImpl *>(f.receiver->impl.get())->rep;
        Context *classCtx = context.getClassContext().get();
        Value *thisRep =
            IncompleteSpecialize::emitSpecialize(context,
                                                 classType,
                                                 inst,
                                                 funcDef->pathToFirstDeclaration
                                                 );
        // lookup the "this" variable, and replace its rep
        VarDefPtr thisVar = context.ns->lookUp("this");
        BArgVarDefImpl *thisImpl = BArgVarDefImplPtr::arcast(thisVar->impl);
        thisImpl->rep = thisRep;
    }
    
    return f.funcDef;
}    

void LLVMBuilder::emitEndFunc(model::Context &context,
                              FuncDef *funcDef) {
    // in certain conditions, (multiple terminating branches) we can end up 
    // with an empty block.  If so, remove.
    if (block->begin() == block->end())
        block->eraseFromParent();

    // restore the block and function
    BBuilderContextData *contextData =
        BBuilderContextDataPtr::rcast(context.builderData);
    func = contextData->func;
    builder.SetInsertPoint(block = contextData->block);
}

FuncDefPtr LLVMBuilder::createExternFunc(Context &context,
                                         FuncDef::Flags flags,
                                         const string &name,
                                         TypeDef *returnType,
                                         const vector<ArgDefPtr> &args,
                                         void *cfunc
                                         ) {
    FuncBuilder f(context, FuncDef::noFlags, BTypeDefPtr::cast(returnType),
                  name,
                  args.size()
                  );
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
                         new LocalNamespace(objClass)
                         );
        localCtx.ns->addDef(new ArgDef(objClass, "this"));

        FuncBuilder funcBuilder(localCtx,
                                FuncDef::method | FuncDef::virtualized,
                                metaClass,
                                "oper class",
                                0
                                );

        // if this is an override, do the wrapping.
        FuncDefPtr override = objClass->lookUpNoArgs("oper class");
        if (override) {
            wrapOverride(objClass, BFuncDefPtr::arcast(override), funcBuilder);
        } else {
            // everything must have an override except for VTableBase::oper 
            // class.
            assert(objClass == context.globalData->vtableBaseType);
            funcBuilder.funcDef->vtableSlot = objClass->nextVTableSlot++;
            funcBuilder.setReceiverType(objClass);
        }

        funcBuilder.finish(false);
        objClass->addDef(funcBuilder.funcDef.get());

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
                                       const vector<TypeDefPtr> &bases) {
    assert(!context.builderData);
    BBuilderContextData *bdata;
    context.builderData = bdata = new BBuilderContextData();

    // create the meta-class
    BGlobalVarDefImplPtr classImpl;
    BTypeDefPtr metaType = createMetaClass(context, name, bases, classImpl);

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
    
    // create the class definition (for classes with no bases, start with 
    // vtable slot 1: slot 0 is the "oper class" function)
    const Type *opaque = OpaqueType::get(getGlobalContext());
    BTypeDefPtr type = new BTypeDef(metaType.get(), name, 
                                    PointerType::getUnqual(opaque),
                                    true,
                                    baseWithVTable ? 
                                        baseWithVTable->nextVTableSlot : 0
                                    );
    type->defaultInitializer = new NullConst(type.get());
    
    // add all of the base classes to the type
    for (vector<TypeDefPtr>::const_iterator iter = bases.begin();
         iter != bases.end();
         ++iter
         ) {
        BTypeDef *base = BTypeDefPtr::rcast(*iter);
        type->addBaseClass(base);
    }

    // make the type the namespace of the context
    context.ns = type;
    
    // tie the meta-class to the class
    metaType->meta = type.get();
    
    // Make the pointer global variable our impl
    type->impl = classImpl;

    // create the unsafeCast() function.
    metaType->addDef(new UnsafeCastDef(type.get()));
    
    // create function to convert to voidptr
    context.ns->addDef(new VoidPtrOpDef(context.globalData->voidPtrType.get()));

    // create the "oper class" function - currently returns voidptr, but 
    // that's good enough for now.
    if (baseWithVTable)
        createOperClassFunc(context, type.get(), metaType.get());

#if 0
    // create the safe cast function.
    if (context.globalData->objectType)
        metaType->context->addDef(new CastDef(type.get(), 
                                              context.globalData->objectType
                                              )
                                  );
#endif

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
    PointerType *ptrType =
        cast<PointerType>(const_cast<Type *>(type->rep));
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type*>(ptrType->getElementType()));
    
    // create the actual type
    Type *newType = StructType::get(getGlobalContext(), members);
    
    // refine the type and store the new pointer type (the existing pointer 
    // to opaque type may not end up getting changed)
    curType->refineAbstractTypeTo(newType);
    type->rep = PointerType::getUnqual(newType);

    // construct the vtable if necessary
    if (type->hasVTable) {
        VTableBuilder vtableBuilder(
            BTypeDefPtr::arcast(context.globalData->vtableBaseType)
        );
        type->createAllVTables(
            vtableBuilder, 
            ".vtable." + type->name,
            BTypeDefPtr::arcast(context.globalData->vtableBaseType)
        );
        vtableBuilder.emit(module, type);
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
                                   name
                                   );
            varDefImpl = new BGlobalVarDefImpl(gvar);
            break;
        }

        case Context::local:
            var = builder.CreateAlloca(tp->rep, 0);
            varDefImpl = new BHeapVarDefImpl(var);
            break;
        
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
 
ModuleDefPtr LLVMBuilder::createModule(Context &context, const string &name) {
    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    module = new llvm::Module(name, lctx);
    llvm::Constant *c =
        module->getOrInsertFunction("__main__", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create(lctx, "__main__", func);
    builder.SetInsertPoint(block);

    // all of the "extern" primitive functions have to be created in each of 
    // the modules - we can not directly reference across modules.
    
    BTypeDef *int32Type = BTypeDefPtr::arcast(context.globalData->int32Type);
    BTypeDef *intType = BTypeDefPtr::arcast(context.globalData->intType);
    BTypeDef *voidType = BTypeDefPtr::arcast(context.globalData->int32Type);
    BTypeDef *float32Type = BTypeDefPtr::arcast(context.globalData->float32Type);
    BTypeDef *byteptrType = 
        BTypeDefPtr::arcast(context.globalData->byteptrType);
    BTypeDef *voidptrType = 
        BTypeDefPtr::arcast(context.globalData->voidPtrType);

    // create "int puts(String)"
    {
        FuncBuilder f(context, FuncDef::noFlags, int32Type, "puts",
                      1
                      );
        f.addArg("text", byteptrType);
        f.finish();
    }
    
    // create "int write(int, String, int)"
    {
        FuncBuilder f(context, FuncDef::noFlags, int32Type, "write",
                      3
                      );
        f.addArg("fd", int32Type);
        f.addArg("buf", byteptrType);
        f.addArg("n", int32Type);
        f.finish();
    }
    
    // create "void printint(int32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint", 1);
        f.addArg("val", int32Type);
        f.finish();
    }

    // create "void printfloat(float32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printfloat", 1);
        f.addArg("val", float32Type);
        f.finish();
    }

    // create "void *calloc(uint size)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidptrType, "calloc", 2);
        f.addArg("size", intType);
        f.addArg("size", intType);
        f.finish();
    }
    
    // create "void __die(byteptr message)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "__die", 1);
        f.addArg("message", byteptrType);
        f.finish();
    }
    
    // create "array[byteptr] __getArgv()"
    {
        TypeDefPtr array = context.ns->lookUp("array");
        TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
        types->push_back(context.globalData->byteptrType.get());
        TypeDefPtr arrayOfByteptr =
            array->getSpecialization(context, types.get());
        FuncBuilder f(context, FuncDef::noFlags, 
                      BTypeDefPtr::arcast(arrayOfByteptr), 
                      "__getArgv", 
                      0
                      );
        f.finish();
    }
    
    // create "int __getArgc()"
    {
        FuncBuilder f(context, FuncDef::noFlags, intType, "__getArgc", 0);
        f.finish();
    }
    
    // bind the module to the execution engine
    bindModule(module);
    
    return new BModuleDef(name, &context);
}

void LLVMBuilder::closeModule(Context &context, ModuleDef *moduleDef) {
    assert(module);
    builder.CreateRetVoid();
    
    // emit the cleanup function
    Function *mainFunc = func;
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction("__del__", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create(lctx, "__del__", func);
    builder.SetInsertPoint(block);
    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();
    
    // restore the main function
    func = mainFunc;
    
    verifyModule(*module, llvm::PrintMessageAction);
    
    // bind the module to the execution engine
    bindModule(module);
    
    // store primitive functions
    for (map<Function *, void *>::iterator iter = primFuncs.begin();
         iter != primFuncs.end();
         ++iter
         )
        execEng->addGlobalMapping(iter->first, iter->second);

    // XXX right now, only checking for > 0, later perhaps we can
    // run specific optimizations at different levels
    if (optimizeLevel) {
        // optimize
        llvm::PassManager passMan;
    
        // Set up the optimizer pipeline.  Start with registering info about how
        // the target lays out data structures.
        passMan.add(new llvm::TargetData(*execEng->getTargetData()));
        // Promote allocas to registers.
        passMan.add(createPromoteMemoryToRegisterPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        passMan.add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        passMan.add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        passMan.add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        passMan.add(llvm::createCFGSimplificationPass());

        passMan.run(*module);
    }
    
    BModuleDefPtr::cast(moduleDef)->cleanup = 
        reinterpret_cast<void (*)()>(
            execEng->getPointerToFunction(module->getFunction("__del__"))
        );
}    

CleanupFramePtr LLVMBuilder::createCleanupFrame(Context &context) {
    return new BCleanupFrame(&context);
}

void LLVMBuilder::closeAllCleanups(Context &context) {
    closeAllCleanupsStatic(context);
}

model::StrConstPtr LLVMBuilder::createStrConst(model::Context &context,
                                               const std::string &val) {
    return new BStrConst(context.globalData->byteptrType.get(), val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, long val,
                                        TypeDef *typeDef
                                        ) {
    // XXX probably need to consider the simplest type that the constant can 
    // fit into (compatibility rules will allow us to coerce it into another 
    // type)
    return new BIntConst(typeDef ? BTypeDefPtr::acast(typeDef) :
                          BTypeDefPtr::arcast(context.globalData->int32Type),
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
                          BTypeDefPtr::arcast(context.globalData->float32Type),
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

    // narrow to the field type.
    BTypeDef *typeDef = BTypeDefPtr::acast(assign->var->owner);
    narrow(aggregate->type.get(), typeDef);
    Value *aggregateRep = lastValue;
    
    // emit the value last, lastValue after this needs to be the expression so 
    // we can chain assignments.
    ResultExprPtr resultExpr = assign->value->emit(context);

    // record the result as being bound to a variable.
    Value *temp = lastValue;
    resultExpr->handleAssignment(context);
    lastValue = temp;

    unsigned index = BInstVarDefImplPtr::rcast(assign->var->impl)->index;
    // if the variable is part of a complete context, just do the store.  
    // Otherwise create a fixup.
    if (typeDef->complete) {
        Value *fieldRef = builder.CreateStructGEP(aggregateRep, index);
        narrow(assign->value->type.get(), assign->var->type.get());
        builder.CreateStore(lastValue, fieldRef);
    } else {
        // create a placeholder instruction
        PlaceholderInstruction *placeholder =
            new IncompleteInstVarAssign(aggregateRep->getType(),
                                        aggregateRep,
                                        index,
                                        lastValue,
                                        block
                                        );

        // store it
        typeDef->addPlaceholder(placeholder);
    }

    return new BResultExpr(assign, lastValue);
}

extern "C" void printfloat(float val) {
    std::cout << val << flush;
}

extern "C" void printint(int val) {
    std::cout << val << flush;
}

extern "C" void __die(const char *message) {
    std::cout << message << endl;
    abort();
}

extern "C" char **__getArgv() {
    return LLVMBuilder::argv;
}

extern "C" int __getArgc() {
    return LLVMBuilder::argc;
}

namespace {
    BTypeDef *createIntPrimType(Context &context, const Type *llvmType,
                                const char *name
                                ) {
        BTypeDefPtr btype = new BTypeDef(context.globalData->classType.get(), 
                                         name, 
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createIntConst(context, 0, btype.get());
        btype->addDef(new BoolOpDef(context.globalData->boolType.get(), 
                                    "toBool"
                                    )
                      );
        
        // if you remove this, for the love of god, change the return type so 
        // we don't leak the pointer.
        context.ns->addDef(btype.get());
        return btype.get();
    }

    BTypeDef *createFloatPrimType(Context &context, const Type *llvmType,
                             const char *name
                             ) {
        BTypeDefPtr btype = new BTypeDef(context.globalData->classType.get(),
                                         name,
                                         llvmType
                                         );
        btype->defaultInitializer =
            context.builder.createFloatConst(context, 0.0, btype.get());
        btype->addDef(new BoolOpDef(context.globalData->boolType.get(),
                                    "toBool"
                                    )
                      );

        // if you remove this, for the love of god, change the return type so
        // we don't leak the pointer.
        context.ns->addDef(btype.get());
        return btype.get();
    }
}

namespace {
    void finishClassType(Context &context, BTypeDef *classType) {
        // for the kinds of things we're about to do, we need a global block 
        // for functions to restore to, and for that we need a function and 
        // module.
        LLVMContext &lctx = getGlobalContext();
        LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
        builder.module = new Module("<builtin>", lctx);
        vector<const Type *> argTypes;
        FunctionType *voidFuncNoArgs =
            FunctionType::get(Type::getVoidTy(lctx), argTypes, false);
        Function *func = Function::Create(voidFuncNoArgs,
                                          Function::ExternalLinkage,
                                          "__builtin_init__",
                                          builder.module
                                          );
        func->setCallingConv(llvm::CallingConv::C);
        builder.block =
            BasicBlock::Create(lctx, "__builtin_init__", builder.func);

        // add "Class"
        int lineNum = __LINE__ + 1;
        string temp("    byteptr name;\n"
                    "    uint numBases;\n"
                    "    array[Class] bases = null;\n"
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
        ContextPtr classCtx = new Context(context.builder,
                                          Context::instance,
                                          &context,
                                          classType
                                          );

        CompositeNamespacePtr ns = new CompositeNamespace(classType, 
                                                          context.ns.get()
                                                          );
        ContextPtr lexicalContext = new Context(context.builder, 
                                                Context::composite,
                                                classCtx.get(),
                                                ns.get()
                                                );
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
        vector<TypeDefPtr> noBases;
        type->type = metaType =
            createMetaClass(context, type->name, noBases, classImpl);
        metaType->meta = type;
        type->impl = classImpl;
    }

    void addExplicitTruncate(BTypeDef *sourceType,
                             BTypeDef *targetType
                             ) {
        FuncDefPtr func = 
            new GeneralOpDef<TruncOpCall>(targetType, FuncDef::noFlags,
                                          "oper new",
                                          1
                                          );
        func->args[0] = new ArgDef(sourceType, "val");
        targetType->addDef(func.get());
    }

    template <typename opType>
    void addExplicitFPTruncate(BTypeDef *sourceType,
                               BTypeDef *targetType
                               ) {
        FuncDefPtr func =
            new GeneralOpDef<opType>(targetType, FuncDef::noFlags,
                                          "oper new",
                                          1
                                          );
        func->args[0] = new ArgDef(sourceType, "val");
        targetType->addDef(func.get());
    }

}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;
    LLVMContext &lctx = getGlobalContext();

    // create the basic types
    
    BTypeDef *classType;
    Type *classTypeRep = OpaqueType::get(lctx);
    Type *classTypePtrRep = PointerType::getUnqual(classTypeRep);
    gd->classType = classType = new BTypeDef(0, "Class", classTypePtrRep);
    classType->type = classType;
    classType->meta = classType;
    context.ns->addDef(classType);

    // some tools for creating meta-classes
    BTypeDefPtr metaType;           // storage for meta-types
    BGlobalVarDefImplPtr classImpl; // storage for class impls
    vector<TypeDefPtr> noBases;     // empty base class list
    
    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef(context.globalData->classType.get(), 
                                           "void", 
                                           Type::getVoidTy(lctx)
                                           );
    context.ns->addDef(voidType);

    BTypeDef *voidPtrType;
    llvmVoidPtrType = 
        PointerType::getUnqual(OpaqueType::get(getGlobalContext()));
    gd->voidPtrType = voidPtrType = new BTypeDef(context.globalData->classType.get(), 
                                                 "voidptr", 
                                                 llvmVoidPtrType
                                                 );
    context.ns->addDef(voidPtrType);
    
    llvm::Type *llvmBytePtrType = 
        PointerType::getUnqual(Type::getInt8Ty(lctx));
    BTypeDef *byteptrType;
    gd->byteptrType = byteptrType = new BTypeDef(context.globalData->classType.get(), 
                                                 "byteptr", 
                                                 llvmBytePtrType
                                                 );
    byteptrType->defaultInitializer = createStrConst(context, "");
    byteptrType->addDef(
        new VoidPtrOpDef(context.globalData->voidPtrType.get())
    );
    context.ns->addDef(byteptrType);
    
    const Type *llvmBoolType = IntegerType::getInt1Ty(lctx);
    BTypeDef *boolType;
    gd->boolType = boolType = new BTypeDef(context.globalData->classType.get(), 
                                           "bool", 
                                           llvmBoolType
                                           );
    gd->boolType->defaultInitializer = new BIntConst(boolType, 0);
    context.ns->addDef(boolType);
    
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

    // create integer operations
    context.ns->addDef(new AddOpDef(byteType));
    context.ns->addDef(new SubOpDef(byteType));
    context.ns->addDef(new MulOpDef(byteType));
    context.ns->addDef(new SDivOpDef(byteType));
    context.ns->addDef(new SRemOpDef(byteType));
    context.ns->addDef(new ICmpEQOpDef(byteType, boolType));
    context.ns->addDef(new ICmpNEOpDef(byteType, boolType));
    context.ns->addDef(new ICmpSGTOpDef(byteType, boolType));
    context.ns->addDef(new ICmpSLTOpDef(byteType, boolType));
    context.ns->addDef(new ICmpSGEOpDef(byteType, boolType));
    context.ns->addDef(new ICmpSLEOpDef(byteType, boolType));
    context.ns->addDef(new NegOpDef(byteType, "oper -"));
    context.ns->addDef(new BitNotOpDef(byteType, "oper ~"));

    context.ns->addDef(new AddOpDef(uint32Type));
    context.ns->addDef(new SubOpDef(uint32Type));
    context.ns->addDef(new MulOpDef(uint32Type));
    context.ns->addDef(new UDivOpDef(uint32Type));
    context.ns->addDef(new URemOpDef(uint32Type));
    context.ns->addDef(new ICmpEQOpDef(uint32Type, boolType));
    context.ns->addDef(new ICmpNEOpDef(uint32Type, boolType));
    context.ns->addDef(new ICmpUGTOpDef(uint32Type, boolType));
    context.ns->addDef(new ICmpULTOpDef(uint32Type, boolType));
    context.ns->addDef(new ICmpUGEOpDef(uint32Type, boolType));
    context.ns->addDef(new ICmpULEOpDef(uint32Type, boolType));
    context.ns->addDef(new NegOpDef(uint32Type, "oper -"));
    context.ns->addDef(new BitNotOpDef(uint32Type, "oper ~"));

    context.ns->addDef(new AddOpDef(int32Type));
    context.ns->addDef(new SubOpDef(int32Type));
    context.ns->addDef(new MulOpDef(int32Type));
    context.ns->addDef(new SDivOpDef(int32Type));
    context.ns->addDef(new SRemOpDef(int32Type));
    context.ns->addDef(new ICmpEQOpDef(int32Type, boolType));
    context.ns->addDef(new ICmpNEOpDef(int32Type, boolType));
    context.ns->addDef(new ICmpSGTOpDef(int32Type, boolType));
    context.ns->addDef(new ICmpSLTOpDef(int32Type, boolType));
    context.ns->addDef(new ICmpSGEOpDef(int32Type, boolType));
    context.ns->addDef(new ICmpSLEOpDef(int32Type, boolType));
    context.ns->addDef(new NegOpDef(int32Type, "oper -"));
    context.ns->addDef(new BitNotOpDef(int32Type, "oper ~"));

    context.ns->addDef(new AddOpDef(uint64Type));
    context.ns->addDef(new SubOpDef(uint64Type));
    context.ns->addDef(new MulOpDef(uint64Type));
    context.ns->addDef(new UDivOpDef(uint64Type));
    context.ns->addDef(new URemOpDef(uint64Type));
    context.ns->addDef(new ICmpEQOpDef(uint64Type, boolType));
    context.ns->addDef(new ICmpNEOpDef(uint64Type, boolType));
    context.ns->addDef(new ICmpUGTOpDef(uint64Type, boolType));
    context.ns->addDef(new ICmpULTOpDef(uint64Type, boolType));
    context.ns->addDef(new ICmpUGEOpDef(uint64Type, boolType));
    context.ns->addDef(new ICmpULEOpDef(uint64Type, boolType));
    context.ns->addDef(new NegOpDef(uint64Type, "oper -"));
    context.ns->addDef(new BitNotOpDef(uint64Type, "oper ~"));

    context.ns->addDef(new AddOpDef(int64Type));
    context.ns->addDef(new SubOpDef(int64Type));
    context.ns->addDef(new MulOpDef(int64Type));
    context.ns->addDef(new SDivOpDef(int64Type));
    context.ns->addDef(new SRemOpDef(int64Type));
    context.ns->addDef(new ICmpEQOpDef(int64Type, boolType));
    context.ns->addDef(new ICmpNEOpDef(int64Type, boolType));
    context.ns->addDef(new ICmpSGTOpDef(int64Type, boolType));
    context.ns->addDef(new ICmpSLTOpDef(int64Type, boolType));
    context.ns->addDef(new ICmpSGEOpDef(int64Type, boolType));
    context.ns->addDef(new ICmpSLEOpDef(int64Type, boolType));
    context.ns->addDef(new NegOpDef(int64Type, "oper -"));
    context.ns->addDef(new BitNotOpDef(int64Type, "oper ~"));

    // float operations
    context.ns->addDef(new FAddOpDef(float32Type));
    context.ns->addDef(new FSubOpDef(float32Type));
    context.ns->addDef(new FMulOpDef(float32Type));
    context.ns->addDef(new FDivOpDef(float32Type));
    context.ns->addDef(new FRemOpDef(float32Type));
    context.ns->addDef(new FCmpOEQOpDef(float32Type, boolType));
    context.ns->addDef(new FCmpONEOpDef(float32Type, boolType));
    context.ns->addDef(new FCmpOGTOpDef(float32Type, boolType));
    context.ns->addDef(new FCmpOLTOpDef(float32Type, boolType));
    context.ns->addDef(new FCmpOGEOpDef(float32Type, boolType));
    context.ns->addDef(new FCmpOLEOpDef(float32Type, boolType));
    context.ns->addDef(new FNegOpDef(float32Type, "oper -"));

    context.ns->addDef(new FAddOpDef(float64Type));
    context.ns->addDef(new FSubOpDef(float64Type));
    context.ns->addDef(new FMulOpDef(float64Type));
    context.ns->addDef(new FDivOpDef(float64Type));
    context.ns->addDef(new FRemOpDef(float64Type));
    context.ns->addDef(new FCmpOEQOpDef(float64Type, boolType));
    context.ns->addDef(new FCmpONEOpDef(float64Type, boolType));
    context.ns->addDef(new FCmpOGTOpDef(float64Type, boolType));
    context.ns->addDef(new FCmpOLTOpDef(float64Type, boolType));
    context.ns->addDef(new FCmpOGEOpDef(float64Type, boolType));
    context.ns->addDef(new FCmpOLEOpDef(float64Type, boolType));
    context.ns->addDef(new FNegOpDef(float64Type, "oper -"));

    // boolean logic
    context.ns->addDef(new LogicAndOpDef(boolType, boolType));
    context.ns->addDef(new LogicOrOpDef(boolType, boolType));
    
    // implicit conversions (no loss of precision)
    byteType->addDef(new ZExtOpDef(int32Type, "oper to int32"));
    byteType->addDef(new ZExtOpDef(int64Type, "oper to int64"));
    byteType->addDef(new ZExtOpDef(uint32Type, "oper to uint32"));
    byteType->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    byteType->addDef(new UIToFPOpDef(float32Type, "oper to float32"));
    byteType->addDef(new UIToFPOpDef(float64Type, "oper to float64"));
    int32Type->addDef(new SExtOpDef(int64Type, "oper to int64"));
    int32Type->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    int32Type->addDef(new SIToFPOpDef(float64Type, "oper to float64"));
    uint32Type->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    uint32Type->addDef(new ZExtOpDef(int64Type, "oper to int64"));
    uint32Type->addDef(new UIToFPOpDef(float64Type, "oper to float64"));
    float32Type->addDef(new FPExtOpDef(float64Type, "oper to float64"));

    // explicit (loss of precision)
    addExplicitTruncate(int64Type, uint64Type);
    addExplicitTruncate(int64Type, int32Type);
    addExplicitTruncate(int64Type, uint32Type);
    addExplicitTruncate(int64Type, byteType);
    addExplicitTruncate(uint64Type, int64Type);
    addExplicitTruncate(uint64Type, int32Type);
    addExplicitTruncate(uint64Type, uint32Type);
    addExplicitTruncate(uint64Type, byteType);
    addExplicitTruncate(int32Type, byteType);
    addExplicitTruncate(int32Type, uint32Type);
    addExplicitTruncate(int32Type, uint32Type);
    addExplicitTruncate(uint32Type, byteType);
    addExplicitTruncate(uint32Type, int32Type);

    addExplicitFPTruncate<FPTruncOpCall>(float64Type, float32Type);
    addExplicitFPTruncate<FPToUIOpCall>(float32Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(float32Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(float32Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(float32Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(float32Type, uint64Type);
    addExplicitFPTruncate<FPToUIOpCall>(float64Type, byteType);
    addExplicitFPTruncate<FPToSIOpCall>(float64Type, int32Type);
    addExplicitFPTruncate<FPToUIOpCall>(float64Type, uint32Type);
    addExplicitFPTruncate<FPToSIOpCall>(float64Type, int64Type);
    addExplicitFPTruncate<FPToUIOpCall>(float64Type, uint64Type);

    // create the array generic
    TypeDefPtr arrayType = new ArrayTypeDef(context.globalData->classType.get(),
                                            "array", 
                                            0
                                            );
    context.ns->addDef(arrayType.get());

    // now that we have byteptr and array and all of the integer types, we can
    // initialize the body of Class.
    context.ns->addDef(new IsOpDef(classType, boolType));
    finishClassType(context, classType);
    
    // back-fill meta class and impls for the existing primitives
    fixMeta(context, voidType);
    fixMeta(context, voidPtrType);
    fixMeta(context, boolType);
    fixMeta(context, byteType);
    fixMeta(context, int32Type);
    fixMeta(context, int64Type);
    fixMeta(context, uint32Type);
    fixMeta(context, uint64Type);
    fixMeta(context, arrayType.get());

    // create OverloadDef's type
    metaType = createMetaClass(context, "Overload", noBases, classImpl);
    BTypeDefPtr overloadDef = new BTypeDef(metaType.get(), "Overload",
                                           0
                                           );
    metaType->meta = overloadDef.get();
    metaType->impl = classImpl;
        
    // Give it a context and an "oper to voidptr" method.
    overloadDef->addDef(
        new VoidPtrOpDef(context.globalData->voidPtrType.get())
    );
    OverloadDef::overloadType = gd->overloadType = overloadDef;
    
    // create an empty structure type and its pointer for VTableBase 
    // Actual type is {}** (another layer of pointer indirection) because 
    // classes need to be pointer types.
    vector<const Type *> members;
    Type *vtableType = StructType::get(getGlobalContext(), members);
    Type *vtablePtrType = PointerType::getUnqual(vtableType);
    metaType = createMetaClass(context, "VTableBase", noBases, classImpl);
    BTypeDef *vtableBaseType;
    gd->vtableBaseType = vtableBaseType =
        new BTypeDef(metaType.get(), "VTableBase", 
                     PointerType::getUnqual(vtablePtrType), 
                     true
                     );
    vtableBaseType->hasVTable = true;
    vtableBaseType->impl = classImpl;
    metaType->meta = vtableBaseType;
    context.ns->addDef(vtableBaseType);
    createOperClassFunc(context, vtableBaseType, metaType.get());

    // build VTableBase's vtable
    VTableBuilder vtableBuilder(vtableBaseType);
    vtableBaseType->createAllVTables(vtableBuilder, ".vtable.VTableBase", 
                                     vtableBaseType
                                     );
    vtableBuilder.emit(module, vtableBaseType);

    // pointer equality check (to allow checking for None)
    context.ns->addDef(new IsOpDef(voidPtrType, boolType));
    context.ns->addDef(new IsOpDef(byteptrType, boolType));
    
    // boolean not
    context.ns->addDef(new BitNotOpDef(boolType, "oper !"));
    
    // byteptr array indexing
    addArrayMethods(context, byteptrType, byteType);    
}

void LLVMBuilder::loadSharedLibrary(const string &name,
                                    const vector<string> &symbols,
                                    Context &context
                                    ) {
    // leak the handle so the library stays mapped for the life of the process.
    void *handle = dlopen(name.c_str(), RTLD_LAZY);
    if (!handle)
        throw spug::Exception(dlerror());
    for (vector<string>::const_iterator iter = symbols.begin();
         iter != symbols.end();
         ++iter
         ) {
        void *sym = dlsym(handle, iter->c_str());
        if (!sym)
            throw spug::Exception(dlerror());

        // store a stub for the symbol        
        context.ns->addDef(new StubDef(context.globalData->voidType.get(), 
                                       *iter,
                                       sym
                                       )
                           );
    }
}

void LLVMBuilder::registerImport(Context &context, VarDef *varDef) {
    // no-op for LLVM builder.
}

void LLVMBuilder::setArgv(int newArgc, char **newArgv) {
    argc = newArgc;
    argv = newArgv;
}

void LLVMBuilder::run() {
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}

void LLVMBuilder::dump() {
    PassManager passMan;
    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
    passMan.run(*module);
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
        BTypeDefPtr::arcast(context.globalData->vtableBaseType);
    PlaceholderInstruction *vtableInit =
        new IncompleteVTableInit(btype, lastValue, vtableBaseType, block);
    // store it
    btype->addPlaceholder(vtableInit);
}
