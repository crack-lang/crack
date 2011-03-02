// Copyright 2011 Google Inc.
// runtime exception handling functions supporting the Itanium API

#include "Exceptions.h"

#include <assert.h>
#include <stdint.h>
#include <iostream>

#include "ItaniumExceptionABI.h"
#include "BorrowedExceptions.h"

using namespace std;
using namespace crack::runtime;

extern "C" _Unwind_Reason_Code __CrackExceptionPersonality(
    int version,
    _Unwind_Action actions,
    uint64_t exceptionClass,
    struct _Unwind_Exception *exceptionObject,
    struct _Unwind_Context *context
) {
    assert(version == 1 && "bad exception API version number");

#ifdef DEBUG
    cerr << "Exception API, got actions = " << actions <<
        " exception class: " << exceptionClass << endl;
#endif

    _Unwind_Reason_Code result;
    result =  handleLsda(version, _Unwind_GetLanguageSpecificData(context),
                         actions, 
                         exceptionClass, 
                         exceptionObject,
                         context
                         );
#ifdef DEBUG
    cerr << "got past the lsda handler: " << result << endl;
#endif

    return result;
}

static void __CrackExceptionCleanup(_Unwind_Reason_Code reason,
                                    struct _Unwind_Exception *exc
                                    ) {
    cerr << "doing exception cleanup" << endl;
// XXX need to free user data
//    delete exc->user_data;
    delete exc;
}

/** Function called by the "throw" statement. */
extern "C" void __CrackThrow(void *crackExceptionObject) {
    _Unwind_Exception *uex = new _Unwind_Exception();
    uex->exception_class = crackClassId;
    uex->exception_cleanup = __CrackExceptionCleanup;
    uex->user_data = crackExceptionObject;
    _Unwind_RaiseException(uex);
}

extern "C" void __CrackPrintPointer(void *pointer) {
    cerr << "pointer is: " << pointer << endl;
}

namespace crack { namespace runtime {

RuntimeHooks runtimeHooks = {0};

void registerHook(HookId hookId, void *hook) {
    switch (hookId) {
        case exceptionMatchFuncHook:
            runtimeHooks.exceptionMatchFunc =
                reinterpret_cast<ExceptionMatchFunc>(hook);\
            break;
        default:
            cerr << "Unknown runtime hook specified: " << hookId << endl;
    }
}

}} // namespace crack::runtime
