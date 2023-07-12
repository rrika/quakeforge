/*
	idstuff.c

	qcc compatable output stuff

	Copyright (C) 2002 Bill Currie <bill@taniwha.org>
	Copyright (C) 1996-1997  Id Software, Inc.

	Author: Bill Currie <bill@taniwha.org>
	Date: 2002/06/04

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
# include <strings.h>
#endif

#include <QF/crc.h>
#include <QF/dstring.h>
#include <QF/quakeio.h>

#include "tools/qfcc/include/def.h"
#include "tools/qfcc/include/defspace.h"
#include "tools/qfcc/include/diagnostic.h"
#include "tools/qfcc/include/qfcc.h"
#include "tools/qfcc/include/expr.h"
#include "tools/qfcc/include/idstuff.h"
#include "tools/qfcc/include/options.h"
#include "tools/qfcc/include/strpool.h"
#include "tools/qfcc/include/symtab.h"
#include "tools/qfcc/include/type.h"

#define	MAX_SOUNDS		1024
#define	MAX_MODELS		1024
#define	MAX_FILES		1024
#define	MAX_DATA_PATH	64

static char precache_sounds[MAX_SOUNDS][MAX_DATA_PATH];
static int  precache_sounds_block[MAX_SOUNDS];
static int  numsounds;

static char precache_models[MAX_MODELS][MAX_DATA_PATH];
static int  precache_models_block[MAX_SOUNDS];
static int  nummodels;

static char precache_files[MAX_FILES][MAX_DATA_PATH];
static int  precache_files_block[MAX_SOUNDS];
static int  numfiles;

void
PrecacheSound (const char *n, int ch)
{
	int         i;

	for (i = 0; i < numsounds; i++) {
		if (!strcmp (n, precache_sounds[i])) {
			return;
		}
	}

	if (numsounds == MAX_SOUNDS) {
		error (0, "PrecacheSound: numsounds == MAX_SOUNDS");
		return;
	}

	strcpy (precache_sounds[i], n);
	if (ch >= '1' && ch <= '9')
		precache_sounds_block[i] = ch - '0';
	else
		precache_sounds_block[i] = 1;

	numsounds++;
}

void
PrecacheModel (const char *n, int ch)
{
	int         i;

	for (i = 0; i < nummodels; i++) {
		if (!strcmp (n, precache_models[i])) {
			return;
		}
	}

	if (nummodels == MAX_MODELS) {
		error (0, "PrecacheModels: nummodels == MAX_MODELS");
		return;
	}

	strcpy (precache_models[i], n);
	if (ch >= '1' && ch <= '9')
		precache_models_block[i] = ch - '0';
	else
		precache_models_block[i] = 1;

	nummodels++;
}

void
PrecacheFile (const char *n, int ch)
{
	int         i;

	for (i = 0; i < numfiles; i++) {
		if (!strcmp (n, precache_files[i])) {
			return;
		}
	}

	if (numfiles == MAX_FILES) {
		error (0, "PrecacheFile: numfiles == MAX_FILES");
		return;
	}

	strcpy (precache_files[i], n);
	if (ch >= '1' && ch <= '9')
		precache_files_block[i] = ch - '0';
	else
		precache_files_block[i] = 1;

	numfiles++;
}

/*
	WriteFiles

	Generates files.dat, which contains all of the data files actually used by
	the game, to be processed by qfiles
*/
int
WriteFiles (const char *sourcedir)
{
	FILE       *f;
	int         i;
	dstring_t  *filename = dstring_newstr ();

	dsprintf (filename, "%s%cfiles.dat", sourcedir, PATH_SEPARATOR);
	f = fopen (filename->str, "wb");
	if (!f) {
		fprintf (stderr, "Couldn't open %s", filename->str);
		return 1;
	}

	fprintf (f, "%i\n", numsounds);
	for (i = 0; i < numsounds; i++)
		fprintf (f, "%i %s\n", precache_sounds_block[i], precache_sounds[i]);

	fprintf (f, "%i\n", nummodels);
	for (i = 0; i < nummodels; i++)
		fprintf (f, "%i %s\n", precache_models_block[i], precache_models[i]);

	fprintf (f, "%i\n", numfiles);
	for (i = 0; i < numfiles; i++)
		fprintf (f, "%i %s\n", precache_files_block[i], precache_files[i]);

	fclose (f);
	dstring_delete (filename);
	return 0;
}

/*
	WriteProgdefs

	Writes the global and entity structures out.
	Returns a crc of the header, to be stored in the progs file for comparison
	at load time.
*/
int
WriteProgdefs (dprograms_t *progs, const char *filename)
{
	ddef_t     *def;
	ddef_t     *fdef;
	dstring_t  *dstr;
	QFile      *f;
	unsigned short crc;
	unsigned    i, j;
	const char *strings;
	const char *name;

	if (options.verbosity >= 1)
		printf ("Calculating CRC\n");

	dstr = dstring_newstr();

	// print global vars until the first field is defined
	dasprintf (dstr, "\n/* file generated by qcc, do not modify */"
					 "\n\ntypedef struct\n{\tint\tpad[%i];\n",
			 RESERVED_OFS);

	strings = (char *) progs + progs->strings.offset;
	for (i = 0; i < progs->globaldefs.count; i++) {
		def = (ddef_t *) ((char *) progs + progs->globaldefs.offset) + i;
		name = strings + def->name;
		if (!strcmp (name, "end_sys_globals"))
			break;
		if (!def->ofs)
			continue;
		if (*name == '.' || !*name)
			continue;

		switch (def->type & ~DEF_SAVEGLOBAL) {
			case ev_float:
				dasprintf (dstr, "\tfloat\t%s;\n", name);
				break;
			case ev_vector:
				dasprintf (dstr, "\tvec3_t\t%s;\n", name);
				break;
			case ev_quaternion:
				dasprintf (dstr, "\tquat_t\t%s;\n", name);
				break;
			case ev_string:
				dasprintf (dstr, "\tstring_t\t%s;\n", name);
				break;
			case ev_func:
				dasprintf (dstr, "\tfunc_t\t%s;\n", name);
				break;
			case ev_entity:
				dasprintf (dstr, "\tint\t%s;\n", name);
				break;
			default:
				dasprintf (dstr, "\tint\t%s;\n", name);
				break;
		}
	}
	dasprintf (dstr, "} globalvars_t;\n\n");

	// print all fields
	dasprintf (dstr, "typedef struct\n{\n");
	for (i = 0, j = 0; i < progs->globaldefs.count; i++) {
		def = (ddef_t *) ((char *) progs + progs->globaldefs.offset) + i;
		name = strings + def->name;
		if (!strcmp (name, "end_sys_fields"))
			break;

		if (!def->ofs)
			continue;
		if (def->type != ev_field)
			continue;
		if (!strcmp (name, ".imm"))
			continue;

		fdef = (ddef_t *) ((char *) progs + progs->fielddefs.offset) + j++;
		if (fdef->name != def->name)
			internal_error (0, "def and field order messup");

		switch (fdef->type) {
			case ev_float:
				dasprintf (dstr, "\tfloat\t%s;\n", name);
				break;
			case ev_vector:
				dasprintf (dstr, "\tvec3_t\t%s;\n", name);
				break;
			case ev_string:
				dasprintf (dstr, "\tstring_t\t%s;\n", name);
				break;
			case ev_func:
				dasprintf (dstr, "\tfunc_t\t%s;\n", name);
				break;
			case ev_entity:
				dasprintf (dstr, "\tint\t%s;\n", name);
				break;
			default:
				dasprintf (dstr, "\tint\t%s;\n", name);
				break;
		}
	}
	dasprintf (dstr, "} entvars_t;\n\n");

	// do a crc of the structs
	crc = CRC_Block ((byte *) dstr->str, dstr->size - 1);

	dasprintf (dstr, "#define PROGHEADER_CRC %u\n", crc);
	dstring_insertstr (dstr, 0, "/* Actually, generated by qfcc, but one must "
					   "maintain formalities */");

	if (filename) {
		if (options.verbosity >= 1)
			printf ("writing %s\n", filename);
		f = Qopen (filename, "wt");
		Qwrite (f, dstr->str, dstr->size - 1);
		Qclose (f);
	}

	return crc;
}
