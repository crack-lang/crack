// Copyright 2009 Google Inc.

#include "FuncDef.h"

#include "Context.h"
#include "ArgDef.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace std;

FuncDef::FuncDef(Flags flags, const std::string &name, size_t argCount) :
    // XXX need function types, but they'll probably be assigned after 
    // the fact.
    VarDef(0, name),
    flags(flags),
    args(argCount) {
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

bool FuncDef::hasInstSlot() {
    return false;
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
