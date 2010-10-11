// Copyright 2010 Google Inc.

#ifndef _crack_ext_Func_h_
#define _crack_ext_Func_h_

#include <string>
#include <vector>

namespace model {
    class Context;
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
            virtualized = 2
        };

    private:
        model::Context *context;
        Type *returnType;
        std::string name;
        void *funcPtr;
        std::vector<Arg *> args;
        
        // these must match the values in FuncDef::Flags
        Flags flags;
        bool finished;

        Func(model::Context *context, Type *returnType, std::string name, 
             void *funcPtr,
             Flags flags
             ) :
            context(context),
            returnType(returnType),
            name(name),
            funcPtr(funcPtr),
            flags(flags),
            finished(false) {
        }

    public:
        // add a new argument to the function.
        void addArg(Type *type, const std::string &name);

        // finish the definition of the function (this will be called 
        // automatically by Module::finish())
        void finish();
};

inline Func::Flags operator |(Func::Flags a, Func::Flags b) {
    return static_cast<Func::Flags>(static_cast<Func::Flags>(a) |
                                    static_cast<Func::Flags>(b)
                                    );
}

}} // namespace crack::ext

#endif

