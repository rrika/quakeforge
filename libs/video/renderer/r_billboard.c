/*
	r_billboard.c

	Billboard frame setup

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

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <string.h>
#endif

#include <math.h>

#include "QF/render.h"
#include "QF/sys.h"

#include "QF/scene/entity.h"

#include "r_internal.h"


int
R_BillboardFrame (entity_t *ent, int orientation, const vec3_t cameravec,
				  vec3_t bbup, vec3_t bbright, vec3_t bbfwd)
{
	vec3_t      tvec;
	float       dot;

	switch (orientation) {
		case SPR_FACING_UPRIGHT:
			// the billboard has its up vector parallel with world up, and
			// its right vector perpendicular to cameravec.
			// Undefined if the camera is too close to the entity.
			VectorNegate (cameravec, tvec);
			VectorNormalize (tvec);
			dot = tvec[2];		// DotProduct (tvec, world up)
			if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree)
				return 0;
			VectorSet (0, 0, 1, bbup);
			//CrossProduct(bbup, -cameravec, bbright)
			VectorSet (tvec[1], -tvec[0], 0, bbright);
			VectorNormalize (bbright);
			//CrossProduct (bbright, bbup, bbfwd)
			VectorSet (-bbright[1], bbright[0], 0, bbfwd);
			break;
		case SPR_VP_PARALLEL:
			// the billboard always has the same orientation as the camera
			VectorCopy (r_refdef.frame.up, bbup);
			VectorCopy (r_refdef.frame.right, bbright);
			VectorCopy (r_refdef.frame.forward, bbfwd);
			break;
		case SPR_VP_PARALLEL_UPRIGHT:
			// the billboar has its up vector parallel with world up, and
			// its right vector parallel with the view plane.
			// Undefined if the camera is looking straight up or down.
			dot = r_refdef.frame.forward[2];
			if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree)
				return 0;
			VectorSet (0, 0, 1, bbup);
			VectorSet (r_refdef.frame.forward[1], -r_refdef.frame.forward[0], 0, bbright);
			VectorNormalize (bbright);
			VectorSet (-bbright[1], bbright[0], 0, bbfwd);
			break;
		case SPR_ORIENTED:
			{
				// The billboard's orientation is fully specified by the
				// entity's orientation.
				mat4f_t     mat;
				Transform_GetWorldMatrix (ent->transform, mat);
				VectorCopy (mat[0], bbfwd);
				VectorNegate (mat[1], bbright);
				VectorCopy (mat[2], bbup);
			}
			break;
		case SPR_VP_PARALLEL_ORIENTED:
			{
				// The billboard is rotated relative to the camera using
				// the entity's local rotation.
				mat4f_t     entmat;
				mat4f_t     spmat;
				Transform_GetLocalMatrix (ent->transform, entmat);
				// FIXME needs proper testing (need to find, make, or fake a
				// parallel oriented sprite)
				mmulf (spmat, r_refdef.camera, entmat);
				VectorCopy (spmat[0], bbfwd);
				VectorNegate (spmat[1], bbright);
				VectorCopy (spmat[1], bbup);
			}
			break;
		default:
			Sys_Error ("R_BillboardFrame: Bad orientation %d", orientation);
	}
	return 1;
}
