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

enum HookId {
    exceptionMatchFuncHook,
    numHooks
};

/**
 * The runtime hooks function holds pointers to elements defined in the 
 * language that need to be used by code in the runtime.
 */
struct RuntimeHooks {
    ExceptionMatchFunc exceptionMatchFunc;
};

extern RuntimeHooks runtimeHooks;

/**
 * Crack-importable function to allow you to register a hook.
 */
void registerHook(HookId hookId, void *hook);

}} // namespace crack::runtime
