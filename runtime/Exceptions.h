// Copyright 2011 Google Inc.
// Copyright 2011 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// header for the exceptions subsystem of the runtime

#define __STDC_CONSTANT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#include <stdint.h>

namespace crack { namespace runtime {

/**
 * The exception match function is called to check if an exception object is a 
 * member of the specified exception type.
 * @param exceptionType the crack exception's class object.
 * @param exceptionObject the crack exception thrown (the value passed in the 
 *  "throw" clause.
 */
typedef int (*ExceptionMatchFunc)(void *exceptionType, void *exceptionObject);

/**
 * The bad cast function is called when a typecast is called on an object that 
 * is not an instance of the type we're casting it to.
 */
typedef void (*BadCastFunc)(void *curClass, void *newClass);

/**
 * The exception release function is called during exception cleanup to allow 
 * the library to do an "oper release" on the exception object.
 */
typedef void (*ExceptionReleaseFunc)(void *crackExceptionObj);

/**
 * This function is called when the runtime's exception personality function 
 * is called on a crack exception during a cleanup or exception handler action.
 * It is used to give the library a chance to attach stack trace information 
 * to the exception.
 * @param address the IP address in the current exception frame that the
 *          was passed to the original personality function.
 * @param itaniumExceptionClass an identifier defining the language that raised 
 *          the exception
 * @param actions the set of actions that the personality function was called 
 *          with.
 */
typedef void (*ExceptionPersonalityFunc)(void *crackExceptionObj,
                                         void *address,
                                         uint64_t itaniumExceptionClass,
                                         int actions
                                         );

/**
 * This function is called when we the system leaves a frame during exception 
 * processing.  Unlike the exception personality function, it is guaranteed to 
 * be called only once per frame.
 */
typedef void (*ExceptionFrameFunc)(void *crackExceptionObj,
                                   void *adddress
                                   );

/**
 * Called by the toplevel exception handler for an exception that was not 
 * caught.
 */
typedef void (*ExceptionUncaughtFunc)(void *crackExceptionObj);

enum HookId {
    exceptionMatchFuncHook,
    badCastFuncHook,
    exceptionReleaseFuncHook,
    exceptionPersonalityFuncHook,
    exceptionFrameFuncHook,
    exceptionUncaughtFuncHook,
    numHooks
};

/**
 * The runtime hooks function holds pointers to elements defined in the 
 * language that need to be used by code in the runtime.
 */
struct RuntimeHooks {
    ExceptionMatchFunc exceptionMatchFunc;
    BadCastFunc badCastFunc;
    ExceptionReleaseFunc exceptionReleaseFunc;
    ExceptionPersonalityFunc exceptionPersonalityFunc;
    ExceptionFrameFunc exceptionFrameFunc;
    ExceptionUncaughtFunc exceptionUncaughtFunc;
};

extern RuntimeHooks runtimeHooks;

/**
 * Crack-importable function to allow you to register a hook.
 */
void registerHook(HookId hookId, void *hook);

}} // namespace crack::runtime
