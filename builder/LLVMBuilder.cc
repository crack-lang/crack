                
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
            BStrConst(TypeDef *type, const std::string &val) :
                StrConst(type, val),
                rep(0) {
            }
    };
    
    class BIntConst : public model::IntConst {
        public:
            llvm::Value *rep;
            BIntConst(TypeDef *type, long val) :
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

    /**
     * This is a special instruction that serves as a placeholder for 
     * operations where we dereference incomplete types.  These get stored in 
     * a block and subsequently replaced with a reference to the actual type.
     */    
    class PlaceholderInstruction : public Instruction {
        protected:
            Value *aggregate;
            unsigned index;

        public:
            PlaceholderInstruction(const Type *type, Value *aggregate,
                                   unsigned index,
                                   BasicBlock *parent
                                   ) :
                Instruction(type, OtherOpsEnd + 1, 0, 0, parent),
                aggregate(aggregate),
                index(index) {
            }

            PlaceholderInstruction(const Type *type, 
                                   Value *aggregate,
                                   unsigned index,
                                   Instruction *insertBefore = 0
                                   ) :
                Instruction(type, OtherOpsEnd + 1, 0, 0, insertBefore),
                aggregate(aggregate),
                index(index) {
            }
            
            void *operator new(size_t s) {
                return User::operator new(s, 0);
            }

            /** Replace the placeholder with a real instruction. */
            void fix() {
                IRBuilder<> builder(getParent(), this);
                Value *fieldPtr = builder.CreateStructGEP(aggregate, index);
                insertInstructions(builder, fieldPtr);
                this->eraseFromParent();
                // ADD NO CODE AFTER SELF-DELETION.
            }
            
            virtual void insertInstructions(IRBuilder<> &builder,
                                            Value *fieldPtr
                                            ) = 0;
    };

    /** an incomplete reference to an instance variable. */
    class IncompleteInstVarRef : public PlaceholderInstruction {
        public:

            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 BasicBlock *parent
                                 ) :
                PlaceholderInstruction(type, aggregate, index, parent) {
            }
            
            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 Instruction *insertBefore = 0
                                 ) :
                PlaceholderInstruction(type, aggregate, index, insertBefore) {
            }

            Instruction *clone() const {
                return new IncompleteInstVarRef(getType(), aggregate, index);
            }
            
            virtual void insertInstructions(IRBuilder<> &builder, 
                                            Value *fieldPtr
                                            ) {
                replaceAllUsesWith(builder.CreateLoad(fieldPtr));
            }
    };
    
    class IncompleteInstVarAssign : public PlaceholderInstruction {
        private:
            Value *rval;

        public:

            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    BasicBlock *parent
                                    ) :
                PlaceholderInstruction(type, aggregate, index, parent),
                rval(rval) {
            }
            
            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    Instruction *insertBefore = 0
                                    ) :
                PlaceholderInstruction(type, aggregate, index, insertBefore),
                rval(rval) {
            }

            Instruction *clone() const {
                return new IncompleteInstVarAssign(getType(), aggregate, index, 
                                                   rval
                                                   );
            }
            
            virtual void insertInstructions(IRBuilder<> &builder,
                                            Value *fieldPtr
                                            ) {
                builder.CreateStore(rval, fieldPtr);
            };
    };

    /**
     * A placeholder for a "narrower" - a GEP instruction that provides
     * pointer to a base class from a derived class.
     */
    class IncompleteNarrower : public PlaceholderInstruction {
        public:
            IncompleteNarrower(const Type *type, Value *aggregate,
                               unsigned index,
                               BasicBlock *parent
                               ) :
                PlaceholderInstruction(type, aggregate, index, parent) {
            }
            
            IncompleteNarrower(const Type *type, Value *aggregate,
                               unsigned index,
                               Instruction *insertBefore = 0
                               ) :
                PlaceholderInstruction(type, aggregate, index, insertBefore) {
            }

            Instruction *clone() const {
                return new IncompleteNarrower(getType(), aggregate, index);
            }
            

            virtual void insertInstructions(IRBuilder<> &builder,
                                            Value *fieldPtr
                                            ) {
                replaceAllUsesWith(fieldPtr);
            };
    };

    SPUG_RCPTR(BBuilderContextData);

    class BBuilderContextData : public BuilderContextData {
        public:
            Function *func;
            BasicBlock *block;
            unsigned fieldCount;
            BTypeDefPtr type;
            vector<PlaceholderInstruction *> placeholders;
            
            BBuilderContextData() :
                func(0),
                block(0),
                fieldCount(0) {
            }
            
            void addBaseClass(const BTypeDefPtr &base) {
                ++fieldCount;
            }
    };
    
    // generates references for 
    class BMemVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BMemVarDefImpl(Value *rep) : rep(rep) {}
            
            virtual void emitRef(Context &context, VarDef *var) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitMemVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, VarDef *var,
                           Expr *expr
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
                                 VarDef *var
                                 ) {
                assert(false && 
                       "attempting to emit a direct reference to a instance "
                       "variable."
                       );
            }
            
            virtual void emitAssignment(Context &context,
                                        VarDef *var,
                                        Expr *expr
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
            BFieldRef(Expr *aggregate, VarDef *varDef) :
                aggregate(aggregate),
                VarRef(varDef) {
            }

            void emit(Context &context) {
                aggregate->emit(context);

                // narrow to the ancestor type where there variable is defined.
                aggregate->type->emitNarrower(*def->context->returnType);

                unsigned index = BInstVarDefImplPtr::rcast(def->impl)->index;
                
                // if the variable is from a complete context, we can emit it. 
                //  Otherwise, we need to store a placeholder.
                LLVMBuilder &bb = dynamic_cast<LLVMBuilder &>(context.builder);
                if (def->context->complete) {
                    Value *fieldPtr = 
                        bb.builder.CreateStructGEP(bb.lastValue, index);
                    bb.lastValue = bb.builder.CreateLoad(fieldPtr);
                } else {
                    // create a fixup object for the reference
                    BTypeDef *typeDef = 
                        BTypeDefPtr::rcast(def->type);

                    // stash the aggregate, emit a placeholder for the 
                    // reference
                    Value *aggregate = bb.lastValue;
                    PlaceholderInstruction *placeholder =
                        new IncompleteInstVarRef(typeDef->rep, aggregate,
                                                 index,
                                                 bb.block
                                                 );
                    bb.lastValue = placeholder;

                    // store the placeholder
                    BBuilderContextData *bdata =
                        BBuilderContextDataPtr::rcast(
                            def->context->builderData
                        );
                    bdata->placeholders.push_back(placeholder);
                }
            }
    };                            

    class BArgVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BArgVarDefImpl(Value *rep) : rep(rep) {}

            virtual void emitRef(Context &context,
                                 VarDef *var
                                 ) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitArgVarRef(context, rep);
            }
            
            virtual void 
            emitAssignment(Context &context, VarDef *var,
                           Expr *expr
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
                        BTypeDef *returnType,
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
                    llvmArgs[i] = BTypeDefPtr::rcast((*iter)->type)->rep;

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
                    context.addDef(funcDef.get());
            }

            void addArg(const char *name, TypeDef *type) {
                assert(argIndex <= funcDef->args.size());
                funcDef->args[argIndex++] = new ArgDef(type, name);
            }
            
            void setArgs(const vector<ArgDefPtr> &args) {
                assert(argIndex == 0 && args.size() == funcDef->args.size());
                argIndex = args.size();
                funcDef->args = args;
            }
            
            void setReceiverType(BTypeDef *type) {
                receiverType = type;
            }
                
    };

    // weird stuff
    
    class MallocExpr : public Expr {
        public:
            MallocExpr(TypeDef *type) : Expr(type) {}
            
            void emit(Context &context) {
                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                BTypeDef *btype = BTypeDefPtr::rcast(type);
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
            BinOpDef(TypeDef *tp,
                     const string &name) :
                FuncDef(FuncDef::noFlags, name, 2) {

                args[0] = new ArgDef(tp, "lhs");
                args[1] = new ArgDef(tp, "rhs");
                type = tp;
            }
            
            virtual void emitCall(Context &context, 
                                  Expr *lhs,
                                  Expr *rhs
                                  ) = 0;
    };

#define BINOP(opCode, op)                                                   \
    class opCode##OpDef : public BinOpDef {                                   \
        public:                                                             \
            opCode##OpDef(TypeDef *type) :                           \
                BinOpDef(type, "oper " op) {                                \
            }                                                               \
                                                                            \
            virtual void emitCall(Context &context,                         \
                                  Expr *lhs,                       \
                                  Expr *rhs                        \
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
                               FuncDef *funcDef, 
                               Expr *receiver,
                               const FuncCall::ExprVector &args) {

    // see if this is s special function
    BinOpDef *binOp = BinOpDefPtr::cast(funcDef);
    if (binOp) {
        assert(args.size() == 2);
        binOp->emitCall(context, args[0].get(), args[1].get());
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
        builder.CreateCall(BFuncDefPtr::cast(funcDef)->rep, valueArgs.begin(),
                           valueArgs.end()
                           );
}

void LLVMBuilder::emitStrConst(Context &context, StrConst *val) {
    BStrConst *bval = BStrConstPtr::cast(val);
    // if the global string hasn't been defined yet, create it
    if (!bval->rep) {
        bval->rep = builder.CreateGlobalStringPtr(val->val.c_str());
    }
    lastValue = bval->rep;
}

void LLVMBuilder::emitIntConst(Context &context, const IntConst &val) {
    lastValue = dynamic_cast<const BIntConst &>(val).rep;
}

BranchpointPtr LLVMBuilder::emitIf(Context &context, Expr *cond) {
    // create blocks for the true and false conditions
    BasicBlock *trueBlock = BasicBlock::Create("cond_true", func);
    BBranchpointPtr result = new BBranchpoint(BasicBlock::Create("cond_false",
                                                                 func
                                                                 )
                                              );

    cond->emit(context);
    // XXX I think we need a "conditional" type so we don't have to convert 
    // everything to a boolean and then check for non-zero.
    BTypeDef *boolType =
        BTypeDefPtr::rcast(context.globalData->boolType);
    Value *comparison =
        builder.CreateICmpNE(lastValue, Constant::getNullValue(boolType->rep));
    builder.CreateCondBr(comparison, trueBlock, result->block);
    
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
    bpos->block = BasicBlock::Create("cond_end", func);
    if (!terminal)
        builder.CreateBr(bpos->block);
    
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
    if (!terminal)
        builder.CreateBr(bpos->block);

    // new block is the next block
    builder.SetInsertPoint(block = bpos->block);
}

BranchpointPtr LLVMBuilder::emitBeginWhile(Context &context, 
                                           Expr *cond) {
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
    BTypeDef *boolType =
        BTypeDefPtr::rcast(context.globalData->boolType);
    Value *comparison =
        builder.CreateICmpNE(lastValue, Constant::getNullValue(boolType->rep));
    builder.CreateCondBr(comparison, whileBody, bpos->block);

    // begin generating code in the while body    
    builder.SetInsertPoint(block = whileBody);

    return bpos;
}

void LLVMBuilder::emitEndWhile(Context &context, Branchpoint *pos) {
    BBranchpoint *bpos = BBranchpointPtr::cast(pos);

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
                                      TypeDef *returnType,
                                      const vector<ArgDefPtr> &args) {
    
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
    
    // see if this is a method, if so store the class type as the receiver type
    if (flags & FuncDef::method) {
        ContextPtr classCtx = context.getClassContext();
        assert(classCtx && "method is not nested in a class context.");
        BBuilderContextData *contextData = 
            BBuilderContextDataPtr::rcast(classCtx->builderData);
        f.setReceiverType(contextData->type.get());
    }

    f.finish(false);

    func = f.funcDef->rep;
    block = BasicBlock::Create(name, func);
    builder.SetInsertPoint(block);
    
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

TypeDefPtr LLVMBuilder::emitBeginClass(Context &context,
                                       const string &name,
                                       const vector<TypeDefPtr> &bases) {
    assert(!context.builderData);
    BBuilderContextData *bdata;
    context.builderData = bdata = new BBuilderContextData();
    
    // process the base classes
    for (vector<TypeDefPtr>::const_iterator iter = bases.begin();
         iter != bases.end();
         ++iter
         )
        bdata->addBaseClass(BTypeDefPtr::rcast(*iter));
    
    bdata->type = new BTypeDef(name, PointerType::getUnqual(OpaqueType::get()));
    bdata->type->defaultInitializer = new MallocExpr(bdata->type.get());
    return bdata->type.get();
}

void LLVMBuilder::emitEndClass(Context &context) {
    // build a vector of the base classes and instance variables
    vector<const Type *> members;
    
    // first the base classes
    for (Context::ContextVec::iterator baseIter = context.parents.begin();
         baseIter != context.parents.end();
         ++baseIter
         ) {
        BTypeDef *typeDef = BTypeDefPtr::rcast((*baseIter)->returnType);
        members.push_back(cast<PointerType>(typeDef->rep)->getElementType());
    }
    
    for (Context::VarDefMap::iterator iter = context.beginDefs();
        iter != context.endDefs();
        ++iter
        ) {
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
    BBuilderContextData *bdata =
        BBuilderContextDataPtr::rcast(context.builderData);
        
    PointerType *ptrType =
        cast<PointerType>(const_cast<Type *>(bdata->type->rep));
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type*>(ptrType->getElementType()));
    Type *newType = StructType::get(members);
    curType->refineAbstractTypeTo(newType);

    // fix all instance variable uses that were created before the structure 
    // was defined.
    for (vector<PlaceholderInstruction *>::iterator iter = 
            bdata->placeholders.begin();
         iter != bdata->placeholders.end();
         ++iter
         )
        (*iter)->fix();
    bdata->placeholders.clear();
}

void LLVMBuilder::emitReturn(model::Context &context,
                             model::Expr *expr) {
    
    if (expr) {
        expr->emit(context);
        builder.CreateRet(lastValue);
    } else {
        builder.CreateRetVoid();
    }
}

VarDefPtr LLVMBuilder::emitVarDef(Context &context, TypeDef *type,
                                  const string &name,
                                  Expr *initializer,
                                  bool staticScope
                                  ) {
    // do initializion
    if (initializer)
        initializer->emit(context);
    else
        type->defaultInitializer->emit(context);
    
    // XXX use InternalLinkage for variables starting with _ (I think that 
    // might work)

    // reveal our type object
    BTypeDef *tp = BTypeDefPtr::cast(type);
    
    // get the defintion context
    ContextPtr defCtx = context.getDefContext();
    
    Value *var = 0;
    switch (defCtx->scope) {

        case Context::instance:
            // class statics share the same context as instance variables: 
            // they are distinguished from instance variables by their 
            // declaration and are equivalent to module scoped globals in the 
            // way they are emitted, so if the staticScope flag is set we want 
            // to fall through to module scope
            if (!staticScope) {
                // first, we need to determine the index of the new field.
                BBuilderContextData *bdata =
                    BBuilderContextDataPtr::rcast(
                        defCtx->builderData
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
    return new BStrConst(context.globalData->byteptrType.get(), val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, long val) {
    // XXX probably need to consider the simplest type that the constant can 
    // fit into (compatibility rules will allow us to coerce it into another 
    // type)
    return new BIntConst(context.globalData->int32Type.get(), val);
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(FuncDef *func) {
    return new FuncCall(func);
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

void LLVMBuilder::emitFieldAssign(Context &context,
                                  Expr *aggregate,
                                  VarDef *varDef,
                                  Expr *val
                                  ) {
    aggregate->emit(context);

    // narrow to the field type.
    Context *varContext = varDef->context;
    aggregate->type->emitNarrower(*varContext->returnType);
    Value *aggregateRep = lastValue;
    
    // emit the value last, lastValue after this needs to be the expression so 
    // we can chain assignments.
    val->emit(context);

    unsigned index = BInstVarDefImplPtr::rcast(varDef->impl)->index;
    // if the variable is part of a complete context, just do the store.  
    // Otherwise create a fixup.
    if (varContext->complete) {
        Value *fieldRef = builder.CreateStructGEP(aggregateRep, index);
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
        BBuilderContextData *bdata =
            BBuilderContextDataPtr::rcast(varContext->builderData);
        bdata->placeholders.push_back(placeholder);
    }
}

void LLVMBuilder::emitNarrower(TypeDef &curType, TypeDef &parent, int index) {
    Context *ctx = curType.context.get();
    if (ctx->complete) {
        lastValue = builder.CreateStructGEP(lastValue, index);
    } else {
        // create a placeholder instruction
        Value *aggregate = lastValue;
        BTypeDef &bparent = dynamic_cast<BTypeDef &>(parent);
        PlaceholderInstruction *placeholder =
            new IncompleteNarrower(bparent.rep, aggregate, index, block);
        lastValue = placeholder;

        // store it
        BBuilderContextData *bdata =
            BBuilderContextDataPtr::rcast(ctx->builderData);
        bdata->placeholders.push_back(placeholder);
    }
}

extern "C" void printint(int val) {
    std::cout << val << flush;
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;

    // create the basic types
    
    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef("void", 0);
    context.addDef(voidType);
    
    llvm::Type *llvmBytePtrType = 
        PointerType::getUnqual(llvm::IntegerType::get(8));
    BTypeDef *byteptrType;
    gd->byteptrType = byteptrType = new BTypeDef("byteptr", llvmBytePtrType);
    gd->byteptrType->defaultInitializer = createStrConst(context, "");
    context.addDef(byteptrType);
    
    const llvm::Type *llvmInt32Type = llvm::IntegerType::get(32);
    BTypeDef *int32Type;
    gd->int32Type = int32Type = new BTypeDef("int32", llvmInt32Type);
    gd->int32Type->defaultInitializer = createIntConst(context, 0);
    context.addDef(int32Type);
    
    // XXX using bool = int32 for now
    gd->boolType = int32Type;
    
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
        f.addArg("n", gd->int32Type.get());
        f.finish();
    }
    
    // create "void printint(int32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint", 1);
        f.addArg("val", gd->int32Type.get());
        f.finish();
    }
    
    // create integer operations
    context.addDef(new AddOpDef(gd->int32Type.get()));
    context.addDef(new SubOpDef(gd->int32Type.get()));
    context.addDef(new MulOpDef(gd->int32Type.get()));
    context.addDef(new SDivOpDef(gd->int32Type.get()));
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
