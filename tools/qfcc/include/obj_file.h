
/*
	obj_file.h

	object file support

	Copyright (C) 2001 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2002/6/16

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

#ifndef __obj_file_h
#define __obj_file_h

#define QFO_VERSION 0x00001001		// MMmmmRRR 0.001.001 (hex)

typedef struct qfo_header_s {
	char        qfo[4];
	int         version;

	int         code_ofs;
	int         code_size;

	int         data_ofs;
	int         data_size;

	int         far_data_ofs;
	int         far_data_size;

	int         strings_ofs;
	int         strings_size;

	int         relocs_ofs;
	int         num_relocs;

	int         defs_ofs;
	int         num_defs;

	int         functions_ofs;
	int         num_functions;
} qfo_header_t;

typedef struct qfo_def_s {
	etype_t     basic_type;
	string_t    full_type;
	string_t    name;
	int         ofs;

	int         refs;
	int         num_refs;

	unsigned    initialized:1;
	unsigned    constant:1;
	unsigned    global:1;
	unsigned    absolute:1;

	string_t    file;
	int         line;
} qfo_def_t;

typedef struct qfo_function_s {
	string_t    name;
	string_t    file;
	int         line;

	int         builtin;
	int         code;

	int         def;

	int         locals_size;
	int         local_defs;
	int         num_local_defs;

	int         line_info;
	int         nun_lines;

	int         num_parms;
	byte        parm_size[MAX_PARMS];
} qfo_function_t;

#endif//__obj_file_h
