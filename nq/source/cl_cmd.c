/*
	cl_cmd.c

	script command processing module

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "client.h"
#include "QF/cmd.h"
#include "QF/console.h"
#include "QF/msg.h"
#include "QF/sizebuf.h"

/*
	Cmd_ForwardToServer

	Sends the entire command line over to the server
*/
void
Cmd_ForwardToServer (void)
{
	if (cls.state != ca_connected) {
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}

	if (cls.demoplayback)
		return;							// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);
	if (strcasecmp (Cmd_Argv (0), "cmd") != 0) {
		SZ_Print (&cls.message, Cmd_Argv (0));
		SZ_Print (&cls.message, " ");
	}
	if (Cmd_Argc () > 1)
		SZ_Print (&cls.message, Cmd_Args ());
	else
		SZ_Print (&cls.message, "\n");
}

/*
	cl_Cmd_Init
*/
void
cl_Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer, "No Description");
}
