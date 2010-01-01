
#ifndef _model_StubDef_h
#define _model_StubDef_h

#include <spug/RCPtr.h>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(StubDef);

/**
 * When we import from a shared library, we end up with an address but no 
 * definition.  A StubDef stores the address and reserves the name in the 
 * namespace until a definition arrives.
 */
class StubDef : public VarDef {
    public:

        void *address;

        /**
         * @param type should be the "void" type.
         */
        StubDef(TypeDef *type, const std::string &name, void *address) :
            VarDef(type, name),
            address(address) {
        }

        virtual bool hasInstSlot() { return false; }
};

} // namespace model


#endif
