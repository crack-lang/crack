
#include "Namespace.h"

#include "Expr.h"
#include "OverloadDef.h"
#include "VarDef.h"

using namespace std;
using namespace model;

void Namespace::storeDef(VarDef *def) {
    FuncDef *funcDef;
    if (funcDef = FuncDefPtr::cast(def)) {
        OverloadDefPtr overloads = getOverload(def->name);
        overloads->addFunc(funcDef);
    } else {        
        defs[def->name] = def;
    }
}

OverloadDefPtr Namespace::getOverload(const std::string &varName) {
    // see if the name exists in the current context
    OverloadDefPtr overloads = lookUp(varName, false);
    if (overloads)
        return overloads;

    overloads = new OverloadDef(varName);
    overloads->type = OverloadDef::overloadType;
    
    // merge in the overloads from the parents
    
    NamespacePtr parent;
    for (unsigned i = 0; parent = getParent(i++);)
        overloads->addParent(parent.get());

    if (overloads) {
        defs[varName] = overloads;
        overloads->setOwner(this);
    }

    return overloads;
}

VarDefPtr Namespace::lookUp(const std::string &varName, bool recurse) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end()) {
        return iter->second;
    } else if (recurse) {
        VarDefPtr def;

        // try to find the definition in the parents
        NamespacePtr parent;
        for (unsigned i = 0; parent = getParent(i++);)
            if (def = parent->lookUp(varName))
                break;
        
        // if we got an overload, we need to create an overload in this 
        // context.
        OverloadDef *overload = OverloadDefPtr::rcast(def);
        if (overload)
            return getOverload(varName);
        else
            return def;
    }

    return 0;
}

FuncDefPtr Namespace::lookUp(Context &context,
                             const std::string &varName,
                             vector<ExprPtr> &args
                             ) {
    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName);
    if (!var)
        return 0;
    
    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef) {
        FuncDefPtr operNew =
            typeDef->lookUp(context, "oper new", args);

        // make sure we got it, and we didn't inherit it
        if (!operNew || operNew->getOwner() != typeDef)
            return 0;
        
        return operNew;
    }
    
    // if this is an overload, get the function from it.
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return 0;
    return overload->getMatch(context, args);
}

FuncDefPtr Namespace::lookUpNoArgs(const std::string &name, bool acceptAlias) {
    OverloadDefPtr overload = getOverload(name);
    if (!overload)
        return 0;

    // we can just check for a signature match here - cheaper and easier.
    FuncDef::ArgVec args;
    FuncDefPtr result = overload->getNoArgMatch(acceptAlias);
    return result;
}

void Namespace::addDef(VarDef *def) {
    assert(!def->getOwner());

    storeDef(def);
    def->setOwner(this);
}

void Namespace::removeDef(VarDef *def) {
    assert(!OverloadDefPtr::cast(def));
    VarDefMap::iterator iter = defs.find(def->name);
    assert(iter != defs.end());
    defs.erase(iter);
}

void Namespace::addAlias(VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->getOwner());

    // overloads should never be aliased - otherwise the new context could 
    // extend them.
    OverloadDef *overload = OverloadDefPtr::cast(def);
    if (overload) {
        OverloadDefPtr child = overload->createChild();
        storeDef(child.get());
        child->setOwner(this);
    } else {
        storeDef(def);
    }
}

void Namespace::addAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->getOwner());
    defs[name] = def;
}

void Namespace::replaceDef(VarDef *def) {
    assert(!def->getOwner());
    assert(!def->hasInstSlot() && 
           "Attempted to replace an instance variable, this doesn't work "
           "because it won't change the 'ordered' vector."
           );
    def->setOwner(this);
    defs[def->name] = def;
}

void Namespace::dump(ostream &out, const string &prefix) {
    out << canonicalName << " (0x" << this << ") {\n";
    string childPfx = prefix + "  ";
    unsigned i = 0;
    Namespace *parent;
    while (parent = getParent(i++).get()) {
        out << childPfx << "parent namespace ";
        parent->dump(out, childPfx);
    }
    
    for (VarDefMap::const_iterator varIter = defs.begin();
         varIter != defs.end();
         ++varIter
         )
        varIter->second->dump(out, childPfx);
    out << prefix << "}\n";
}

void Namespace::dump() {
    dump(cerr, "");
}
