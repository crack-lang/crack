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
                // if we have a parent, create a canonical name
                // based on it and the given cName. if cName is empty
                // though (think local subcontext in a module), we use
                // just the parent name. if we don't have a parent,
                // we just use cName
                Namespace((parent && !parent->getNamespaceName().empty())?
                        ((cName.empty())?parent->getNamespaceName() :
                                         parent->getNamespaceName()+"."+cName) :
                               cName),
                parent(parent) {}
        virtual NamespacePtr getParent(unsigned index);
};

} // namespace model

#endif
