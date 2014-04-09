// Copyright 2014 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Visitor_h_
#define _model_Visitor_h_

namespace model {

// Visitor interface for traversing the model object hierarchy.
class Visitor {
    public:
        virtual void onModuleDef(ModuleDef *module) = 0;
        virtual void onTypeDef(TypeDef *type) = 0;
        virtual void onVarDef(VarDef *var) = 0;
        virtual void onOverloadDef(OverloadDef *ovld) = 0;
        virtual void onFuncDef(FuncDef *func) = 0;
};

}

#endif
