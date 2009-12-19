
#include "AssignExpr.h"

#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "parser/Parser.h"
#include "Context.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace parser;

AssignExpr::AssignExpr(Expr *aggregate, VarDef *var, Expr *value) :
    Expr(value->type.get()),
    aggregate(aggregate),
    var(var),
    value(value) {
}

AssignExprPtr AssignExpr::create(const Token &varName,
                                 Expr *aggregate,
                                 VarDef *var, 
                                 Expr *value
                                 ) {
    // check the types
    if (!var->type->matches(*value->type))
        Parser::error(varName,
                      SPUG_FSTR("Assigning variable " << var->name <<
                                 " of type " << var->type->name <<
                                 " from value of type " <<
                                 value->type->name
                                )
                      );
    
    return new AssignExpr(aggregate, var, value);
}

void AssignExpr::emit(Context &context) {
    if (aggregate)
        context.builder.emitFieldAssign(context, aggregate.get(), var.get(), 
                                        value.get()
                                        );
    else
        var->emitAssignment(context, value.get());
}
