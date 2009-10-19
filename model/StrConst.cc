
#include "StrConst.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;

StrConst::StrConst(const TypeDefPtr &type, const std::string &val) :
    Expr(type),
    val(val) {
}

void StrConst::emit(Context &context) { 
    context.builder.emitStrConst(context, this);
}
