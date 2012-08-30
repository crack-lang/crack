// Copyright 2010-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Arno Rehn <arno@arnorehn.de>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_ext_Func_h_
#define _crack_ext_Func_h_

#include <string>
#include <vector>

namespace model {
    class Context;
    class TypeDef;
}

namespace crack { namespace ext {

struct Arg;
class Module;
class Type;

class Func {
    friend class Module;
    friend class Type;

    public:
        enum Flags {
            noFlags = 0,
            method = 1,
            virtualized = 2,
            variadic = 8,

            // mask for flags that map to FuncDef flags.
            funcDefFlags = 15,

            // flags that don't map to FuncDef flags.
            constructor = 1024,
            vwrap = 2048
        };

    private:
        model::Context *context;
        // receiver type gets set for methods, wrapperClass gets set for 
        // vwrapped types.
        model::TypeDef *receiverType, *wrapperClass;
        Type *returnType;
        std::string name;
        std::string symbolName;
        void *funcPtr;
        std::vector<Arg *> args;

        std::string funcBody;
        std::string ctorInitializers;

        unsigned int vtableSlot;
        
        // these must match the values in FuncDef::Flags
        Flags flags;
        bool finished;

        Func(model::Context *context, Type *returnType, std::string name, 
             void *funcPtr,
             Flags flags
             ) :
            context(context),
            receiverType(0),
            wrapperClass(0),
            returnType(returnType),
            name(name),
            funcPtr(funcPtr),
            vtableSlot(0),
            flags(flags),
            finished(false) {
        }

        Func(model::Context *context, Type *returnType, std::string name,
             const std::string& body,
             Flags flags
             ) :
            context(context),
            receiverType(0),
            wrapperClass(0),
            returnType(returnType),
            name(name),
            funcPtr(0),
            funcBody(body),
            vtableSlot(0),
            flags(flags),
            finished(false) {
        }

    public:

        // specify a symbol name for this function, which will be used in
        // low level IR to resolve calls to this function. if it's not
        // specified, at attempt is made to reverse it from the function
        // pointer
        void setSymbolName(const std::string &name) { symbolName = name; }

        // add a new argument to the function.
        void addArg(Type *type, const std::string &name);

        // sets whether the Func maps to a variadic function
        void setIsVariadic(bool isVariadic);

        // gets whether the Func maps to a variadic function
        bool isVariadic() const;
        
        // sets the "vwrap" flag, which indicates that the function is a 
        // virtual that wraps a call to another function.
        void setVWrap(bool vwrapEnabled);
        
        // gets the "vwrap" flag.
        bool getVWrap() const;
        
        // sets the "virtualized" flag
        void setVirtual(bool virtualizedEnabled);
        
        // gets the "virtualized" flag
        bool getVirtual() const;

        // sets the function body; sets funcPtr to 0
        void setBody(const std::string& body);

        // returns the function body
        std::string getBody() const;

        // sets the initializers for constructors
        void setInitializers(const std::string& initializers);

        // returns the initializers for constructors
        std::string getInitializers() const;

        // returns this method's offset in the vtable; only well-defined
        // after the containing type has been finish()'ed.
        unsigned int getVTableOffset() const;

        // finish the definition of the function (this will be called 
        // automatically by Module::finish())
        void finish();
};

inline Func::Flags operator |(Func::Flags a, Func::Flags b) {
    return static_cast<Func::Flags>(static_cast<int>(a) |
                                    static_cast<int>(b)
                                    );
}

}} // namespace crack::ext

#endif

