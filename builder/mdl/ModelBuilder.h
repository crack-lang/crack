// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_mdl_ModelBuilder_h_
#define _builder_mdl_ModelBuilder_h_

#include "model/AllocExpr.h"
#include "model/AssignExpr.h"
#include "model/Branchpoint.h"
#include "model/CleanupFrame.h"
#include "model/FloatConst.h"
#include "model/FuncDef.h"
#include "model/IntConst.h"
#include "model/NullConst.h"
#include "model/StrConst.h"
#include "model/ResultExpr.h"
#include "model/TernaryExpr.h"
#include "model/TypeDef.h"
#include "model/VarDef.h"
#include "model/VarRef.h"

#include "builder/Builder.h"

#include "builder/mdl/ModelModuleDef.h"

namespace builder { namespace mdl {

SPUG_RCPTR(ModelBuilder);

class ModelBuilder : public builder::Builder {
    public:
        struct ModelResultExpr : public model::ResultExpr {
            ModelResultExpr(model::Expr *expr) : ResultExpr(expr) {}
            virtual model::ResultExprPtr emit(model::Context &context) {
                return new ModelResultExpr(this);
            }
        };

        struct ModelFuncDef : public model::FuncDef {
            ModelFuncDef(model::FuncDef::Flags flags, const std::string &name,
                        size_t argsSize
                        ) :
                FuncDef(flags, name, argsSize) {
            }
            virtual void *getFuncAddr(builder::Builder &builder) {
                return 0;
            }
        };

        struct ModelCleanupFrame : public model::CleanupFrame {
            ModelCleanupFrame(model::Context *context) : CleanupFrame(context) {}
            void addCleanup(model::Expr *expr) {}
            void close() {}
        };

        virtual builder::BuilderPtr createChildBuilder() {
            return new ModelBuilder();
        }

        virtual model::ResultExprPtr emitFuncCall(
            model::Context &context,
            model::FuncCall *funcCall
        ) {
            return new ModelResultExpr(funcCall);
        }

        virtual model::ResultExprPtr emitStrConst(model::Context &context,
                                                  model::StrConst *strConst
                                                  ) {
            return new ModelResultExpr(strConst);
        }

        virtual model::ResultExprPtr emitIntConst(model::Context &context,
                                                  model::IntConst *val
                                                  ) {
            return new ModelResultExpr(val);
        }

        virtual model::ResultExprPtr emitFloatConst(model::Context &context,
                                                    model::FloatConst *val
                                                   ) {
            return new ModelResultExpr(val);
        }

        virtual model::ResultExprPtr emitNull(model::Context &context,
                                              model::NullConst *nullExpr
                                              ) {
            return new ModelResultExpr(nullExpr);
        }

        virtual model::ResultExprPtr emitAlloc(model::Context &context,
                                               model::AllocExpr *allocExpr,
                                               model::Expr *countExpr = 0
                                               ) {
            return new ModelResultExpr(allocExpr);
        }

        virtual void emitTest(model::Context &context,
                              model::Expr *expr
                              ) {
        }

        virtual model::BranchpointPtr emitIf(model::Context &context,
                                             model::Expr *cond
                                             ) {
            return new model::Branchpoint(&context);
        }

        virtual model::BranchpointPtr emitElse(model::Context &context,
                                               model::Branchpoint *pos,
                                               bool terminal
                                               ) {
            return new model::Branchpoint(&context);
        }

        virtual void emitEndIf(model::Context &context,
                               model::Branchpoint *pos,
                               bool terminal
                               ) {
        }

        virtual model::TernaryExprPtr createTernary(model::Context &context,
                                                    model::Expr *cond,
                                                    model::Expr *trueVal,
                                                    model::Expr *falseVal,
                                                    model::TypeDef *type
                                                    ) {
            return new model::TernaryExpr(cond, trueVal, falseVal, type);
        }

        virtual model::ResultExprPtr emitTernary(model::Context &context,
                                                 model::TernaryExpr *expr
                                                 ) {
            return new ModelResultExpr(expr);
        }

        virtual model::BranchpointPtr emitBeginWhile(model::Context &context,
                                                     model::Expr *cond,
                                                     bool gotPostLoop
                                                     ) {
            return new model::Branchpoint();
        }

        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  ) {
        }

        virtual void emitPostLoop(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  ) {
        }

        virtual void emitBreak(model::Context &context,
                               model::Branchpoint *branch
                               ) {
        }

        virtual void emitContinue(model::Context &context,
                                  model::Branchpoint *branch
                                  ) {
        }

        virtual model::FuncDefPtr
            createFuncForward(model::Context &context,
                              model::FuncDef::Flags flags,
                              const std::string &name,
                              model::TypeDef *returnType,
                              const std::vector<model::ArgDefPtr> &args,
                              model::FuncDef *override
                              ) {
            model::FuncDefPtr result =
                new ModelFuncDef(flags, name, args.size());
            result->args = args;
        }

        virtual model::TypeDefPtr createClassForward(model::Context &context,
                                                     const std::string &name
                                                     ) {
            return new model::TypeDef(context.construct->classType.get(),
                                      name,
                                      true
                                      );
        }

        virtual model::FuncDefPtr emitBeginFunc(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            const std::vector<model::ArgDefPtr> &args,
            model::FuncDef *existing
        ) {
            if (existing)
                return existing;

            model::FuncDefPtr func = new ModelFuncDef(flags, name, args.size());
            func->args = args;
            return func;
        }

        virtual void emitEndFunc(model::Context &context,
                                 model::FuncDef *funcDef) {
        }

        virtual model::FuncDefPtr createExternFunc(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            model::TypeDef *receiverType,
            const std::vector<model::ArgDefPtr> &args,
            void *cfunc,
            const char *symbolName=0
        ) {
            model::FuncDefPtr func = new ModelFuncDef(flags, name, args.size());
            func->args = args;
            return func;
        }

        virtual model::TypeDefPtr emitBeginClass(
            model::Context &context,
            const std::string &name,
            const std::vector<model::TypeDefPtr> &bases,
            model::TypeDef *forwardDef
        ) {
            model::TypeDefPtr result;
            if (forwardDef)
                result = forwardDef;
            else
                result = new model::TypeDef(context.construct->classType.get(),
                                            name,
                                            true
                                            );
            result->parents = bases;
            return result;
        }

        virtual void emitEndClass(model::Context &context) {
        }

        virtual void emitReturn(model::Context &context,
                                model::Expr *expr) {
        }

        virtual model::BranchpointPtr emitBeginTry(model::Context &context
                                                   ) {
            return new model::Branchpoint();
        }

        virtual model::ExprPtr emitCatch(model::Context &context,
                                         model::Branchpoint *branchpoint,
                                         model::TypeDef *catchType,
                                         bool terminal
                                         ) {
            return new model::NullConst(catchType);
        }

        virtual void emitEndTry(model::Context &context,
                                model::Branchpoint *branchpoint,
                                bool terminal
                                ) {
        }

        virtual void emitExceptionCleanup(model::Context &context) {}

        virtual void emitThrow(model::Context &context,
                               model::Expr *expr
                               ) {
        }

        virtual model::VarDefPtr emitVarDef(
            model::Context &container,
            model::TypeDef *type,
            const std::string &name,
            model::Expr *initializer = 0,
            bool staticScope = false
        ) {
            return new model::VarDef(type, name);
        }

        virtual model::VarDefPtr createOffsetField(model::Context &context,
                                                   model::TypeDef *type,
                                                   const std::string &name,
                                                   size_t offset
                                                   ) {
            return new model::VarDef(type, name);
        }

        virtual model::ArgDefPtr createArgDef(model::TypeDef *type,
                                              const std::string &name
                                              ) {
            return new model::ArgDef(type, name);
        }

        virtual model::FuncCallPtr createFuncCall(model::FuncDef *func,
                                                  bool squashVirtual = false
                                                  ) {
            return new model::FuncCall(func, squashVirtual);
        }

        virtual model::VarRefPtr createVarRef(model::VarDef *varDef) {
            return new model::VarRef(varDef);
        }

        virtual model::VarRefPtr createFieldRef(model::Expr *aggregate,
                                                model::VarDef *varDef
                                                ) {
            return new model::VarRef(varDef);
        }

        virtual model::ResultExprPtr emitFieldAssign(model::Context &context,
                                                     model::Expr *aggregate,
                                                     model::AssignExpr *assign
                                                     ) {
            return new ModelResultExpr(assign);
        }

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name,
                                                 const std::string &path,
                                                 model::ModuleDef *owner
                                                 ) {
            return new ModelModuleDef(name, owner);
        }

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *modDef
                                 ) {
        }

        virtual CacheFilePtr getCacheFile(
            model::Context &context,
            const std::string &canonicalName
        ) {
            return new CacheFile();
        }

        virtual model::ModuleDefPtr materializeModule(
            model::Context &context,
            CacheFile *cacheFile,
            const std::string &canonicalName,
            model::ModuleDef *owner
        ) {
            return new ModelModuleDef(canonicalName, owner);
        }

        virtual model::VarDefPtr materializeVar(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type,
            int instSlot
        ) {
            return new model::VarDef(type, name);
        }

        virtual model::ArgDefPtr materializeArg(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type
        ) {
            return new model::ArgDef(type, name);
        }

        virtual model::TypeDefPtr materializeType(
            model::Context &context,
            const std::string &name,
            const std::string &namespaceName
        ) {
            return new model::TypeDef(context.construct->classType.get(), name,
                                      true
                                      );
        }

        virtual model::FuncDefPtr materializeFunc(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            const model::ArgVec &args
        ) {
            model::FuncDefPtr result =
                new ModelFuncDef(flags, name, args.size());
            result->args = args;
            result->returnType = returnType;
            return result;
        }

        virtual void cacheModule(model::Context &context,
                                 model::ModuleDef *module,
                                 const std::string &uniquifier
                                 ) {
        }

        virtual void finishCachedModule(
            model::Context &context,
            model::ModuleDef *module,
            const std::string &uniquifier,
            bool retain
        ) {
        }

        virtual model::CleanupFramePtr createCleanupFrame(
            model::Context &context
        ) {
            return new ModelCleanupFrame(&context);
        }

        virtual void closeAllCleanups(model::Context &context) {}

        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val
                                                  ) {
            return new model::StrConst(context.construct->stringType.get(),
                                       val
                                       );
        }

        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  int64_t val,
                                                  model::TypeDef *type = 0
                                                  ) {
            return new model::IntConst(type, val);
        }

        virtual model::IntConstPtr createUIntConst(model::Context &context,
                                                   uint64_t val,
                                                   model::TypeDef *type = 0
                                                   ) {
            return new model::IntConst(type, val);
        }

        virtual model::FloatConstPtr createFloatConst(model::Context &context,
                                                    double val,
                                                    model::TypeDef *type = 0
                                                    ) {
            return new model::FloatConst(type, val);
        }

        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context);

        virtual void initialize(model::Context &context) {}

        virtual void *loadSharedLibrary(const std::string &name) {
            return 0;
        }

        virtual void importSharedLibrary(const std::string &name,
                                         const model::ImportedDefVec &symbols,
                                         model::Context &context,
                                         model::Namespace *ns
                                         ) {
        }

        virtual void registerImportedDef(model::Context &context,
                                         model::VarDef *varDef
                                         ) {
        }

        virtual void initializeImport(model::ModuleDef*,
                                      const model::ImportedDefVec &symbols
                                      ) {
        }

        virtual void setArgv(int argc, char **argv) {}

        virtual void finishBuild(model::Context &context) {}

        virtual bool isExec() { return true; }

        virtual void emitVTableInit(model::Context &context,
                                    model::TypeDef *typeDef
                                    ) {
        }
};

}} // namespace builder::mdl
#endif
