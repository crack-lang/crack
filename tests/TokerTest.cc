// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include <sstream>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include "parser/Toker.h"

using namespace parser;
using namespace std;

class TokerTests : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TokerTests);
    CPPUNIT_TEST(testBasics);
    CPPUNIT_TEST(testComments);
    CPPUNIT_TEST_SUITE_END();
    
    public:
        
        void setUp() {}
        void tearDown() {}
        
        void constructTest() {}
        
        void testBasics() {
            
            Token::Type types[] = {
                Token::ident, Token::string, Token::semi, Token::comma, 
                Token::colon, Token::dot, Token::assign, Token::lparen, 
                Token::rparen, Token::lcurly, Token::rcurly, //Token::oper, 
                Token::integer, Token::plus, Token::minus, 
                Token::asterisk, Token::slash, Token::end
            };
            const char *vals[] = {
                "ident", "test", ";", ",", ":", ".", "=", "(", ")", "{",
                "}", "100", "+", "-", "*", "/", ""
            };
            stringstream src("ident'test';,:.=(){}100+-*/\n");
            Toker toker(src, "input");
            
            for (int i = 0; i < sizeof(types) / sizeof(Token::Type); ++i) {
                Token tok = toker.getToken();
                CPPUNIT_ASSERT_EQUAL(tok.getType(), types[i]);
                CPPUNIT_ASSERT(!strcmp(tok.getData(), vals[i]));
            }
        }
    
        void testComments() {
            stringstream src("ident1 // comment\nident2");
            Toker toker(src, "input");
            Token tok = toker.getToken();
            CPPUNIT_ASSERT_EQUAL(Token::ident, tok.getType());
            CPPUNIT_ASSERT(!strcmp(tok.getData(), (const char *)"ident1"));
            tok = toker.getToken();
            CPPUNIT_ASSERT_EQUAL(Token::ident, tok.getType());
            CPPUNIT_ASSERT(!strcmp(tok.getData(), (const char *)"ident2"));
        }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TokerTests);

int main(int argc, char* argv[])
{
  // Get the top level suite from the registry
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  // Adds the test to the list of test to run
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( suite );

  // Change the default outputter to a compiler error format outputter
  runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(),
                                                       std::cerr ) );
  // Run the tests.
  bool wasSucessful = runner.run();

  // Return error code 1 if the one of test failed.
  return wasSucessful ? 0 : 1;
}
