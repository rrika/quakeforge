/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2002 #AUTHOR#

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

static __attribute__ ((unused))
const char  rcsid[] =
	"$Id$";

#include <stdlib.h>
#include <string.h>

#include "QF/sys.h"
#include "QF/dstring.h"
#include "QF/hash.h"
#include "QF/cbuf.h"
#include "QF/va.h"
#include "QF/gib.h"

#include "gib_buffer.h"
#include "gib_tree.h"
#include "gib_function.h"
#include "gib_vars.h"

hashtab_t  *gib_functions = 0;

/*
	GIB_Function_New
	
	Builds a new function struct and returns
	a pointer to it.
*/
static gib_function_t *
GIB_Function_New (const char *name)
{
	gib_function_t *new = calloc (1, sizeof (gib_function_t));

	new->text = dstring_newstr ();
	new->name = strdup (name);
	return new;
}

/* 
	Hashtable callbacks
*/
static const char *
GIB_Function_Get_Key (void *ele, void *ptr)
{
	return ((gib_function_t *) ele)->name;
}

static void
GIB_Function_Free (void *ele, void *ptr)
{
	gib_function_t *func = (gib_function_t *) ele;

	dstring_delete (func->text);
	free ((void *) func->name);
	if (func->program)
		GIB_Tree_Free_Recursive (func->program);
	if (func->script && !(--func->script->refs)) {
		free ((void *) func->script->text);
		free ((void *) func->script->file);
		free (func->script);
	}
	free (func);
}

/*
	GIB_Function_Define
	
	Sets the program and text of a GIB function,
	allocating one and adding it to the functions
	hash if needed.
*/
gib_function_t *
GIB_Function_Define (const char *name, const char *text, gib_tree_t * program,
					 gib_script_t * script, hashtab_t * globals)
{
	gib_function_t *func;

	GIB_Tree_Ref (&program);
	if (script)
		script->refs++;
	if (!gib_functions)
		gib_functions =
			Hash_NewTable (1024, GIB_Function_Get_Key, GIB_Function_Free, 0);

	func = Hash_Find (gib_functions, name);
	if (func) {
		dstring_clearstr (func->text);
		GIB_Tree_Unref (&func->program);
		if (func->script && !(--func->script->refs)) {
			free ((void *) func->script->text);
			free ((void *) func->script->file);
			free (func->script);
		}
	} else {
		func = GIB_Function_New (name);
		Hash_Add (gib_functions, func);
	}
	dstring_appendstr (func->text, text);
	func->program = program;
	func->globals = globals;
	func->script = script;

	return func;
}

/*
	GIB_Function_Find
	
	Looks up a function in the function hash
	and returns a pointer to it on success,
	0 otherwise
*/
gib_function_t *
GIB_Function_Find (const char *name)
{
	if (!gib_functions)
		return 0;
	return (gib_function_t *) Hash_Find (gib_functions, name);
}

void
GIB_Function_Prepare_Args (cbuf_t * cbuf, const char **args, unsigned int argc)
{
	static hashtab_t *zero = 0;
	unsigned int i;
	gib_var_t  *var;
	static char argss[] = "args";

	var =
		GIB_Var_Get_Complex (&GIB_DATA (cbuf)->locals, &zero, argss, &i, true);
	var->array = realloc (var->array, sizeof (struct gib_varray_s) * argc);
	memset (var->array + 1, 0, (argc - 1) * sizeof (struct gib_varray_s));
	var->size = argc;
	for (i = 0; i < argc; i++) {
		if (var->array[i].value)
			dstring_clearstr (var->array[i].value);
		else
			var->array[i].value = dstring_newstr ();
		dstring_appendstr (var->array[i].value, args[i]);
	}
}

void
GIB_Function_Prepare_Args_D (cbuf_t * cbuf, dstring_t **args, unsigned int argc)
{
	static hashtab_t *zero = 0;
	unsigned int i;
	gib_var_t  *var;
	static char argss[] = "args";

	var =
		GIB_Var_Get_Complex (&GIB_DATA (cbuf)->locals, &zero, argss, &i, true);
	var->array = realloc (var->array, sizeof (struct gib_varray_s) * argc);
	memset (var->array + 1, 0, (argc - 1) * sizeof (struct gib_varray_s));
	var->size = argc;
	for (i = 0; i < argc; i++) {
		if (var->array[i].value)
			dstring_clearstr (var->array[i].value);
		else
			var->array[i].value = dstring_newstr ();
		dstring_appendstr (var->array[i].value, args[i]->str);
	}
}

/*
	GIB_Function_Execute
	
	Prepares a buffer to execute
	a GIB function with certain arguments
*/

void
GIB_Function_Execute (cbuf_t * cbuf, gib_function_t * func, const char ** args,
					  unsigned int argc)
{
	GIB_Tree_Ref (&func->program);
	if (func->script)
		func->script->refs++;
	GIB_Buffer_Set_Program (cbuf, func->program);
	GIB_DATA (cbuf)->script = func->script;
	GIB_DATA (cbuf)->globals = func->globals;
	GIB_Function_Prepare_Args (cbuf, args, argc);
}

void
GIB_Function_Execute_D (cbuf_t * cbuf, gib_function_t * func, dstring_t ** args,
					  unsigned int argc)
{
	GIB_Tree_Ref (&func->program);
	if (func->script)
		func->script->refs++;
	GIB_Buffer_Set_Program (cbuf, func->program);
	GIB_DATA (cbuf)->script = func->script;
	GIB_DATA (cbuf)->globals = func->globals;
	GIB_Function_Prepare_Args_D (cbuf, args, argc);
}
