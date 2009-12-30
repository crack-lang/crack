
#include "StrConst.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

StrConst::StrConst(TypeDef *type, const std::string &val) :
    Expr(type),
    val(val) {
}

ResultExprPtr StrConst::emit(Context &context) { 
    return context.builder.emitStrConst(context, this);
}

void StrConst::writeTo(ostream &out) const {
    out << '"' << val << '"';
}
