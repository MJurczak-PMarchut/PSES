//============================================================================
// Name        : PSES.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

//#define CATCH_CONFIG_RUNNER
#include <iostream>

//#include "catch.hpp"
//#include "test1.hpp"
#include "acutest.h"
#include "UT_CanTP.hpp"
#include "UT_CanTp_Timers.hpp"


TEST_LIST = {
	// CanTP tests
    { "Test of CanTp_Init", test_CanTp_Init },
    { "Test of CanTP_GetNsduFromPduID", test_CanTP_GetNsduFromPduID },
    { "Test of CanTP_GetFreeNsdu", test_CanTP_GetFreeNsdu },
    { "Test of CanTP_CopyDefaultNsduConfig", test_CanTP_CopyDefaultNsduConfig },
	{ "Test of CanTp_GetVersionInfo", test_CanTp_GetVersionInfo },
	{ "Test of CanTp_GetPCI", test_CanTp_GetPCI },
	{ "Test of CanTp_RxIndication", test_CanTp_RxIndication },
	{ "Test of CanTp_RxIndicationHandleSuspendedState", test_CanTp_RxIndicationHandleSuspendedState },
	{ "Test of CanTp_FirstFrameReceived", test_CanTp_FirstFrameReceived },
	// CanTp_Timers tests
    { "Test of CanTp_TStart", test_CanTp_TStart },
	{ "Test of CanTp_Timer_Incr", test_CanTp_Timer_Incr },
	{ "Test of CanTp_Timer_Timeout", test_CanTp_Timer_Timeout },
    { NULL, NULL }
};
