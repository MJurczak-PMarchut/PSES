//============================================================================
// Name        : PSES.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#define CATCH_CONFIG_RUNNER
#include <iostream>

//#include <stdint.h>
#include "Std_Types.h"

#include "catch.hpp"
#include "test1.hpp"

using namespace std;

Std_ReturnType Det_ReportRuntimeError(uint16 moduleId,
                                      uint8 instanceId,
                                      uint8 apiId,
                                      uint8 errorId)
{
    (void)moduleId;
    (void)instanceId;
    (void)apiId;
    (void)errorId;

    return E_OK;
}

TEST_CASE( "Test virtual method", "[virtual_method]" )
{
	REQUIRE(test_return1() == 1);
}

int main() {
	int result = Catch::Session().run();
	return result;
}
