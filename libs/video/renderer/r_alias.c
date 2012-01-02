/*
	r_alias.c

	Draw Alias Model

	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "QF/sys.h"

#include "r_local.h"

static __attribute__ ((used)) const char rcsid[] = "$Id$";

VISIBLE maliasskindesc_t *
R_AliasGetSkindesc (int skinnum, aliashdr_t *ahdr)
{
	maliasskindesc_t *pskindesc;
	maliasskingroup_t *paliasskingroup;

	if ((skinnum >= ahdr->mdl.numskins) || (skinnum < 0)) {
		Sys_MaskPrintf (SYS_DEV, "R_AliasSetupSkin: no such skin # %d\n",
						skinnum);
		skinnum = 0;
	}

	pskindesc = ((maliasskindesc_t *)
				 ((byte *) ahdr + ahdr->skindesc)) + skinnum;

	if (pskindesc->type == ALIAS_SKIN_GROUP) {
		int         numskins, i;
		float       fullskininterval, skintargettime, skintime;
		float      *pskinintervals;

		paliasskingroup = (maliasskingroup_t *) ((byte *) ahdr +
												 pskindesc->skin);
		pskinintervals = (float *)
			((byte *) ahdr + paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins - 1];

		skintime = r_realtime + currententity->syncbase;

		// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
		// values are positive, so we don't have to worry about division by 0
		skintargettime = skintime -
			((int) (skintime / fullskininterval)) * fullskininterval;

		for (i = 0; i < (numskins - 1); i++) {
			if (pskinintervals[i] > skintargettime)
				break;
		}
		pskindesc = &paliasskingroup->skindescs[i];
	}

	return pskindesc;
}

VISIBLE maliasframedesc_t *
R_AliasGetFramedesc (int framenum, aliashdr_t *hdr)
{
	float      *intervals;
	float       fullinterval, time, targettime;
	maliasframedesc_t *frame;
	maliasgroup_t *group;
	int         numframes;
	int         i;

	if ((framenum >= hdr->mdl.numframes) || (framenum < 0)) {
		Sys_MaskPrintf (SYS_DEV, "R_AliasSetupFrame: no such frame %d\n",
						framenum);
		framenum = 0;
	}

	frame = &hdr->frames[framenum];
	if (frame->type == ALIAS_SINGLE)
		return frame;

	group = (maliasgroup_t *) ((byte *) hdr + frame->frame);
	intervals = (float *) ((byte *) hdr + group->intervals);
	numframes = group->numframes;
	fullinterval = intervals[numframes - 1];

	time = r_realtime + currententity->syncbase;

	// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
	targettime = time - ((int) (time / fullinterval)) * fullinterval;

	for (i = 0; i < (numframes - 1); i++) {
		if (intervals[i] > targettime)
			break;
	}
	return &group->frames[i];
}
