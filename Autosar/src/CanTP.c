/*
 * CanTP.c
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */


#include "CanTP.h"

#include "CanTp_Timers.h"
#define CANTP_CAN_FRAME_SIZE (0x08u)

#ifndef CANTP_MAX_NUM_OF_N_SDU
#define CANTP_MAX_NUM_OF_N_SDU (0x10u)
#endif

#define NULL_PTR NULL
#define FC_WAIT_FRAME_CTR_MAX 10

#define NO_OF_NSDUS 5

typedef uint8 CanTp_NPciType;

typedef uint8 CanTp_FlowStatusType;

typedef enum
{
    CANTP_FRAME_STATE_INVALID = 0x00u,
    CANTP_RX_FRAME_STATE_FC_TX_REQUEST,
    CANTP_RX_FRAME_STATE_FC_TX_CONFIRMATION,
    CANTP_RX_FRAME_STATE_FC_OVFLW_TX_CONFIRMATION,
    CANTP_RX_FRAME_STATE_CF_RX_INDICATION,
    CANTP_TX_FRAME_STATE_SF_TX_REQUEST,
    CANTP_TX_FRAME_STATE_SF_TX_CONFIRMATION,
    CANTP_TX_FRAME_STATE_FF_TX_REQUEST,
    CANTP_TX_FRAME_STATE_FF_TX_CONFIRMATION,
    CANTP_TX_FRAME_STATE_CF_TX_REQUEST,
    CANTP_TX_FRAME_STATE_CF_TX_CONFIRMATION,
    CANTP_TX_FRAME_STATE_FC_RX_INDICATION,
    CANTP_FRAME_STATE_OK,
    CANTP_FRAME_STATE_ABORT
} CanTp_FrameStateType;

typedef enum {
    CANTP_RX_WAIT = 0x00u,
    CANTP_RX_PROCESSING,
	CANTP_RX_SUSPENDED //When waiting for buffer or TxConfirmation on FC
} CanTp_RxState_Type;


typedef enum {
    CANTP_TX_WAIT = 0x00u,
    CANTP_TX_PROCESSING,
	CANTP_TX_SUSPENDED //When waiting for confirmation or FC
} CanTp_TxState_Type;

typedef struct
{
    uint8 can[CANTP_CAN_FRAME_SIZE];
    PduLengthType size;
    PduLengthType rmng;
} CanTp_NSduBufferType;

// struct of rx state variables
typedef struct{
    uint16 CanTp_MessageLength;
    uint16 CanTp_ExpectedCFNo;
    uint16 CanTp_ReceivedBytes;
    CanTp_RxState_Type CanTp_RxState;
    uint8 CanTp_NoOfBlocksTillCTS;
	uint16 CanTpRxWftMax;
	uint32 sTMin;
	uint8 bs;
} CanTp_RxStateVariablesType;

typedef struct{
    CanTp_TxState_Type CanTp_TxState;

    uint16 CanTp_BytesSent;
    uint8_t CanTp_SN;
    uint16 CanTp_BlocksToFCFrame;
    uint16 CanTp_MsgLegth;
} CanTp_TxStateVariablesType;

typedef struct{
	PduIdType CanTp_NsduID;
	CanTp_RxStateVariablesType 	RxState;
	CanTp_TxStateVariablesType	TxState;
	CanTp_Timer_type N_Ar;
	CanTp_Timer_type N_Br;
	CanTp_Timer_type N_Cr;
	CanTp_Timer_type N_As;
	CanTp_Timer_type N_Bs;
	CanTp_Timer_type N_Cs;
	//ECUC_CanTp_00251
} CanTP_NSdu_Type;

typedef struct{
	CanTP_NSdu_Type				Nsdu[NO_OF_NSDUS];
	CanTp_StateType				CanTP_State;
} CanTP_InternalStateType;




typedef enum{
	FS_OVFLW = 0,
	FS_WAIT,
	FS_CTS
}CanTP_FCStatusType;

/*
 * Recv from LL
 */

typedef enum {
    SingleFrame = 0,
    FirstFrame,
    ConsecutiveFrame,
    FlowControlFrame,
    } CanTPFrameType;

typedef struct{
    uint8 SN; // Sequence Number of consecutive frame
    uint8 BS; // Block Size FC frame
    CanTP_FCStatusType FS; // Status from flow control
    uint8 ST; // Separation Time FF
    CanTPFrameType FrameType;
    uint32 FrameLength;
} CanPCI_Type;

//Store information about CanTp and nsdu state
static CanTP_InternalStateType CanTP_State = {0};

static CanTP_NSdu_Type* CanTP_GetNsduFromPduID(PduIdType PduID)
{
	CanTP_NSdu_Type *pNsdu = NULL;
	for(uint8 nsdu_iter = 0; nsdu_iter < NO_OF_NSDUS; nsdu_iter++)
	{
		if(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID == PduID)
		{
			pNsdu = &CanTP_State.Nsdu[nsdu_iter];
			break;
		}
	}
	return pNsdu;
}

/*
 * Returns a pointer to Nsdu that is free and can be ussed
 */
static CanTP_NSdu_Type* CanTP_GetFreeNsdu(PduIdType PduID)
{
	CanTP_NSdu_Type *pNsdu;
	for(uint8 nsdu_iter = 0; nsdu_iter <  NO_OF_NSDUS; nsdu_iter++)
	{
		//look for problems
		pNsdu = &CanTP_State.Nsdu[nsdu_iter];
		if(pNsdu->CanTp_NsduID == PduID){
			//If PduID is already in use, gnore it
			return NULL;
		}
	}
	for(uint8 nsdu_iter = 0; nsdu_iter <  NO_OF_NSDUS; nsdu_iter++)
	{
		//look for problems
		pNsdu = &CanTP_State.Nsdu[nsdu_iter];
		if((pNsdu->CanTp_NsduID == 0) && (pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT) && (pNsdu->TxState.CanTp_TxState == CANTP_TX_WAIT)){
			return pNsdu;
		}
	}
	return NULL;
}


/*
 * Sets a region of memory to a value
 * (Sets each byte)
 */
static void* CanTP_MemSet(void* destination, int value, uint64 num)
{
	uint8* dest = (uint8*)destination;
	for(uint64 u64Iter = 0; u64Iter < num; u64Iter++)
	{
		dest[u64Iter] = (uint8)value;
	}
	return destination;
}

/*
 * Copies regions of memory from source to destination
 */
static void* CanTP_MemCpy(void* destination, void* source, uint64 num)
{
	uint8* dest = (uint8*)destination;
	uint8* src =  (uint8*)source;
	for(uint64 u64Iter = 0; u64Iter < num; u64Iter++)
	{
		dest[u64Iter] = src[u64Iter];
	}
	return destination;
}

/*
 * Restores default values to Nsdu
 */
static void* CanTP_CopyDefaultNsduConfig(CanTP_NSdu_Type *nsdu)
{
	CanTp_Timer_type N_ArTimerDefault = {TIMER_NOT_ACTIVE, 0, N_AR_TIMEOUT_VAL};
	CanTp_Timer_type N_BrTimerDefault = {TIMER_NOT_ACTIVE, 0, N_BR_TIMEOUT_VAL};
	CanTp_Timer_type N_CrTimerDefault = {TIMER_NOT_ACTIVE, 0, N_CR_TIMEOUT_VAL};

	CanTp_Timer_type N_AsTimerDefault = {TIMER_NOT_ACTIVE, 0, N_AS_TIMEOUT_VAL};
	CanTp_Timer_type N_BsTimerDefault = {TIMER_NOT_ACTIVE, 0, N_BS_TIMEOUT_VAL};
	CanTp_Timer_type N_CsTimerDefault = {TIMER_NOT_ACTIVE, 0, N_CS_TIMEOUT_VAL};
	//zero the data inside nsdu
	CanTP_MemSet(&nsdu->RxState, 0, sizeof(CanTp_RxStateVariablesType));
	CanTP_MemSet(&nsdu->TxState, 0, sizeof(CanTp_TxStateVariablesType));
	nsdu->CanTp_NsduID = 0;

	//reset all timers
	//TODO @Justyna please check if we need to do anything else

	CanTP_MemCpy(&nsdu->N_Ar, &N_ArTimerDefault, sizeof(N_ArTimerDefault));
	CanTP_MemCpy(&nsdu->N_Br, &N_BrTimerDefault, sizeof(N_BrTimerDefault));
	CanTP_MemCpy(&nsdu->N_Cr, &N_CrTimerDefault, sizeof(N_CrTimerDefault));

	CanTP_MemCpy(&nsdu->N_As, &N_AsTimerDefault, sizeof(N_AsTimerDefault));
	CanTP_MemCpy(&nsdu->N_Bs, &N_BsTimerDefault, sizeof(N_BsTimerDefault));
	CanTP_MemCpy(&nsdu->N_Cs, &N_CsTimerDefault, sizeof(N_CsTimerDefault));

	return nsdu;
}


/*
 * Handler that is called to check if we can send something (CF)
 */
static Std_ReturnType CanTP_NSDuTransmitHandler(PduIdType PduID){
	Std_ReturnType retval = E_NOT_OK;
	//Check if there is anything to send
	CanTP_NSdu_Type *pNsdu = NULL;

	pNsdu = CanTP_GetNsduFromPduID(PduID);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if((pNsdu->TxState.CanTp_TxState == CANTP_TX_WAIT) || (pNsdu->TxState.CanTp_TxState == CANTP_TX_SUSPENDED)){
		return E_OK;
	}
	if(pNsdu->TxState.CanTp_BlocksToFCFrame == 0){
		//we are waiting for CF
		return E_OK;
	}

	if(pNsdu->TxState.CanTp_BytesSent == 0){
		//First farme, we should probably send it here
		PduInfoType Tmp_Pdu;
		BufReq_ReturnType BufReq_State;
		PduLengthType Len_Pdu;
		CanTP_MemSet(&Tmp_Pdu, 0, sizeof(Tmp_Pdu));
		if(pNsdu->TxState.CanTp_MsgLegth < 8 ){

			Tmp_Pdu.SduLength = pNsdu->TxState.CanTp_MsgLegth;
			//call PduR
			BufReq_State = PduR_CanTpCopyTxData(PduID, &Tmp_Pdu, NULL, &Len_Pdu);
			if(BufReq_State == BUFREQ_OK){

				//Start sending single frame: SF
				PduInfoType PDU_To_send;
				CanTP_MemSet(&PDU_To_send, 0, sizeof(PDU_To_send));
				PDU_To_send.SduDataPtr[0] = SingleFrame << 4;
				PDU_To_send.SduDataPtr[0] = 0x0F & pNsdu->TxState.CanTp_MsgLegth;
				for (uint8 data_iter = 0; data_iter < pNsdu->TxState.CanTp_MsgLegth; data_iter++)
				{
					PDU_To_send.SduDataPtr[data_iter + 1] = Tmp_Pdu.SduDataPtr[data_iter];
				}
				if(CanIf_Transmit(PduID, &PDU_To_send) == E_OK ){
					CanTp_TStart(&pNsdu->N_As);
					CanTp_TStart(&pNsdu->N_Bs);
					pNsdu->TxState.CanTp_BytesSent = pNsdu->TxState.CanTp_MsgLegth;
					pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;
					retval = E_OK;
				}
				else{
				 /*
					 [SWS_CanTp_00343]  CanTp shall terminate the current  transmission connection
					  when CanIf_Transmit() returns E_NOT_OK when transmitting an SF, FF, of CF ()
				 */
				 PduR_CanTpTxConfirmation(PduID, E_NOT_OK);
				 retval = E_NOT_OK;
				}
			}
			else if (BufReq_State == BUFREQ_E_BUSY) {
				//And now we wait for the buffer to become ready
				//Start timers
				CanTp_TStart(&pNsdu->N_Cs);
				//JUSTYNA na pewno tu ten timer startujemy??
				pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;

			}
			else
			{
				CanTP_CopyDefaultNsduConfig(pNsdu);
				retval = E_NOT_OK;
			}
		}
		else if(pNsdu->TxState.CanTp_MsgLegth < 4096){
			// FF format 1
			PduInfoType PDU_To_send;
			CanTP_MemSet(&PDU_To_send, 0, sizeof(PDU_To_send));
			//We want 6 bytes
			Tmp_Pdu.SduLength = 6;
			BufReq_State = PduR_CanTpCopyTxData(PduID, &Tmp_Pdu, NULL, &Len_Pdu);
			PDU_To_send.SduDataPtr[0] = FirstFrame << 4;
			PDU_To_send.SduDataPtr[0] |= 0x0F & (pNsdu->TxState.CanTp_MsgLegth >> 8);
			PDU_To_send.SduDataPtr[1] = 0xFF & pNsdu->TxState.CanTp_MsgLegth;
			for (uint8 data_iter = 0; data_iter < 6; data_iter++)
			{
				PDU_To_send.SduDataPtr[data_iter + 2] = Tmp_Pdu.SduDataPtr[data_iter];
			}
			if(CanIf_Transmit(PduID, &PDU_To_send) == E_OK ){
				CanTp_TStart(&pNsdu->N_As);
				CanTp_TStart(&pNsdu->N_Bs);
				pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;
				pNsdu->TxState.CanTp_BytesSent = 6;
				retval = E_OK;
			}
			else{
			 /*
				 [SWS_CanTp_00343]  CanTp shall terminate the current  transmission connection
				  when CanIf_Transmit() returns E_NOT_OK when transmitting an SF, FF, of CF ()
			 */
			 PduR_CanTpTxConfirmation(PduID, E_NOT_OK);
			 retval = E_NOT_OK;
			}
		}
		else if(pNsdu->TxState.CanTp_MsgLegth >= 4096){
			// FF format 2
			PduInfoType PDU_To_send;
			CanTP_MemSet(&PDU_To_send, 0, sizeof(PDU_To_send));
			BufReq_State = PduR_CanTpCopyTxData(PduID, &Tmp_Pdu, NULL, &Len_Pdu);
			//only 2 bytes here
			Tmp_Pdu.SduLength = 2;
			PDU_To_send.SduDataPtr[0] = FirstFrame << 4;
			PDU_To_send.SduDataPtr[1] = 0;

			PDU_To_send.SduDataPtr[2] = (Tmp_Pdu.SduLength >> 24) & 0xFF;
			PDU_To_send.SduDataPtr[3] = (Tmp_Pdu.SduLength >> 16) & 0xFF;
			PDU_To_send.SduDataPtr[4] = (Tmp_Pdu.SduLength >> 8) & 0xFF;
			PDU_To_send.SduDataPtr[5] = Tmp_Pdu.SduLength & 0xFF;
			//Data at the end of the world
			PDU_To_send.SduDataPtr[6] = Tmp_Pdu.SduDataPtr[0];
			PDU_To_send.SduDataPtr[7] = Tmp_Pdu.SduDataPtr[1];

			if(CanIf_Transmit(PduID, &PDU_To_send) == E_OK ){
				CanTp_TStart(&pNsdu->N_As);
				CanTp_TStart(&pNsdu->N_Bs);
				//JUSTYNA tu w sumie nie wiem jakie, o te chodzi? XD w sensie czy tu tez normlnie wysylamy
				pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;
				pNsdu->TxState.CanTp_BytesSent = 2;
				retval = E_OK;
			}
			else{
			 /*
				 [SWS_CanTp_00343]  CanTp shall terminate the current  transmission connection
				  when CanIf_Transmit() returns E_NOT_OK when transmitting an SF, FF, of CF ()
			 */
				PduR_CanTpTxConfirmation(PduID, E_NOT_OK);
				retval = E_NOT_OK;
			}
		}
		else{
			//wrong length
			retval = E_NOT_OK;
		}
	}
	else if(pNsdu->TxState.CanTp_BytesSent >= pNsdu->TxState.CanTp_MsgLegth){
		//something went wrong nothing to send
		CanTP_MemSet(&pNsdu->TxState, 0, sizeof(pNsdu->TxState));
		pNsdu->TxState.CanTp_TxState= CANTP_TX_WAIT;
	}
	else{
		//CF frame to be sent

		if(pNsdu->TxState.CanTp_TxState == CANTP_TX_PROCESSING)
		{
			PduInfoType PduInfoCopy;
			BufReq_ReturnType Buf_Status;
			PduLengthType buffer_size;
			CanTP_MemSet(&PduInfoCopy, 0, sizeof(PduInfoCopy));
			/*
			 * [SWS_CanTp_00167] ⌈After a transmission request from upper layer, the CanTp
			 * module shall start time-out N_Cs before the call of PduR_CanTpCopyTxData. If no
			 * data is available before the timer elapsed, the CanTp module shall abort the
			 * communication. ⌋ ( )
			 * @Justyna
			 */
			Buf_Status = PduR_CanTpCopyTxData(PduID, &PduInfoCopy, NULL, &buffer_size);
			if(Buf_Status == BUFREQ_OK){
				PduInfoType PduToBeSent;
				CanTP_MemSet(&PduToBeSent, 0, sizeof(PduToBeSent));
				PduToBeSent.SduLength = 7;
				//PduToBeSent.SduLength = PduInfoCopy.SduLength;
				PduToBeSent.SduDataPtr[0] = ConsecutiveFrame << 4;
				PduToBeSent.SduDataPtr[0] |= pNsdu->TxState.CanTp_SN & 0xF;
				for(uint8 data_iter = 0; data_iter < 7; data_iter++){
					PduToBeSent.SduDataPtr[data_iter + 1] = PduToBeSent.SduDataPtr[data_iter];
				}
				pNsdu->TxState.CanTp_SN++;
				pNsdu->TxState.CanTp_BytesSent += 7;
				//pNsdu->TxState.CanTp_BytesSent += PduInfoCopy.SduLength;;
				retval = CanIf_Transmit(PduID, &PduToBeSent);
				if(retval == E_OK){
					pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;
				}
				else{
					CanTP_CopyDefaultNsduConfig(pNsdu);
					PduR_CanTpTxConfirmation(PduID, E_NOT_OK);
					//Error
				}
			}
			else{
				//Error
				retval = E_NOT_OK;
			}
		}

	}

	return retval;
}

/*
 * Send Flow Control frame according to information in PCI structure
 */
static Std_ReturnType CanTP_SendFlowControlFrame(PduIdType PduID, CanPCI_Type *CanPCI){
	Std_ReturnType retval = E_NOT_OK;
	PduInfoType PduInfo = {0};
	uint8_t data[8];
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(PduID);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if((CanPCI->FS == FS_OVFLW) || (CanPCI->FS == FS_WAIT ) || (CanPCI->FS == FS_CTS)){
		PduInfo.SduDataPtr = data;
		PduInfo.SduDataPtr[0] = FlowControlFrame << 4; // FrameID
		PduInfo.SduDataPtr[0] |= (CanPCI->FS & 0xF);
		PduInfo.SduDataPtr[1] = CanPCI->BS;
		PduInfo.SduDataPtr[2] = CanPCI->ST;
		PduInfo.SduLength = sizeof(data);
		retval =  CanIf_Transmit(PduID, &PduInfo);
		if(retval == E_NOT_OK){
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			PduR_CanTpRxIndication(PduID, E_NOT_OK);
		}
		else{
			CanTp_TStart(&pNsdu->N_Ar);
			if(CanPCI->FS == FS_CTS){
				CanTp_TStart(&pNsdu->N_Cr);
			}
			else if(CanPCI->FS == FS_WAIT ){
				CanTp_TStart(&pNsdu->N_Br);
			}
		}
	}
	else{
		retval = E_NOT_OK;
	}
	return retval;
}

/*
 * Initializes the CanTP module
 */
void CanTp_Init (const CanTp_ConfigType* CfgPtr)
{
	//Init all to 0
	CanTP_MemSet(&CanTP_State, 0, sizeof(CanTP_State));
	//CanTP in on state
	CanTP_State.CanTP_State = CANTP_ON;
	//And tx and rx is waiting
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		CanTP_CopyDefaultNsduConfig(&CanTP_State.Nsdu[NsduIter]);

	}

}

/*
 * returns a struct with version info of CanTP module
 */
void CanTp_GetVersionInfo ( Std_VersionInfoType* versioninfo)
{
	if(versioninfo != NULL)
	{
		versioninfo->vendorID = _VENDOR_ID;
		versioninfo->moduleID = _MODULE_ID;
		versioninfo->instanceID = _INSTANCE_ID;
		versioninfo->ar_major_version = CANTP_AR_MAJOR_VERSION;
		versioninfo->ar_minor_version = CANTP_AR_MINOR_VERSION;
		versioninfo->ar_patch_version = CANTP_AR_PATCH_VERSION;
		versioninfo->sw_major_version = CANTP_SW_MAJOR_VERSION;
		versioninfo->sw_minor_version = CANTP_SW_MINOR_VERSION;
		versioninfo->sw_patch_version = CANTP_SW_PATCH_VERSION;
	}
}

/*
 * Cancels transmission
 */
Std_ReturnType CanTp_CancelTransmit (PduIdType TxPduId)
{
	Std_ReturnType ret = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(TxPduId);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if(CanTP_State.CanTP_State == CANTP_ON)
	{
		PduR_CanTpTxConfirmation(TxPduId, E_NOT_OK);
		CanTP_CopyDefaultNsduConfig(pNsdu);
		pNsdu->TxState.CanTp_TxState = CANTP_TX_WAIT;
		CanTp_TReset(&pNsdu->N_As);
		CanTp_TReset(&pNsdu->N_Bs);
		CanTp_TReset(&pNsdu->N_Cs);
		ret = E_OK;
	}
	return ret;
}

/*
 * Turns off CanTP module
 */
void CanTp_Shutdown (void)
{
	//TODO Stop all connections
	//reset all timers
	for(uint8 NsduIter = 0; NsduIter < NO_OF_NSDUS; NsduIter++){
		CanTP_MemSet(&CanTP_State.Nsdu[NsduIter], 0, sizeof(CanTP_State.Nsdu[NsduIter]));
		CanTP_State.Nsdu[NsduIter].RxState.CanTp_RxState = CANTP_RX_WAIT;
		CanTP_State.Nsdu[NsduIter].TxState.CanTp_TxState = CANTP_TX_WAIT;

		//reset all timers
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_As);
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_Bs);
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_Cs);
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_Ar);
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_Br);
		CanTp_TReset(&CanTP_State.Nsdu[NsduIter].N_Cr);
	}

	CanTP_State.CanTP_State = CANTP_OFF;
}


/*
 * Returns a parameter for rx connection
 */
Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value)
{
	uint16 val = 0;
	Std_ReturnType ret = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(id);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if((CanTP_State.CanTP_State == CANTP_ON) &&
        (pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT))
	{
		switch(parameter)
		{
			case TP_STMIN:
				val = pNsdu->RxState.sTMin;
				ret = E_OK;
				break;
			case TP_BS:
				val = pNsdu->RxState.bs;
				ret = E_OK;
				break;
			default:
				return E_NOT_OK;
		}
		*value = val;
	}
	return ret;
}

/*
 * Cancels receive process
 */
Std_ReturnType CanTp_CancelReceive(PduIdType RxPduId)
{
	Std_ReturnType ret = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if(CanTP_State.CanTP_State == CANTP_ON)
	{
		CanTP_CopyDefaultNsduConfig(pNsdu);
		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
		PduR_CanTpRxConfirmation(RxPduId, E_NOT_OK);
		ret = E_OK;
	}

	return ret;
}

/*
 * Changes a rx connection parameter, assumed to be unused as it is optional
 */
Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
	Std_ReturnType ret = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(id);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if((CanTP_State.CanTP_State == CANTP_ON) &&
		(pNsdu->RxState.CanTp_RxState == CANTP_RX_WAIT))
	{
		switch(parameter)
		{
			case TP_STMIN:
				pNsdu->RxState.sTMin = value;
				ret = E_OK;
				break;
			case TP_BS:
				pNsdu->RxState.bs = value;
				ret = E_OK;
				break;
			default:
				break;
		}
    }
	return ret;
}


/*
 * Transmits or starts a transission of PDU
 */
Std_ReturnType CanTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr )
{
    Std_ReturnType tmp_return = E_NOT_OK;
    PduInfoType Tmp_Pdu;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetFreeNsdu(TxPduId);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
    CanTP_MemSet(&Tmp_Pdu, 0, sizeof(Tmp_Pdu));
    //Has to be on to transmit
    if (CanTP_State.CanTP_State != CANTP_ON)
    {
    	//CAN_TP is off
    	//We can report an error here
    	//TODO: Add req here
    	return E_NOT_OK;
    }
	if (PduInfoPtr == NULL_PTR)
	{
    	//nullptr passed as PduInfoPtr, nothing to send
    	//We can report an error here
    	return E_NOT_OK;
	}
	if (PduInfoPtr->SduLength == 0)
	{
    	//no data to send
    	return E_NOT_OK;
	}

	if (PduInfoPtr->MetaDataPtr != NULL_PTR)
	{

		/* SWS_CanTp_00334: When CanTp_Transmit is called for an N-SDU with MetaData,
		 * the CanTp module shall store the addressing information contained in the
		 * MetaData of the N-SDU and use this information for transmission of SF, FF,
		 * and CF N-PDUs and for identification of FC N-PDUs. The addressing information
		 * in the MedataData depends on the addressing format:
		 * - Normal: none
		 * - Extended: N_TA
		 * - Mixed 11 bit: N_AE
		 * - Normal fixed: N_SA, N_TA
		 * - Mixed 29 bit: N_SA, N_TA, N_AE. */
		//We assume Normal addressing format so we ignore it
		//We do static config
	}
	else
	{
		//Do nothing
	}
	CanTP_CopyDefaultNsduConfig(pNsdu);
	pNsdu->CanTp_NsduID = TxPduId;
	pNsdu->TxState.CanTp_MsgLegth = PduInfoPtr->SduLength;
	pNsdu->TxState.CanTp_TxState = CANTP_TX_PROCESSING;
	CanTp_TReset(&pNsdu->N_As);
	CanTp_TReset(&pNsdu->N_Bs);
	CanTp_TReset(&pNsdu->N_Cs);
	CanTp_TStart(&pNsdu->N_As);
	CanTp_TStart(&pNsdu->N_Bs);
	tmp_return = E_OK;

    return tmp_return;
}


/*
 * Handle Consecutive frame reception according to input parameters and internal state
 */
static Std_ReturnType CanTp_ConsecutiveFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
    PduLengthType buffer_size;
    BufReq_ReturnType buf_Status;
    PduInfoType ULPduInfo;
    uint16 required_blocks;
    Std_ReturnType retval = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}

    CanTp_TReset(&pNsdu->N_Cr);

	if(pNsdu->RxState.CanTp_ExpectedCFNo == Can_PCI->SN)
	{
		ULPduInfo.SduDataPtr = PduInfoPtr->SduDataPtr+1;
		ULPduInfo.SduLength = Can_PCI->FrameLength;
		/*
		 * [SWS_CanTp_00269] ⌈After reception of each Consecutive Frame the CanTp module shall
		 * call the PduR_CanTpCopyRxData() function with a PduInfo pointer
		 * containing data buffer and data length:
		*/
		buf_Status = PduR_CanTpCopyRxData(RxPduId,  &ULPduInfo, &buffer_size);
		if(buf_Status == BUFREQ_OK)
		{
			pNsdu->RxState.CanTp_ReceivedBytes += PduInfoPtr->SduLength;
			//CanTP_State.RxState.CanTp_ReceivedBytes += Can_PCI->FrameLength
			pNsdu->RxState.CanTp_NoOfBlocksTillCTS--;
			if(pNsdu->RxState.CanTp_MessageLength == pNsdu->RxState.CanTp_ReceivedBytes){
				//All data received
				/*
				 * [SWS_CanTp_00084] ⌈When the transport reception session is completed
				 * (successfully or not) the CanTp module shall call the upper layer
				 * notification service PduR_CanTpRxIndication().
				 */
				PduR_CanTpRxIndication(RxPduId, E_OK);
				CanTP_CopyDefaultNsduConfig(pNsdu);
				pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
				retval = E_OK;
			}
			else{
				//We will need to send flow control
				CanPCI_Type FlowControl_PCI = {0};
				//More expected
				pNsdu->RxState.CanTp_ExpectedCFNo++;
				//check if FC needed
				//We assume a buffer is locked so we did not lose any space
				if(pNsdu->RxState.CanTp_NoOfBlocksTillCTS == 0){
					//We may require time so lets calculate how many block we can receive
					required_blocks = buffer_size / 7;
					// if non zero then we can at least receive one frame
					if(required_blocks > 0){
						FlowControl_PCI.FrameType = FlowControlFrame;
						FlowControl_PCI.FS = FS_CTS;
						FlowControl_PCI.BS = buffer_size;
						FlowControl_PCI.ST = 0;
						CanTP_SendFlowControlFrame(RxPduId, &FlowControl_PCI);
						pNsdu->RxState.CanTp_NoOfBlocksTillCTS = required_blocks;
						pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
					}
					else{
						FlowControl_PCI.FrameType = FlowControlFrame;
						FlowControl_PCI.FS = FS_WAIT;
						FlowControl_PCI.BS = buffer_size;
						FlowControl_PCI.ST = 0;
						CanTP_SendFlowControlFrame(RxPduId, &FlowControl_PCI);
						pNsdu->RxState.CanTp_RxState = CANTP_RX_SUSPENDED;
					}
					retval = E_OK;
				}
				else{
					//We still got more frames before FC
					//Nothing to do
					retval = E_OK;
				}
			}
		}
		else{
			//Bufreq error
			/* [SWS_CanTp_00271] ⌈If the PduR_CanTpCopyRxData() returns BUFREQ_E_NOT_OK
			 * after reception of a Consecutive Frame in a block the CanTp shall abort the
			 * reception of N-SDU and notify the PduR module by calling the
			 * PduR_CanTpRxIndication() with the result E_NOT_OK.
			 */
			PduR_CanTpRxIndication(RxPduId, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
		}
	}
	else{
		//Zły numer SN ramki
		/*
		 * [SWS_CanTp_00314] ⌈The CanTp shall check the correctness of each SN received during a segmented reception.
		 * In case of wrong SN received the CanTp module shall abort reception and notify the upper layer of this failure
		 * by calling the indication function PduR_CanTpRxIndication() with the result E_NOT_OK.
		*/
		PduR_CanTpRxIndication(RxPduId, E_NOT_OK);
		CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	}
    return retval;
}
/*
 * Handle Flow Control frame reception according to input parameters and internal state
 */
static Std_ReturnType CanTp_FlowFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	Std_ReturnType retval = E_NOT_OK;
	//We received FC so we are transmitting
	//Did we get correct frame?
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	if(pNsdu == NULL){
		return E_NOT_OK;
	}
	if((pNsdu->TxState.CanTp_TxState == CANTP_TX_PROCESSING) || (pNsdu->TxState.CanTp_TxState == CANTP_TX_SUSPENDED))
	{
		/*
		 * [SWS_CanTp_00057] ⌈If unexpected frames are received, the CanTp module shall
		 * behave according to the table below. This table specifies the N-PDU handling
		 * considering the current CanTp internal status (CanTp status). ⌋
		 *
		 *  (SRS_Can_01082)
		 *  It must be understood, that the received N-PDU contains the same address
		 *  information (N_AI) as the reception or transmission, which may be in progress at the
		 *  time the N_PDU is received.
		 */
		if(pNsdu->CanTp_NsduID == RxPduId){
			switch(Can_PCI->FS){
				case FS_CTS:
					pNsdu->TxState.CanTp_BlocksToFCFrame = Can_PCI->BS;
					retval = E_OK;
					//Todo @Paulina Send consecutive frame
					break;
				case FS_WAIT:
					CanTp_TReset(&pNsdu->N_Bs);
					CanTp_TReset(&pNsdu->N_Bs);
					retval = E_OK;
					break;
				case FS_OVFLW:
					/*[SWS_CanTp_00309] ⌈If a FC frame is received with the FS set to OVFLW the CanTp module shall
					 * abort the transmit request and notify the upper layer by calling the callback
					 * function PduR_CanTpTxConfirmation() with the result E_NOT_OK. ⌋
					 */
					PduR_CanTpTxConfirmation(RxPduId, E_NOT_OK);
					CanTP_MemSet(&pNsdu->TxState, 0, sizeof(pNsdu->TxState));
					pNsdu->TxState.CanTp_TxState = CANTP_TX_WAIT;
					retval = E_OK;
					break;
				default:
					PduR_CanTpTxConfirmation(RxPduId, E_NOT_OK);
					CanTP_MemSet(&pNsdu->TxState, 0, sizeof(pNsdu->TxState));
					pNsdu->TxState.CanTp_TxState = CANTP_TX_WAIT;
					retval = E_OK;
					break;
			}
		}
	}
	return retval;
}
/*
 * Handle Single frame reception according to input parameters and internal state
 */
static Std_ReturnType CanTp_SingleFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	BufReq_ReturnType Buf_Status;
	PduLengthType buffer_size;
	Std_ReturnType retval = E_NOT_OK;
	PduInfoType ULPduInfo;
	Buf_Status = PduR_CanTpStartOfReception(RxPduId, &ULPduInfo, Can_PCI->FrameLength, &buffer_size);
	switch(Buf_Status)
	{
		case BUFREQ_OK:
			if(buffer_size >= Can_PCI->FrameLength){
				ULPduInfo.SduDataPtr = PduInfoPtr->SduDataPtr + 1;
				ULPduInfo.SduLength = Can_PCI->FrameLength;
				PduR_CanTpCopyRxData(RxPduId, &ULPduInfo, &buffer_size);
				PduR_CanTpRxIndication (RxPduId, Buf_Status);
				retval = E_OK;
			}
			else{
				/* [SWS_CanTp_00339] ⌈After the reception of a First Frame or Single Frame,
				 * if the function PduR_CanTpStartOfReception() returns BUFREQ_OK with a
				 * smaller available buffer size than needed for the already received data,
				 * the CanTp module shall abort the reception of the N-SDU and call
				 *  PduR_CanTpRxIndication() with the result E_NOT_OK. ⌋ ( )
				 */
				PduR_CanTpRxIndication (RxPduId, E_NOT_OK);
				retval = E_NOT_OK;
			}
			break;
		default:
            /* [SWS_CanTp_00081] ⌈After the reception of a First Frame or Single Frame,
             *  if the function PduR_CanTpStartOfReception()returns BUFREQ_E_NOT_OK to
             *  the CanTp module, the CanTp module shall abort the reception of this N-SDU.
             *  No Flow Control will be sent and PduR_CanTpRxIndication() will not be called
             *  in this case. ⌋ ( )
            */

            /*
             * [SWS_CanTp_00353]⌈After the reception of a Single Frame, if the function
             * PduR_CanTpStartOfReception()returns BUFREQ_E_OVFL to the CanTp module,
             * the CanTp module shall abort the N-SDU reception.⌋()
            */
			retval = E_NOT_OK;
			break;
	}

	return retval;
}

/*
 * Handle first frame reception according to input parameters and internal state
 */
static Std_ReturnType CanTp_FirstFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	BufReq_ReturnType Buf_Status;
	PduLengthType buffer_size;
	Std_ReturnType retval = E_NOT_OK;
	PduInfoType ULPduInfo;
	//Prepare PCI container for FC
	CanPCI_Type FlowControl_PCI = {0};
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	if(pNsdu != NULL){
		return E_NOT_OK;
	}
	/*
	 * [SWS_CanTp_00166] ⌈At the reception of a FF or last CF of a block, the CanTp
	 * module shall start a time-out N_Br before calling PduR_CanTpStartOfReception or PduR_CanTpCopyRxData. ⌋ ( )
	 */



	/*
	 * CanTp ask PDU Router to make a buffer available for
	 *incoming data with PduR_CanTpStartOfReception callback.
	 *
	 */
	if(Can_PCI->FrameLength < 4096){
		ULPduInfo.SduDataPtr = PduInfoPtr->SduDataPtr + 2;
		ULPduInfo.SduLength = PduInfoPtr->SduLength;
	}
	else{
		ULPduInfo.SduDataPtr = PduInfoPtr->SduDataPtr + 6;
		ULPduInfo.SduLength = PduInfoPtr->SduLength;
	}
	Buf_Status = PduR_CanTpStartOfReception(RxPduId, &ULPduInfo, Can_PCI->FrameLength, &buffer_size);
    /*
    [SWS_CanTp_00318] ⌈After the reception of a First Frame, if the function PduR_CanTpStartOfReception()returns BUFREQ_E_OVFL to the CanTp module,
    the CanTp module shall send a Flow Control N-PDU with overflow status (FC(OVFLW)) and abort the N-SDU reception. */
	if(Buf_Status == BUFREQ_E_OVFL){ //Maybe change to switch-case?
		/*
		 * [SWS_CanTp_00318] ⌈After the reception of a First Frame, if the function
		 * PduR_CanTpStartOfReception()returns BUFREQ_E_OVFL to the CanTp module,
		 * the CanTp module shall send a Flow Control N-PDU with overflow status
		 * (FC(OVFLW)) and abort the N-SDU reception.⌋ ( )
		 */
		FlowControl_PCI.FrameType = FlowControlFrame;
		FlowControl_PCI.FS = FS_OVFLW;
		FlowControl_PCI.BS = buffer_size;
		FlowControl_PCI.ST = 0;

		CanTP_SendFlowControlFrame(RxPduId, &FlowControl_PCI);
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_E_NOT_OK){
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_E_BUSY){
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_OK){
		//TODO check if we have free nsdu slot
		pNsdu = CanTP_GetFreeNsdu(RxPduId);
		if(pNsdu == NULL)
		{PduR_CanTpRxIndication(RxPduId, E_NOT_OK);
		retval = E_NOT_OK;
	}
	else{
		pNsdu->RxState.CanTp_MessageLength = Can_PCI->FrameLength;
		pNsdu->CanTp_NsduID = RxPduId;
        /*
         * [SWS_CanTp_00339] ⌈After the reception of a First Frame or Single Frame, if the
         * function PduR_CanTpStartOfReception() returns BUFREQ_OK with a smaller
         * available buffer size than needed for the already received data, the CanTp module
         * shall abort the reception of the N-SDU and call PduR_CanTpRxIndication() with
         * the result E_NOT_OK. ⌋ ( )
         */
        //@Justyna Time-Out N-Br
		//JUSTYNA nie wiem czy to dobrze jest??????
		/*
		 * [SWS_CanTp_00082] ⌈After the reception of a First Frame, if the function
		 * PduR_CanTpStartOfReception() returns BUFREQ_OK with a smaller available
		 * buffer size than needed for the next block, the CanTp module shall start the timer N_Br.⌋ ( )
		 */
		//
        // so if less than 4096
			if(buffer_size < PduInfoPtr->SduLength){
				//[SWS_CanTp_00339]
				// We need at least SduLength bytes for FF if SDU length is < 4096
				CanTp_TStart(&pNsdu->N_Br);
				PduR_CanTpRxIndication(RxPduId, E_NOT_OK);
				retval = E_NOT_OK;
			}
			else{
				//[SWS_CanTp_00082] for the next block we need SduLength + 7
				if(buffer_size < PduInfoPtr->SduLength + 7)
				//if(buffer_size < 14)
				{
					CanTp_TStart(&pNsdu->N_Br);
					FlowControl_PCI.FrameType = FlowControlFrame;
					FlowControl_PCI.FS = FS_WAIT;
					FlowControl_PCI.BS = buffer_size;
					FlowControl_PCI.ST = 0;
					CanTP_SendFlowControlFrame(RxPduId, &FlowControl_PCI);
				}
				else{
					FlowControl_PCI.FrameType = FlowControlFrame;
					FlowControl_PCI.FS = FS_CTS;
					FlowControl_PCI.BS = buffer_size;
					FlowControl_PCI.ST = 0;
					CanTP_SendFlowControlFrame(RxPduId, &FlowControl_PCI);
				}
				pNsdu->RxState.sTMin = FlowControl_PCI.ST;
				pNsdu->RxState.bs = FlowControl_PCI.BS;
				retval = E_OK;
			}
		}
        if(retval == E_OK)
        {
        	PduR_CanTpCopyRxData(RxPduId, &ULPduInfo, &buffer_size);
        	pNsdu->RxState.CanTp_RxState = CANTP_RX_PROCESSING;
        	pNsdu->CanTp_NsduID = RxPduId;
        	pNsdu->RxState.CanTp_NoOfBlocksTillCTS = (buffer_size/7);
        }
	}
	else{
		retval = E_NOT_OK;
	}
	return retval;
}

/*
 * @Brief
 * Fill the CanPCI_Type with info decoded from frame
 */
static Std_ReturnType CanTp_GetPCI(const PduInfoType* PduInfoPtr, CanPCI_Type* CanPCI)
{
	Std_ReturnType ret = E_NOT_OK;
	CanTP_MemSet(CanPCI, 0, sizeof(CanPCI));
	switch((PduInfoPtr->SduDataPtr[0]) >> 4)
	{
		case FirstFrame:
			CanPCI->FrameType = FirstFrame;
			if(((PduInfoPtr->SduDataPtr[0] & 0xF) == 0x00) && (PduInfoPtr->SduDataPtr[1] == 0x00)){
				//more than 4096 bytes
				CanPCI->FrameLength = PduInfoPtr->SduDataPtr[2];
				CanPCI->FrameLength = ((CanPCI->FrameLength << 8) | PduInfoPtr->SduDataPtr[3]);
				CanPCI->FrameLength = ((CanPCI->FrameLength << 8) | PduInfoPtr->SduDataPtr[4]);
				CanPCI->FrameLength = ((CanPCI->FrameLength << 8) | PduInfoPtr->SduDataPtr[5]);
			}
			else
			{
				CanPCI->FrameLength = PduInfoPtr->SduDataPtr[0] & 0xF;
				CanPCI->FrameLength = ((CanPCI->FrameLength << 8) | PduInfoPtr->SduDataPtr[1]);
			}
			ret = E_OK;
			break;
		case SingleFrame:
			CanPCI->FrameType = SingleFrame;
			if((PduInfoPtr->SduDataPtr[0] & 0xF) == 0x00){
				//CAN FD only, not really needing this, not going to implement this further
				CanPCI->FrameLength = (uint32)PduInfoPtr->SduDataPtr[1];
			}
			else{
				CanPCI->FrameLength = (uint32)(PduInfoPtr->SduDataPtr[0] & 0xF);
			}
			ret = E_OK;
			break;
		case FlowControlFrame:
			CanPCI->FrameType = FlowControlFrame;
			CanPCI->FS = (CanTP_FCStatusType)(PduInfoPtr->SduDataPtr[0] & 0xF);
			CanPCI->BS = PduInfoPtr->SduDataPtr[1];
			CanPCI->ST = PduInfoPtr->SduDataPtr[2];
			ret = E_OK;
			break;
		case ConsecutiveFrame:
			CanPCI->FrameType = ConsecutiveFrame;
			CanPCI->SN = PduInfoPtr->SduDataPtr[0] & 0xF;
			ret = E_OK;
			break;
		default:
			//put garbage data into frame type (forcing default in switch-case)
			CanPCI->FrameType = (CanTPFrameType)-1;
			ret = E_NOT_OK;
			break;
	}
	return ret;
}


/*
 * Handle Rx indication if CanTP rx is in Wait state
 */
static void CanTp_RxIndicationHandleWaitState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	// In wait state we should only receive start of communication or flow control frame if we are waiting for it
	// disregard CF
	Std_ReturnType retval = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case SingleFrame:
			retval = CanTp_SingleFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case FlowControlFrame:
			retval = CanTp_FlowFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case ConsecutiveFrame:
			//Do nothing, ignore
			break;
		default:
			//Error to be reported or ignored
			break;
	}
	if(retval != E_OK)
	{
		//report error or abort
		CanTP_CopyDefaultNsduConfig(pNsdu);
		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	}
}

/*
 * Handle Rx indication if CanTP rx is in Processing state
 */
static void CanTp_RxIndicationHandleProcessingState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	/*
	 * In processing state we already have received SF and FF so we restart communication if we receive another one
	 *
	 *
	 */
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	if(pNsdu == NULL){
		//report error
	}
	Std_ReturnType retval = E_NOT_OK;
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			PduR_CanTpRxIndication(pNsdu->CanTp_NsduID, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			CanTp_TReset(&pNsdu->N_Ar);
			CanTp_TReset(&pNsdu->N_Br);
			CanTp_TReset(&pNsdu->N_Cr);
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication(pNsdu->CanTp_NsduID, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			CanTp_TReset(&pNsdu->N_Ar);
			CanTp_TReset(&pNsdu->N_Br);
			CanTp_TReset(&pNsdu->N_Cr);
			retval = CanTp_SingleFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case FlowControlFrame:
			retval = CanTp_FlowFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case ConsecutiveFrame:
			retval = CanTp_ConsecutiveFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		default:
			//Error to be reported or ignored
			break;
	}
	if(retval != E_OK)
	{
		//report error or abort
		CanTP_CopyDefaultNsduConfig(pNsdu);
		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	}
}

/*
 *Handle Rx indication if CanTP rx is in Suspended state (waits for buffer)
 */
static void CanTp_RxIndicationHandleSuspendedState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	//Same as processing (N_Br timer expired and not enough buffer space)
	//FF and SF cause restart of the reception
	Std_ReturnType retval = E_NOT_OK;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			PduR_CanTpRxIndication (RxPduId, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication (pNsdu->CanTp_NsduID, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_SingleFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case FlowControlFrame:
			//We might receive FC frame
			retval = CanTp_FlowFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case ConsecutiveFrame:
			//We should not receive CF at this time as we do not have enough buffer space
			//We reset communication and report an error
			PduR_CanTpRxIndication (RxPduId, E_NOT_OK);
			CanTP_MemSet(&pNsdu->RxState, 0, sizeof(pNsdu->RxState));
			pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
			break;
		default:
			//Error to be reported or ignored
			break;
	}
	if(retval != E_OK)
	{
		//report error or abort
		CanTP_CopyDefaultNsduConfig(pNsdu);
		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
	}
}

/*
 *[SWS_CanTp_00078] ⌈For reception indication, the CanTp module shall provide
 * CanTp_RxIndication().⌋ ( )
 *
 */
void CanTp_RxIndication (PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{

    CanPCI_Type Can_PCI;
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(RxPduId);
    if(CanTP_State.CanTP_State == CANTP_OFF)
    {
    	//We can report error here, nothing to do
    	return;
    }

	if(pNsdu == NULL){
		//report error
		return;
	}
    CanTp_GetPCI(PduInfoPtr, &Can_PCI);
    switch(pNsdu->RxState.CanTp_RxState)
    {
    	case CANTP_RX_WAIT:
    		CanTp_RxIndicationHandleWaitState(RxPduId, PduInfoPtr, &Can_PCI);
    		break;
    	case CANTP_RX_PROCESSING:
    		CanTp_RxIndicationHandleProcessingState(RxPduId, PduInfoPtr, &Can_PCI);
    		break;
    	case CANTP_RX_SUSPENDED:
    		CanTp_RxIndicationHandleSuspendedState(RxPduId, PduInfoPtr, &Can_PCI);
    		break;
    	default:
    		//Report error
    		//JUSTYNA tu tez nie dziala xdddddd
    		CanTP_MemSet(&CanTP_State.Nsdu, 0, sizeof(CanTP_State.Nsdu));
    		pNsdu->RxState.CanTp_RxState = CANTP_RX_WAIT;
    		CanTp_TReset(&pNsdu->N_Ar);
    		CanTp_TReset(&pNsdu->N_Br);
    		CanTp_TReset(&pNsdu->N_Cr);
    		break;
    }
}


/*
 *
 * [SWS_CanTp_00076] ⌈For confirmation calls, the CanTp module shall provide the
 * function CanTp_TxConfirmation().⌋ ( )
 *
 */
void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
	CanTP_NSdu_Type *pNsdu = NULL;
	pNsdu = CanTP_GetNsduFromPduID(TxPduId);
	if(pNsdu == NULL){
		//report error
		return;
	}
	if(CanTP_State.CanTP_State == CANTP_OFF)
	{
		//report error
	}

	else{
		if(result == E_NOT_OK){
			PduR_CanTpTxConfirmation(TxPduId, result);
			CanTP_MemSet(&pNsdu->TxState, 0, sizeof(pNsdu->TxState));
			pNsdu->TxState.CanTp_TxState= CANTP_TX_WAIT;
			CanTp_TReset(&pNsdu->N_As);
			CanTp_TReset(&pNsdu->N_Bs);
			CanTp_TReset(&pNsdu->N_Cs);
			//We do not retry transmission, abort
			/*
			 * [SWS_CanTp_00355] ⌈ CanTp shall abort the corrensponding session, when
			 * CanTp_TxConfirmation() is called with the result E_NOT_OK.⌋ ()
			 *
			 */
		}
		else if(result == E_OK){
			//did we finish transmission ?
			if(pNsdu->TxState.CanTp_BytesSent == pNsdu->TxState.CanTp_MsgLegth){
				PduR_CanTpTxConfirmation(TxPduId, result);
				CanTP_CopyDefaultNsduConfig(pNsdu);
			}
			else{
				//We are still transmitting
				CanTp_TReset(&pNsdu->N_As);
				CanTp_TReset(&pNsdu->N_Bs);
				CanTp_TReset(&pNsdu->N_Cs);
				pNsdu->TxState.CanTp_BlocksToFCFrame--;
				if(pNsdu->TxState.CanTp_BlocksToFCFrame == 0){
					pNsdu->TxState.CanTp_TxState = CANTP_TX_SUSPENDED;
				}
				else{
					pNsdu->TxState.CanTp_TxState = CANTP_TX_PROCESSING;
				}
			}
		}
		else{
			//error
		}
	}
}

/*------------------------------------------------------------------------------------------------*/
/* global scheduled function definitions.                                                         */
/*------------------------------------------------------------------------------------------------*/

void CanTp_MainFunction(void){
	//Just for linter
	BufReq_ReturnType BufReq_State = BUFREQ_E_NOT_OK;
	uint16 block_size = 0;

	PduLengthType buffer_len;
	PduInfoType ULPduInfo= {0,0};
	CanPCI_Type FlowControl_PCI = {0};

	CanTp_Timer_type *N_Ar;
	CanTp_Timer_type *N_Br;
	CanTp_Timer_type *N_Cr;

	CanTp_Timer_type *N_As;
	CanTp_Timer_type *N_Bs;
	CanTp_Timer_type *N_Cs;

	for(uint8 nsdu_iter = 0; nsdu_iter < NO_OF_NSDUS; nsdu_iter++)
	{
		CanTP_NSdu_Type nsdu = CanTP_State.Nsdu[nsdu_iter];
		//static boolean N_Ar_timeout, N_Br_timeout, N_Cr_timeout;
		//TODO Do stuff here
		N_Ar = &nsdu.N_Ar;
		N_Br = &nsdu.N_Br;
		N_Cr = &nsdu.N_Cr;

		N_As = &nsdu.N_As;
		N_Bs = &nsdu.N_Bs;
		N_Cs = &nsdu.N_Cs;

		CanTp_Timer_Incr(N_Ar);
		CanTp_Timer_Incr(N_Br);
		CanTp_Timer_Incr(N_Cr);

		CanTp_Timer_Incr(N_As);
		CanTp_Timer_Incr(N_Bs);
		CanTp_Timer_Incr(N_Cs);

		if(N_Br->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_Br)){
				nsdu.RxState.CanTpRxWftMax ++;
				N_Br->counter = 0;
				if(nsdu.RxState.CanTpRxWftMax >= FC_WAIT_FRAME_CTR_MAX){
					PduR_CanTpRxIndication(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
					CanTP_CopyDefaultNsduConfig(&nsdu);
					nsdu.RxState.CanTpRxWftMax = 0;
				}
				else{
					FlowControl_PCI.FrameType = FlowControlFrame;
					FlowControl_PCI.FS = FS_WAIT;
					FlowControl_PCI.BS = block_size;
					FlowControl_PCI.ST = 0;
					if(CanTP_SendFlowControlFrame(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, &FlowControl_PCI) == E_NOT_OK){
						CanTP_CopyDefaultNsduConfig(&nsdu);
					}
				}
			}
			else if(nsdu.RxState.CanTp_RxState==CANTP_RX_SUSPENDED){
			BufReq_State=PduR_CanTpCopyRxData(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, &ULPduInfo, &buffer_len);
			if(BufReq_State == BUFREQ_E_NOT_OK){
				PduR_CanTpRxIndication(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
			}
			else{
				if((nsdu.RxState.CanTp_MessageLength-nsdu.RxState.CanTp_ReceivedBytes)>=7){
				block_size=buffer_len/7;
				}
				else{
					block_size=buffer_len/(nsdu.RxState.CanTp_MessageLength-nsdu.RxState.CanTp_ReceivedBytes);
				}
				if(block_size > 0){

					CanTP_State.Nsdu[nsdu_iter].RxState.CanTp_NoOfBlocksTillCTS=block_size;
					CanTP_State.Nsdu[nsdu_iter].RxState.CanTp_RxState=CANTP_RX_PROCESSING;

					FlowControl_PCI.FrameType = FlowControlFrame;
					FlowControl_PCI.FS = FS_OVFLW;
					FlowControl_PCI.BS = block_size;
					FlowControl_PCI.ST = 0;

					if(CanTP_SendFlowControlFrame(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, &FlowControl_PCI) == E_NOT_OK){
						CanTP_CopyDefaultNsduConfig(&nsdu);
					}
					else{
						CanTp_TReset(N_Br);
					}
				}
			}
		}
	}
		if(N_Cr->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_Cr) == E_NOT_OK){
				PduR_CanTpRxIndication(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
				CanTP_CopyDefaultNsduConfig(&nsdu);
			}
		}
		if(N_Ar->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_Ar) == E_NOT_OK){
				PduR_CanTpRxIndication(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
				CanTP_CopyDefaultNsduConfig(&nsdu);
			}
		}
		if(N_Cs->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_Cs) == E_NOT_OK){
				PduR_CanTpTxConfirmation(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
				CanTP_CopyDefaultNsduConfig(&nsdu);
			}
		}
		if(N_Bs->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_Bs) == E_NOT_OK){
				PduR_CanTpTxConfirmation(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
				CanTP_CopyDefaultNsduConfig(&nsdu);
			}
		}
		if(N_As->state == TIMER_ACTIVE){
			if(CanTp_Timer_Timeout(N_As) == E_NOT_OK){
				PduR_CanTpTxConfirmation(CanTP_State.Nsdu[nsdu_iter].CanTp_NsduID, E_NOT_OK);
				CanTP_CopyDefaultNsduConfig(&nsdu);
			}
		}
		CanTP_NSDuTransmitHandler(nsdu.CanTp_NsduID);
	}
}
