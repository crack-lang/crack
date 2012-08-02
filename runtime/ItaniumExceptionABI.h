// Copyright 2011 Google Inc.
// Copyright 2011 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _ItaniumExceptionABI_h_
#define _ItaniumExceptionABI_h_

#define __STDC_CONSTANT_MACROS 1
#define __STDC_LIMIT_MACROS 1

#include <stdint.h>

namespace crack { namespace runtime {

// System C++ ABI unwind types from: 
//     http://refspecs.freestandards.org/abi-eh-1.21.html
// for now we're just going to define the entire Itanium exception API here 
// (since there's no standard header file for it)

enum _Unwind_Action {
    _UA_SEARCH_PHASE = 1,
    _UA_CLEANUP_PHASE = 2,
    _UA_HANDLER_FRAME = 4,
    _UA_FORCE_UNWIND = 8
};

enum _Unwind_Reason_Code {
    _URC_NO_REASON = 0,
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_FATAL_PHASE2_ERROR = 2,
    _URC_FATAL_PHASE1_ERROR = 3,
    _URC_NORMAL_STOP = 4,
    _URC_END_OF_STACK = 5,
    _URC_HANDLER_FOUND = 6,
    _URC_INSTALL_CONTEXT = 7,
    _URC_CONTINUE_UNWIND = 8
};

struct _Unwind_Exception;

typedef void (*_Unwind_Exception_Cleanup_Fn)(_Unwind_Reason_Code reason,
                                             struct _Unwind_Exception *exc
                                             );

struct _Unwind_Exception {
    uint64_t exception_class;
    _Unwind_Exception_Cleanup_Fn exception_cleanup;
    uint64_t private_1, private_2;
    
    // crack-specific reference count and user data (points to the exception 
    // object).
    unsigned int ref_count;
    void *user_data;
    
    // the last IP address that the exception personality function got called 
    // for.
    void *last_ip;
} __attribute__((__aligned__));

struct _Unwind_Context;

extern "C" uint8_t *_Unwind_GetLanguageSpecificData(_Unwind_Context *context);
extern "C" uint64_t _Unwind_GetIP(_Unwind_Context *context);
extern "C" uint64_t _Unwind_GetRegionStart(_Unwind_Context *context);
extern "C" void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                              uint64_t new_value
                              );
extern "C" void _Unwind_SetIP(struct _Unwind_Context *context, 
                              uint64_t new_value
                              );
extern "C" _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception *ex);
extern "C" void _Unwind_DeleteException (_Unwind_Exception *ex);

// end of borrowed exception API

// some crack-specific stuff put here out of laziness.
const uint64_t crackClassId = UINT64_C(0x537075674372616b); // "SpugCrak"

}} // namespace crack::runtime

#endif
