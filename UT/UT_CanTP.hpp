/*
 * test1.hpp
 *
 *  Created on: 22 paź 2022
 *      Author: Mateusz
 */

#ifndef UT_CANTP_HPP_
#define UT_CANTP_HPP_
#include <stdlib.h>

void test_CanTp_Init(void);
void test_CanTP_GetNsduFromPduID(void);
void test_CanTP_GetFreeNsdu(void);
void test_CanTP_CopyDefaultNsduConfig(void);
void test_CanTp_GetVersionInfo(void);
void test_CanTp_GetPCI(void);
void test_CanTp_RxIndication(void);
void test_CanTp_RxIndicationHandleSuspendedState(void);
void test_CanTp_FirstFrameReceived(void);
void test_CanTP_SendFlowControlFrame(void);
void test_CanTP_MemSet(void);
void test_CanTP_MemCpy(void);

#endif /* UT_CANTP_HPP_ */
