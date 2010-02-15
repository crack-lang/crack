
#include "FuncCall.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;

FuncCall::FuncCall(FuncDef *funcDef) :
    Expr(funcDef->returnType.get()),
    func(funcDef) {
    if (!funcDef->returnType)
        std::cerr << "bad func def: " << funcDef->name << std::endl;
    assert(funcDef->returnType);
}

ResultExprPtr FuncCall::emit(Context &context) {
    return context.builder.emitFuncCall(context, this);
}

void FuncCall::writeTo(std::ostream &out) const {
    if (receiver)
        out << "(" << *receiver << ").";
    out << "call \"" << func->name << "\"(";
    for (ExprVec::const_iterator iter = args.begin();
         iter != args.end();
         ++iter
         )
        out << **iter << ", ";
    out << ")";
}
