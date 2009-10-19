
#include "IntConst.h"

#include "builder/Builder.h"
#include "BuilderVarDefData.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;

IntConst::IntConst(const TypeDefPtr &type, long val) :
    Expr(type),
    val(val) {
}

void IntConst::emit(Context &context) { 
    context.builder.emitIntConst(context, *this);
}
