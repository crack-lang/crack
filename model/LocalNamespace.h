// Copyright 2009 Google Inc.

#ifndef _model_LocalNamespace_h_
#define _model_LocalNamespace_h_

#include "Namespace.h"

namespace model {

SPUG_RCPTR(LocalNamespace);

/** A single-parent namespace for block or function contexts. */
class LocalNamespace : public Namespace {
    private:
        NamespacePtr parent;

    public:
        LocalNamespace(Namespace *parent, const std::string &cName) :
                Namespace((parent)?parent->getName()+"."+cName:
                                   cName),
                parent(parent) {}
        virtual NamespacePtr getParent(unsigned index);
};

} // namespace model

#endif
