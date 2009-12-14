
#include <sstream>
#include <stdexcept>
#include "Toker.h"
#include "ParseError.h"

using namespace std;
using namespace parser;

Location::Location() :
    name(""),
    lineNumber(0) {
}

Location::Location(const char *name, int lineNumber) :
    name(name),
    lineNumber(lineNumber) {
}

Toker::Toker(std::istream &src, const char *sourceName, int lineNumber) :
    src(src) {
    locationMap.setName(sourceName, lineNumber);
}

Token Toker::fixIdent(const string &data, const Location &loc) {
    if (data == "if")
        return Token(Token::ifKw, data, loc);
    else if (data == "else")
        return Token(Token::elseKw, data, loc);
    else if (data == "while")
        return Token(Token::whileKw, data, loc);
    else if (data == "return")
        return Token(Token::returnKw, data, loc);
    else if (data == "class")
        return Token(Token::classKw, data, loc);
    else
        return Token(Token::ident, data, 
                     locationMap.getLocation()
                     );
}

Token Toker::readToken() {
    char ch, terminator;
    enum { 
        st_none, 
        st_ident, 
        st_slash,
        st_digram,
        st_comment, 
        st_string, 
        st_escapeChar,
        st_integer
    } state = st_none;
    
    // information on the last character for digrams
    char ch1;
    Token::Type t1, t2;

    stringstream buf;
 
    while (true) {
        // read the next character from the stream
        if (!src.read(&ch, 1)) break;
  
        // if we got a newline, unconditionally increment the line number
        if (ch == '\n') 
           locationMap.incrementLineNumber();
  
        // processing varies according to state
        switch (state) {
            case st_none:
                if (isspace(ch)) {
                   ;
                } else if (isalpha(ch) || ch == '_') {
                    buf << ch;
                    state = st_ident;
                } else if (ch == '#') {
                    state = st_comment;
                } else if (ch == ';') {
                    return Token(Token::semi, ";", locationMap.getLocation());
                } else if (ch == ',') {
                    return Token(Token::comma, ",", locationMap.getLocation());
                } else if (ch == '=') {
                    ch1 = ch; t1 = Token::assign; t2 =Token::eq;
                    state = st_digram;
                } else if (ch == '!') {
                    ch1 = ch; t1 = Token::bang; t2 =Token::ne;
                    state = st_digram;
                } else if (ch == '>') {
                    ch1 = ch; t1 = Token::gt; t2 =Token::ge;
                    state = st_digram;
                } else if (ch == '<') {
                    ch1 = ch; t1 = Token::lt; t2 =Token::le;
                    state = st_digram;
                } else if (ch == '(') {
                    return Token(Token::lparen, "(", locationMap.getLocation());
                } else if (ch == ')') {
                    return Token(Token::rparen, ")", locationMap.getLocation());
                } else if (ch == '{') {
                    return Token(Token::lcurly, "{", locationMap.getLocation());
                } else if (ch == '}') {
                    return Token(Token::rcurly, "}", locationMap.getLocation());
                } else if (ch == '+') {
                    return Token(Token::plus, "+", locationMap.getLocation());
                } else if (ch == '-') {
                    return Token(Token::minus, "-", locationMap.getLocation());
                } else if (ch == '*') {
                    return Token(Token::asterisk, "*", locationMap.getLocation());
                } else if (ch == '%') {
                    return Token(Token::percent, "%", 
                                 locationMap.getLocation()
                                 );
                } else if (ch == '/') {
                    state = st_slash;
                } else if (ch == '"' || ch == '\'') {
                    terminator = ch;
                    state = st_string;
                } else if (ch == ':') {
                    ch1 = ch; t1 = Token::colon; t2 = Token::define;
                    state = st_digram;
                } else if (ch == '.') {
                    return Token(Token::dot, ".", locationMap.getLocation());
                } else if (isdigit(ch)) {
                    buf << ch;
                    state = st_integer;
                } else {
                    ParseError::abort(Token(Token::dot, "", 
                                            locationMap.getLocation()
                                            ),
                                      "unknown token"
                                      );
                }
                break;
            
            case st_digram:
                if (ch == '=') {
                    char all[3] = {ch1, ch, 0};
                    return Token(t2, all, locationMap.getLocation());
                } else {
                    char all[2] = {ch1, 0};
                    src.putback(ch);
                    return Token(t1, all, locationMap.getLocation());
                }
            

   
            case st_ident:
   
                // if we got a non-alphanumeric, non-underscore we're done
                if (!isalnum(ch) && ch != '_') {
                    src.putback(ch);
                    return fixIdent(buf.str().c_str(), 
                                    locationMap.getLocation()
                                    );
                }
    
                buf << ch;
                break;
   
            case st_slash:
                if (ch == '/') {
                    state = st_comment;
                } else {
                    src.putback(ch);
                    return Token(Token::slash, "/", locationMap.getLocation());
                }
                break;
            
            case st_comment:
   
                // newline character takes us out of the comment state
                if (ch == '\n')
                   state = st_none;
                break;
   
            case st_string:
   
                // check for the terminator
                if (ch == terminator) {
                    return Token(Token::string, buf.str().c_str(),
                                 locationMap.getLocation()
                                 );
                } else if (ch == '\\') {
                    state = st_escapeChar;
                } else {
                    buf << ch;
                }
    
                break;
   
            case st_escapeChar:
   
                switch (ch) {
                    case 't':
                        buf << '\t';
                        break;
                    case 'n':
                        buf << '\n';
                        break;
                    case 'a':
                        buf << '\a';
                        break;
                    case 'r':
                        buf << '\r';
                        break;
                    case 'b':
                        buf << '\b';
                        break;
                    default:
                        buf << ch;
                }
                state = st_string;
                break;

            case st_integer:
                if (isdigit(ch))
                    buf << ch;
                else {
                    src.putback(ch);
                    return Token(Token::integer, buf.str().c_str(),
                                 locationMap.getLocation()
                                 );
                }
    
                break;
            
            default:
               throw logic_error("tokenizer in illegal state");
        }
    }
 
    // if we got here, we got to the end of the stream, make sure it was
    // expected
    if (state == st_none)
        return Token(Token::end, "", locationMap.getLocation());
    else if (state == st_ident)
        // it's ok for identifiers to be up against the end of the stream
        return Token(Token::ident, buf.str().c_str(),
                     locationMap.getLocation()
                     );
    else
        ParseError::abort(Token(Token::end, "", locationMap.getLocation()),
                          "End of stream in the middle of a token"
                          );
}

Token Toker::getToken() {
    // if any tokens have been put back, use them first
    if (tokens.size()) {
        Token temp = tokens.back();
        tokens.pop_back();
        return temp;
    } else {
        return readToken();
    }
}
