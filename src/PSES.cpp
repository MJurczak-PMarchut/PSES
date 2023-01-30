//============================================================================
// Name        : PSES.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "acutest.h"
#include "UT_CanTP.hpp"
#include "UT_CanTp_Timers.hpp"


TEST_LIST = {
	// CanTP tests
    { "Test of CanTp_Init", test_CanTp_Init },
    { "Test of CanTp_Shutdown", test_CanTp_Shutdown },
    { "Test of CanTP_GetNsduFromPduID", test_CanTP_GetNsduFromPduID },
    { "Test of CanTP_GetFreeNsdu", test_CanTP_GetFreeNsdu },
    { "Test of CanTP_CopyDefaultNsduConfig", test_CanTP_CopyDefaultNsduConfig },
	{ "Test of CanTp_GetVersionInfo", test_CanTp_GetVersionInfo },
	{ "Test of CanTp_GetPCI", test_CanTp_GetPCI },
	{ "Test of CanTp_RxIndication", test_CanTp_RxIndication },
	{ "Test of CanTp_RxIndicationHandleSuspendedState", test_CanTp_RxIndicationHandleSuspendedState },
	{ "Test of CanTp_FirstFrameReceived", test_CanTp_FirstFrameReceived },
	{ "Test of CanTP_SendFlowControlFrame", test_CanTP_SendFlowControlFrame },
	{ "Test of CanTp_CancelTransmit", test_CanTp_CancelTransmit },
	{ "Test of CanTP_MemSet", test_CanTP_MemSet },
	{ "Test of CanTP_MemCpy", test_CanTP_MemCpy },
	{ "Test of CanTp_ReadParameter", test_CanTp_ReadParameter },
	{ "Test of CanTp_ChangeParameter", test_CanTp_ChangeParameter },
	{ "Test of CanTp_CancelReceive", test_CanTp_CancelReceive },
	{ "Test of CanTp_Transmit", test_CanTp_Transmit },
	{ "Test of CanTp_ConsecutiveFrameReceived", test_CanTp_ConsecutiveFrameReceived },
	// CanTp_Timers tests
    { "Test of CanTp_TStart", test_CanTp_TStart },
	{ "Test of CanTp_Timer_Incr", test_CanTp_Timer_Incr },
	{ "Test of CanTp_Timer_Timeout", test_CanTp_Timer_Timeout },
    { NULL, NULL }
};
