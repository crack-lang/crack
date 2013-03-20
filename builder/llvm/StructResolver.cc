// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
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
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/User.h>
#include <llvm/Constants.h>
#include <llvm/Analysis/FindUsedTypes.h>

using namespace llvm;
using namespace std;
using namespace builder::mvll;

#define SR_DEBUG if (trace)
bool StructResolver::trace = false;

namespace {
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
StructResolver::StructListType StructResolver::getDisjointStructs() {

    string sName;
    StructListType result;
    SetVector<Type*> usedTypes;

    FindUsedTypes findTypes;
    findTypes.runOnModule(*module);
    usedTypes = findTypes.getTypes();

    // NOTE this gets a list by struct name only: no isomorphism checks

    for (int i=0; i < usedTypes.size(); ++i) {
        if (!usedTypes[i]->isStructTy())
            continue;

        StructType *structTy = static_cast<StructType*>(usedTypes[i]);

        // does it have a name?
        if (!structTy->hasName())
            continue;
        sName = structTy->getName().str();
        // does it have a numeric suffix?
        int pos = sName.rfind(".");
        if (pos != string::npos && sName.size() > pos) {
            string suffix = sName.substr(pos+1);
            bool isNumeric = false;
            for (string::const_iterator si = suffix.begin();
                 si != suffix.end();
                 ++si) {
                isNumeric = isdigit(*si);
                if (!isNumeric)
                    break;
            }
            if (isNumeric)
                result[sName] = structTy;
        }
    }

    return result;

}

void StructResolver::run(StructMapType *m) {

    if (m->empty())
        return;

    module->MaterializeAll();

    typeMap = m;
    for (StructMapType::iterator iter = m->begin(); iter != m->end(); ++iter)
        reverseMap[iter->second] = iter->first;
    mapGlobals();
    mapFunctions();
    mapMetadata();
    SR_DEBUG module->dump();

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

    if (typeMap->find(t) != typeMap->end()) {
        SR_DEBUG cout << "\t\t## --- MAPPING --- ##\n";
        SR_DEBUG cout << "was:\n";
        SR_DEBUG t->dump();
        SR_DEBUG cout << "\nnow:\n";
        SR_DEBUG (*typeMap)[t]->dump();
        SR_DEBUG cout << "\n";
        return (*typeMap)[t];
    } else if (reverseMap.find(t) != reverseMap.end()) {
        SR_DEBUG cout << "\t\t## --- PREMAPPED --- ##\n";
        SR_DEBUG t->dump();
        SR_DEBUG cout << "\n";
        return t;
    } else if (visitedStructs.find(t) != visitedStructs.end()) {
        SR_DEBUG cout << "\t\t## -- VISITED STRUCT --- ##\n";
        SR_DEBUG t->dump();
        SR_DEBUG cout << "\n";
        return t;
    }

    if (isa<FunctionType>(t)) {
        FunctionType *funcType = dyn_cast<FunctionType>(t);
        SR_DEBUG cout << "\t\t## func type: " << llvmStr(*funcType) << endl;
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
            SR_DEBUG cout << "\t\treplacing func type: " << 
                llvmStr(*funcType) << endl;
            Type *m = FunctionType::get(retType, argTypes, 
                                        funcType->isVarArg()
                                        );
            (*typeMap)[t] = m;
            return maybeGetMappedType(t);
        }
    }

    // short cut if not composite
    if (!isa<CompositeType>(t))
        return t;

    if (isa<PointerType>(t)) {
        PointerType *a = dyn_cast<PointerType>(t);
        SR_DEBUG cout << "\t\t## pointer, points to type: " << tName(a->getElementType()->getTypeID()) << "\n";
        Type *p = maybeGetMappedType(a->getElementType());
        // p will be the end of a pointer chain
        if (p != a->getElementType()) {
            // it's one we are mapping, so map and recurse back up
            Type *m = PointerType::get(p, a->getAddressSpace());
            (*typeMap)[t] = m;
            //return m;
            return maybeGetMappedType(t);
        }
    }
    else if (isa<ArrayType>(t)) {
        ArrayType *a = dyn_cast<ArrayType>(t);
        SR_DEBUG cout << "\t\t## array of type: " << tName(a->getElementType()->getTypeID()) << "\n";
        Type *p = maybeGetMappedType(a->getElementType());
        if (p != a->getElementType()) {
            // it's one we are mapping, so map and recurse back up
            Type *m = ArrayType::get(p, a->getNumElements());
            (*typeMap)[t] = m;
            //return m;
            return maybeGetMappedType(t);
        }
    }
    else if (isa<StructType>(t)) {
        StructType *origType = dyn_cast<StructType>(t);
        visitedStructs[t] = 0;
        SR_DEBUG cout << "\t\t## struct\n";
        if (origType->hasName()) {
            SR_DEBUG cout << "\t\t## has name: " << 
                origType->getName().str() << "\n";
        }

        // build the element vector.
        vector<Type *> elems;
        bool modified = buildElementVec(origType, elems);
        
        int cycleCount = visitedStructs[t];
        visitedStructs.erase(t);
        if (modified) {
            StructType *newType = StructType::create(getGlobalContext());
            (*typeMap)[t] = newType;
            
            // if the type contains any cycles, add a placeholder for it and 
            // recalculate the member types.
            if (visitedStructs[t])
                buildElementVec(origType, elems);

            SR_DEBUG cout << "\t\t## replacing struct " << 
                (origType->hasName() ? origType->getName().str() : 
                 string("<noname>")
                 )
                << endl;

            newType->setBody(elems);
            if (!origType->isLiteral()) {
                // take over the name
                string origName = origType->getName().str();
                origType->setName("");
                newType->setName(origName);
            }

            return maybeGetMappedType(t);
        }
    }

    // unchanged
    return t;

}

void StructResolver::mapValue(Value &val) {

    // we only care about compsite types
    /*
    if (!isa<CompositeType>(val.getType())) {
        cout << "\t@@ skipping non composite type\n";
        return;
    }*/

    SR_DEBUG cout << "@@ mapValue ["<<&val<<"], before\n";
    SR_DEBUG val.dump();

    if (visited.find(&val) != visited.end()) {
        SR_DEBUG cout << "\t@@ already seen\n";
        return;
    }

    if (isa<Function>(val)) {
        SR_DEBUG cout << "\t@@ skipping function\n";
        return;
    }

    Type *t = maybeGetMappedType(val.getType());
    if (t != val.getType()) {
        val.mutateType(t);
    }

    visited[&val] = true;

    SR_DEBUG cout << "@@ mapValue ["<<&val<<"], after\n";
    SR_DEBUG val.dump();

}

// User is a Value and may have a list of Value operands
void StructResolver::mapUser(User &val) {

    SR_DEBUG cout << "#mapUser, before\n";
    //val.dump();

    if (visited.find(&val) != visited.end()) {
        SR_DEBUG cout << "\t@@ already seen\n";
        return;
    }

    SR_DEBUG cout << "#value itself:\n";
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
                    cout << "#op named: " << (*o)->getValueName()->getKey().str() << "\n";
                else
                    cout << "#op #" << opNum++ << "\n";*/
                if (isa<User>(op))
                    mapUser(cast<User>(*op));
                else
                    mapValue(*op);
            }
        }
    }

    SR_DEBUG cout << "#mapUser, after\n";
    //val.dump();

}

void StructResolver::mapGlobals() {

    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i) {
        SR_DEBUG cout << "---------------------------------------] looking at global: " << i->getName().str() << "----------------------------------\n";
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
        SR_DEBUG cout << "mapping function type\n";
        fun.mutateType(PointerType::getUnqual(ft));
    }

}

void StructResolver::mapFunctions() {

    for (Module::iterator i = module->begin();
         i != module->end();
         ++i) {
        SR_DEBUG cout << "---------------------------------------] looking at function: " << i->getName().str() << "----------------------------------\n";
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
        SR_DEBUG cout << "---------------------------------------] looking at metadata: " << i->getName().str() << "----------------------------------\n";
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
