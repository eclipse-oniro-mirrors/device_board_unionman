/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/

#include "swdemux_internal.h"

void
swdmx_cb_list_add (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data)
{
	SWDMX_CbEntry *ent;

	SWDMX_ASSERT(l && cb);

	ent = swdmx_malloc(sizeof(SWDMX_CbEntry));
	SWDMX_ASSERT(ent);

	ent->cb   = cb;
	ent->data = data;

	swdmx_list_append(l, &ent->ln);
}

void
swdmx_cb_list_remove (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data)
{
	SWDMX_CbEntry *ent, *nent;

	SWDMX_LIST_FOR_EACH_SAFE(ent, nent, l, ln) {
		if ((ent->cb == cb) && (ent->data == data)) {
			swdmx_list_remove(&ent->ln);
			swdmx_free(ent);
			break;
		}
	}
}

void
swdmx_cb_list_clear (SWDMX_List *l)
{
	SWDMX_CbEntry *ent, *nent;

	SWDMX_LIST_FOR_EACH_SAFE(ent, nent, l, ln) {
		swdmx_list_remove(&ent->ln);
		swdmx_free(ent);
	}

	swdmx_list_init(l);
}

