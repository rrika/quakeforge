/*
	cmd.c

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

#include <ctype.h>

#include "QF/cmd.h"
#include "QF/cvar.h"
#include "QF/hash.h"
#include "QF/qargs.h"
#include "QF/qendian.h"
#include "QF/sizebuf.h"
#include "QF/sys.h"
#include "QF/vfs.h"
#include "QF/zone.h"
#include "compat.h"
#include "QF/dstring.h"
#include "QF/exp.h"
#include "QF/va.h"

typedef struct cmdalias_s {
	struct cmdalias_s *next;
	const char *name;
	const char *value;
} cmdalias_t;

cmdalias_t *cmd_alias;
cmd_source_t cmd_source;

/* FIXME: All these separate buffers are sort of hacky
The command buffer interface should be generalized and
each part of QF (console, stufftext, config files and scripts)
that needs one should allocate and maintain its own.
*/
cmd_buffer_t *cmd_consolebuffer;		// Console buffer
cmd_buffer_t *cmd_legacybuffer;			// Server stuffcmd buffer with
										// absolute backwards-compatibility
cmd_buffer_t *cmd_activebuffer;			// Buffer currently being executed

dstring_t  *cmd_backtrace;

qboolean    cmd_error;

cvar_t     *cmd_warncmd;
cvar_t     *cmd_highchars;

hashtab_t  *cmd_alias_hash;
hashtab_t  *cmd_hash;

//=============================================================================

/* Local variable stuff */

cmd_localvar_t *
Cmd_NewLocal (const char *key, const char *value)
{
	cmd_localvar_t *new;
	dstring_t  *dkey, *dvalue;

	new = malloc (sizeof (cmd_localvar_t));
	if (!new)
		Sys_Error ("Cmd_NewLocal:  Memory allocation failed!");
	dkey = dstring_newstr ();
	dvalue = dstring_newstr ();
	dstring_appendstr (dkey, key);
	dstring_appendstr (dvalue, value);
	new->key = dkey;
	new->value = dvalue;
	return new;
}

void
Cmd_SetLocal (cmd_buffer_t *buffer, const char *key, const char *value)
{
	cmd_localvar_t *var;

	var = (cmd_localvar_t *)Hash_Find(buffer->locals, key);
	if (!var) {
		var = Cmd_NewLocal (key, value);
		Hash_Add (buffer->locals, (void *) var);
	} else {
		dstring_clearstr (var->value);
		dstring_appendstr (var->value, value);
	}
}

const char *
Cmd_LocalGetKey (void *ele, void *ptr)
{
	return ((cmd_localvar_t *) ele)->key->str;
}

void
Cmd_LocalFree (void *ele, void *ptr)
{
	dstring_delete (((cmd_localvar_t *) ele)->key);
	dstring_delete (((cmd_localvar_t *) ele)->value);
	free (ele);
}

cmd_buffer_t	*
Cmd_NewBuffer (qboolean ownvars)
{
	cmd_buffer_t *new;

	new = calloc (1, sizeof (cmd_buffer_t));

	new->buffer = dstring_newstr ();
	new->line = dstring_newstr ();
	new->realline = dstring_newstr ();
	new->looptext = dstring_newstr ();
	if (ownvars)
		new->locals = Hash_NewTable (1021, Cmd_LocalGetKey, Cmd_LocalFree, 0);
	new->ownvars = ownvars;
	return new;
}

void
Cmd_FreeBuffer (cmd_buffer_t *del)
{
	int         i;

	dstring_delete (del->buffer);
	dstring_delete (del->line);
	dstring_delete (del->realline);
	if (del->maxargc) {
		for (i = 0; i < del->maxargc; i++)
			if (del->argv[i])
				dstring_delete (del->argv[i]);
		free (del->argv);
	}
	if (del->args)
		free(del->args);
	if (del->ownvars)
		Hash_DelTable(del->locals);
	free(del);
}

/* Quick function to determine if a character is escaped */
qboolean
escaped (const char *str, int i)
{
	int         n, c;

	if (!i)
		return 0;
	for (n = i - 1, c = 0; n >= 0 && str[n] == '\\'; n--, c++)
		;
	return c & 1;
}

/* Quick function to escape stuff in a dstring */
void
escape (dstring_t * dstr)
{
	int         i;

	for (i = 0; i < strlen (dstr->str); i++) {
		switch (dstr->str[i]) {
			case '\\':
			case '$':
			case '{':
			case '}':
			case '\"':
			case '\'':
			case '<':
			case '>':
				dstring_insertstr (dstr, "\\", i);
				i++;
				break;
		}
	}
}

/*
	Cmd_Wait_f

	Causes execution of the remainder of the
	command buffer and stack to be delayed until
	next frame.  This allows commands like:
	bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
*/
void
Cmd_Wait_f (void)
{
	cmd_buffer_t *cur;

	for (cur = cmd_activebuffer; cur; cur = cur->prev)
		cur->wait = true;
}

void
Cmd_Error (const char *message)
{
	cmd_buffer_t *cur;

	Sys_Printf ("GIB:  Error in execution.  "
				"Type backtrace for a description and execution path to "
				"the error\n");
	cmd_error = true;
	dstring_clearstr (cmd_backtrace);
	dstring_appendstr (cmd_backtrace, message);
	dstring_appendstr (cmd_backtrace, "Path of execution:\n");
	for (cur = cmd_activebuffer; cur; cur = cur->prev)
		dstring_appendstr (cmd_backtrace, va ("--> %s\n", cur->realline->str));
}


/*
						COMMAND BUFFER
*/

void
Cbuf_Init (void)
{
	cmd_consolebuffer = Cmd_NewBuffer (true);
	
	cmd_legacybuffer = Cmd_NewBuffer (true);
	cmd_legacybuffer->legacy = true;

	cmd_activebuffer = cmd_consolebuffer;

	cmd_backtrace = dstring_newstr ();
}


void
Cbuf_AddTextTo (cmd_buffer_t *buffer, const char *text)
{
	dstring_appendstr (buffer->buffer, text);
}

/* 
	Cbuf_AddText
	
	Add text to the active buffer
*/

void
Cbuf_AddText (const char *text)
{
	Cbuf_AddTextTo (cmd_activebuffer, text);
}

void
Cbuf_InsertTextTo (cmd_buffer_t *buffer, const char *text)
{
	dstring_insertstr (buffer->buffer, "\n", 0);
	dstring_insertstr (buffer->buffer, text, 0);
}

/* Cbuf_InsertText

	Add text to the beginning of the active buffer
*/

void
Cbuf_InsertText (const char *text)
{
	Cbuf_InsertTextTo (cmd_activebuffer, text);
}


/*
	extract_line
	
	Finds the next \n,\r, or ;-delimeted
	line in the command buffer and copies
	it into a buffer.  Also shifts the rest
	of the command buffer back to the start.
*/

void
extract_line (dstring_t * buffer, dstring_t * line)
{
	int         i, squotes = 0, dquotes = 0, braces = 0, n;

	for (i = 0; buffer->str[i]; i++) {
		if (buffer->str[i] == '\'' && !escaped (buffer->str, i)
			&& !dquotes && !braces)
			squotes ^= 1;
		if (buffer->str[i] == '"' && !escaped (buffer->str, i)
			&& !squotes && !braces)
			dquotes ^= 1;
		if (buffer->str[i] == ';' && !escaped (buffer->str, i)
			&& !squotes && !dquotes && !braces)
			break;
		if (buffer->str[i] == '{' && !escaped (buffer->str, i)
			&& !squotes && !dquotes)
			braces++;
		if (buffer->str[i] == '}' && !escaped (buffer->str, i)
			&& !squotes && !dquotes)
			braces--;
		if (buffer->str[i] == '/' && buffer->str[i + 1] == '/'
			&& !squotes && !dquotes) {
			// Filter out comments until newline
			for (n = 0;
				 buffer->str[i + n] != '\n' && buffer->str[i + n] != '\r';
				 n++)
				;
			dstring_snip (buffer, i, n);
		}
		if (buffer->str[i] == '\n' || buffer->str[i] == '\r') {
			if (braces) {
				dstring_snip (buffer, i, 1);
				i--;
			} else
				break;
		}
	}
	if (i)
		dstring_insert (line, buffer->str, i, 0);
	if (buffer->str[i]) {
		dstring_snip (buffer, 0, i + 1);
	} else {
		// We've hit the end of the buffer, just clear it
		dstring_clearstr (buffer);
	}
}

/*
	Cbuf_ExecuteBuffer
	
	Extracts and executes each line in the
	command buffer, until it is empty or
	a wait command is executed
*/

void
Cbuf_ExecuteBuffer (cmd_buffer_t *buffer)
{
	dstring_t  *buf = dstring_newstr ();
	cmd_buffer_t *temp = cmd_activebuffer;	// save old context

	cmd_activebuffer = buffer;
	buffer->wait = false;
	cmd_error = false;
	while (1) {
		if (!strlen(buffer->buffer->str)) {
			if (buffer->loop)
				Cbuf_InsertTextTo(buffer, buffer->looptext->str);
			else
				break;
		}
		extract_line (buffer->buffer, buf);
		Cmd_ExecuteString (buf->str, src_command);
		if (buffer->wait)
			break;
		if (cmd_error)
			break;
		dstring_clearstr (buf);
	}
	dstring_delete (buf);
	cmd_activebuffer = temp;			// restore old context
}

void
Cbuf_ExecuteStack (cmd_buffer_t *buffer)
{
	qboolean    wait = false;
	cmd_buffer_t *cur;

	for (cur = buffer; cur->next; cur = cur->next);
	for (; cur != buffer; cur = cur->prev) {
		Cbuf_ExecuteBuffer (cur);
		if (cur->wait) {
			wait = true;
			break;
		} else if (cmd_error)
			break;
		else {
			cur->prev->next = 0;
			Cmd_FreeBuffer (cur);
		}
	}
	if (!wait)
		Cbuf_ExecuteBuffer (buffer);
	if (cmd_error) {
		// If an error occured, nuke the entire stack
		for (cur = buffer->next; cur; cur = cur->next)
			Cmd_FreeBuffer (cur);
		buffer->next = 0;
		dstring_clearstr (buffer->buffer);	// And the root buffer
	}
}

/*
	Cbuf_Execute

	Executes both root buffers
*/

void
Cbuf_Execute (void)
{
	Cbuf_ExecuteStack (cmd_consolebuffer);
	Cbuf_ExecuteStack (cmd_legacybuffer);
}

void
Cmd_ExecuteSubroutine (cmd_buffer_t *buffer)
{
	cmd_activebuffer->next = buffer;
	buffer->prev = cmd_activebuffer;
	Cbuf_ExecuteBuffer (buffer);
	if (!buffer->wait) {
		Cmd_FreeBuffer (buffer);
		cmd_activebuffer->next = 0;
	}
	return;
}

/*
	Cbuf_Execute_Sets
	
	Similar to Cbuf_Execute, but only
	executes set and setrom commands,
	and only in the console buffer.
	Used for reading config files
	before the cmd subsystem is
	entirely loaded.
*/

void
Cbuf_Execute_Sets (void)
{
	dstring_t  *buf = dstring_newstr ();

	while (strlen (cmd_consolebuffer->buffer->str)) {
		extract_line (cmd_consolebuffer->buffer, buf);
		if (!strncmp (buf->str, "set", 3) && isspace ((int) buf->str[3])) {
			Cmd_ExecuteString (buf->str, src_command);
		} else if (!strncmp (buf->str, "setrom", 6)
				   && isspace ((int) buf->str[6])) {
			Cmd_ExecuteString (buf->str, src_command);
		}
		dstring_clearstr (buf);
	}
	dstring_delete (buf);
}


/*
						SCRIPT COMMANDS
*/

/*
	Cmd_StuffCmds_f

	Adds command line parameters as script statements
	Commands lead with a +, and continue until a - or another +
	quake +prog jctest.qp +cmd amlev1
	quake -nosound +cmd amlev1
*/
void
Cmd_StuffCmds_f (void)
{
	int         i, j;
	int         s;
	char       *build, c;

	s = strlen (com_cmdline);
	if (!s)
		return;

	// pull out the commands
	build = malloc (s + 1);
	if (!build)
		Sys_Error ("Cmd_StuffCmds_f: Memory Allocation Failure\n");
	build[0] = 0;

	for (i = 0; i < s - 1; i++) {
		if (com_cmdline[i] == '+') {
			i++;

			for (j = i; !((com_cmdline[j] == '+')
						  || (com_cmdline[j] == '-'
							  && (j == 0 || com_cmdline[j - 1] == ' '))
						  || (com_cmdline[j] == 0)); j++)
				;

			c = com_cmdline[j];
			com_cmdline[j] = 0;

			strncat (build, com_cmdline + i, s - strlen (build));
			strncat (build, "\n", s - strlen (build));
			com_cmdline[j] = c;
			i = j - 1;
		}
	}

	// Sys_Printf("[\n%s]\n",build);

	if (build[0])
		Cbuf_InsertText (build);

	free (build);
}

void
Cmd_Exec_File (const char *path)
{
	char       *f;
	int         len;
	VFile      *file;

	if (!path || !*path)
		return;
	if ((file = Qopen (path, "r")) != NULL) {
		len = COM_filelength (file);
		f = (char *) malloc (len + 1);
		if (f) {
			f[len] = 0;
			Qread (file, f, len);
			Qclose (file);
			// Always insert into console
			Cbuf_InsertTextTo (cmd_consolebuffer, f);
			free (f);
		}
	}
}

void
Cmd_Exec_f (void)
{
	char       *f;
	int         mark;
	cmd_buffer_t *sub;

	if (Cmd_Argc () != 2) {
		Sys_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	mark = Hunk_LowMark ();
	f = (char *) COM_LoadHunkFile (Cmd_Argv (1));
	if (!f) {
		Sys_Printf ("couldn't exec %s\n", Cmd_Argv (1));
		return;
	}
	if (!Cvar_Command ()
		&& (cmd_warncmd->int_val || (developer && developer->int_val)))
		Sys_Printf ("execing %s\n", Cmd_Argv (1));
	sub = Cmd_NewBuffer (true);
	Cbuf_AddTextTo (sub, f);
	Hunk_FreeToLowMark (mark);
	Cmd_ExecuteSubroutine (sub);		// Execute file in it's own buffer
}

/*
	Cmd_Echo_f

	Just prints the rest of the line to the console
*/
void
Cmd_Echo_f (void)
{
	if (Cmd_Argc() == 2)
		Sys_Printf ("%s\n", Cmd_Argv(1));
	else
		Sys_Printf ("%s\n", Cmd_Args (1));
}

/*
	Cmd_Alias_f

	Creates a new command that executes a command string (possibly ; seperated)
*/
void
Cmd_Alias_f (void)
{
	cmdalias_t *alias;
	char       *cmd;
	int         i, c;
	const char *s;

	if (Cmd_Argc () == 1) {
		Sys_Printf ("Current alias commands:\n");
		for (alias = cmd_alias; alias; alias = alias->next)
			Sys_Printf ("alias %s \"%s\"\n", alias->name, alias->value);
		return;
	}

	s = Cmd_Argv (1);
	// if the alias already exists, reuse it
	alias = (cmdalias_t *) Hash_Find (cmd_alias_hash, s);
	if (alias) {
		free ((char *) alias->value);
	} else {
		cmdalias_t **a;

		alias = calloc (1, sizeof (cmdalias_t));
		if (!alias)
			Sys_Error ("Cmd_Alias_f: Memory Allocation Failure\n");
		alias->name = strdup (s);
		Hash_Add (cmd_alias_hash, alias);
		for (a = &cmd_alias; *a; a = &(*a)->next)
			if (strcmp ((*a)->name, alias->name) >= 0)
				break;
		alias->next = *a;
		*a = alias;
	}
	// copy the rest of the command line
	cmd = malloc (strlen (Cmd_Args (1)) + 2);	// can never be longer
	if (!cmd)
		Sys_Error ("Cmd_Alias_f: Memory Allocation Failure\n");
	cmd[0] = 0;							// start out with a null string
	c = Cmd_Argc ();
	for (i = 2; i < c; i++) {
		strcat (cmd, Cmd_Argv (i));
		if (i != c - 1)
			strcat (cmd, " ");
	}

	alias->value = cmd;
}

void
Cmd_UnAlias_f (void)
{
	cmdalias_t *alias;
	const char *s;

	if (Cmd_Argc () != 2) {
		Sys_Printf ("unalias <alias>: erase an existing alias\n");
		return;
	}

	s = Cmd_Argv (1);
	alias = Hash_Del (cmd_alias_hash, s);

	if (alias) {
		cmdalias_t **a;

		for (a = &cmd_alias; *a != alias; a = &(*a)->next)
			;
		*a = alias->next;

		free ((char *) alias->name);
		free ((char *) alias->value);
		free (alias);
	} else {
		Sys_Printf ("Unknown alias \"%s\"\n", s);
	}
}


/*
					COMMAND EXECUTION
*/

typedef struct cmd_function_s {
	struct cmd_function_s *next;
	const char *name;
	xcommand_t  function;
	const char *description;
} cmd_function_t;

static cmd_function_t *cmd_functions;	// possible commands to execute

int
Cmd_Argc (void)
{
	return cmd_activebuffer->argc;
}

const char *
Cmd_Argv (int arg)
{
	if (arg >= cmd_activebuffer->argc)
		return "";
	return cmd_activebuffer->argv[arg]->str;
}

/*
	Cmd_Args

	Returns a single string containing argv(start) to argv(argc()-1)
*/
const char *
Cmd_Args (int start)
{
	if (start >= cmd_activebuffer->argc)
		return "";
	return cmd_activebuffer->line->str + cmd_activebuffer->args[start];
}


int
Cmd_EndDoubleQuote (const char *str)
{
	int         i;

	for (i = 1; i < strlen (str); i++) {
		if (str[i] == '\"' && !escaped (str, i))
			return i;
	}
	return -1;							// Not found
}

int
Cmd_EndSingleQuote (const char *str)
{
	int         i;

	for (i = 1; i < strlen (str); i++) {
		if (str[i] == '\'' && !escaped (str, i))
			return i;
	}
	return -1;							// Not found
}

int
Cmd_EndBrace (const char *str)
{
	int         i, n;

	for (i = 1; i < strlen (str); i++) {
		if (str[i] == '{' && !escaped (str, i)) {
			n = Cmd_EndBrace (str + i);
			if (n < 0)
				return n;
			else
				i += n;
		} else if (str[i] == '\"' && !escaped (str, i)) {
			n = Cmd_EndDoubleQuote (str + i);
			if (n < 0)
				return n;
			else
				i += n;
		} else if (str[i] == '\'' && !escaped (str, i)) {
			n = Cmd_EndSingleQuote (str + i);
			if (n < 0)
				return n;
			else
				i += n;
		} else if (str[i] == '}' && !escaped (str, i))
			return i;
	}
	return -1;							// No matching brace found
}

int
Cmd_GetToken (const char *str)
{
	int         i;

	switch (*str) {
		case '\'':
			return Cmd_EndSingleQuote (str);
		case '\"':
			return Cmd_EndDoubleQuote (str);
		case '{':
			return Cmd_EndBrace (str);
		case '}':
			return -1;
		default:
			for (i = 0; i < strlen (str); i++)
				if (isspace (str[i]))
					break;
			return i;
	}
	return -1;							// We should never get here  
}

int         tag_shift = 0;
int         tag_special = 0;

struct stable_s {
	char        a, b;
} stable1[] = {
	{'f', 0x0D},							// Fake message

	{'[', 0x90},							// Gold braces
	{']', 0x91},

	{'(', 0x80},							// Scroll bar characters
	{'=', 0x81},
	{')', 0x82},
	{'|', 0x83},

	{'<', 0x9D},							// Vertical line characters
	{'-', 0x9E},
	{'>', 0x9F},

	{'.', 0x05},							// White dot

	{'#', 0x0B},							// White block

	{'a', 0x7F},							// White arrow.
	// DO NOT USE <a> WITH <b> TAG IN ANYTHING SENT TO SERVER.  PERIOD.
	{'A', 0x8D},							// Brown arrow

	{'0', 0x92},							// Golden numbers
	{'1', 0x93},
	{'2', 0x94},
	{'3', 0x95},
	{'4', 0x96},
	{'5', 0x97},
	{'6', 0x98},
	{'7', 0x99},
	{'8', 0x9A},
	{'9', 0x9B},
	{0, 0}
};

/*
	Cmd_ProcessTags
	
	Looks for html-like tags in a dstring and
	modifies the string accordingly

	FIXME:  This has become messy.  Create tag.[ch]
	and write a more generalized tag parser using
	callbacks
*/

void
Cmd_ProcessTags (dstring_t * dstr)
{
	int         close = 0, ignore = 0, i, n, c;

	for (i = 0; i < strlen (dstr->str); i++) {
		if (dstr->str[i] == '<' && !escaped (dstr->str, i)) {
			close = 0;
			for (n = 1;
				 dstr->str[i + n] != '>' || escaped (dstr->str, i + n);
				 n++)
				if (dstr->str[n] == 0)
					return;
			if (dstr->str[i + 1] == '/')
				close = 1;
			if (!strncmp (dstr->str + i + close + 1, "i", 1)) {
				if (ignore && !close)
					// If we are ignoring, ignore a non close
					continue;
				else if (close && ignore)
					// If we are closing, turn off ignore
					ignore--;
				else if (!close)
					ignore++;			// Otherwise, turn ignore on
			} else if (ignore)
				// If ignore isn't being changed and we are ignore, go on
				continue;
			else if (!strncmp (dstr->str + i + close + 1, "b", 1))
				tag_shift = close ? tag_shift - 1 : tag_shift + 1;
			else if (!strncmp (dstr->str + i + close + 1, "s", 1))
				tag_special = close ? tag_special - 1 : tag_special + 1;
			if (tag_shift < 0)
				tag_shift = 0;
			if (tag_special < 0)
				tag_special = 0;
			dstring_snip (dstr, i, n + 1);
			i--;
			continue;
		}
		c = dstr->str[i];
		/* This ignores escape characters, unless it is itself escaped */
		if (c == '\\' && !escaped (dstr->str, i))
			continue;
		if (tag_special) {
			for (n = 0; stable1[n].a; n++)
				if (c == stable1[n].a)
					c = dstr->str[i] = stable1[n].b;
		}
		if (tag_shift && c < 128)
			c = (dstr->str[i] += 128);
	}
}

/*
	Cmd_ProcessVariablesRecursive
	
	Looks for occurances of ${varname} and
	replaces them with the contents of the
	variable.  Will first replace occurances
	within itself.
*/

int
Cmd_ProcessVariablesRecursive (dstring_t * dstr, int start)
{
	dstring_t  *varname;
	cmd_localvar_t *lvar;
	cvar_t     *cvar;
	int         i, n;

	varname = dstring_newstr ();

	for (i = start + 2;; i++) {
		if (dstr->str[i] == '$' && dstr->str[i + 1] == '{'
			&& !escaped (dstr->str, i)) {
			n = Cmd_ProcessVariablesRecursive (dstr, i);
			if (n < 0) {
				break;
			} else {
				i += n - 1;
				continue;
			}
		} else if (dstr->str[i] == '}' && !escaped (dstr->str, i)) {
			dstring_clearstr (varname);
			dstring_insert (varname, dstr->str + start + 2, i - start - 2, 0);
			// Nuke it, even if no match is found
			dstring_snip (dstr, start, i - start + 1);
			lvar = (cmd_localvar_t *) Hash_Find (cmd_activebuffer->locals,
												 varname->str);
			if (lvar) {
				// Local variables get precedence
				dstring_insertstr (dstr, lvar->value->str, start);
				n = strlen (lvar->value->str);
			} else if ((cvar = Cvar_FindVar (varname->str))) {
				// Then cvars
				// Stick in the value of variable
				dstring_insertstr (dstr, cvar->string, start);
				n = strlen (cvar->string);
			} else
				n = 0;
			break;
		} else if (!dstr->str[i]) {		// No closing brace
			n = -1;
			break;
		}
	}
	dstring_delete (varname);
	return n;
}

int
Cmd_ProcessVariables (dstring_t * dstr)
{
	int         i, n;

	for (i = 0; i < strlen (dstr->str); i++) {
		if (dstr->str[i] == '$' && dstr->str[i + 1] == '{'
			&& !escaped (dstr->str, i)) {
			n = Cmd_ProcessVariablesRecursive (dstr, i);
			if (n < 0)
				return n;
			else
				i += n - 1;
		}
	}
	return 0;
}

int
Cmd_ProcessMath (dstring_t * dstr)
{
	dstring_t  *statement;
	int         i, n, paren;
	float       value;
	char       *temp;
	int         ret = 0;

	statement = dstring_newstr ();

	for (i = 0; i < strlen (dstr->str); i++) {
		if (dstr->str[i] == '$' && dstr->str[i + 1] == '('
			&& !escaped (dstr->str, i)) {
			paren = 1;
			for (n = 2;; n++) {
				if (dstr->str[i + n] == '(')
					paren++;
				else if (dstr->str[i + n] == ')') {
					paren--;
					if (!paren)
						break;
				} else if (!dstr->str[i + n]) {
					ret = -1;
					break;
				}
			}
			/* Copy text between parentheses into a buffer */
			dstring_clearstr (statement);
			dstring_insert (statement, dstr->str + i + 2, n - 2, 0);
			value = EXP_Evaluate (statement->str);
			if (EXP_ERROR == EXP_E_NORMAL) {
				temp = va ("%g", value);
				dstring_snip (dstr, i, n + 1);	// Nuke the statement
				dstring_insertstr (dstr, temp, i);	// Stick in the value
				i += strlen (temp) - 1;
			} else {
				ret = -2;
				Cmd_Error (va("Math error: invalid expression %s\n", statement->str));
				break;					// Math evaluation error
			}
		}
	}
	dstring_delete (statement);
	return ret;
}

/*
	Cmd_ProcessEscapes
	
	Looks for the escape character \ and
	removes it.  Special cases exist for
	\\ and \n; otherwise, it is simply
	filtered.  This should be the last
	step in the parser so that quotes,
	tags, etc. can be escaped
*/

void
Cmd_ProcessEscapes (dstring_t * dstr)
{
	int         i;

	for (i = 0; i < strlen (dstr->str); i++) {
		if (dstr->str[i] == '\\' && dstr->str[i + 1]) {
			dstring_snip (dstr, i, 1);
			if (dstr->str[i] == '\\')
				i++;
			if (dstr->str[i] == 'n')
				dstr->str[i] = '\n';
			i--;
		}
	}
}

/*
	Cmd_TokenizeString
	
	This takes a normal string, parses it
	into tokens, runs various filters on
	each token, and recombines them with the
	correct white space into a string for
	the purpose of executing a console
	command.  If the string begins with a \,
	filters are not run and the \ is stripped.
	Anything that stuffs commands into the
	console that requires absolute backwards
	compatibility should be changed to prepend
	it with a \.  An example of this is
	fullserverinfo, which requires that \
	be left alone since it is the delimeter
	for info keys.
*/

void
Cmd_TokenizeString (const char *text, qboolean filter)
{
	int         i = 0, len = 0, quotes, braces, space, res;
	const char *str = text;
	unsigned int cmd_argc = 0;

	dstring_clearstr (cmd_activebuffer->realline);
	dstring_appendstr (cmd_activebuffer->realline, text);

	/* Turn off tags at the beginning of a command. This causes tags to
	   continue past token boundaries. */
	tag_shift = 0;
	tag_special = 0;
	if (text[0] == '|')
		str++;
	dstring_clearstr (cmd_activebuffer->line);
	while (strlen (str + i)) {
		space = 0;
		while (isspace (str[i])) {
			i++;
			space++;
		}
		dstring_appendsubstr (cmd_activebuffer->line, str + i - space, space);
		len = Cmd_GetToken (str + i);
		if (len < 0) {
			Cmd_Error ("Parse error:  Unmatched quotes, braces, or "
					   "double quotes\n");
			cmd_activebuffer->argc = 0;
			return;
		} else if (len == 0)
			break;
		cmd_argc++;
		if (cmd_argc > cmd_activebuffer->maxargc) {
			cmd_activebuffer->argv = realloc (cmd_activebuffer->argv,
											  sizeof (dstring_t *) * cmd_argc);
			SYS_CHECKMEM (cmd_activebuffer->argv);

			cmd_activebuffer->args = realloc (cmd_activebuffer->args,
											  sizeof (int) * cmd_argc);
			SYS_CHECKMEM (cmd_activebuffer->args);

			cmd_activebuffer->argv[cmd_argc - 1] = dstring_newstr ();
			cmd_activebuffer->maxargc++;
		}
		dstring_clearstr (cmd_activebuffer->argv[cmd_argc - 1]);
		/* Remove surrounding quotes or double quotes or braces */
		quotes = 0;
		braces = 0;
		cmd_activebuffer->args[cmd_argc - 1] = strlen (cmd_activebuffer->line->str);
		if ((str[i] == '\'' && str[i + len] == '\'')
			|| (str[i] == '"' && str[i + len] == '"')) {
			dstring_appendsubstr (cmd_activebuffer->line, str + i, 1);
			dstring_appendsubstr (cmd_activebuffer->line, str + i, 1);
			i++;
			len -= 1;
			quotes = 1;
		}
		if (str[i] == '{' && str[i + len] == '}') {
			i++;
			len -= 1;
			braces = 1;
		}
		dstring_insert (cmd_activebuffer->argv[cmd_argc - 1], str + i, len, 0);
		if (filter && text[0] != '|') {
			if (!braces) {
				Cmd_ProcessTags (cmd_activebuffer->argv[cmd_argc - 1]);
				res =
					Cmd_ProcessVariables (cmd_activebuffer->argv[cmd_argc - 1]);
				if (res < 0) {
					Cmd_Error ("Parse error: Unmatched braces in "
							   "variable substitution expression.\n");
					cmd_activebuffer->argc = 0;
					return;
				}
				res = Cmd_ProcessMath (cmd_activebuffer->argv[cmd_argc - 1]);
				if (res == -1) {
					Cmd_Error ("Parse error:  Unmatched parenthesis\n");
					cmd_activebuffer->argc = 0;
					return;
				}
				if (res == -2) {
					cmd_activebuffer->argc = 0;
					return;
				}
			}
			Cmd_ProcessEscapes (cmd_activebuffer->argv[cmd_argc - 1]);
		}
		dstring_insertstr (cmd_activebuffer->line,
						   cmd_activebuffer->argv[cmd_argc - 1]->str,
						   strlen (cmd_activebuffer->line->str) - quotes);
		i += len + quotes + braces;		/* If we ended on a quote or brace,
										   skip it */
	}
	cmd_activebuffer->argc = cmd_argc;
}

void
Cmd_AddCommand (const char *cmd_name, xcommand_t function,
				const char *description)
{
	cmd_function_t *cmd;
	cmd_function_t **c;

	// fail if the command is a variable name
	if (Cvar_FindVar (cmd_name)) {
		Sys_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	// fail if the command already exists
	cmd = (cmd_function_t *) Hash_Find (cmd_hash, cmd_name);
	if (cmd) {
		Sys_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
		return;
	}

	cmd = malloc (sizeof (cmd_function_t));
	if (!cmd)
		Sys_Error ("Cmd_AddCommand: Memory_Allocation_Failure\n");
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->description = description;
	Hash_Add (cmd_hash, cmd);
	for (c = &cmd_functions; *c; c = &(*c)->next)
		if (strcmp ((*c)->name, cmd->name) >= 0)
			break;
	cmd->next = *c;
	*c = cmd;
}

qboolean
Cmd_Exists (const char *cmd_name)
{
	cmd_function_t *cmd;

	cmd = (cmd_function_t *) Hash_Find (cmd_hash, cmd_name);
	if (cmd) {
		return true;
	}

	return false;
}

const char *
Cmd_CompleteCommand (const char *partial)
{
	cmd_function_t *cmd;
	int         len;
	cmdalias_t *a;

	len = strlen (partial);

	if (!len)
		return NULL;

	// check for exact match
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!strcasecmp (partial, cmd->name))
			return cmd->name;
	for (a = cmd_alias; a; a = a->next)
		if (!strcasecmp (partial, a->name))
			return a->name;

	// check for partial match
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!strncasecmp (partial, cmd->name, len))
			return cmd->name;
	for (a = cmd_alias; a; a = a->next)
		if (!strncasecmp (partial, a->name, len))
			return a->name;

	return NULL;
}

/*
	Cmd_CompleteCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
*/
int
Cmd_CompleteCountPossible (const char *partial)
{
	cmd_function_t *cmd;
	int         len;
	int         h;

	h = 0;
	len = strlen (partial);

	if (!len)
		return 0;

	// Loop through the command list and count all partial matches
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!strncasecmp (partial, cmd->name, len))
			h++;

	return h;
}

/*
	Cmd_CompleteBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
*/
const char **
Cmd_CompleteBuildList (const char *partial)
{
	cmd_function_t *cmd;
	int         len = 0;
	int         bpos = 0;
	int         sizeofbuf;
	const char **buf;

	sizeofbuf = (Cmd_CompleteCountPossible (partial) + 1) * sizeof (char *);
	len = strlen (partial);
	buf = malloc (sizeofbuf + sizeof (char *));

	if (!buf)
		Sys_Error ("Cmd_CompleteBuildList: Memory Allocation Failure\n");
	// Loop through the alias list and print all matches
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!strncasecmp (partial, cmd->name, len))
			buf[bpos++] = cmd->name;

	buf[bpos] = NULL;
	return buf;
}

/*
	Cmd_CompleteAlias

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
*/
const char *
Cmd_CompleteAlias (const char *partial)
{
	cmdalias_t *alias;
	int         len;

	len = strlen (partial);

	if (!len)
		return NULL;

	// Check functions
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!strncasecmp (partial, alias->name, len))
			return alias->name;

	return NULL;
}

/*
	Cmd_CompleteAliasCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
*/
int
Cmd_CompleteAliasCountPossible (const char *partial)
{
	cmdalias_t *alias;
	int         len;
	int         h;

	h = 0;

	len = strlen (partial);

	if (!len)
		return 0;

	// Loop through the command list and count all partial matches
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!strncasecmp (partial, alias->name, len))
			h++;

	return h;
}

/*
	Cmd_CompleteAliasBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
*/
const char **
Cmd_CompleteAliasBuildList (const char *partial)
{
	cmdalias_t *alias;
	int         len = 0;
	int         bpos = 0;
	int         sizeofbuf = (Cmd_CompleteAliasCountPossible (partial) + 1) *
		sizeof (char *);
	const char **buf;

	len = strlen (partial);
	buf = malloc (sizeofbuf + sizeof (char *));

	if (!buf)
		Sys_Error ("Cmd_CompleteAliasBuildList: Memory Allocation Failure\n");
	// Loop through the alias list and print all matches
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!strncasecmp (partial, alias->name, len))
			buf[bpos++] = alias->name;

	buf[bpos] = NULL;
	return buf;
}

/*
	Cmd_ExecuteString

	A complete command line has been parsed, so try to execute it
*/
void
Cmd_ExecuteString (const char *text, cmd_source_t src)
{
	cmd_function_t *cmd;
	cmdalias_t *a;

	cmd_source = src;

	Cmd_TokenizeString (text, !cmd_activebuffer->legacy);

	// execute the command line
	if (!Cmd_Argc ())
		return;							// no tokens

	// check functions
	cmd = (cmd_function_t *) Hash_Find (cmd_hash, Cmd_Argv (0));
	if (cmd) {
		if (cmd->function)
			cmd->function ();
		return;
	}
	// Tonik: check cvars
	if (Cvar_Command ())
		return;

	// check alias
	a = (cmdalias_t *) Hash_Find (cmd_alias_hash, Cmd_Argv (0));
	if (a) {
		int i;
		cmd_buffer_t *sub; // Create a new buffer to execute the alias in
		sub = Cmd_NewBuffer (true);
		Cbuf_InsertTextTo (sub, a->value);
		for (i = 0; i < Cmd_Argc (); i++)
			Cmd_SetLocal (sub, va ("%i", i), Cmd_Argv (i));
		Cmd_SetLocal (sub, "#", va ("%i", Cmd_Argc ()));
		// This will handle freeing the buffer for us, leave it alone
		Cmd_ExecuteSubroutine (sub);
		return;
	}

	if (cmd_warncmd->int_val || developer->int_val)
		Sys_Printf ("Unknown command \"%s\"\n", Cmd_Argv (0));
}

/*
	Cmd_CheckParm

	Returns the position (1 to argc-1) in the command's argument list
	where the given parameter apears, or 0 if not present
*/
int
Cmd_CheckParm (const char *parm)
{
	int         i;

	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (!strcasecmp (parm, Cmd_Argv (i)))
			return i;

	return 0;
}

void
Cmd_CmdList_f (void)
{
	cmd_function_t *cmd;
	int         i;
	int         show_description = 0;

	if (Cmd_Argc () > 1)
		show_description = 1;
	for (cmd = cmd_functions, i = 0; cmd; cmd = cmd->next, i++) {
		if (show_description) {
			Sys_Printf ("%-20s :\n%s\n", cmd->name, cmd->description);
		} else {
			Sys_Printf ("%s\n", cmd->name);
		}
	}

	Sys_Printf ("------------\n%d commands\n", i);
}

void
Cmd_Help_f (void)
{
	const char *name;
	cvar_t     *var;
	cmd_function_t *cmd;

	if (Cmd_Argc () != 2) {
		Sys_Printf ("usage: help <cvar/command>\n");
		return;
	}

	name = Cmd_Argv (1);

	for (cmd = cmd_functions; cmd && strcasecmp (name, cmd->name);
		 cmd = cmd->next)
		;
	if (cmd) {
		Sys_Printf ("%s\n", cmd->description);
		return;
	}

	var = Cvar_FindVar (name);
	if (!var)
		var = Cvar_FindAlias (name);
	if (var) {
		Sys_Printf ("%s\n", var->description);
		return;
	}

	Sys_Printf ("variable/command not found\n");
}

/*
	Scripting commands
	
	The following functions are commands for enhanced scripting
	
*/

void
Cmd_If_f (void)
{
	long int    num;

	if (Cmd_Argc () < 3) {
		Sys_Printf ("Usage: if {condition} {commands}\n");
		return;
	}
	
	num = strtol (Cmd_Argv(1), 0, 10);
	
	if (!strcmp(Cmd_Argv(0), "ifnot"))
		num = !num;
	
	if (num)
		Cbuf_InsertText (Cmd_Argv (2));
	return;
}

void
Cmd_While_f (void) {
	cmd_buffer_t *sub;
	
	if (Cmd_Argc() < 3) {
		Sys_Printf("Usage: while {condition} {commands}\n");
		return;
	}
	
	sub = Cmd_NewBuffer (false);
	sub->locals = cmd_activebuffer->locals; // Use current local variables
	sub->loop = true;
	dstring_appendstr (sub->looptext, va("ifnot '%s' {break;};\n", Cmd_Argv(1)));
	dstring_appendstr (sub->looptext, Cmd_Argv(2));
	Cmd_ExecuteSubroutine (sub);
	return;
}

void
Cmd_Break_f (void) {
	if (cmd_activebuffer->loop) {
		cmd_activebuffer->loop = false;
		dstring_clearstr(cmd_activebuffer->buffer);
		return;
	}
	else {
		Cmd_Error("break command used outside of loop!\n");
		return;
	}
}

void
Cmd_Lset_f (void)
{
	if (Cmd_Argc () != 3) {
		Sys_Printf ("Usage: lset [local variable] [value]\n");
		return;
	}
	Cmd_SetLocal (cmd_activebuffer, Cmd_Argv (1), Cmd_Argv (2));
}

void
Cmd_Backtrace_f (void)
{
	Sys_Printf ("%s", cmd_backtrace->str);
}

void
Cmd_Hash_Stats_f (void)
{
	Sys_Printf ("alias hash table:\n");
	Hash_Stats (cmd_alias_hash);
	Sys_Printf ("command hash table:\n");
	Hash_Stats (cmd_hash);
}

static void
cmd_alias_free (void *_a, void *unused)
{
	cmdalias_t *a = (cmdalias_t *) _a;

	free ((char *) a->name);
	free ((char *) a->value);
	free (a);
}

static const char *
cmd_alias_get_key (void *_a, void *unused)
{
	cmdalias_t *a = (cmdalias_t *) _a;

	return a->name;
}

static const char *
cmd_get_key (void *c, void *unused)
{
	cmd_function_t *cmd = (cmd_function_t *) c;

	return cmd->name;
}

/*
	Cmd_Init_Hash

	initialise the command and alias hash tables
*/

void
Cmd_Init_Hash (void)
{
	cmd_hash = Hash_NewTable (1021, cmd_get_key, 0, 0);
	cmd_alias_hash = Hash_NewTable (1021, cmd_alias_get_key, cmd_alias_free, 0);
}

void
Cmd_Init (void)
{
	// register our commands
	Cmd_AddCommand ("stuffcmds", Cmd_StuffCmds_f, "Execute the commands given "
					"at startup again");
	Cmd_AddCommand ("exec", Cmd_Exec_f, "Execute a script file");
	Cmd_AddCommand ("echo", Cmd_Echo_f, "Print text to console");
	Cmd_AddCommand ("alias", Cmd_Alias_f, "Used to create a reference to a "
					"command or list of commands.\n"
					"When used without parameters, displays all current "
					"aliases.\n"
					"Note: Enclose multiple commands within quotes and "
					"seperate each command with a semi-colon.");
	Cmd_AddCommand ("unalias", Cmd_UnAlias_f, "Remove the selected alias");
	Cmd_AddCommand ("wait", Cmd_Wait_f, "Wait a game tic");
	Cmd_AddCommand ("cmdlist", Cmd_CmdList_f, "List all commands");
	Cmd_AddCommand ("help", Cmd_Help_f, "Display help for a command or "
					"variable");
	Cmd_AddCommand ("if", Cmd_If_f, "Conditionally execute a set of commands.");
	Cmd_AddCommand ("ifnot", Cmd_If_f, "Conditionally execute a set of commands if the condition is false.");
	Cmd_AddCommand ("while", Cmd_While_f, "Execute a set of commands while a condition is true.");
	Cmd_AddCommand ("break", Cmd_Break_f, "Break out of a loop.");
	Cmd_AddCommand ("lset", Cmd_Lset_f, "Sets the value of a local variable (not cvar).");
	Cmd_AddCommand ("backtrace", Cmd_Backtrace_f, "Show a description of the last GIB error and a backtrace.");
	//Cmd_AddCommand ("cmd_hash_stats", Cmd_Hash_Stats_f, "Display statistics "
	//				"alias and command hash tables");
	cmd_warncmd = Cvar_Get ("cmd_warncmd", "0", CVAR_NONE, NULL, "Toggles the "
							"display of error messages for unknown commands");
}

char       *com_token;
static size_t com_token_size;

static inline void
write_com_token (size_t pos, char c)
{
	if (pos + 1 <= com_token_size) {
	  write:
		com_token[pos] = c;
		return;
	}
	com_token_size = (pos + 1024) & ~1023;
	com_token = realloc (com_token, com_token_size);
	if (!com_token)
		Sys_Error ("COM_Parse: could not allocate %ld bytes",
				   (long) com_token_size);
	goto write;
}

/*
	COM_Parse

	Parse a token out of a string
	FIXME:  Does anything still need this crap?
*/
const char *
COM_Parse (const char *_data)
{
	const byte *data = (const byte *) _data;
	unsigned int c;
	size_t      len = 0;

	write_com_token (len, 0);

	if (!data)
		return NULL;

	// skip whitespace
  skipwhite:
	while ((c = *data) <= ' ') {
		if (c == 0)
			return NULL;				// end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/') {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	// handle quoted strings specially
	if (c == '\"') {
		data++;
		while (1) {
			c = *data++;
			if (c == '\"' || !c) {
				write_com_token (len, 0);
				return c ? data : data - 1;
			}
			write_com_token (len, c);
			len++;
		}
	}
	// parse a regular word
	do {
		write_com_token (len, c);
		data++;
		len++;

		c = *data;
	} while (c > 32);

	write_com_token (len, 0);
	return data;
}
