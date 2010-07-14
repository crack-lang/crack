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
    model/FloatConst.cc \
    model/ModuleDef.cc \
    model/NullConst.cc \
    model/OverloadDef.cc \
    model/ResultExpr.cc \
    model/StrConst.cc \
    model/TypeDef.cc \
    model/VarDef.cc \
    model/VarRef.cc \
    Crack.cc
libcrack_la_LDFLAGS = -version-info 0:0:0

noinst_HEADERS = model/AllocExpr.h model/FloatConst.h model/FuncDef.h \
    model/ArgDef.h model/TypeDef.h model/InstVarDef.h model/Expr.h \
    model/AssignExpr.h model/StrConst.h model/VarDef.h model/IntConst.h \
    model/Branchpoint.h model/OverloadDef.h model/VarRef.h model/ModuleDef.h model/Context.h \
    model/BuilderContextData.h model/VarDefImpl.h model/NullConst.h \
    model/CleanupFrame.h model/Initializers.h model/StubDef.h \
    model/ResultExpr.h model/FuncCall.h builder/Builder.h \
    builder/LLVMBuilder.h parser/Parser.h parser/Token.h parser/LocationMap.h \
    parser/ParseError.h parser/Location.h parser/Toker.h Crack.h

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
