// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2009-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include <limits.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "Toker.h"
#include "ParseError.h"

using namespace std;
using namespace parser;

Location Toker::getLocation() {
    if (currentName == lastLoc.getName() &&
        currentLine == lastLoc.getLineNumber() &&
        currentStartCol == lastLoc.getStartCol() &&
        currentEndCol == lastLoc.getEndCol())
        return lastLoc;
    lastLoc = new LocationImpl(currentName,
                               currentLine,
                               currentStartCol,
                               currentEndCol-1);
    return lastLoc;
}

bool Toker::getChar(char &ch) {
    bool result;
    if (putbackIndex < putbackSize) {
        ch = putbackBuf[putbackIndex++];
        result = true;
    } else {
        result = src.read(&ch, 1);
    }
    currentEndCol++;
    if (result && ch == '\n') {
        currentLine++;
        saveEndCol = currentEndCol-1;
        currentStartCol = 1;
        currentEndCol = 1;
    }
    return result;
}

void Toker::ungetChar(char ch) {
    assert(putbackIndex && "Toker putback overflow");
    putbackBuf[--putbackIndex] = ch;
    if (ch == '\n') {
        currentLine--;
        currentEndCol = saveEndCol;
    } else {
        currentEndCol--;
    }
}

void Toker::initIndent(bool indented) {
    if (indented) {
        indentedString = true;
        indentLevel = 0;
        minIndentLevel = INT_MAX;
    } else {
        indentedString = false;
    }
}

void Toker::evaluateIndentation(const string &val) {
    bool inPrefix = false;
    int indent;
    for (int i = 0; i < val.size(); ++i) {
        if (val[i] == '\n') {
            inPrefix = true;
            indent = 0;
        } else if (inPrefix) {
            if (val[i] == ' ') {
                indent += 1;
            } else if (val[i] == '\t') {
                indent = (indent / tabWidth + 1) * tabWidth;
            } else if (val[i] == '\n') {
                // ignore blank lines
                indent = 0;
            } else {
                inPrefix = false;
                if (indent < minIndentLevel)
                    minIndentLevel = indent;
            }
        }
    }
    
    if (inPrefix && indent && indent < minIndentLevel)
        minIndentLevel = indent;
}

void Toker::fixIndentation(string &val) {
    string result;
    bool inPrefix = false;
    int indent;
    for (int i = 0; i < val.size(); ++i) {
        if (val[i] == '\n') {
            inPrefix = true;
            indent = 0;
        } else if (inPrefix) {
            if (val[i] == ' ')
                indent += 1;
            else if (val[i] == '\t')
                indent = (indent / tabWidth + 1) * tabWidth;
            else {
                inPrefix = false;
                if (indent >= minIndentLevel)
                    indent -= minIndentLevel;
                result.push_back('\n');
                result.append(indent, ' ');
                result.push_back(val[i]);
            }
        } else {
            result.push_back(val[i]);
        }
    }
    
    // if we ended in a prefix, flush it.
    if (inPrefix) {
        result.push_back('\n');
        if (indent >= minIndentLevel)
            indent -= minIndentLevel;
        result.append(indent, ' ');
    }
    
    val = result;
}
    
Toker::Toker(std::istream &src, const char *sourceName, int lineNumber) :
    src(src),
    state(st_none),
    putbackIndex(putbackSize),
    indentedString(false),
    currentName(sourceName),
    currentLine(lineNumber),
    currentStartCol(1),
    currentEndCol(1) {
    lastLoc = new LocationImpl(sourceName, 1, 1, 0);
}

Token Toker::fixIdent(const string &data, const Location &loc) {
    if (data == "break")
        return Token(Token::breakKw, data, loc);
    else if (data == "catch")
        return Token(Token::catchKw, data, loc);
    else if (data == "class")
        return Token(Token::classKw, data, loc);
    else if (data == "continue")
        return Token(Token::continueKw, data, loc);
    else if (data == "else")
        return Token(Token::elseKw, data, loc);
    else if (data == "if")
        return Token(Token::ifKw, data, loc);
    else if (data == "import")
        return Token(Token::importKw, data, loc);
    else if (data == "in")
        return Token(Token::inKw, data, loc);
    else if (data == "is")
        return Token(Token::isKw, data, loc);
    else if (data == "null")
        return Token(Token::nullKw, data, loc);
    else if (data == "return")
        return Token(Token::returnKw, data, loc);
    else if (data == "throw")
        return Token(Token::throwKw, data, loc);
    else if (data == "try")
        return Token(Token::tryKw, data, loc);
    else if (data == "while")
        return Token(Token::whileKw, data, loc);
    else if (data == "on")
        return Token(Token::onKw, data, loc);
    else if (data == "oper")
        return Token(Token::operKw, data, loc);
    else if (data == "for")
        return Token(Token::forKw, data, loc);
    else if (data == "typeof")
        return Token(Token::typeofKw, data, loc);
    else if (data == "const")
        return Token(Token::constKw, data, loc);
    else if (data == "module")
        return Token(Token::moduleKw, data, loc);
    else if (data == "lambda")
        return Token(Token::lambdaKw, data, loc);
    else if (data == "enum")
        return Token(Token::enumKw, data, loc);
    else if (data == "alias")
        return Token(Token::aliasKw, data, loc);
    else if (data == "case")
        return Token(Token::caseKw, data, loc);
    else if (data == "switch")
        return Token(Token::switchKw, data, loc);
    else
        return Token(Token::ident, data, 
                     getLocation()
                     );
}

Token Toker::readToken() {
    char ch, terminator;
    
    // information on the preceeding characters for compound symbols
    char symchars[4];
    int sci = 0;
    Token::Type t1, t2, t3;
    
    // for parsing octal and hex character code escape sequences.
    char codeChar;
    int codeLen;

    stringstream buf;

    // we should only be able to enter this in one of three states.
    assert((state == st_none || state == st_interpNone || state == st_istr) && 
           "readToken(): tokenizer in invalid state"
           );

    // start col is where lastLocation ended, plus one
    currentStartCol = lastLoc.getEndCol() + 1;

    while (true) {
        // read the next character from the stream
        if (!getChar(ch)) break;

        // processing varies according to state
        switch (state) {
            case st_none:
                if (ch == 'i' || ch == 'b') {
                    // deal with i'str' and b'c' tokens
                    buf << ch;
                    state = st_strint;
                    continue;
                } else if (ch == 'r') {
                    // deal with r'raw string' tokens
                    buf << ch;
                    state = st_rawStr;
                    continue;
                } else if (ch == 'I') {
                    // deal with I'indented string' tokens.
                    buf << ch;
                    state = st_indentStr;
                    continue;
                }
                // fall through to interpNone

            // the "interpolation none" state is a base state that doesn't 
            // except the augmented string tokens (e.g. i'1234', b'x'...).  
            // These produce unexpected behavior in an interpolation 
            // expression, e.g. `value = '$i'` would treat the "i'" as the 
            // beginning of an integer string token.  Whenever we come into 
            // this state, we revert the state to st_none - it's only valid 
            // after being explicitly set.
            case st_interpNone:
                state = st_none;
                if (isspace(ch)) {
                    // we increase our
                    // startCol so that when the Location is made for this
                    // token, the startCol points to the first non whitespace
                    // character
                    if (isblank(ch))
                        currentStartCol++;
                } else if (isalpha(ch) || ch == '_' || ch < 0) {
                    buf << ch;
                    state = st_ident;
                } else if (ch == '#') {
                    state = st_comment;
                } else if (ch == ';') {
                    return Token(Token::semi, ";", getLocation());
                } else if (ch == ',') {
                    return Token(Token::comma, ",", getLocation());
                } else if (ch == '=') {
                    symchars[sci++] = ch; t1 = Token::assign; t2 =Token::eq;
                    state = st_digram;
                } else if (ch == '!') {
                    symchars[sci++] = ch; t1 = Token::bang; t2 =Token::ne;
                    state = st_digram;
                } else if (ch == '>') {
                    symchars[sci++] = ch; t1 = Token::gt; t2 = Token::ge; 
                    t3 = Token::bitRSh;
                    state = st_ltgt;
                } else if (ch == '<') {
                    symchars[sci++] = ch; t1 = Token::lt; t2 =Token::le;
                    t3 = Token::bitLSh;
                    state = st_ltgt;
                } else if (ch == '(') {
                    return Token(Token::lparen, "(", getLocation());
                } else if (ch == ')') {
                    return Token(Token::rparen, ")", getLocation());
                } else if (ch == '{') {
                    return Token(Token::lcurly, "{", getLocation());
                } else if (ch == '}') {
                    return Token(Token::rcurly, "}", getLocation());
                } else if (ch == '[') {
                    return Token(Token::lbracket, "[", 
                                 getLocation()
                                 );
                } else if (ch == ']') {
                    return Token(Token::rbracket, "]",
                                 getLocation()
                                 );
                } else if (ch == '$') {
                    return Token(Token::dollar, "$", 
                                 getLocation()
                                 );
                } else if (ch == '+') {
                    state = st_plus;
                } else if (ch == '-') {
                    state = st_minus;
                } else if (ch == '&') {
                    state = st_amp;
                } else if (ch == '|') {
                    state = st_pipe;
                } else if (ch == '*') {
                    t1 = Token::asterisk;
                    t2 = Token::assignAsterisk;
                    symchars[sci++] = ch;
                    state = st_postaug;
                } else if (ch == '%') {
                    t1 = Token::percent; t2 = Token::assignPercent;
                    symchars[sci++] = ch;
                    state = st_postaug;
                } else if (ch == '/') {
                    state = st_slash;
                } else if (ch == '^') {
                    t1 = Token::bitXor;
                    t2 = Token::assignXor;
                    symchars[sci++] = ch;
                    state = st_postaug;
                } else if (ch == '"' || ch == '\'') {
                    terminator = ch;
                    initIndent(false);
                    state = st_string;
                    t1 = Token::string;
                } else if (ch == ':') {
                    symchars[sci++] = ch; t1 = Token::colon; 
                    t2 = Token::define;
                    t3 = Token::scoping;
                    state = st_digram;
                } else if (ch == '.') {
                    state = st_period;
                } else if (ch == '@') {
                    return Token(Token::ann, "@", getLocation());
                } else if (isdigit(ch)) {
                    if (ch == '0') {
                        state = st_zero;
                    } else {
                        // [1-9]
                        buf << ch;
                        state = st_number;
                    }
                } else if (ch == '~') {
                    return Token(Token::tilde, "~", getLocation());
                } else if (ch == '`') {
                    initIndent(false);
                    state = st_istr;
                    return Token(Token::istrBegin, "`", 
                                 getLocation()
                                 );
                } else if (ch == '?') {
                    return Token(Token::quest, "?", getLocation());
                } else {
                    ParseError::abort(Token(Token::dot, "", 
                                            getLocation()
                                            ),
                                      "unknown token"
                                      );
                }
                break;

            case st_amp:
                state = st_none;
                if (ch == '&') {
                    return Token(Token::logicAnd, "&&",
                                 getLocation()
                                 );
                } else if (ch == '=') {
                    return Token(Token::assignAnd, "&=",
                                 getLocation()
                                 );
                } else {
                    ungetChar(ch);
                    return Token(Token::bitAnd, "&", 
                                 getLocation()
                                 );
                }
                break;

            case st_pipe:
                state = st_none;
                if (ch == '|') {
                    return Token(Token::logicOr, "||",
                                 getLocation()
                                 );
                } else if (ch == '=') {
                    return Token(Token::assignOr, "|=",
                                 getLocation()
                                 );
                } else {
                    ungetChar(ch);
                    return Token(Token::bitOr, "|", 
                                 getLocation()
                                 );
                }
                break;

            case st_minus:
                state = st_none;
                if (ch == '-') {
                    return Token(Token::decr, "--", getLocation());
                } else if (ch == '=') {
                    return Token(Token::assignMinus, "-=",
                                 getLocation()
                                 );
                } else {
                    ungetChar(ch);
                    return Token(Token::minus, "-", getLocation());
                }
                break;

            case st_ltgt:
                if (ch == symchars[0]) {
                    symchars[sci++] = ch;
                    t1 = t3;
                    t2 = (ch == '<') ? Token::assignLSh : Token::assignRSh;
                    state = st_postaug;
                    break;
                }
                // fall through to digram

            case st_digram:
                state = st_none;
                if (ch == '=') {
                    symchars[sci++] = ch;
                    symchars[sci++] = 0;
                    return Token(t2, symchars, getLocation());
                } else if (ch == ':' && t3 == Token::scoping) {
                    return Token(t3, "::", getLocation());
                } else {
                    symchars[1] = 0;
                    ungetChar(ch);
                    return Token(t1, symchars, getLocation());
                }
                break;
            
            case st_period:
                // check for float
                if (isdigit(ch)) {
                    state = st_float;
                    buf << "." << ch;
                }
                else {
                    ungetChar(ch);
                    state = st_none;
                    return Token(Token::dot, ".", getLocation());
                }
                break;

            // integer or byte coded as a string
            case st_strint:
                if (ch == '"' || ch == '\'') {
                    // store a slash to disambiguate this from a hex value 
                    // with a leading 'b'.
                    buf << '/';
                    state = st_string;
                    t1 = Token::integer;
                    terminator = ch;
                    break;
                }
                // fall through to ident processing via rawStr
            
            // raw string
            case st_rawStr:
                if (ch == '"' || ch == '\'') {
                    state = st_rawStrBody;
                    terminator = ch;
                    buf.str("");
                    break;
                }
                // fall through to ident processing via st_indentStr

            // indented string
            case st_indentStr:
                if (ch == '"' || ch == '\'') {
                    state = st_string;
                    initIndent(true);
                    terminator = ch;
                    buf.str("");
                    t1 = Token::string;
                    break;
                } else if (state == st_indentStr && ch == '`') {
                    state = st_istr;
                    initIndent(true);
                    return Token(Token::istrBegin, "`", 
                                 getLocation()
                                 );
                } else {
                    // last guy in the chain needs to set the state.
                    state = st_ident;
                }
                // fall through to ident processing

            case st_ident:
   
                // if we got a non-alphanumeric, non-underscore we're done
                if (!isalnum(ch) && ch != '_' && ch > 0) {
                    ungetChar(ch);
                    state = st_none;
                    return fixIdent(buf.str(), getLocation());
                }
    
                buf << ch;
                break;
   
            case st_slash:
                if (ch == '/') {
                    state = st_comment;
                } else if (ch == '=') {
                    state = st_none;
                    return Token(Token::assignSlash, "/=", 
                                 getLocation()
                                 );
                } else if (ch == '*') {
                    state = st_ccomment;
                } else {
                    ungetChar(ch);
                    state = st_none;
                    return Token(Token::slash, "/", getLocation());
                }
                break;
            
            case st_comment:
   
                // newline character takes us out of the comment state
                if (ch == '\n')
                   state = st_none;
                break;
            
            case st_ccomment:
                if (ch == '*')
                    state = st_ccomment2;
                break;
            
            case st_ccomment2:
                if (ch == '/')
                    state = st_none;
                else if (ch != '*')
                    state = st_ccomment;
                break;
   
            case st_string:
   
                // check for the terminator
                if (ch == terminator) {
                    state = st_none;
                    string val = buf.str();
                    if (indentedString)
                        reindent(val);
                    return Token(t1, val, getLocation());
                } else if (ch == '\\') {
                    state = st_strEscapeChar;
                } else {
                    buf << ch;
                }
    
                break;
   
            case st_strEscapeChar:
            case st_istrEscapeChar:
   
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
                    case 'x':
                        state = (state == st_strEscapeChar) ?
                                    st_strHex :
                                    st_istrHex;
                        codeChar = codeLen = 0;
                        break;
                    case '\n':
                        if (indentedString) {
                            indentLevel = 0;
                            state = (state == st_strEscapeChar) ?
                                st_strEscapedIndentedNewline :
                                st_istrEscapedIndentedNewline;
                        }
                        break;
                    default:
                        if (isdigit(ch) && ch < '8') {
                            codeChar = ch - '0';
                            codeLen = 1;
                            state = (state == st_strEscapeChar) ?
                                        st_strOctal :
                                        st_istrOctal;
                        } else {
                            buf << ch;
                        }
                }
                
                // if we haven't moved on to one of the character code states, 
                // return to the normal string processing state
                if (state == st_strEscapeChar)
                    state = st_string;
                else if (state == st_istrEscapeChar)
                    state = st_istr;
                break;
            
            case st_strEscapedIndentedNewline:
            case st_istrEscapedIndentedNewline:
                if (ch == ' ') {
                    ++indentLevel;
                } else if (ch == '\t') {
                    indentLevel = (indentLevel / tabWidth + 1) * tabWidth;
                } else {
                    ungetChar(ch);
                    if (indentLevel < minIndentLevel)
                        minIndentLevel = indentLevel;
                    state = (state == st_strEscapedIndentedNewline) ?
                                st_string : st_istr;
                                
                }
                break;

            case st_strOctal:
            case st_istrOctal:
                
                if (isdigit(ch) && ch < '8' && codeLen < 3) {
                    codeChar = (codeChar << 3) | (ch - '0');
                    ++codeLen;
                } else {
                    buf << codeChar;
                    ungetChar(ch);
                    state = (state == st_strOctal) ? st_string : st_istr;
                }
                break;
            
            case st_strHex:
            case st_istrHex:
                
                if (isdigit(ch)) {
                    ch = ch - '0';
                } else if (ch >= 'a' && ch <= 'f') {
                    ch = ch - 'a' + 10;
                } else if (ch >= 'A' && ch <= 'F') {
                    ch = ch - 'A' + 10;
                } else {
                    ParseError::abort(Token(Token::string, buf.str(),
                                            getLocation()
                                            ),
                                      "invalid hex code escape sequence (must "
                                       "be two hex digits)"
                                      );
                }

                codeChar = (codeChar << 4) | ch;
                ++codeLen;
                
                if (codeLen == 2) {
                    buf << codeChar;
                    state = (state == st_strHex) ? st_string : st_istr;
                }
                break;

            case st_binary:
                if (ch == '0' || ch == '1')
                    buf << ch;
                else {
                    ungetChar(ch);
                    if (buf.str().size() == 0) {
                        ParseError::abort(Token(Token::string, buf.str(),
                                                getLocation()
                                                ),
                                          "invalid binary constant"
                                          );
                    }
                    state = st_none;
                    return Token(Token::binLit,
                                 buf.str(),
                                 getLocation()
                                 );
                }
                break;

            case st_rawStrBody:
                // check for the terminator
                if (ch == terminator) {
                    state = st_none;
                    return Token(Token::string, buf.str(), 
                                 getLocation()
                                 );
                }

                buf << ch;
                if (ch == '\\')
                    state = st_rawStrEscape;

                break;
            
            case st_rawStrEscape:
                // the only thing special about escape chars in raw strings is 
                // that they can't preceed a terminator.  This is how python 
                // does it, I'm not sure why, but barring compelling reasons 
                // to do anything else...
                buf << ch;
                state = st_rawStrBody;
                break;

            case st_octal:
                if (ch >= '0' && ch <= '7')
                    buf << ch;
                else {
                    ungetChar(ch);
                    if (buf.str().size() == 0) {
                        ParseError::abort(Token(Token::string, buf.str(),
                                                getLocation()
                                                ),
                                          "invalid octal constant"
                                          );
                    }
                    state = st_none;
                    return Token(Token::octalLit,
                                 buf.str(),
                                 getLocation()
                                 );
                }
                break;

            case st_hex:
                if (isxdigit(ch))
                    buf << ch;
                else {
                    ungetChar(ch);
                    if (buf.str().size() == 0) {
                        ParseError::abort(Token(Token::string, buf.str(),
                                                getLocation()
                                                ),
                                          "invalid hex constant"
                                          );
                    }
                    state = st_none;
                    return Token(Token::hexLit,
                                 buf.str(),
                                 getLocation()
                                 );
                }
                break;

            case st_zero:
                // got a zero, check for hex, octal, and binary constants
                if (ch == 'x' || ch == 'X') {
                    state = st_hex; // eats the 'x', ready to parse
                                    // first hex digit
                } else if (ch == 'o' || ch == 'O') {
                    state = st_octal; // eats the 'o', ready to parse
                                      // first octal digit
                    // since strtol expects old style of octal, we
                    // add the leading 0
                    buf << '0';
                } else if (ch == 'b' || ch == 'b') {
                    state = st_binary; // eats the 'b', ready to parse
                                       // first binary digit
                } else if (ch == '.') {
                    buf << ch;
                    state = st_float; // float
                } else if (isdigit(ch)) {
                    // old school style octal
                    state = st_octal;
                    ungetChar(ch); // need to read this in octal state
                                   // will fail there if it's > 7
                } else {
                    ungetChar(ch);
                    state = st_none;
                    return Token(Token::integer, "0", 
                                 getLocation()
                                 );
                }
                break;

            case st_number:
                if (isdigit(ch)) {
                    buf << ch;
                } else if (ch == '.') {
                    state = st_intdot;
                } else if (ch == 'e' || ch == 'E') {
                    buf << ch;
                    state = st_exponent;
                } else {
                    ungetChar(ch);
                    state = st_none;
                    return Token(Token::integer, buf.str(), 
                                 getLocation()
                                 );
                }
                break;

            case st_intdot:
                // integer followed by a period, could be a float if followed 
                // by another digit...
                if (isdigit(ch)) {
                    buf << '.' << ch;
                    state = st_float;
                } else {
                    // unget both the last character and the period since 
                    // neither is part of this integer.
                    ungetChar(ch);
                    ungetChar('.');
                    state = st_none;
                    return Token(Token::integer, buf.str(), 
                                 getLocation()
                                 );
                }
                break;

            case st_float:
                if (isdigit(ch)) {
                    buf << ch;
                } else if ((ch == 'e') || (ch == 'E')) {
                    state = st_exponent;
                    buf << ch;
                } else {
                    ungetChar(ch);
                    Token::Type tt = (state == st_float) ? Token::floatLit :
                              Token::integer;
                    state = st_none;
                    return Token(tt,
                                 buf.str(),
                                 getLocation()
                                 );
                }
                break;

            case st_exponent:
                // eat possible + or - immediately and make sure
                // we have at least one digit in exponent
                if ((ch == '+') || (ch == '-')) {
                    buf << ch;
                    state = st_exponent2;
                    break;
                }

                // fall through to exponent2...

            case st_exponent2:
                // after E+/-, make sure we got at least one digit.
                if (isdigit(ch)) {
                    buf << ch;
                    state = st_exponent3;
                } else {
                    ParseError::abort(Token(Token::string, buf.str(),
                                            getLocation()
                                            ),
                                      "invalid float specification");
                }
                break;

            case st_exponent3:
                if (isdigit(ch)) {
                    buf << ch;
                } else {
                    ungetChar(ch);
                    state = st_none;
                    return Token(Token::floatLit, buf.str(),
                                 getLocation()
                                 );
                }
                break;

            case st_istr:
                // note that we don't reindent in any of these values, 
                // reindenting of i-strings is done at the next level up.

                if (ch == '`') {
                    if (buf.tellp()) {
                        // if we've accumulated some raw data since the last 
                        // token was returned, return it as a string now and 
                        // putback the '`' so we can do the istrEnd the next 
                        // time.
                        ungetChar(ch);
                        return Token(Token::string, buf.str(),
                                     getLocation()
                                     );
                    } else {
                        state = st_none;
                        return Token(Token::istrEnd, "`",
                                    getLocation()
                                    );
                    }
                } else if (ch == '$') {
                    state = st_interpNone;
                    return Token(Token::string, buf.str(),
                                 getLocation()
                                 );
                } else if (ch == '\\') {
                    state = st_istrEscapeChar;
                } else {
                    buf << ch;
                }
                break;
            
            case st_plus:
                state = st_none;
                if (ch == '+') {
                    return Token(Token::incr, "++", getLocation());
                } else if (ch == '=') {
                    return Token(Token::assignPlus, "+=", 
                                 getLocation()
                                 );
                } else {
                    ungetChar(ch);
                    return Token(Token::plus, "+", getLocation());
                }
                break;
            
            // we just scanned a sequence of characters that can be used for 
            // augmented assignment.
            case st_postaug:
                state = st_none;
                if (ch == '=') {
                    symchars[sci++] = ch;
                    symchars[sci++] = 0;
                    return Token(t2, symchars, getLocation());
                } else {
                    symchars[sci++] = 0;
                    ungetChar(ch);
                    return Token(t1, symchars, getLocation());
                }
                break;
            
            default:
               throw logic_error("tokenizer in illegal state");
        }
    }
 
    // if we got here, we got to the end of the stream, make sure it was
    // expected
    if (state == st_none || state == st_comment) {
        state = st_none;
        return Token(Token::end, "", getLocation());
    } else if (state == st_ident) {
        // it's ok for identifiers to be up against the end of the stream
        state = st_none;
        return Token(Token::ident, buf.str(), getLocation());
    } else {
        ParseError::abort(Token(Token::end, "", getLocation()),
                          "End of stream in the middle of a token"
                          );
    }
}

Token Toker::getToken() {
    // if any tokens have been put back, use them first
    if (tokens.size()) {
        Token temp = tokens.back();
        tokens.pop_back();
        
        // if we're currently in the i-string state, leave it if we don't pass
        // a string.  If we're not in it and we've got an i-string begin, get 
        // in it.
        if (state == st_istr && !temp.isString() && !temp.isIstrBegin())
            state = st_none;
        else if (temp.isIstrBegin())
            state = st_istr;
        return temp;
    } else {
        vector<Token> toks;
        toks.push_back(readToken());

        // we want to read all of the i-string tokens as a batch so that we 
        // can apply the indentation transforms to them all collectively. 
        if (indentedString && toks.front().isIstrBegin()) {
            
            // we have to approximate the parser here so that we can go back 
            // into i-string mode after parsing an expression
            int parens = 0;
            while (!toks.back().isIstrEnd() && !toks.back().isEnd()) {
                Token &tok = toks.back();
                if (parens) {
                    if (tok.isLParen()) {
                        ++parens;
                    } else if (tok.isRParen()) {
                        --parens;
                        if (!parens)
                            continueIString();
                    } else if (tok.isIstrBegin()) {
                        ParseError::abort(tok,
                                          "Nested i-strings are not "
                                          "currently supported."
                                          );
                    }
                } else if (tok.isLParen()) {
                    ++parens;
                } else if (tok.isIdent()) {
                    continueIString();
                }
                toks.push_back(readToken());
            }
            
            for (int i = 0; i < toks.size(); ++i)
                if (toks[i].isString())
                    evaluateIndentation(toks[i].data);
            for (int i = 0; i < toks.size(); ++i)
                if (toks[i].isString())
                    fixIndentation(toks[i].data);
            
            // push everything but the first token
            for (int i = toks.size() - 1; i; --i)
                tokens.push_back(toks[i]);
        }
        return toks[0];
    }
}
