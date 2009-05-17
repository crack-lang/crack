
#include "Token.h"

using namespace std;
using namespace parser;

Token::Token() :
   type(Token::end) {
}

Token::Token(Type type, const char *data, const Location &loc) :
    type(type),
    data(data),
    loc(loc) {
}

