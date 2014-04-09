// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2013 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "StructResolver.h"

#include <assert.h>
#include <ctype.h>

#include <vector>
#include <iostream>

#include <llvm/ADT/StringMap.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Constants.h>
#include <llvm/Analysis/FindUsedTypes.h>

#include "spug/check.h"
#include "spug/stlutil.h"
#include "spug/StringFmt.h"
#include "spug/Tracer.h"
#include "LLVMBuilder.h"

using namespace llvm;
using namespace std;
using namespace builder::mvll;

#define SR_DEBUG if (trace)
bool StructResolver::trace = false;

namespace {

    spug::Tracer tracer("StructResolver", StructResolver::trace,
                        "LLVM module struct type normalization."
                        );

    const char *tName(int t) {
        if (t == 11)
            return "Structure";
        else if (t == 12)
            return "Array";
        else if (t == 13)
            return "Pointer";
        else
            return "-Other-";
    }
}

StructResolver::StructListType StructResolver::buildTypeMap() {

    StructListType result;
    SetVector<Type *> usedTypes;

    FindUsedTypes findTypes;
    findTypes.runOnModule(*module);
    usedTypes = findTypes.getTypes();

    // NOTE this gets a list by struct name only: no isomorphism checks

    for (SetVector<Type *>::iterator i = usedTypes.begin(); 
         i != usedTypes.end(); ++i
         ) {
        if (!(*i)->isStructTy())
            continue;

        StructType *structTy = static_cast<StructType*>(*i);

        // does it have a name?
        if (!structTy->hasName())
            continue;

        string name = structTy->getName().str();

        // does it have a numeric suffix? (i.e. is it a duplicate type name?)
        int pos = name.rfind(".");
        bool isDuplicate = false;
        if (pos != string::npos && name.size() > pos) {
            string suffix = name.substr(pos + 1);
            for (string::const_iterator si = suffix.begin();
                 si != suffix.end();
                 ++si) {
                isDuplicate = isdigit(*si);
                if (!isDuplicate)
                    break;
            }
        }
        
        if (!isDuplicate)
            continue;
        
        string canonicalName = name.substr(0, pos);

        SR_DEBUG cerr << "Mapping type " << structTy->getName().str() << 
            " to " << canonicalName << endl;

        // The numeric suffix is only applied when we discover a new type with 
        // the same name.  Therefore, the fact that there is a numeric suffix 
        // implies that we should have already loaded the type.
        StructType *type = LLVMBuilder::getLLVMType(canonicalName);
        
        // since we're now deferring type resolution until after the linker, 
        // we can get into a situation where multiple modules load the type 
        // before it is registered.  Most likely this will cause problems with 
        // collapsing isomorphic types, too.
        if (!type) {
            SR_DEBUG cerr << "Unregistered duplicate type found for " <<
                canonicalName << endl;
            typeMap[structTy] = structTy;
            LLVMBuilder::putLLVMType(canonicalName, structTy);
            structTy->setName(canonicalName);
            continue;
        } else if (type->getName() != canonicalName) {
            // This was to help me debug something.
            SR_DEBUG cerr << "restoring missing name for original type " <<
                canonicalName << endl;
            type->setName(canonicalName);
            if (type == structTy)
                continue;
        }

        StructType *left = structTy;
        assert(left);
        StructType *right = type;
        SPUG_CHECK(left != right, 
                   "Mapping type " << canonicalName << " to itself");
        //cout << "struct map [" << left->getName().str() << "] to [" << right->getName().str() << "]\n";
        typeMap[left] = right;
    }

    return result;

}

void StructResolver::run() {
    SR_DEBUG cerr << ">>>> Running struct resolutiopn on " << 
        module->getModuleIdentifier() << endl;

    if (typeMap.empty())
        return;

    module->MaterializeAll();

    for (StructMapType::iterator iter = typeMap.begin(); 
         iter != typeMap.end(); 
         ++iter
         )
        reverseMap[iter->second] = iter->first;
    mapGlobals();
    mapFunctions();
    mapMetadata();
    
    // all of the types should now be righteous.  Register any unknown types 
    // with LLVM.
    FindUsedTypes typeFinder;
    typeFinder.runOnModule(*module);
    SetVector<Type *> usedTypes = typeFinder.getTypes();
    for (SetVector<Type *>::iterator iter = usedTypes.begin();
         iter != usedTypes.end();
         ++iter
         ) {
        if (!(*iter)->isStructTy())
            continue;
        StructType *structType = cast<StructType>(*iter);

        if (structType->hasName() && 
            !LLVMBuilder::getLLVMType(structType->getName().str())
            )
            LLVMBuilder::putLLVMType(structType->getName().str(), structType);
    }
    SR_DEBUG module->dump();
}

void StructResolver::restoreOriginalTypes() {
    SPUG_FOR(ValueTypeMap, i, originalTypes)
        i->first->mutateType(i->second);
}

namespace {
    template <typename T>
    string llvmStr(const T &obj) {
        string temp;
        raw_string_ostream out(temp);
        obj.print(out);
//        out << obj;
        return out.str();
    }
}

bool StructResolver::buildElementVec(StructType *type, vector<Type *> &elems) {
    // element iterate on types
    bool modified = false;
    for (StructType::element_iterator e = type->element_begin();
         e != type->element_end();
         ++e
         ) {

        // accumulate the types. if we find one we have to map, we'll use
        // the accumlated types to create a new structure with the mapped
        // type. if it doesn't contain one, we discard it
        Type *p = maybeGetMappedType(*e);
        if (p != *e) {
            modified = true;
            elems.push_back(p);
        } else {
            elems.push_back(*e);
        }
    }
    
    return modified;
}

Type *StructResolver::maybeGetMappedType(Type *t) {

    if (typeMap.find(t) != typeMap.end()) {
        SR_DEBUG cerr << "\t\t## --- FOUND EXISTING MAPPING --- ##\n";
        SR_DEBUG cerr << "was:\n";
        SR_DEBUG t->dump();
        SR_DEBUG cerr << "\nnow:\n";
        SR_DEBUG typeMap[t]->dump();
        SR_DEBUG cerr << "\n";
        return typeMap[t];
    } else if (reverseMap.find(t) != reverseMap.end()) {
        SR_DEBUG cerr << "\t\t## --- PREMAPPED --- ##\n";
        SR_DEBUG t->dump();
        SR_DEBUG cerr << "\n";
        return t;
    } else if (visitedStructs.find(t) != visitedStructs.end()) {
        SR_DEBUG cerr << "\t\t## -- VISITED STRUCT --- ##\n";
        SR_DEBUG t->dump();
        SR_DEBUG cerr << "\n";
        return t;
    }

    if (isa<FunctionType>(t)) {
        FunctionType *funcType = dyn_cast<FunctionType>(t);
        SR_DEBUG cerr << "\t\t## func type: " << llvmStr(*funcType) << endl;
        bool remap = false;
        Type *retType = maybeGetMappedType(funcType->getReturnType());
        if (retType != funcType->getReturnType())
            remap = true;
        
        vector<Type *> argTypes;
        argTypes.reserve(funcType->getNumParams());
        for (FunctionType::param_iterator pi = funcType->param_begin();
             pi != funcType->param_end();
             ++pi
             ) {
            Type *argType = maybeGetMappedType(*pi);
            argTypes.push_back(argType);
            remap = remap || argType != *pi;
        }
        
        if (remap) {
            SR_DEBUG cerr << "\t\treplacing func type: " << 
                llvmStr(*funcType) << endl;
            Type *m = FunctionType::get(retType, argTypes, 
                                        funcType->isVarArg()
                                        );
            typeMap[t] = m;
            return maybeGetMappedType(t);
        }
    }

    // short cut if not composite
    if (!isa<CompositeType>(t))
        return t;

    if (isa<PointerType>(t)) {
        PointerType *a = dyn_cast<PointerType>(t);
        SR_DEBUG cerr << "\t\t## pointer, points to type: " << 
                         tName(a->getElementType()->getTypeID()) << "\n";
        Type *p = maybeGetMappedType(a->getElementType());
        // p will be the end of a pointer chain
        if (p != a->getElementType()) {
            // it's one we are mapping, so map and recurse back up
            Type *m = PointerType::get(p, a->getAddressSpace());
            typeMap[t] = m;
            //return m;
            return maybeGetMappedType(t);
        }
    }
    else if (isa<ArrayType>(t)) {
        ArrayType *a = dyn_cast<ArrayType>(t);
        SR_DEBUG cerr << "\t\t## array of type: " << 
            tName(a->getElementType()->getTypeID()) << "\n";
        Type *p = maybeGetMappedType(a->getElementType());
        if (p != a->getElementType()) {
            // it's one we are mapping, so map and recurse back up
            Type *m = ArrayType::get(p, a->getNumElements());
            typeMap[t] = m;
            //return m;
            return maybeGetMappedType(t);
        }
    }
    else if (isa<StructType>(t)) {
        StructType *origType = dyn_cast<StructType>(t);
        visitedStructs[t] = 0;
        SR_DEBUG cerr << "\t\t## struct\n";
        if (origType->hasName()) {
            SR_DEBUG cerr << "\t\t## has name: " << 
                origType->getName().str() << "\n";
        }

        // build the element vector.
        vector<Type *> elems;
        bool modified = buildElementVec(origType, elems);
        
        int cycleCount = visitedStructs[t];
        visitedStructs.erase(t);
        if (modified) {
            // check to see if we have mapped the type while mapping the 
            // elements.
            StructMapType::iterator nestedMapping = typeMap.find(t);
            if (nestedMapping != typeMap.end()) {
                SR_DEBUG cerr << "\t\t## reusing mapping for " <<
                    (origType->hasName() ? origType->getName().str() : "") <<
                    endl;
                return nestedMapping->second;
            }
        
            StructType *newType = StructType::create(getGlobalContext());
            typeMap[t] = newType;
            reverseMap[newType] = t;
            
            // if the type contains any cycles, add a placeholder for it and 
            // recalculate the member types.
            if (visitedStructs[t])
                buildElementVec(origType, elems);

            SR_DEBUG cerr << "\t\t## replacing struct " << 
                (origType->hasName() ? origType->getName().str() : 
                 string("<noname>")
                 )
                << endl;

            newType->setBody(elems);
            if (!origType->isLiteral()) {
                // take over the name
                string origName = origType->getName().str();
                origType->setName(SPUG_FSTR(origName << "." << origType));
                newType->setName(origName);
                LLVMBuilder::putLLVMType(origName, newType);
            }

            return newType;
        }
    }

    // unchanged
    return t;

}

void StructResolver::mapValue(Value &val) {

    // we only care about compsite types
    /*
    if (!isa<CompositeType>(val.getType())) {
        cerr << "\t@@ skipping non composite type\n";
        return;
    }*/

    SR_DEBUG cerr << "@@ mapValue ["<<&val<<"], before\n";
    SR_DEBUG val.dump();

    if (visited.find(&val) != visited.end()) {
        SR_DEBUG cerr << "\t@@ already seen\n";
        return;
    }

    Type *t = maybeGetMappedType(val.getType());
    if (t != val.getType()) {
        if (isa<Constant>(val) && !spug::contains(originalTypes, &val))
            originalTypes[&val] = val.getType();
        val.mutateType(t);
    }
    
    if (isa<Function>(val)) {
        SR_DEBUG cerr << "\t@@ function type\n";
        SR_DEBUG val.dump();
    }

    visited[&val] = true;

    SR_DEBUG cerr << "@@ mapValue ["<<&val<<"], after\n";
    SR_DEBUG val.dump();

}

// User is a Value and may have a list of Value operands
void StructResolver::mapUser(User &val) {

    SR_DEBUG cerr << "#mapUser, before\n";
    //val.dump();

    if (visited.find(&val) != visited.end()) {
        SR_DEBUG cerr << "\t@@ already seen\n";
        return;
    }

    SR_DEBUG cerr << "#value itself:\n";
    mapValue(val);

    if (val.getNumOperands()) {
        int opNum = 1;
        for (User::op_iterator o = val.op_begin();
             o != val.op_end();
             ++o) {
            // o iterates through Use, which is essentially a wrapper for Value
            Value *op = *o;
            if (isa<CompositeType>(op->getType())) {
                /*if ((*o)->hasName())
                    cerr << "#op named: " << (*o)->getValueName()->getKey().str() << "\n";
                else
                    cerr << "#op #" << opNum++ << "\n";*/
                if (isa<User>(op))
                    mapUser(cast<User>(*op));
                else
                    mapValue(*op);
            }
        }
    }

    SR_DEBUG cerr << "#mapUser, after\n";
    //val.dump();

}

void StructResolver::mapGlobals() {

    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i) {
        SR_DEBUG cerr << "-----] looking at global: " << i->getName().str() << 
            endl;
        mapUser(*i);
    }
    //module->dump();

}

void StructResolver::mapFunction(Function &fun) {

    // get a FunctionType based on running the existing one through
    // our mapper. since llvm uniques them, if it's unchanged, we should have
    // the same FunctionType back
    vector<Type*> ftArgs;
    for (Function::arg_iterator a = fun.arg_begin();
         a != fun.arg_end();
         ++a) {
        ftArgs.push_back(maybeGetMappedType(a->getType()));
    }
    FunctionType *ft = FunctionType::get(maybeGetMappedType(fun.getReturnType()),
                                         ftArgs,
                                         fun.isVarArg());
    if (ft != fun.getFunctionType()) {
        SR_DEBUG cerr << "mapping function type\n";
        fun.mutateType(PointerType::getUnqual(ft));
    }

}

void StructResolver::mapFunctions() {

    for (Module::iterator i = module->begin();
         i != module->end();
         ++i) {
        SR_DEBUG cerr << "-----] looking at function: " << 
            i->getName().str() << endl;
        Function &f = (*i);
        SR_DEBUG f.dump();
        // mutate FunctionType if necessary
        mapFunction(f);
        // Body
        for (Function::iterator b = f.begin();
             b != f.end();
             ++b) {
            for (BasicBlock::iterator inst = (*b).begin();
                 inst != (*b).end();
                 ++inst) {
                mapUser(*inst);
            }
        }
        SR_DEBUG f.dump();
    }

    //module->dump();

}

void StructResolver::mapMetadata() {

    for (Module::named_metadata_iterator i = module->named_metadata_begin();
         i != module->named_metadata_end();
         ++i) {
        SR_DEBUG cerr << "-----] looking at metadata: " << 
            i->getName().str() << endl;
        NamedMDNode &m = (*i);
        // MD nodes in named node
        for (int o = 0;
             o < m.getNumOperands();
             ++o) {

            // operands of nodes
            MDNode *node = m.getOperand(o);
            for (int n = 0;
                 n < node->getNumOperands();
                 ++n) {

                Value *v = node->getOperand(n);
                if (v)
                    mapValue(*v);

            }

        }
    }
    //module->dump();

}
