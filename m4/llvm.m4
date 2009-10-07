# convert LLVM config options to macros usable from autoconf

AC_DEFUN([AM_PATH_LLVM],
[
    LLVM_CXXFLAGS=`llvm-config --cxxflags`
    LLVM_LDFLAGS=`llvm-config --ldflags`
    LLVM_LIBS=`llvm-config --libs core jit native`
    AC_SUBST(LLVM_CXXFLAGS)
    AC_SUBST(LLVM_LDFLAGS)
    AC_SUBST(LLVM_LIBS)
])
