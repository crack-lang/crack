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
#include "spug/check.h"
#include "Deserializer.h"
#include "Context.h"
#include "ArgDef.h"
#include "Expr.h"
#include "NestedDeserializer.h"
#include "OverloadDef.h"
#include "ProtoBuf.h"
#include "Serializer.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace std;

void FuncDef::Spec::deserialize(Deserializer &deser) {
    returnType = TypeDef::deserializeRef(deser);
    flags = static_cast<Flags>(deser.readUInt("flags"));

    args = deserializeArgs(deser);

    // read the optional data, the only field we're interested in is the 
    // receiverType
    CRACK_PB_BEGIN(deser, 256, optional)
        CRACK_PB_FIELD(1, ref)
            receiverType = TypeDef::deserializeRef(optionalDeser);
            break;
        CRACK_PB_FIELD(2, varInt)
            vtableSlot = optionalDeser.readUInt("vtableSlot");
            break;
    CRACK_PB_END    
}

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
                } else if ((*val)->type && 
                            (*arg)->type->matches(*(*val)->type)
                           ) {
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
                // We have to check for a null value type because we could 
                // have a VarRef to an OverloadDef, whose type is null.
                if (!(*val)->type || !(*arg)->type->matches(*(*val)->type))
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

bool FuncDef::hasInstSlot() const {
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

string FuncDef::getUniqueId(Namespace *ns) const {
    ostringstream out;
    SPUG_CHECK(owner || ns, 
               "getUniqueId() called without a namespace or owner for "
                "function " << name
               );
    if (!ns) ns = owner;
    out << ns->getNamespaceName() << "." << name << "(";
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         ) {
        if (first)
            first = false;
        else
            out << ",";
        out << (*iter)->type->getFullName();
    }
    out << ")";
    return out.str();
}

bool FuncDef::isVirtualOverride() const {
    return flags & virtualized && receiverType != owner;
}

TypeDef *FuncDef::getThisType() const {
    return TypeDefPtr::cast(owner);
}

bool FuncDef::isConstant() {
    return true;
}

bool FuncDef::isAliasIn(const OverloadDef &overload) const {
    return overload.getOwner() != owner || overload.name != name;
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

void FuncDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {
    mod->addDependency(getModule());
    returnType->addDependenciesTo(mod, added);
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->type->addDependenciesTo(mod, added);
}

bool FuncDef::isUsableFrom(const Context &context) const {
    // Functions are always usable because even if there are methods we can 
    // reference the address (they may not be callable as methods, but we can 
    // deal with that later).
    return true;
}

bool FuncDef::needsReceiver() const {
    return !isStatic();
}

bool FuncDef::isSerializable() const {
    return VarDef::isSerializable() && !(flags & FuncDef::builtin);
}

void FuncDef::serializeArgs(Serializer &serializer) const {
    serializer.write(args.size(), "#args");
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->serialize(serializer, false, 0);
}

void FuncDef::serializeCommon(Serializer &serializer) const {
    returnType->serialize(serializer, false, 0);
    serializer.write(static_cast<unsigned>(flags), "flags");
    
    serializeArgs(serializer);
    
    if (flags & method) {
        ostringstream temp;
        Serializer sub(serializer, temp);
        sub.write(CRACK_PB_KEY(1, ref), "receiverType.header");
        receiverType->serialize(sub, false, 0);
        
        if (flags & virtualized) {
            sub.write(CRACK_PB_KEY(2, varInt), "vtableSlot.header");
            sub.write(vtableSlot, "vtableSlot");
        }

        serializer.write(temp.str(), "optional");
    } else {
        serializer.write(0, "optional");
    }
}

void FuncDef::serialize(Serializer &serializer) const { 
    serializer.write(0, "isAlias");
    serializeCommon(serializer);
}

void FuncDef::serializeAlias(Serializer &serializer) const {
    serializer.write(1, "isAlias");
    serializeExtern(serializer);
    serializeArgs(serializer);
}

void FuncDef::serialize(Serializer &serializer, bool writeKind,
                        const Namespace *ns
                        ) const {
    SPUG_CHECK(false, 
               "Directly called FuncDef::serialize() for function " <<
                *this
               );
}

FuncDef::ArgVec FuncDef::deserializeArgs(Deserializer &deser) {
    int argCount = deser.readUInt("#args");
    ArgVec args;
    for (int i = 0; i < argCount; ++i)
        args.push_back(ArgDef::deserialize(deser));
    return args;
}

FuncDefPtr FuncDef::deserialize(Deserializer &deser, const string &name) {
    bool alias = deser.readUInt("isAlias");
    if (alias) {
        OverloadDefPtr ovld = deserializeOverloadAliasBody(deser);
        return ovld->getSigMatch(deserializeArgs(deser), true);
    }
    Spec spec;
    spec.deserialize(deser);
    
    FuncDefPtr result = deser.context->builder.materializeFunc(
        *deser.context,
        spec.flags,
        name,
        spec.returnType.get(),
        spec.args
    );
    
    result->receiverType = spec.receiverType;
    result->vtableSlot = spec.vtableSlot;

    return result;
}
