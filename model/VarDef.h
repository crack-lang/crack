
#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(Expr);
SPUG_RCPTR(VarDefImpl);
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(VarDef);

// Variable definition.  All names in a context (including functions and 
// types) are derived from VarDef's.
class VarDef : public spug::RCBase {
    public:
        Context *context;
        TypeDefPtr type;
        std::string name;
        VarDefImplPtr impl;

        VarDef(TypeDef *type, const std::string &name);
        virtual ~VarDef();
        
        void emitAssignment(Context &context, Expr *expr);
        
        /**
         * Returns true if the definition type requires a slot in the instance 
         * variable.
         */
        virtual bool hasInstSlot();        
};

} // namespace model

#endif
