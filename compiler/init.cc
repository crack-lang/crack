// Copyright 2010 Google Inc.

#include "init.h"

#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "CrackContext.h"

using namespace crack::ext;

namespace compiler {

void init(Module *mod) {
    Type *cc = mod->addType("CrackContext");
    Func *f = cc->addMethod(mod->getVoidType(), "inject",
                            (void *)&CrackContext::inject
                            );
    f->addArg(mod->getByteptrType(), "code");
    cc->finish();
}

} // namespace compiler
