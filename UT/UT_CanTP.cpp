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


FAKE_VOID_FUNC(PduR_CanTpRxIndication, PduIdType, Std_ReturnType);
FAKE_VALUE_FUNC(BufReq_ReturnType, PduR_CanTpStartOfReception, PduIdType, PduInfoType*, uint32, PduLengthType*);


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

void test_CanTp_RxIndication(void)
{
	PduIdType RxPduId;
	PduInfoType PduInfo;
//	RESET_FAKE(CanTp_GetPCI);
//	RESET_FAKE(CanTp_RxIndicationHandleWaitState);
//	RESET_FAKE(CanTp_RxIndicationHandleProcessingState);
//	RESET_FAKE(CanTp_RxIndicationHandleSuspendedState);
//	CanTp_GetPCI_fake.return_val = E_OK;
//	// CanTP_State OFF
//	CanTP_State.CanTP_State = CANTP_OFF;
//	CanTp_RxIndication(RxPduId, &PduInfo);
//	TEST_CHECK(CanTp_GetPCI_fake.call_count == 0);
//	TEST_CHECK(CanTp_RxIndicationHandleWaitState_fake.call_count == 0);
//	TEST_CHECK(CanTp_RxIndicationHandleProcessingState_fake.call_count == 0);
//	TEST_CHECK(CanTp_RxIndicationHandleSuspendedState_fake.call_count == 0);
//	// CanTP_State ON, RxState WAIT
//	CanTP_State.CanTP_State = CANTP_ON;
//	CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
//	CanTp_RxIndication(RxPduId, &PduInfo);
//	TEST_CHECK(CanTp_GetPCI_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleWaitState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleProcessingState_fake.call_count == 0);
//	TEST_CHECK(CanTp_RxIndicationHandleSuspendedState_fake.call_count == 0);
//	// CanTP_State ON, RxState PROCESSING
//	CanTP_State.CanTP_State = CANTP_ON;
//	CanTP_State.RxState.CanTp_RxState = CANTP_RX_PROCESSING;
//	CanTp_RxIndication(RxPduId, &PduInfo);
//	TEST_CHECK(CanTp_GetPCI_fake.call_count == 2);
//	TEST_CHECK(CanTp_RxIndicationHandleWaitState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleProcessingState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleSuspendedState_fake.call_count == 0);
//	// CanTP_State ON, RxState SUSPENDED
//	CanTP_State.CanTP_State = CANTP_ON;
//	CanTP_State.RxState.CanTp_RxState = CANTP_RX_SUSPENDED;
//	CanTp_RxIndication(RxPduId, &PduInfo);
//	TEST_CHECK(CanTp_GetPCI_fake.call_count == 3);
//	TEST_CHECK(CanTp_RxIndicationHandleWaitState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleProcessingState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleSuspendedState_fake.call_count == 1);
//	// CanTP_State ON, RxState INCORRECT
//	CanTP_State.CanTP_State = CANTP_ON;
//	CanTP_State.RxState.CanTp_RxState = (CanTp_RxState_Type)3;
//	CanTp_RxIndication(RxPduId, &PduInfo);
//	TEST_CHECK(CanTp_GetPCI_fake.call_count == 4);
//	TEST_CHECK(CanTp_RxIndicationHandleWaitState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleProcessingState_fake.call_count == 1);
//	TEST_CHECK(CanTp_RxIndicationHandleSuspendedState_fake.call_count == 1);

}

void test_CanTp_RxIndicationHandleSuspendedState(void)
{
	PduIdType RxPduId = 1;
	PduInfoType PduInfo;
	CanPCI_Type Can_PCI;
	RESET_FAKE(PduR_CanTpRxIndication);

	Can_PCI.FrameType = FirstFrame;
	CanTP_State.Nsdu[0].RxState.CanTp_RxState = CANTP_RX_PROCESSING;//TODO @Paulina
	CanTP_State.Nsdu[0].CanTp_NsduID = 1;
	CanTp_RxIndicationHandleSuspendedState(RxPduId, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(CanTP_State.Nsdu[0].RxState.CanTp_RxState == CANTP_RX_WAIT);//TODO @Paulina

}

void test_CanTp_FirstFrameReceived(void)
{
	PduIdType RxPduId = 2;
	PduInfoType PduInfo;
	CanPCI_Type Can_PCI;
	PduLengthType buffer_size;
	Std_ReturnType ret;
	RESET_FAKE(PduR_CanTpStartOfReception);
	RESET_FAKE(PduR_CanTpRxIndication);
	// incorrect Buf_Status
	Can_PCI.FrameLength = 4095;
	PduInfo.SduLength = 4095;
	buffer_size = 4094;
	PduR_CanTpStartOfReception_fake.return_val = (BufReq_ReturnType)4;
	PduR_CanTpStartOfReception_fake.arg3_val = &buffer_size;
	ret = CanTp_FirstFrameReceived(RxPduId, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpStartOfReception_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 0);
	TEST_CHECK(ret = E_NOT_OK);
	// BUFREQ_OK, buffer_size < PduInfoPtr->SduLength
	Can_PCI.FrameLength = 4095;
	PduInfo.SduLength = 4095;
	buffer_size = 4094;
	PduR_CanTpStartOfReception_fake.return_val = BUFREQ_OK;
	PduR_CanTpStartOfReception_fake.arg3_val = &buffer_size;
	ret = CanTp_FirstFrameReceived(RxPduId, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpStartOfReception_fake.call_count == 2);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(ret = E_NOT_OK);
}

void test_CanTP_SendFlowControlFrame(void)
{
	PduIdType PduID;
	CanPCI_Type CanPCI;
}


#endif /* UT_CANTP_CPP_ */
