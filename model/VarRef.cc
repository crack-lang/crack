// Copyright 2009 Google Inc.

#include "VarRef.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;
using namespace std;

VarRef::VarRef(VarDef *def) :
    Expr(def->type.get()),
    def(def) {
}

ResultExprPtr VarRef::emit(Context &context) {
    assert(def->impl);
    return def->impl->emitRef(context, this);
}

bool VarRef::isProductive() const {
    return false;
}

void VarRef::writeTo(ostream &out) const {
    out << "ref(" << def->name << ')';
}
