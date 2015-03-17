// Copyright 2009-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Context.h"

#include <stdlib.h>
#include <string.h>
#include <spug/StringFmt.h>
#include <fstream>
#include "builder/Builder.h"
#include "parser/Token.h"
#include "parser/Location.h"
#include "parser/ParseError.h"
#include "util/CacheFiles.h"
#include "Annotation.h"
#include "AssignExpr.h"
#include "BuilderContextData.h"
#include "CleanupFrame.h"
#include "ConstVarDef.h"
#include "ConstSequenceExpr.h"
#include "Deserializer.h"
#include "FuncAnnotation.h"
#include "StatState.h"
#include "ArgDef.h"
#include "Branchpoint.h"
#include "GlobalNamespace.h"
#include "IntConst.h"
#include "FloatConst.h"
#include "LocalNamespace.h"
#include "ModuleDef.h"
#include "NullConst.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "Serializer.h"
#include "StrConst.h"
#include "TernaryExpr.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;
using namespace crack::util;
using namespace parser;

Location Context::emptyLoc;

Construct* Context::getCompileTimeConstruct() {
    if (construct->compileTimeConstruct.get())
        return construct->compileTimeConstruct.get();
    else
        return construct;
}

void Context::showSourceLoc(const Location &loc, ostream &out) {

    // set some limits
    if (!loc.getName() || loc.getName()[0] == '\0' ||
        loc.getLineNumber() == 0 ||
        loc.getStartCol() >= 500)
        return;

    // this should be reasonably fast, as we're assuming the error is
    // thrown based on a source file already opened and cached by the os
    ifstream in(loc.getName());
    if (in.fail())
        return;
    char buf[512];
    int line = loc.getLineNumber();
    while (!in.eof() && line--) {
        in.getline(buf, 512);
        if (in.gcount() == 511)
            // we reached a really long line, bail out of trying to
            // show anything meaningful
            return;
    }
    in.close();

    out << buf << "\n";

    int c;
    for (c = 0; c < loc.getStartCol() - 1 && c < 510; c++)
        buf[c] = ' ';
    buf[c++] = '^';
    if (loc.getStartCol() != loc.getEndCol()) {
        int len = loc.getEndCol() - loc.getStartCol();
        for (int i = 0; i < len && c < 510; c++, i++)
            buf[c] = '-';
    }
    buf[c] = 0;

    out << buf << "\n";

}

void Context::warnOnHide(const string &name) {
    if (ns->lookUp(name))
        cerr << loc.getName() << ":" << loc.getLineNumber() << ": " << 
            "Symbol " << name << 
            " hides another definition in an enclosing context." << endl;
}

OverloadDefPtr Context::replicateOverload(const std::string &varName,
                                          Namespace *srcNs
                                          ) {
    OverloadDefPtr overload = new OverloadDef(varName);
    overload->type = construct->overloadType;
    
    // merge in the overloads from the parents
    overload->collectAncestors(srcNs);
    srcNs->addDef(overload.get());
    return overload;
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext,
                 Namespace *ns,
                 Namespace *compileNS
                 ) :
    loc(parentContext ? parentContext->loc : emptyLoc),
    parent(parentContext),
    ns(ns),
    compileNS(compileNS ? compileNS : 
                (parentContext ?
                  new LocalNamespace(parentContext->compileNS.get(),
                                     ns ? ns->getNamespaceName() : ""
                                     ) : 
                  NamespacePtr(0))
                 ),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    generic(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    nextFuncFlags(FuncDef::noFlags),
    nextClassFlags(TypeDef::noFlags),
    vtableOffset(0),
    construct(parentContext->construct),
    cleanupFrame(builder.createCleanupFrame(*this)) {
    assert(construct && "parent context must have a construct");
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Construct *construct,
                 Namespace *ns,
                 Namespace *compileNS
                 ) :
    ns(ns),
    compileNS(compileNS),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    generic(false),
    returnType(TypeDefPtr(0)),
    nextFuncFlags(FuncDef::noFlags),
    vtableOffset(0),
    construct(construct),
    cleanupFrame(builder.createCleanupFrame(*this)) {
    assert(compileNS);
}

Context::~Context() {}

ContextPtr Context::createSubContext(Scope newScope, Namespace *ns,
                                     const string *name,
                                     Namespace *cns
                                     ) {
    if (!ns) {
        switch (newScope) {
            case local:
                ns = new LocalNamespace(this->ns.get(), name ? *name : "");
                break;
            case module:
                ns = new GlobalNamespace(this->ns.get(), name ? *name : "");
                break;
            case composite:
            case instance:
                assert(false && 
                        "missing namespace when creating composite or instance "
                        "context"
                       );
        }
    }
    
    return new Context(builder, newScope, this, ns, cns);
}

ContextPtr Context::getClassContext() {
    if (scope == instance)
        return this;
    else if (parent)
        return parent->getClassContext();
    else
        return 0;
}

ContextPtr Context::getDefContext() {
    if (scope != composite)
        return this;
    else if (parent)
        return parent->getDefContext();
    else
        return 0;
}

ContextPtr Context::getToplevel() {
    if (toplevel)
        return this;
    else if (parent)
        return parent->getToplevel();
    else
        return 0;
}

ContextPtr Context::getModuleContext() {
    if (scope == module) {
        return this;
    } else {
        assert(parent);
        return parent->getModuleContext();
    }
}

bool Context::encloses(const Context &other) const {
    if (this == &other)
        return true;
    else if (other.parent)
        return encloses(*other.parent);
    else
        return false;
}

ModuleDefPtr Context::createModule(const string &name,
                                   const string &path,
                                   ModuleDef *owner) {
    ModuleDefPtr result = builder.createModule(*this, name, path, owner);
    ns = result;
    return result;
}

ModuleDefPtr Context::materializeModule(const string &canonicalName,
                                        ModuleDef *owner) {
    // check the cache path for module metadata.
    string metaDataPath = getCacheFilePath(builder.options.get(),
                                           *construct,
                                           canonicalName,
                                           "crkmeta"
                                           );
    
    if (!Construct::isFile(metaDataPath))
        return 0;
    
    ifstream src(metaDataPath.c_str());
    Deserializer deser(src, this);

    // try to read the module
    ModuleDefPtr result = ModuleDef::deserialize(deser, canonicalName);
    if (result)
        ns = result;
    return result;
}

void Context::cacheModule(ModuleDef *mod) {
    string metaDataPath = getCacheFilePath(builder.options.get(),
                                           *construct,
                                           mod->getNamespaceName(),
                                           "crkmeta"
                                           );
    
    // Here's how collision-safe caching works:
    // 1) We try to open the metadata file ('canonical-name.crkmeta') and 
    //    read it if successful:
    //    a) Verify that it contains the correct hash of the source 
    //       file and fail if it doesn't.
    //    b) Try to open the builder file (Builder::getCacheFile()) and fail 
    //       if we can't.
    //    c) Read the remainder of the metadata file (after the header).
    //    d) Load the builder file.
    // 2) If we fail at any point during 1):
    //    a) recompile the module.
    //    b) Write a copy of the metadata file qualified by a GUID so that no 
    //       other process could be writing or reading that copy.
    //    c) Delete the existing builder file and begin writing the builder 
    //      file also qualified by the GUID.
    //    d) Rename the metadata file so it no longer contains the GUID (other 
    //      processes can now load it in 1)
    //    e) Rename the builder file so that it no longer contains the GUID.
    // This algorithm assumes serializetion of remove/rename operations.  It 
    // should also be noted that failure in step 1.c and 1.d is catastrophic: 
    // it currently breaks the running process and requires manual 
    // intervention to repair the cache.  This is desirable for 0.10 where
    // we want to see instances of cache failure, but should probably be fixed 
    // for 1.0.
    
    // Try to construct a globally unique filename to prevent one process from 
    // reading a cachefile while another process is writing it.
    char hostname[256];
    gethostname(hostname, 256);
    hostname[255] = 0;
    pid_t pid = getpid();
    
    string uniquifier = SPUG_FSTR('.' << hostname << '.' << pid);
    string tempFileName = metaDataPath + uniquifier;
    
    try {
        ofstream dst(tempFileName.c_str());
        Serializer ser(dst);
        mod->serialize(ser);
        dst.close();
    } catch (...) {
        unlink(tempFileName.c_str());
        throw;
    }

    try {    
        builder.cacheModule(*this, mod, uniquifier);
        
        // When the builder is done, we can move the meta-file into place because 
        // it's ready to be used by another process.
        if (Construct::traceCaching)
            cerr << "Moving cached file from " << tempFileName << " to " <<
                metaDataPath << endl;
        bool metaDataMoved = move(tempFileName, metaDataPath);
        
        // Now let the builder move its cache files into place.
        builder.finishCachedModule(*this, mod, uniquifier, metaDataMoved);
        if (!metaDataMoved)
            unlink(tempFileName.c_str());
    } catch (...) {
        builder.finishCachedModule(*this, mod, uniquifier, false);
        unlink(tempFileName.c_str());
        throw;
    }
}

ExprPtr Context::getStrConst(const std::string &value, bool raw) {
    
    // look up the raw string constant
    StrConstPtr strConst;
    Construct::StrConstTable::iterator iter = 
        construct->strConstTable.find(value);
    if (iter != construct->strConstTable.end()) {
        strConst = iter->second;
    } else {
        // create a new one
        strConst = builder.createStrConst(*this, value);
        construct->strConstTable[value] = strConst;
    }
    
    // if we don't have a StaticString type yet (or the caller wants a raw
    // bytestr), we're done.
    if (raw || !construct->staticStringType)
        return strConst;
    
    // create the "new" expression for the string.
    vector<ExprPtr> args;
    args.push_back(strConst);
    args.push_back(builder.createIntConst(*this, value.size(),
                                          construct->uintType.get()
                                          )
                   );
    FuncDefPtr newFunc =
        lookUp("oper new", args, construct->staticStringType.get());
    FuncCallPtr funcCall = builder.createFuncCall(newFunc.get());
    funcCall->args = args;
    return funcCall;    
}

CleanupFramePtr Context::createCleanupFrame() {
    CleanupFramePtr frame = builder.createCleanupFrame(*this);
    frame->parent = cleanupFrame;
    cleanupFrame = frame;
    return frame;
}

void Context::closeCleanupFrame() {
    CleanupFramePtr frame = cleanupFrame;
    cleanupFrame = frame->parent;
    frame->close();
}

TypeDefPtr Context::createForwardClass(const string &name) {
    ContextPtr classContext = new Context(builder, Context::instance, this, 0);
    TypeDefPtr type = builder.createClassForward(*classContext, name);
    ns->addDef(type.get());
    return type;
}

void Context::checkForUnresolvedForwards() {
    for (Namespace::VarDefMap::iterator iter = ns->beginDefs();
         iter != ns->endDefs();
         ++iter
         ) {
        
        // check for an overload
        OverloadDef *overload;
        if (overload = OverloadDefPtr::rcast(iter->second)) {
            for (OverloadDef::FuncList::iterator fi = 
                    overload->beginTopFuncs();
                 fi != overload->endTopFuncs();
                 ++fi
                 )
                if ((*fi)->flags & FuncDef::forward)
                    error(SPUG_FSTR("Forward declared function not defined at "
                                     "the end of the block: " << 
                                     (*fi)->getDisplayName()
                                    )
                          );
        }
        
        // Check for an unresolved type.
        if (TypeDef *type = TypeDefPtr::rcast(iter->second)) {
            if (type->forward && type->getOwner() == ns)
                error(SPUG_FSTR("Forward declared type not defined at the end " 
                                 "of the block: " << type->getDisplayName()
                                )
                      );
        }
    }
}

VarDefPtr Context::emitVarDef(Context *defCtx, TypeDef *type,
                              const std::string &name,
                              Expr *initializer,
                              bool constant
                              ) {
    // make sure the type isn't void
    if (construct->voidType->matches(*type))
        error("Can not create a variable of type 'void'");

    VarDefPtr varDef;

    // if this is a constant, and the expression is a constant integer or 
    // float, create a ConstVarDef.
    if (constant && (IntConstPtr::cast(initializer) ||
                     FloatConstPtr::cast(initializer)
                     )
        ) {
        varDef = new ConstVarDef(type, name, initializer);
    } else {
        createCleanupFrame();
        varDef = type->emitVarDef(*this, name, initializer);
        varDef->constant = constant;
        closeCleanupFrame();
        cleanupFrame->addCleanup(varDef.get());
    }
    defCtx->ns->addDef(varDef.get());
    return varDef;
}

VarDefPtr Context::emitVarDef(TypeDef *type, const parser::Token &tok, 
                              Expr *initializer,
                              bool constant
                              ) {

    if (construct->migrationWarnings) {
        if (initializer && NullConstPtr::cast(initializer)) {
            cerr << loc.getName() << ":" << loc.getLineNumber() << ": " <<
                "unnecessary initialization to null" << endl;
        } else if (!initializer && type->pointer) {
            cerr << loc.getName() << ":" << loc.getLineNumber() << ": " <<
                "default initializer is now null!" << endl;
        }
    }

    // make sure we aren't using a forward declared type (we disallow this 
    // because we don't know if oper release() is defined for the type)
    if (type->forward) {
        setLocation(tok.getLocation());
        error(SPUG_FSTR("You cannot define a variable of a forward declared "
                         "type."
                        )
              );
    }
    
    // if the definition context is an instance context, make sure that we 
    // haven't generated any constructors.
    ContextPtr defCtx = getDefContext();
    if (defCtx->scope == Context::instance && 
        TypeDefPtr::arcast(defCtx->ns)->initializersEmitted) {
        throw parser::ParseError(tok.getLocation(),
                                 "Adding an instance variable "
                                  "after 'oper init' has been "
                                  "defined."
                                 );
    }

    setLocation(tok.getLocation());
    return emitVarDef(defCtx.get(), type, tok.getData(), initializer, constant);
}

ExprPtr Context::createTernary(Expr *cond, Expr *trueVal, Expr *falseVal) {
    // make sure the condition can be converted to bool
    ExprPtr boolCond = cond->convert(*this, construct->boolType.get());
    if (!boolCond)
        error("Condition in ternary operator is not boolean.");

    ExprPtr converted;

    // make sure the types are compatible
    TypeDefPtr type;
    if (falseVal && trueVal->type != falseVal->type) {
        if (trueVal->type->isDerivedFrom(falseVal->type.get())) {
            type = falseVal->type;
        } else if (falseVal->type->isDerivedFrom(trueVal->type.get())) {
            type = trueVal->type;
        } else if (converted = falseVal->convert(*this, trueVal->type.get())) {
            type = trueVal->type;
            falseVal = converted.get();
        } else if (converted = trueVal->convert(*this, falseVal->type.get())) {
            type = falseVal->type;
            trueVal = converted.get();
        } else {
            error("Value types in ternary operator are not compatible.");
        }
    } else {
        // the types are equal
        type = trueVal->type;
    }
    
    return builder.createTernary(*this, boolCond.get(), trueVal, falseVal, 
                                 type.get()
                                 );
}

ExprPtr Context::emitConstSequence(TypeDef *type, 
                                   const std::vector<ExprPtr> &elems
                                   ) {
    // see if there is a new function for the type that accepts an element 
    // count.
    vector<ExprPtr> consArgs(1);
    consArgs[0] = builder.createIntConst(*this, elems.size());
    FuncDefPtr cons = type->getOperNew(*this, consArgs);
    if (!cons) {
        consArgs.clear();
        cons = type->getOperNew(*this, consArgs);
    }

    if (!cons)
        error(SPUG_FSTR(type->name << " has neither a constructor "
                         "accepting a uint nor a default constructor."
                        )
              );

    ConstSequenceExprPtr expr = new ConstSequenceExpr(type);
    expr->container = builder.createFuncCall(cons.get());
    expr->container->args = consArgs;
    
    // create append calls for each of the elements
    for (int i = 0; i < elems.size(); ++i) {
        ExprPtr elem = elems[i];
        
        vector<ExprPtr> args(1);
        args[0] = elem;
        FuncDefPtr appender = lookUp("append", args, type);
        
        FuncCallPtr appendCall;
        if (appender && appender->flags & FuncDef::method) {
            appendCall = builder.createFuncCall(appender.get());
            appendCall->args = args;

        // if there's no appender, try looking up index assignment
        } else {
            TypeDef *uintType = construct->uintType.get();
            args.insert(args.begin(),
                        builder.createIntConst(*this, i, uintType)
                        );
            
            appender = lookUp("oper []=", args, type);
            if (!appender || !(appender->flags & FuncDef::method))
                error(SPUG_FSTR(type->name << 
                                 " has neither an append() nor an oper []= "
                                 " method accepting " << elem->type->name <<
                                 " for element at index " << i
                                )
                      );
            
            appendCall = builder.createFuncCall(appender.get());
            appendCall->args = args;
        }
        
        // add the append to the sequence expression
        expr->elems.push_back(appendCall);
    }
    
    return expr;
}

ModuleDefPtr Context::emitImport(Namespace *ns, 
                                 const std::vector<string> &moduleName,
                                 const ImportedDefVec &imports,
                                 bool annotation,
                                 bool recordImport,
                                 bool rawSharedLib,
                                 vector<Location> *symLocs
                                 ) {
    string canonicalName;
    
    // The raw shared library case is simple, deal with it up front.
    if (rawSharedLib) {
        try {
            StatState s1(this, ConstructStats::builder);
            builder.importSharedLibrary(moduleName[0], imports, *this, ns);
            return 0;
        } catch (const spug::Exception &ex) {
            error(ex.getMessage());
        }
    }
    
    ModuleDefPtr mod = construct->getModule(moduleName.begin(), 
                                            moduleName.end(),
                                            canonicalName
                                            );
    if (!mod)
        error(SPUG_FSTR("unable to find module " << canonicalName));
    
    // make sure the module is finished (no recursive imports)
    else if (!mod->finished)
        error(SPUG_FSTR("Attempting to import module " << canonicalName <<
                         " recursively."
                        )
              );
    
    // add an implicit dependency to the current module (assuming we're 
    // currently in a module, in an annotation we might not be).
    if (ModuleDef *curMod = 
          ModuleDefPtr::rcast(getModuleContext()->ns)
        )
        curMod->imports.push_back(mod);

    {
        StatState s1(this, ConstructStats::builder);
        if (!annotation)
            builder.initializeImport(mod.get(), imports);
    }

    // alias all of the names in the new module
    int st = 0;
    ModuleDefPtr curModule = ns->getModule();
    for (ImportedDefVec::const_iterator iter = imports.begin();
         iter != imports.end();
         ++iter, ++st
         ) {
        // make sure that the symbol is not private
        if (iter->source[0] == '_')
            error(symLocs ? (*symLocs)[st] : loc,
                  SPUG_FSTR("Can not import private symbol " << iter->source << 
                             "."
                            )
                  );
        
        // make sure we don't already have it in the local context
        if (ns->lookUp(iter->local, false))
            error(symLocs ? (*symLocs)[st] : loc, SPUG_FSTR("imported name " << iter->local <<
                                  " hides existing definition."
                                 )
                  );
        VarDefPtr symVal = mod->lookUp(iter->source);
        if (!symVal)
            error(symLocs ? (*symLocs)[st] : loc, 
                  SPUG_FSTR("name " << iter->source <<
                             " is not defined in module " << 
                             canonicalName
                            )
                  );
        
        // make sure the symbol either belongs to the module or was 
        // explicitly exported by the module (no implicit second-order 
        // imports).
        if (!symVal->isImportableFrom(mod.get(), iter->source))
            error(symLocs ? (*symLocs)[st] : loc, 
                  SPUG_FSTR("Name " << iter->source <<
                             " does not belong to module " <<
                             canonicalName << ".  Second-order imports " 
                             "are not allowed."
                            )
                  );
        
        {
            StatState s1(this, ConstructStats::builder);
            builder.registerImportedDef(*this, symVal.get());
        }
        
        ns->addAlias(iter->local, symVal.get());
        if (curModule) {
            VarDef::Set added;
            symVal->addDependenciesTo(curModule.get(), added);
        }
    }
    
    // add a dependency on the module itself.  We have to check that 
    // curModule is not null when we do this, it can be null in the case of 
    // an annotation import.
    if (curModule)
        curModule->addDependency(mod.get());
   
    // If we're recording imports, track the import in the parent context.  
    // (the current context is a temporary annotation context in this case 
    // where we do this)
    if (annotation && recordImport)
        parent->compileNSImports.push_back(new Import(moduleName, imports));
    
    return mod;
}

bool Context::inSameFunc(Namespace *varNS) {
    if (scope != local)
        // this is not a function.
        return false;

    // see if the namespace is our namespace
    if (varNS == ns)
        return true;
        
    // if this isn't the toplevel context, check the parent
    else if (!toplevel)
        return parent->inSameFunc(varNS);
    else
        return false;
}
    

ExprPtr Context::createVarRef(VarDef *varDef) {

    // is the variable a constant?
    ConstVarDefPtr constDef;
    if (constDef = ConstVarDefPtr::cast(varDef))
        return constDef->expr;

    // verify that the variable is reachable
    
    // functions and types don't have reachability issues
    if (TypeDefPtr::cast(varDef) || FuncDefPtr::cast(varDef)) {
        return builder.createVarRef(varDef);
    
    // if the variable is in a module context, it is accessible
    } else if (ModuleDefPtr::cast(varDef->getOwner()) ||
               GlobalNamespacePtr::cast(varDef->getOwner())
               ) {
        return builder.createVarRef(varDef);
    
    // if it's in an instance context, verify that this is either the composite
    // context of the class or a method of the class that contains the variable.
    } else if (TypeDefPtr::cast(varDef->getOwner())) {
        if (scope == composite && varDef->getOwner() == parent->ns ||
            getToplevel()->parent->ns
            ) {
            return builder.createVarRef(varDef);
        }
    
    // if it's in a function context, make sure it's this function
    } else if (inSameFunc(varDef->getOwner())) {
        return builder.createVarRef(varDef);
    }
    
    error(SPUG_FSTR("Variable '" << varDef->name << 
                     "' is not accessible from within this context."
                    )
          );
}

VarRefPtr Context::createFieldRef(Expr *aggregate, VarDef *var) {
    TypeDef *aggType = aggregate->type.get();
    TypeDef *varNS = TypeDefPtr::cast(var->getOwner());
    if (!varNS || !aggType->isDerivedFrom(varNS))
        error(SPUG_FSTR("Variable '" << var->name <<
                         "' is not accessible from within this context."
                        )
              );
    
    return builder.createFieldRef(aggregate, var);
}

void Context::setBreak(Branchpoint *branch) {
    breakBranch = branch;
}

void Context::setContinue(Branchpoint *branch) {
    continueBranch = branch;
}

void Context::setCatchBranchpoint(Branchpoint *branch) {
    catchBranch = branch;
}

Branchpoint *Context::getBreak() {
    if (breakBranch)
        return breakBranch.get();
    
    // don't attempt to propagate out of an execution scope
    if (!toplevel && parent) {
        return parent->getBreak();
    } else {
        return 0;
    }
}

Branchpoint *Context::getContinue() {
    if (continueBranch)
        return continueBranch.get();

    // don't attempt to propagate out of an execution scope
    if (!toplevel && parent) {
        return parent->getContinue();
    } else {
        return 0;
    }
}

ContextPtr Context::getCatch() {
    if (catchBranch)
        return this;
    else if (toplevel)
        return this;
    else if (parent)
        return parent->getCatch();
}

BranchpointPtr Context::getCatchBranchpoint() {
    return catchBranch;
}

ExprPtr Context::makeThisRef(const string &memberName) {
   VarDefPtr thisVar = ns->lookUp("this");
   if (!thisVar)
      error(SPUG_FSTR("Instance member \"" << memberName <<
                       "\" may not be used in a static context."
                      )
            );
          
   return createVarRef(thisVar.get());
}

bool Context::hasInstanceOf(TypeDef *type) const {
    VarDefPtr thisVar = ns->lookUp("this");
    return thisVar && thisVar->type->isDerivedFrom(type);
}

void Context::expandIteration(const std::string &name, bool defineVar,
                              bool isIter,
                              Expr *seqExpr,
                              ExprPtr &cond,
                              ExprPtr &beforeBody,
                              ExprPtr &afterBody
                              ) {
    // verify that the sequence has an "iter" method
    FuncDefPtr iterFunc = lookUpNoArgs("iter", true, seqExpr->type.get());
    if (!iterFunc)
        error(SPUG_FSTR("iteration expression has no 'iter' method "
                        "(was type " << seqExpr->type->getFullName() << ")")
                        );
    
    // create an expression from it
    FuncCallPtr iterCall = builder.createFuncCall(iterFunc.get());
    if (iterFunc->flags & FuncDef::method)
        iterCall->receiver = seqExpr;
    
    // if the variable provided is an iterator, just use it and clear the 
    // "var" argument as an indicator to later code.
    VarDefPtr iterVar, var;
    FuncDefPtr elemFunc;
    if (isIter) {
        // this is a "for on" and the variable is an iterator.
        if (defineVar) {
            assert(scope != composite && 
                    "iteration expanded in a non-definition context"
                );
            warnOnHide(name);
            iterVar = 
                emitVarDef(this, iterCall->type.get(), name, iterCall.get());
        } else {
            iterVar = ns->lookUp(name);
            if (iterVar->isConstant())
                error("Cannot use a constant as a loop iterator");
            if (!iterVar->type->matches(*iterCall->type))
                error("Loop iterator variable type does not match the type of "
                       "the iterator."
                      );

            // emit code to assign the iterator
            createCleanupFrame();
            ExprPtr iterAssign =
                AssignExpr::create(*this, iterVar.get(), iterCall.get());
            iterAssign->emit(*this)->handleTransient(*this);
            closeCleanupFrame();
        }
    } else {
        // we're passing in "this" and assuming that the current context is a 
        // definition context.
        assert(scope != composite && 
                "iteration expanded in a non-definition context"
               );
        iterVar = emitVarDef(this, iterCall->type.get(), ":iter", 
                             iterCall.get()
                             );

        elemFunc = lookUpNoArgs("elem", true, iterCall->type.get());
        if (!elemFunc)
            error(SPUG_FSTR("Iterator type " << 
                            iterCall->type->getDisplayName() <<
                            " does not have an 'elem()' method."
                            )
                  );
                            
        
        if (defineVar) {
            warnOnHide(name);
            
            // if the element type doesn't have a default initializer, we need 
            // to create a null identifier for it so we don't seg-fault.
            ExprPtr initializer;
            if (!elemFunc->returnType->defaultInitializer)
                initializer = new NullConst(elemFunc->returnType.get());

            var = emitVarDef(this, elemFunc->returnType.get(), name, 
                             initializer.get()
                             );
        } else {
            var = ns->lookUp(name);
            if (var->isConstant())
                error("Cannot use a constant as a loop variable");
            if (!var->type->matches(*elemFunc->returnType))
                error("Loop variable type does not match the type of the "
                       "return value of the iterator's elem() method."
                      );
        }
    }
    
    // create a reference expression for the iterator
    ExprPtr iterRef = createVarRef(iterVar.get());
    
    if (var) {
        // assign the variable before the body
        FuncCallPtr elemCall = builder.createFuncCall(elemFunc.get());
        if (elemFunc->flags & FuncDef::method)
            elemCall->receiver = iterRef;
        beforeBody = AssignExpr::create(*this, var.get(), elemCall.get());
    }
    
    // convert it to a boolean for the condition
    cond = iterRef->convert(*this, construct->boolType.get());
    if (!cond)
        error("The iterator in a 'for' loop must convert to boolean.");
    
    // create the "iter.next()" expression
    FuncDefPtr nextFunc = lookUpNoArgs("next", true, iterRef->type.get());
    if (!nextFunc)
        error("The iterator in a 'for' loop must provide a 'next()' method");
    FuncCallPtr nextCall = builder.createFuncCall(nextFunc.get());
    if (nextFunc->flags & FuncDef::method)
        nextCall->receiver = iterRef;
    afterBody = nextCall;
}

VarDefPtr Context::lookUp(const std::string &varName, Namespace *srcNs) {
    if (!srcNs)
        srcNs = ns.get();
    VarDefPtr def = srcNs->lookUp(varName);

    // if we got an overload, we may need to create an overload in this
    // context.  (we can get away with checking the owner because overloads 
    // are never aliased)
    OverloadDef *overload = OverloadDefPtr::rcast(def);
    if (overload && overload->getOwner() != srcNs)
        return replicateOverload(varName, srcNs);
    else
        return def;
}

void Context::maybeExplainOverload(std::ostream &out,
                           const std::string &varName,
                           Namespace *srcNs) {

    if (!srcNs)
        srcNs = ns.get();

    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName, srcNs);
    if (!var)
        return;

    // if it's an overload...
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return;

    // explain the overload
    out << "\nPossible overloads for " << varName << ":\n";
    overload->display(out);

}

FuncDefPtr Context::lookUp(const std::string &varName,
                           vector<ExprPtr> &args,
                           Namespace *srcNs,
                           bool allowOverrides
                           ) {
    if (!srcNs)
        srcNs = ns.get();

    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName, srcNs);
    if (!var)
        return 0;
    
    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef) {
        FuncDefPtr operNew = lookUp("oper new", args, typeDef);

        // make sure we got it, and we didn't inherit it
        if (!operNew || operNew->getOwner() != typeDef)
            return 0;
        
        return operNew;
    }
    
    // make sure we got an overload
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return 0;
    
    // look up the signature in the overload
    return overload->getMatch(*this, args, allowOverrides);
}

FuncDefPtr Context::lookUpNoArgs(const std::string &name, bool acceptAlias,
                                 Namespace *srcNs
                                 ) {
    OverloadDefPtr overload = OverloadDefPtr::rcast(lookUp(name, srcNs));
    if (!overload)
        return 0;

    // we can just check for a signature match here - cheaper and easier.
    ArgVec args;
    FuncDefPtr result = overload->getNoArgMatch(acceptAlias);
    return result;
}

VarDefPtr Context::addDef(VarDef *varDef, Namespace *srcNs) {
    if (!srcNs)
        srcNs = ns.get();
    FuncDef *funcDef = FuncDefPtr::cast(varDef);
    if (funcDef) {
        OverloadDefPtr overload = 
            OverloadDefPtr::rcast(lookUp(varDef->name, srcNs));
        if (!overload)
            overload = replicateOverload(varDef->name, srcNs);
        overload->addFunc(funcDef);
        funcDef->setOwner(srcNs);
        return overload;
    } else {
        srcNs->addDef(varDef);
        return varDef;
    }
}

void Context::insureOverloadPath(Context *ancestor, OverloadDef *overload) {
    // see if we define the overload, if not we're done.
    OverloadDefPtr localOverload =
        OverloadDefPtr::rcast(ns->lookUp(overload->name, false));
    if (!localOverload)
        return;

    // this code assumes that 'ancestor' must always be a direct ancestor of 
    // our namespace.  This isn't strictly essential, but it shouldn't be 
    // necessary to support the general case.  If we change this assumption, 
    // we'll get an assertion failure to warn us.

    // see if the overload is one of our overload's parents, if so we're done.
    if (localOverload->hasParent(overload))
        return;

    // verify that we are directly derived from the namespace.
    NamespacePtr parentNs;
    for (int i = 0; parentNs = ns->getParent(i++);)
        if (parent.get() == ancestor)
            break;
    assert(parent && "insureOverloadPath(): parent is not a direct parent.");
    
    localOverload->addParent(overload);
}

AnnotationPtr Context::lookUpAnnotation(const std::string &name) {
    VarDefPtr result = compileNS->lookUp(name);
    if (!result)
        return 0;

    // if we've already got an annotation, we're done.
    AnnotationPtr ann;
    if (ann = AnnotationPtr::rcast(result))
        return ann;

    // create the arg list for the signature of an annotation (we don't need 
    // the builder to create an ArgDef here because it's just for a signature 
    // match).
    ArgVec args(1);
    args[0] = new ArgDef(construct->crackContext.get(), "context");

    OverloadDef *ovld = OverloadDefPtr::rcast(result);
    if (ovld) {
        FuncDefPtr f = ovld->getSigMatch(args);
        if (!f)
            return 0;
        ann = new FuncAnnotation(f.get());
        compileNS->addDef(ann.get());
        return ann;
    }
    
    // XXX can't deal with variables yet
    return 0;
}

void Context::collectCompileNSImports(vector<ImportPtr> &imports) const {
    if (parent)
        parent->collectCompileNSImports(imports);
        
    for (vector<ImportPtr>::const_iterator i = compileNSImports.begin();
         i != compileNSImports.end();
         ++i
         )
        imports.push_back(*i);
}

namespace {
    struct ContextStack {
        const list<string> &stack;
        ContextStack(const list<string> &stack) : stack(stack) {}
    };

    ostream &operator <<(ostream &out, ContextStack a) {
        for (list<string>::const_iterator iter = a.stack.begin(); 
             iter != a.stack.end();
             ++iter
             )
            out << "\n  " << *iter;
        return out;
    }
}

void Context::error(const Location &loc, const string &msg, 
                    bool throwException
                    ) {
    
    list<string> &ec = construct->errorContexts;
    if (throwException) {
        stringstream diag;
        if (!construct->rootBuilder->options->quiet) {
            showSourceLoc(loc, diag);
        }
        throw parser::ParseError(loc,
                                 SPUG_FSTR(msg <<
                                            ContextStack(ec) <<
                                            endl <<
                                            diag.str()
                                           )
                                 );
    }
    else {
        cerr << "ParseError: " << loc.getName() << ":" <<
            loc.getLineNumber() << ":" << loc.getColNumber() << ": " << msg <<
            ContextStack(ec) << endl;
        exit(1);
    }
    
}

void Context::warn(const Location &loc, const string &msg) {
    cerr << loc.getName() << ":" <<
            loc.getLineNumber() << ":" <<
            loc.getColNumber() << ": " << msg << endl;
}

void Context::pushErrorContext(const string &msg) {
    construct->errorContexts.push_front(msg);
}

void Context::popErrorContext() {
    construct->errorContexts.pop_front();
}

void Context::checkAccessible(VarDef *var, const string &name) {
    size_t nameSize = name.size();
    if (nameSize && name[0] == '_') {

        // See if the variable is aliased in the current module.  This is 
        // to accomodate generic arguments, which can alias private types.
        if (getModuleContext()->ns->hasAliasFor(var))
            return;

        if (nameSize > 1 && name[1] == '_') {

            // private variable: if it is owned by a class, we must be 
            // in the scope of that class.
            TypeDef *varClass = TypeDefPtr::cast(var->getOwner());
            if (!varClass)
                return;
            ContextPtr myClassCtx = getClassContext();
            if (!myClassCtx || TypeDefPtr::rcast(myClassCtx->ns) != varClass)
                error(SPUG_FSTR(name << " is private to class " <<
                                 varClass->name << " and not acessible in this "
                                 "context."
                                )
                      );
            
        } else {

            // module protected variable: no problem if part of the same 
            // module (use the owner module if there is one)
            ModuleDefPtr varMod = var->getOwner()->getRealModule();
            ModuleDefPtr curMod = getModuleContext()->ns->getRealModule();
            if (varMod.get() == curMod.get())
                return;

            // see if this class is derived from the variable's class
            ContextPtr myClassCtx = getClassContext();
            TypeDef *varClass = TypeDefPtr::cast(var->getOwner());
            if (varClass && myClassCtx && 
                TypeDefPtr::rcast(myClassCtx->ns)->isDerivedFrom(varClass)
                )
                return;
            
            // the symbol is from a different module and we are not in a 
            // derived class.
            error(SPUG_FSTR(name << " is private to module " <<
                             varMod->getNamespaceName() << 
                             " and not accessible in this context."
                            )
                  );
        }
    }
}

void Context::recordDependency(ModuleDef *module) {
    // indicate that our module depends on this other one.
    ContextPtr modCtx = getModuleContext();
    ModuleDefPtr::arcast(modCtx->ns->getModule())->recordDependency(module);
}

void Context::dump(ostream &out, const std::string &prefix) const {
    switch (scope) {
        case module: out << "module "; break;
        case instance: out << "instance "; break;
        case local: out << "local "; break;
        case composite: out << "composite "; break;
        default: out << "UNKNOWN ";
    }
    ns->dump(out, prefix);
}

void Context::dump() {
    dump(cerr, "");
}
