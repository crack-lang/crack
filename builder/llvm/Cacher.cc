// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

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

#include "builder/llvm/LLVMBuilder.h"
#include "builder/BuilderOptions.h"
#include "builder/llvm/VarDefs.h"
#include "builder/llvm/BFuncDef.h"
#include "builder/util/CacheFiles.h"
#include "builder/util/SourceDigest.h"
#include "builder/llvm/Consts.h"

#include <assert.h>
#include <sstream>
#include <vector>

#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/LLVMContext.h>

using namespace llvm;
using namespace llvm::sys;
using namespace builder::mvll;
using namespace std;
using namespace model;

// metadata version
const std::string Cacher::MD_VERSION = "1";

namespace {
    ConstantInt *constInt(int c) {
        return ConstantInt::get(Type::getInt32Ty(getGlobalContext()), c);
    }
}

Function *Cacher::getEntryFunction() {

    NamedMDNode *node = modDef->rep->getNamedMetadata("crack_entry_func");
    if (!node)
        cerr << "in module: " << modDef->name << endl;
    assert(node && "no crack_entry_func");
    MDNode *funcNode = node->getOperand(0);
    assert(funcNode && "malformed crack_entry_func");
    Function *func = dyn_cast<Function>(funcNode->getOperand(0));
    assert(func && "entry function not LLVM Function!");
    return func;

}

void Cacher::getExterns(std::vector<std::string> &symList) {

    assert(symList.size() == 0 && "invalid or nonempty symList");

    NamedMDNode *externs = modDef->rep->getNamedMetadata("crack_externs");
    assert(externs && "no crack_externs node");

    if (externs->getNumOperands()) {
        MDString *sym;
        MDNode *symNode = externs->getOperand(0);
        for (int i = 0; i < symNode->getNumOperands(); ++i) {

            sym = dyn_cast<MDString>(symNode->getOperand(i));
            assert(sym && "malformed crack_externs");

            symList.push_back(sym->getString().str());

        }
    }

}

void Cacher::addNamedStringNode(const string &key, const string &val) {

    vector<Value *> dList;
    NamedMDNode *node;

    node = modDef->rep->getOrInsertNamedMetadata(key);
    dList.push_back(MDString::get(getGlobalContext(), val));
    node->addOperand(MDNode::get(getGlobalContext(), dList));

}

string Cacher::getNamedStringNode(const std::string &key) {

    NamedMDNode *node;
    MDNode *mnode;
    MDString *str;

    node = modDef->rep->getNamedMetadata(key);
    assert(node && "missing required named string node");
    mnode = node->getOperand(0);
    assert(mnode && "malformed string node 1");
    str = dyn_cast<MDString>(mnode->getOperand(0));
    assert(str && "malformed string node 2");
    return str->getString().str();

}

void Cacher::writeMetadata() {

    // encode metadata into the bitcode
    addNamedStringNode("crack_md_version", Cacher::MD_VERSION);
    addNamedStringNode("crack_origin_digest", modDef->digest.asHex());
    addNamedStringNode("crack_origin_path", modDef->path);

    vector<Value *> dList;
    NamedMDNode *node;
    Module *module = modDef->rep;

    // crack_imports: operand list points to import nodes
    node = module->getOrInsertNamedMetadata("crack_imports");
    for (BModuleDef::ImportListType::const_iterator iIter =
            modDef->importList.begin();
         iIter != modDef->importList.end();
         ++iIter
         ) {
        // op 1: canonical name
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*iIter).first->getFullName()));
        // op 2: digest
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*iIter).first->digest.asHex()));

        // op 3..n: symbols to be imported (aliased)
        for (vector<string>::const_iterator sIter = (*iIter).second.begin();
             sIter != (*iIter).second.end();
             ++sIter) {
            dList.push_back(MDString::get(getGlobalContext(), *sIter));
        }

        node->addOperand(MDNode::get(getGlobalContext(), dList));
        dList.clear();
    }

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
        for (vector<string>::const_iterator sIter = (*iIter).second.begin();
             sIter != (*iIter).second.end();
             ++sIter) {
            dList.push_back(MDString::get(getGlobalContext(), *sIter));
        }

        node->addOperand(MDNode::get(getGlobalContext(), dList));
        dList.clear();
    }

    // crack_externs: these we need to resolve upon load. in the JIT, that means
    // global mappings. we need to resolve functions and globals
    node = module->getOrInsertNamedMetadata("crack_externs");
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    // functions
    for (LLVMBuilder::ModFuncMap::const_iterator i = b.moduleFuncs.begin();
         i != b.moduleFuncs.end();
         ++i) {

        // only include it if it's a decl...
        if (!i->second->isDeclaration())
            continue;

        Namespace *owningNS = i->first->getOwner();
        assert(owningNS && "no owner");

        // skips anything in builtin namespace
        if (owningNS->getNamespaceName().substr(0,8) == ".builtin") {
            continue;
        }

        // and it's defined in another module
        // but skip externals from extensions, since these are found by
        // the jit through symbol searching the process
        ModuleDefPtr owningModule = owningNS->getModule();
        if ((owningModule && owningModule->fromExtension) ||
            (owningNS == modDef->getParent(0).get())) {
            continue;
        }

        dList.push_back(MDString::get(getGlobalContext(), i->second->getNameStr()));
    }

    // globals
    for (LLVMBuilder::ModVarMap::const_iterator i = b.moduleVars.begin();
         i != b.moduleVars.end();
         ++i) {
        if (!i->second->isDeclaration())
            continue;
        dList.push_back(MDString::get(getGlobalContext(), i->second->getNameStr()));
    }
    if (dList.size()) {
        node->addOperand(MDNode::get(getGlobalContext(), dList));
        dList.clear();
    }

    // crack_defs: the symbols defined in this module that we need to rebuild
    // at compile time in order to use this cached module to compile fresh code
    // from
    writeNamespace(modDef.get());

    //module->dump();

}

void Cacher::writeNamespace(Namespace *ns) {

    OverloadDef *ol;
    TypeDef *td;
    TypeDef *owner = dynamic_cast<TypeDef*>(ns);
    NamedMDNode *node = modDef->rep->getOrInsertNamedMetadata("crack_defs");

    for (Namespace::VarDefVec::const_iterator i = ns->beginOrderedForCache();
         i != ns->endOrderedForCache();
         ++i) {
        if (ol = dynamic_cast<OverloadDef*>((*i).get())) {
            for (OverloadDef::FuncList::const_iterator f = ol->beginTopFuncs();
                 f != ol->endTopFuncs();
                 ++f) {
                // skip aliases
                if ((*f)->getOwner() == ns) {
                    MDNode *n = writeFuncDef((*f).get(), owner);
                    if (n)
                        node->addOperand(n);
                }
            }
        }
        else {
            // skip aliases
            if ((*i)->getOwner() != ns)
                continue;
            if (td = TypeDefPtr::rcast(*i)) {
                MDNode *typeNode = writeTypeDef(td);
                if (typeNode) {
                    node->addOperand(typeNode);
                    writeNamespace(td);
                }
            }
            else {

                // VarDef

                // XXX hack to not write exStruct
                if ((*i)->name == ":exStruct")
                    continue;

                if ((*i)->isConstant())
                    node->addOperand(writeConstant((*i).get(), owner));
                else
                    node->addOperand(writeVarDef((*i).get(), owner));

            }
        }
    }

}

MDNode *Cacher::writeTypeDef(model::TypeDef* t) {

    if (options->verbosity >= 2)
        cerr << "writing type " << t->name << " in module " << modDef->name <<
            endl;
    BTypeDef *bt = dynamic_cast<BTypeDef *>(t);
    assert((bt || t->generic) && "not BTypeDef");

    vector<Value *> dList;

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), t->name));

    // operand 1: symbol type
    dList.push_back(constInt(t->generic ? Cacher::generic : Cacher::type));

    // operand 2: if this is a generic, operand 2 is the serialized generic
    // value.  Otherwise it is the initializer.
    if (t->generic) {
        ostringstream tmp;
        model::Serializer ser(tmp);
        t->genericInfo->serialize(ser);
        dList.push_back(MDString::get(getGlobalContext(), tmp.str()));
    } else {
        dList.push_back(Constant::getNullValue(bt->rep));
    }

    // operand 3: metatype type (name string)
    TypeDef *metaClass = t->type.get();
    assert(metaClass && "no meta class");
    dList.push_back(MDString::get(getGlobalContext(), metaClass->name));

    // operand 4: metatype type (null initializer)
    Type *metaTypeRep = BTypeDefPtr::acast(metaClass)->rep;
    dList.push_back(Constant::getNullValue(metaTypeRep));

    return MDNode::get(getGlobalContext(), dList);

}

MDNode *Cacher::writeFuncDef(FuncDef *sym, TypeDef *owner) {

    vector<Value *> dList;

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), sym->name));

    // operand 1: symbol type
    if (owner)
        dList.push_back(constInt(Cacher::method));
    else
        dList.push_back(constInt(Cacher::function));

    // operand 2: llvm rep
    BFuncDef *bf = dynamic_cast<BFuncDef *>(sym);
    if (bf)
        dList.push_back(bf->rep);
    else {
        //cout << "skipping " << sym->name << "\n";
        //dList.push_back(NULL);
        return NULL;
    }

    // operand 3: typedef owner
    if (owner)
        dList.push_back(MDString::get(getGlobalContext(),
                                      owner->getFullName()));
    else
        dList.push_back(NULL);

    // operand 4: funcdef flags
    dList.push_back(constInt(sym->flags));

    // operand 5: return type
    dList.push_back(MDString::get(getGlobalContext(),
                                  sym->returnType->getFullName()));

    // operand 6..ARITY: pairs of parameter symbol names and their types
    for (FuncDef::ArgVec::const_iterator i = sym->args.begin();
         i != sym->args.end();
         ++i) {
        dList.push_back(MDString::get(getGlobalContext(), (*i)->name));
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*i)->type->getFullName()));
    }

    // we register with the cache map because a cached module may be
    // on this depended one for this run
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, sym);

    return MDNode::get(getGlobalContext(), dList);

}

MDNode *Cacher::writeConstant(VarDef *sym, TypeDef *owner) {

    vector<Value *> dList;

    BTypeDef *type = dynamic_cast<BTypeDef *>(sym->type.get());

    // int and float will be ConstVarDef so we can get at the value
    ConstVarDef *ivar = dynamic_cast<ConstVarDef *>(sym);

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), sym->name));

    // operand 1: symbol type
    dList.push_back(constInt(Cacher::constant));

    // operand 2: int or float value, if we have one
    if (ivar) {
        BIntConst *bi = dynamic_cast<BIntConst *>(ivar->expr.get());
        if (bi) {
            dList.push_back(bi->rep);
        }
        else {
            BFloatConst *bf = dynamic_cast<BFloatConst *>(ivar->expr.get());
            assert(bf && "unknown ConstVarDef: not int or float");
            dList.push_back(bf->rep);
        }
    }
    else {
        // const object, no rep
        dList.push_back(NULL);
    }

    // operand 3: type name
    dList.push_back(MDString::get(getGlobalContext(),
                                  type->getFullName()));

    /*
    // operand 4: typedef owner (XXX future?)
    if (owner)
        dList.push_back(MDString::get(getGlobalContext(), owner->name));
    else
        dList.push_back(NULL);
    */

    // we register with the cache map because a cached module may be
    // on this depended one for this run
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, sym);

    return MDNode::get(getGlobalContext(), dList);

}

MDNode *Cacher::writeVarDef(VarDef *sym, TypeDef *owner) {

    vector<Value *> dList;

    BTypeDef *type = dynamic_cast<BTypeDef *>(sym->type.get());
    assert(type && "writeVarDef: no type");
    BGlobalVarDefImpl *gvar = dynamic_cast<BGlobalVarDefImpl *>(sym->impl.get());
    BInstVarDefImpl *ivar = dynamic_cast<BInstVarDefImpl *>(sym->impl.get());
    assert(gvar || ivar && "not global or instance");

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), sym->name));

    // operand 1: symbol type
    if (owner)
        dList.push_back(constInt(Cacher::member));
    else
        dList.push_back(constInt(Cacher::global));

    // operand 2: llvm rep (gvar) or null val (instance var)
    if (gvar)
        dList.push_back(gvar->rep);
    else
        dList.push_back(Constant::getNullValue(type->rep));

    // operand 3: type name
    if (options->verbosity > 2)
        cerr << "Writing variable type " << type->getFullName() << endl;
    dList.push_back(MDString::get(getGlobalContext(), type->getFullName()));

    // operand 4: typedef owner
    if (owner)
        dList.push_back(MDString::get(getGlobalContext(),
                                      owner->getFullName()));
    else
        dList.push_back(NULL);

    // operand 5: instance var index
    if (!gvar)
        dList.push_back(constInt(ivar->index));
    else
        dList.push_back(NULL);

    // we register with the cache map because a cached module may be
    // on this depended one for this run
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, sym);

    return MDNode::get(getGlobalContext(), dList);

}

bool Cacher::readImports() {

    MDNode *mnode;
    MDString *cname, *digest, *symStr;
    SourceDigest iDigest;
    BModuleDefPtr m;
    VarDefPtr symVal;
    NamedMDNode *imports = modDef->rep->getNamedMetadata("crack_imports");

    assert(imports && "missing crack_imports node");

    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // op 1: canonical name
        cname = dyn_cast<MDString>(mnode->getOperand(0));
        assert(cname && "malformed import node: canonical name");

        // op 2: source digest
        digest = dyn_cast<MDString>(mnode->getOperand(1));
        assert(digest && "malformed import node: digest");

        iDigest = SourceDigest::fromHex(digest->getString().str());

        // load this module. if the digest doesn't match, we miss
        m = context.construct->loadModule(cname->getString().str());
        if (m->digest != iDigest)
            return false;

        // op 3..n: imported (namespace aliased) symbols from m
        for (unsigned si = 2; si < mnode->getNumOperands(); ++si) {
            symStr = dyn_cast<MDString>(mnode->getOperand(si));
            assert(symStr && "malformed import node: symbol name");
            symVal = m->lookUp(symStr->getString().str());
            // if we failed to lookup the symbol, then something is wrong
            // with our digest mechanism
            assert(symVal.get() && "import: inconsistent state");
            modDef->addAlias(symVal.get());
        }


    }

    imports = modDef->rep->getNamedMetadata("crack_shlib_imports");
    assert(imports && "missing crack_shlib_imports node");

    vector<string> symList;
    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // op 1: lib name
        cname = dyn_cast<MDString>(mnode->getOperand(0));
        assert(cname && "malformed shlib import node: lib name");

        // op 2..n: imported symbols from m
        for (unsigned si = 1; si < mnode->getNumOperands(); ++si) {
            symStr = dyn_cast<MDString>(mnode->getOperand(si));
            assert(symStr && "malformed shlib import node: symbol name");
            shlibImported[symStr->getString().str()] = true;
            symList.push_back(symStr->getString().str());
        }

        context.builder.importSharedLibrary(cname->getString().str(),
                                            symList, context, modDef.get());

    }

    return true;

}

TypeDefPtr Cacher::resolveType(const string &name) {
    TypeDefPtr td =
        TypeDefPtr::rcast(context.construct->getRegisteredDef(name));

    if (!td) {
        // is it a generic?
        int i;
        for (i = 0; i < name.size(); ++i)
            if (name[i] == '[') break;

        if (i == name.size()) {
            cerr << "unable to get type " << name << endl;
            assert(0 && "resolveType: type not found");
        }

        // resolve the basic type
        TypeDefPtr generic = resolveType(name.substr(0, i));

        // read the paramters
        TypeDef::TypeVecObjPtr parms = new TypeDef::TypeVecObj;
        while (name[i++] != ']') {
            int start = i;
            for (; name[i] != ']' && name[i] != ','; ++i);
            parms->push_back(resolveType(name.substr(start, i - start)));
        }

        return generic->getSpecialization(context, parms.get());
    }

    if (options->verbosity > 2)
        cerr << "  type " << td->name << " is " <<
            td->getFullName() << " and owner is " <<
            td->getOwner()->getNamespaceName() <<
            " original name is " << name <<
            endl;
    return td;
}

void Cacher::readVarDefGlobal(const std::string &sym,
                        llvm::Value *rep,
                        llvm::MDNode *mnode) {

    // rep for gvar is the actual global

    // operand 3: type name
    MDString *typeStr = dyn_cast<MDString>(mnode->getOperand(3));
    assert(typeStr && "readVarDefGlobal: invalid type string");

    if (options->verbosity > 2)
        cerr << "loading global " << sym << " of type " <<
            typeStr->getString().str() << endl;

    TypeDefPtr td = resolveType(typeStr->getString().str());

    GlobalVariable *lg = dyn_cast<GlobalVariable>(rep);
    assert(lg && "readVarDefGlobal: not GlobalVariable rep");
    BGlobalVarDefImpl *impl = new BGlobalVarDefImpl(lg);

    // the member def itself
    VarDef *g = new VarDef(td.get(), sym);
    g->impl = impl;
    modDef->addDef(g);

    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, g);

}

void Cacher::readConstant(const std::string &sym,
                        llvm::Value *rep,
                        llvm::MDNode *mnode) {

    VarDef *cnst;

    // operand 3: type
    MDString *typeStr = dyn_cast<MDString>(mnode->getOperand(3));
    assert(typeStr && "readConstant: invalid type string");

    TypeDefPtr td = resolveType(typeStr->getString().str());
    assert(td && "readConstant: type not found");

    if (rep) {
        ConstVarDef *cvar;
        if (rep->getType()->isIntegerTy()) {
            ConstantInt *ival = dyn_cast<ConstantInt>(rep);
            assert(ival && "not ConstantInt");
            Expr *iexpr = new BIntConst(BTypeDefPtr::arcast(td),
                                        ival->getLimitedValue());
            cvar = new ConstVarDef(td.get(), sym, iexpr);
        }
        else {
            assert(rep->getType()->isFloatingPointTy() && "not int or float");
            ConstantFP *fval = dyn_cast<ConstantFP>(rep);
            assert(fval && "not ConstantFP");
            Expr *iexpr = new BFloatConst(BTypeDefPtr::arcast(td),
                                          // XXX convertToDouble?? llvm asserts
                                         fval->getValueAPF().convertToFloat());
            cvar = new ConstVarDef(td.get(), sym, iexpr);
        }
        cnst = cvar;
    }
    else {
        // class
        cnst = new VarDef(td.get(), sym);
        cnst->constant = true;
    }

    // the member def itself
    modDef->addDef(cnst);

    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, cnst);

}

void Cacher::readVarDefMember(const std::string &sym,
                        llvm::Value *rep,
                        llvm::MDNode *mnode) {

    // rep for instance var is null val for member type
    // XXX rep unused?

    // operand 3: member type name
    MDString *typeStr = dyn_cast<MDString>(mnode->getOperand(3));
    assert(typeStr && "readVarDefMember: invalid type string");

    TypeDefPtr td = resolveType(typeStr->getString().str());
    if (!td)
        cerr << "Type " << typeStr->getString().str() <<
            " not found in module " << modDef->name << endl;
    assert(td && "readVarDefMember: type not found");

    // operand 4: type owner (class we're a member of)
    MDString *ownerStr = dyn_cast<MDString>(mnode->getOperand(4));
    assert(ownerStr && "readVarDefMember: invalid owner");

    TypeDefPtr otd = resolveType(ownerStr->getString().str());

    // operand 5: instance var index
    ConstantInt *index = dyn_cast<ConstantInt>(mnode->getOperand(5));
    assert(index && "readVarDefMember: no index");

    BInstVarDefImpl *impl = new BInstVarDefImpl(index->getLimitedValue());

    // the member def itself
    VarDefPtr mbr = new VarDef(td.get(), sym);
    mbr->impl = impl;

    otd->addDef(mbr.get());

}

BTypeDefPtr Cacher::readMetaType(MDNode *mnode) {
    // operand 3: metatype type (name string)
    MDString *cname = dyn_cast<MDString>(mnode->getOperand(3));
    assert(cname && "invalid metatype name");

    // operand 4: metatype type (null initializer)
    Value *mtrep = mnode->getOperand(4);
    BTypeDefPtr metaType = new BTypeDef(0,
                                        cname->getString().str(),
                                        mtrep->getType(),
                                        true,
                                        0 /* nextVTableslot */
                                        );
    return metaType;
}

void Cacher::finishType(TypeDef *type, BTypeDef *metaType) {
    // tie the meta-class to the class
    if (metaType)
        metaType->meta = type;

    // make the class default to initializing to null
    type->defaultInitializer = new NullConst(type);
    type->complete = true;

    modDef->addDef(type);
}

void Cacher::readTypeDef(const std::string &sym,
                         llvm::Value *rep,
                         llvm::MDNode *mnode) {

    BTypeDefPtr metaType = readMetaType(mnode);
    BTypeDefPtr type = new BTypeDef(metaType.get(),
                        sym,
                        rep->getType(),
                        true,
                        0 /* nextVTableslot */
                        );
    finishType(type.get(), metaType.get());
    assert(type->getOwner());
    context.construct->registerDef(type.get());
}

void Cacher::readGenericTypeDef(const std::string &sym,
                                llvm::Value *rep,
                                llvm::MDNode *mnode) {

    BTypeDefPtr metaType = readMetaType(mnode);
    TypeDefPtr type = new TypeDef(metaType.get(), sym, true);

    // read the generic info
    string srcString = dyn_cast<MDString>(rep)->getString();
    istringstream srcStream(srcString);
    model::Deserializer src(srcStream);
    type->genericInfo = Generic::deserialize(src);
    type->generic = new TypeDef::SpecializationCache();

    // store the module namespace in the generic info
    // XXX should also be storing the compile namespace
    type->genericInfo->ns = modDef.get();
    type->genericInfo->compileNS = new GlobalNamespace(0, "");

    finishType(type.get(), metaType.get());
    assert(type->getOwner());
    context.construct->registerDef(type.get());
}

void Cacher::readFuncDef(const std::string &sym,
                         llvm::Value *rep,
                         llvm::MDNode *mnode) {

    assert(rep && "no rep");

    // if this symbol was defined in a shared library, skip reading
    // XXX this may change depending on how exporting of second order symbols
    // works out in the cache.
    if (shlibImported.find(sym) != shlibImported.end())
        return;

    // operand 3: typedef owner (if exists)
    MDString *ownerStr(0);
    if (mnode->getOperand(3))
        ownerStr = dyn_cast<MDString>(mnode->getOperand(3));

    // operand 4: func flags
    ConstantInt *flags = dyn_cast<ConstantInt>(mnode->getOperand(4));
    assert(flags && "malformed def node: function flags");

    // operand 5: return type
    MDString *rtStr = dyn_cast<MDString>(mnode->getOperand(5));
    assert(rtStr && "malformed def node: function return type");

    // llvm function
    Function *f = dyn_cast<Function>(rep);
    assert(f && "malformed def node: llvm rep not function");

    // model funcdef
    // note we don't use FunctionBuilder here because we already have an
    // llvm rep
    size_t bargCount = f->getArgumentList().size();
    FuncDef::Flags bflags = (FuncDef::Flags)flags->getLimitedValue();
    if (bflags & FuncDef::method)
        // if method, we adjust the BFuncDef for "this", which exists in
        // llvm arguments but is only implied in BFuncDef
        bargCount--;
    BFuncDef *newF = new BFuncDef(bflags,
                                  sym,
                                  bargCount);
    newF->rep = f;

    NamespacePtr owner;
    if (ownerStr)
        owner = resolveType(ownerStr->getString().str());
    else
        owner = modDef;

    newF->setOwner(owner.get());
    newF->ns = owner;

    newF->returnType = resolveType(rtStr->getString().str());;
    if (mnode->getNumOperands() > 4) {

        MDString *aSym, *aTypeStr;
        VarDefPtr aTypeV;
        TypeDefPtr aType;

        // operand 6..arity: function parameter names and types
        for (int i = 6, ai=0; i < mnode->getNumOperands(); i+=2, ++ai) {

            aSym = dyn_cast<MDString>(mnode->getOperand(i));
            assert(aSym && "function arg: missing symbol");
            aTypeStr = dyn_cast<MDString>(mnode->getOperand(i+1));
            assert(aTypeStr && "function arg: missing type");
            aType = resolveType(aTypeStr->getString().str());
            newF->args[ai] = new ArgDef(aType.get(), aSym->getString().str());

        }
    }

    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.registerDef(context, newF);

    OverloadDef *o(0);
    VarDefPtr vd = owner->lookUp(sym);
    if (vd)
        o = OverloadDefPtr::rcast(vd);

    // at this point o may be null here if 1) vd is null 2) vd is not an
    // overloaddef. 2 can happen when a function is overriding an existing
    // definition
    if (!vd || !o) {
        o = new OverloadDef(sym);
        o->addFunc(newF);
        owner->addDef(o);
    }
    else if (o) {
        o->addFunc(newF);
    }
    else {
        assert(0 && "readFuncDef: maybe unreachable");
    }


}

void Cacher::readDefs() {

    MDNode *mnode;
    MDString *mstr;
    string sym;
    Value *rep;
    NamedMDNode *imports = modDef->rep->getNamedMetadata("crack_defs");

    assert(imports && "missing crack_defs node");

    // first pass: read all of the types
    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // operand 0: symbol name
        mstr = dyn_cast<MDString>(mnode->getOperand(0));
        assert(mstr && "malformed def node: symbol name");
        sym = mstr->getString().str();

        // operand 1: symbol type
        ConstantInt *type = dyn_cast<ConstantInt>(mnode->getOperand(1));
        assert(type && "malformed def node: symbol type");

        // operand 2: llvm rep
        rep = mnode->getOperand(2);
        //assert(rep && "malformed def node: llvm rep");

        int nodeType = type->getLimitedValue();
        if (nodeType == Cacher::type)
            readTypeDef(sym, rep, mnode);
        else if (nodeType == Cacher::generic)
            readGenericTypeDef(sym, rep, mnode);
    }

    // second pass: everything else
    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // operand 0: symbol name
        mstr = dyn_cast<MDString>(mnode->getOperand(0));
        assert(mstr && "malformed def node: symbol name");
        sym = mstr->getString().str();

        // operand 1: symbol type
        ConstantInt *type = dyn_cast<ConstantInt>(mnode->getOperand(1));
        assert(type && "malformed def node: symbol type");

        // operand 2: llvm rep
        rep = mnode->getOperand(2);
        //assert(rep && "malformed def node: llvm rep");

        switch (type->getLimitedValue()) {
        case Cacher::function:
        case Cacher::method:
            readFuncDef(sym, rep, mnode);
            break;
        case Cacher::member:
            readVarDefMember(sym, rep, mnode);
            break;
        case Cacher::global:
            readVarDefGlobal(sym, rep, mnode);
            break;
        case Cacher::type:
        case Cacher::generic:
            // should have done this in the first pass
            break;
        case Cacher::constant:
            readConstant(sym, rep, mnode);
            break;

        default:
            assert(0 && "unhandled def type");
        }

    }


}

bool Cacher::readMetadata() {

    string snode;

    // register everything in the builtin module if we haven't already
    if (!context.construct->getRegisteredDef(".builtin.int")) {

        // for some reason, we have two levels of ancestry in builtin.
        Namespace *bi = context.construct->builtinMod.get();
        while (bi) {
            for (Namespace::VarDefMap::iterator iter = bi->beginDefs();
                iter != bi->endDefs();
                ++iter
                ) {
                if (TypeDefPtr typeDef = TypeDefPtr::rcast(iter->second))
                    context.construct->registerDef(typeDef.get());
            }

            bi = bi->getParent(0).get();
        }
    }

    // first check metadata version
    snode = getNamedStringNode("crack_md_version");
    if (snode != Cacher::MD_VERSION)
        return false;

    // compare the digest stored in the bitcode against the current
    // digest of the source file on disk. if they don't match, we miss
    snode = getNamedStringNode("crack_origin_digest");
    SourceDigest bcDigest = SourceDigest::fromHex(snode);
    if (bcDigest != modDef->digest)
        return false;

    modDef->path = getNamedStringNode("crack_origin_path");

    // import list
    // if readImports returns false, then one of our dependencies has
    // changed on disk and our own cache therefore fails
    if (!readImports())
        return false;

    // var defs
    readDefs();

    // cache hit
    return true;

}

void Cacher::writeBitcode(const string &path) {

    Module *module = modDef->rep;

    std::string Err;
    unsigned OpenFlags = 0;
    OpenFlags |= raw_fd_ostream::F_Binary;

    tool_output_file *FDOut = new tool_output_file(path.c_str(),
                                                   Err,
                                                   OpenFlags);
    if (!Err.empty()) {
        cerr << Err << '\n';
        delete FDOut;
        return;
    }

    {
        formatted_raw_ostream FOS(FDOut->os());
        // llvm bitcode
        WriteBitcodeToFile(module, FOS);
    }

    // note FOS needs to destruct before we can keep
    FDOut->keep();
    delete FDOut;

}

BModuleDefPtr Cacher::maybeLoadFromCache(const string &canonicalName,
                                         const string &path) {

    string cacheFile = getCacheFilePath(options, path, "bc");
    if (cacheFile.empty())
        return NULL;

    if (options->verbosity >= 2)
        cerr << "[" << canonicalName << "] cache: maybeLoad "
             << cacheFile << endl;

    OwningPtr<MemoryBuffer> fileBuf;
    if (error_code ec = MemoryBuffer::getFile(cacheFile.c_str(), fileBuf)) {
        if (options->verbosity >= 2)
            cerr << "[" << canonicalName <<
                    "] cache: not cached or inaccessible" << endl;
        return NULL;
    }

    string errMsg;
    Module *module = getLazyBitcodeModule(fileBuf.take(),
                                          getGlobalContext(),
                                          &errMsg);
    if (!module) {
        fileBuf.reset();
        if (options->verbosity >= 1)
            cerr << "[" << canonicalName <<
                    "] cache: exists but unable to load bitcode" << endl;
        return NULL;
    }

    // if we get here, we've loaded bitcode successfully
    modDef = new BModuleDef(canonicalName, context.ns.get(), module);

    // if cache file exists, we need to get a digest for comparison
    // if it doesn't exist, we digest it when we save
    modDef->digest = SourceDigest::fromFile(path);

    if (readMetadata()) {

        // cache hit
        if (options->verbosity >= 2)
            cerr << "[" << canonicalName <<
                    "] cache materialized" << endl;
        return modDef;
    }
    else {
        // during meta data read, we determined we will miss
        delete module;
        return NULL;
    }

}

void Cacher::saveToCache() {

    assert(modDef && "empty modDef for saveToCache");

    // XXX These are the kinds of modules that get excluded by virtue of this
    // logic:
    // - internals (crack.compiler)
    // - implicit parent modules (directories containing modules)
    // - stub modules for shared libraries
    // in all cases, we should probably be caching them.
    if (modDef->path.empty() || Construct::isDir(modDef->path))
        return;
    string cacheFile = getCacheFilePath(options,
                                        modDef->path,
                                        "bc");
    if (cacheFile.empty()) {
        if (options->verbosity >= 1)
            cerr << "unable to find writable directory for cache, won't cache: "
                 << modDef->path
                 << endl;
        return;
    }

    if (options->verbosity >= 2)
        cerr << "[" << modDef->getFullName() << "] cache: saved from "
             << modDef->path
             << " to file: " << cacheFile << endl;

    // digest the source file
    modDef->digest = SourceDigest::fromFile(modDef->path);

    writeMetadata();
    writeBitcode(cacheFile);

}



