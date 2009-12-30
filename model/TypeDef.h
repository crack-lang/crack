
#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <spug/RCPtr.h>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
SPUG_RCPTR(VarDef);

SPUG_RCPTR(TypeDef);

// a type.
class TypeDef : public VarDef {
    public:
        
        // the type's context - contains all of the method/attribute 
        // definitions for the type.
        ContextPtr context;
        
        // the default initializer expression (XXX I'm not sure that we want 
        // to keep this, for now it's expedient to be able to do variable 
        // initialization without the whole "oper new" business)
        ExprPtr defaultInitializer;
        
        // if true, the type is a pointer type (points to a structure)
        bool pointer;
        
        // XXX need a metatype
        TypeDef(const std::string &name, bool pointer = false) :
            VarDef(0, name),
            pointer(pointer) {
        }
       
        /**
         * Overrides VarDef::hasInstSlot() to return false (nested classes 
         * don't need an instance slot).
         */
        virtual bool hasInstSlot();
        
        /** Emit a variable definition for the type. */
        VarDefPtr emitVarDef(Context &container, const std::string &name,
                             Expr *initializer
                             );
        
        /** 
         * Returns true if "other" satisfies the type - in other words, if 
         * "other" either equals "this" or is a subclass of "this".
         */
        bool matches(const TypeDef &other) const;
        
        /**
         * Emit code to "narrow" the type context to the specified 
         * target type (which must be a base class of this type).
         * 
         * This can be used by a Builder to force the generation of special 
         * code to refocus a field or method reference to a specific ancestor 
         * class.
         * 
         * The base implementation recursively calls itself if the expression 
         * type's context is not "target", then calls Builder::emitNarrower() 
         * to narrow down from the specific type.  It may be overriden to 
         * provide functionality better suited to the Builder.
         * 
         * @return true if target is a base type.
         */
        virtual bool emitNarrower(TypeDef &target);

        /**
         * Create the default initializer.
         */
        FuncDefPtr createDefaultInit();

        /**
         * Create a "new" function to wrap the specified "init" function.
         */
        void createNewFunc(FuncDef *initFunc);
        
        /**
         * Return a function to convert to the specified type, if such a 
         * function exists.
         */
        virtual FuncDefPtr getConverter(const TypeDef &other);

        /**
         * Fill in everything that's missing from the class.
         */
        void rectify();

};

} // namespace model


#endif
