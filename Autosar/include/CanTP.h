/*
 * CanTP.h
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */

#ifndef INCLUDE_CANTP_H_
#define INCLUDE_CANTP_H_

#include "CanTP_Types.h"
#define _VENDOR_ID 0
#define _MODULE_ID 0
#define _INSTANCE_ID 0

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
