# Copyright 2003 Michael A. Muller
# Portions Copyright 2009 Google Inc.

lib_LTLIBRARIES = libcrack.la
libcrack_la_SOURCES = \
    builder/LLVMBuilder.cc \
    parser/Token.cc \
    parser/Toker.cc \
    parser/ParseError.cc \
    parser/Parser.cc \
    model/AllocExpr.cc \
    model/AssignExpr.cc \
    model/CleanupFrame.cc \
    model/Context.cc \
    model/Expr.cc \
    model/FuncCall.cc \
    model/FuncDef.cc \
    model/Initializers.cc \
    model/IntConst.cc \
    model/ModuleDef.cc \
    model/NullConst.cc \
    model/OverloadDef.cc \
    model/ResultExpr.cc \
    model/StrConst.cc \
    model/TypeDef.cc \
    model/VarDef.cc \
    model/VarRef.cc \
    Crack.cc

bin_PROGRAMS = crack

crack_SOURCES = crack.cc
crack_LDADD = libcrack.la $(LDFLAGS) @LLVM_LDFLAGS@ @LLVM_LIBS@
# use LLVM flags, but override their "--no-exceptions" flag.
AM_CXXFLAGS = @LLVM_CXXFLAGS@ -fexceptions

# install under prefix/lib instead of libdir so it's not platform dependent.
cracklib = ${prefix}/lib/crack-${VERSION}
AM_CPPFLAGS = -DCRACKLIB=\"${cracklib}\"

INCLUDES =  @SPUGPP_CXXFLAGS@

TESTS = test/run_all_tests

install-cracklib:
	$(INSTALL) -d $(cracklib)/crack
	$(INSTALL) lib/crack/lang.crk $(cracklib)/crack
	$(INSTALL) lib/crack/io.crk $(cracklib)/crack
