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

	$Id$
*/

typedef struct gib_thread_s {
	unsigned long int id;
	struct cbuf_s *cbuf;
	struct gib_thread_s *next,*prev;
} gib_thread_t;

void GIB_Thread_Add (gib_thread_t *thread);
void GIB_Thread_Remove (gib_thread_t *thread);
gib_thread_t *GIB_Thread_Find (unsigned long int id);
gib_thread_t *GIB_Thread_New (void);
void GIB_Thread_Execute (void);
void GIB_Thread_Callback (const char *func, unsigned int argc, ...);
