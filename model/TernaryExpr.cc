
#include "TernaryExpr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "ResultExpr.h"

using namespace std;
using namespace model;

ResultExprPtr TernaryExpr::emit(Context &context) {
    return context.builder.emitTernary(context, this);
}

void TernaryExpr::writeTo(ostream &out) const {
    out << "(";
    cond->writeTo(out);
    out << " ? ";
    trueVal->writeTo(out);
    out << " : ";
    falseVal->writeTo(out);
    out << ")";
}
