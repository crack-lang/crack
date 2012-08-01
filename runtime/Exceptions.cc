// Copyright 2011-2012 Google Inc.
// Copyright 2011 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// runtime exception handling functions supporting the Itanium API

#include "Exceptions.h"

#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <dlfcn.h>

#include "debug/DebugTools.h"
#include "ItaniumExceptionABI.h"
#include "BorrowedExceptions.h"

using namespace std;
using namespace crack::runtime;

namespace crack { namespace runtime {

// per-thread exception object variable and a "once" variable to allow us to 
// initialize it.
static pthread_key_t exceptionObjectKey;
static pthread_once_t exceptionObjectKeyOnce = PTHREAD_ONCE_INIT;

void deleteException(_Unwind_Exception *exc) {
    if (runtimeHooks.exceptionReleaseFunc)
        runtimeHooks.exceptionReleaseFunc(exc->user_data);
    delete exc;
    pthread_setspecific(crack::runtime::exceptionObjectKey, 0);
}

void initExceptionObjectKey() {
    int rc = pthread_key_create(
        &exceptionObjectKey, 
        reinterpret_cast<void (*)(void *)>(deleteException)
    );
    assert(rc == 0 && "Unable to create pthread key for exception object.");
}

}} // namespace crack::runtime

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

    exceptionObject->last_ip = 
        reinterpret_cast<void *>(_Unwind_GetIP(context));
    if (runtimeHooks.exceptionPersonalityFunc)
        runtimeHooks.exceptionPersonalityFunc(exceptionObject->user_data,
                                              exceptionObject->last_ip,
                                              exceptionClass,
                                              actions
                                              );

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
    if (--exc->ref_count == 0) {
        crack::runtime::deleteException(exc);
    }
}


/** Function called by the "throw" statement. */
extern "C" void __CrackThrow(void *crackExceptionObject) {
    pthread_once(&exceptionObjectKeyOnce, 
                 crack::runtime::initExceptionObjectKey
                 );
    _Unwind_Exception *uex =
        reinterpret_cast<_Unwind_Exception *>(
            pthread_getspecific(crack::runtime::exceptionObjectKey)
        );
    if (uex) {
        // we don't need an atomic reference count for these, they are thread 
        // specific.
        ++uex->ref_count;
        
        // release the original exception object XXX need to give the crack 
        // library the option to associate the old exception with the new one.
        if (runtimeHooks.exceptionReleaseFunc)
            runtimeHooks.exceptionReleaseFunc(uex->user_data);
    } else {
        // XXX it's possible for this to be called when there is a non-crack 
        // exception active.  In that case, the results are undefined.
        uex = new _Unwind_Exception();
        uex->exception_class = crackClassId;
        uex->exception_cleanup = __CrackExceptionCleanup;
        uex->ref_count = 1;
        int rc = pthread_setspecific(crack::runtime::exceptionObjectKey, uex);
        assert(rc == 0 && "unable to store exception key");
    }
    uex->user_data = crackExceptionObject;
    _Unwind_Reason_Code urc;
    if (urc = _Unwind_RaiseException(uex)) {
        cerr << "Failed to raise exception, reason code = " << urc << endl;
        abort();
    }
}

/** 
 * Function called to obtain the original crack exception object from the 
 * ABI's exception object.
 * Should only be called for a Crack exception.
 */
extern "C" void *__CrackGetException(_Unwind_Exception *uex) {
    assert(uex->exception_class == crackClassId);
    return uex->user_data;
} 

/**
 * Called at the end of a catch-clause that processes the exception.
 * Should be able to deal with exceptions thrown from any language.
 */
extern "C" void __CrackCleanupException(_Unwind_Exception *uex) {
    _Unwind_DeleteException(uex);
}

extern "C" void __CrackPrintPointer(void *pointer) {
    cerr << "pointer is: " << pointer << endl;
}

// added this as part of the work-around for a tricky exception seg-fault.
extern "C" void __CrackNOP(void *ex) {}

extern "C" void __CrackBadCast(void *curType, void *newType) {
    if (runtimeHooks.badCastFunc) {
        runtimeHooks.badCastFunc(curType, newType);
    } else {
        cerr << "Invalid class cast." << endl;
        abort();
    }
}

/** Called at every stack frame during an exception unwind. */
extern "C" void __CrackExceptionFrame() {
    if (runtimeHooks.exceptionFrameFunc) {
        _Unwind_Exception *uex =
            reinterpret_cast<_Unwind_Exception *>(
                pthread_getspecific(crack::runtime::exceptionObjectKey)
            );
        
        // if this is a crack exception, call the exception frame hook.
        if (uex)
            runtimeHooks.exceptionFrameFunc(uex->user_data, uex->last_ip);
    }
}                               

/** Called from the toplevel when there is an uncaught exception. 
 * Returns true if the exception was a crack exception.
 */
extern "C" bool __CrackUncaughtException() {
    _Unwind_Exception *uex =
        reinterpret_cast<_Unwind_Exception *>(
            pthread_getspecific(crack::runtime::exceptionObjectKey)
        );
    if (uex && uex->exception_class == crackClassId) {
        if (runtimeHooks.exceptionUncaughtFunc)
            runtimeHooks.exceptionUncaughtFunc(uex->user_data);
        return true;
    }
    
    return false;
}

namespace crack { namespace runtime {

RuntimeHooks runtimeHooks = {0};

void registerHook(HookId hookId, void *hook) {
    switch (hookId) {
        case exceptionMatchFuncHook:
            runtimeHooks.exceptionMatchFunc =
                reinterpret_cast<ExceptionMatchFunc>(hook);
            break;
        case exceptionReleaseFuncHook:
            runtimeHooks.exceptionReleaseFunc =
                reinterpret_cast<ExceptionReleaseFunc>(hook);
            break;
        case badCastFuncHook:
            runtimeHooks.badCastFunc = reinterpret_cast<BadCastFunc>(hook);
            break;
        case exceptionPersonalityFuncHook:
            runtimeHooks.exceptionPersonalityFunc =
                reinterpret_cast<ExceptionPersonalityFunc>(hook);
            break;
        case exceptionFrameFuncHook:
            runtimeHooks.exceptionFrameFunc =
                reinterpret_cast<ExceptionFrameFunc>(hook);
            break;
        case exceptionUncaughtFuncHook:
            runtimeHooks.exceptionUncaughtFunc =
                reinterpret_cast<ExceptionUncaughtFunc>(hook);
            break;
        default:
            cerr << "Unknown runtime hook specified: " << hookId << endl;
    }
}

}} // namespace crack::runtime
