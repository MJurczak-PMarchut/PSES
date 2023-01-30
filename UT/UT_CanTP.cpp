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
FAKE_VOID_FUNC(PduR_CanTpRxConfirmation, PduIdType, Std_ReturnType);
FAKE_VOID_FUNC(PduR_CanTpTxConfirmation, PduIdType, Std_ReturnType);
FAKE_VALUE_FUNC(BufReq_ReturnType, PduR_CanTpStartOfReception, PduIdType, PduInfoType*, uint32, PduLengthType*);
FAKE_VALUE_FUNC(Std_ReturnType, CanIf_Transmit, PduIdType, PduInfoType*);
FAKE_VALUE_FUNC(BufReq_ReturnType, PduR_CanTpCopyRxData, PduIdType, PduInfoType*, PduLengthType*);


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

void test_CanTP_GetNsduFromPduID(void)
{
	CanTp_ConfigType CfgPtr = {};
	PduIdType PduID;
	CanTP_NSdu_Type* pNsdu = NULL;
	CanTp_Init(&CfgPtr);

	PduID = 0x34;
	CanTP_State.Nsdu[3].CanTp_NsduID = PduID;
	pNsdu = CanTP_GetNsduFromPduID(PduID);
	TEST_CHECK(pNsdu == &CanTP_State.Nsdu[3]);

	PduID = 0x43;
	pNsdu = CanTP_GetNsduFromPduID(PduID);
	TEST_CHECK(pNsdu == NULL);
}

void test_CanTP_GetFreeNsdu(void)
{
	PduIdType PduID;
	CanTP_NSdu_Type* pNsdu = NULL;
	CanTp_ConfigType CfgPtr = {};

	//all Nsdu in use
	CanTp_Init(&CfgPtr);
	PduID = 0x21;
	for(uint8 nsdu_iter = 0; nsdu_iter <  NO_OF_NSDUS; nsdu_iter++)
	{
		CanTP_State.Nsdu[nsdu_iter].RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	}
	pNsdu = CanTP_GetFreeNsdu(PduID);
	TEST_CHECK(pNsdu == NULL);

	//PduID is already in use
	CanTp_Init(&CfgPtr);
	PduID = 0x21;
	CanTP_State.Nsdu[2].CanTp_NsduID = PduID;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	TEST_CHECK(pNsdu == NULL);

	//free Nsdu available
	CanTp_Init(&CfgPtr);
	PduID = 0x21;
	CanTP_State.Nsdu[0].TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	TEST_CHECK(pNsdu == &CanTP_State.Nsdu[1]);
}

void test_CanTP_MemSet(void)
{
	uint8 test_data[8] = {0, 1, 2, 3 ,4, 5, 6, 7};
	CanTP_MemSet(test_data, 10, sizeof(test_data));
	for(uint8 data_iter = 0; data_iter < sizeof(test_data); data_iter++)
	{
		TEST_CHECK(test_data[data_iter] == 10);
	}
}

void test_CanTP_MemCpy(void)
{
	uint8 test_data_src[8] = {0, 1, 2, 3 ,4, 5, 6, 7};
	uint8 test_data_dst[8] = {0, 0, 0, 0 ,0, 0, 0, 0};
	CanTP_MemCpy(test_data_dst, test_data_src, sizeof(test_data_src));
	for(uint8 data_iter = 0; data_iter < sizeof(test_data_src); data_iter++)
	{
		TEST_CHECK(test_data_dst[data_iter] == test_data_src[data_iter]);
	}

}

void test_CanTP_CopyDefaultNsduConfig(void)
{
	CanTP_NSdu_Type nsdu = {0};
	nsdu.N_Ar.counter = 0x21;
	nsdu.N_Bs.state = TIMER_ACTIVE;
	*(uint32*)(&nsdu.N_Cs.timeout) = 0x37;
	nsdu.RxState.bs = 0x0F;
	nsdu.TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	CanTP_CopyDefaultNsduConfig(&nsdu);
	TEST_CHECK(nsdu.N_Ar.counter == 0);
	TEST_CHECK(nsdu.N_Bs.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(nsdu.N_Cs.timeout == N_CS_TIMEOUT_VAL);
	TEST_CHECK(nsdu.RxState.bs == 0);
	TEST_CHECK(nsdu.TxState.CanTp_TxState == 0);
}

void test_CanTP_NSDuTransmitHandler(void)//ToDo
{
	PduIdType PduID;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTP_NSdu_Type* nsdu = NULL;
	// wrong PduID
	CanTp_Init(&CfgPtr);
	PduID = 0x55;
	ret = CanTP_NSDuTransmitHandler(PduID);
	TEST_CHECK(ret == E_NOT_OK);
	// TxState = CANTP_TX_WAIT
	PduID = 0x55;
	nsdu = CanTP_GetFreeNsdu(PduID);
	nsdu->TxState.CanTp_TxState = CANTP_TX_WAIT;
	nsdu->CanTp_NsduID = PduID;
	ret = CanTP_NSDuTransmitHandler(PduID);
	TEST_CHECK(ret == E_OK);
	// TxState = CANTP_TX_PROCESSING, BlocksToFCFrame = 0
	nsdu->TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	nsdu->TxState.CanTp_BlocksToFCFrame = 0;
	PduID = 0x55;
	ret = CanTP_NSDuTransmitHandler(PduID);
	TEST_CHECK(ret == E_OK);
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

void test_CanTp_RxIndication(void)//ToDo
{
//	PduIdType RxPduId;
//	PduInfoType PduInfo;
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

void test_CanTp_RxIndicationHandleSuspendedState(void)//ToDo
{
//	PduIdType RxPduId = 1;
//	PduInfoType PduInfo;
//	CanPCI_Type Can_PCI;
//	RESET_FAKE(PduR_CanTpRxIndication);
//
//	Can_PCI.FrameType = FirstFrame;
//	CanTP_State.Nsdu[0].RxState.CanTp_RxState = CANTP_RX_PROCESSING;//TODO @Paulina
//	CanTP_State.Nsdu[0].CanTp_NsduID = 1;
//	CanTp_RxIndicationHandleSuspendedState(RxPduId, &PduInfo, &Can_PCI);
//	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
//	TEST_CHECK(CanTP_State.Nsdu[0].RxState.CanTp_RxState == CANTP_RX_WAIT);//TODO @Paulina

}

void test_CanTp_FirstFrameReceived(void)//ToDo
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
//	// BUFREQ_OK, buffer_size < PduInfoPtr->SduLength
//	Can_PCI.FrameLength = 4095;
//	PduInfo.SduLength = 4095;
//	buffer_size = 4094;
//	PduR_CanTpStartOfReception_fake.return_val = BUFREQ_OK;
//	PduR_CanTpStartOfReception_fake.arg3_val = &buffer_size;
//	ret = CanTp_FirstFrameReceived(RxPduId, &PduInfo, &Can_PCI);
//	TEST_CHECK(PduR_CanTpStartOfReception_fake.call_count == 2);
//	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
//	TEST_CHECK(ret = E_NOT_OK);
}

void test_CanTP_SendFlowControlFrame(void)
{
	PduIdType PduID;
	CanPCI_Type CanPCI;
	Std_ReturnType ret = E_NOT_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTP_NSdu_Type *pNsdu = NULL;
	RESET_FAKE(CanIf_Transmit);
	RESET_FAKE(PduR_CanTpRxIndication);
	CanTp_Init(&CfgPtr);
	// PduID does not match any nsdu
	PduID = 0x88;
	ret = CanTP_SendFlowControlFrame(PduID, &CanPCI);
	TEST_CHECK(CanIf_Transmit_fake.call_count == 0);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 0);
	TEST_CHECK(ret = E_NOT_OK);
	// incorrect FS value
	PduID = 0x88;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	CanPCI.FS = (CanTP_FCStatusType)4;
	ret = CanTP_SendFlowControlFrame(PduID, &CanPCI);
	TEST_CHECK(CanIf_Transmit_fake.call_count == 0);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 0);
	TEST_CHECK(ret = E_NOT_OK);
	// FS = FS_OVFLW, CanIf_Transmit returns E_NOT_OK
	PduID = 0x88;
	CanPCI.FS = FS_OVFLW;
	CanIf_Transmit_fake.return_val = E_NOT_OK;
	ret = CanTP_SendFlowControlFrame(PduID, &CanPCI);
	TEST_CHECK(CanIf_Transmit_fake.call_count == 1);
	TEST_CHECK(CanIf_Transmit_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg1_history[0] == E_NOT_OK);
	TEST_CHECK(ret = E_NOT_OK);
	// FS = FS_WAIT, CanIf_Transmit returns E_OK
	PduID = 0x88;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	CanPCI.FS = FS_WAIT;
	CanIf_Transmit_fake.return_val = E_OK;
	ret = CanTP_SendFlowControlFrame(PduID, &CanPCI);
	TEST_CHECK(CanIf_Transmit_fake.call_count == 2);
	TEST_CHECK(CanIf_Transmit_fake.arg0_history[1] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(pNsdu->N_Ar.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_Br.state == TIMER_ACTIVE);
	TEST_CHECK(ret == E_OK);
	// FS = FS_CTS, CanIf_Transmit returns E_OK
	PduID = 0x88;
	CanPCI.FS = FS_CTS;
	CanIf_Transmit_fake.return_val = E_OK;
	ret = CanTP_SendFlowControlFrame(PduID, &CanPCI);
	TEST_CHECK(CanIf_Transmit_fake.call_count == 3);
	TEST_CHECK(CanIf_Transmit_fake.arg0_history[2] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(pNsdu->N_Ar.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_Cr.state == TIMER_ACTIVE);
	TEST_CHECK(ret == E_OK);
}

void test_CanTp_CancelTransmit(void)
{
	PduIdType PduID;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTP_NSdu_Type *pNsdu = NULL;
	RESET_FAKE(PduR_CanTpTxConfirmation);
	CanTp_Init(&CfgPtr);
	// PduID does not match any nsdu
	PduID = 0x12;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_CancelTransmit(PduID);
	TEST_CHECK(PduR_CanTpTxConfirmation_fake.call_count == 0);
	TEST_CHECK(ret = E_NOT_OK);
	// CanTP_State = CANTP_OFF
	PduID = 0x12;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	CanTP_State.CanTP_State = CANTP_OFF;
	ret = CanTp_CancelTransmit(PduID);
	TEST_CHECK(PduR_CanTpTxConfirmation_fake.call_count == 0);
	TEST_CHECK(pNsdu->TxState.CanTp_TxState == CANTP_TX_PROCESSING);
	TEST_CHECK(ret = E_NOT_OK);
	// CanTP_State = CANTP_ON
	pNsdu->TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_CancelTransmit(PduID);
	TEST_CHECK(PduR_CanTpTxConfirmation_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpTxConfirmation_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpTxConfirmation_fake.arg1_history[0] == E_NOT_OK);
	TEST_CHECK(pNsdu->TxState.CanTp_TxState == CANTP_TX_WAIT);
	TEST_CHECK(ret == E_OK);
}

void test_CanTp_Shutdown(void)
{
	CanTP_State.CanTP_State = CANTP_ON;
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		CanTP_State.Nsdu[NsduIter].RxState.CanTp_RxState = CANTP_RX_PROCESSING;
		CanTP_State.Nsdu[NsduIter].TxState.CanTp_TxState = CANTP_TX_PROCESSING;
		CanTP_State.Nsdu[NsduIter].CanTp_NsduID = NsduIter;
		CanTP_State.Nsdu[NsduIter].RxState.bs = NsduIter;
		CanTP_State.Nsdu[NsduIter].TxState.CanTp_SN = NsduIter;
		CanTP_State.Nsdu[NsduIter].N_As.counter = NsduIter;
		CanTP_State.Nsdu[NsduIter].N_Br.state = TIMER_ACTIVE;
	}
	CanTp_Shutdown();
	TEST_CHECK(CanTP_State.CanTP_State == CANTP_OFF);
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].RxState.CanTp_RxState == CANTP_RX_WAIT);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].TxState.CanTp_TxState == CANTP_TX_WAIT);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].CanTp_NsduID == 0);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].RxState.bs == 0);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].TxState.CanTp_SN == 0);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].N_As.counter == 0);
		TEST_CHECK(CanTP_State.Nsdu[NsduIter].N_Br.state == TIMER_NOT_ACTIVE);
	}
}

void test_CanTp_ReadParameter(void)
{
	PduIdType PduID;
	uint16 value = 0;
	CanTP_NSdu_Type *pNsdu = NULL;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTp_Init(&CfgPtr);
	// PduID does not match any nsdu
	PduID = 0x55;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ReadParameter(PduID, TP_STMIN, &value);
	TEST_CHECK(value == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// CanTP_State = CANTP_OFF
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	pNsdu->RxState.sTMin = 0x01;
	pNsdu->RxState.bs = 0x02;
	CanTP_State.CanTP_State = CANTP_OFF;
	ret = CanTp_ReadParameter(PduID, TP_STMIN, &value);
	TEST_CHECK(value == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// RxState = CANTP_RX_PROCESSING
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ReadParameter(PduID, TP_STMIN, &value);
	TEST_CHECK(value == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// incorrect TPParameterType
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ReadParameter(PduID, TP_BC, &value);
	TEST_CHECK(value == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// read sTMin
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ReadParameter(PduID, TP_STMIN, &value);
	TEST_CHECK(value == 0x01);
	TEST_CHECK(ret == E_OK);
	// read sTMin
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ReadParameter(PduID, TP_BS, &value);
	TEST_CHECK(value == 0x02);
	TEST_CHECK(ret == E_OK);
}

void test_CanTp_CancelReceive(void)
{
	PduIdType PduID;
	CanTP_NSdu_Type *pNsdu = NULL;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	RESET_FAKE(PduR_CanTpRxConfirmation);
	CanTp_Init(&CfgPtr);
	// PduID does not match any nsdu
	PduID = 0x11;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_CancelReceive(PduID);
	TEST_CHECK(PduR_CanTpRxConfirmation_fake.call_count == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// CanTP_State = CANTP_OFF
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	CanTP_State.CanTP_State = CANTP_OFF;
	ret = CanTp_CancelReceive(PduID);
	TEST_CHECK(PduR_CanTpRxConfirmation_fake.call_count == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_PROCESSING);
	TEST_CHECK(ret == E_NOT_OK);
	// cancel receive
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_CancelReceive(PduID);
	TEST_CHECK(PduR_CanTpRxConfirmation_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpRxConfirmation_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpRxConfirmation_fake.arg1_history[0] == E_NOT_OK);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT);
	TEST_CHECK(ret == E_OK);
}

void test_CanTp_ChangeParameter(void)
{
	PduIdType PduID;
	uint16 value = 0;
	CanTP_NSdu_Type *pNsdu = NULL;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTp_Init(&CfgPtr);
	// PduID does not match any nsdu
	value = 0x01;
	PduID = 0x78;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ChangeParameter(PduID, TP_STMIN, value);
	TEST_CHECK(ret == E_NOT_OK);
	// CanTP_State = CANTP_OFF
	value = 0x01;
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	pNsdu->RxState.sTMin = 0x00;
	pNsdu->RxState.bs = 0x00;
	CanTP_State.CanTP_State = CANTP_OFF;
	ret = CanTp_ChangeParameter(PduID, TP_STMIN, value);
	TEST_CHECK(pNsdu->RxState.sTMin == 0);
	TEST_CHECK(pNsdu->RxState.bs == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// RxState = CANTP_RX_PROCESSING
	value = 0x01;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ChangeParameter(PduID, TP_STMIN, value);
	TEST_CHECK(pNsdu->RxState.sTMin == 0);
	TEST_CHECK(pNsdu->RxState.bs == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// incorrect TPParameterType
	value = 0x01;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ChangeParameter(PduID, TP_BC, value);
	TEST_CHECK(pNsdu->RxState.sTMin == 0);
	TEST_CHECK(pNsdu->RxState.bs == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// set sTMin
	value = 0xFE;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ChangeParameter(PduID, TP_STMIN, value);
	TEST_CHECK(pNsdu->RxState.sTMin == 0xFE);
	TEST_CHECK(pNsdu->RxState.bs == 0);
	TEST_CHECK(ret == E_OK);
	// set sTMin
	value = 0xFF;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_ChangeParameter(PduID, TP_BS, value);
	TEST_CHECK(pNsdu->RxState.sTMin == 0xFE);
	TEST_CHECK(pNsdu->RxState.bs == 0xFF);
	TEST_CHECK(ret == E_OK);
}

void test_CanTp_Transmit(void)
{
	PduIdType PduID;
	PduInfoType PduInfo;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTP_NSdu_Type *pNsdu = NULL;
	uint8 data[] = {0, 1, 2, 3};
	PduInfo.SduLength = sizeof(data);
	PduInfo.SduDataPtr = data;
	PduInfo.MetaDataPtr = NULL;
	PduID = 0x77;
	// Nsdu with this PduID already taken
	CanTp_Init(&CfgPtr);
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->CanTp_NsduID = PduID;
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_Transmit(PduID, &PduInfo);
	TEST_CHECK(ret == E_NOT_OK);
	// CanTP_State = CANTP_OFF
	CanTp_Init(&CfgPtr);
	CanTP_State.CanTP_State = CANTP_OFF;
	ret = CanTp_Transmit(PduID, &PduInfo);
	TEST_CHECK(ret == E_NOT_OK);
	// PduInfoType* = NULL
	CanTp_Init(&CfgPtr);
	CanTP_State.CanTP_State = CANTP_ON;
	ret = CanTp_Transmit(PduID, NULL);
	TEST_CHECK(ret == E_NOT_OK);
	// no data to send
	CanTp_Init(&CfgPtr);
	CanTP_State.CanTP_State = CANTP_ON;
	PduInfo.SduLength = 0;
	ret = CanTp_Transmit(PduID, &PduInfo);
	TEST_CHECK(ret == E_NOT_OK);
	// MetaData is present
	CanTp_Init(&CfgPtr);
	PduInfo.SduLength = sizeof(data);
	CanTP_State.CanTP_State = CANTP_ON;
	PduInfo.MetaDataPtr = data;
	ret = CanTp_Transmit(PduID, &PduInfo);
	pNsdu = CanTP_GetNsduFromPduID(PduID);
	TEST_CHECK(pNsdu->TxState.CanTp_MsgLegth == sizeof(data));
	TEST_CHECK(pNsdu->TxState.CanTp_TxState == CANTP_TX_PROCESSING);
	TEST_CHECK(pNsdu->N_As.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_As.counter == 0);
	TEST_CHECK(pNsdu->N_Bs.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_Bs.counter == 0);
	TEST_CHECK(pNsdu->N_Cs.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cs.counter == 0);
	TEST_CHECK(ret == E_OK);
	// no MetaData
	CanTp_Init(&CfgPtr);
	CanTP_State.CanTP_State = CANTP_ON;
	PduInfo.MetaDataPtr = NULL;
	pNsdu->TxState.CanTp_TxState = CANTP_TX_WAIT;
	ret = CanTp_Transmit(PduID, &PduInfo);
	TEST_CHECK(pNsdu->TxState.CanTp_MsgLegth == sizeof(data));
	TEST_CHECK(pNsdu->TxState.CanTp_TxState == CANTP_TX_PROCESSING);
	TEST_CHECK(pNsdu->N_As.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_As.counter == 0);
	TEST_CHECK(pNsdu->N_Bs.state == TIMER_ACTIVE);
	TEST_CHECK(pNsdu->N_Bs.counter == 0);
	TEST_CHECK(pNsdu->N_Cs.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cs.counter == 0);
	TEST_CHECK(ret == E_OK);
}

BufReq_ReturnType custom_mockPdur_cpyRx(PduIdType id, PduInfoType* info, PduLengthType* availableDataPtr )
{
	static uint8 call_count = 0;
	BufReq_ReturnType retval[2] = {BUFREQ_OK, BUFREQ_E_NOT_OK};
	PduLengthType len[2] = {7, 0};
	*availableDataPtr = len[call_count];
	call_count++;
	call_count = call_count % 2;
	return retval[call_count - 1];
}

void test_CanTp_ConsecutiveFrameReceived(void)
{
	PduIdType PduID;
	PduInfoType PduInfo;
	CanPCI_Type Can_PCI;
	PduLengthType buffer_size;
	Std_ReturnType ret = E_OK;
	CanTp_ConfigType CfgPtr = {};
	CanTP_NSdu_Type *pNsdu = NULL;

	RESET_FAKE(PduR_CanTpCopyRxData);
	RESET_FAKE(PduR_CanTpRxIndication);
	CanTp_Init(&CfgPtr);
	PduID = 0xAB;
	// PduID does not match any nsdu
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 0);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 0);
	TEST_CHECK(ret == E_NOT_OK);
	// pNsdu->RxState.CanTp_ExpectedCFNo != Can_PCI.SN
	pNsdu = CanTP_GetFreeNsdu(PduID);
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 1;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	Can_PCI.SN = 2;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 0);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg1_history[0] == E_NOT_OK);
	TEST_CHECK(pNsdu->N_Cr.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cr.counter == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT);
	TEST_CHECK(ret == E_NOT_OK);
	// buf_Status = BUFREQ_E_NOT_OK
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 3;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	Can_PCI.SN = 3;
	PduR_CanTpCopyRxData_fake.return_val = BUFREQ_E_NOT_OK;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 1);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.arg0_history[0] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 2);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg0_history[1] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg1_history[1] == E_NOT_OK);
	TEST_CHECK(pNsdu->N_Cr.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cr.counter == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT);
	TEST_CHECK(ret == E_NOT_OK);
	// pNsdu->RxState.CanTp_MessageLength = pNsdu->RxState.CanTp_ReceivedBytes
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 3;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	pNsdu->RxState.CanTp_ReceivedBytes = 2;
	pNsdu->RxState.CanTp_NoOfBlocksTillCTS = 7;
	pNsdu->RxState.CanTp_MessageLength = 5;
	PduInfo.SduLength = 3;
	Can_PCI.SN = 3;
	PduR_CanTpCopyRxData_fake.custom_fake = custom_mockPdur_cpyRx;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 2);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.arg0_history[1] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 3);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg0_history[2] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.arg1_history[2] == E_OK);
	TEST_CHECK(pNsdu->N_Cr.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cr.counter == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT);
	TEST_CHECK(ret == E_OK);
	// pNsdu->RxState.CanTp_MessageLength != pNsdu->RxState.CanTp_ReceivedBytes
	// CanTp_NoOfBlocksTillCTS != 0
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 3;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	pNsdu->RxState.CanTp_ReceivedBytes = 2;
	pNsdu->RxState.CanTp_NoOfBlocksTillCTS = 7;
	pNsdu->RxState.CanTp_MessageLength = 6;
	PduInfo.SduLength = 3;
	Can_PCI.SN = 3;
	PduR_CanTpCopyRxData_fake.custom_fake = custom_mockPdur_cpyRx;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 3);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.arg0_history[2] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 3);
	TEST_CHECK(pNsdu->N_Cr.state == TIMER_NOT_ACTIVE);
	TEST_CHECK(pNsdu->N_Cr.counter == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_PROCESSING);
	TEST_CHECK(pNsdu->RxState.CanTp_NoOfBlocksTillCTS == 6);
	TEST_CHECK(pNsdu->RxState.CanTp_ExpectedCFNo == 4);
	TEST_CHECK(ret == E_OK);
	// pNsdu->RxState.CanTp_MessageLength != pNsdu->RxState.CanTp_ReceivedBytes
	// CanTp_NoOfBlocksTillCTS = 1
	// required_blocks > 0
	buffer_size = 4094;
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 3;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_SUSPENDED;
	pNsdu->RxState.CanTp_ReceivedBytes = 2;
	pNsdu->RxState.CanTp_NoOfBlocksTillCTS = 1;
	pNsdu->RxState.CanTp_MessageLength = 10;
	PduInfo.SduLength = 3;
	Can_PCI.SN = 3;
	PduR_CanTpCopyRxData_fake.custom_fake = custom_mockPdur_cpyRx;
	PduR_CanTpCopyRxData_fake.arg2_val = &buffer_size;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 4);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.arg0_history[3] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 3);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_PROCESSING);
	TEST_CHECK(pNsdu->RxState.CanTp_NoOfBlocksTillCTS == 1);
	TEST_CHECK(pNsdu->RxState.CanTp_ExpectedCFNo == 4);
	TEST_CHECK(ret == E_OK);
	// pNsdu->RxState.CanTp_MessageLength != pNsdu->RxState.CanTp_ReceivedBytes
	// CanTp_NoOfBlocksTillCTS = 1
	// required_blocks = 0
	buffer_size = 0;
	pNsdu->N_Cr.state = TIMER_ACTIVE;
	pNsdu->N_Cr.counter = 0x09;
	pNsdu->CanTp_NsduID = PduID;
	pNsdu->RxState.CanTp_ExpectedCFNo = 3;
	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
	pNsdu->RxState.CanTp_ReceivedBytes = 2;
	pNsdu->RxState.CanTp_NoOfBlocksTillCTS = 1;
	pNsdu->RxState.CanTp_MessageLength = 6;
	PduInfo.SduLength = 3;
	Can_PCI.SN = 3;
	PduR_CanTpCopyRxData_fake.custom_fake = custom_mockPdur_cpyRx;
	PduR_CanTpCopyRxData_fake.arg2_val = &buffer_size;
	ret = CanTp_ConsecutiveFrameReceived(PduID, &PduInfo, &Can_PCI);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.call_count == 5);
	TEST_CHECK(PduR_CanTpCopyRxData_fake.arg0_history[4] == PduID);
	TEST_CHECK(PduR_CanTpRxIndication_fake.call_count == 3);
	TEST_CHECK(pNsdu->RxState.CanTp_RxState == CANTP_RX_SUSPENDED);
	TEST_CHECK(pNsdu->RxState.CanTp_NoOfBlocksTillCTS == 0);
	TEST_CHECK(pNsdu->RxState.CanTp_ExpectedCFNo == 4);
	TEST_CHECK(ret == E_OK);
}

#endif /* UT_CANTP_CPP_ */
