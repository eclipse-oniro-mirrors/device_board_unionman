/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

/*************************************************************
 * include files
 ************************************************************/

#include "mp_precomp.h"
#include "phydm_precomp.h"

void phydm_init_soft_ml_setting(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 soml_mask = BIT(31) | BIT(30) | BIT(29) | BIT(28);

#if (RTL8822B_SUPPORT == 1)
	if (!*dm->mp_mode) {
		if (dm->support_ic_type & ODM_RTL8822B) {
#if 0
			/*odm_set_bb_reg(dm, R_0x19a8, MASKDWORD, 0xd10a0000);*/
#endif
			phydm_somlrxhp_setting(dm, true);
			dm->bsomlenabled = true;
		}
	}
#endif
#if (RTL8821C_SUPPORT == 1)
	if (!*dm->mp_mode) {
		if (dm->support_ic_type & ODM_RTL8821C)
			odm_set_bb_reg(dm, R_0x19a8, soml_mask, 0xd);
	}
#endif
#if (RTL8195B_SUPPORT == 1)
	if (!*dm->mp_mode) {
		if (dm->support_ic_type & ODM_RTL8195B)
			odm_set_bb_reg(dm, R_0x19a8, soml_mask, 0xd);
	}
#endif
}
