// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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

        /** required implementation of Namespace::getModule() */
        virtual ModuleDefPtr getModule();

        virtual NamespacePtr getParent(unsigned index);
};

} // namespace model

#endif
