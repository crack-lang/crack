// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BaseMetaClass.h"

#include <string>
#include <sstream>

#include "builder/Builder.h"
#include "parser/Parser.h"
#include "parser/ParseError.h"
#include "parser/Toker.h"
#include "CompositeNamespace.h"
#include "Context.h"

using namespace std;

namespace model {

void populateBaseMetaClass(Context &context) {
    int lineNum = __LINE__ + 1;
    string temp("    byteptr name;\n"
                "    uint numBases;\n"
                "    array[Class] bases;\n"
                "    array[intz] offsets;\n"
                "    uint numVTables;\n"

                // 'vtables' is of size numVTables * 2.  It's really a two
                // dimensional array alternating pointer to vtable with
                // the instance root offset for that vtable.
                "\n\n\n\n\n"
                "    array[voidptr] vtables;\n"
                "    bool isSubclass(Class other) {\n"
                "        if (this is other)\n"
                "            return (1==1);\n"
                "        uint i;\n"
                "        while (i < numBases) {\n"
                "            if (bases[i].isSubclass(other))\n"
                "                return (1==1);\n"
                "            i = i + uint(1);\n"
                "        }\n"
                "        return (1==0);\n"
                "    }\n"
                "    intz getInstOffset(Class other) {\n"
                "        if (this is other)\n"
                "            return 0;\n"
                "        uint i;\n"
                "        for (int i = 0; i < numBases; ++i) {\n"
                "            off := bases[i].getInstOffset(other);\n"
                "            if (off >= 0)\n"
                "                return offsets[i] + off;\n"
                "        }\n"
                "        return -1;\n"
                "    }\n"

                    // Returns the offset of the instance body of
                    // 'ancestor' in 'cls', returns -1 if 'ancestor' is
                    // not an ancestor of the class.
                "\n\n\n\n\n"  // newlines to adjust for the lines above.
                "    int findAncestorOffset(Class ancestor) {\n"
                "        if (this is ancestor)\n"
                "            return 0;\n"
                "        for (int i = 0; i < numBases; ++i) {\n"
                "            int baseResult =\n"
                "                bases[i].findAncestorOffset(ancestor);\n"
                "            if (baseResult != -1)\n"
                "                return offsets[i] + baseResult;\n"
                "        }\n"
                "        \n"
                "        return -1;\n"
                "    }\n"

                    // Returns the offset to the instance root for the specific vtable.
                    "\n\n\n"
                "    intz findInstanceRootOffset(voidptr vtable) {\n"
                "        for (int i = 0; i < numVTables; ++i) {\n"
                "            if (vtables[i * 2] is vtable)\n"
                "                return uintz(vtables[i * 2 + 1]);\n"
                "        }\n"
                "        return -1;\n"
                "    }\n"

                    // Returns the "instance root" which is the pointer
                    // to the beginning of the most specific class
                    // encompassing 'inst'.
                "\n\n\n\n\n"
                "    byteptr findInstanceRoot(voidptr inst) {\n"
                "        voidptr vtable = array[voidptr](inst)[0];\n"
                "        offset := findInstanceRootOffset(vtable);\n"
                "        return byteptr(inst) + -offset;\n"
                "    }\n"

                    // Return a pointer to the root of 'cls' in the
                    // instance area of 'inst'
                "\n\n\n\n"
                "    byteptr rehome(voidptr inst) {\n"
                "        if (inst is null) return null;\n"
                "        Class instClass =\n"
                "            array[array[function[Class]]](inst)[0][0]();\n"
                "        instRoot := instClass.findInstanceRoot(inst);\n"
                "        if (instRoot is null)\n"
                "            // This is actually a much bigger problem\n"
                "            // and points to memory corruption.\n"
                "            return null;\n"
                "        \n"
                "        targetOffset := instClass.findAncestorOffset(this);\n"
                "        if (targetOffset == -1)\n"
                "            return null;\n"
                "        \n"
                "        return instRoot + targetOffset;\n"
                "    }\n"
                "}\n"
                );

    TypeDef *classType = context.construct->classType.get();

    // create the class context
    ContextPtr classCtx =
        context.createSubContext(Context::instance, classType);

    CompositeNamespacePtr ns = new CompositeNamespace(classType,
                                                      context.ns.get()
                                                      );
    ContextPtr lexicalContext =
        classCtx->createSubContext(Context::composite, ns.get());

    istringstream src(temp);
    try {
        parser::Toker toker(src, "<builtin>", lineNum);
        parser::Parser p(toker, lexicalContext.get());
        p.parseClassBody();
    } catch (parser::ParseError &ex) {
        std::cerr << ex << endl;
        assert(false);
    }

    // let the "end class" emitter handle the rest of this.
    context.builder.emitEndClass(*classCtx);
}

}
