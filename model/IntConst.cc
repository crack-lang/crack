
#include "IntConst.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

void IntConst::emit(Context &context) { 
    context.builder.emitIntConst(context, *this);
}
