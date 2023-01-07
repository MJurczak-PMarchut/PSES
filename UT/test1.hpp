/*
 * test1.hpp
 *
 *  Created on: 22 paź 2022
 *      Author: Mateusz
 */

#ifndef TEST1_HPP_
#define TEST1_HPP_
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "Std_Types.h"
#include "CanTP.h"

uint8_t test_return1(void)
{
	return 1;
}

uint8_t test_return2(void)
{
	Std_VersionInfoType ver = {0,0,0,0,0,0,4,4,0};
	Std_VersionInfoType pver;
	CanTp_GetVersionInfo(&pver);
	if(	(pver.ar_major_version == ver.ar_major_version) &&
		(pver.ar_minor_version == ver.ar_minor_version) &&
		(pver.ar_patch_version == ver.ar_patch_version) &&
		(pver.instanceID == ver.instanceID)				&&
		(pver.moduleID == ver.moduleID) 				&&
		(pver.sw_major_version == ver.sw_major_version) &&
		(pver.sw_minor_version == ver.sw_minor_version) &&
		(pver.sw_patch_version == ver.sw_patch_version) &&
		(pver.vendorID == ver.vendorID))
		{
			return 1;
		}
	return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* TEST1_HPP_ */
