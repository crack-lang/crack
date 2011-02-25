# Copyright 2009 Google Inc.
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
    LLVM_LIBS=`llvm-config --libs core jit native instrumentation bitwriter ipo linker`
    AC_SUBST(LLVM_CXXFLAGS)
    AC_SUBST(LLVM_CPPFLAGS)
    AC_SUBST(LLVM_LDFLAGS)
    AC_SUBST(LLVM_LIBS)
])
