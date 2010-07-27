# Copyright 2003 Michael A. Muller
# Portions Copyright 2009 Google Inc.

lib_LTLIBRARIES = libcrack.la
libcrack_la_SOURCES = \
    builder/llvm/BFieldRef.cc \
    builder/llvm/BFuncDef.cc \
    builder/llvm/BTypeDef.cc \
    builder/llvm/Consts.cc \
    builder/llvm/FuncBuilder.cc \
    builder/llvm/Incompletes.cc \
    builder/llvm/LLVMBuilder.cc \
    builder/llvm/Ops.cc \
    builder/llvm/PlaceholderInstruction.cc \
    builder/llvm/VTableBuilder.cc \
    builder/llvm/VarDefs.cc \
    parser/Token.cc \
    parser/Toker.cc \
    parser/ParseError.cc \
    parser/Parser.cc \
    model/AllocExpr.cc \
    model/AssignExpr.cc \
    model/CleanupFrame.cc \
    model/CompositeNamespace.cc \
    model/Context.cc \
    model/Expr.cc \
    model/FuncCall.cc \
    model/FuncDef.cc \
    model/Initializers.cc \
    model/IntConst.cc \
    model/FloatConst.cc \
    model/LocalNamespace.cc \
    model/ModuleDef.cc \
    model/Namespace.cc \
    model/NullConst.cc \
    model/OverloadDef.cc \
    model/ResultExpr.cc \
    model/StrConst.cc \
    model/TypeDef.cc \
    model/VarDef.cc \
    model/VarRef.cc \
    Crack.cc
libcrack_la_LDFLAGS = -version-info 0:0:0

noinst_HEADERS = \
    builder/Builder.h \
    builder/llvm/BBranchPoint.h \
    builder/llvm/BFuncDef.h \
    builder/llvm/BModuleDef.h \
    builder/llvm/BResultExpr.h \
    builder/llvm/BTypeDef.h \
    builder/llvm/Consts.h \
    builder/llvm/FuncBuilder.h \
    builder/llvm/LLVMBuilder.h \
    builder/llvm/Ops.h \
    builder/llvm/PlaceholderInstruction.h \
    builder/llvm/VTableBuilder.h \
    builder/llvm/VarDefs.h \
    model/AllocExpr.h \
    model/ArgDef.h \
    model/AssignExpr.h \
    model/Branchpoint.h \
    model/BuilderContextData.h \
    model/CleanupFrame.h \
    model/CompositeNamespace.h \
    model/Context.h \
    model/Expr.h \
    model/FloatConst.h \
    model/FuncCall.h \
    model/FuncDef.h \
    model/Initializers.h \
    model/InstVarDef.h \
    model/IntConst.h \
    model/LocalNamespace.h \
    model/ModuleDef.h \
    model/Namespace.h \
    model/NullConst.h \
    model/OverloadDef.h \
    model/ResultExpr.h \
    model/StrConst.h \
    model/StubDef.h \
    model/TypeDef.h \
    model/VarDef.h \
    model/VarDefImpl.h \
    model/VarRef.h \
    parser/Location.h \
    parser/LocationMap.h \
    parser/ParseError.h \
    parser/Parser.h \
    parser/Token.h \
    parser/Toker.h \
    Crack.h

EXTRA_DIST = doc lib test Credits CMakeLists.txt sourceModules.txt cmake

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

doc/Manual.html: doc/Manual.nml
	@if which nml2html >/dev/null; then echo generating $@; nml2html $?; \
	else \
	   echo "To regenerate the manual, you need the nml2html tool and "; \
	   echo "the spug libraries."; \
	fi

install-data-local: doc/Manual.html
	ls -l $(prefix)/lib
	$(MKDIR_P) $(DESTDIR)$(cracklib)
	$(MKDIR_P) $(DESTDIR)$(cracklib)/crack
	$(INSTALL) $(srcdir)/lib/crack/container.crk \
	   $(DESTDIR)$(cracklib)/crack
	$(INSTALL) $(srcdir)/lib/crack/lang.crk $(DESTDIR)$(cracklib)/crack
	$(INSTALL) $(srcdir)/lib/crack/io.crk $(DESTDIR)$(cracklib)/crack
	$(MKDIR_P) $(DESTDIR)$(cracklib)/crack/exp
	$(INSTALL) $(srcdir)/lib/crack/exp/bindings.crk \
	   $(DESTDIR)$(cracklib)/crack/exp
	$(INSTALL) $(srcdir)/lib/crack/exp/gtk.crk \
	   $(DESTDIR)$(cracklib)/crack/exp
	$(INSTALL) $(srcdir)/lib/crack/exp/regex.crk \
	   $(DESTDIR)$(cracklib)/crack/exp
	$(MKDIR_P) $(DESTDIR)$(docdir)
	$(INSTALL) $(srcdir)/doc/Manual.html $(DESTDIR)$(docdir)

uninstall-local:
	rm -r $(DESTDIR)$(cracklib)
	rm -r $(DESTDIR)$(docdir)
