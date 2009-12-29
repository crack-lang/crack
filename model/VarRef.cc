
#include "VarRef.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;
using namespace std;

VarRef::VarRef(VarDef *def) :
    Expr(def->type.get()),
    def(def) {
}

void VarRef::emit(Context &context) {
    assert(def->impl);
    def->impl->emitRef(context, def.get());
}

void VarRef::writeTo(ostream &out) const {
    out << "ref(" << def->name << ')';
}
