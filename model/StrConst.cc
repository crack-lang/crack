
#include "StrConst.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

void StrConst::emit(Context &context) { 
    context.builder.emitStrConst(context, this);
}
