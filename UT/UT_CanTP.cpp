/*
 * test1.hpp
 *
 *  Created on: 22 paź 2022
 *      Author: Mateusz
 */

#ifndef UT_CANTP_CPP_
#define UT_CANTP_CPP_
#include <stdlib.h>

#define TEST_NO_MAIN

#include "acutest.h"
#include "fff.h"
#include "UT_CanTP.hpp"
#include "CanTP.c"

DEFINE_FFF_GLOBALS;

void test_CanTp_Init(void)
{
	CanTp_ConfigType CfgPtr = {};
	CanTP_State.CanTP_State = CANTP_OFF;
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		CanTP_State.Nsdu[NsduIter].RxState.CanTp_RxState = CANTP_RX_PROCESSING;
		CanTP_State.Nsdu[NsduIter].TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	}

	CanTp_Init(&CfgPtr);
	TEST_CHECK(CanTP_State.CanTP_State == CANTP_ON);
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].RxState.CanTp_RxState == CANTP_RX_WAIT);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].TxState.CanTp_TxState == CANTP_TX_WAIT);
	}
}

void test_CanTp_GetVersionInfo(void)
{
	Std_VersionInfoType ver = {0,0,0,0,0,0,4,4,0};
	Std_VersionInfoType pver;
	CanTp_GetVersionInfo(&pver);
	TEST_CHECK(pver.ar_major_version == ver.ar_major_version);
	TEST_CHECK(pver.ar_minor_version == ver.ar_minor_version);
	TEST_CHECK(pver.ar_patch_version == ver.ar_patch_version);
	TEST_CHECK(pver.instanceID == ver.instanceID);
	TEST_CHECK(pver.moduleID == ver.moduleID);
	TEST_CHECK(pver.sw_major_version == ver.sw_major_version);
	TEST_CHECK(pver.sw_minor_version == ver.sw_minor_version);
	TEST_CHECK(pver.sw_patch_version == ver.sw_patch_version);
	TEST_CHECK(pver.vendorID == ver.vendorID);
}

#endif /* UT_CANTP_CPP_ */
