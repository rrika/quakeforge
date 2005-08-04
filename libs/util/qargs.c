/*
	qargs.c

	command line argument processing routines

	Copyright (C) 1996-1997  Id Software, Inc.
	Copyright (C) 1999,2000  contributors of the QuakeForge project
	Please see the file "AUTHORS" for a list of contributors

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

static __attribute__ ((used)) const char rcsid[] = 
	"$Id$";

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "QF/cmd.h"
#include "QF/crc.h"
#include "QF/qargs.h"
#include "QF/qtypes.h"
#include "QF/sys.h"

static const char **largv;
static const char *argvdummy = " ";

static const char *safeargvs[] =
	{ "-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse",
		"-dibonly" };

#define NUM_SAFE_ARGVS ((int) (sizeof(safeargvs)/sizeof(safeargvs[0])))

int         com_argc;
const char **com_argv;
const char *com_cmdline;

qboolean    nouse = false;				// 1999-10-29 +USE fix by Maddes


/*
	COM_CheckParm

	Returns the position (1 to argc-1) in the program's argument list
	where the given parameter apears, or 0 if not present
*/
int
COM_CheckParm (const char *parm)
{
	int         i;

	for (i = 1; i < com_argc; i++) {
		if (!com_argv[i])
			continue;					// NEXTSTEP sometimes clears appkit
										// vars.
		if (!strcmp (parm, com_argv[i]))
			return i;
	}

	return 0;
}

void
COM_InitArgv (int argc, const char **argv)
{
	qboolean    safe;
	int         i, len;
	char       *cmdline;

	safe = false;

	largv = (const char **) calloc (1, (argc + NUM_SAFE_ARGVS + 1) *
									sizeof (const char **));

	for (com_argc = 0, len = 0; com_argc < argc; com_argc++) {
		largv[com_argc] = argv[com_argc];
		if ((argv[com_argc]) && !strcmp ("-safe", argv[com_argc]))
			safe = true;
		if (com_argc)
			len += strlen (argv[com_argc]) + 1;
	}

	cmdline = (char *) calloc (1, len + 1);	// need strlen(cmdline)+2
	cmdline[0] = 0;
	if (len) {
		for (i = 1; i < argc; i++) {
			strncat (cmdline, argv[i], len - strlen (cmdline));
			assert (len - strlen (cmdline) > 0);
			strncat (cmdline, " ", len - strlen (cmdline));
		}
		cmdline[len - 1] = '\0';
	}
	com_cmdline = cmdline;

	if (safe) {
		// force all the safe-mode switches. Note that we reserved extra space
		// in case we need to add these, so we don't need an overflow check
		for (i = 0; i < NUM_SAFE_ARGVS; i++) {
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

	if (COM_CheckParm ("-nouse")) {
		nouse = true;
	}
}

/*
	COM_AddParm

	Adds the given string at the end of the current argument list
*/
void
COM_AddParm (const char *parm)
{
	largv[com_argc++] = parm;
}
