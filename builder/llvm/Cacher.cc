// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Cacher.h"
#include "BModuleDef.h"
#include "model/Deserializer.h"
#include "model/Generic.h"
#include "model/GlobalNamespace.h"
#include "model/Namespace.h"
#include "model/OverloadDef.h"
#include "model/Serializer.h"
#include "model/NullConst.h"
#include "model/ConstVarDef.h"

#include "spug/check.h"

#include <assert.h>
#include <sstream>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/IR/LLVMContext.h>

#include "model/EphemeralImportDef.h"
#include "builder/BuilderOptions.h"
#include "util/CacheFiles.h"
#include "util/SourceDigest.h"
#include "LLVMBuilder.h"
#include "VarDefs.h"
#include "BFuncDef.h"
#include "Consts.h"
#include "StructResolver.h"

using namespace llvm;
using namespace llvm::sys;
using namespace builder::mvll;
using namespace std;
using namespace model;
using namespace crack::util;

#define VLOG(level) if (options->verbosity >= (level)) cerr

void Cacher::writeMetadata() {
    vector<Value *> dList;
    NamedMDNode *node;
    Module *module = modDef->rep;

    // crack_shlib_imports: operand list points to shared lib import nodes
    node = module->getOrInsertNamedMetadata("crack_shlib_imports");
    for (BModuleDef::ShlibImportListType::const_iterator iIter =
         modDef->shlibImportList.begin();
         iIter != modDef->shlibImportList.end();
         ++iIter
         ) {

        // op 1: shared lib name
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*iIter).first));

        // op 2..n: symbols to be imported
        for (ImportedDefVec::const_iterator sIter = (*iIter).second.begin();
             sIter != (*iIter).second.end();
             ++sIter) {
            dList.push_back(MDString::get(getGlobalContext(), sIter->local));
            dList.push_back(MDString::get(getGlobalContext(), sIter->source));
        }

        node->addOperand(MDNode::get(getGlobalContext(), dList));
        dList.clear();
    }
}

bool Cacher::readImports() {

    MDNode *mnode;
    MDString *cname, *localStr, *sourceStr;

    NamedMDNode *imports = modDef->rep->getNamedMetadata("crack_shlib_imports");
    assert(imports && "missing crack_shlib_imports node");

    ImportedDefVec symList;
    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // op 1: lib name
        cname = dyn_cast<MDString>(mnode->getOperand(0));
        assert(cname && "malformed shlib import node: lib name");

        // op 2..n: imported symbols from m
        for (unsigned si = 1; si < mnode->getNumOperands(); ++si) {
            localStr = dyn_cast<MDString>(mnode->getOperand(si));
            sourceStr = dyn_cast<MDString>(mnode->getOperand(si));
            assert(localStr && "malformed shlib import node: local name");
            assert(sourceStr && "malformed shlib import node: source name");
            shlibImported[localStr->getString().str()] = true;
            symList.push_back(ImportedDef(localStr->getString().str(),
                                          sourceStr->getString().str()
                                          )
                              );
        }

        builder->importSharedLibrary(cname->getString().str(), symList, 
                                     *context,
                                     0
                                     );

    }

    return true;

}

Cacher::Cacher(model::Context &c, builder::BuilderOptions *o, 
               BModuleDef *m
               ) :
    modDef(m), 
    parentContext(c),
    options(o) {
}

bool Cacher::getCacheFile(const string &canonicalName,
                          OwningPtr<MemoryBuffer> &fileBuf
                          ) {
    string cacheFile = getCacheFilePath(options, *parentContext.construct, 
                                        canonicalName, 
                                        "bc"
                                        );
    if (cacheFile.empty())
        return false;

    VLOG(2) << "[" << canonicalName << "] cache: maybeLoad "
        << cacheFile << endl;

    if (error_code ec = MemoryBuffer::getFile(cacheFile.c_str(), fileBuf)) {
        VLOG(2) << "[" << canonicalName <<
            "] cache: not cached or inaccessible" << endl;
        return false;
    }

    return true;
}

BModuleDefPtr Cacher::maybeLoadFromCache(
    const string &canonicalName,
    OwningPtr<MemoryBuffer> &fileBuf
) {

    // create a builder and module context
    builder = 
        LLVMBuilderPtr::rcast(parentContext.builder.createChildBuilder());
    context = new Context(*builder, Context::module, &parentContext,
                          new GlobalNamespace(parentContext.ns.get(),
                                              canonicalName
                                              ),
                          0 // no compile namespace necessary
                          );
    context->toplevel = true;

    string errMsg;
    Module *module = getLazyBitcodeModule(fileBuf.take(),
                                          getGlobalContext(),
                                          &errMsg);
    if (!module) {
        fileBuf.reset();
        VLOG(1) << "[" << canonicalName <<
            "] cache: exists but unable to load bitcode" << endl;
        return NULL;
    }

    // if we get here, we've loaded bitcode successfully
    modDef = builder->instantiateModule(*context, canonicalName, module);
    builder->module = module;
    readImports();

    // cache hit
    VLOG(2) << "[" << canonicalName << "] cache materialized" << endl;
    return modDef;
}
