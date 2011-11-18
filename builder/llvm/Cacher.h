// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>

namespace llvm {
    class Module;
    class MDNode;
    class Value;
}

namespace model {
    class VarDef;
    class Namespace;
}

namespace builder {

class BuilderOptions;

namespace mvll {

class BModuleDef;

class Cacher {

    static const std::string MD_VERSION;

    enum DefTypes {
        global = 0,
        function,
        member,
        method,
        type
    };

    BModuleDef *modDef;
    model::Context &context;
    const builder::BuilderOptions *options;

protected:
    void addNamedStringNode(const std::string &key, const std::string &val);
    std::string getNamedStringNode(const std::string &key);

    void writeNamespace(model::Namespace* ns);

    llvm::MDNode *writeType(model::TypeDef* t);
    llvm::MDNode *writeVarDef(model::VarDef *, model::TypeDef *owner);
    llvm::MDNode *writeFuncDef(model::FuncDef *, model::TypeDef *owner);

    void readFuncDef(const std::string &, llvm::Value *, llvm::MDNode *);
    void readTypeDef(const std::string &, llvm::MDNode *);

    void writeBitcode(const std::string &path);

    bool readImports();
    void readDefs();

    void writeMetadata();
    bool readMetadata();

public:

    Cacher(model::Context &c, builder::BuilderOptions* o, BModuleDef *m = NULL):
        modDef(m), context(c), options(o) { }

    BModuleDef *maybeLoadFromCache(const std::string &canonicalName,
                                   const std::string &path);
    void saveToCache();

};

} // end namespace builder::vmll
} // end namespace builder

#endif
