
#include "IntConst.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;

IntConst::IntConst(TypeDef *type, long val) :
    Expr(type),
    val(val) {
}

void IntConst::emit(Context &context) { 
    context.builder.emitIntConst(context, *this);
}
