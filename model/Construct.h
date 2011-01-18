// Copyright 2010 Google Inc.

#ifndef _model_Construct_h_
#define _model_Construct_h_

#include "ModuleDef.h"

namespace builder {
    SPUG_RCPTR(Builder);
}

namespace model {

SPUG_RCPTR(ModuleDef);

SPUG_RCPTR(Construct);
SPUG_RCPTR(StrConst);

/**
 * Construct is a bundle containing the builder and all of the modules created 
 * using the builder.  It serves as a module cache and a way of associated the 
 * cache with a Builder that can create new modules.
 * 
 * A crack executor will contain either one or two Constructs - there is one 
 * for the program being executed and there may be different one.
 */
class Construct : public spug::RCBase {
    public: // XXX should be private
        // if non-null, this is the alternate construct used for annotations.  
        // If it is null, either this _is_ the annotation construct or both 
        // the main program and its annotations use the same construct.
        ConstructPtr annotationConstruct;
    
        // the toplevel builder        
        builder::BuilderPtr rootBuilder;

        // .builtin module, containing primitive types and functions
        model::ModuleDefPtr builtinMod;

        // mapping from the canonical name of the module to the module 
        // definition.
        typedef std::map<std::string, model::ModuleDefPtr> ModuleMap;
        ModuleMap moduleCache;

        // list of all modules in the order that they were loaded.
        std::vector<model::ModuleDefPtr> loadedModules;

    public:        
        
        // global string constants
        typedef std::map<std::string, StrConstPtr> StrConstTable;
        StrConstTable strConstTable;
        
        // built-in types.
        TypeDefPtr classType,
                   voidType,
                   voidptrType,
                   boolType,
                   byteptrType,
                   byteType,
                   int32Type,
                   int64Type,
                   uint32Type,
                   uint64Type,
                   intType,
                   uintType,
                   float32Type,
                   float64Type,
                   floatType,
                   vtableBaseType,
                   objectType,
                   stringType,
                   staticStringType,
                   overloadType,
                   crackContext;
        
};

} // namespace model

#endif

