
lib_LTLIBRARIES = libcrack.la
libcrack_la_SOURCES = \
    builder/LLVMBuilder.cc \
    parser/Token.cc \
    parser/Toker.cc \
    parser/ParseError.cc \
    parser/Parser.cc \
    model/AssignExpr.cc \
    model/Context.cc \
    model/FuncCall.cc \
    model/FuncDef.cc \
    model/IntConst.cc \
    model/OverloadDef.cc \
    model/StrConst.cc \
    model/TypeDef.cc \
    model/VarDef.cc \
    model/VarRef.cc

bin_PROGRAMS = crack

crack_SOURCES = crack.cc
crack_LDADD = libcrack.la $(LDFLAGS) @LLVM_LDFLAGS@ @LLVM_LIBS@
AM_CXXFLAGS = @LLVM_CXXFLAGS@

INCLUDES =  @SPUGPP_CXXFLAGS@

TESTS = test/run_all_tests
