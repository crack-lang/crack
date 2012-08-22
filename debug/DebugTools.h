// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_debug_DebugTools_h_
#define _crack_debug_DebugTools_h_

#include <string>

namespace crack { namespace debug {

/** Register the debug info for a function in the lookup tables. */
void registerDebugInfo(void *address, const std::string &funcName,
                       const std::string &fileName,
                       int lineNumber
                       );

/** Register an entire function table.  */
void registerFuncTable(const char **table);

/**
 * Finds the source location for the specified address.
 * Source info is an array of three elements:
 * - the function name
 * - the source file name
 * - the line number (stored as an integer in the pointer).
 * All three values may be null if the source information can not be obtained.
 */
void getLocation(void *address, const char *info[3]);

/**
 * Write the entire function table to the specified stream.
 */
void dumpFuncTable(std::ostream &out);

/**
 * Returns the address of the current stack frame.  This can mean different
 * things on different architecturs.
 */
void *getStackFrame();

}} // namespace crack::debug

#endif
