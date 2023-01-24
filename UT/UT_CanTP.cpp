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
	CanTP_State.RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	CanTP_State.TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	CanTp_Init(&CfgPtr);
	TEST_CHECK(CanTP_State.CanTP_State == CANTP_ON);
	TEST_CHECK(CanTP_State.RxState.CanTp_RxState == CANTP_RX_WAIT);
	TEST_CHECK(CanTP_State.TxState.CanTp_TxState == CANTP_TX_WAIT);
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

void test_CanTp_GetPCI(void)
{
	PduInfoType PduInfoPtr;
	uint8_t SduData[6] = {0};
	CanPCI_Type CanPCI = {0};
	Std_ReturnType ret = E_NOT_OK;
	PduInfoPtr.SduDataPtr = SduData;
	// FirstFrame, FF_DL > 4095
	SduData[0] = (FirstFrame << 4) + 0x0;
	SduData[1] = 0x00;
	SduData[2] = 0xF0;
	SduData[3] = 0xF0;
	SduData[4] = 0xF0;
	SduData[5] = 0xF0;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == FirstFrame);
	TEST_CHECK(CanPCI.FrameLength == 0xF0F0F0F0);
	// FirstFrame, FF_DL = 4095
	SduData[0] = (FirstFrame << 4) + 0xF;
	SduData[1] = 0xFF;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == FirstFrame);
	TEST_CHECK(CanPCI.FrameLength == 0xFFF);
	// FirstFrame, FF_DL < 4095
	SduData[0] = (FirstFrame << 4) + 0xF;
	SduData[1] = 0xFE;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == FirstFrame);
	TEST_CHECK(CanPCI.FrameLength == 0xFFE);
	// SingleFrame, CAN_DL > 8
	SduData[0] = (SingleFrame << 4) + 0x0;
	SduData[1] = 0xFE;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == SingleFrame);
	TEST_CHECK(CanPCI.FrameLength == 0xFE);
	// SingleFrame, CAN_DL = 8
	SduData[0] = (SingleFrame << 4) + 0x8;
	SduData[1] = 0xFE;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == SingleFrame);
	TEST_CHECK(CanPCI.FrameLength == 0x8);
	// SingleFrame, CAN_DL < 8
	SduData[0] = (SingleFrame << 4) + 0x2;
	SduData[1] = 0xFE;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == SingleFrame);
	TEST_CHECK(CanPCI.FrameLength == 0x2);
	// FlowControlFrame
	SduData[0] = (FlowControlFrame << 4) + 0x1;
	SduData[1] = 0x02;
	SduData[2] = 0x03;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == FlowControlFrame);
	TEST_CHECK(CanPCI.FS == 0x01);
	TEST_CHECK(CanPCI.BS == 0x02);
	TEST_CHECK(CanPCI.ST == 0x03);
	// ConsecutiveFrame
	SduData[0] = (ConsecutiveFrame << 4) + 0x1;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(CanPCI.FrameType == ConsecutiveFrame);
	TEST_CHECK(CanPCI.SN == 0x01);
	// IncorrectFrame
	SduData[0] = (0xF << 4) + 0x1;
	ret = CanTp_GetPCI(&PduInfoPtr, &CanPCI);
	TEST_CHECK(ret == E_NOT_OK);
	TEST_CHECK(CanPCI.FrameType == (CanTPFrameType)-1);
}

#endif /* UT_CANTP_CPP_ */
