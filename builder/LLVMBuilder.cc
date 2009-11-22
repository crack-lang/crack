                
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
            BFuncDef(FuncDef::Flags flags, const string &name,
                     size_t argCount
                     ) :
                model::FuncDef(flags, name, argCount) {
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

    SPUG_RCPTR(IncompleteInstVar);
    
    /**
     * Abstract base for classes that store information on references to 
     * incomplete instance variables (instance variables that were referenced 
     * or assigned prior to the completion of the class)
     */
    class IncompleteInstVar : public spug::RCBase {
        protected:
            // position where we need to do the insert
            BasicBlock *block;
            BasicBlock::iterator pos;
            
            // the variable index
            unsigned index;
            
            // should be a pointer to the aggregate
            Value *aggregate;
            
            // For a reference, this is the placeholder value which needs to 
            // be replaced and delete.  For an assignment, this is the R-value 
            // that we are assigning.
            Value *val;

        public:
            IncompleteInstVar(BasicBlock *block, BasicBlock::iterator pos,
                              unsigned index,
                              Value *aggregate,
                              Value *val
                              ) :
                block(block),
                pos(pos),
                index(index),
                aggregate(aggregate),
                val(val) {
            }

            void fix() {
                IRBuilder<> builder(block, pos);
                builder.SetInsertPoint(block, pos);
                Value *fieldPtr = builder.CreateStructGEP(aggregate, index);
                insertInstructions(builder, fieldPtr);
                pos->eraseFromParent();
            }

            virtual void insertInstructions(IRBuilder<> &builder,
                                            Value *fieldPtr
                                            ) = 0;
    };
    
    /** an incomplete reference to an instance variable. */
    class IncompleteInstVarRef : public IncompleteInstVar {
        public:

            IncompleteInstVarRef(BasicBlock *block, BasicBlock::iterator pos,
                                 unsigned index,
                                 Value *aggregate,
                                 Value *placeholder
                                 ) :
                IncompleteInstVar(block, pos, index, aggregate, placeholder) {
            }
            
            virtual void insertInstructions(IRBuilder<> &builder, 
                                            Value *fieldPtr
                                            ) {
                val->replaceAllUsesWith(builder.CreateLoad(fieldPtr));
                delete val;
            }
    };
    
    class IncompleteInstVarAssign : public IncompleteInstVar {
        public:
            IncompleteInstVarAssign(BasicBlock *block,
                                    BasicBlock::iterator pos,
                                    unsigned index,
                                    Value *aggregate,
                                    Value *val
                                    ) :
                IncompleteInstVar(block, pos, index, aggregate, val) {
            }

            virtual void insertInstructions(IRBuilder<> &builder,
                                            Value *fieldPtr
                                            ) {
                builder.CreateStore(val, fieldPtr);
            };
    };

    class BBuilderContextData : public BuilderContextData {
        public:
            Function *func;
            BasicBlock *block;
            unsigned fieldCount;
            BTypeDefPtr type;
            vector<IncompleteInstVarPtr> incompleteInstVars;
            
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
            
            virtual void emitRef(Context &context, const VarDefPtr &var) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitMemVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, const VarDefPtr &var,
                           const ExprPtr &expr
                           ) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                expr->emit(context);
                b.builder.CreateStore(b.lastValue, rep);
            }
    };
    
    SPUG_RCPTR(BInstVarDefImpl);

    // Impl object for instance variables.  These should never be used to emit 
    // instance variables, so when used they just raise an assertion error.
    class BInstVarDefImpl : public VarDefImpl {
        public:
            unsigned index;
            BInstVarDefImpl(unsigned index) : index(index) {}
            virtual void emitRef(Context &context,
                                 const VarDefPtr &var
                                 ) {
                assert(false && 
                       "attempting to emit a direct reference to a instance "
                       "variable."
                       );
            }
            
            virtual void emitAssignment(Context &context,
                                        const VarDefPtr &var,
                                        const ExprPtr &expr
                                        ) {
                assert(false && 
                       "attempting to assign a direct reference to a instance "
                       "variable."
                       );
            }
    };
    
    class BFieldRef : public VarRef {
        public:
            ExprPtr aggregate;
            BFieldRef(const ExprPtr &aggregate, const VarDefPtr &varDef) :
                aggregate(aggregate),
                VarRef(varDef) {
            }

            void emit(Context &context) {
                aggregate->emit(context);

                unsigned index =
                    dynamic_cast<BInstVarDefImpl *>(def->impl.obj)->index;
                
                // if the variable is from a complete context, we can emit it. 
                //  Otherwise, we need to store a placeholder.
                LLVMBuilder &bb = dynamic_cast<LLVMBuilder &>(context.builder);
                if (def->context->complete) {
                    Value *fieldPtr = 
                        bb.builder.CreateStructGEP(bb.lastValue, index);
                    bb.lastValue = bb.builder.CreateLoad(fieldPtr);
                } else {
                    // create a fixup object for the reference
                    
                    // stash the aggregate.
                    Value *aggregate = bb.lastValue;

                    // we have to emit a fake instruction to serve as a 
                    // placeholder in the block because in some cases, 
                    // emitting the aggregate won't generate an instruction.
                    bb.builder.CreateLoad(bb.lastValue);
                    BasicBlock::iterator lastInst = bb.block->end();
                    --lastInst;

                    BTypeDef *typeDef = 
                        dynamic_cast<BTypeDef *>(def->type.obj);
                    bb.lastValue =
                        new Value(typeDef->rep, Value::ConstantExprVal);
                    IncompleteInstVarPtr fixup =
                        new IncompleteInstVarRef(bb.block, lastInst, index,
                                                 aggregate,
                                                 bb.lastValue
                                                 );
                    
                    // store it in our fixup list for the instance variable's 
                    // containing class
                    BBuilderContextData *bdata =
                        dynamic_cast<BBuilderContextData *>(
                            def->context->builderData.obj
                        );
                    bdata->incompleteInstVars.push_back(fixup);
                }
            }
    };                            

    class BArgVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BArgVarDefImpl(Value *rep) : rep(rep) {}

            virtual void emitRef(Context &context,
                                 const VarDefPtr &var
                                 ) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitArgVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, const VarDefPtr &var,
                           const ExprPtr &expr
                           ) {
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

            FuncBuilder(Context &context, FuncDef::Flags flags,
                        const BTypeDefPtr &returnType,
                        const string &name,
                        size_t argCount,
                        Function::LinkageTypes linkage = 
                            Function::ExternalLinkage
                        ) :
                    context(context),
                    returnType(returnType),
                    funcDef(new BFuncDef(flags, name, argCount)),
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
                FuncDef(FuncDef::noFlags, name, 2) {

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
                                      FuncDef::Flags flags,
                                      const string &name,
                                      const TypeDefPtr &returnType,
                                      const vector<ArgDefPtr> &args) {
    
    // store the current function and block in the context
    BBuilderContextData *contextData;
    context.builderData = contextData = new BBuilderContextData();
    contextData->func = func;
    contextData->block = block;

    // create the function
    FuncBuilder f(context, flags, returnType, name, args.size());
    f.setArgs(args);
    
    // see if this is a method - assuming that methods are nested exactly one 
    // level within the class, which may not be valid.
    if (flags & FuncDef::method) {
        BuilderContextData *contextData0 = context.parents[0]->builderData.obj;
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
    bdata->type = new BTypeDef(name, PointerType::getUnqual(OpaqueType::get()));
    bdata->type->defaultInitializer = new MallocExpr(bdata->type);
    return TypeDefPtr::ucast(bdata->type);
}

void LLVMBuilder::emitEndClass(Context &context) {
    // build a vector of the instance variables
    vector<const Type *> members;
    vector<BInstVarDefImplPtr> instVarImpls;
    for (Context::VarDefMap::iterator iter = context.beginDefs();
         iter != context.endDefs();
         ++iter
         ) {
        // see if the variable needs an instance slot
        if (iter->second->hasInstSlot()) {
            BInstVarDefImplPtr impl = 
                BInstVarDefImplPtr::dcast(iter->second->impl);
            instVarImpls.push_back(impl);
            
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
        
    PointerType *ptrType =
        cast<PointerType>(const_cast<Type *>(bdata->type->rep));
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type*>(ptrType->getElementType()));
    Type *newType = StructType::get(members);
    curType->refineAbstractTypeTo(newType);

    // fix all instance variable uses that were created before the structure 
    // was defined.
    for (vector<IncompleteInstVarPtr>::iterator iter = 
            bdata->incompleteInstVars.begin();
         iter != bdata->incompleteInstVars.end();
         ++iter
         )
        (*iter)->fix();
    bdata->incompleteInstVars.clear();
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

VarRefPtr LLVMBuilder::createFieldRef(const ExprPtr &aggregate,
                                      const VarDefPtr &varDef
                                      ) {
    return new BFieldRef(aggregate, varDef);
}

void LLVMBuilder::emitFieldAssign(Context &context,
                                  const ExprPtr &aggregate,
                                  const VarDefPtr &varDef,
                                  const ExprPtr &val
                                  ) {
    aggregate->emit(context);
    Value *aggregateRep = lastValue;
    
    // emit the value last, lastValue after this needs to be the expression so 
    // we can chain assignments.
    val->emit(context);

    unsigned index = dynamic_cast<BInstVarDefImpl *>(varDef->impl.obj)->index;
    Context *varContext = varDef->context;
    
    // if the variable is part of a complete context, just do the store.  
    // Otherwise create a fixup.
    if (varContext->complete) {
        Value *fieldRef = builder.CreateStructGEP(aggregateRep, index);
        builder.CreateStore(lastValue, fieldRef);
    } else {
        // store a load just so we can be sure that we have a 
        // placeholder and get it's position.
        builder.CreateLoad(aggregateRep);
        BasicBlock::iterator pos = block->end();
        --pos;

        BBuilderContextData *bdata =
            dynamic_cast<BBuilderContextData *>(varContext->builderData.obj);
        
        IncompleteInstVarPtr fixup =
            new IncompleteInstVarAssign(block, pos, index,
                                        aggregateRep,
                                        lastValue
                                        );
        bdata->incompleteInstVars.push_back(fixup);
    }
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
        FuncBuilder f(context, FuncDef::noFlags, gd->int32Type, "puts", 1);
        f.addArg("text", gd->byteptrType);
        f.finish();
    }
    
    // create "int write(int, String, int)"
    {
        FuncBuilder f(context, FuncDef::noFlags, gd->int32Type, "write", 3);
        f.addArg("fd", gd->int32Type);
        f.addArg("buf", gd->byteptrType);
        f.addArg("n", gd->int32Type);
        f.finish();
    }
    
    // create "void printint(int32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, gd->voidType, "printint", 1);
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
