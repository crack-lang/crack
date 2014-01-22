// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_Incompletes_h_
#define _builder_llvm_Incompletes_h_

#include "PlaceholderInstruction.h"
#include "model/Context.h"
#include <vector>

// for reasons I don't understand, we have to specialize the OperandTraits
// templates in the llvm namespace.
namespace builder { namespace mvll {

    class BTypeDef;
    class BFuncDef;

    class IncompleteInstVarRef;
    class IncompleteInstVarAddr;
    class IncompleteInstVarAssign;
    class IncompleteCatchSelector;
    class IncompleteNarrower;
    class IncompleteVTableInit;
    class IncompleteSpecialize;
    class IncompleteVirtualFunc;
    class IncompleteSizeOf;
} }

namespace llvm {
    template<>
    struct OperandTraits<builder::mvll::IncompleteInstVarRef> :
        FixedNumOperandTraits<builder::mvll::IncompleteInstVarRef, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteInstVarAddr> :
        FixedNumOperandTraits<builder::mvll::IncompleteInstVarAddr, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteInstVarAssign> :
        FixedNumOperandTraits<builder::mvll::IncompleteInstVarAssign, 2> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteCatchSelector> :
        FixedNumOperandTraits<builder::mvll::IncompleteCatchSelector, 0> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteNarrower> :
        FixedNumOperandTraits<builder::mvll::IncompleteNarrower, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteVTableInit> :
        FixedNumOperandTraits<builder::mvll::IncompleteVTableInit, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteSpecialize> :
        FixedNumOperandTraits<builder::mvll::IncompleteSpecialize, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteVirtualFunc> :
        VariadicOperandTraits<builder::mvll::IncompleteVirtualFunc, 1> {
    };
    template<>
    struct OperandTraits<builder::mvll::IncompleteSizeOf> :
        VariadicOperandTraits<builder::mvll::IncompleteSizeOf, 1> {
    };

    class BasicBlock;
    class Type;
    class Value;
    class Use;
    class Instruction;
}

// This is from llvm/OperandTraits.h. It's reproduced here because that
// version assumes use within the llvm namespace
#define CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(VALUECLASS) \
  public: \
  inline VALUECLASS *getOperand(unsigned) const; \
  inline void setOperand(unsigned, VALUECLASS*); \
  inline op_iterator op_begin(); \
  inline const_op_iterator op_begin() const; \
  inline op_iterator op_end(); \
  inline const_op_iterator op_end() const; \
  protected: \
  template <int> inline llvm::Use &Op(); \
  template <int> inline const llvm::Use &Op() const; \
  public: \
  inline unsigned getNumOperands() const


namespace builder {
namespace mvll {

SPUG_RCPTR(BFieldDefImpl);

/** an incomplete reference to an instance variable. */
class IncompleteInstVarRef : public PlaceholderInstruction {
private:
    BFieldDefImplPtr fieldImpl;

public:
    // allocate space for 1 operand
    void *operator new(size_t s);

    IncompleteInstVarRef(llvm::Type *type,
                         llvm::Value *aggregate,
                         BFieldDefImpl *fieldImpl,
                         llvm::BasicBlock *parent
                         );

    IncompleteInstVarRef(llvm::Type *type,
                         llvm::Value *aggregate,
                         BFieldDefImpl *fieldImpl,
                         llvm::Instruction *insertBefore = 0
                         );

    virtual llvm::Instruction *clone_impl() const;

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

/**
 * An incomplete reference to an instance variable address.  This should
 * really be merged with IncompleteInstVarRef
 */
class IncompleteInstVarAddr : public PlaceholderInstruction {
private:
    BFieldDefImplPtr fieldImpl;

public:
    // allocate space for 1 operand
    void *operator new(size_t s);

    IncompleteInstVarAddr(llvm::Type *type,
                          llvm::Value *aggregate,
                          BFieldDefImpl *fieldImpl,
                          llvm::BasicBlock *parent
                          );

    IncompleteInstVarAddr(llvm::Type *type,
                          llvm::Value *aggregate,
                          BFieldDefImpl *fieldImpl,
                          llvm::Instruction *insertBefore = 0
                          );

    virtual llvm::Instruction *clone_impl() const;

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);
};

class IncompleteInstVarAssign : public PlaceholderInstruction {
private:
    BFieldDefImplPtr fieldDefImpl;

public:
    // allocate space for 2 operands
    void *operator new(size_t s);

    IncompleteInstVarAssign(llvm::Type *type,
                            llvm::Value *aggregate,
                            BFieldDefImpl *fieldDefImpl,
                            llvm::Value *rval,
                            llvm::BasicBlock *parent
                            );

    IncompleteInstVarAssign(llvm::Type *type,
                            llvm::Value *aggregate,
                            BFieldDefImpl *fieldDefImpl,
                            llvm::Value *rval,
                            llvm::Instruction *insertBefore = 0
                            );

    virtual llvm::Instruction *clone_impl() const;

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

class IncompleteCatchSelector : public PlaceholderInstruction {
private:
    llvm::Value *personalityFunc;

public:
    // pointers to the type implementation globals, which are set on
    // completion of the catch clause.
    std::vector<llvm::Value *> *typeImpls;

    // allocate space for 0 operands
    // NOTE: We don't make use of any of the operand magic because none of the
    // associated value objects should be replacable.  If you start seeing
    // value breakage in the exception selectors, look here because that
    // assumption has probably been violated.
    void *operator new(size_t s);

    IncompleteCatchSelector(llvm::Type *type,
                            llvm::Value *personalityFunc,
                            llvm::BasicBlock *parent
                            );

    IncompleteCatchSelector(llvm::Type *type,
                            llvm::Value *personalityFunc,
                            llvm::Instruction *insertBefore = 0
                            );

    ~IncompleteCatchSelector();

    virtual llvm::Instruction *clone_impl() const;

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

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
    void *operator new(size_t s);

    IncompleteNarrower(llvm::Value *aggregate,
                       BTypeDef *startType,
                       BTypeDef *ancestor,
                       llvm::BasicBlock *parent
                       );

    IncompleteNarrower(llvm::Value *aggregate,
                       BTypeDef *startType,
                       BTypeDef *ancestor,
                       llvm::Instruction *insertBefore = 0
                                                   );

    virtual llvm::Instruction *clone_impl() const;

    /**
     * Emits the GEP instructions to narrow 'inst' from 'type' to
     * 'ancestor'.  Returns the resulting end-value.
     */
    static llvm::Value *emitGEP(llvm::IRBuilder<> &builder,
                                BTypeDef *type,
                                BTypeDef *ancestor,
                                llvm::Value *inst
                                );

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

class IncompleteVTableInit : public PlaceholderInstruction {
public:
    // allocate space for 1 operand
    void *operator new(size_t s);

    // we can make these raw pointers, the type _must_ be in existence
    // during the lifetime of this object.
    BTypeDef *aggregateType;
    BTypeDef *vtableBaseType;

    IncompleteVTableInit(BTypeDef *aggregateType,
                         llvm::Value *aggregate,
                         BTypeDef *vtableBaseType,
                         llvm::BasicBlock *parent
                         );

    IncompleteVTableInit(BTypeDef *aggregateType,
                         llvm::Value *aggregate,
                         BTypeDef *vtableBaseType,
                         llvm::Instruction *insertBefore = 0
                         );

    virtual llvm::Instruction *clone_impl() const;

    // emit the code to initialize the first VTable in btype.
    void emitInitOfFirstVTable(llvm::IRBuilder<> &builder,
                               BTypeDef *btype,
                               llvm::Value *inst,
                               llvm::Constant *vtable
                               );

    // emit the code to initialize all vtables in an object.
    void emitVTableInit(llvm::IRBuilder<> &builder,
                        BTypeDef *btype,
                        llvm::Value *inst
                        );

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

class IncompleteVirtualFunc : public PlaceholderInstruction {
private:
    BTypeDef *vtableBaseType;
    BFuncDef *funcDef;
    llvm::BasicBlock *normalDest, *unwindDest;

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
    static llvm::Value *getVTableReference(llvm::IRBuilder<> &builder,
                                           BTypeDef *vtableBaseType,
                                           llvm::Type *finalVTableType,
                                           BTypeDef *curType,
                                           llvm::Value *inst
                                           );

    static llvm::Value *innerEmitCall(llvm::IRBuilder<> &builder,
                                      BTypeDef *vtableBaseType,
                                      BFuncDef *funcDef,
                                      llvm::Value *receiver,
                                      const std::vector<llvm::Value *> &args,
                                      llvm::BasicBlock *normalDest,
                                      llvm::BasicBlock *unwindDest
                                      );

    void init(llvm::Value *receiver, const std::vector<llvm::Value *> &args);

    IncompleteVirtualFunc(BTypeDef *vtableBaseType,
                          BFuncDef *funcDef,
                          llvm::Value *receiver,
                          const std::vector<llvm::Value *> &args,
                          llvm::BasicBlock *parent,
                          llvm::BasicBlock *normalDest,
                          llvm::BasicBlock *unwindDest
                          );

    IncompleteVirtualFunc(BTypeDef *vtableBaseType,
                          BFuncDef *funcDef,
                          llvm::Value *receiver,
                          const std::vector<llvm::Value *> &args,
                          llvm::BasicBlock *normalDest,
                          llvm::BasicBlock *unwindDest,
                          llvm::Instruction *insertBefore = 0
                          );

    IncompleteVirtualFunc(BTypeDef *vtableBaseType,
                          BFuncDef *funcDef,
                          llvm::Use *operands,
                          unsigned numOperands,
                          llvm::BasicBlock *normalDest,
                          llvm::BasicBlock *unwindDest
                          );

public:

    virtual llvm::Instruction *clone_impl() const;

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    static llvm::Value *emitCall(model::Context &context,
                                 BFuncDef *funcDef,
                                 llvm::Value *receiver,
                                 const std::vector<llvm::Value *> &args,
                                 llvm::BasicBlock *normalDest,
                                 llvm::BasicBlock *unwindDest
                                 );

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

/**
 * Instruction that does an un-GEP - widens from a base class to a
 * derived class.
s */
class IncompleteSpecialize : public PlaceholderInstruction {
private:
    Value *value;
    BTypeDef *curType, *ancestorType;
    llvm::Type *uintzType;

public:
    // allocate space for 1 operand
    void *operator new(size_t s);

    virtual llvm::Instruction *clone_impl() const;

    /**
     * ancestorPath: path from the target class to the ancestor that
     *  value is referencing an instance of.
     */
    IncompleteSpecialize(llvm::Type *type,
                         llvm::Value *value,
                         BTypeDef *curType,
                         BTypeDef *ancestorType,
                         llvm::Type *uintzType,
                         llvm::Instruction *insertBefore = 0
                         );

    IncompleteSpecialize(llvm::Type *type,
                         llvm::Value *value,
                         BTypeDef *curType,
                         BTypeDef *ancestorType,
                         llvm::Type *uintzType,
                         llvm::BasicBlock *parent
                         );

    static Value *emitSpecializeInner(
        llvm::IRBuilder<> &builder,
        llvm::Type *type,
        llvm::Value *value,
        BTypeDef *curType,
        BTypeDef *ancestorType,
        llvm::Type *uintzType
    );

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    // Utility function - emits the specialize instructions if the
    // target class is defined, emits a placeholder instruction if it
    // is not.
    static Value *emitSpecialize(
        model::Context &context,
        BTypeDef *type,
        llvm::Value *value,
        BTypeDef *ancestorType
    );

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

};

class IncompleteSizeOf : public PlaceholderInstruction {
private:
    llvm::Type *type, *intType;

public:
    // allocate space for 0 operands
    void *operator new(size_t s);

    virtual llvm::Instruction *clone_impl() const;

    IncompleteSizeOf(llvm::Type *type,
                     llvm::Type *intType,
                     llvm::Instruction *insertBefore = 0
                     );

    IncompleteSizeOf(llvm::Type *type,
                     llvm::Type *intType,
                     llvm::BasicBlock *parent
                     );

    static llvm::Value *emitInner(llvm::Type *type, llvm::Type *intType,
                                  llvm::IRBuilder<> &builder
                                  );

    virtual void insertInstructions(llvm::IRBuilder<> &builder);

    // Emit the sizeof instructions if the type is finished, creates the
    // placeholders if not.
    static Value *emitSizeOf(model::Context &context,
                             BTypeDef *type,
                             llvm::Type *intType
                             );

    CRACK_DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);
};

} // end namespace builder::vmll
} // end namespace builder

#endif
