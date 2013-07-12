# Copyright 2009-2011 Google Inc.
# Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# convert LLVM config options to macros usable from autoconf
# "jit" and "native" are required for jitting, 
# 'instrumentation' is required for profiling support
# 'ipo' is required for optimizations
# 'linker', 'native', 'bitwriter' are for native binary support

AC_DEFUN([AM_PATH_LLVM],
[
    LLVM_CXXFLAGS=`llvm-config --cxxflags`
    LLVM_CPPFLAGS=`llvm-config --cppflags`
    LLVM_LDFLAGS=`llvm-config --ldflags`
    LLVM_LIBS=`llvm-config --libs core jit native instrumentation bitreader bitwriter ipo linker`
    AC_SUBST(LLVM_CXXFLAGS)
    AC_SUBST(LLVM_CPPFLAGS)
    AC_SUBST(LLVM_LDFLAGS)
    AC_SUBST(LLVM_LIBS)
])
