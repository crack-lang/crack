// Copyright 2009 Google Inc.

#include "AssignExpr.h"

#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "parser/Parser.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace parser;
using namespace std;

AssignExpr::AssignExpr(Expr *aggregate, VarDef *var, Expr *value) :
    Expr(value->type.get()),
    aggregate(aggregate),
    var(var),
    value(value) {
}

AssignExprPtr AssignExpr::create(Context &context,
                                 Expr *aggregate,
                                 VarDef *var, 
                                 Expr *value
                                 ) {
    // check the types
    ExprPtr converted = value->convert(context, var->type.get());
    if (!converted)
        context.error(SPUG_FSTR("Assigning variable " << var->name <<
                                 " of type " << var->type->name <<
                                 " from value of type " <<
                                 value->type->name
                                )
                      );

    // XXX should let the builder do this    
    return new AssignExpr(aggregate, var, converted.get());
}

ResultExprPtr AssignExpr::emit(Context &context) {
    // see if the variable has a release function
    bool gotReleaseFunc = context.lookUpNoArgs("oper release", false,
                                               var->type.get()
                                               );

    if (aggregate) {

        ExprPtr agg = aggregate;
        if (gotReleaseFunc) {

            // emit the aggregate, store the ResultExpr for use when we emit 
            // the field assignment.
            ResultExprPtr aggResult;
            agg = aggResult = aggregate->emit(context);
            aggResult->handleTransient(context);

            // emit the release call on the result
            VarRefPtr varRef = 
                context.builder.createFieldRef(aggregate.get(), var.get());
            varRef->emit(context)->forceCleanup(context);
        }

        return context.builder.emitFieldAssign(context, agg.get(), this);
    } else {
        if (gotReleaseFunc) {
            // emit a release call on the existing value.
            VarRefPtr varRef = context.builder.createVarRef(var.get());
            varRef->emit(context)->forceCleanup(context);
        }
        return var->emitAssignment(context, value.get());
    }
}

bool AssignExpr::isProductive() const {
    return false;
}

void AssignExpr::writeTo(std::ostream &out) const {
    out << var->name << " = ";
    value->writeTo(out);
}
