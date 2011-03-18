// Copyright 2011 Google Inc.
// header for the exceptions subsystem of the runtime

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

enum HookId {
    exceptionMatchFuncHook,
    badCastFuncHook,
    exceptionReleaseFuncHook,
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
};

extern RuntimeHooks runtimeHooks;

/**
 * Crack-importable function to allow you to register a hook.
 */
void registerHook(HookId hookId, void *hook);

}} // namespace crack::runtime
