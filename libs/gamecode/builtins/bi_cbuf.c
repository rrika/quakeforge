/*
	bi_cbuf.c

	CSQC cbuf builtins

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
static const char rcsid[] = 
	"$Id$";

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "QF/cmd.h"
#include "QF/progs.h"

static void
bi_Cbuf_AddText (progs_t *pr)
{
	const char *text = P_STRING (pr, 0);

	Cbuf_AddText (text);
}

static void
bi_Cbuf_InsertText (progs_t *pr)
{
	const char *text = P_STRING (pr, 0);

	Cbuf_InsertText (text);
}

static void
bi_Cbuf_Execute (progs_t *pr)
{
	Cbuf_Execute ();
}

static void
bi_Cbuf_Execute_Sets (progs_t *pr)
{
	Cbuf_Execute_Sets ();
}

void
Cbuf_Progs_Init (progs_t *pr)
{
	PR_AddBuiltin (pr, "Cbuf_AddText", bi_Cbuf_AddText, -1);
	PR_AddBuiltin (pr, "Cbuf_InsertText", bi_Cbuf_InsertText, -1);
	PR_AddBuiltin (pr, "Cbuf_Execute", bi_Cbuf_Execute, -1);
	PR_AddBuiltin (pr, "Cbuf_Execute_Sets", bi_Cbuf_Execute_Sets, -1);
}
