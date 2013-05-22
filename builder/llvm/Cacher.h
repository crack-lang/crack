// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>
#include <vector>
#include <map>

namespace llvm {
    class Module;
    class MDNode;
    class Value;
    class Function;
}

namespace model {
    class Namespace;
    class TypeDef;
    class VarDef;
}

namespace builder {

class BuilderOptions;

namespace mvll {

SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(BTypeDef);
SPUG_RCPTR(LLVMBuilder);

class Cacher {

    static const std::string MD_VERSION;

    enum DefTypes {
        global = 0,
        function,
        member,
        method,
        type,
        constant,
        generic,
        ephemeralImport
    };

    BModuleDefPtr modDef;
    model::ContextPtr context;
    model::Context &parentContext;
    builder::BuilderOptions *options;
    builder::mvll::LLVMBuilderPtr builder;

    // vardefs which were created as a result of shared lib import
    // we skip these in crack_defs
    std::map<std::string, bool> shlibImported;

protected:
    void addNamedStringNode(const std::string &key, const std::string &val);
    std::string getNamedStringNode(const std::string &key);

    void writeNamespace(model::Namespace* ns);

    llvm::MDNode *writeTypeDef(model::TypeDef* t);
    llvm::MDNode *writeConstant(model::VarDef *, model::TypeDef *owner);
    llvm::MDNode *writeVarDef(model::VarDef *, model::TypeDef *owner);
    llvm::MDNode *writeFuncDef(model::FuncDef *, model::TypeDef *owner);
    llvm::MDNode *writeEphemeralImport(BModuleDef *mod);

    void readConstant(const std::string &, llvm::Value *, llvm::MDNode *);
    void readVarDefMember(const std::string &, llvm::Value *, llvm::MDNode *);
    model::TypeDefPtr resolveType(const std::string &name);
    void readVarDefGlobal(const std::string &, llvm::Value *, llvm::MDNode *);
    void readFuncDef(const std::string &, llvm::Value *, llvm::MDNode *);
    BTypeDefPtr readMetaType(llvm::MDNode *mnode);
    void finishType(model::TypeDef *type, BTypeDef *metaType,
                    model::NamespacePtr owner);
    void readTypeDef(const std::string &, llvm::Value *, llvm::MDNode *);
    void readGenericTypeDef(const std::string &, llvm::Value *, llvm::MDNode *);
    void readEphemeralImport(llvm::MDNode *mnode);

    void resolveStructs(llvm::Module *);

    void writeBitcode(const std::string &path);

    bool readImports();
    void readDefs();

    bool readMetadata();

public:

    Cacher(model::Context &c, builder::BuilderOptions *o,
           BModuleDef *m = NULL
           );

    void writeMetadata();
    llvm::Function *getEntryFunction();

    void getExterns(std::vector<std::string> &symList);

    BModuleDefPtr maybeLoadFromCache(const std::string &canonicalName);
    void saveToCache();

};

} // end namespace builder::vmll
} // end namespace builder

#endif
