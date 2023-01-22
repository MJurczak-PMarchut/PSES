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
    uint16 CanTp_ReceivedBytes;
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


static CanTP_InternalStateType CanTP_State;

static void* CanTP_MemSet(void* destination, int value, uint64 num)
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
 * Handle Single frame reception
 */
static Std_ReturnType CanTp_ConsecutiveFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
    PduLengthType buffer_size;      // use when calling PduR callbacks
    BufReq_ReturnType Buf_Status;   // use when calling PduR callbacks
    PduInfoType Extracted_Data;
    uint16 current_block_size;
    Std_ReturnType retval = E_NOT_OK;

    //@Justyna
    return retval;
}

/*
 * TODO Add brief
 * Handle Single frame reception
 */
static Std_ReturnType CanTp_FlowFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	Std_ReturnType retval = E_NOT_OK;
	//We received FC so we are transmitting
	//Did we get correct frame?
	if((CanTP_State.TxState.CanTp_TxState == CANTP_TX_PROCESSING) || (CanTP_State.TxState.CanTp_TxState == CANTP_TX_SUSPENDED))
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
		if(CanTP_State.TxState.CanTp_CurrentTxId == RxPduId){
			switch(Can_PCI->FS){
				case FS_CTS:
					CanTP_State.TxState.CanTp_BlocksToFCFrame = Can_PCI->BS;
					retval = E_OK;
					//Todo Send consecutive frame
					break;
				case FS_WAIT:
					//TODO restart timer N_Bs
					//@Justyna
					retval = E_OK;
					break;
				case FS_OVFLW:
					/*[SWS_CanTp_00309] ⌈If a FC frame is received with the FS set to OVFLW the CanTp module shall
					 * abort the transmit request and notify the upper layer by calling the callback
					 * function PduR_CanTpTxConfirmation() with the result E_NOT_OK. ⌋
					 */
					PduR_CanTpTxConfirmation(CanTP_State.TxState.CanTp_CurrentTxId, E_NOT_OK);
					CanTP_MemSet(&CanTP_State.TxState, 0, sizeof(CanTP_State.TxState));
					CanTP_State.TxState.CanTp_TxState = CANTP_TX_WAIT;
					retval = E_OK;
					break;
				default:
					PduR_CanTpTxConfirmation(CanTP_State.TxState.CanTp_CurrentTxId, E_NOT_OK);
					CanTP_MemSet(&CanTP_State.TxState, 0, sizeof(CanTP_State.TxState));
					CanTP_State.TxState.CanTp_TxState = CANTP_TX_WAIT;
					retval = E_OK;
					break;
			}
		}
	}
	return retval;
}
/*
 * TODO Add brief
 * Handle Single frame reception
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
 * TODO Add brief
 * Handle first frame reception
 * TODO Handle FirstFrame reception
 */
static Std_ReturnType CanTp_FirstFrameReceived(PduIdType RxPduId, const PduInfoType *PduInfoPtr, CanPCI_Type *Can_PCI)
{
	BufReq_ReturnType Buf_Status;
	PduLengthType buffer_size;
	Std_ReturnType retval = E_NOT_OK;
	PduInfoType ULPduInfo;
	/*
	 * [SWS_CanTp_00166] ⌈At the reception of a FF or last CF of a block, the CanTp
	 * module shall start a time-out N_Br before calling PduR_CanTpStartOfReception or PduR_CanTpCopyRxData. ⌋ ( )
	 */

	//TODO @Justyna start a time-out N_Br

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
		//TODO Send FC
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_E_NOT_OK){
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_E_BUSY){
		retval = E_NOT_OK;
	}
	else if(Buf_Status == BUFREQ_OK){
        CanTP_State.RxState.CanTp_MessageLength = Can_PCI->FrameLength;
        CanTP_State.RxState.CanTp_CurrentRxId = RxPduId;
        /*
         * [SWS_CanTp_00339] ⌈After the reception of a First Frame or Single Frame, if the
         * function PduR_CanTpStartOfReception() returns BUFREQ_OK with a smaller
         * available buffer size than needed for the already received data, the CanTp module
         * shall abort the reception of the N-SDU and call PduR_CanTpRxIndication() with
         * the result E_NOT_OK. ⌋ ( )
         */

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
			PduR_CanTpRxIndication(CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			retval = E_NOT_OK;
		}
		else{
			//[SWS_CanTp_00082] for the next block we need SduLength + 7
			if(buffer_size < PduInfoPtr->SduLength + 7)
			//if(buffer_size < 14)
			{
				/*  TODO Send FC(WAIT) to wait for buffer
				 *  @Justyna start N_Br
				 */
			}
			else{
				/*
				 * TODO Send FC(CTS) to wait for buffer
				 */
			}

			retval = E_OK;
		}
        if(retval == E_OK)
        {
        	//TODO I assume we need to copy the data to PduR (basis: SWS_CanTp_00339)
        	if(Can_PCI->FrameLength < 4096){
				PduR_CanTpCopyRxData(RxPduId, &ULPduInfo, &buffer_size);
        	}
        	else{
        		//Nothing to copy
        	}
        	CanTP_State.RxState.CanTp_RxState = CANTP_RX_PROCESSING;
        	CanTP_State.RxState.CanTp_CurrentRxId = RxPduId;
        	CanTP_State.RxState.CanTp_NoOfBlocksTillCTS = (buffer_size/7);
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

static void CanTp_RxIndicationHandleWaitState(PduIdType RxPduId, const PduInfoType* PduInfoPtr, CanPCI_Type *Can_PCI)
{
	// In wait state we should only receive start of communication or flow control frame if we are waiting for it
	// disregard CF
	Std_ReturnType retval = E_NOT_OK;
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
		CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
		CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
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
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication(CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_SingleFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case FlowControlFrame:
			retval = CanTp_FlowFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
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
		CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
		CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
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
			retval = CanTp_FirstFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case SingleFrame:
			PduR_CanTpRxIndication ( CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
			CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
			CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
			retval = CanTp_SingleFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case FlowControlFrame:
			//We might receive FC frame
			retval = CanTp_FlowFrameReceived(RxPduId, PduInfoPtr, Can_PCI);
			break;
		case ConsecutiveFrame:
			//We should not receive CF at this time as we do not have enough buffer space
			//We reset communication and report an error
			PduR_CanTpRxIndication (CanTP_State.RxState.CanTp_CurrentRxId, E_NOT_OK);
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
		CanTP_MemSet(&CanTP_State.RxState, 0, sizeof(CanTP_State.RxState));
		CanTP_State.RxState.CanTp_RxState = CANTP_RX_WAIT;
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
