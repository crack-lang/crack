// copyright 2011 Google Inc.

#ifndef _model_Generic_h_
#define _model_Generic_h_

#include "parser/Token.h"
#include "GenericParm.h"

namespace parser {
    class Toker;
}

namespace model {

SPUG_RCPTR(Namespace);

/** Stores information used to replay a generic. */
class Generic {
    public:
        // the generic parameters
        GenericParmVec parms;
        
        // the body of the generic, stored in reverse order.
        std::vector<parser::Token> body;
        
        // the original module Namespace
        NamespacePtr moduleNS;
        
        /** Add the token to the body. */
        void addToken(const parser::Token &tok) {
            body.push_back(tok);
        }
        
        /** Get the named parameter.   Returns null if it is not defined. */
        GenericParm *getParm(const std::string &name);
        
        /** Replay the body into the tokenizer. */
        void replay(parser::Toker &toker);
};

} // namespace model

#endif
