/*
	varrays.h

	OpenGL-specific definitions and prototypes

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

	$Id$
*/

#ifndef __qf_varrays_h
#define __qf_varrays_h

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif

#include "QF/GL/types.h"

typedef struct varray_t2f_c4f_v3f_s {
	 GLfloat	texcoord[2];
	 GLfloat	color[4];
	 GLfloat	vertex[3];
} varray_t2f_c4f_v3f_t;

typedef struct varray_t2f_c4ub_v3f_s {
	 GLfloat	texcoord[2];
	 GLubyte	color[4];
	 GLfloat	vertex[3];
} varray_t2f_c4ub_v3f_t;

typedef struct varray_t2f_c4f_n3f_v3f_s {
	 GLfloat	texcoord[2];
	 GLfloat	color[4];
	 GLfloat	normal[3];
	 GLfloat	vertex[3];
} varray_t2f_c4f_n3f_v3f_t;

#define MAX_VARRAY_VERTS	10000
extern varray_t2f_c4ub_v3f_t varray[MAX_VARRAY_VERTS];

#endif // __qf_varrays_h
