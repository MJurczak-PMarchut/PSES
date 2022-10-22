//============================================================================
// Name        : PSES.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#define CATCH_CONFIG_RUNNER
#include <iostream>

#include "catch.hpp"
#include "test1.hpp"

using namespace std;

TEST_CASE( "Test virtual method", "[virtual_method]" )
{
	REQUIRE(test_return1() == 1);
}

int main() {
	int result = Catch::Session().run();
	return result;
}
