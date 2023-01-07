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
#include "mandatory_interfaces_mock.h"
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

<<<<<<< HEAD
/*
 * @brief
 * Service ID: 0x40
 * This function allows lower layer communication interface module to confirm the transmission of a PDU, or the failure to transmit a PDU
 */
void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
=======
>>>>>>> cf17f5d01222eb30a530c8dd1d1ecf6bcfa650c7

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

#endif /* INCLUDE_CANTP_H_ */
