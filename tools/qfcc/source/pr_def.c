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
*/
static const char rcsid[] =
	"$Id$";

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>

#include <QF/hash.h>
#include <QF/sys.h>
#include <QF/va.h>

#include "qfcc.h"
#include "def.h"
#include "expr.h"
#include "struct.h"
#include "type.h"

typedef struct locref_s {
	struct locref_s *next;
	int         ofs;
} locref_t;

def_t       def_void = { &type_void, "temp" };
def_t       def_function = { &type_function, "temp" };

def_t       def_ret, def_parms[MAX_PARMS];

static def_t *free_temps[4];			// indexted by type size
static def_t temp_scope;
static def_t *free_defs;
static locref_t *free_locs[4];			// indexted by type size
static locref_t *free_free_locs;

static hashtab_t *defs_by_name;

static const char *
defs_get_key (void *_def, void *_tab)
{
	def_t      *def = (def_t *) _def;
	hashtab_t **tab = (hashtab_t **) _tab;

	if (tab == &defs_by_name) {
		return def->name;
	}
	return "";
}

static def_t *
check_for_name (type_t *type, const char *name, def_t *scope, int *allocate)
{
	def_t      *def;

	if (!defs_by_name) {
		defs_by_name = Hash_NewTable (16381, defs_get_key, 0, &defs_by_name);
	}
	if (!name)
		return 0;
	if (!scope && (find_struct (name) || get_enum (name))) {
		error (0, "%s redeclared", name);
		return 0;
	}
	// see if the name is already in use
	def = (def_t *) Hash_Find (defs_by_name, name);
	if (def) {
		if (allocate && scope == def->scope)
			if (type && def->type != type)
				error (0, "Type mismatch on redeclaration of %s", name);
		if (!allocate || def->scope == scope)
			return def;
	}
	return 0;
}

/*
	PR_GetDef

	If type is NULL, it will match any type
	If allocate is true, a new def will be allocated if it can't be found
*/
def_t *
PR_GetDef (type_t *type, const char *name, def_t *scope, int *allocate)
{
	def_t      *def = check_for_name (type, name, scope, allocate);
	int         size;

	if (def || !allocate)
		return def;

	// allocate a new def
	def = PR_NewDef (type, name, scope);
	if (name)
		Hash_Add (defs_by_name, def);

	// FIXME: need to sort out location re-use
	def->ofs = *allocate;

	/* 
		make automatic defs for the vectors elements .origin can be accessed
		as .origin_x, .origin_y, and .origin_z
	*/
	if (type->type == ev_vector && name) {
		def_t      *d;

		d = PR_GetDef (&type_float, va ("%s_x", name), scope, allocate);
		d->used = 1;
		d->parent = def;

		d = PR_GetDef (&type_float, va ("%s_y", name), scope, allocate);
		d->used = 1;
		d->parent = def;

		d = PR_GetDef (&type_float, va ("%s_z", name), scope, allocate);
		d->used = 1;
		d->parent = def;
	} else {
		*allocate += type_size (type);
	}

	if (type->type == ev_field) {
		G_INT (def->ofs) = pr.size_fields;

		if (type->aux_type->type == ev_vector) {
			def_t      *d;

			d = PR_GetDef (&type_floatfield,
						   va ("%s_x", name), scope, allocate);
			d->used = 1;				// always `used'
			d->parent = def;

			d = PR_GetDef (&type_floatfield,
						   va ("%s_y", name), scope, allocate);
			d->used = 1;				// always `used'
			d->parent = def;

			d = PR_GetDef (&type_floatfield,
						   va ("%s_z", name), scope, allocate);
			d->used = 1;				// always `used'
			d->parent = def;
		} else if (type->aux_type->type == ev_pointer) {
			//FIXME I don't think this is right for a field pointer
			size = type_size  (type->aux_type->aux_type);
			pr.size_fields += type->aux_type->num_parms * size;
		} else {
			size = type_size  (type->aux_type);
			pr.size_fields += size;
		}
	} else if (type->type == ev_pointer && type->num_parms) {
		int         ofs = *allocate;

		size = type_size  (type->aux_type);
		*allocate += type->num_parms * size;

		if (pr_scope) {
			expr_t     *e1 = new_expr ();
			expr_t     *e2 = new_expr ();

			e1->type = ex_def;
			e1->e.def = def;

			e2->type = ex_def;
			e2->e.def = PR_NewDef (type->aux_type, 0, pr_scope);
			e2->e.def->ofs = ofs;

			append_expr (local_expr, new_binary_expr ('=', e1, address_expr (e2, 0, 0)));
		} else {
			G_INT (def->ofs) = ofs;
			def->constant = 1;
		}
		def->initialized = 1;
	}

	return def;
}

def_t *
PR_NewDef (type_t *type, const char *name, def_t *scope)
{
	def_t      *def;

	ALLOC (16384, def_t, defs, def);

	if (name) {
		*pr.def_tail = def;
		pr.def_tail = &def->def_next;
	}

	if (scope) {
		def->scope_next = scope->scope_next;
		scope->scope_next = def;
	}

	def->name = name ? strdup (name) : 0;
	def->type = type;

	def->scope = scope;

	def->file = s_file;
	def->line = pr_source_line;

	return def;
}

int
PR_NewLocation (type_t *type)
{
	int         size = type_size (type);
	locref_t   *loc;

	if (free_locs[size]) {
		loc = free_locs[size];
		free_locs[size] = loc->next;

		loc->next = free_free_locs;
		free_free_locs = loc;

		return loc->ofs;
	}
	pr.num_globals += size;
	return pr.num_globals - size;
}

void
PR_FreeLocation (def_t *def)
{
	int         size = type_size (def->type);
	locref_t   *loc;

	ALLOC (1024, locref_t, free_locs, loc);
	loc->ofs = def->ofs;
	loc->next = free_locs[size];
	free_locs[size] = loc;
}

def_t *
PR_GetTempDef (type_t *type, def_t *scope)
{
	int         size = type_size (type);
	def_t      *def;

	if (free_temps[size]) {
		def = free_temps[size];
		free_temps[size] = def->next;
		def->type = type;
	} else {
		def = PR_NewDef (type, 0, scope);
		def->ofs = *scope->alloc;
		*scope->alloc += size;
	}
	def->freed = def->removed = def->users = 0;
	def->next = temp_scope.next;
	temp_scope.next = def;
	return def;
}

void
PR_FreeTempDefs (void)
{
	def_t     **def, *d;
	int         size;

	def = &temp_scope.next;
	while (*def) {
		if ((*def)->users <= 0) {
			d = *def;
			*def = d->next;

			if (d->users < 0)
				printf ("%s:%d: warning: %s %3d %3d %d\n", pr.strings + d->file,
						d->line, pr_type_name[d->type->type], d->ofs, d->users,
						d->managed);
			size = type_size (d->type);
			if (d->expr)
				d->expr->e.temp.def = 0;

			if (!d->freed) {
				d->next = free_temps[size];
				free_temps[size] = d;
				d->freed = 1;
			}
		} else {
			def = &(*def)->next;
		}
	}
}

void
PR_ResetTempDefs (void)
{
	int         i;
	def_t      *d;

	for (i = 0; i < sizeof (free_temps) / sizeof (free_temps[0]); i++) {
		free_temps[i] = 0;
	}

	for (d = temp_scope.next; d; d = d->next)
		printf ("%s:%d: warning: %s %3d %3d %d\n", pr.strings + d->file, d->line,
				pr_type_name[d->type->type], d->ofs, d->users, d->managed);
	temp_scope.next = 0;
}

void
PR_FlushScope (def_t *scope, int force_used)
{
	def_t      *def;

	for (def = scope->scope_next; def; def = def->scope_next) {
		if (def->name) {
			if (!force_used && !def->used) {
				expr_t      e;

				e.line = def->line;
				e.file = def->file;
				warning (&e, "unused variable `%s'", def->name);
			}
			if (!def->removed) {
				Hash_Del (defs_by_name, def->name);
				def->removed = 1;
			}
		}
	}
}

void
PR_DefInitialized (def_t *d)
{
	d->initialized = 1;
	if (d->type == &type_vector
		|| (d->type->type == ev_field && d->type->aux_type == &type_vector)) {
		d = d->def_next;
		d->initialized = 1;
		d = d->def_next;
		d->initialized = 1;
		d = d->def_next;
		d->initialized = 1;
	}
	if (d->parent) {
		d = d->parent;
		if (d->type == &type_vector
			&& d->def_next->initialized
			&& d->def_next->def_next->initialized
			&& d->def_next->def_next->def_next->initialized)
			d->initialized = 1;
	}
}
