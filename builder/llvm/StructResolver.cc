// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>

#include "StructResolver.h"

#include <assert.h>
#include <ctype.h>

#include <vector>
#include <iostream>

#include <llvm/ADT/StringMap.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/DerivedTypes.h>
#include <llvm/User.h>
#include <llvm/Constants.h>

using namespace llvm;
using namespace std;

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
    vector<StructType*> usedTypes;

    module->findUsedStructTypes(usedTypes);

    // NOTE this gets a list by struct name only: no isomorphism checks

    for (int i=0; i < usedTypes.size(); ++i) {
        // does it have a name?
        if (!usedTypes[i]->hasName())
            continue;
        sName = usedTypes[i]->getName().str();
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
                result[sName] = usedTypes[i];
        }
    }

    return result;

}

void StructResolver::run(StructMapType *m) {

    if (m->empty())
        return;

    typeMap = m;
    mapGlobals();
    mapFunctions();
    mapMetadata();
    module->dump();

}

Type *StructResolver::maybeGetMappedType(Type *t) {

    if (typeMap->find(t) != typeMap->end()) {
        cout << "\t\t## --- MAPPING --- ##\n";
        cout << "was:\n";
        t->dump();
        cout << "\nnow:\n";
        (*typeMap)[t]->dump();
        cout << "\n";
        return (*typeMap)[t];
    }

    // short cut if not composite
    if (!isa<CompositeType>(t))
        return t;

    if (isa<PointerType>(t)) {
        PointerType *a = dyn_cast<PointerType>(t);
        cout << "\t\t## pointer, points to type: " << tName(a->getElementType()->getTypeID()) << "\n";
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
        cout << "\t\t## array of type: " << tName(a->getElementType()->getTypeID()) << "\n";
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
        StructType *a = dyn_cast<StructType>(t);
        cout << "\t\t## struct\n";
        if (a->hasName()) {
            cout << "\t\t## has name: " << a->getName().str() << "\n";
        }
        // element iterate on types
        vector<Type*> sVec;
        Type *p;
        bool modified = false;
        for (StructType::element_iterator e = a->element_begin();
             e != a->element_end();
             ++e) {

            //cout << "\t\t\t---> type: [[[[[\n";
            //(*e)->dump();
            //cout << "]]]]]\n";

            // accumulate the types. if we find one we have to map, we'll use
            // the accumlated types to create a new structure with the mapped
            // type. if it doesn't contain one, we discard it
            p = maybeGetMappedType(*e);
            if (p != *e) {
                modified = true;
                sVec.push_back(p);
            }
            else {
                sVec.push_back(*e);
            }
        }
        if (modified) {            
            StructType *m;
            StructType *origS = cast<StructType>(t);
            if (origS->isLiteral()) {
                m = StructType::get(getGlobalContext(), sVec);
            }
            else {
                // take over the name
                origS->setName("");
                m = StructType::create(sVec, origS->getName().str());
            }
            (*typeMap)[t] = m;
            //return m;
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

    cout << "@@ mapValue, before\n";
    //val.dump();

    if (visited.find(&val) != visited.end()) {
        cout << "\t@@ already seen\n";
        return;
    }

    if (isa<Constant>(val)) {
        cout << "\t@@ is constant\n";
    }

    Type *t = maybeGetMappedType(val.getType());
    if (t != val.getType()) {
        val.mutateType(t);
    }

    visited[&val] = true;

    cout << "@@ mapValue, after\n";
    //val.dump();

}

// User is a Value and may have a list of Value operands
void StructResolver::mapUser(User &val) {

    cout << "#mapUser, before\n";
    //val.dump();

    cout << "#value itself:\n";
    mapValue(val);

    if (val.getNumOperands()) {
        int opNum = 1;
        for (User::op_iterator o = val.op_begin();
             o != val.op_end();
             ++o) {
            // o iterates through Use, which is essentially a wrapper for Value
            Value *op = *o;
            if (isa<CompositeType>(op->getType())) {
                if ((*o)->hasName())
                    cout << "#op named: " << (*o)->getValueName()->getKey().str() << "\n";
                else
                    cout << "#op #" << opNum++ << "\n";
                if (isa<User>(op))
                    mapUser(cast<User>(*op));
                else
                    mapValue(*op);
            }
        }
    }

    cout << "#mapUser, after\n";
    //val.dump();

}

void StructResolver::mapGlobals() {

    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i) {
        cout << "---------------------------------------] looking at global: " << i->getName().str() << "----------------------------------\n";
        mapUser(*i);
    }
    //module->dump();

}

void StructResolver::mapFunctions() {

    for (Module::iterator i = module->begin();
         i != module->end();
         ++i) {
        cout << "---------------------------------------] looking at function: " << i->getName().str() << "----------------------------------\n";
        Function &f = (*i);
        // Arguments
        for (Function::arg_iterator a = f.arg_begin();
             a != f.arg_end();
             ++a) {
            mapValue(*a);
        }
        f.dump();
    }
    //module->dump();

}

void StructResolver::mapMetadata() {

    for (Module::named_metadata_iterator i = module->named_metadata_begin();
         i != module->named_metadata_end();
         ++i) {
        cout << "---------------------------------------] looking at metadata: " << i->getName().str() << "----------------------------------\n";
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
