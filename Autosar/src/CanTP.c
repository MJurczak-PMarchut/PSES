/*
 * CanTP.c
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */


#include "CanTP.h"
#include "mandatory_interfaces_mock.h"
#define CANTP_CAN_FRAME_SIZE (0x08u)

#ifndef CANTP_MAX_NUM_OF_N_SDU
#define CANTP_MAX_NUM_OF_N_SDU (0x10u)
#endif

#define NULL_PTR NULL


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
	CANTP_RX_SUSPENDED
} CanTp_RxState_Type;


typedef enum {
    CANTP_TX_WAIT = 0x00u,
    CANTP_TX_PROCESSING,
	CANTP_TX_SUSPENDED
} CanTp_TxState_Type;

typedef struct
{
    uint8 can[CANTP_CAN_FRAME_SIZE];
    PduLengthType size;
    PduLengthType rmng;
} CanTp_NSduBufferType;

typedef struct
{
    const CanTp_RxNSduType *cfg;
    CanTp_NSduBufferType buf;
    uint8 meta_data_lower[0x04u];
    uint8 meta_data_upper[0x04u];
    CanTp_NSaType saved_n_sa;
    CanTp_NTaType saved_n_ta;
    CanTp_NAeType saved_n_ae;
    boolean has_meta_data;
    CanTp_FlowStatusType fs;
    uint32 st_min;
    uint8 bs;
    uint8 sn;
    uint16 wft_max;
    PduInfoType can_if_pdu_info;
    PduInfoType pdu_r_pdu_info;
    struct
    {
    	CanTp_RxState_Type taskState;
        CanTp_FrameStateType state;
        /**
         * @brief structure containing all parameters accessible via @ref CanTp_ReadParameter/@ref
         * CanTp_ChangeParameter.
         */
        struct
        {
            uint32 st_min;
            uint8 bs;
        } m_param;
    } shared;
} CanTp_RxConnectionType;

typedef struct
{
    const CanTp_TxNSduType *cfg;
    CanTp_NSduBufferType buf;
    uint8 meta_data[0x04u];
    CanTp_NSaType saved_n_sa;
    CanTp_NTaType saved_n_ta;
    CanTp_NAeType saved_n_ae;
    boolean has_meta_data;
    CanTp_FlowStatusType fs;
    uint32 target_st_min;
    uint32 st_min;
    uint16 bs;
    uint8 sn;
    PduInfoType can_if_pdu_info;
    CanTp_TxState_Type taskState;
    struct
    {
        CanTp_FrameStateType state;
        uint32 flag;
    } shared;
} CanTp_TxConnectionType;

// struct of rx state variables
typedef struct{
    uint16 CanTp_MessageLength;
    uint16 CanTp_ExpectedCFNo;
    uint16 CanTp_ReceivedSent;
    CanTp_RxState_Type CanTp_RxState;
    PduIdType CanTp_CurrentRxId;
    uint8 CanTp_NoOfBlocksTillCTS;
} CanTp_RxStateVariablesType;

typedef struct{
    CanTp_TxState_Type CanTp_TxState;
    PduIdType CanTp_CurrentTxId;
    uint16 CanTp_BytesSent;
    uint8_t CanTp_SN;
    uint16 CanTp_BlocksToFCFrame;
    uint16 CanTp_MsgLegth;
} CanTp_TxStateVariablesType;

typedef struct
{
    CanTp_RxConnectionType rx;
    CanTp_TxConnectionType tx;
    uint32 n[0x06u];
    uint8_least dir;
    uint32 t_flag;
} CanTp_NSduType;

// typedef struct
// {
//     CanTp_NSduType sdu[CANTP_MAX_NUM_OF_N_SDU];
// } CanTp_ChannelRtType;
//

// static CanTp_ChannelRtType CanTp_Rt[CANTP_MAX_NUM_OF_CHANNEL];

typedef struct{
	CanTp_RxStateVariablesType 	RxState;
	CanTp_TxStateVariablesType	TxState;
	CanTp_StateType				CanTP_State;
} CanTP_InternalStateType;


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
    uint8 SN; // Sequence Nubmer of consecutive frame
    uint8 BS; // Block Size FC frame
    uint8 FS; // Status from flow control
    uint8 ST; // Sepsration Time FF
    CanTPFrameType FrameType;
    uint32 FrameLenght;
} CanPCI_Type;


static CanTP_InternalStateType CanTP_State;

static void* CanTP_MemSet(void* destination, int value, size_t num)
{
	uint8* dest = destination;
	for(uint64 u64Iter; u64Iter < num; u64Iter++)
	{
		dest[u64Iter] = (uint8)value;
	}
	return destination;
}

void CanTp_Init (const CanTp_ConfigType* CfgPtr)
{
	//Init all to 0
	CanTP_MemSet(&CanTP_State, 0, sizeof(CanTP_State));
	//CanTP in on state
	CanTP_State.CanTP_State = CANTP_ON;
	//And tx and rx is waiting
	CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
	CanTP_State.TxState.CanTp_TxState = CANTP_TX_WAIT;

}

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

Std_ReturnType CanTp_CancelTransmit (PduIdType TxPduId)
{
	Std_ReturnType ret = E_NOT_OK;
	CanTp_TxNSduType *nsdu;

	if((CanTP_State.CanTP_State == CANTP_ON))
	{
		PduR_CanTpTxConfirmation(TxPduId, E_NOT_OK);
		// nsdu->tx.taskState = CANTP_TX_WAIT;
		ret = E_OK;
	}
	return ret;
}

void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
	CanTp_NSduType *nsdu;
	if(E_OK)
	{
// ToDo
	}
	else
	{
		CanTp_CancelTransmit(TxPduId);
	}
}

void CanTp_Shutdown (void)
{
	//TODO Stop all connections
	CanTP_State.CanTP_State = CANTP_OFF;
}

Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value)
{
	CanTp_RxNSduType *nsdu;
	uint16 val = 0;
	Std_ReturnType ret = E_NOT_OK;

	if((CanTP_State.CanTP_State == CANTP_ON) &&
        (CanTP_State.RxState.CanTp_RxState == CANTP_RX_WAIT))
	{
		switch(parameter)
		{
			case TP_STMIN:
				val = nsdu->sTMin;
				ret = E_OK;
				break;
			case TP_BS:
				val = nsdu->bs;
				ret = E_OK;
				break;
			default:
				break;
		}
	}
	*value = val;
	return ret;
}

Std_ReturnType CanTp_CancelReceive(PduIdType RxPduId)
{
	CanTp_RxNSduType *nsdu;
	Std_ReturnType ret = E_NOT_OK;

	if(CanTP_State.CanTP_State == CANTP_ON)
	{
		PduR_CanTpRxConfirmation(RxPduId, E_NOT_OK);
	}

	return ret;
}

Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
	CanTp_RxNSduType *nsdu;
	Std_ReturnType ret = E_NOT_OK;

	if((CanTP_State.CanTP_State == CANTP_ON) &&
		(CanTP_State.RxState.CanTp_RxState == CANTP_RX_WAIT))
	{
		switch(parameter)
		{
			case TP_STMIN:
				nsdu->sTMin= value;
				ret = E_OK;
				break;
			case TP_BS:
				nsdu->bs = value;
				ret = E_OK;
				break;
			default:
				break;
		}
    }
	return ret;
}

Std_ReturnType CanTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr )
{
	CanTp_TxNSduType *p_n_sdu = NULL_PTR;
    Std_ReturnType tmp_return = E_NOT_OK;
    PduInfoType Tmp_Pdu;
    BufReq_ReturnType BufReq_State;
    PduLengthType Len_Pdu;
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
	if(CanTP_State.TxState.CanTp_TxState == CANTP_TX_PROCESSING)
	{
		/*[SWS_CanTp_00123] ⌈If the configured transmit connection channel is in use
		(state CANTP_TX_PROCESSING), the CanTp module shall reject new transmission
		requests linked to this channel. To reject a transmission, CanTp returns
		E_NOT_OK when the upper layer asks for a transmission with the
		CanTp_Transmit() function.⌋ (SRS_Can_01066) */
		return E_NOT_OK;
	}
	if(PduInfoPtr->SduLength < 8){
		//We only need to send a single frame
		Tmp_Pdu.SduLength = PduInfoPtr->SduLength;
		//call PduR
		BufReq_State = PduR_CanTpCopyTxData(TxPduId, &Tmp_Pdu, NULL, &Len_Pdu);
		if(BufReq_State == BUFREQ_OK){
			//Start sending single frame: SF
			//TODO Start sending data here
			CanTP_State.TxState.CanTp_TxState = CANTP_TX_PROCESSING;
		}
		else if (BufReq_State == BUFREQ_E_BUSY) {
			//And now we wait for the buffer to become ready
			//Start timers
		}
		else
		{
			//Undefined ?
			//TODO Make sure
			return E_NOT_OK;
		}
	}
	else
	{
		//Multiple frames to be sent
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
		}
		else
		{
		}

		/* SWS_CanTp_00206: the function CanTp_Transmit shall reject a request if the
		 * CanTp_Transmit service is called for a N-SDU identifier which is being used in a
		 * currently running CAN Transport Layer session. */
		if ((CanTP_State.TxState.CanTp_TxState != CANTP_TX_PROCESSING) &&
			(PduInfoPtr->SduLength > 0x0000u) && (PduInfoPtr->SduLength <= 0x0FFFu))
		{
		}
	}


    return tmp_return;
}


/*
 * TODO Add brief
 * Handle first frame reception
 * TODO Handle FirstFrame reception
 */
static Std_ReturnType CanTp_FirstFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	return E_NOT_OK;
}

static Std_ReturnType CanTp_GetPCI(const PduInfoType* PduInfoPtr, CanPCI_Type* CanPCI)
{
	//TODO Fill this
	return E_NOT_OK;
}

static void CanTp_RxIndicationHandleWaitState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	// In wait state we should only receive start of communication or flow control frame if we are waiting for it
	// disregard CF
	Std_ReturnType retval = E_NOT_OK;
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, &Can_PCI);
			break;
		case SingleFrame:
			//TODO Handle SF reception
			break;
		case FlowControlFrame:
			//TODO Handle FC reception
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
	}
}

static void CanTp_RxIndicationHandleProcessingState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	/*
	 * In processing state we already have received SF and FF so we restart communication if we receive another one
	 *
	 *
	 */
	Std_ReturnType retval = E_NOT_OK;
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			PduR_CanTpRxIndication(CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, &Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication(CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			//TODO Handle SF reception
			break;
		case FlowControlFrame:
			//TODO Handle FC reception
			break;
		case ConsecutiveFrame:
			//TODO Handle CF reception
			break;
		default:
			//Error to be reported or ignored
			break;
	}
	if(retval != E_OK)
	{
		//report error or abort
	}
}

static void CanTp_RxIndicationHandleSuspendedState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	//Same as processing (N_Br timer expired and not enough buffer space)
	//FF and SF cause restart of the reception
	Std_ReturnType retval = E_NOT_OK;
	switch(Can_PCI->FrameType)
	{
		case FirstFrame:
			PduR_CanTpRxIndication ( CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, &Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication ( CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			//TODO Handle SF reception
			break;
		case FlowControlFrame:
			//We might receive FC frame
			//TODO Handle FC reception
			break;
		case ConsecutiveFrame:
			//We should not receive CF at this time as we do not have enough buffer space
			//We reset communication and report an error
			PduR_CanTpRxIndication ( CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			break;
		default:
			//Error to be reported or ignored
			break;
	}
	if(retval != E_OK)
	{
		//report error or abort
	}
}

void CanTp_RxIndication (PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{

    CanPCI_Type Can_PCI;
    if(CanTP_State.CanTP_State == CANTP_OFF)
    {
    	//We can report error here, nothing to do
    	return;
    }
    CanTp_GetPCI(PduInfoPtr, &Can_PCI);
    switch(CanTP_State.RxState.CanTp_RxState)
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
    		break;
    }
}
