
#include "ParseError.h"
#include "Token.h"
#include "sstream"

using namespace std;
using namespace parser;

void ParseError::abort(const Token &tok, const char *msg) {
   Location loc = tok.getLocation();
   stringstream text;
   cout << loc.getName() << ':' << loc.getLineNumber() << ": " << msg << endl;
   text << loc.getName() << ':' << loc.getLineNumber() << ": " << msg;
   throw ParseError(text.str().c_str());
}
