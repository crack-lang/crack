// Copyright 2009-2012 Google Inc.
// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "FuncDef.h"

#include <sstream>
#include "builder/Builder.h"
#include "Deserializer.h"
#include "Context.h"
#include "ArgDef.h"
#include "Expr.h"
#include "Serializer.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace std;

FuncDef::FuncDef(Flags flags, const std::string &name, size_t argCount) :
    // function types are assigned after the fact.
    VarDef(0, name),
    flags(flags),
    args(argCount),
    vtableSlot(0) {
}

bool FuncDef::matches(Context &context, const vector<ExprPtr> &vals, 
                      vector<ExprPtr> &newVals,
                      FuncDef::Convert convertFlag
                      ) {
    ArgVec::iterator arg;
    vector<ExprPtr>::const_iterator val;
    int i;
    for (arg = args.begin(), val = vals.begin(), i = 0;
         arg != args.end() && val != vals.end();
         ++arg, ++val, ++i
         ) {
        switch (convertFlag) {
            case adapt:
            case adaptSecondary:
                if ((convertFlag == adapt || i) &&
                    (*val)->isAdaptive()
                    ) {
                    newVals[i] = (*val)->convert(context, (*arg)->type.get());
                    if (!newVals[i])
                        return false;
                } else if ((*arg)->type->matches(*(*val)->type)) {
                    newVals[i] = *val;
                } else {
                    return false;
                }
                break;
            case convert:
                newVals[i] = (*val)->convert(context, (*arg)->type.get());
                if (!newVals[i])
                    return false;
                break;
            case noConvert:
                if (!(*arg)->type->matches(*(*val)->type))
                    return false;
                break;
        }
    }

    // make sure that we checked everything in both lists
    if (arg != args.end() || val != vals.end())
        return false;
    
    return true;
}

bool FuncDef::matches(const ArgVec &other_args) {
    ArgVec::iterator arg;
    ArgVec::const_iterator other_arg;
    for (arg = args.begin(), other_arg = other_args.begin();
         arg != args.end() && other_arg != other_args.end();
         ++arg, ++other_arg
         )
        // if the types don't _exactly_ match, the signatures don't match.
        if ((*arg)->type.get() != (*other_arg)->type.get())
            return false;

    // make sure that we checked everything in both lists   
    if (arg != args.end() || other_arg != other_args.end())
        return false;
    
    return true;
}

bool FuncDef::matchesWithNames(const ArgVec &other_args) {
    ArgVec::iterator arg;
    ArgVec::const_iterator other_arg;
    for (arg = args.begin(), other_arg = other_args.begin();
         arg != args.end() && other_arg != other_args.end();
         ++arg, ++other_arg
         ) {
        // if the types don't _exactly_ match, the signatures don't match.
        if ((*arg)->type.get() != (*other_arg)->type.get())
            return false;
        
        // if the arg names don't exactly match, the signatures don't match.
        if ((*arg)->name != (*other_arg)->name)
            return false;
    }

    // make sure that we checked everything in both lists   
    if (arg != args.end() || other_arg != other_args.end())
        return false;

    return true;
}

bool FuncDef::isOverridable() const {
    return flags & virtualized || name == "oper init" || flags & forward;
}

unsigned int FuncDef::getVTableOffset() const {
    return vtableSlot;
}

bool FuncDef::hasInstSlot() {
    return false;
}

bool FuncDef::isStatic() const {
    return !(flags & method);
}

string FuncDef::getDisplayName() const {
    ostringstream out;
    display(out, "");
    return out.str();
}

bool FuncDef::isVirtualOverride() const {
    return flags & virtualized && pathToFirstDeclaration.size();
}

TypeDef *FuncDef::getReceiverType() const {
    TypeDef *result;
    if (pathToFirstDeclaration.size())
        result = pathToFirstDeclaration.back().ancestor.get();
    else
        result = TypeDefPtr::cast(owner);
    return result;
}

TypeDef *FuncDef::getThisType() const {
    return TypeDefPtr::cast(owner);
}

bool FuncDef::isConstant() {
    return true;
}

ExprPtr FuncDef::foldConstants(const vector<ExprPtr> &args) const {
    return 0;
}
    

void FuncDef::dump(ostream &out, const string &prefix) const {
    out << prefix << returnType->getFullName() << " " << getFullName() <<
        args << "\n";
}

void FuncDef::dump(ostream &out, const ArgVec &args) {
    out << '(';
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end(); 
         ++iter
         ) {
        if (!first)
            out << ", ";
        else
            first = false;
        out << (*iter)->type->getFullName() << ' ' << (*iter)->name;
    }
    out << ")";    
}

void FuncDef::display(ostream &out, const ArgVec &args) {
    out << '(';
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end(); 
         ++iter
         ) {
        if (!first)
            out << ", ";
        else
            first = false;
        out << (*iter)->type->getDisplayName() << ' ' << (*iter)->name;
    }
    out << ")";    
}

void FuncDef::display(ostream &out, const string &prefix) const {
    out << prefix << returnType->getDisplayName() << " " << 
        VarDef::getDisplayName();
    display(out, args);
}

void FuncDef::addDependenciesTo(const ModuleDef *mod, 
                                ModuleDefMap &deps
                                ) const {
    returnType->addDependenciesTo(mod, deps);
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->type->addDependenciesTo(mod, deps);
}

bool FuncDef::isSerializable(const Namespace *ns) const {
    return VarDef::isSerializable(ns) && 
           !(flags & FuncDef::builtin);
}

void FuncDef::serialize(Serializer &serializer, bool writeKind,
                        const Namespace *ns
                        ) const {
    assert(!writeKind && owner == ns);
    returnType->serialize(serializer, false, 0);
    
    serializer.write(args.size(), "#args");
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->serialize(serializer, false, 0);
}

FuncDefPtr FuncDef::deserialize(Deserializer &deser, const string &name) {
    TypeDefPtr returnType = TypeDef::deserialize(deser);
    
    int argCount = deser.readUInt("#args");
    ArgVec args;
    for (int i = 0; i < argCount; ++i)
        args.push_back(ArgDef::deserialize(deser));

    return deser.context->builder.materializeFunc(
        *deser.context,
        name,
        args
    );
}
