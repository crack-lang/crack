
#ifndef _model_FuncCall_h_
#define _model_FuncCall_h_

#include <string>
#include <vector>
#include "Expr.h"

namespace model {

SPUG_RCPTR(FuncCall);

class FuncCall : public Expr {
    public:
        std::string name;
        typedef std::vector<ExprPtr> ExprVector;
        ExprVector args;

        FuncCall(const TypeDefPtr &type, const std::string &name);
        virtual void emit(Context &context);
};


} // namespace model

#endif
