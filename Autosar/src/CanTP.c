/*
 * CanTP.c
 *
 *  Created on: 1 sty 2023
 *      Author: Mateusz
 */


#include "CanTP.h"


CanTp_StateType CanTPInternalState = CANTP_OFF;

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
