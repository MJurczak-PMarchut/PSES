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
} CanTp_RxState_Type;


typedef enum {
    CANTP_TX_WAIT = 0x00u,
    CANTP_TX_PROCESSING,
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
// typedef struct
// {
//     const CanTp_RxNSduType *cfg;
//     CanTp_NSduBufferType buf;
//     uint8 meta_data_lower[0x04u];
//     uint8 meta_data_upper[0x04u];
//     CanTp_NSaType saved_n_sa;
//     CanTp_NTaType saved_n_ta;
//     CanTp_NAeType saved_n_ae;
//     boolean has_meta_data;
//     CanTp_FlowStatusType fs;
//     uint32 st_min;
//     uint8 bs;
//     uint8 sn;
//     uint16 wft_max;
//     PduInfoType can_if_pdu_info;
//     PduInfoType pdu_r_pdu_info;
//     struct
//     {
//         CanTp_TaskStateType taskState;
//         CanTp_FrameStateType state;

//         /**
//          * @brief structure containing all parameters accessible via @ref CanTp_ReadParameter/@ref
//          * CanTp_ChangeParameter.
//          */
//         struct
//         {
//             uint32 st_min;
//             uint8 bs;
//         } m_param;
//     } shared;
// } CanTp_RxConnectionType;

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

// static CanTp_ChannelRtType CanTp_Rt[CANTP_MAX_NUM_OF_CHANNEL];

static Std_ReturnType CanTp_GetNSduFromPduId(PduIdType pduId, CanTp_NSduType **pNSdu);

typedef struct{
	CanTp_RxStateVariablesType 	RxState;
	CanTp_TxStateVariablesType	TxState;
	CanTp_StateType				CanTP_State;
} CanTP_InternalStateType;


static CanTP_InternalStateType CanTP_State;

void CanTp_Init (const CanTp_ConfigType* CfgPtr)
{
	//Init all to 0
	memcpy(&CanTP_State, 0, sizeof(CanTP_State));
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
	CanTPInternalState = CANTP_OFF;
}



static Std_ReturnType CanTp_GetNSduFromPduId(PduIdType pduId, CanTp_NSduType **pNSdu)
{
    Std_ReturnType tmp_return = E_NOT_OK;
    CanTp_NSduType *p_n_sdu;
    CanTp_ChannelRtType *p_channel_rt;
    uint32_least channel_idx;

    for (channel_idx = 0x00u; channel_idx < (uint32_least)CANTP_MAX_NUM_OF_CHANNEL; channel_idx++)
    {
        p_channel_rt = &CanTp_Rt[channel_idx];

        if (pduId < CANTP_MAX_NUM_OF_N_SDU)
        {
            p_n_sdu = &p_channel_rt->sdu[pduId];

            if (((p_n_sdu->rx.cfg != NULL_PTR) && (p_n_sdu->rx.cfg->nSduId == pduId)) ||
                ((p_n_sdu->tx.cfg != NULL_PTR) && (p_n_sdu->tx.cfg->nSduId == pduId)))
            {
                *pNSdu = p_n_sdu;
                tmp_return = E_OK;

                break;
            }
        }
    }

    return tmp_return;
}

Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value)
{
	CanTp_NSduType *nsdu;
	uint16 val = 0;
	Std_ReturnType ret = E_NOT_OK;

	if((CanTPInternalState == CANTP_ON) &&
		(CanTp_GetNSduFromPduId(id, &nsdu) == E_OK))
	{
		switch(parameter)
		{
			case TP_STMIN:
				if(nsdu->dir == CANTP_DIR_RX)
				{
					val = nsdu->rx.shared.m_param.st_min;
					ret = E_OK;
				}
				else if(nsdu->dir == CANTP_DIR_TX)
				{
					val = nsdu->tx.st_min;
					ret = E_OK;
				}
				break;
			case TP_BS:
				if(nsdu->dir == CANTP_DIR_RX)
				{
					val = nsdu->rx.shared.m_param.bs;
					ret = E_OK;
				}
				else if(nsdu->dir == CANTP_DIR_TX)
				{
					val = nsdu->tx.bs;
					ret = E_OK;
				}
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

	if((CanTPInternalState == CANTP_ON) &&
		(CanTp_GetNSduFromPduId(RxPduId, &nsdu) == E_OK))
	{
		PduR_CanTpRxConfirmation(RxPduId, E_NOT_OK);
	}

	return ret;
}

Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
	CanTp_RxNSduType *nsdu;
	Std_ReturnType ret = E_NOT_OK;

	if((CanTPInternalState == CANTP_ON) &&
		(CanTp_GetNSduFromPduId(id, &nsdu) == E_OK) &&
		(nsdu->rx.shared.taskState == CANTP_RX_WAIT))
	{
		switch(parameter)
		{
			case TP_STMIN:
				nsdu->STMin= value;
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
    CanTp_NSduType *p_n_sdu = NULL_PTR;
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
		if ((p_n_sdu->tx.taskState != CANTP_TX_PROCESSING) &&
			(PduInfoPtr->SduLength > 0x0000u) && (PduInfoPtr->SduLength <= 0x0FFFu))
		{
		}
	}


    return tmp_return;
}
