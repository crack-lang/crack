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

#define SR_DEBUG if (StructResolver::trace)
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

    template <typename T>
    string llvmStr(const T &obj) {
        string temp;
        raw_string_ostream out(temp);
        obj.print(out);
        return out.str();
    }

    class MapGuard {
        typedef map<Type *, bool> MapType;
        MapType &myMap;
        Type *type;
    public:
        MapGuard(MapType &myMap, Type *type) : myMap(myMap), type(type) {
            myMap[type] = true;
        }
        ~MapGuard() {
            MapType::iterator iter = myMap.find(type);
            SPUG_CHECK(iter != myMap.end(), "MapGuard RAII failed.");
            myMap.erase(iter);
        }
    };

    // We want to:
    //  1) reconstruct all StructTypes that are duplicates
    //  2) reconstruct all types that depend on reconstructed types
    class TypeMapper {
    private:
        map<Type *, bool> traversed;
        map<Type *, Type *> &typeMap;

    public:
        TypeMapper(map<Type *, Type *> &typeMap) : typeMap(typeMap) {}

        // 'type' should not be a duplicate.
        Type *mapStructTypeByElements(StructType *type) {
            vector<Type *> elements;
            bool mustMap = false;
            for (StructType::element_iterator iter = type->element_begin();
                iter != type->element_end();
                ++iter
                ) {
                Type *elemType = *iter;
                if (Type *newType = mapType(elemType)) {
                    elemType = newType;
                    mustMap = true;
                }
                elements.push_back(elemType);
            }

            if (mustMap) {
                // See if the type already got mapped when doing the fields.
                StructResolver::StructMapType::iterator iter =
                    typeMap.find(type);
                if (iter != typeMap.end())
                    return iter->second;

                // Make the current type relinquish its name.
                string origName = type->getName();
                SPUG_CHECK(origName.substr(0, 3) != ":::",
                           "re-renaming type " << origName
                           );
                type->setName(SPUG_FSTR(":::" << origName << "." << type));

                SR_DEBUG cerr << "Rebuilding struct type " << origName <<
                    " (depends on mapped types)" << endl;

                // XXX this is lame, We add this to the map here to avoid
                // iterating over a type we've already changed, but as it
                // stands we'll end up adding the type twice.
                StructType *newType =
                    StructType::create(elements, origName, type->isPacked());
                typeMap[type] = newType;

                SPUG_CHECK(!LLVMBuilder::getLLVMType(origName),
                           "Renamed registered type " << origName
                           );
                LLVMBuilder::putLLVMType(origName, newType);

                return newType;
            } else {
                // If we get here, this is the original instance of the type,
                // loaded from the cache, and it contains no nested types that
                // need to be converted.  Register it.
                typeMap[type] = type;
                LLVMBuilder::putLLVMType(string(type->getName()), type);

                return 0;
            }
        }

        Type *mapStructType(StructType *structTy) {
            // does it have a name?
            if (!structTy->hasName())
                return 0;

            if (spug::contains(traversed, structTy))
                return 0;
            MapGuard guard(traversed, structTy);

            string name = structTy->getName().str();

            // does it have a numeric suffix? (i.e. is it a duplicate type name?)
            int pos = name.rfind(".");
            bool isDuplicate = false;
            if (pos != string::npos && name.size() > pos) {
                string suffix = name.substr(pos + 1);
                for (string::const_iterator si = suffix.begin();
                    si != suffix.end();
                    ++si
                    ) {
                    isDuplicate = isdigit(*si);
                    if (!isDuplicate)
                        break;
                }
            }

            if (!isDuplicate) {
                // The type itself isn't a duplicate.  Now we have to check all
                // nested types since these might have been duplicates even if the
                // outer type wasn't.
                return mapStructTypeByElements(structTy);
            }

            string canonicalName = name.substr(0, pos);

            SR_DEBUG cerr << "Mapping type " << structTy->getName().str() <<
                " to " << canonicalName << endl;

            // The numeric suffix is only applied when we discover a new type
            // with the same name.  Therefore, the fact that there is a
            // numeric suffix implies that we should have already loaded the
            // type.
            StructType *type = LLVMBuilder::getLLVMType(canonicalName);

            // since we're now deferring type resolution until after the linker,
            // we can get into a situation where multiple modules load the type
            // before it is registered.  Most likely this will cause problems with
            // collapsing isomorphic types, too.
            if (!type) {
                SR_DEBUG cerr << "Unregistered duplicate type found for " <<
                    canonicalName << endl;
                LLVMBuilder::putLLVMType(canonicalName, structTy);
                structTy->setName(canonicalName);
                return structTy;
            } else if (type->getName() != canonicalName) {
                // This was to help me debug something.
                SR_DEBUG cerr << "Restoring missing name for original type " <<
                    canonicalName << " (was " << llvmStr(*type) << ")" << endl;
                type->setName(canonicalName);
                if (type == structTy)
                    return 0;
            }

            StructType *left = structTy;
            assert(left);
            StructType *right = type;
            SPUG_CHECK(left != right,
                       "Mapping type " << canonicalName << " to itself");
            //cout << "struct map [" << left->getName().str() << "] to [" << right->getName().str() << "]\n";
            typeMap[left] = right;
            return right;
        }

        void traceMapping(Type *type, Type *newType, const char *designator) {
            cerr << "Mapping " << designator << " type " << llvmStr(*type) <<
                " to " << llvmStr(*newType) << endl;
        }

        Type *mapArrayType(ArrayType *type) {
            Type *mappedElemType = mapType(type->getElementType());
            if (mappedElemType) {
                Type *newType =
                    ArrayType::get(mappedElemType, type->getNumElements());
                if (StructResolver::trace)
                    traceMapping(type, newType, "array");
                typeMap[type] = newType;
                return newType;
            } else {
                return 0;
            }
        }

        Type *mapPointerType(PointerType *type) {
            Type *mappedElemType = mapType(type->getElementType());
            if (mappedElemType) {
                Type *newType =
                    PointerType::get(mappedElemType, type->getAddressSpace());
                if (StructResolver::trace)
                    traceMapping(type, newType, "pointer");
                typeMap[type] = newType;
                return newType;
            } else {
                return 0;
            }
        }

        Type *mapFunctionType(FunctionType *type) {
            bool needsMap = false;
            Type *returnType = type->getReturnType();
            Type *newType;
            if (newType = mapType(returnType)) {
                needsMap = true;
                returnType = newType;
            }

            vector<Type *> parmTypes;
            for (FunctionType::param_iterator iter = type->param_begin();
                iter != type->param_end();
                ++iter
                ) {
                Type *paramType = *iter;
                if (newType = mapType(paramType)) {
                    needsMap = true;
                    paramType = newType;
                }
                parmTypes.push_back(paramType);
            }

            if (needsMap) {
                newType = FunctionType::get(returnType, parmTypes,
                                            type->isVarArg()
                                            );
                if (StructResolver::trace)
                    traceMapping(type, newType, "function");
                typeMap[type] = newType;
                return newType;
            } else {
                return 0;
            }
        }

        Type *mapType(Type *type) {
            StructResolver::StructMapType::iterator iter = typeMap.find(type);
            if (iter != typeMap.end())
                return iter->second;

            switch (type->getTypeID()) {
                case Type::StructTyID:
                    return mapStructType(cast<StructType>(type));
                case Type::ArrayTyID:
                    return mapArrayType(cast<ArrayType>(type));
                case Type::PointerTyID:
                    return mapPointerType(cast<PointerType>(type));
                case Type::FunctionTyID:
                    return mapFunctionType(cast<FunctionType>(type));
            }
            return 0;
        }
    };

} // anon namespace

void StructResolver::buildTypeMap() {

    SetVector<Type *> usedTypes;

    FindUsedTypes findTypes;
    findTypes.runOnModule(*module);
    usedTypes = findTypes.getTypes();

    // NOTE this gets a list by struct name only: no isomorphism checks

    for (SetVector<Type *>::iterator i = usedTypes.begin();
         i != usedTypes.end(); ++i
         ) {
        // See if we've already mapped the type.
        if (spug::contains(typeMap, *i))
            continue;

        TypeMapper mapper(typeMap);
        mapper.mapType(*i);
    }
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

Type *StructResolver::remapType(Type *srcType) {
    StructMapType::const_iterator iter = typeMap.find(srcType);
    if (iter != typeMap.end())
        return iter->second;
    else
        return srcType;
}
