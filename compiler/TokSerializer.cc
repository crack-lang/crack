// Copyright 2017 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "TokSerializer.h"

#include <string.h>

#include <sstream>

#include "spug/stlutil.h"
#include "model/Deserializer.h"
#include "model/Generic.h"
#include "model/Serializer.h"
#include "model/TypeDef.h"  // for RCPtr<TypeDef> use.
#include "parser/Location.h"
#include "parser/Token.h"
#include "compiler/Location.h"
#include "compiler/Token.h"

using namespace compiler;
using namespace model;
using namespace std;

TokSerializer::~TokSerializer() {
    // Release all of the tokens.
    SPUG_FOR(list<Token *>, iter, tokens)
        (*iter)->release();

    if (loc)
        loc->release();
}

TokSerializer *TokSerializer::create(const char *serialized,
                                     size_t size
                                     ) {
    TokSerializer *result = create();

    // Use Generic's token deserializer to extract the tokens.
    istringstream data(string(serialized, size));
    Deserializer deser(data);
    string fileName;
    int lineNum = 0;
    while (true) {
        parser::Token tok = Generic::deserializeToken(deser, fileName, lineNum);
        if (tok.isEnd()) {
            result->loc = new Location(tok.getLocation());
            break;
        }
        if (tok.getType() != parser::Token::fileName &&
            tok.getType() != parser::Token::lineNumber
            )
            result->tokens.push_back(new Token(tok));
    }

    // Deal with the pathological case where there were tokens but no end
    // token by setting the location from that of the first token.  This
    // can happen if 'serialized' was constructed from somewhere other than
    // TokSerializer::serialize().
    if (!result->loc && result->tokens.size())
        result->loc = result->tokens.front()->getLocation();

    return result;
}

// Create an empty serializer.
TokSerializer *TokSerializer::create() {
    TokSerializer *result = new TokSerializer();
    result->bind();
    return result;
}

// Insert a new token at the head of the list.
void TokSerializer::insert(TokSerializer *inst, Token *tok) {
    tok->bind();
    inst->tokens.push_front(tok);
}

// Get the next token.  Returns "null" if there is no next token (this is
// different from Context.getToken(), which returns the EOF token, but is
// generally easier to manage).
Token *TokSerializer::getToken(TokSerializer *inst) {
    if (inst->tokens.empty())
        return 0;
    Token *result = inst->tokens.front();

    // Convert "end" to null.
    if (result->isEnd())
        return 0;

    inst->tokens.pop_front();
    return result;
}

// Serialize the token to a string.  Caller takes ownership of the
// resulting memory.
// The result is prefixed by a 32-bit size.
char *TokSerializer::serialize(TokSerializer *inst) {
    // Serialize the tokens.
    ostringstream dst;
    Serializer ser(dst);
    string fileName;
    int lineNum = -1;
    SPUG_FOR(list<Token *>, iter, inst->tokens) {
        Generic::serializeFullToken(ser, fileName, lineNum, *(*iter)->rep);
        if (!inst->loc) {
            inst->loc = (*iter)->getLocation();
            if (inst->loc)
                inst->loc->bind();
        }
    }

    // Serialize the "end" token so we don't run over.
    Generic::serializeFullToken(ser,
                                fileName,
                                lineNum,
                                parser::Token(parser::Token::end, "",
                                              inst->loc ? *inst->loc->rep :
                                                          parser::Location()
                                              )
                                );

    string stringVal = dst.str();
    size_t size = stringVal.size();

    // Allocate a buffer with enough space for the size.  Store the size first.
    // Note that this stores the size in platform-dependent ordering, since
    // it's always going to be consumed in-process.
    char *result = new char[size + 4];
    *reinterpret_cast<int32_t *>(result) = size;
    memcpy(result + 4, stringVal.c_str(), size);

    return result;
}

void TokSerializer::setLocation(TokSerializer *inst, Location *loc) {
    if (inst->loc == loc)
        return;

    if (inst->loc)
        inst->loc->release();
    if (loc)
        loc->bind();
    inst->loc = loc;
}

Location *TokSerializer::getLocation(TokSerializer *inst) {
    inst->loc->bind();
    return inst->loc;
}