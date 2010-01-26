                
#include "LLVMBuilder.h"

#include <dlfcn.h>

// LLVM includes
#include <stddef.h>
#include <stdlib.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/ModuleProvider.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT

#include <spug/Exception.h>

#include <model/AllocExpr.h>
#include <model/AssignExpr.h>
#include <model/ArgDef.h>
#include <model/Branchpoint.h>
#include <model/BuilderContextData.h>
#include <model/CleanupFrame.h>
#include <model/VarDefImpl.h>
#include <model/Context.h>
#include <model/FuncDef.h>
#include <model/FuncCall.h>
#include <model/InstVarDef.h>
#include <model/IntConst.h>
#include <model/NullConst.h>
#include <model/ResultExpr.h>
#include <model/StrConst.h>
#include <model/StubDef.h>
#include <model/TypeDef.h>
#include <model/VarDef.h>
#include <model/VarRef.h>


using namespace builder;

using namespace std;
using namespace llvm;
using namespace model;
typedef model::FuncCall::ExprVec ExprVec;

namespace {

    // utility function to resize a vector to accomodate a new element, but 
    // only if necessary.
    template<typename T>
    void accomodate(vector<T *> &vec, size_t index) {
        if (vec.size() < index + 1)
            vec.resize(index + 1, 0);
    }


    SPUG_RCPTR(BFuncDef);

    class BFuncDef : public model::FuncDef {
        public:
            // this holds the function object for the last module to request 
            // it.
            llvm::Function *rep;
            
            // for a virtual function, this holds the vtable slot.
            unsigned vtableSlot;

            BFuncDef(FuncDef::Flags flags, const string &name,
                     size_t argCount
                     ) :
                model::FuncDef(flags, name, argCount),
                rep(0),
                vtableSlot(0) {
            }
            
            /**
             * Returns the module-specific Function object for the function.
             */
            Function *getRep(LLVMBuilder &builder) {
                if (rep->getParent() != builder.module)
                    rep = builder.getModFunc(this);
                return rep;
            }
            
    };
        
    SPUG_RCPTR(BTypeDef)

    class BTypeDef : public model::TypeDef {
        public:
            const Type *rep;
            unsigned nextVTableSlot;
            const PointerType *vtableType;
            Constant *vtable;

            BTypeDef(const string &name, const llvm::Type *rep,
                     bool pointer = false,
                     unsigned nextVTableSlot = 0
                     ) :
                model::TypeDef(name, pointer),
                rep(rep),
                nextVTableSlot(nextVTableSlot),
                vtable(0) {
            }

            void populateVTable(vector<const Type *> &vtableTypes,
                                vector<Constant *> &vtableVals
                                ) {
                // populate the beginning of the vtable from the first base 
                // class with a vtable
                // XXX this needs to change for virtual base classes
                for (Context::ContextVec::iterator baseIter = 
                        context->parents.begin();
                     baseIter != context->parents.end();
                     ++baseIter
                     ) {
                    BTypeDef *typeDef =
                        BTypeDefPtr::arcast((*baseIter)->returnType);
                    if (typeDef->hasVTable) {
                        typeDef->populateVTable(vtableTypes, vtableVals);
                        break;
                    }
                }
                
                // find all of the virtual functions
                for (Context::VarDefMap::iterator varIter =
                        context->beginDefs();
                     varIter != context->endDefs();
                     ++varIter
                     ) {
                    BFuncDef *funcDef = BFuncDefPtr::rcast(varIter->second);
                    if (funcDef && (funcDef->flags & FuncDef::virtualized)) {
                        accomodate(vtableTypes, funcDef->vtableSlot);
                        vtableTypes[funcDef->vtableSlot] =
                            funcDef->rep->getType();
                        accomodate(vtableVals, funcDef->vtableSlot);
                        vtableVals[funcDef->vtableSlot] = funcDef->rep;
                    }
                }
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
            BIntConst(BTypeDef *type, long val) :
                IntConst(type, val),
                rep(ConstantInt::get(type->rep, val)) {
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
        public:
            PlaceholderInstruction(const Type *type, BasicBlock *parent) :
                Instruction(type, OtherOpsEnd + 1, 0, 0, parent) {
            }

            PlaceholderInstruction(const Type *type,
                                   Instruction *insertBefore = 0
                                   ) :
                Instruction(type, OtherOpsEnd + 1, 0, 0, insertBefore) {
            }
            
            void *operator new(size_t s) {
                return User::operator new(s, 0);
            }

            /** Replace the placeholder with a real instruction. */
            void fix() {
                IRBuilder<> builder(getParent(), this);
                insertInstructions(builder);
                this->eraseFromParent();
                // ADD NO CODE AFTER SELF-DELETION.
            }
            
            virtual void insertInstructions(IRBuilder<> &builder) = 0;
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
        
    /**
     * A placeholder instruction for operations involving structure fields.
     */
    class FieldInstructionPlaceholder : public PlaceholderInstruction {
        protected:
            Value *aggregate;
            unsigned index;

        public:
            FieldInstructionPlaceholder(const Type *type, Value *aggregate,
                                        unsigned index,
                                        BasicBlock *parent
                                        ) :
                PlaceholderInstruction(type, parent),
                aggregate(aggregate),
                index(index) {
            }

            FieldInstructionPlaceholder(const Type *type, 
                                        Value *aggregate,
                                        unsigned index,
                                        Instruction *insertBefore = 0
                                        ) :
                PlaceholderInstruction(type, insertBefore),
                aggregate(aggregate),
                index(index) {
            }
            
            /** Replace the placeholder with a real instruction. */
            virtual void insertInstructions(IRBuilder<> &builder) {
                Value *fieldPtr = builder.CreateStructGEP(aggregate, index);
                insertFieldInstructions(builder, fieldPtr);
            }
            
            virtual void insertFieldInstructions(IRBuilder<> &builder,
                                                 Value *fieldPtr
                                                 ) = 0;
    };

    /** an incomplete reference to an instance variable. */
    class IncompleteInstVarRef : public FieldInstructionPlaceholder {
        public:

            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 BasicBlock *parent
                                 ) :
                FieldInstructionPlaceholder(type, aggregate, index, parent) {
            }
            
            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 Instruction *insertBefore = 0
                                 ) :
                FieldInstructionPlaceholder(type, aggregate, index, insertBefore) {
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteInstVarRef(getType(), aggregate, index);
            }
            
            virtual void insertFieldInstructions(IRBuilder<> &builder, 
                                                 Value *fieldPtr
                                                 ) {
                replaceAllUsesWith(builder.CreateLoad(fieldPtr));
            }
    };
    
    class IncompleteInstVarAssign : public FieldInstructionPlaceholder {
        private:
            Value *rval;

        public:

            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    BasicBlock *parent
                                    ) :
                FieldInstructionPlaceholder(type, aggregate, index, parent),
                rval(rval) {
            }
            
            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    Instruction *insertBefore = 0
                                    ) :
                FieldInstructionPlaceholder(type, aggregate, index, insertBefore),
                rval(rval) {
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteInstVarAssign(getType(), aggregate, index, 
                                                   rval
                                                   );
            }
            
            virtual void insertFieldInstructions(IRBuilder<> &builder,
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
        private:
            Value *aggregate;
            BTypeDef *startType, *ancestor;

        public:
            IncompleteNarrower(Value *aggregate,
                               BTypeDef *startType,
                               BTypeDef *ancestor,
                               BasicBlock *parent
                               ) :
                PlaceholderInstruction(ancestor->rep, parent),
                aggregate(aggregate),
                startType(startType),
                ancestor(ancestor) {
            }
            
            IncompleteNarrower(Value *aggregate,
                               BTypeDef *startType,
                               BTypeDef *ancestor,
                               Instruction *insertBefore = 0
                               ) :
                PlaceholderInstruction(ancestor->rep, insertBefore),
                aggregate(aggregate),
                startType(startType),
                ancestor(ancestor) {
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteNarrower(aggregate, startType, ancestor);
            }
            
            /**
             * Emits the GEP instructions to narrow 'inst' from 'type' to 
             * 'ancestor'.  Returns the resulting end-value.
             */
            static Value *emitGEP(IRBuilder<> &builder, BTypeDef *type, 
                                  BTypeDef *ancestor,
                                  Value *inst
                                  ) {
                if (type == ancestor)
                    return inst;

                int i = 0;
                for (Context::ContextVec::iterator iter = 
                        type->context->parents.begin();
                     iter != type->context->parents.end();
                     ++iter, ++i
                     )
                    if ((*iter)->returnType->isDerivedFrom(ancestor)) {
                        inst = builder.CreateStructGEP(inst, i);
                        BTypeDef *base = 
                            BTypeDefPtr::arcast((*iter)->returnType);
                        return emitGEP(builder, base, ancestor, inst);
                    }
                assert(false && "narrowing to non-ancestor!");
            }
            
            virtual void insertInstructions(IRBuilder<> &builder) {
                replaceAllUsesWith(emitGEP(builder, startType, ancestor,
                                           aggregate
                                           )
                                   );
            }
    };
    
    class IncompleteVTableInit : public PlaceholderInstruction {
        public:
            // we can make these raw pointers, the type _must_ be in existence 
            // during the lifetime of this object.
            BTypeDef *aggregateType;
            BTypeDef *vtableBaseType;
            Value *aggregate;

            IncompleteVTableInit(BTypeDef *aggregateType, Value *aggregate,
                                 BTypeDef *vtableBaseType,
                                 BasicBlock *parent
                                 ) :
                PlaceholderInstruction(aggregateType->rep, parent),
                aggregate(aggregate),
                aggregateType(aggregateType),
                vtableBaseType(vtableBaseType) {
            }
            
            IncompleteVTableInit(BTypeDef *aggregateType, Value *aggregate,
                                 BTypeDef *vtableBaseType,
                                 Instruction *insertBefore = 0
                                 ) :
                PlaceholderInstruction(aggregateType->rep, insertBefore),
                aggregateType(aggregateType),
                vtableBaseType(vtableBaseType) {
            }
            
            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteVTableInit(aggregateType, aggregate, 
                                                vtableBaseType
                                                );
            }

            void emitVTableInit(IRBuilder<> &builder, BTypeDef *btype,
                                Value *inst
                                ) {
                if (btype == vtableBaseType) {
                    
                    // convert the instance pointer to the address of a 
                    // vtable pointer.
                    Value *vtableFieldRef =
                        builder.CreateBitCast(inst, aggregateType->vtableType);
            
                    // store the vtable pointer in the field.
                    builder.CreateStore(aggregateType->vtable, vtableFieldRef);
                } else {
                    // recurse through all parents with vtables
                    Context::ContextVec &parents = btype->context->parents;
                    int i = 0;
                    for (Context::ContextVec::iterator ctxIter = 
                            parents.begin();
                         ctxIter != parents.end();
                         ++ctxIter, ++i
                         ) {
                        BTypeDef *base = 
                            BTypeDefPtr::arcast((*ctxIter)->returnType);
                        if (base->hasVTable) {
                            Value *baseInst =
                                builder.CreateStructGEP(inst, i);
                            emitVTableInit(builder, base, baseInst);
                        }
                    }
                }
            }
                        
            virtual void insertInstructions(IRBuilder<> &builder) {
                emitVTableInit(builder, aggregateType, aggregate);
            }
    };
    
    class IncompleteVirtualFunc : public PlaceholderInstruction {
        private:
            BTypeDef *vtableBaseType;
            BFuncDef *funcDef;
            BTypeDef *receiverType;
            Value *receiver;
            vector<Value *> args;

            static Value *getVTableReference(IRBuilder<> &builder,
                                             BTypeDef *vtableBaseType,
                                             BTypeDef *aggregateType,
                                             BTypeDef *curType,
                                             Value *inst
                                             ) {
                
                // (the logic here looks painfully like that of 
                // emitVTableInit(), but IMO converting this to an internal 
                // iterator would just make the code harder to grok)

                if (curType == vtableBaseType) {
                    
                    // XXX this is fucked
                    
                    // convert the instance pointer to the address of a 
                    // vtable pointer.
                    return builder.CreateBitCast(inst, 
                                                 aggregateType->vtableType
                                                 );
            
                } else {
                    // recurse through all parents with vtables
                    Context::ContextVec &parents = curType->context->parents;
                    int i = 0;
                    for (Context::ContextVec::iterator ctxIter = 
                            parents.begin();
                         ctxIter != parents.end();
                         ++ctxIter, ++i
                         ) {
                        BTypeDef *base = 
                            BTypeDefPtr::arcast((*ctxIter)->returnType);
                        if (base->hasVTable) {
                            Value *baseInst =
                                builder.CreateStructGEP(inst, i);
                            Value *vtable = 
                                getVTableReference(builder, vtableBaseType,
                                                   aggregateType,
                                                   base,
                                                   baseInst
                                                   );
                            if (vtable)
                                return vtable;
                        }
                    
                    }

                    return 0;
                }
            }

            static Value *innerEmitCall(IRBuilder<> &builder,
                                        BTypeDef *vtableBaseType,
                                        BFuncDef *funcDef, 
                                        BTypeDef *receiverType,
                                        Value *receiver,
                                        const vector<Value *> &args
                                        ) {
                Value *vtable = 
                    getVTableReference(builder, vtableBaseType,
                                       receiverType, 
                                       receiverType, 
                                       receiver
                                       );
                assert(vtable && "virtual function receiver has no vtable");
                
                vtable = builder.CreateLoad(vtable);
                Value *funcFieldRef =
                    builder.CreateStructGEP(vtable, funcDef->vtableSlot);
                Value *funcPtr = builder.CreateLoad(funcFieldRef);
                Value *result = builder.CreateCall(funcPtr, args.begin(), 
                                                   args.end()
                                                   );
            }
        
            IncompleteVirtualFunc(BTypeDef *vtableBaseType, BFuncDef *funcDef,
                                  BTypeDef *receiverType,
                                  Value *receiver,
                                  const vector<Value *> &args,
                                  BasicBlock *parent
                                  ) :
                PlaceholderInstruction(BTypeDefPtr::arcast(funcDef->type)->rep, 
                                       parent
                                       ),
                vtableBaseType(vtableBaseType),
                funcDef(funcDef),
                receiverType(receiverType),
                receiver(receiver),
                args(args) {
            }
                                    
            IncompleteVirtualFunc(BTypeDef *vtableBaseType, BFuncDef *funcDef,
                                  BTypeDef *receiverType,
                                  Value *receiver,
                                  const vector<Value *> &args,
                                  Instruction *insertBefore = 0
                                  ) :
                PlaceholderInstruction(BTypeDefPtr::arcast(funcDef->type)->rep, 
                                       insertBefore
                                       ),
                vtableBaseType(vtableBaseType),
                funcDef(funcDef),
                receiverType(receiverType),
                receiver(receiver),
                args(args) {
            }
        
        public:


            virtual Instruction *clone(LLVMContext &context) const {
                return new IncompleteVirtualFunc(vtableBaseType, funcDef,
                                                 receiverType, 
                                                 receiver,
                                                 args
                                                 );
            }

            virtual void insertInstructions(IRBuilder<> &builder) {
                Value *callInst =
                    innerEmitCall(builder, vtableBaseType, funcDef, 
                                  receiverType, 
                                  receiver, 
                                  args
                                  );
                replaceAllUsesWith(callInst);
            }
            
            static Value *emitCall(Context &context, 
                                   BFuncDef *funcDef, 
                                   Value *receiver,
                                   const vector<Value *> &args
                                   ) {
                // do some conversions that we need to do either way.
                LLVMBuilder &llvmBuilder = 
                    dynamic_cast<LLVMBuilder &>(context.builder);
                BTypeDef *vtableBaseType =
                    BTypeDefPtr::arcast(context.globalData->vtableBaseType);
                Context *classCtx = funcDef->context->getClassContext().get();
                BTypeDef *receiverType =
                    BTypeDefPtr::arcast(classCtx->returnType);

                // if this is for a complete class, go ahead and emit the code.  
                // Otherwise just emit a placeholder.
                if (classCtx->complete) {
                    Value *val = innerEmitCall(llvmBuilder.builder, 
                                         vtableBaseType,
                                         funcDef, 
                                         receiverType, 
                                         receiver,
                                         args
                                         );
                    return val;
                } else {
                    PlaceholderInstruction *placeholder =
                        new IncompleteVirtualFunc(
                            vtableBaseType,
                            funcDef,
                            receiverType,
                            receiver,
                            args,
                            llvmBuilder.block
                        );
                    BBuilderContextData *bdata =
                        BBuilderContextDataPtr::rcast(
                            classCtx->builderData
                        );
                    bdata->placeholders.push_back(placeholder);
                    return placeholder;
                }
            }
    };

    /**
     * Implements a ResultExpr that tracks the result by storing the Value.
     */
    class BResultExpr : public ResultExpr {
        public:
            Value *value;

            BResultExpr(Expr *sourceExpr, Value *value) :
                ResultExpr(sourceExpr),
                value(value) {
            }
            
            ResultExprPtr emit(Context &context) {
                dynamic_cast<LLVMBuilder &>(context.builder).lastValue = 
                    value;
                return this;
            }
    };
    
    SPUG_RCPTR(BCleanupFrame)

    class BCleanupFrame : public CleanupFrame {
        public:
            vector<ExprPtr> cleanups;
    
            BCleanupFrame(Context *context) : CleanupFrame(context) {}
            
            virtual void addCleanup(Expr *cleanup) {
                cleanups.insert(cleanups.begin(), cleanup);
            }
            
            virtual void close() {
                context->emittingCleanups = true;
                for (vector<ExprPtr>::iterator iter = cleanups.begin();
                     iter != cleanups.end();
                     ++iter)
                    (*iter)->emit(*context);
                context->emittingCleanups = false;
            }
    };            
    
    // generates references for memory variables (globals and instance vars)
    SPUG_RCPTR(BMemVarDefImpl);
    class BMemVarDefImpl : public VarDefImpl {
        public:

            virtual ResultExprPtr emitRef(Context &context, VarRef *var) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitMemVarRef(context, getRep(b));
                return new BResultExpr(var, b.lastValue);
            }
            
            virtual ResultExprPtr
            emitAssignment(Context &context, AssignExpr *assign) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                ResultExprPtr result = assign->value->emit(context);
                b.narrow(assign->value->type.get(), assign->var->type.get());
                Value *exprVal = b.lastValue;
                b.builder.CreateStore(exprVal, getRep(b));
                result->handleAssignment(context);
                b.lastValue = exprVal;
                
                return new BResultExpr(assign, exprVal);
            }
            
            virtual Value *getRep(LLVMBuilder &builder) = 0;
    };
    
    class BHeapVarDefImpl : public BMemVarDefImpl {
        public:
            Value *rep;

            BHeapVarDefImpl(Value *rep) : rep(rep) {}
            
            virtual Value *getRep(LLVMBuilder &builder) {
                return rep;
            }
    };
    
    SPUG_RCPTR(BGlobalVarDefImpl);
    class BGlobalVarDefImpl : public BMemVarDefImpl {
        public:
            GlobalVariable *rep;

            BGlobalVarDefImpl(GlobalVariable *rep) : rep(rep) {}

            virtual Value *getRep(LLVMBuilder &builder) {
                if (rep->getParent() != builder.module)
                    rep = builder.getModVar(this);
                return rep;
            }
    };                        
    
    SPUG_RCPTR(BInstVarDefImpl);

    // Impl object for instance variables.  These should never be used to emit 
    // instance variables, so when used they just raise an assertion error.
    class BInstVarDefImpl : public VarDefImpl {
        public:
            unsigned index;
            BInstVarDefImpl(unsigned index) : index(index) {}
            virtual ResultExprPtr emitRef(Context &context,
                                          VarRef *var
                                          ) { 
                assert(false && 
                       "attempting to emit a direct reference to a instance "
                       "variable."
                       );
            }
            
            virtual ResultExprPtr emitAssignment(Context &context,
                                                 AssignExpr *assign
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

            ResultExprPtr emit(Context &context) {
                ResultExprPtr aggregateResult = aggregate->emit(context);
                LLVMBuilder &bb = dynamic_cast<LLVMBuilder &>(context.builder);

                // narrow to the ancestor type where there variable is defined.
                bb.narrow(aggregate->type.get(),
                          def->context->returnType.get()
                          );

                unsigned index = BInstVarDefImplPtr::rcast(def->impl)->index;
                
                // if the variable is from a complete context, we can emit it. 
                //  Otherwise, we need to store a placeholder.
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
                
                // release the aggregate
                aggregateResult->handleTransient(context);
                
                return new BResultExpr(this, bb.lastValue);
            }
    };

    class BArgVarDefImpl : public VarDefImpl {
        public:
            Value *rep;
            
            BArgVarDefImpl(Value *rep) : rep(rep) {}

            virtual ResultExprPtr emitRef(Context &context,
                                          VarRef *var
                                          ) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.emitArgVarRef(context, rep);
                
                return new BResultExpr(var, b.lastValue);
            }
            
            virtual ResultExprPtr
            emitAssignment(Context &context, AssignExpr *assign) {
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
                    returnType->rep ? returnType->rep : 
                                      Type::getVoidTy(getGlobalContext());
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
                
                // store the LLVM function in the table for the module
                builder.setModFunc(funcDef.get(), func);
                
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
            
            ResultExprPtr emit(Context &context) {
                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                BTypeDef *btype = BTypeDefPtr::rcast(type);
                PointerType *tp =
                    cast<PointerType>(const_cast<Type *>(btype->rep));
                builder.lastValue =
                    builder.builder.CreateMalloc(tp->getElementType());

                return new BResultExpr(this, builder.lastValue);
            }
            
            virtual void writeTo(ostream &out) const {
                out << "malloc(" << type->name << ')';
            }
    };
    
    // primitive operations

    SPUG_RCPTR(OpDef);

    class OpDef : public FuncDef {
        public:
            
            OpDef(TypeDef *resultType, FuncDef::Flags flags,
                  const string &name,
                  size_t argCount
                  ) :
                FuncDef(flags, name, argCount) {
                
                type = resultType;
            }
            
            virtual FuncCallPtr createFuncCall() = 0;
    };

    class BinOpDef : public OpDef {
        public:
            BinOpDef(TypeDef *argType,
                     TypeDef *resultType,
                     const string &name) :
                OpDef(resultType, FuncDef::noFlags, name, 2) {

                args[0] = new ArgDef(argType, "lhs");
                args[1] = new ArgDef(argType, "rhs");
            }

            virtual FuncCallPtr createFuncCall() = 0;
    };
    
    /** Unary operator base class. */
    class UnOpDef : public OpDef {
        public:
            UnOpDef(TypeDef *resultType, const string &name) :
                OpDef(resultType, FuncDef::method, name, 0) {
            }
    };
    
    /** Operator to convert simple types to booleans. */
    class BoolOpCall : public FuncCall {
        public:
            BoolOpCall(FuncDef *def) : FuncCall(def) {}
            
            virtual ResultExprPtr emit(Context &context) {
                // emit the receiver
                receiver->emit(context)->handleTransient(context);

                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                builder.lastValue =
                    builder.builder.CreateICmpNE(
                        builder.lastValue,
                        Constant::getNullValue(builder.lastValue->getType())
                    );
                
                return new BResultExpr(this, builder.lastValue);
            }
    };
    
    class BoolOpDef : public UnOpDef {
        public:
            BoolOpDef(TypeDef *resultType, const string &name) :
                UnOpDef(resultType, name) {
            }
            
            virtual FuncCallPtr createFuncCall() {
                return new BoolOpCall(this);
            }
    };
    
    /** Operator to convert any pointer type to void. */
    class VoidPtrOpCall : public FuncCall {
        public:
            VoidPtrOpCall(FuncDef *def) : FuncCall(def) {}
            virtual ResultExprPtr emit(Context &context) {
                // emit the receiver
                receiver->emit(context)->handleTransient(context);

                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                builder.lastValue =
                    builder.builder.CreateBitCast(builder.lastValue,
                                                  builder.llvmVoidPtrType
                                                  );

                return new BResultExpr(this, builder.lastValue);
            }
    };

    class VoidPtrOpDef : public UnOpDef {
        public:
            VoidPtrOpDef(TypeDef *resultType) :
                UnOpDef(resultType, "oper to voidptr") {
            }
            
            virtual FuncCallPtr createFuncCall() {
                return new VoidPtrOpCall(this);
            }
    };
    
#define QUAL_BINOP(prefix, opCode, op)                                      \
    class prefix##OpCall : public FuncCall {                                \
        public:                                                             \
            prefix##OpCall(FuncDef *def) :                                  \
                FuncCall(def) {                                             \
            }                                                               \
                                                                            \
            virtual ResultExprPtr emit(Context &context) {                  \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                                                                            \
                args[0]->emit(context)->handleTransient(context);           \
                Value *lhs = builder.lastValue;                             \
                args[1]->emit(context)->handleTransient(context);           \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(lhs,                     \
                                                   builder.lastValue        \
                                                   );                       \
                                                                            \
                return new BResultExpr(this, builder.lastValue);            \
            }                                                               \
    };                                                                      \
                                                                            \
    class prefix##OpDef : public BinOpDef {                                 \
        public:                                                             \
            prefix##OpDef(TypeDef *argType, TypeDef *resultType = 0) :      \
                BinOpDef(argType, resultType ? resultType : argType,        \
                         "oper " op                                         \
                         ) {                                                \
            }                                                               \
                                                                            \
            virtual FuncCallPtr createFuncCall() {                          \
                return new prefix##OpCall(this);                            \
            }                                                               \
    };

#define BINOP(opCode, op) QUAL_BINOP(opCode, opCode, op)

    BINOP(Add, "+");
    BINOP(Sub, "-");
    BINOP(Mul, "*");
    BINOP(SDiv, "/");

    BINOP(ICmpEQ, "==");
    BINOP(ICmpNE, "!=");
    BINOP(ICmpSGT, ">");
    BINOP(ICmpSLT, "<");
    BINOP(ICmpSGE, ">=");
    BINOP(ICmpSLE, "<=");
    
    QUAL_BINOP(Is, ICmpEQ, "is");

} // anon namespace

void LLVMBuilder::emitFunctionCleanups(Context &context) {
    
    // close all cleanups in this context.
    closeAllCleanups(context);
    
    // recurse up through the parents.
    if (!context.toplevel)
        for (Context::ContextVec::iterator parent = context.parents.begin();
            parent != context.parents.end();
            ++parent
            )
            if ((*parent)->scope == Context::local)
                emitFunctionCleanups(**parent);
}

ExecutionEngine *LLVMBuilder::bindModule(ModuleProvider *mp) {
    if (execEng) {
        execEng->addModuleProvider(mp);
    } else {
        if (rootBuilder) 
            execEng = rootBuilder->bindModule(mp);
        else
            execEng = ExecutionEngine::create(mp);
    }
    
    return execEng;
}

void LLVMBuilder::narrow(TypeDef *curType, TypeDef *ancestor) {
    // quick short-circuit to deal with the trivial case
    if (curType == ancestor)
        return;

    Context *ctx = curType->context.get();
    BTypeDef *bcurType = BTypeDefPtr::acast(curType);
    BTypeDef *bancestor = BTypeDefPtr::acast(ancestor);
    if (ctx->complete) {
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
        BBuilderContextData *bdata =
            BBuilderContextDataPtr::rcast(ctx->builderData);
        bdata->placeholders.push_back(placeholder);
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
            new GlobalVariable(*module, type, false,
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

    // get the LLVM arg list from the receiver and the argument expressions
    vector<Value*> valueArgs;
    
    // if there's a receiver, use it as the first argument.
    Value *receiver;
    if (funcCall->receiver) {
        funcCall->receiver->emit(context)->handleTransient(context);
        narrow(funcCall->receiver->type.get(), 
               funcCall->func->context->returnType.get()
               );
        valueArgs.push_back(lastValue);
        receiver = lastValue;
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
    
    BFuncDef *funcDef = BFuncDefPtr::arcast(funcCall->func);
    if (funcDef->flags & FuncDef::virtualized)
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
        bval->rep = builder.CreateGlobalStringPtr(val->val.c_str());
    }
    lastValue = bval->rep;
    return new BResultExpr(val, lastValue);
}

ResultExprPtr LLVMBuilder::emitIntConst(Context &context, IntConst *val) {
    lastValue = dynamic_cast<const BIntConst *>(val)->rep;
    return new BResultExpr(val, lastValue);
}

ResultExprPtr LLVMBuilder::emitNull(Context &context,
                                    NullConst *nullExpr
                                    ) {
    BTypeDef *btype = BTypeDefPtr::arcast(nullExpr->type);
    lastValue = Constant::getNullValue(btype->rep);
    
    return new BResultExpr(nullExpr, lastValue);
}

ResultExprPtr LLVMBuilder::emitAlloc(Context &context, AllocExpr *allocExpr) {
    // XXX need to be able to do this for an incomplete class when we 
    // allow user defined oper new.
    BTypeDef *btype = BTypeDefPtr::arcast(allocExpr->type);
    PointerType *tp =
        cast<PointerType>(const_cast<Type *>(btype->rep));
    lastValue = builder.CreateMalloc(tp->getElementType());
    
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
    // create blocks for the true and false conditions
    LLVMContext &lctx = getGlobalContext();
    BasicBlock *trueBlock = BasicBlock::Create(lctx, "cond_true", func);
    BBranchpointPtr result = new BBranchpoint(BasicBlock::Create(lctx,
                                                                 "cond_false",
                                                                 func
                                                                 )
                                              );

    cond->emitCond(context);
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
    bpos->block = BasicBlock::Create(getGlobalContext(), "cond_end", func);
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
    LLVMContext &lctx = getGlobalContext();
    BBranchpointPtr bpos = new BBranchpoint(BasicBlock::Create(lctx,
                                                               "while_end", 
                                                               func
                                                               )
                                            );

    BasicBlock *whileCond = bpos->block2 =
        BasicBlock::Create(lctx, "while_cond", func);
    BasicBlock *whileBody = BasicBlock::Create(lctx, "while_body", func);
    builder.CreateBr(whileCond);
    builder.SetInsertPoint(block = whileCond);

    // XXX see notes above on a conditional type.
    cond->emitCond(context);
    builder.CreateCondBr(lastValue, whileBody, bpos->block);

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
    
    // see if this is a method, if so store the class type as the receiver type
    unsigned vtableSlot = 0;
    if (flags & FuncDef::method) {
        ContextPtr classCtx = context.getClassContext();
        assert(classCtx && "method is not nested in a class context.");
        BBuilderContextData *contextData = 
            BBuilderContextDataPtr::rcast(classCtx->builderData);
        BTypeDef *classType = contextData->type.get();
        f.setReceiverType(classType);

        // create the vtable slot for a virtual function
        if (flags & FuncDef::virtualized)
            // use the original's slot if this is an override.
            if (override)
                vtableSlot = BFuncDefPtr::cast(override)->vtableSlot;
            else
                vtableSlot = classType->nextVTableSlot++;
    }

    f.finish(false);

    f.funcDef->vtableSlot = vtableSlot;
    func = f.funcDef->rep;
    block = BasicBlock::Create(getGlobalContext(), name, func);
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

TypeDefPtr LLVMBuilder::emitBeginClass(Context &context,
                                       const string &name,
                                       const vector<TypeDefPtr> &bases) {
    assert(!context.builderData);
    BBuilderContextData *bdata;
    context.builderData = bdata = new BBuilderContextData();
    
    // process the base classes, get the first base class with a vtable
    BTypeDef *baseWithVTable = 0;
    for (vector<TypeDefPtr>::const_iterator iter = bases.begin();
         iter != bases.end();
         ++iter
         ) {
        BTypeDef *base = BTypeDefPtr::rcast(*iter);
        bdata->addBaseClass(base);
        if (!baseWithVTable && base->hasVTable)
            baseWithVTable = base;
    }
    
    OpaqueType *opaque = OpaqueType::get(getGlobalContext());
    bdata->type = new BTypeDef(name, PointerType::getUnqual(opaque), true,
                               baseWithVTable ? 
                                baseWithVTable->nextVTableSlot : 0
                               );
    bdata->type->defaultInitializer = new MallocExpr(bdata->type.get());
    
    // create function to convert to voidptr
    context.addDef(new VoidPtrOpDef(context.globalData->voidPtrType.get()));
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
        BTypeDef *typeDef = BTypeDefPtr::arcast((*baseIter)->returnType);
        members.push_back(cast<PointerType>(typeDef->rep)->getElementType());
    }
    
    for (Context::VarDefMap::iterator iter = context.beginDefs();
        iter != context.endDefs();
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
    BBuilderContextData *bdata =
        BBuilderContextDataPtr::rcast(context.builderData);
    PointerType *ptrType =
        cast<PointerType>(const_cast<Type *>(bdata->type->rep));
    DerivedType *curType = 
        cast<DerivedType>(const_cast<Type*>(ptrType->getElementType()));
    
    // create the actual type
    Type *newType = StructType::get(getGlobalContext(), members);
    
    // refine the type and store the new pointer type (the existing pointer 
    // to opaque type may not end up getting changed)
    curType->refineAbstractTypeTo(newType);
    bdata->type->rep = PointerType::getUnqual(newType);

    // construct the vtable if necessary
    if (bdata->type->hasVTable) {
        vector<const Type *> vtableTypes;
        vector<Constant *> vtableVals;
        bdata->type->populateVTable(vtableTypes, vtableVals);
        
        // create a constant structure that actually is the vtable
        const StructType *vtableStructType =
            StructType::get(getGlobalContext(), vtableTypes);
        bdata->type->vtable =
            new GlobalVariable(*module, vtableStructType, true, // isConstant
                               GlobalValue::ExternalLinkage,
                               
                               // initializer - this needs to be 
                               // provided or the global will be 
                               // treated as an extern.
                               ConstantStruct::get(vtableStructType, 
                                                   vtableVals
                                                   ),
                               ".vtable." + bdata->type->name
                               );
        
        // for pragmatic reasons, the "vtable type" is a pointer to a pointer 
        // to the vtable struct type.
        bdata->type->vtableType =
            PointerType::getUnqual(PointerType::getUnqual(vtableStructType));
    }

    // fix-up all of the placeholder instructions
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
    
    // XXX need to emit all cleanups up to the function level.
    if (expr) {
        ResultExprPtr resultExpr = expr->emit(context);
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
        if (initializer) {
            initializer->emit(context)->handleAssignment(context);
            narrow(initializer->type.get(), type);
        } else {
            // assuming that we don't need to narrow a default initializer.
            type->defaultInitializer->emit(context)->handleAssignment(context);
        }
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
                BBuilderContextData *bdata =
                    BBuilderContextDataPtr::rcast(
                        defCtx->builderData
                    );
                unsigned idx = bdata->fieldCount++;
                
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
 
void LLVMBuilder::createModule(Context &context, const std::string &name) {
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
    BTypeDef *voidType = BTypeDefPtr::arcast(context.globalData->int32Type);
    BTypeDef *byteptrType = BTypeDefPtr::arcast(context.globalData->byteptrType);

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
    
    // bind the module to the execution engine
    bindModule(new ExistingModuleProvider(module));
}

void LLVMBuilder::closeModule() {
    assert(module);
    builder.CreateRetVoid();
    verifyModule(*module, llvm::PrintMessageAction);
    
    // bind the module to the execution engine
    bindModule(new ExistingModuleProvider(module));
    
    // store primitive functions
    for (map<Function *, void *>::iterator iter = primFuncs.begin();
         iter != primFuncs.end();
         ++iter
         )
        execEng->addGlobalMapping(iter->first, iter->second);

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

CleanupFramePtr LLVMBuilder::createCleanupFrame(Context &context) {
    return new BCleanupFrame(&context);
}

void LLVMBuilder::closeAllCleanups(model::Context &context) {
    BCleanupFrame* frame = BCleanupFramePtr::rcast(context.cleanupFrame);
    while (frame) {
        frame->close();
        frame = BCleanupFramePtr::rcast(frame->parent);
    }
}

model::StrConstPtr LLVMBuilder::createStrConst(model::Context &context,
                                               const std::string &val) {
    return new BStrConst(context.globalData->byteptrType.get(), val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, long val) {
    // XXX probably need to consider the simplest type that the constant can 
    // fit into (compatibility rules will allow us to coerce it into another 
    // type)
    return new BIntConst(BTypeDefPtr::arcast(context.globalData->int32Type), 
                         val
                         );
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(FuncDef *func) {
    // try to create a BinCmp
    OpDef *specialOp = OpDefPtr::cast(func);
    if (specialOp) {
        // if this is a bin op, let it create the call
        return specialOp->createFuncCall();
    } else {
        // normal function call
        return new FuncCall(func);
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
                                           AssignExpr *assign
                                           ) {
    assign->aggregate->emit(context);

    // narrow to the field type.
    Context *varContext = assign->var->context;
    narrow(assign->aggregate->type.get(), varContext->returnType.get());
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

    return new BResultExpr(assign, lastValue);
}

extern "C" void printint(int val) {
    std::cout << val << flush;
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;
    LLVMContext &lctx = getGlobalContext();

    // create the basic types
    
    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef("void", Type::getVoidTy(lctx));
    context.addDef(voidType);
    
    BTypeDef *voidPtrType;
    llvmVoidPtrType = 
        PointerType::getUnqual(OpaqueType::get(getGlobalContext()));
    gd->voidPtrType = voidPtrType = new BTypeDef("voidptr", llvmVoidPtrType);
    voidPtrType->context = new Context(*this, Context::instance, gd);
    context.addDef(voidPtrType);
    
    llvm::Type *llvmBytePtrType = 
        PointerType::getUnqual(Type::getInt8Ty(lctx));
    BTypeDef *byteptrType;
    gd->byteptrType = byteptrType = new BTypeDef("byteptr", llvmBytePtrType);
    byteptrType->defaultInitializer = createStrConst(context, "");
    byteptrType->context = new Context(*this, Context::instance, gd);
    byteptrType->context->returnType = byteptrType;
    byteptrType->context->addDef(
        new VoidPtrOpDef(context.globalData->voidPtrType.get())
    );
    context.addDef(byteptrType);
    
    const Type *llvmBoolType = IntegerType::getInt1Ty(lctx);
    BTypeDef *boolType;
    gd->boolType = boolType = new BTypeDef("bool", llvmBoolType);
    gd->boolType->defaultInitializer = new BIntConst(boolType, 0);
    boolType->context = new Context(*this, Context::instance, gd);
    boolType->context->returnType = boolType;
    context.addDef(boolType);
    
    const llvm::Type *llvmInt32Type = Type::getInt32Ty(lctx);
    BTypeDef *int32Type;
    gd->int32Type = int32Type = new BTypeDef("int32", llvmInt32Type);
    gd->int32Type->defaultInitializer = createIntConst(context, 0);
    int32Type->context = new Context(*this, Context::instance, gd);
    int32Type->context->returnType = int32Type;
    int32Type->context->addDef(new BoolOpDef(boolType, "toBool"));
    context.addDef(int32Type);

    // create an empty structure type and its pointer for VTableBase 
    // Actual type is {}** (another layer of pointer indirection) because 
    // classes need to be pointer types.
    vector<const Type *> members;
    Type *vtableType = StructType::get(getGlobalContext(), members);
    Type *vtablePtrType = PointerType::getUnqual(vtableType);
    BTypeDef *vtableBaseType;
    gd->vtableBaseType = vtableBaseType =
        new BTypeDef("VTableBase", PointerType::getUnqual(vtablePtrType), 
                     true
                     );
    vtableBaseType->hasVTable = true;
    vtableBaseType->context = new Context(*this, Context::instance, gd);
    vtableBaseType->context->returnType = vtableBaseType;
    context.addDef(vtableBaseType);
    
    // create integer operations
    context.addDef(new AddOpDef(int32Type));
    context.addDef(new SubOpDef(int32Type));
    context.addDef(new MulOpDef(int32Type));
    context.addDef(new SDivOpDef(int32Type));
    context.addDef(new ICmpEQOpDef(int32Type, boolType));
    context.addDef(new ICmpNEOpDef(int32Type, boolType));
    context.addDef(new ICmpSGTOpDef(int32Type, boolType));
    context.addDef(new ICmpSLTOpDef(int32Type, boolType));
    context.addDef(new ICmpSGEOpDef(int32Type, boolType));
    context.addDef(new ICmpSLEOpDef(int32Type, boolType));
    
    // pointer equality check (to allow checking for None)
    context.addDef(new IsOpDef(voidPtrType, boolType));
    context.addDef(new IsOpDef(byteptrType, boolType));
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
        context.addDef(new StubDef(context.globalData->voidType.get(), *iter,
                                   sym
                                   )
                       );
    }
}

void LLVMBuilder::registerImport(Context &context, VarDef *varDef) {
    // no-op for LLVM builder.
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
    BBuilderContextData *bdata =
        BBuilderContextDataPtr::rcast(context.getClassContext()->builderData);
    bdata->placeholders.push_back(vtableInit);
}
