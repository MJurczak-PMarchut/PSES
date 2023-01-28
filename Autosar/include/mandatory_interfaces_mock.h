/*
 * mandatory_interfaces_mock.h
 *
 *  Created on: 7 sty 2023
 *      Author: Paulina
 */

#ifndef INCLUDE_MANDATORY_INTERFACES_MOCK_H_
#define INCLUDE_MANDATORY_INTERFACES_MOCK_H_

#include "CanTP_Types.h"

__attribute__((weak)) void PduR_CanTpRxConfirmation(PduIdType  TxPduId, Std_ReturnType result);
__attribute__((weak)) void PduR_CanTpTxConfirmation(PduIdType  TxPduId, Std_ReturnType result);
__attribute__((weak)) BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id, PduInfoType* info, RetryInfoType* retry, PduLengthType* availableDataPtr );
__attribute__((weak)) BufReq_ReturnType PduR_CanTpCopyRxData(PduIdType id, PduInfoType* info, PduLengthType* availableDataPtr );
__attribute__((weak)) void PduR_CanTpRxIndication ( PduIdType id, Std_ReturnType result );
__attribute__((weak)) BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType id, PduInfoType* info, uint32 TpSduLength, PduLengthType* bufferSizePtr);

#endif /* INCLUDE_MANDATORY_INTERFACES_MOCK_H_ */
