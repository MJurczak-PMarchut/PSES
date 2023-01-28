/*
 * mandatory_interfaces_mock.c
 *
 *  Created on: 28 sty 2023
 *      Author: Mateusz
 */
#include "mandatory_interfaces_mock.h"



__attribute__((weak)) void PduR_CanTpRxIndication ( PduIdType id, Std_ReturnType result )
{

}

__attribute__((weak)) BufReq_ReturnType PduR_CanTpCopyRxData(PduIdType id, PduInfoType* info, PduLengthType* availableDataPtr )
{
	return BUFREQ_E_NOT_OK;
}

__attribute__((weak)) BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id, PduInfoType* info, RetryInfoType* retry, PduLengthType* availableDataPtr )
{
	return BUFREQ_E_NOT_OK;
}

__attribute__((weak)) void PduR_CanTpTxConfirmation(PduIdType  TxPduId, Std_ReturnType result)
{

}

__attribute__((weak)) void PduR_CanTpRxConfirmation(PduIdType  TxPduId, Std_ReturnType result)
{

}

__attribute__((weak)) BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType id, PduInfoType* info, uint32 TpSduLength, PduLengthType* bufferSizePtr)
{
	return BUFREQ_E_NOT_OK;
}

__attribute__((weak)) Std_ReturnType CanIf_Transmit(PduIdType TxPduId, PduInfoType *PduInfoPtr0)
{
	return E_NOT_OK;
}
