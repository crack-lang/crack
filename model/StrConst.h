// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_StrConst_h_
#define _model_StrConst_h_
    
#include "Context.h"
#include "Expr.h"

namespace model {

class StrConst : public Expr {
    public:
        std::string val;
        
        StrConst(TypeDef *type, const std::string &val);
        
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};

} // namespace parser

#endif
