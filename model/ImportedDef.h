// Copyright 2012 Google Inc.

#ifndef _model_ImportedDef_h_
#define _model_ImportedDef_h_

#include <string>
#include <vector>

namespace model {

// stores the names for an imported definition.
struct ImportedDef {
    // the "local name" is the alias that a symbol is imported under.
    // the "source name" is the unqualified name that the definition has in
    // the module we are importing it from.
    std::string local, source;

    ImportedDef(const std::string &local, const std::string &source) :
        local(local),
        source(source) {
    }

    /**
     * Initialize both the local and source names to "name".
     */
    ImportedDef(const std::string &name) :
        local(name),
        source(name) {
    }
};

typedef std::vector<ImportedDef> ImportedDefVec;

} // namespace model

#endif
