/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.

	$Id$
*/

#ifndef __qfcc_h
#define __qfcc_h

#include "QF/pr_comp.h"
#include "QF/pr_debug.h"

// offsets are allways multiplied by 4 before using
typedef int	gofs_t;				// offset in global data block
typedef struct function_s function_t;

typedef struct type_s {
	etype_t			type;
	struct type_s	*next;
// function/pointer/struct types are more complex
	struct type_s	*aux_type;	// return type or field type
	int				num_parms;	// -1 = variable args
	struct type_s	*parm_types[MAX_PARMS];	// only [num_parms] allocated
	struct hashtab_s *struct_fields;
	struct struct_field_s	*struct_head;
	struct struct_field_s	**struct_tail;
	struct class_s	*class;		// for ev_class
} type_t;

typedef struct statref_s {
	struct statref_s *next;
	dstatement_t	*statement;
	int				field;		// a, b, c (0, 1, 2)
} statref_t;

typedef struct def_s {
	type_t			*type;
	const char		*name;
	int				locals;
	int				*alloc;
	gofs_t			ofs;
	int				initialized;	// for uninit var detection
	int				constant;	// 1 when a declaration included "= immediate"
	statref_t		*refs;			// for relocations

	unsigned		freed:1;		// already freed from the scope
	unsigned		removed:1;		// already removed from the symbol table
	unsigned		used:1;			// unused local detection
	unsigned		absolute:1;		// don't relocate (for temps for shorts)
	unsigned		managed:1;		// managed temp
	string_t		file;			// source file
	int				line;			// source line

	int				users;			// ref counted temps
	struct expr_s	*expr;			// temp expr using this def

	struct def_s	*def_next;		// for writing out the global defs list
	struct def_s	*next;			// general purpose linking
	struct def_s	*scope_next;	// to facilitate hash table removal
	struct def_s	*scope;			// function the var was defined in, or NULL
	struct def_s	*parent;		// vector/quaternion member
} def_t;

//============================================================================

// pr_loc.h -- program local defs

#define	MAX_ERRORS		10

#define	MAX_REGS		65536

//=============================================================================

struct function_s {
	struct function_s	*next;
	dfunction_t			*dfunc;
	pr_auxfunction_t	*aux;		// debug info;
	int					builtin;	// if non 0, call an internal function
	int					code;		// first statement
	const char			*file;		// source file with definition
	int					file_line;
	struct def_s		*def;
	int					parm_ofs[MAX_PARMS];	// allways contiguous, right?
};

extern function_t *pr_functions;
extern function_t *current_func;

//
// output generated by prog parsing
//
typedef struct {
	int			current_memory;
	type_t		*types;
	
	def_t		def_head;		// unused head of linked list
	def_t		*def_tail;		// add new defs after this and move it
	def_t		*search;		// search chain through defs

	int			size_fields;
} pr_info_t;

extern	pr_info_t	pr;

extern opcode_t *op_done;
extern opcode_t *op_return;
extern opcode_t *op_if;
extern opcode_t *op_ifnot;
extern opcode_t *op_ifbe;
extern opcode_t *op_ifb;
extern opcode_t *op_ifae;
extern opcode_t *op_ifa;
extern opcode_t *op_state;
extern opcode_t *op_goto;
extern opcode_t *op_jump;
extern opcode_t *op_jumpb;

statref_t *PR_NewStatref (dstatement_t *st, int field);
void PR_AddStatementRef (def_t *def, dstatement_t *st, int field);
def_t *PR_Statement (opcode_t *op, def_t *var_a, def_t *var_b);
opcode_t *PR_Opcode_Find (const char *name,
						  def_t *var_a, def_t *var_b, def_t *var_c);
void PR_Opcode_Init_Tables (void);

//============================================================================

extern	def_t		*pr_global_defs[MAX_REGS];	// to find def for a global

struct expr_s;
def_t *PR_ReuseConstant (struct expr_s *expr, def_t *def);

extern	char		destfile[];
extern	int			pr_source_line;

extern	def_t	*pr_scope;
extern	int		pr_error_count;

def_t *PR_GetArray (type_t *etype, const char *name, int size, def_t *scope,
					int *allocate);

def_t *PR_GetDef (type_t *type, const char *name, def_t *scope,
				  int *allocate);
def_t *PR_NewDef (type_t *type, const char *name, def_t *scope);
int PR_NewLocation (type_t *type);
void PR_FreeLocation (def_t *def);
def_t *PR_GetTempDef (type_t *type, def_t *scope);
void PR_FreeTempDefs ();
void PR_ResetTempDefs ();
void PR_FlushScope (def_t *scope, int force_used);
void PR_DefInitialized (def_t *d);

#define	G_FLOAT(o) (pr_globals[o])
#define	G_INT(o) (*(int *)&pr_globals[o])
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (strings + *(string_t *)&pr_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])
#define G_STRUCT(t,o)   (*(t *)&pr_globals[o])

extern	string_t	s_file;			// filename for function definition

extern	def_t	def_ret, def_parms[MAX_PARMS];

//=============================================================================

#define	MAX_STRINGS		500000
#define	MAX_GLOBALS		65536
#define	MAX_FIELDS		1024
#define	MAX_STATEMENTS	131072
#define	MAX_FUNCTIONS	8192

#define	MAX_SOUNDS		1024
#define	MAX_MODELS		1024
#define	MAX_FILES		1024
#define	MAX_DATA_PATH	64

extern	char	strings[MAX_STRINGS];
extern	int		strofs;

extern	dstatement_t	statements[MAX_STATEMENTS];
extern	int			numstatements;
extern	int			statement_linenums[MAX_STATEMENTS];

extern	dfunction_t	functions[MAX_FUNCTIONS];
extern	int			numfunctions;

extern	float		pr_globals[MAX_REGS];
extern	int			numpr_globals;

extern	int         num_auxfunctions;
extern	pr_auxfunction_t *auxfunctions;

extern	int         num_linenos;
extern	pr_lineno_t *linenos;

extern	int         num_locals;
extern	ddef_t      *locals;

pr_auxfunction_t *new_auxfunction (void);
pr_lineno_t *new_lineno (void);
ddef_t *new_local (void);

int	CopyString (const char *str);
int	ReuseString (const char *str);
const char *strip_path (const char *filename);

typedef struct {
	qboolean	cow;				// Turn constants into variables if written to
	qboolean	debug;				// Generate debug info for the engine
	int			progsversion;		// Progs version to generate code for
} code_options_t;

typedef struct {
	qboolean	promote;			// Promote warnings to errors
	qboolean	cow;				// Warn on copy-on-write detection
	qboolean	undefined_function;	// Warn on undefined function use
	qboolean	uninited_variable;	// Warn on use of uninitialized vars
	qboolean	vararg_integer;		// Warn on passing an integer to vararg func
	qboolean	integer_divide;		// Warn on integer constant division
} warn_options_t;

typedef struct {
	code_options_t	code;			// Code generation options
	warn_options_t	warnings;		// Warning options

	int				verbosity;		// 0=silent, goes up to 2 currently
	qboolean		save_temps;		// save temporary files
	qboolean		files_dat;		// generate files.dat
	qboolean		traditional;	// behave more like qcc
	int				strip_path;		// number of leading path elements to strip
									// from source file names
} options_t;

extern options_t options;

//XXX eww :/
void PrecacheSound (def_t *e, int ch);
void PrecacheModel (def_t *e, int ch);
void PrecacheFile (def_t *e, int ch);
void WriteFiles (const char *sourcedir);
int  WriteProgdefs (char *filename);

#endif//__qfcc_h
