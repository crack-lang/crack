
#include "LLVMBuilder.h"

#include <dlfcn.h>

// LLVM includes
#include <stddef.h>
#include <stdlib.h>
#include "llvm/LinkAllPasses.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/PassManager.h"
#include "llvm/CallingConv.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetData.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <model/ArgDef.h>
#include <model/Branchpoint.h>
#include <model/BuilderContextData.h>
#include <model/VarDefImpl.h>
#include <model/Context.h>
#include <model/FuncDef.h>
#include <model/FuncCall.h>
#include <model/IntConst.h>
#include <model/StrConst.h>
#include <model/TypeDef.h>
#include <model/VarDef.h>
#include <model/VarRef.h>


using namespace builder;

using namespace std;
using namespace llvm;
using namespace model;
typedef model::FuncCall::ExprVector ExprVector;

namespace {

    SPUG_RCPTR(BFuncDef);

    class BFuncDef : public model::FuncDef {
        public:
            llvm::Function *rep;
            BFuncDef(const string &name, size_t argCount) :
                model::FuncDef(name, argCount) {
            }
            
    };
        
    SPUG_RCPTR(BTypeDef)

    class BTypeDef : public model::TypeDef {
        public:
            const Type *rep;
            BTypeDef(const string &name, const llvm::Type *rep) :
                model::TypeDef(name),
                rep(rep) {
            }
    };
    
    SPUG_RCPTR(BStrConst);

    class BStrConst : public model::StrConst {
        public:
            // XXX need more specific type?
            llvm::Value *rep;
            BStrConst(const TypeDefPtr &type, const std::string &val) :
                StrConst(type, val),
                rep(0) {
            }
    };
    
    class BIntConst : public model::IntConst {
        public:
            llvm::Value *rep;
            BIntConst(const TypeDefPtr &type, long val) :
                IntConst(type, val),
                rep(llvm::ConstantInt::get(llvm::Type::Int32Ty, val)) {
            }
    };
    
    SPUG_RCPTR(BBranchpoint);

    class BBranchpoint : public model::Branchpoint {
        public:
            BasicBlock *block, *block2;
            
            BBranchpoint(BasicBlock *block) : block(block), block2(0) {}
    };

    class BBuilderContextData : public BuilderContextData {
        public:
            Function *func;
            BasicBlock *block;
            unsigned fieldCount;
            BTypeDefPtr type;
            
            BBuilderContextData() :
                func(0),
                block(0),
                fieldCount(0) {
            }
    };
    
    // generates references for 
    class BMemVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BMemVarDefImpl(Value *rep) : rep(rep) {}
            
            virtual void emitRef(Context &context) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitMemVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, const ExprPtr &expr) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                expr->emit(context);
                b.builder.CreateStore(b.lastValue, rep);
            }
    };
    
    SPUG_RCPTR(BInstVarDefImpl);

    // Impl object for instance variables.
    class BInstVarDefImpl : public VarDefImpl {
        public:
            unsigned index;

            BInstVarDefImpl(unsigned index) : index(index) {}
            virtual void emitRef(Context &context) {
                VarDefPtr thisVar = context.lookUp("this");
                assert(thisVar);

                // XXX we can probably cache these values in the 
                // ContextBuilderData for thisVar->context.
                thisVar->impl->emitRef(context);
                LLVMBuilder &bbuilder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                bbuilder.lastValue->dump();
                Value *body = bbuilder.builder.CreateLoad(bbuilder.lastValue);
                bbuilder.lastValue =
                    bbuilder.builder.CreateExtractValue(bbuilder.lastValue,
                                                        index
                                                        );
            }
            
            virtual void emitAssignment(Context &context,
                                        const ExprPtr &expr) {
                VarDefPtr thisVar = context.lookUp("this");
                assert(thisVar);

                LLVMBuilder &bbuilder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                
                // XXX again, may want to cache the "load %this"
                thisVar->impl->emitRef(context);
                Value *body = bbuilder.builder.CreateLoad(bbuilder.lastValue);
                expr->emit(context);
                bbuilder.builder.CreateInsertValue(body, bbuilder.lastValue,
                                                   index
                                                   );
            }
    };
    
    class BArgVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BArgVarDefImpl(Value *rep) : rep(rep) {}

            virtual void emitRef(Context &context) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitArgVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, const ExprPtr &expr) {
                // XXX implement argument assignment
                assert(false && "can't assign arguments yet");
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
//                b.emitArgVarAssgn(context, rep);
            }
    };

    class FuncBuilder {
        public:
            Context &context;
            BTypeDefPtr returnType;
            BTypeDefPtr receiverType;
            BFuncDefPtr funcDef;
            int argIndex;
            Function::LinkageTypes linkage;

            FuncBuilder(Context &context, const BTypeDefPtr &returnType,
                        const string &name,
                        size_t argCount,
                        Function::LinkageTypes linkage = 
                            Function::ExternalLinkage
                        ) :
                    context(context),
                    returnType(returnType),
                    funcDef(new BFuncDef(name, argCount)),
                    linkage(linkage),
                    argIndex(0) {
                funcDef->type = returnType;
            }
            
            void finish(bool storeDef = true) {
                size_t argCount = funcDef->args.size();
                assert(argIndex == argCount);
                vector<const Type *> llvmArgs(argCount + 
                                               (receiverType ? 1 : 0)
                                              );
                
                // create the array of LLVM arguments
                int i = 0;
                if (receiverType)
                    llvmArgs[i++] = receiverType->rep;
                for (vector<ArgDefPtr>::iterator iter = 
                        funcDef->args.begin();
                     iter != funcDef->args.end();
                     ++iter, ++i)
                    llvmArgs[i] = BTypeDefPtr::dcast((*iter)->type)->rep;

                // register the function with LLVM
                const Type *rawRetType =
                    returnType->rep ? returnType->rep : Type::VoidTy;
                FunctionType *funcType =
                    FunctionType::get(rawRetType, llvmArgs, false);
                LLVMBuilder &builder = 
                    dynamic_cast<LLVMBuilder &>(context.builder);
                Function *func = Function::Create(funcType,
                                                  linkage,
                                                  funcDef->name,
                                                  builder.module
                                                  );
                func->setCallingConv(llvm::CallingConv::C);

                // back-fill builder data and set arg names
                Function::arg_iterator llvmArg = func->arg_begin();
                vector<ArgDefPtr>::const_iterator crackArg =
                    funcDef->args.begin();
                if (receiverType) {
                    llvmArg->setName("this");
                    
                    // add the implementation to the "this" var
                    VarDefPtr thisDef = context.lookUp("this");
                    assert(thisDef &&
                            "missing 'this' variable in the context of a "
                            "function with a receiver"
                           );
                    thisDef->impl = new BArgVarDefImpl(llvmArg);
                    ++llvmArg;
                }
                for (; llvmArg != func->arg_end(); ++llvmArg, ++crackArg) {
                    llvmArg->setName((*crackArg)->name);
            
                    // need the address of the value here because it is going 
                    // to be used in a "load" context.
                    (*crackArg)->impl = new BArgVarDefImpl(llvmArg);
                }
                
                funcDef->rep = func;
                if (storeDef)
                    context.addDef(VarDefPtr::ucast(funcDef));
            }

            void addArg(const char *name, const TypeDefPtr &type) {
                assert(argIndex <= funcDef->args.size());
                funcDef->args[argIndex++] = new ArgDef(type, name);
            }
            
            void setArgs(const vector<ArgDefPtr> &args) {
                assert(argIndex == 0 && args.size() == funcDef->args.size());
                argIndex = args.size();
                funcDef->args = args;
            }
            
            void setReceiverType(const BTypeDefPtr &type) {
                receiverType = type;
            }
                
    };

    // weird stuff
    
    class MallocExpr : public Expr {
        public:
            MallocExpr(const TypeDefPtr &type) : Expr(type) {}
            
            void emit(Context &context) {
                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                BTypeDef *btype = dynamic_cast<BTypeDef *>(type.obj);
                PointerType *tp =
                    cast<PointerType>(const_cast<Type *>(btype->rep));
                builder.lastValue =
                    builder.builder.CreateMalloc(tp->getElementType());
            }
    };
    
    // primitive operations

    SPUG_RCPTR(BinOpDef);

    class BinOpDef : public FuncDef {
        public:
            BinOpDef(const TypeDefPtr &tp,
                     const string &name) :
                FuncDef(name, 2) {

                args[0] = new ArgDef(tp, "lhs");
                args[1] = new ArgDef(tp, "rhs");
                type = tp;
            }
            
            virtual void emitCall(Context &context, 
                                  const ExprPtr &lhs,
                                  const ExprPtr &rhs
                                  ) = 0;
    };

#define BINOP(opCode, op)                                                   \
    class opCode##OpDef : public BinOpDef {                                   \
        public:                                                             \
            opCode##OpDef(const TypeDefPtr &type) :                           \
                BinOpDef(type, "oper " op) {                                \
            }                                                               \
                                                                            \
            virtual void emitCall(Context &context,                         \
                                  const ExprPtr &lhs,                       \
                                  const ExprPtr &rhs                        \
                                  ) {                                       \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                                                                            \
                lhs->emit(context);                                         \
                Value *lhsVal = builder.lastValue;                          \
                rhs->emit(context);                                         \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(lhsVal,                  \
                                                   builder.lastValue        \
                                                   );                       \
            }                                                               \
    };

    BINOP(Add, "+");
    BINOP(Sub, "-");
    BINOP(Mul, "*");
    BINOP(SDiv, "/");

} // anon namespace

LLVMBuilder::LLVMBuilder() :
    module(0),
    func(0),
    block(0),
    lastValue(0) {
}

void LLVMBuilder::emitFuncCall(Context &context,
                               const FuncDefPtr &funcDef, 
                               const ExprPtr &receiver,
                               const FuncCall::ExprVector &args) {

    // see if this is s special function
    BinOpDef *binOp = dynamic_cast<BinOpDef *>(funcDef.obj);
    if (binOp) {
        assert(args.size() == 2);
        binOp->emitCall(context, args[0], args[1]);
        return;
    }
                    
    // get the LLVM arg list from the receiver and the argument expressions
    vector<Value*> valueArgs;
    
    // if there's a receiver, use it as the first argument.
    if (receiver) {
        receiver->emit(context);
        valueArgs.push_back(lastValue);
    }
    
    // emit the arguments
    for (ExprVector::const_iterator iter = args.begin(); iter < args.end(); 
         ++iter) {
        (*iter)->emit(context);
        valueArgs.push_back(lastValue);
    }
    
    lastValue =
        builder.CreateCall(BFuncDefPtr::dcast(funcDef)->rep, valueArgs.begin(),
                           valueArgs.end()
                           );
}

void LLVMBuilder::emitStrConst(model::Context &context, 
                               const StrConstPtr &val) {
    BStrConstPtr bval = BStrConstPtr::dcast(val);
    // if the global string hasn't been defined yet, create it
    if (!bval->rep) {
        bval->rep = builder.CreateGlobalStringPtr(val->val.c_str());
    }
    lastValue = bval->rep;
}

void LLVMBuilder::emitIntConst(model::Context &context, const IntConst &val) {
    lastValue = dynamic_cast<const BIntConst &>(val).rep;
}

BranchpointPtr LLVMBuilder::emitIf(model::Context &context,
                                   const model::ExprPtr &cond) {
    // stash the current block and the "false condition" block in the result 
    // branchpoint and create a new block for the condition
    BBranchpointPtr result = new BBranchpoint(BasicBlock::Create("cond_false",
                                                                 func
                                                                 )
                                              );
    BasicBlock *trueBlock = BasicBlock::Create("cond_true", func);

    cond->emit(context);
    // XXX I think we need a "conditional" type so we don't have to convert 
    // everything to a boolean and then check for non-zero.
    BTypeDefPtr boolType =
        BTypeDefPtr::dcast(context.globalData->boolType);
    Value *comparison =
        builder.CreateICmpNE(lastValue, Constant::getNullValue(boolType->rep));
    builder.CreateCondBr(comparison, trueBlock, result->block);
    
    // repoint to the new ("if true") block
    builder.SetInsertPoint(block = trueBlock);
    return BranchpointPtr::ucast(result);
}

BranchpointPtr LLVMBuilder::emitElse(model::Context &context,
                                     const model::BranchpointPtr &pos) {
    BBranchpointPtr bpos = BBranchpointPtr::dcast(pos);

    // create a block to come after the else and jump to it from the current 
    // "if true" block.
    BasicBlock *falseBlock = bpos->block;
    bpos->block = BasicBlock::Create("cond_end", func);
    builder.CreateBr(bpos->block);
    
    // new block is the "false" condition
    block = falseBlock;
    builder.SetInsertPoint(block);
    return pos;
}
        
void LLVMBuilder::emitEndIf(model::Context &context,
                            const model::BranchpointPtr &pos) {
    BBranchpointPtr bpos = BBranchpointPtr::dcast(pos);

    // branch from the current block to the next block
    builder.CreateBr(bpos->block);

    // new block is the next block
    builder.SetInsertPoint(block = bpos->block);
}

BranchpointPtr LLVMBuilder::emitBeginWhile(Context &context, 
                                           const ExprPtr &cond) {
    BBranchpointPtr bpos = new BBranchpoint(BasicBlock::Create("while_end", 
                                                               func
                                                               )
                                            );

    BasicBlock *whileCond = bpos->block2 =
        BasicBlock::Create("while_cond", func);
    BasicBlock *whileBody = BasicBlock::Create("while_body", func);
    builder.CreateBr(whileCond);
    builder.SetInsertPoint(block = whileCond);

    // XXX see notes above on a conditional type.
    cond->emit(context);
    BTypeDefPtr boolType =
        BTypeDefPtr::dcast(context.globalData->boolType);
    Value *comparison =
        builder.CreateICmpNE(lastValue, Constant::getNullValue(boolType->rep));
    builder.CreateCondBr(comparison, whileBody, bpos->block);

    // begin generating code in the while body    
    builder.SetInsertPoint(block = whileBody);

    return bpos;
}

void LLVMBuilder::emitEndWhile(Context &context, const BranchpointPtr &pos) {
    BBranchpointPtr bpos = BBranchpointPtr::dcast(pos);

    // emit the branch back to conditional expression in the block
    builder.CreateBr(bpos->block2);

    // new code goes to the following block
    builder.SetInsertPoint(block = bpos->block);
}

Value *emitGEP(IRBuilder<> &builder, Value *obj) {
    Value *zero = llvm::ConstantInt::get(llvm::Type::Int32Ty, 0);
    Value *gepArgs[] = { zero, zero };
    return builder.CreateGEP(obj, zero);
}
    
FuncDefPtr LLVMBuilder::emitBeginFunc(Context &context,
                                      const string &name,
                                      const TypeDefPtr &returnType,
                                      const vector<ArgDefPtr> &args) {
    
    // store the current function and block in the context
    BBuilderContextData *contextData;
    context.builderData = contextData = new BBuilderContextData();
    contextData->func = func;
    contextData->block = block;

    // create the function
    FuncBuilder f(context, returnType, name, args.size());
    f.setArgs(args);
    
    // see if this is a method - assuming that methods are nested exactly one 
    // level within the class, which may not be valid.
    if (context.parent && context.parent->scope == Context::instance) {
        BuilderContextData *contextData0 = context.parent->builderData.obj;
        BBuilderContextData *contextData = 
            dynamic_cast<BBuilderContextData *>(contextData0);
        f.setReceiverType(contextData->type);
    }

    f.finish(false);

    func = f.funcDef->rep;
    block = BasicBlock::Create(name, func);
    builder.SetInsertPoint(block);
    
    return f.funcDef;
}    

void LLVMBuilder::emitEndFunc(model::Context &context,
                              const FuncDefPtr &funcDef) {
    // restore the block and function
    BBuilderContextData *contextData =
        dynamic_cast<BBuilderContextData *>(context.builderData.obj);
    func = contextData->func;
    builder.SetInsertPoint(block = contextData->block);
}

TypeDefPtr LLVMBuilder::emitBeginClass(Context &context,
                                       const string &name,
                                       const vector<TypeDefPtr> bases) {
    assert(!context.builderData);
    BBuilderContextData *bdata;
    context.builderData = bdata = new BBuilderContextData();
    bdata->type = new BTypeDef(name, OpaqueType::get());
    bdata->type->defaultInitializer = new MallocExpr(bdata->type);
    return TypeDefPtr::ucast(bdata->type);
}

void LLVMBuilder::emitEndClass(Context &context) {
    // build a vector of the instance variables
    vector<const Type *> members;
    for (Context::VarDefMap::iterator iter = context.beginDefs();
         iter != context.endDefs();
         ++iter
         ) {
        // see if the variable needs an instance slot
        if (iter->second->hasInstSlot()) {
            BInstVarDefImplPtr impl = 
                BInstVarDefImplPtr::dcast(iter->second->impl);
            
            // resize the set of members if the new guy doesn't fit
            if (impl->index >= members.size())
                members.resize(impl->index + 1, 0);
            
            // get the underlying type object, add it to the vector
            BTypeDefPtr typeDef = BTypeDefPtr::dcast(iter->second->type);
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
    BBuilderContextData *bdata =
        dynamic_cast<BBuilderContextData *>(context.builderData.obj);
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type *>(bdata->type->rep));
    Type *newType = PointerType::getUnqual(StructType::get(members));
    curType->refineAbstractTypeTo(newType);
    bdata->type->rep = newType;
}

void LLVMBuilder::emitReturn(model::Context &context,
                             const model::ExprPtr &expr) {
    
    if (expr) {
        expr->emit(context);
        builder.CreateRet(lastValue);
    } else {
        builder.CreateRetVoid();
    }
}

VarDefPtr LLVMBuilder::emitVarDef(Context &context, const TypeDefPtr &type,
                                  const string &name,
                                  const ExprPtr &initializer,
                                  bool staticScope
                                  ) {
    // do initializion
    if (initializer)
        initializer->emit(context);
    else
        type->defaultInitializer->emit(context);
    
    // XXX use InternalLinkage for variables starting with _ (I think that 
    // might work)
    BTypeDef *tp = dynamic_cast<BTypeDef *>(type.obj);
    
    Value *var = 0;
    switch (context.scope) {

        case Context::instance:
            // class statics share the same context as instance variables: 
            // they are distinguished from instance variables by their 
            // declaration and are equivalent to module scoped globals in the 
            // way they are emitted, so if the staticScope flag is set we want 
            // to fall through to module scope
            if (!staticScope) {
                // first, we need to determine the index of the new field.
                BBuilderContextData *bdata =
                    dynamic_cast<BBuilderContextData *>(
                        context.builderData.obj
                    );
                unsigned idx = bdata->fieldCount++;
                
                // instance variables are unlike the other stored types - we 
                // use a different kind of implementation object.
                VarDefPtr varDef = new VarDef(type, name);
                varDef->impl = new BInstVarDefImpl(idx);
                return varDef;
            }
                
        case Context::module:
            var = new GlobalVariable(tp->rep,
                                    false, // isConstant
                                    GlobalValue::ExternalLinkage, // linkage tp
                                    
                                    // initializer - this needs to be provided 
                                    // or the global will be treated as an 
                                    // extern.
                                    Constant::getNullValue(tp->rep),
                                    name,
                                    module
                                    );
            break;

        case Context::local:
            var = builder.CreateAlloca(tp->rep, 0);
            break;
        
        default:
            assert(false && "invalid context value!");
    }
    
    // allocate the variable and assign it
    lastValue = builder.CreateStore(lastValue, var);
    
    // create the definition object.
    VarDefPtr varDef = new VarDef(type, name);
    varDef->impl = new BMemVarDefImpl(var);
    return varDef;
}
 
void LLVMBuilder::createModule(const char *name) {
    assert(!module);
    module = new llvm::Module(name);
    llvm::Constant *c =
        module->getOrInsertFunction("__main__", llvm::Type::VoidTy, NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create("__main__", func);
    builder.SetInsertPoint(block);
}

void LLVMBuilder::closeModule() {
    assert(module);
    builder.CreateRetVoid();
    llvm::verifyModule(*module, llvm::PrintMessageAction);
    
    // create the execution engine
    execEng = llvm::ExecutionEngine::create(module);

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

model::StrConstPtr LLVMBuilder::createStrConst(model::Context &context,
                                               const std::string &val) {
    return new BStrConst(context.globalData->byteptrType, val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, long val) {
    // XXX probably need to consider the simplest type that the constant can 
    // fit into (compatibility rules will allow us to coerce it into another 
    // type)
    return new BIntConst(context.globalData->int32Type, val);
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(const FuncDefPtr &func) {
    return new FuncCall(func);
}
    
model::FuncDefPtr LLVMBuilder::createFuncDef(const char *name) {
    return new BFuncDef(name, 0);
}

ArgDefPtr LLVMBuilder::createArgDef(const TypeDefPtr &type,
                                    const string &name
                                    ) {
    // we don't create BBuilderVarDefData for these yet - we will back-fill 
    // the builder data when we create the function object.
    ArgDefPtr argDef = new ArgDef(type, name);
    return argDef;
}

VarRefPtr LLVMBuilder::createVarRef(const VarDefPtr &varDef) {
    return new VarRef(varDef);
}

extern "C" void printint(int val) {
    std::cout << val << flush;
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;

    // create the basic types
    
    gd->voidType = new BTypeDef("void", 0);
    context.addDef(gd->voidType);
    
    llvm::Type *llvmBytePtrType = 
        PointerType::getUnqual(llvm::IntegerType::get(8));
    gd->byteptrType = new BTypeDef("byteptr", llvmBytePtrType);
    gd->byteptrType->defaultInitializer = createStrConst(context, "");
    context.addDef(gd->byteptrType);
    
    const llvm::Type *llvmInt32Type = llvm::IntegerType::get(32);
    gd->int32Type = new BTypeDef("int32", llvmInt32Type);
    gd->int32Type->defaultInitializer = createIntConst(context, 0);
    context.addDef(gd->int32Type);
    
    // XXX using bool = int32 for now
    gd->boolType = gd->int32Type;
    
    // create "int puts(String)"
    {
        FuncBuilder f(context, gd->int32Type, "puts", 1);
        f.addArg("text", gd->byteptrType);
        f.finish();
    }
    
    // create "int write(int, String, int)"
    {
        FuncBuilder f(context, gd->int32Type, "write", 3);
        f.addArg("fd", gd->int32Type);
        f.addArg("buf", gd->byteptrType);
        f.addArg("n", gd->int32Type);
        f.finish();
    }
    
    // create "void printint(int32)"
    {
        FuncBuilder f(context, gd->voidType, "printint", 1);
        f.addArg("val", gd->int32Type);
        f.finish();
    }
    
    // create integer operations
    context.addDef(new AddOpDef(gd->int32Type));
    context.addDef(new SubOpDef(gd->int32Type));
    context.addDef(new MulOpDef(gd->int32Type));
    context.addDef(new SDivOpDef(gd->int32Type));
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
