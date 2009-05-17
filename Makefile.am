
#bin_PROGRAMS = crack
lib_LIBRARIES = libcrack.a
libcrack_a_SOURCES = parser/Token.cc parser/Toker.cc parser/ParseError.cc

noinst_PROGRAMS = toker_test
toker_test_SOURCES = tests/TokerTest.cc
toker_test_LDADD = @CPPUNIT_LIBS@ libcrack.a
toker_test_CXXFLAGS = @CPPUNIT_CFLAGS@
TESTS = toker_test


INCLUDES =  @SPUGPP_CXXFLAGS@

