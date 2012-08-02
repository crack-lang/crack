// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_runtime_BorrowedExceptions_h_
#define _crack_runtime_BorrowedExceptions_h_

#include "ItaniumExceptionABI.h"

namespace crack { namespace runtime {
    
    _Unwind_Reason_Code handleLsda(int version, 
                               const uint8_t* lsda,
                               _Unwind_Action actions,
                               uint64_t exceptionClass, 
                               struct _Unwind_Exception* exceptionObject,
                               _Unwind_Context *context);
    
}} // namespace crack::runtime

#endif
