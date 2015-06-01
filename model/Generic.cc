// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Generic.h"

#include <string.h>
#include "spug/check.h"
#include "CompositeNamespace.h"
#include "Context.h"
#include "Import.h"
#include "Serializer.h"
#include "Deserializer.h"
#include "GlobalNamespace.h"
#include "TypeDef.h"
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

NamespacePtr Generic::getInstanceModuleOwner(bool generic) {
    // if our enclosing module is another generic instantiation, we want to
    // use its module (we don't want to be mod.A[int].A.B[int], we want to
    // be owned by mod.A[int].B[int]).
    NamespacePtr tempNS = ns;
    if (CompositeNamespacePtr composite =
        CompositeNamespacePtr::rcast(tempNS)) {
        tempNS = composite->getParent(0);
    }

    TypeDefPtr enclosingClass;
    if ((enclosingClass = TypeDefPtr::rcast(tempNS)) && generic) {
        return enclosingClass->getOwner();
    } else {
        return ns;
    }
}

void Generic::seedCompileNS(Context &context) {
    compileNS = new GlobalNamespace(
        context.construct->rootContext->compileNS.get(),
        context.getModuleContext()->ns->getNamespaceName()
    );

    // create a subcontext for the compile-time construct
    ContextPtr ctx = context.createSubContext(Context::module, 0, 0,
                                              compileNS.get()
                                              );
    ctx->construct = context.getCompileTimeConstruct();
    for (vector<ImportPtr>::iterator i = compileNSImports.begin();
         i != compileNSImports.end();
         ++i
         )
        ctx->emitImport(compileNS.get(), (*i)->moduleName, (*i)->syms,
                        true,
                        false, // do not record
                        false // XXX raw shared lib.
                        );
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
        case Token::ident:
        case Token::floatLit:
        case Token::octalLit:
        case Token::hexLit:
        case Token::binLit:
            out.write(tok.getData(), "tokenData");
    }
    out.write(tok.getLocation().getStartCol(), "column");
}

#define TOKTXT(type, txt) case Token::type: tokText = txt; break;
Token Generic::deserializeToken(Deserializer &src, string &fileName,
                                int &lineNum
                                ) {
    Token::Type tokType = static_cast<Token::Type>(src.readUInt("tokenType"));
    string tokText;

    switch (tokType) {
        case Token::integer:
        case Token::string:
        case Token::ident:
        case Token::floatLit:
        case Token::octalLit:
        case Token::hexLit:
        case Token::binLit:
            tokText = src.readString(32, "tokenData");
            break;

        // Deal with file name/line number tokens.
        case Token::fileName:
            fileName = src.readString(Serializer::modNameSize, "fileName");
            return Token(tokType, "", Location());
        case Token::lineNumber:
            lineNum = src.readUInt("lineNum");
            return Token(tokType, "", Location());

        TOKTXT(ann, "@");
        TOKTXT(bitAnd, "&");
        TOKTXT(bitLSh, "<<");
        TOKTXT(bitOr, "|");
        TOKTXT(bitRSh, ">>");
        TOKTXT(bitXor, "^");
        TOKTXT(dollar, "$");
        TOKTXT(assign, "=");
        TOKTXT(assignAnd, "&=");
        TOKTXT(assignAsterisk, "*=");
        TOKTXT(assignLSh, "<<=");
        TOKTXT(assignOr, "|=");
        TOKTXT(assignRSh, ">>=");
        TOKTXT(assignXor, "^=");
        TOKTXT(assignMinus, "-=");
        TOKTXT(assignPercent, "%=");
        TOKTXT(assignPlus, "+=");
        TOKTXT(assignSlash, "/=");
        TOKTXT(asterisk, "*");
        TOKTXT(bang, "!");
        TOKTXT(colon, ":");
        TOKTXT(comma, ",");
        TOKTXT(decr, "--");
        TOKTXT(define, ":=");
        TOKTXT(dot, ".");
        TOKTXT(eq, "==");
        TOKTXT(ge, ">=");
        TOKTXT(gt, ">");
        TOKTXT(incr, "++");
        TOKTXT(lbracket, "[");
        TOKTXT(lcurly, "{");
        TOKTXT(le, "<=");
        TOKTXT(lparen, "(");
        TOKTXT(lt, "<");
        TOKTXT(minus, "-");
        TOKTXT(ne, "!=");
        TOKTXT(percent, "%");
        TOKTXT(plus, "+");
        TOKTXT(quest, "?");
        TOKTXT(rbracket, "]");
        TOKTXT(rcurly, "}");
        TOKTXT(rparen, ")");
        TOKTXT(semi, ";");
        TOKTXT(slash, "/");
        TOKTXT(tilde, "~");
        TOKTXT(logicAnd, "&&");
        TOKTXT(logicOr, "||");
        TOKTXT(scoping, "::");
        TOKTXT(isKw, "is");
    }
    int column = src.readUInt("column");
    Location loc = new LocationImpl(fileName.c_str(), lineNum, column,
                                    column + tokText.size()
                                    );
    return Token(tokType, tokText, loc);
}

void Generic::serialize(Serializer &out) const {
    // serialize imports
    out.write(compileNSImports.size(), "#importedAnnotations");
    for (vector<ImportPtr>::const_iterator i = compileNSImports.begin();
         i != compileNSImports.end();
         ++i
         )
        if (out.writeObject(i->get(), "importedAnnotations"))
            (*i)->serialize(out);

    // serialize the parameters
    out.write(parms.size(), "#parms");
    for (GenericParmVec::const_iterator iter = parms.begin();
         iter != parms.end();
         ++iter
         )
        out.write((*iter)->name, "parms");

    // count the number of tokens, including line number changes which are
    // stored as special tokens.  We support multiple filenames because
    // annotations let us rewrite the token stream and could inject tokens
    // from another source file.
    string fileName;
    int lineNum = -1;
    int numTokens = body.size();
    for (TokenVec::const_iterator iter = body.begin(); iter != body.end();
         ++iter
         ) {
        const Location &loc = iter->getLocation();
        if (loc.getName() != fileName) {
            ++numTokens;
            fileName = loc.getName();
        }
        if (loc.getLineNumber() != lineNum) {
            ++numTokens;
            lineNum = loc.getLineNumber();
        }
    }

    out.write(numTokens, "#tokens");
    int recount = 0;
    lineNum = -1;
    fileName = "";
    for (TokenVec::const_iterator iter = body.begin();
         iter != body.end();
         ++iter
         ) {
        const Location &loc = iter->getLocation();
        if (loc.getName() != fileName) {
            fileName = loc.getName();
            out.write(Token::fileName, "tokenType");
            out.write(fileName, "fileName");
            ++recount;
        }
        if (loc.getLineNumber() != lineNum) {
            lineNum = loc.getLineNumber();
            out.write(Token::lineNumber, "tokenType");
            out.write(lineNum, "lineNum");
            ++recount;
        }
        serializeToken(out, *iter);
        ++recount;
    }
    SPUG_CHECK(recount == numTokens,
               "Token count mismatch: expected = " << numTokens <<
                " recount =" << recount
               );

    out.write(0, "optional");
}

namespace {
    struct ImportReader : public Deserializer::ObjectReader {
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            return Import::deserialize(deser);
        }
    };
}

Generic *Generic::deserialize(Deserializer &src) {
    Generic *result = new Generic();

    // deserialize imports
    int impCount = src.readUInt("#importedAnnotations");
    ImportReader impReader;
    for (int i = 0; i < impCount; ++i)
        result->compileNSImports.push_back(
            src.readObject(impReader, "importedAnnotations").object
        );

    int parmCount = src.readUInt("#parms");
    result->parms.reserve(parmCount);
    for (int i = 0; i < parmCount; ++i)
        result->parms.push_back(new GenericParm(src.readString(32, "parm")));

    int tokCount = src.readUInt("#tokens");
    result->body.reserve(tokCount);
    string fileName;
    int lineNum;
    for (int i = 0; i < tokCount; ++i) {
        Token tok = deserializeToken(src, fileName, lineNum);
        if (tok.getType() < Token::fileName)
            result->body.push_back(tok);
    }

    src.readString(64, "optional");
    return result;
}
