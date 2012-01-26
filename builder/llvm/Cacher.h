// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>
#include <vector>

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

class Cacher {

    static const std::string MD_VERSION;

    enum DefTypes {
        global = 0,
        function,
        member,
        method,
        type,
        constant,
        generic
    };

    BModuleDefPtr modDef;
    model::Context &context;
    const builder::BuilderOptions *options;

protected:
    void addNamedStringNode(const std::string &key, const std::string &val);
    std::string getNamedStringNode(const std::string &key);

    void writeNamespace(model::Namespace* ns);

    llvm::MDNode *writeTypeDef(model::TypeDef* t);
    llvm::MDNode *writeConstant(model::VarDef *, model::TypeDef *owner);
    llvm::MDNode *writeVarDef(model::VarDef *, model::TypeDef *owner);
    llvm::MDNode *writeFuncDef(model::FuncDef *, model::TypeDef *owner);

    void readConstant(const std::string &, llvm::Value *, llvm::MDNode *);
    void readVarDefMember(const std::string &, llvm::Value *, llvm::MDNode *);
    void readVarDefGlobal(const std::string &, llvm::Value *, llvm::MDNode *);
    void readFuncDef(const std::string &, llvm::Value *, llvm::MDNode *);
    BTypeDefPtr readMetaType(llvm::MDNode *mnode);
    void finishType(model::TypeDef *type, BTypeDef *metaType);
    void readTypeDef(const std::string &, llvm::Value *, llvm::MDNode *);
    void readGenericTypeDef(const std::string &, llvm::Value *, llvm::MDNode *);

    void writeBitcode(const std::string &path);

    bool readImports();
    void readDefs();

    void writeMetadata();
    bool readMetadata();

public:

    Cacher(model::Context &c, builder::BuilderOptions* o, BModuleDef *m = NULL):
        modDef(m), context(c), options(o) { }

    llvm::Function *getEntryFunction();

    void getExterns(std::vector<std::string> &symList);

    BModuleDefPtr maybeLoadFromCache(const std::string &canonicalName,
                                     const std::string &path);
    void saveToCache();

};

} // end namespace builder::vmll
} // end namespace builder

#endif
