
#include "IntConst.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;

IntConst::IntConst(TypeDef *type, long val) :
    Expr(type),
    val(val) {
}

ResultExprPtr IntConst::emit(Context &context) { 
    return context.builder.emitIntConst(context, this);
}

void IntConst::writeTo(std::ostream &out) const {
    out << val;
}
