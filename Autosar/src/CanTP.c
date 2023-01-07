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

CanTp_StateType CanTPInternalState = CANTP_OFF;

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

void CanTp_Shutdown (void)
{
	//TODO Stop all connections
	CanTPInternalState = CANTP_OFF;
}



Std_ReturnType CanTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr )
{

}
