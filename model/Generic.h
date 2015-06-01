// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Generic_h_
#define _model_Generic_h_

#include "parser/Token.h"
#include "GenericParm.h"
#include "Namespace.h"
#include "VarDef.h"

namespace parser {
    class Toker;
}

namespace model {

class Deserializer;
SPUG_RCPTR(Import);
class Serializer;

SPUG_RCPTR(Namespace);

/** Stores information used to replay a generic. */
class Generic {
    public:
        // the generic parameters
        GenericParmVec parms;
        
        // the body of the generic, stored in reverse order.
        typedef std::vector<parser::Token> TokenVec;
        TokenVec body;
        
        // the original context Namespace and compile namespace
        NamespacePtr ns, compileNS;
        
        // The list of compile namespace imports.
        std::vector<ImportPtr> compileNSImports;
        
        /** Add the token to the body. */
        void addToken(const parser::Token &tok) {
            body.push_back(tok);
        }
        
        /** Get the named parameter.   Returns null if it is not defined. */
        GenericParm *getParm(const std::string &name);
        
        /** 
         * Returns the owner namespace for ephemeral modules that instantiate 
         * the generic.
         * @param generic set to true if we are in a generic context.
         */
        NamespacePtr getInstanceModuleOwner(bool generic);

        /** 
         * Create the compile namespace for the generic by replaying the 
         * imports in compileNSImports.
         */
        void seedCompileNS(Context &context);
        
        /** Replay the body into the tokenizer. */
        void replay(parser::Toker &toker);

        /** Serialization API. */
        /** @{ */
        void serialize(Serializer &out) const;
        static Generic *deserialize(Deserializer &src);
        /** @} */

        /** Token serialization (public for testability) */
        /** @{ */
        static void serializeToken(Serializer &out, const parser::Token &tok);
        static parser::Token deserializeToken(Deserializer &src,
                                              std::string &fileName,
                                              int &lineNum
                                              );
        /** @} */
};

} // namespace model

#endif
