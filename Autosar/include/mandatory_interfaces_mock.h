/*
 * mandatory_interfaces_mock.h
 *
 *  Created on: 7 sty 2023
 *      Author: Paulina
 */

#ifndef INCLUDE_MANDATORY_INTERFACES_MOCK_H_
#define INCLUDE_MANDATORY_INTERFACES_MOCK_H_

#include "CanTP_Types.h"

void PduR_CanTpRxConfirmation(PduIdType  TxPduId, Std_ReturnType result) {};
void PduR_CanTpTxConfirmation(PduIdType  TxPduId, Std_ReturnType result) {};
BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id, PduInfoType* info, RetryInfoType* retry, PduLengthType* availableDataPtr ){};
void PduR_CanTpRxIndication ( PduIdType id, Std_ReturnType result ){};
BufReq_ReturnType PduR_CanTpStartOfReception(id, info, TpSduLength, bufferSizePtr){};

#endif /* INCLUDE_MANDATORY_INTERFACES_MOCK_H_ */
