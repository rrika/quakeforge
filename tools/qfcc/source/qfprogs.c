/*
	qfprogs.c

	Progs dumping, main file.

	Copyright (C) 2002 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2002/05/13

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

#include <getopt.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#include <string.h>
#include <getopt.h>
#include <sys/types.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# include <sys/fcntl.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>

#include "QF/cmd.h"
#include "QF/cvar.h"
#include "QF/hash.h"
#include "QF/pr_comp.h"
#include "QF/progs.h"
#include "QF/quakeio.h"
#include "QF/sys.h"
#include "QF/va.h"
#include "QF/zone.h"

#include "obj_file.h"
#include "obj_type.h"
#include "qfprogs.h"
#include "reloc.h"

const char *reloc_names[] = {
	"none",
	"op_a_def",
	"op_b_def",
	"op_c_def",
	"op_a_op",
	"op_b_op",
	"op_c_op",
	"def_op",
	"def_def",
	"def_func",
	"def_string",
	"def_field",
	"op_a_def_ofs",
	"op_b_def_ofs",
	"op_c_def_ofs",
	"def_def_ofs",
};

int         sorted = 0;
int         verbosity = 0;

static const struct option long_options[] = {
	{"disassemble", no_argument, 0, 'd'},
	{"fields", no_argument, 0, 'f'},
	{"functions", no_argument, 0, 'F'},
	{"globals", no_argument, 0, 'g'},
	{"help", no_argument, 0, 'h'},
	{"lines", no_argument, 0, 'l'},
	{"modules", no_argument, 0, 'M'},
	{"numeric", no_argument, 0, 'n'},
	{"path", required_argument, 0, 'P'},
	{"relocs", no_argument, 0, 'r'},
	{"strings", no_argument, 0, 's'},
	{"verbose", no_argument, 0, 'v'},
	{NULL, 0, NULL, 0},
};

static const char *short_options =
	"d"		// disassemble
	"F"		// functions
	"f"		// fields
	"g"		// globals
	"h"		// help
	"l"		// lines
	"M"		// modules
	"n"		// numeric
	"P:"	// path
	"r"		// relocs
	"s"		// strings
	"v"		// verbose
	;

static edict_t *edicts;
static int      num_edicts;
static int      reserved_edicts = 1;
static progs_t  pr;

static pr_debug_header_t debug;
static qfo_t   *qfo;
static dprograms_t progs;

static const char *source_path = "";

static hashtab_t *func_tab;

static void __attribute__((noreturn))
usage (int status)
{
	printf ("%s - QuakeForge progs utility\n", "qfprogs");
	printf ("Usage: %s [options] [files]\n", "qfprogs");
	printf (
"    -d, --disassemble   Dump code disassembly.\n"
"    -f, --fields        Dump entity fields.\n"
"    -F, --functions     Dump functions.\n"
"    -g, --globals       Dump global variables.\n"
"    -h, --help          Display this help and exit\n"
"    -l, --lines         Dump line number information.\n"
"    -M, --modules       Dump Objective-QuakeC data.\n"
"    -n, --numeric       Sort globals by address.\n"
"    -P, --path DIR      Source path.\n"
"    -r, --relocs        Dump reloc information.\n"
"    -s, --strings       Dump static strings.\n"
"    -v, --verbose       Display more output than usual.\n"
    );
	exit (status);
}

static QFile *
open_file (const char *path, int *len)
{
	QFile      *file = Qopen (path, "rbz");

	if (!file)
		return 0;
	*len = Qfilesize (file);
	return file;
}

static void
file_error (progs_t *pr, const char *name)
{
	perror (name);
}

static void *
load_file (progs_t *pr, const char *name)
{
	QFile      *file;
	int         size;
	char       *sym;

	file = open_file (name, &size);
	if (!file) {
		file = open_file (va ("%s.gz", name), &size);
		if (!file)
			return 0;
	}
	sym = malloc (size + 1);
	sym[size] = 0;
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

static uintptr_t
func_hash (void *func, void *unused)
{
	return ((dfunction_t *) func)->first_statement;
}

static int
func_compare (void *f1, void *f2, void *unused)
{
	return ((dfunction_t *) f1)->first_statement
			== ((dfunction_t *) f2)->first_statement;
}

dfunction_t *
func_find (int st_ofs)
{
	dfunction_t f;

	f.first_statement = st_ofs;
	return Hash_FindElement (func_tab, &f);
}

static void
init_qf (void)
{
	Cvar_Init_Hash ();
	Cmd_Init_Hash ();
	Cvar_Init ();
	Sys_Init_Cvars ();
	Cmd_Init ();

	Cvar_Get ("pr_debug", va ("%d", verbosity), 0, 0, "");
	Cvar_Get ("pr_source_path", source_path, 0, 0, "");
	PR_Init_Cvars ();
	PR_Init ();

	pr.edicts = &edicts;
	pr.num_edicts = &num_edicts;
	pr.reserved_edicts = &reserved_edicts;
	pr.file_error = file_error;
	pr.load_file = load_file;
	pr.allocate_progs_mem = allocate_progs_mem;
	pr.free_progs_mem = free_progs_mem;

	func_tab = Hash_NewTable (1021, 0, 0, 0);
	Hash_SetHashCompare (func_tab, func_hash, func_compare);
}

static etype_t
get_type (pointer_t type)
{
	qfot_type_t *type_def;
	if (type < 0 || type >= qfo->spaces[qfo_type_space].data_size)
		return ev_void;
	type_def = QFO_POINTER (qfo, qfo_type_space, qfot_type_t, type);
	switch ((ty_type_e)type_def->ty) {
		case ty_none:
			// field, pointer and function types store their basic type in
			// the same location.
			return type_def->t.type;
		case ty_struct:
		case ty_union:
			return ev_invalid;
		case ty_enum:
			return ev_integer;	// FIXME v6 progs should be float
		case ty_array:
		case ty_class:
			return ev_invalid;
	}
	return ev_invalid;
}

static etype_t
get_type_size (pointer_t type)
{
	qfot_type_t *type_def;
	int          i, size;
	if (type < 0 || type >= qfo->spaces[qfo_type_space].data_size)
		return 1;
	type_def = QFO_POINTER (qfo, qfo_type_space, qfot_type_t, type);
	switch ((ty_type_e)type_def->ty) {
		case ty_none:
			// field, pointer and function types store their basic type in
			// the same location.
			return pr_type_size[type_def->t.type];
		case ty_struct:
			for (i = size = 0; i < type_def->t.strct.num_fields; i++)
				size += get_type_size (type_def->t.strct.fields[i].type);
			return size;
		case ty_union:
			for (i = size = 0; i < type_def->t.strct.num_fields; i++) {
				int         s;
				s = get_type_size (type_def->t.strct.fields[i].type);
				if (s > size)
					size = s;
			}
			return size;
		case ty_enum:
			return pr_type_size[ev_integer];
		case ty_array:
			return type_def->t.array.size
					* get_type_size (type_def->t.array.type);
		case ty_class:
			return 0;	// FIXME
	}
	return 0;
}

static void
function_params (dfunction_t *df, qfo_func_t *func)
{
	qfot_type_t *type;
	int         num_params;
	int         i;

	if (func->type < 0 || func->type >= qfo->spaces[qfo_type_space].data_size)
		return;
	type = QFO_POINTER (qfo, qfo_type_space, qfot_type_t, func->type);
	if (type->ty != ty_none && type->t.type != ev_func)
		return;
	df->numparms = num_params = type->t.func.num_params;
	if (num_params < 0)
		num_params = ~num_params;
	for (i = 0; i < num_params; i++)
		df->parm_size[i] = get_type_size (type->t.func.param_types[i]);
}

static void
convert_def (const qfo_def_t *def, ddef_t *ddef)
{
	ddef->type = get_type (def->type);
	ddef->ofs = def->offset;
	ddef->s_name = def->name;
	if (!(def->flags & QFOD_NOSAVE)
		&& !(def->flags & QFOD_CONSTANT)
		&& (def->flags & QFOD_GLOBAL)
		&& ddef->type != ev_func
		&& ddef->type != ev_field)
		ddef->type |= DEF_SAVEGLOBAL;
}

static void
convert_qfo (void)
{
	int         i, j, num_locals = 0, num_externs = 0;
	qfo_def_t  *defs;
	ddef_t     *ld;

	defs = malloc (qfo->num_defs * sizeof (qfo_def_t));
	memcpy (defs, qfo->defs, qfo->num_defs * sizeof (qfo_def_t));

	pr.progs = &progs;
	progs.version = PROG_VERSION;

	pr.pr_statements = malloc (qfo->spaces[qfo_code_space].data_size
							   * sizeof (dstatement_t));
	memcpy (pr.pr_statements, qfo->spaces[qfo_code_space].d.code,
			qfo->spaces[qfo_code_space].data_size * sizeof (dstatement_t));
	progs.numstatements = qfo->spaces[qfo_code_space].data_size;

	pr.pr_strings = qfo->spaces[qfo_strings_space].d.strings;
	progs.numstrings = qfo->spaces[qfo_strings_space].data_size;
	pr.pr_stringsize = qfo->spaces[qfo_strings_space].data_size;

	progs.numfunctions = qfo->num_funcs + 1;
	pr.pr_functions = calloc (progs.numfunctions, sizeof (dfunction_t));
	pr.auxfunctions = calloc (qfo->num_funcs, sizeof (pr_auxfunction_t));
	pr.auxfunction_map = calloc (progs.numfunctions,
								 sizeof (pr_auxfunction_t *));
	ld = pr.local_defs = calloc (qfo->num_defs, sizeof (ddef_t));
	for (i = 0; i < qfo->num_funcs; i++) {
		qfo_func_t *func = qfo->funcs + i;
		dfunction_t df;

		memset (&df, 0, sizeof (df));

		df.first_statement = func->code;
		df.parm_start = qfo->spaces[qfo_near_data_space].data_size;
		df.locals = qfo->spaces[func->locals_space].data_size;
		df.profile = 0;
		df.s_name = func->name;
		df.s_file = func->file;
		function_params (&df, func);

		if (df.locals > num_locals)
			num_locals = df.locals;

		pr.pr_functions[i + 1] = df;

		pr.auxfunction_map[i + 1] = pr.auxfunctions + i;
		pr.auxfunctions[i].function = i + 1;
		pr.auxfunctions[i].source_line = func->line;
		pr.auxfunctions[i].line_info = func->line_info;
		pr.auxfunctions[i].local_defs = ld - pr.local_defs;
		pr.auxfunctions[i].num_locals =
			qfo->spaces[func->locals_space].num_defs;

		for (j = 0; j < qfo->spaces[func->locals_space].num_defs; j++) {
			qfo_def_t  *d = qfo->spaces[func->locals_space].defs + j;
			convert_def (d, ld++);
			ld->ofs += qfo->spaces[qfo_near_data_space].data_size;
		}
	}

	progs.numglobaldefs = 0;
	progs.numfielddefs = 0;
	progs.entityfields = 0;
	pr.pr_globaldefs = calloc (qfo->num_defs, sizeof (ddef_t));
	pr.pr_fielddefs = calloc (qfo->num_defs, sizeof (ddef_t));
	for (i = 0; i < qfo->num_defs; i++) {
		qfo_def_t  *def = defs + i;
		ddef_t      ddef;

		if (!(def->flags & QFOD_LOCAL) && def->name) {
			if (def->flags & QFOD_EXTERNAL) {
				int         size = get_type_size (def->type);
				if (!size)
					size = 1;
				def->offset += qfo->spaces[qfo_near_data_space].data_size
					+ num_locals + num_externs;
				num_externs += size;
			}

			convert_def (def, &ddef);
			pr.pr_globaldefs[progs.numglobaldefs++] = ddef;
			if (ddef.type == ev_field) {
				ddef.type = get_type (def->type);
				progs.entityfields += get_type_size (def->type);
				ddef.ofs = QFO_INT (qfo, qfo_near_data_space, ddef.ofs);
				pr.pr_fielddefs[progs.numfielddefs++] = ddef;
			}
		}
	}

	progs.numglobals = qfo->spaces[qfo_near_data_space].data_size;
	pr.globals_size = progs.numglobals + num_locals + num_externs;
	pr.globals_size += qfo->spaces[qfo_far_data_space].data_size;
	pr.pr_globals = calloc (pr.globals_size, sizeof (pr_type_t));
	memcpy (pr.pr_globals, qfo->spaces[qfo_near_data_space].d.data,
			qfo->spaces[qfo_near_data_space].data_size * sizeof (pr_type_t));
	memcpy (pr.pr_globals + progs.numglobals + num_locals + num_externs,
			qfo->spaces[qfo_far_data_space].d.data,
			qfo->spaces[qfo_far_data_space].data_size * sizeof (pr_type_t));

	for (i = 0; i < qfo->num_defs; i++) {
		break;
		qfo_def_t  *def = defs + i;

		for (j = 0; j < def->num_relocs; j++) {
			qfo_reloc_t *reloc = qfo->relocs + def->relocs + j;
			switch ((reloc_type)reloc->type) {
				case rel_none:
					break;
				case rel_op_a_def:
					pr.pr_statements[reloc->offset].a = def->offset;
					break;
				case rel_op_b_def:
					pr.pr_statements[reloc->offset].b = def->offset;
					break;
				case rel_op_c_def:
					pr.pr_statements[reloc->offset].c = def->offset;
					break;
				case rel_op_a_def_ofs:
					pr.pr_statements[reloc->offset].a += def->offset;
					break;
				case rel_op_b_def_ofs:
					pr.pr_statements[reloc->offset].b += def->offset;
					break;
				case rel_op_c_def_ofs:
					pr.pr_statements[reloc->offset].c += def->offset;
					break;
				case rel_def_def:
					pr.pr_globals[reloc->offset].integer_var = def->offset;
					break;
				case rel_def_def_ofs:
					pr.pr_globals[reloc->offset].integer_var += def->offset;
					break;
				// these are relative and fixed up before the .qfo is written
				case rel_op_a_op:
				case rel_op_b_op:
				case rel_op_c_op:
				// these aren't relevant here
				case rel_def_func:
				case rel_def_op:
				case rel_def_string:
				case rel_def_field:
				case rel_def_field_ofs:
					break;
			}
		}
	}

	pr.pr_edict_size = progs.entityfields * 4;

	pr.linenos = qfo->lines;
	debug.num_auxfunctions = qfo->num_funcs;
	debug.num_linenos = qfo->num_lines;
	debug.num_locals = ld - pr.local_defs;

	if (verbosity)
		pr.debug = &debug;

}

static int
load_progs (const char *name)
{
	QFile      *file;
	int         i, size;
	char        buff[5];

	Hash_FlushTable (func_tab);

	file = open_file (name, &size);
	if (!file) {
		perror (name);
		return 0;
	}
	Qread (file, buff, 4);
	buff[4] = 0;
	Qseek (file, 0, SEEK_SET);
	if (!strcmp (buff, QFO)) {
		qfo = qfo_read (file);
		Qclose (file);

		if (!qfo)
			return 0;

		convert_qfo ();
	} else {
		pr.progs_name = name;
		PR_LoadProgsFile (&pr, file, size, 1, 0);
		Qclose (file);

		if (!pr.progs)
			return 0;

		PR_LoadStrings (&pr);
		PR_ResolveGlobals (&pr);
		PR_LoadDebug (&pr);
	}
	for (i = 0; i < pr.progs->numfunctions; i++) {
		// don't bother with builtins
		if (pr.pr_functions[i].first_statement > 0)
			Hash_AddElement (func_tab, &pr.pr_functions[i]);
	}
	return 1;
}

typedef struct {
	void      (*progs) (progs_t *pr);
	void      (*qfo) (qfo_t *qfo);
} operation_t;

operation_t operations[] = {
	{disassemble_progs, 0},					// disassemble
	{dump_globals,		qfo_globals},		// globals
	{dump_strings,		0},					// strings
	{dump_fields,		0},					// fields
	{dump_functions,	qfo_functions},		// functions
	{dump_lines,		0},					// lines
	{dump_modules,		0},					// modules
	{0,					qfo_relocs},		// relocs
};

int
main (int argc, char **argv)
{
	int         c;
	operation_t *func = &operations[0];

	while ((c = getopt_long (argc, argv, short_options,
							 long_options, 0)) != EOF) {
		switch (c) {
			case 'd':
				func = &operations[0];
				break;
			case 'F':
				func = &operations[4];
				break;
			case 'f':
				func = &operations[3];
				break;
			case 'g':
				func = &operations[1];
				break;
			case 'h':
				usage (0);
			case 'l':
				func = &operations[5];
				break;
			case 'M':
				func = &operations[6];
				break;
			case 'n':
				sorted = 1;
				break;
			case 'P':
				source_path = strdup (optarg);
				break;
			case 'r':
				func = &operations[7];
				break;
			case 's':
				func = &operations[2];
				break;
			case 'v':
				verbosity++;
				break;
			default:
				usage (1);
		}
	}
	init_qf ();
	while (optind < argc) {
		if (!load_progs (argv[optind++]))
			return 1;
		if (qfo && func->qfo)
			func->qfo (qfo);
		else if (func->progs)
			func->progs (&pr);
		else
			fprintf (stderr, "can't process %s\n", argv[optind - 1]);
	}
	return 0;
}
