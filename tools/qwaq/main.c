/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2001 #AUTHOR#

	Author: #AUTHOR#
	Date: #DATE#

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

static __attribute__ ((unused)) const char rcsid[] = 
	"$Id$";

#include <stdlib.h>

#include <QF/cmd.h>
#include <QF/cvar.h>
#include <QF/progs.h>
#include <QF/quakefs.h>
#include <QF/sys.h>
#include "QF/va.h"
#include <QF/zone.h>

#include "qwaq.h"

#define MAX_EDICTS 1024

static edict_t *edicts;
static int num_edicts;
static int reserved_edicts;
static progs_t pr;
static void *membase;
static int memsize = 16*1024*1024;

static QFile *
open_file (const char *path, int *len)
{
	QFile      *file = Qopen (path, "rbz");

	if (!file) {
		perror (path);
		return 0;
	}
	*len = Qfilesize (file);
	return file;
}

static void *
load_file (progs_t *pr, const char *name)
{
	QFile      *file;
	int         size;
	void       *sym;

	file = open_file (name, &size);
	if (!file) {
		file = open_file (va ("%s.gz", name), &size);
		if (!file) {
			return 0;
		}
	}
	sym = malloc (size);
	Qread (file, sym, size);
	return sym;
}

static void *
allocate_progs_mem (progs_t *pr, int size)
{
	return malloc (size);
}

static void
free_progs_mem (progs_t *pr, void *mem)
{
	free (mem);
}

static void
init_qf (void)
{
	Cvar_Init_Hash ();
	Cmd_Init_Hash ();
	Cvar_Init ();
	Sys_Init_Cvars ();
	Cmd_Init ();

	membase = malloc (memsize);
	Memory_Init (membase, memsize);

	Cvar_Get ("pr_debug", "1", 0, 0, 0);
	Cvar_Get ("pr_boundscheck", "0", 0, 0, 0);

	pr.edicts = &edicts;
	pr.num_edicts = &num_edicts;
	pr.reserved_edicts = &reserved_edicts;
	pr.load_file = load_file;
	pr.allocate_progs_mem = allocate_progs_mem;
	pr.free_progs_mem = free_progs_mem;

	PR_Init_Cvars ();
	PR_Init ();
	PR_Obj_Progs_Init (&pr);
	BI_Init (&pr);
}

static int
load_progs (const char *name)
{
	QFile      *file;
	int         size;

	file = open_file (name, &size);
	if (!file) {
		return 0;
	}
	pr.progs_name = name;
	PR_LoadProgsFile (&pr, file, size, 1, 1024 * 1024);
	Qclose (file);
	if (!PR_ResolveGlobals (&pr))
		PR_Error (&pr, "unable to load %s", pr.progs_name);
	PR_LoadStrings (&pr);
	PR_LoadDebug (&pr);
	PR_Check_Opcodes (&pr);
	PR_RelocateBuiltins (&pr);
	PR_InitRuntime (&pr);
	return 1;
}

int
main (int argc, char **argv)
{
	func_t main_func;
	const char *name = "qwaq.dat";
	string_t   *pr_argv;
	int         pr_argc = 1, i;

	init_qf ();

	if (argc > 1)
		name = argv[1];

	if (!load_progs (name))
		Sys_Error ("couldn't load %s", "qwaq.dat");

	if (argc > 2)
		pr_argc = argc - 1;
	pr_argv = PR_Zone_Malloc (&pr, (pr_argc + 1) * 4);
	pr_argv[0] = PR_SetString (&pr, name);
	for (i = 1; i < pr_argc; i++)
		pr_argv[i] = PR_SetString (&pr, argv[1 + i]);
	pr_argv[i] = 0;

	main_func = PR_GetFunctionIndex (&pr, "main");
	P_INT (&pr, 0) = pr_argc;
	P_INT (&pr, 1) = POINTER_TO_PROG (&pr, pr_argv);
	PR_ExecuteProgram (&pr, main_func);
	return R_INT (&pr);
}
