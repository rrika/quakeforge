/*
	sv_sys_win.c

	(description)

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
# include "config.h"
#endif

#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <winsock.h>

#include "QF/cvar.h"
#include "QF/qargs.h"
#include "QF/sys.h"

#include "compat.h"
#include "server.h"

qboolean    is_server = true;
qboolean    WinNT;
server_static_t svs;
char       *svs_info = svs.info;


void
Sys_Init (void)
{
	OSVERSIONINFO vinfo;

#ifdef USE_INTEL_ASM
	Sys_SetFPCW ();
#endif
	// make sure the timer is high precision, otherwise NT gets 18ms resolution
	timeBeginPeriod (1);

	vinfo.dwOSVersionInfoSize = sizeof (vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s)) {
		Sys_Error (PROGRAM " requires at least Win95 or NT 4.0");
	}

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else
		WinNT = false;
}


/*
	Sys_ConsoleInput

	Checks for a complete line of text typed in at the console, then forwards
	it to the host command processor
*/
const char *
Sys_ConsoleInput (void)
{
	static char text[256];
	static int  len;
	int         c;

	// read a line out
	while (kbhit ()) {
		c = _getch ();
		putch (c);
		if (c == '\r') {
			text[len] = 0;
			putch ('\n');
			len = 0;
			return text;
		}
		if (c == 8) {
			if (len) {
				putch (' ');
				putch (c);
				len--;
				text[len] = 0;
			}
			continue;
		}
		text[len] = c;
		len++;
		text[len] = 0;
		if (len == sizeof (text))
			len = 0;
	}

	return NULL;
}

char       *newargv[256];

int
main (int argc, const char **argv)
{
	double      newtime, time, oldtime;
	fd_set      fdset;
	int         sleep_msec, t;
	struct timeval timeout;

	COM_InitArgv (argc, argv);

	host_parms.argc = com_argc;
	host_parms.argv = com_argv;

	host_parms.memsize = 8 * 1024 * 1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 && t + 1 < com_argc)
		host_parms.memsize = atoi (com_argv[t + 1]) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 && t + 1 < com_argc)
		host_parms.memsize = atoi (com_argv[t + 1]) * 1024 * 1024;

	host_parms.membase = malloc (host_parms.memsize);

	if (!host_parms.membase)
		Sys_Error ("Insufficient memory.\n");

	SV_Init ();

	if (COM_CheckParm ("-nopriority")) {
		Cvar_Set (sys_sleep, "0");
	} else {
		if (!SetPriorityClass (GetCurrentProcess (), HIGH_PRIORITY_CLASS))
			SV_Printf ("SetPriorityClass() failed\n");
		else
			SV_Printf ("Process priority class set to HIGH\n");
	}

	// sys_sleep > 0 seems to cause packet loss on WinNT (why?)
	if (WinNT)
		Cvar_Set (sys_sleep, "0");

	Sys_RegisterShutdown (Net_LogStop);

	// run one frame immediately for first heartbeat
	SV_Frame (0.1);

	// main loop
	oldtime = Sys_DoubleTime () - 0.1;
	while (1) {
		// Now we want to give some processing time to other applications,
		// such as qw_client, running on this machine.
		sleep_msec = sys_sleep->int_val;
		if (sleep_msec > 0) {
			if (sleep_msec > 13)
				sleep_msec = 13;
			Sleep (sleep_msec);
		}
		// select on the net socket and stdin
		// the only reason we have a timeout at all is so that if the last
		// connected client times out, the message would not otherwise
		// be printed until the next event.
		FD_ZERO (&fdset);
		FD_SET (net_socket, &fdset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
		if (select (net_socket + 1, &fdset, NULL, NULL, &timeout) == -1)
			continue;

		// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;

		SV_Frame (time);
	}

	return true;
}
