// Copyright 2009 Google Inc.
                
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
#include <model/ModuleDef.h>
#include <model/NullConst.h>
#include <model/OverloadDef.h>
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

// for reasons I don't understand, we have to specialize the OperandTraits 
// templates in the llvm namespace.
namespace {
    class IncompleteInstVarRef;
    class IncompleteInstVarAssign;
    class IncompleteNarrower;
    class IncompleteVTableInit;
    class IncompleteSpecialize;
    class IncompleteVirtualFunc;
};

namespace llvm {
    template<>
    struct OperandTraits<IncompleteInstVarRef> :
        FixedNumOperandTraits<1> {
    };
    template<>
    struct OperandTraits<IncompleteInstVarAssign> :
        FixedNumOperandTraits<2> {
    };
    template<>
    struct OperandTraits<IncompleteNarrower> :
        FixedNumOperandTraits<1> {
    };
    template<>
    struct OperandTraits<IncompleteVTableInit> :
        FixedNumOperandTraits<1> {
    };
    template<>
    struct OperandTraits<IncompleteSpecialize> :
        FixedNumOperandTraits<1> {
    };
    template<>
    struct OperandTraits<IncompleteVirtualFunc> :
        VariadicOperandTraits<1> {
    };
}


namespace {

    // utility function to resize a vector to accomodate a new element, but 
    // only if necessary.
    template<typename T>
    void accomodate(vector<T *> &vec, size_t index) {
        if (vec.size() < index + 1)
            vec.resize(index + 1, 0);
    }

    
    SPUG_RCPTR(BTypeDef);

    SPUG_RCPTR(BFuncDef);

    class BFuncDef : public model::FuncDef {
        public:
            // this holds the function object for the last module to request 
            // it.
            llvm::Function *rep;
            
            // for a virtual function, these are the path to the base class 
            // where the vtable is defined and the vtable slot position.
            TypeDef::AncestorPath pathToFirstDeclaration;
            unsigned vtableSlot;
            
            // for a virtual function, this holds the ancestor class that owns 
            // the vtable pointer
            BTypeDefPtr vtableBase;

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
            
            /**
             * Returns the "receiver type."  For a non-virtual function, this 
             * is simply the type that the function was declared in.  For a 
             * virtual function, it is the type of the base class in which the 
             * function was first declared.
             */
            BTypeDef *getReceiverType() const {
                TypeDef *result;
                if (pathToFirstDeclaration.size())
                    result = pathToFirstDeclaration.back().ancestor.get();
                else
                    result = context->returnType.get();
                return BTypeDefPtr::acast(result);
            }
            
            /**
             * Returns the "this" type of the function.  This is always either 
             * the receiver type or a specialization of it.
             */
            BTypeDef *getThisType() const {
                return BTypeDefPtr::arcast(context->returnType);
            }
            
    };
    
    SPUG_RCPTR(VTableInfo);
    
    // stores information on a single VTable
    class VTableInfo : public spug::RCBase {
        private:
            // give an error if we try to copy this
            VTableInfo(const VTableInfo &other);

        public:
            // the vtable variable name
            string name;
            
            // the vtable entries
            vector<Constant *> entries;
            
            VTableInfo(const string &name) : name(name) {}
    };

    // encapsulates all of the vtables for a new type
    class VTableBuilder {
        private:
            typedef map<BTypeDef *, VTableInfoPtr> VTableMap;
            VTableMap vtables;
            
            // keep track of the first VTable in the type
            VTableInfo *firstVTable;

        public:
            // add a new function entry to the appropriate VTable
            void add(BFuncDef *func) {
                
                // find the ancestor whose vtable this function needs to go 
                // into
                BTypeDef *ancestor;
                TypeDef::AncestorPath &path = func->pathToFirstDeclaration;
                if (path.size())
                    ancestor = BTypeDefPtr::arcast(path.back().ancestor);
                else
                    ancestor = BTypeDefPtr::arcast(func->context->returnType);
                
                // lookup the vtable
                VTableMap::iterator iter = vtables.find(ancestor);

                // if we didn't find a vtable in the ancestors, append the 
                // function to the first vtable
                VTableInfo *targetVTable;
                if (iter == vtables.end()) {
                    assert(firstVTable && "no first vtable");
                    targetVTable = firstVTable;
                } else {
                    targetVTable = iter->second.get();
                }
                
                // insert the function
                vector<Constant *> &entries = targetVTable->entries;
                accomodate(entries, func->vtableSlot);
                entries[func->vtableSlot] = func->rep;
            }
            
            // create a new VTable
            void createVTable(BTypeDef *type, const std::string &name, 
                              bool first
                              ) {
                assert(vtables.find(type) == vtables.end());
                VTableInfo *info;
                vtables[type] = info = new VTableInfo(name);
                if (first)
                    firstVTable = info;
            }

            // emit all of the VTable globals            
            void emit(Module *module, BTypeDef *type);
    };

    // (RCPTR defined above)    
    class BTypeDef : public model::TypeDef {
        public:
            const Type *rep;
            unsigned nextVTableSlot;

            // mapping from base types to their vtables.            
            map<BTypeDef *, Constant *> vtables;
            const Type *firstVTableType;

            BTypeDef(const string &name, const llvm::Type *rep,
                     bool pointer = false,
                     unsigned nextVTableSlot = 0
                     ) :
                model::TypeDef(name, pointer),
                rep(rep),
                nextVTableSlot(nextVTableSlot),
                firstVTableType(0) {
            }

        // add all of my virtual functions to 'vtb'
        void extendVTables(VTableBuilder &vtb) {
            // find all of the virtual functions
            for (Context::VarDefMap::iterator varIter = context->beginDefs();
                 varIter != context->endDefs();
                 ++varIter
                 ) {
                    
                BFuncDef *funcDef = BFuncDefPtr::rcast(varIter->second);
                if (funcDef && (funcDef->flags & FuncDef::virtualized)) {
                    vtb.add(funcDef);
                    continue;
                }
                
                // check for an overload (if it's not an overload, assume that 
                // it's not a function).  Iterate over all of the overloads up 
                // until the first parent class - the parent classes have 
                // already had their shot at extendVTables, and we don't want 
                // their overloads to clobber ours.
                OverloadDef *overload =
                    OverloadDefPtr::rcast(varIter->second);
                if (overload)
                    for (OverloadDef::FuncList::iterator fiter =
                            overload->funcs.begin();
                         fiter != overload->startOfParents;
                         ++fiter
                         )
                        if ((*fiter)->flags & FuncDef::virtualized)
                            vtb.add(BFuncDefPtr::arcast(*fiter));
            }
        }

        /**
         * Create all of the vtables for 'type'.
         * 
         * @param vtb the vtable builder
         * @param name the name stem for the VTable global variables.
         * @param vtableBaseType the global vtable base type.
         * @param firstVTable if true, we have not yet discovered the first 
         *  vtable in the class schema.
         */
        void createAllVTables(VTableBuilder &vtb, const string &name,
                              BTypeDef *vtableBaseType,
                              bool firstVTable = true
                              ) {
            for (Context::ContextVec::iterator baseIter = 
                    context->parents.begin();
                 baseIter != context->parents.end();
                 ++baseIter
                 ) {
                BTypeDef *base = BTypeDefPtr::arcast((*baseIter)->returnType);
                
                // if the base class is VTableBase, we've hit bottom - 
                // construct the initial vtable and store the first vtable 
                // type if this is it.
                if (base == vtableBaseType) {
                    vtb.createVTable(this, name, firstVTable);

                // otherwise, if the base has a vtable, create all of its 
                // vtables
                } else if (base->hasVTable) {
                    if (firstVTable)
                        base->createAllVTables(vtb, name, vtableBaseType,
                                               firstVTable
                                               );
                    else
                        base->createAllVTables(vtb,
                                               name + ':' + base->getFullName(),
                                               vtableBaseType,
                                               firstVTable
                                               );
                }

                firstVTable = false;
            }
            
            // add my functions to their vtables
            extendVTables(vtb);
        }
    };
    
    SPUG_RCPTR(BModuleDef);
    
    class BModuleDef : public ModuleDef {
        
        public:
            // primitive cleanup function
            void (*cleanup)();
            
            BModuleDef(const string &canonicalName, Context *context) :
                ModuleDef(canonicalName, context),
                cleanup(0) {
            }
            
            void callDestructor() {
                if (cleanup)
                    cleanup();
            }
    };

    // have to define this after BTypeDef because it uses BTypeDef.
    void VTableBuilder::emit(Module *module, BTypeDef *type) {
        for (VTableMap::iterator iter = vtables.begin();
             iter != vtables.end();
             ++iter
             ) {
            // populate the types array
            vector<Constant *> &entries = iter->second->entries;
            vector<const Type *> vtableTypes(entries.size());
            int i = 0;
            for (vector<Constant *>::iterator entryIter =
                    entries.begin();
                entryIter != entries.end();
                ++entryIter, ++i
                )
                vtableTypes[i] = (*entryIter)->getType();

            // create a constant structure that actually is the vtable
            const StructType *vtableStructType =
                StructType::get(getGlobalContext(), vtableTypes);
            type->vtables[iter->first] =
                new GlobalVariable(*module, vtableStructType,
                                   true, // isConstant
                                   GlobalValue::ExternalLinkage,
                                   
                                   // initializer - this needs to be 
                                   // provided or the global will be 
                                   // treated as an extern.
                                   ConstantStruct::get(
                                       vtableStructType, 
                                       iter->second->entries
                                   ),
                                   iter->second->name
                                   );
            
            // store the first VTable pointer (a pointer-to-pointer to the 
            // struct type, actually, because that is what we need to cast our 
            // VTableBase instances to)
            if (iter->second == firstVTable)
                type->firstVTableType = 
                    PointerType::getUnqual(
                        PointerType::getUnqual(vtableStructType)
                    );
        }

        assert(type->firstVTableType);
    }
    
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
            PlaceholderInstruction(const Type *type, BasicBlock *parent,
                                   Use *ops = 0, 
                                   unsigned opCount = 0
                                   ) :
                Instruction(type, OtherOpsEnd + 1, ops, opCount, parent) {
            }

            PlaceholderInstruction(const Type *type,
                                   Instruction *insertBefore = 0,
                                   Use *ops = 0,
                                   unsigned opCount = 0
                                   ) :
                Instruction(type, OtherOpsEnd + 1, ops, opCount, 
                            insertBefore
                            ) {
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
            bool complete;
            
            BBuilderContextData() :
                func(0),
                block(0),
                fieldCount(0),
                complete(false) {
            }
            
            void addBaseClass(const BTypeDefPtr &base) {
                ++fieldCount;
            }
            
            void addPlaceholder(PlaceholderInstruction *inst) {
                assert(!complete && "Adding placeholder to a completed class");
                placeholders.push_back(inst);
            }
    };
    
    /** an incomplete reference to an instance variable. */
    class IncompleteInstVarRef : public PlaceholderInstruction {
        private:
            unsigned index;

        public:
            // allocate space for 1 operand
            void *operator new(size_t s) {
                return User::operator new(s, 1);
            }

            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 BasicBlock *parent
                                 ) :
                PlaceholderInstruction(
                    type,
                    parent,
                    OperandTraits<IncompleteInstVarRef>::op_begin(this),
                    OperandTraits<IncompleteInstVarRef>::operands(this)
                ),
                index(index) {
                Op<0>() = aggregate;
            }
            
            IncompleteInstVarRef(const Type *type, Value *aggregate,
                                 unsigned index,
                                 Instruction *insertBefore = 0
                                 ) :
                PlaceholderInstruction(
                    type,
                    insertBefore,
                    OperandTraits<IncompleteInstVarRef>::op_begin(this),
                    OperandTraits<IncompleteInstVarRef>::operands(this)
                ),
                index(index) {
                Op<0>() = aggregate;
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteInstVarRef(getType(), Op<0>(), index);
            }
            
            virtual void insertInstructions(IRBuilder<> &builder) {
                Value *fieldPtr = builder.CreateStructGEP(Op<0>(), index);
                replaceAllUsesWith(builder.CreateLoad(fieldPtr));
            }
    };

    class IncompleteInstVarAssign : public PlaceholderInstruction {
        private:
            unsigned index;

        public:
            // allocate space for 2 operands
            void *operator new(size_t s) {
                return User::operator new(s, 2);
            }

            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    BasicBlock *parent
                                    ) :
                PlaceholderInstruction(
                    type,
                    parent,
                    OperandTraits<IncompleteInstVarAssign>::op_begin(this),
                    OperandTraits<IncompleteInstVarAssign>::operands(this)
                ),
                index(index) {
                Op<0>() = aggregate;
                Op<1>() = rval;
            }
            
            IncompleteInstVarAssign(const Type *type, Value *aggregate,
                                    unsigned index,
                                    Value *rval,
                                    Instruction *insertBefore = 0
                                    ) :
                PlaceholderInstruction(
                    type,
                    insertBefore,
                    OperandTraits<IncompleteInstVarAssign>::op_begin(this),
                    OperandTraits<IncompleteInstVarAssign>::operands(this)
                ),
                index(index) {
                Op<0>() = aggregate;
                Op<1>() = rval;
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteInstVarAssign(getType(), Op<0>(), index, 
                                                   Op<1>()
                                                   );
            }
            
            virtual void insertInstructions(IRBuilder<> &builder) {
                Value *fieldPtr = builder.CreateStructGEP(Op<0>(), index);
                builder.CreateStore(Op<1>(), fieldPtr);
            }
    };

    /**
     * A placeholder for a "narrower" - a GEP instruction that provides
     * pointer to a base class from a derived class.
     */
    class IncompleteNarrower : public PlaceholderInstruction {
        private:
            BTypeDef *startType, *ancestor;

        public:
            // allocate space for 1 operand
            void *operator new(size_t s) {
                return User::operator new(s, 1);
            }

            IncompleteNarrower(Value *aggregate,
                               BTypeDef *startType,
                               BTypeDef *ancestor,
                               BasicBlock *parent
                               ) :
                PlaceholderInstruction(
                    ancestor->rep,
                    parent,
                    OperandTraits<IncompleteNarrower>::op_begin(this),
                    OperandTraits<IncompleteNarrower>::operands(this)
                ),
                startType(startType),
                ancestor(ancestor) {
                Op<0>() = aggregate;
            }
            
            IncompleteNarrower(Value *aggregate,
                               BTypeDef *startType,
                               BTypeDef *ancestor,
                               Instruction *insertBefore = 0
                               ) :
                PlaceholderInstruction(
                    ancestor->rep,
                    insertBefore,
                    OperandTraits<IncompleteNarrower>::op_begin(this),
                    OperandTraits<IncompleteNarrower>::operands(this)
                ),
                startType(startType),
                ancestor(ancestor) {
                Op<0>() = aggregate;
            }

            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteNarrower(Op<0>(), startType, ancestor);
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
                                           Op<0>()
                                           )
                                   );
            }
    };

    class IncompleteVTableInit : public PlaceholderInstruction {
        public:
            // allocate space for 1 operand
            void *operator new(size_t s) {
                return User::operator new(s, 1);
            }

            // we can make these raw pointers, the type _must_ be in existence 
            // during the lifetime of this object.
            BTypeDef *aggregateType;
            BTypeDef *vtableBaseType;

            IncompleteVTableInit(BTypeDef *aggregateType, Value *aggregate,
                                 BTypeDef *vtableBaseType,
                                 BasicBlock *parent
                                 ) :
                PlaceholderInstruction(
                    aggregateType->rep,
                    parent,
                    OperandTraits<IncompleteVTableInit>::op_begin(this),
                    OperandTraits<IncompleteVTableInit>::operands(this)
                ),
                aggregateType(aggregateType),
                vtableBaseType(vtableBaseType) {
                Op<0>() = aggregate;
            }
            
            IncompleteVTableInit(BTypeDef *aggregateType, Value *aggregate,
                                 BTypeDef *vtableBaseType,
                                 Instruction *insertBefore = 0
                                 ) :
                PlaceholderInstruction(
                    aggregateType->rep,
                    insertBefore,
                    OperandTraits<IncompleteVTableInit>::op_begin(this),
                    OperandTraits<IncompleteVTableInit>::operands(this)
                ),
                aggregateType(aggregateType),
                vtableBaseType(vtableBaseType) {
                Op<0>() = aggregate;
            }
            
            virtual Instruction *clone(LLVMContext &lctx) const {
                return new IncompleteVTableInit(aggregateType, Op<0>(), 
                                                vtableBaseType
                                                );
            }

            // emit the code to initialize the first VTable in btype.
            void emitInitOfFirstVTable(IRBuilder<> &builder, 
                                       BTypeDef *btype,
                                       Value *inst,
                                       Constant *vtable
                                       ) {
                
                Context::ContextVec &parents = btype->context->parents;
                int i = 0;
                for (Context::ContextVec::iterator ctxIter = 
                        parents.begin();
                     ctxIter != parents.end();
                     ++ctxIter, ++i
                     ) {
                    BTypeDef *base = 
                        BTypeDefPtr::arcast((*ctxIter)->returnType);
                    if (base == vtableBaseType) {
                        inst = builder.CreateStructGEP(inst, i);

                        // convert the vtable to {}*
                        const PointerType *emptyStructPtrType =
                            cast<PointerType>(vtableBaseType->rep);
                        const Type *emptyStructType = 
                            emptyStructPtrType->getElementType();
                        Value *castVTable =
                            builder.CreateBitCast(vtable, emptyStructType);
                
                        // store the vtable pointer in the field.
                        builder.CreateStore(castVTable, inst);
                        return;
                    }
                }
                
                assert(false && "no vtable base class");
            }

            // emit the code to initialize all vtables in an object.
            void emitVTableInit(IRBuilder<> &builder, BTypeDef *btype,
                                Value *inst
                                ) {

                // if btype has a registered vtable, startClass gets 
                // incremented so that we don't emit a parent vtable that
                // overwrites it.
                int startClass = 0;

                // check for the vtable of the current class
                map<BTypeDef *, Constant *>::iterator firstVTableIter =
                    aggregateType->vtables.find(btype);
                if (firstVTableIter != aggregateType->vtables.end()) {
                    emitInitOfFirstVTable(builder, btype, inst,
                                          firstVTableIter->second
                                          );
                    startClass = 1;
                }

                // recurse through all other parents with vtables
                Context::ContextVec &parents = btype->context->parents;
                int i = 0;
                for (Context::ContextVec::iterator ctxIter = 
                        parents.begin() + startClass;
                     ctxIter != parents.end();
                     ++ctxIter, ++i
                     ) {
                    BTypeDef *base = 
                        BTypeDefPtr::arcast((*ctxIter)->returnType);
                    
                    // see if this class has a vtable in the aggregate type
                    map<BTypeDef *, Constant *>::iterator vtableIter =
                        aggregateType->vtables.find(base);
                    if (vtableIter != aggregateType->vtables.end()) {
                        Value *baseInst =
                            builder.CreateStructGEP(inst, i);
                        emitInitOfFirstVTable(builder, base, baseInst,
                                              vtableIter->second
                                              );
                    } else if (base->hasVTable) {
                        Value *baseInst =
                            builder.CreateStructGEP(inst, i);
                        emitVTableInit(builder, base, baseInst);
                    }
                }
            }
                        
            virtual void insertInstructions(IRBuilder<> &builder) {
                emitVTableInit(builder, aggregateType, Op<0>());
            }
    };

    Value *narrowToAncestor(IRBuilder<> &builder, Value *receiver,
                            const TypeDef::AncestorPath &path
                            ) {
        for (TypeDef::AncestorPath::const_iterator iter = path.begin();
             iter != path.end();
             ++iter
             )
            receiver =
                builder.CreateStructGEP(receiver, iter->index);
        
        return receiver;
    }

    
    class IncompleteVirtualFunc : public PlaceholderInstruction {
        private:
            BTypeDef *vtableBaseType;
            BFuncDef *funcDef;

            /**
             * Returns the first vtable pointer in the instance layout, casted 
             * to finalVTableType.
             * 
             * @param builder the builder in which to generate the GEPs
             * @param vtableBaseType the type object for the global VTableBase 
             *        class
             * @param finalVTableType the type that we need to cast the 
             *        VTable to.
             * @param curType the current type of 'inst'
             * @param inst the instance that we are retrieving the vtable for.
             */
            static Value *getVTableReference(IRBuilder<> &builder,
                                             BTypeDef *vtableBaseType,
                                             const Type *finalVTableType,
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
                    return builder.CreateBitCast(inst, finalVTableType);
            
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
                                                   finalVTableType,
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
                                        Value *receiver,
                                        const vector<Value *> &args
                                        ) {
                
                BTypeDef *receiverType = funcDef->getReceiverType();
                assert(receiver->getType() == receiverType->rep);

                // get the underlying vtable
                Value *vtable =
                    getVTableReference(builder, vtableBaseType,
                                       receiverType->firstVTableType,
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
                return result;
            }

            void init(Value *receiver, const vector<Value *> &args) {
                // fill in all of the operands
                assert(NumOperands == args.size() + 1);
                OperandList[0] = receiver;
                for (int i = 0; i < args.size(); ++i)
                    OperandList[i + 1] = args[i];
            }
        
            IncompleteVirtualFunc(BTypeDef *vtableBaseType, BFuncDef *funcDef,
                                  Value *receiver,
                                  const vector<Value *> &args,
                                  BasicBlock *parent
                                  ) :
                PlaceholderInstruction(
                    BTypeDefPtr::arcast(funcDef->returnType)->rep, 
                    parent,
                    OperandTraits<IncompleteVirtualFunc>::op_end(this) - 
                     (args.size() + 1),
                    args.size() + 1
                ),
                vtableBaseType(vtableBaseType),
                funcDef(funcDef) {

                init(receiver, args);
            }
                                    
            IncompleteVirtualFunc(BTypeDef *vtableBaseType, BFuncDef *funcDef,
                                  Value *receiver,
                                  const vector<Value *> &args,
                                  Instruction *insertBefore = 0
                                  ) :
                PlaceholderInstruction(
                    BTypeDefPtr::arcast(funcDef->returnType)->rep, 
                    insertBefore,
                    OperandTraits<IncompleteVirtualFunc>::op_end(this) - 
                     (args.size() + 1),
                    args.size() + 1
                ),
                vtableBaseType(vtableBaseType),
                funcDef(funcDef) {
                
                init(receiver, args);
            }
        
            IncompleteVirtualFunc(BTypeDef *vtableBaseType, BFuncDef *funcDef,
                                  Use *operands,
                                  unsigned numOperands
                                  ) :
                PlaceholderInstruction(
                    BTypeDefPtr::arcast(funcDef->returnType)->rep,
                    static_cast<Instruction *>(0),
                    operands,
                    numOperands
                ),
                vtableBaseType(vtableBaseType),
                funcDef(funcDef) {
                
                for (int i = 0; i < numOperands; ++i)
                    OperandList[i] = operands[i];
            }
        
        public:

            virtual Instruction *clone(LLVMContext &context) const {
                return new(NumOperands) IncompleteVirtualFunc(vtableBaseType,
                                                              funcDef,
                                                              OperandList,
                                                              NumOperands
                                                              );
            }

            virtual void insertInstructions(IRBuilder<> &builder) {
                vector<Value *> args(NumOperands - 1);
                for (int i = 1; i < NumOperands; ++i)
                    args[i - 1] = OperandList[i];
                Value *callInst =
                    innerEmitCall(builder, vtableBaseType, funcDef, 
                                  OperandList[0],
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

                // if this is for a complete class, go ahead and emit the code.  
                // Otherwise just emit a placeholder.
                if (classCtx->complete) {
                    Value *val = innerEmitCall(llvmBuilder.builder, 
                                               vtableBaseType,
                                               funcDef, 
                                               receiver,
                                               args
                                               );
                    return val;
                } else {
                    PlaceholderInstruction *placeholder =
                        new(args.size() + 1) IncompleteVirtualFunc(
                            vtableBaseType,
                            funcDef,
                            receiver,
                            args,
                            llvmBuilder.block
                        );
                    BBuilderContextData *bdata =
                        BBuilderContextDataPtr::rcast(
                            classCtx->builderData
                        );
                    bdata->addPlaceholder(placeholder);
                    return placeholder;
                }
            }
    };
    
    const Type *llvmIntType = 0;
    
    /**
     * Instruction that does an un-GEP - widens from a base class to a 
     * derived class.
     */
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
                return new BResultExpr(this, value);
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
    
    class BConstDefImpl : public VarDefImpl {
        public:
            Constant *rep;
            
            BConstDefImpl(Constant *rep) : rep(rep) {}

            virtual ResultExprPtr emitRef(Context &context, VarRef *var) {
                LLVMBuilder &b =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                b.lastValue = rep;
                return new BResultExpr(var, b.lastValue);
            }
            
            virtual ResultExprPtr
            emitAssignment(Context &context, AssignExpr *assign) {
                assert(false && "assignment to a constant");
                return 0;
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
                    bdata->addPlaceholder(placeholder);
                }
                
                // release the aggregate
                aggregateResult->handleTransient(context);
                
                return new BResultExpr(this, bb.lastValue);
            }
    };

    SPUG_RCPTR(BArgVarDefImpl);

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

    class IncompleteSpecialize : public PlaceholderInstruction {
        private:
            Value *value;
            BFuncDefPtr funcDef;

        public:
            // allocate space for 1 operand
            void *operator new(size_t s) {
                return User::operator new(s, 1);
            }

            virtual Instruction *clone(LLVMContext &context) const {
                return new IncompleteSpecialize(getType(), value, 
                                                funcDef.get()
                                                );
            }
            
            /**
             * funcDef: the function that specialization is being called in.
             */
            IncompleteSpecialize(const Type *type, Value *value, 
                                 BFuncDef *funcDef,
                                 Instruction *insertBefore = 0
                                 ) :
                PlaceholderInstruction(
                    type,
                    insertBefore,
                    OperandTraits<IncompleteSpecialize>::op_begin(this),
                    OperandTraits<IncompleteSpecialize>::operands(this)
                ),
                value(value),
                funcDef(funcDef) {
                Op<0>() = value;
            }
            
            IncompleteSpecialize(const Type *type, Value *value, 
                                 BFuncDef *funcDef,
                                 BasicBlock *parent
                                 ) :
                PlaceholderInstruction(
                    type,
                    parent,
                    OperandTraits<IncompleteSpecialize>::op_begin(this),
                    OperandTraits<IncompleteSpecialize>::operands(this)
                ),
                value(value),
                funcDef(funcDef) {
                Op<0>() = value;
            }
            
            virtual void insertInstructions(IRBuilder<> &builder) {
                // XXX won't work for virtual base classes
                
                // create a constant offset from the start of the derived 
                // class to the start of the base class
                Value *offset =
                    narrowToAncestor(builder, 
                                     Constant::getNullValue(getType()),
                                     funcDef->pathToFirstDeclaration
                                     );

                // convert to an integer and subtract from the pointer to the 
                // base class.
                assert(llvmIntType && "integer type has not been initialized");
                offset = builder.CreatePtrToInt(offset, llvmIntType);
                value = builder.CreatePtrToInt(value, llvmIntType);
                Value *derived = builder.CreateSub(value, offset);
                Value *specialized = 
                    builder.CreateIntToPtr(derived, getType());
                replaceAllUsesWith(specialized);
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
            
            // the receiver variable
            VarDefPtr receiver;

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
                funcDef->returnType = returnType;
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
                FunctionType *llvmFuncType =
                    FunctionType::get(rawRetType, llvmArgs, false);
                LLVMBuilder &builder = 
                    dynamic_cast<LLVMBuilder &>(context.builder);
                Function *func = Function::Create(llvmFuncType,
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
                    receiver = context.lookUp("this");
                    assert(receiver &&
                            "missing 'this' variable in the context of a "
                            "function with a receiver"
                           );
                    receiver->impl = new BArgVarDefImpl(llvmArg);
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

                // get or create the type registered for the function                
                BTypeDef *crkFuncType = 
                    BTypeDefPtr::acast(builder.getFuncType(context,
                                                           llvmFuncType
                                                           )
                                       );
                funcDef->type = crkFuncType;
                
                // create an implementation object to return the function 
                // pointer
                funcDef->impl = new BConstDefImpl(func);
                
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
                
                // XXX we don't have a function type for these
                returnType = resultType;
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

#define UNOP(opCode) \
    class opCode##OpCall : public FuncCall {                                \
        public:                                                             \
            opCode##OpCall(FuncDef *def) : FuncCall(def) {}                 \
                                                                            \
            virtual ResultExprPtr emit(Context &context) {                  \
                receiver->emit(context)->handleTransient(context);          \
                                                                            \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(                         \
                        builder.lastValue,                                  \
                        BTypeDefPtr::arcast(func->returnType)->rep          \
                    );                                                      \
                                                                            \
                return new BResultExpr(this, builder.lastValue);            \
            }                                                               \
    };                                                                      \
                                                                            \
    class opCode##OpDef : public UnOpDef {                                  \
        public:                                                             \
            opCode##OpDef(TypeDef *resultType, const string &name) :        \
                UnOpDef(resultType, name) {                                 \
            }                                                               \
                                                                            \
            virtual FuncCallPtr createFuncCall() {                          \
                return new opCode##OpCall(this);                            \
            }                                                               \
    };

    UNOP(Trunc);
    UNOP(SExt);
    UNOP(ZExt);

    class NegateOpCall : public FuncCall {
        public:
            NegateOpCall(FuncDef *def) : FuncCall(def) {}
            
            virtual ResultExprPtr emit(Context &context) {
                args[0]->emit(context)->handleTransient(context);
                
                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                builder.lastValue =
                    builder.builder.CreateXor(
                        builder.lastValue,
                        ConstantInt::get(
                            BTypeDefPtr::arcast(func->returnType)->rep,
                            1
                            )
                    );
                
                return new BResultExpr(this, builder.lastValue);
            }
    };

    class NegateOpDef : public OpDef {
        public:
            NegateOpDef(BTypeDef *resultType, const std::string &name) :
                OpDef(resultType, FuncDef::noFlags, name, 1) {
                args[0] = new ArgDef(resultType, "operand");
            }
            
            virtual FuncCallPtr createFuncCall() {
                return new NegateOpCall(this);
            }
    };
    
    template<class T>
    class GeneralOpDef : public OpDef {
        public:
            GeneralOpDef(TypeDef *resultType, FuncDef::Flags flags,
                         const string &name,
                         size_t argCount
                         ) :
                OpDef(resultType, flags, name, argCount) {
                
                type = resultType;
            }
            
            virtual FuncCallPtr createFuncCall() {
                return new T(this);
            }
    };
    
    class ArrayGetItemCall : public FuncCall {
        public:
            ArrayGetItemCall(FuncDef *def) : FuncCall(def) {}
            
            virtual ResultExprPtr emit(Context &context) {
                receiver->emit(context)->handleTransient(context);

                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                Value *r = builder.lastValue;
                args[0]->emit(context)->handleTransient(context);
                Value *addr = builder.builder.CreateGEP(r, builder.lastValue);
                builder.lastValue = builder.builder.CreateLoad(addr);

                return new BResultExpr(this, builder.lastValue);
            }
    };
    
    class ArraySetItemCall : public FuncCall {
        public:
            ArraySetItemCall(FuncDef *def) : FuncCall(def) {}
            
            virtual ResultExprPtr emit(Context &context) {
                receiver->emit(context)->handleTransient(context);

                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);
                Value *r = builder.lastValue;
                args[0]->emit(context)->handleTransient(context);
                Value *i = builder.lastValue;
                args[1]->emit(context)->handleTransient(context);
                Value *addr = builder.builder.CreateGEP(r, i);
                builder.lastValue =
                    builder.builder.CreateStore(builder.lastValue, addr);
                
                return new BResultExpr(this, builder.lastValue);
            }
    };
    
    // implements pointer arithmetic
    class ArrayOffsetCall : public FuncCall {
        public:
            ArrayOffsetCall(FuncDef *def) : FuncCall(def) {}
            
            virtual ResultExprPtr emit(Context &context) {
                LLVMBuilder &builder =
                    dynamic_cast<LLVMBuilder &>(context.builder);

                args[0]->emit(context)->handleTransient(context);
                Value *base = builder.lastValue;

                args[1]->emit(context)->handleTransient(context);
                builder.lastValue =
                    builder.builder.CreateGEP(base, builder.lastValue);
                
                return new BResultExpr(this, builder.lastValue);
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
    BINOP(UDiv, "/");
    BINOP(SRem, "%");  // Note: C'99 defines '%' as the remainder, not modulo
    BINOP(URem, "%");  // the sign is that of the dividend, not divisor.

    BINOP(ICmpEQ, "==");
    BINOP(ICmpNE, "!=");
    BINOP(ICmpSGT, ">");
    BINOP(ICmpSLT, "<");
    BINOP(ICmpSGE, ">=");
    BINOP(ICmpSLE, "<=");
    BINOP(ICmpUGT, ">");
    BINOP(ICmpULT, "<");
    BINOP(ICmpUGE, ">=");
    BINOP(ICmpULE, "<=");
    
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
    
    assert(curType->isDerivedFrom(ancestor));

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
        bdata->addPlaceholder(placeholder);
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

TypeDef *LLVMBuilder::getFuncType(Context &context,
                                  const llvm::Type *llvmFuncType
                                  ) {

    // see if we've already got it
    FuncTypeMap::const_iterator iter = funcTypes.find(llvmFuncType);
    if (iter != funcTypes.end())
        return TypeDefPtr::rcast(iter->second);

    // nope.  create a new type object and store it
    BTypeDefPtr crkFuncType = new BTypeDef("", llvmFuncType);
    funcTypes[llvmFuncType] = crkFuncType;
    
    // Give it a context and an "oper to voidptr" method.
    crkFuncType->context =
        new Context(context.builder, Context::instance,
                    context.globalData
                    );
    crkFuncType->context->addDef(
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
    
    // XXX mega-hack, clear the contents of the allocated memory (this is to 
    // get around the temporary lack of automatic member initialization)
    
    // calculate the size of the structure
    assert(llvmIntType && "integer type has not been initialized");
    Value *startPos = builder.CreatePtrToInt(lastValue, llvmIntType);
    Value *endPos = 
        builder.CreatePtrToInt(
            builder.CreateConstGEP1_32(lastValue, 1),
            llvmIntType
            );
    Value *size = builder.CreateSub(endPos, startPos);
    
    // construct a call to the "memclear" function
    Function *memclearFunc = module->getFunction("__memclear");
    assert(memclearFunc && "__memclear function has not been defined");
    BTypeDef *voidPtrType =
        BTypeDefPtr::arcast(context.globalData->voidPtrType);
    vector<Value *> memclearArgs(2);
    memclearArgs[0] = builder.CreateBitCast(lastValue, voidPtrType->rep);
    memclearArgs[1] = size;
    builder.CreateCall(memclearFunc, memclearArgs.begin(), memclearArgs.end());
    
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

    context.createCleanupFrame();
    cond->emitCond(context);
    Value *condVal = lastValue;
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
    BFuncDef *funcDef = f.funcDef.get();
    
    // see if this is a method, if so store the class type as the receiver type
    unsigned vtableSlot = 0;
    BTypeDef *classType = 0;
    BBuilderContextData *classContextData;
    if (flags & FuncDef::method) {
        ContextPtr classCtx = context.getClassContext();
        assert(classCtx && "method is not nested in a class context.");
        classContextData = BBuilderContextDataPtr::rcast(classCtx->builderData);
        classType = classContextData->type.get();

        // create the vtable slot for a virtual function
        if (flags & FuncDef::virtualized)
            // use the original's slot if this is an override.
            if (override) {
                
                // find the path to the override's class
                BTypeDef *overrideClass =
                    BTypeDefPtr::arcast(override->context->returnType);
                classType->getPathToAncestor(*overrideClass, 
                                             funcDef->pathToFirstDeclaration
                                             );
                
                // augment it with the path from the override to its first 
                // declaration.
                BFuncDef *boverride = BFuncDefPtr::acast(override);
                f.funcDef->pathToFirstDeclaration.insert(
                    funcDef->pathToFirstDeclaration.end(),
                    boverride->pathToFirstDeclaration.begin(),
                    boverride->pathToFirstDeclaration.end()
                );

                // the type of the receiver is that of its first declaration                
                BTypeDef *receiverClass = boverride->getReceiverType();
                f.setReceiverType(receiverClass);
                
                vtableSlot = boverride->vtableSlot;
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
        PlaceholderInstruction *placeholder =
            new IncompleteSpecialize(classType->rep, inst, funcDef, block);
        classContextData->addPlaceholder(placeholder);

        // lookup the "this" variable, and replace its rep
        VarDefPtr thisVar = context.lookUp("this");
        BArgVarDefImpl *thisImpl = BArgVarDefImplPtr::arcast(thisVar->impl);
        thisImpl->rep = placeholder;
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
        VTableBuilder vtableBuilder;
        bdata->type->createAllVTables(
            vtableBuilder, 
            ".vtable." + bdata->type->name,
            BTypeDefPtr::arcast(context.globalData->vtableBaseType)
        );
        vtableBuilder.emit(module, bdata->type.get());
    }

    // fix-up all of the placeholder instructions
    for (vector<PlaceholderInstruction *>::iterator iter = 
            bdata->placeholders.begin();
         iter != bdata->placeholders.end();
         ++iter
         )
        (*iter)->fix();
    bdata->placeholders.clear();
    bdata->complete = true;
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
    
    // create "void __memclear(voidptr p, uint size)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "__memclear", 2);
        f.addArg("p", voidptrType);
        f.addArg("size", intType);
        f.finish();
    }
    
    // bind the module to the execution engine
    bindModule(new ExistingModuleProvider(module));
    
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
    closeAllCleanups(context);
    builder.CreateRetVoid();
    
    // restore the main function
    func = mainFunc;
    
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
    
    BModuleDefPtr::cast(moduleDef)->cleanup = 
        reinterpret_cast<void (*)()>(
            execEng->getPointerToFunction(module->getFunction("__del__"))
        );
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
    Context *varContext = assign->var->context;
    narrow(aggregate->type.get(), varContext->returnType.get());
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
        BBuilderContextData *bdata =
            BBuilderContextDataPtr::rcast(varContext->builderData);
        bdata->addPlaceholder(placeholder);
    }

    return new BResultExpr(assign, lastValue);
}

extern "C" void printint(int val) {
    std::cout << val << flush;
}

extern "C" void __memclear(void *p, size_t size) {
    memset(p, 0, size);
}

namespace {
    BTypeDef *createIntPrimType(Context &context, const Type *llvmType,
                             const char *name
                             ) {
        BTypeDefPtr btype = new BTypeDef(name, llvmType);
        btype->defaultInitializer =
            context.builder.createIntConst(context, 0, btype.get());
        btype->context =
            new Context(context.builder, Context::instance, 
                        context.globalData
                        );
        btype->context->returnType = btype;
        btype->context->addDef(new BoolOpDef(context.globalData->boolType.get(), 
                                             "toBool"
                                             )
                               );
        
        // if you remove this, for the love of god, change the return type so 
        // we don't leak the pointer.
        context.addDef(btype.get());
        return btype.get();
    }
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;
    LLVMContext &lctx = getGlobalContext();

    // create the basic types
    
    BTypeDef *voidType;
    gd->voidType = voidType = new BTypeDef("void", Type::getVoidTy(lctx));
    voidType->context = new Context(*this, Context::instance, gd);
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
    
    BTypeDef *byteType = createIntPrimType(context, Type::getInt8Ty(lctx),
                                           "byte"
                                           );

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
    
    // XXX bad assumptions about sizeof
    if (sizeof(int) == 4) {
        context.addAlias("int", int32Type);
        context.addAlias("uint", uint32Type);
        gd->uintType = uint32Type;
        gd->intType = int32Type;
        llvmIntType = int32Type->rep;
    } else {
        assert(sizeof(int) == 8);
        context.addAlias("int", int64Type);
        context.addAlias("uint", uint64Type);
        gd->uintType = uint64Type;
        gd->intType = int64Type;
        llvmIntType = int64Type->rep;
    }
        

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
    
    // conversions
    byteType->context->addDef(new ZExtOpDef(int32Type, "oper to int32"));
    byteType->context->addDef(new ZExtOpDef(int64Type, "oper to int64"));
    byteType->context->addDef(new ZExtOpDef(uint32Type, "oper to uint32"));
    byteType->context->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    int64Type->context->addDef(new TruncOpDef(uint64Type, "oper to uint64"));
    int64Type->context->addDef(new TruncOpDef(int32Type, "oper to int32"));
    int64Type->context->addDef(new TruncOpDef(uint32Type, "oper to uint32"));
    int64Type->context->addDef(new TruncOpDef(byteType, "oper to byte"));
    uint64Type->context->addDef(new TruncOpDef(int64Type, "oper to int64"));
    uint64Type->context->addDef(new TruncOpDef(int32Type, "oper to int32"));
    uint64Type->context->addDef(new TruncOpDef(uint32Type, "oper to uint32"));
    uint64Type->context->addDef(new TruncOpDef(byteType, "oper to byte"));
    int32Type->context->addDef(new TruncOpDef(byteType, "oper to byte"));
    int32Type->context->addDef(new TruncOpDef(uint32Type, "oper to uint32"));
    int32Type->context->addDef(new SExtOpDef(int64Type, "oper to int64"));
    int32Type->context->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    uint32Type->context->addDef(new TruncOpDef(byteType, "oper to byte"));
    uint32Type->context->addDef(new TruncOpDef(int32Type, "oper to int32"));
    uint32Type->context->addDef(new ZExtOpDef(uint64Type, "oper to uint64"));
    uint32Type->context->addDef(new ZExtOpDef(int64Type, "oper to int64"));
    
    // pointer equality check (to allow checking for None)
    context.addDef(new IsOpDef(voidPtrType, boolType));
    context.addDef(new IsOpDef(byteptrType, boolType));
    
    // boolean negate
    context.addDef(new NegateOpDef(boolType, "oper !"));
    
    // byteptr array indexing

    FuncDefPtr arrayGetItem = 
        new GeneralOpDef<ArrayGetItemCall>(byteType, FuncDef::method, 
                                           "oper []",
                                           1
                                           );
    arrayGetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    byteptrType->context->addDef(arrayGetItem.get());

    FuncDefPtr arraySetItem = 
        new GeneralOpDef<ArraySetItemCall>(byteType, FuncDef::method, 
                                           "oper []=",
                                           2
                                           );
    arraySetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    arraySetItem->args[1] = new ArgDef(byteType, "value");
    byteptrType->context->addDef(arraySetItem.get());
    
    FuncDefPtr arrayOffset =
        new GeneralOpDef<ArrayOffsetCall>(byteptrType, FuncDef::noFlags, 
                                          "oper +",
                                          2
                                          );
    arrayOffset->args[0] = new ArgDef(byteptrType, "base");
    arrayOffset->args[1] = new ArgDef(gd->uintType.get(), "offset");
    context.addDef(arrayOffset.get());
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
    bdata->addPlaceholder(vtableInit);
}
