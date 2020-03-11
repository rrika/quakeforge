/*
	expr_compound.c

	compound intializer expression construction and manipulations

	Copyright (C) 2020 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2020/03/11

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
#include <stdlib.h>

#include "QF/alloc.h"
#include "QF/dstring.h"
#include "QF/mathlib.h"
#include "QF/sys.h"
#include "QF/va.h"

#include "diagnostic.h"
#include "expr.h"
#include "options.h"
#include "symtab.h"
#include "type.h"

static element_t *elements_freelist;

element_t *
new_element (expr_t *expr, symbol_t *symbol)
{
	element_t  *element;
	ALLOC (256, element_t, elements, element);
	element->expr = expr;
	element->symbol = symbol;
	return element;
}

static element_t *
append_init_element (element_chain_t *element_chain, element_t *element)
{
	element->next = 0;
	*element_chain->tail = element;
	element_chain->tail = &element->next;
	return element;
}

expr_t *
new_compound_init (void)
{
	expr_t     *c = new_expr ();
	c->type = ex_compound;
	c->e.compound.head = 0;
	c->e.compound.tail = &c->e.compound.head;
	return c;
}

void
build_element_chain (element_chain_t *element_chain, type_t *type,
					 expr_t *eles, int base_offset)
{
	element_t  *ele = eles->e.compound.head;

	if (is_array (type)) {
		type_t     *array_type = type->t.array.type;
		int         array_size = type->t.array.size;
		int         i;

		for (i = 0; i < array_size; i++) {
			int         offset = base_offset + i * type_size (array_type);
			if (ele && ele->expr && ele->expr->type == ex_compound) {
				build_element_chain (element_chain, array_type,
									 ele->expr, offset);
			} else {
				element_t  *element = new_element (0, 0);
				element->type = array_type;
				element->offset = offset;
				element->expr = ele ? ele->expr : 0;	// null -> nil
				append_init_element (element_chain, element);
			}
			if (ele) {
				ele = ele->next;
			}
		}
	} else if (is_struct (type) || is_vector (type) || is_quaternion (type)) {
		symtab_t   *symtab = type->t.symtab;
		symbol_t   *field;

		for (field = symtab->symbols; field; field = field->next) {
			int         offset = base_offset + field->s.offset;
			if (field->sy_type != sy_var
				|| field->visibility == vis_anonymous) {
				continue;
			}
			if (ele && ele->expr && ele->expr->type == ex_compound) {
				build_element_chain (element_chain, field->type,
									 ele->expr, offset);
			} else {
				element_t  *element = new_element (0, 0);
				element->type = field->type;
				element->offset = offset;
				element->expr = ele ? ele->expr : 0;	// null -> nil
				append_init_element (element_chain, element);
			}
			if (ele) {
				ele = ele->next;
			}
		}
	} else {
		error (eles, "invalid initializer");
	}
	if (ele && ele->next && options.warnings.initializer) {
		warning (eles, "excessive elements in initializer");
	}
}

void free_element_chain (element_chain_t *element_chain)
{
	*element_chain->tail = elements_freelist;
	elements_freelist = element_chain->head;
	element_chain->head = 0;
	element_chain->tail = &element_chain->head;
}

expr_t *
append_element (expr_t *compound, element_t *element)
{
	if (compound->type != ex_compound) {
		internal_error (compound, "not a compound expression");
	}

	if (!element || (element->expr && element->expr->type == ex_error)) {
		return compound;
	}

	if (element->next) {
		internal_error (compound, "append_element: element loop detected");
	}
	append_init_element (&compound->e.compound, element);
	return compound;
}

void
assign_elements (expr_t *local_expr, expr_t *init,
				 element_chain_t *element_chain)
{
	element_t  *element;

	for (element = element_chain->head; element; element = element->next) {
		int         offset = element->offset;
		type_t     *type = element->type;
		expr_t     *alias = new_offset_alias_expr (type, init, offset);

		expr_t     *c;

		if (element->expr) {
			c = constant_expr (element->expr);
		} else {
			c = new_nil_expr ();
		}
		if (c->type == ex_nil) {
			c = convert_nil (c, type);
		}
		append_expr (local_expr, assign_expr (alias, c));
	}
}

expr_t *
initialized_temp_expr (type_t *type, expr_t *compound)
{
	element_chain_t element_chain;
	expr_t     *temp = new_temp_def_expr (type);
	expr_t     *block = new_block_expr ();

	element_chain.head = 0;
	element_chain.tail = &element_chain.head;
	build_element_chain (&element_chain, type, compound, 0);
	assign_elements (block, temp, &element_chain);
	block->e.block.result = temp;
	free_element_chain (&element_chain);
	return block;
}
