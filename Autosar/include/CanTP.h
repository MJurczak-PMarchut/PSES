/*
 * CanTP.h
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */

#ifndef INCLUDE_CANTP_H_
#define INCLUDE_CANTP_H_

#include "CanTP_Types.h"
#include "ComStack_Types.h"

#define _VENDOR_ID 0
#define _MODULE_ID 0
#define _INSTANCE_ID 0


#define CANTP_E_INVALID_TATYPE (0x90u)
#define CANTP_TRANSMIT_API_ID (0x49u)
#define CANTP_E_INVALID_TX_ID (0x30u)
#define CANTP_E_PARAM_POINTER (0x03u)
#define CANTP_E_UNINIT (0x20u)

#define CANTP_MAX_NUM_OF_CHANNEL (0x10u)

#define CANTP_SW_MAJOR_VERSION    	0
#define CANTP_SW_MINOR_VERSION 		0
#define CANTP_SW_PATCH_VERSION    	0
#define CANTP_AR_MAJOR_VERSION    	4
#define CANTP_AR_MINOR_VERSION    	4
#define CANTP_AR_PATCH_VERSION    	0

#define CANTP_DIR_RX 0
#define CANTP_DIR_TX 1


/*
 * @brief
 * Service ID: 0x01
 * This function initializes the CanTp module
 */
void CanTp_Init (const CanTp_ConfigType* CfgPtr);

/*
 * @brief
 * Service ID: 0x07
 * This function returns the version information of the CanTp module
 */
void CanTp_GetVersionInfo ( Std_VersionInfoType* versioninfo);

/*
 * @brief
 * Service ID: 0x4a
 * This function requests cancellation of an ongoing transmission of a PDU in a lower layer communication module
 */
Std_ReturnType CanTp_CancelTransmit (PduIdType TxPduId);


/*
 * @brief
 * Service ID: 0x40
 * This function allows lower layer communication interface module to confirm the transmission of a PDU, or the failure to transmit a PDU
 */
void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);


/*
 * @brief
 * Service ID: 0x02
 * This function is called to shutdown the CanTp module.
 */
void CanTp_Shutdown (void);


/*
 * @brief
 * Service ID:0x49
 * Requests transmission of a PDU.
 *
 */
Std_ReturnType CanTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr );


/*
 * @brief
 * Service ID:0x0b
 * This service is used to read the current value of reception parameters BS and STmin for a specified N-SDU.
 *
 */
Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value);


/*
 * TODO add brief
 */
void CanTp_RxIndication ( PduIdType RxPduId, const PduInfoType* PduInfoPtr);

#endif /* INCLUDE_CANTP_H_ */
