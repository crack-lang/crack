
#ifndef PARSEERROR_H
#define PARSEERROR_H

#include <spug/Exception.h>

namespace parser {

class Token;

/**
 * Exception class for parsing errors.
 */
class ParseError : public spug::Exception {
    public:
        ParseError() {}
        ParseError(const char *msg) : spug::Exception(msg) {}
        ParseError(const std::string &msg) : spug::Exception(msg) {}
        virtual const char *getClassName() const { return "ParseError"; }
        
        static void abort(const Token &tok, const char *msg);

};

} // namespace parser

#endif
