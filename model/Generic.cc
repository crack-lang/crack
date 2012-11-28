// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Generic.h"

#include <string.h>
#include "Serializer.h"
#include "Deserializer.h"
#include "parser/Toker.h"

using namespace std;
using namespace model;
using namespace parser;

GenericParm *Generic::getParm(const std::string &name) {
    for (int i = 0; i < parms.size(); ++i)
        if (parms[i]->name == name)
            return parms[i].get();

    return 0;
}

void Generic::replay(parser::Toker &toker) {
    // we have to put back the token list in reverse order.
    for (int i = body.size() - 1; i >= 0; --i)
        toker.putBack(body[i]);
}

void Generic::serializeToken(Serializer &out, const Token &tok) {
    out.write(static_cast<int>(tok.getType()), "tokenType");

    // only write data for token types where it matters
    switch (tok.getType()) {
        case Token::integer:
        case Token::string:
        case Token::floatLit:
        case Token::octalLit:
        case Token::hexLit:
        case Token::binLit:
            out.write(tok.getData(), "tokenData");
    }
    const Location &loc = tok.getLocation();
    if (out.writeObject(loc.get(), "loc")) {
        const char *name = loc.getName();
        out.write(strlen(name), name, "sourceName");
        out.write(loc.getLineNumber(), "lineNum");
    }
}

namespace {
    struct LocReader : public Deserializer::ObjectReader {
        virtual spug::RCBasePtr read(Deserializer &src) const {
            string name = src.readString(256, "sourceName");
            int lineNum = src.readUInt("lineNum");

            // we don't need to use LocationMap for this: the deserializer's object
            // map serves the same function.
            return new LocationImpl(name.c_str(), lineNum);
        }
    };
}

Token Generic::deserializeToken(Deserializer &src) {
    Token::Type tokType = static_cast<Token::Type>(src.readUInt("tokenType"));
    string tokText;
    switch (tokType) {
        case Token::integer:
        case Token::string:
        case Token::floatLit:
        case Token::octalLit:
        case Token::hexLit:
        case Token::binLit:
            tokText = src.readString(32, "tokenData");
    }
    Location loc =
        LocationImplPtr::rcast(src.readObject(LocReader(), "loc").object);
    return Token(tokType, tokText, loc);
}

void Generic::serialize(Serializer &out) const {
    // serialize the parameters
    out.write(parms.size(), "#parms");
    for (GenericParmVec::const_iterator iter = parms.begin();
         iter != parms.end();
         ++iter
         )
        out.write((*iter)->name, "parm");

    out.write(body.size(), "#tokens");
    for (TokenVec::const_iterator iter = body.begin();
         iter != body.end();
         ++iter
         )
        serializeToken(out, *iter);
}

Generic *Generic::deserialize(Deserializer &src) {
    Generic *result = new Generic();
    int parmCount = src.readUInt("#parms");
    result->parms.reserve(parmCount);
    for (int i = 0; i < parmCount; ++i)
        result->parms.push_back(new GenericParm(src.readString(32, "parm")));

    int tokCount = src.readUInt("#tokens");
    result->body.reserve(tokCount);
    for (int i = 0; i < tokCount; ++i)
        result->body.push_back(deserializeToken(src));
    return result;
}
