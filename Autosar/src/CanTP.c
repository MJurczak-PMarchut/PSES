/*
 * CanTP.c
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */


#include "CanTP.h"

#define CANTP_CAN_FRAME_SIZE (0x08u)

#ifndef CANTP_MAX_NUM_OF_N_SDU
#define CANTP_MAX_NUM_OF_N_SDU (0x10u)
#endif

#define NULL_PTR NULL

CanTp_StateType CanTPInternalState = CANTP_OFF;
static CanTp_TxStateType CanTpTxState = CANTP_TX_WAIT;

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

typedef enum
{
    CANTP_WAIT = 0x00u,
    CANTP_PROCESSING
} CanTp_TaskStateType;

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
        CanTp_TaskStateType taskState;
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
    CanTp_TaskStateType taskState;
    struct
    {
        CanTp_FrameStateType state;
        uint32 flag;
    } shared;
} CanTp_TxConnectionType;

typedef struct
{
    CanTp_RxConnectionType rx;
    CanTp_TxConnectionType tx;
    uint32 n[0x06u];
    uint8_least dir;
    uint32 t_flag;
} CanTp_NSduType;

typedef struct
{
    CanTp_NSduType sdu[CANTP_MAX_NUM_OF_N_SDU];
} CanTp_ChannelRtType;

static CanTp_ChannelRtType CanTp_Rt[CANTP_MAX_NUM_OF_CHANNEL];

void CanTp_Init (const CanTp_ConfigType* CfgPtr)
{
	//TODO Init other variables
	CanTPInternalState = CANTP_ON;
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
		versioninfo->ar_minor_version = CANTP_SW_MINOR_VERSION;
		versioninfo->ar_patch_version = CANTP_SW_PATCH_VERSION;
	}
}

Std_ReturnType CanTp_CancelTransmit (PduIdType TxPduId)
{
	Std_ReturnType ret = E_NOT_OK;
	if(CanTPInternalState == CANTP_ON)
	{
		ret = E_OK;
//		PduR_CanTpTxConfirmation(p_n_sdu->tx.cfg->nSduId, E_NOT_OK);
		CanTpTxState = CANTP_TX_WAIT;
	}
	return ret;
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



Std_ReturnType CanTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr )
{
    CanTp_NSduType *p_n_sdu = NULL_PTR;
    Std_ReturnType tmp_return = E_NOT_OK;

    //If we
    if ((CanTp_StateType)CanTPInternalState == (CanTp_StateType)CANTP_ON)
    {
        if (PduInfoPtr != NULL_PTR)
        {
            if (CanTp_GetNSduFromPduId(TxPduId, &p_n_sdu) == E_OK)
            {
                if (PduInfoPtr->MetaDataPtr != NULL_PTR)
                {
                    p_n_sdu->tx.has_meta_data = TRUE;

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
                    if (p_n_sdu->tx.cfg->af == CANTP_EXTENDED)
                    {
                        p_n_sdu->tx.saved_n_ta.nTa = PduInfoPtr->MetaDataPtr[0x00u];
                    }
                    else if (p_n_sdu->tx.cfg->af == CANTP_MIXED)
                    {
                        p_n_sdu->tx.saved_n_ae.nAe = PduInfoPtr->MetaDataPtr[0x00u];
                    }
                    else if (p_n_sdu->tx.cfg->af == CANTP_NORMALFIXED)
                    {
                        p_n_sdu->tx.saved_n_sa.nSa = PduInfoPtr->MetaDataPtr[0x00u];
                        p_n_sdu->tx.saved_n_ta.nTa = PduInfoPtr->MetaDataPtr[0x01u];
                    }
                    else if (p_n_sdu->tx.cfg->af == CANTP_MIXED29BIT)
                    {
                        p_n_sdu->tx.saved_n_sa.nSa = PduInfoPtr->MetaDataPtr[0x00u];
                        p_n_sdu->tx.saved_n_ta.nTa = PduInfoPtr->MetaDataPtr[0x01u];
                        p_n_sdu->tx.saved_n_ae.nAe = PduInfoPtr->MetaDataPtr[0x02u];
                    }
                    else
                    {
                        /* MISRA C, do nothing. */
                    }
                }
                else
                {
                    p_n_sdu->tx.has_meta_data = FALSE;
                }

                /* SWS_CanTp_00206: the function CanTp_Transmit shall reject a request if the
                 * CanTp_Transmit service is called for a N-SDU identifier which is being used in a
                 * currently running CAN Transport Layer session. */
                if ((p_n_sdu->tx.taskState != CANTP_PROCESSING) &&
                    (PduInfoPtr->SduLength > 0x0000u) && (PduInfoPtr->SduLength <= 0x0FFFu))
                {
                    p_n_sdu->tx.buf.size = PduInfoPtr->SduLength;

                    if ((((p_n_sdu->tx.cfg->af == CANTP_STANDARD) || (p_n_sdu->tx.cfg->af == CANTP_NORMALFIXED)) && (PduInfoPtr->SduLength <= 0x07u)) ||
                        (((p_n_sdu->tx.cfg->af == CANTP_EXTENDED) || (p_n_sdu->tx.cfg->af == CANTP_MIXED) || (p_n_sdu->tx.cfg->af == CANTP_MIXED29BIT)) && (PduInfoPtr->SduLength <= 0x06u)))
                    {
                        p_n_sdu->tx.shared.state = CANTP_TX_FRAME_STATE_SF_TX_REQUEST;
                        tmp_return = E_OK;
                    }
                    else
                    {
                        if (p_n_sdu->tx.cfg->taType == CANTP_PHYSICAL)
                        {
                            p_n_sdu->tx.shared.state = CANTP_TX_FRAME_STATE_FF_TX_REQUEST;
                            tmp_return = E_OK;
                        }
                        else
                        {
                            /* SWS_CanTp_00093:(on both receiver and sender side) with a handle whose communication type is functional,
                             * the CanTp module shall reject the request and report the runtime error code CANTP_E_INVALID_TATYPE to the Default Error Tracer.*/
                            //TODO
//                        	CanTp_ReportRuntimeError(0x00u,
//                                                     CANTP_TRANSMIT_API_ID,
//                                                     CANTP_E_INVALID_TATYPE);
                        }
                    }

                    if (tmp_return == E_OK)
                    {
                        p_n_sdu->tx.taskState = CANTP_PROCESSING;
                    }
                }
            }
            else
            {
            	//TODO
//                CanTp_ReportRuntimeError(0x00u, CANTP_TRANSMIT_API_ID, CANTP_E_INVALID_TX_ID);
            }
        }
        else
        {
        	//TODO
//        	CanTp_ReportRuntimeError(0x00u, CANTP_TRANSMIT_API_ID, CANTP_E_PARAM_POINTER);
        }
    }
    else
    {
    	//TODO
//    	CanTp_ReportRuntimeError(0x00u, CANTP_TRANSMIT_API_ID, CANTP_E_UNINIT);
    }

    return tmp_return;
}
