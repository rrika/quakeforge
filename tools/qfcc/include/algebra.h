/*
	algebra.h

	QC geometric algebra support code

	Copyright (C) 2023 Bill Currie <bill@taniwha.org>

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

#ifndef __algebra_h
#define __algebra_h

#include "QF/set.h"
#include "QF/progs/pr_comp.h"

typedef struct basis_blade_s {
	pr_uint_t   mask;				///< bit-mask of basis vectors
	int         scale;				///< 1, 0, or -1
} basis_blade_t;

typedef struct basis_group_s {
	int         count;
	pr_uint_t   group_mask;
	pr_uivec2_t range;
	basis_blade_t *blades;
	int        *map;
	set_t      *set;
} basis_group_t;

typedef struct basis_layout_s {
	int         count;
	pr_uivec2_t range;
	basis_group_t *groups;
	pr_ivec3_t *group_map;
	int        *mask_map;
	int         blade_count;
	set_t      *set;
} basis_layout_t;

typedef struct metric_s {
	pr_uint_t   plus;			///< mask of elements that square to +1
	pr_uint_t   minus;			///< mask of elements that square to -1
	pr_uint_t   zero;			///< mask of elements that square to  0
} metric_t;

typedef struct algebra_s {
	struct type_s *type;		///< underlying type (float or double)
	struct type_s *algebra_type;///< type for algebra
	metric_t    metric;
	basis_layout_t layout;
	basis_group_t *groups;
	struct type_s **mvec_types;
	struct symbol_s *mvec_sym;
	int         num_components;	///< number of componets (2^d)
	int         dimension;		///< number of dimensions (plus + minus + zero)
	int         plus;			///< number of elements squaring to +1
	int         minus;			///< number of elements squaring to -1
	int         zero;			///< number of elements squaring to 0
} algebra_t;

typedef struct multivector_s {
	int         num_components;
	int         group_mask;
	algebra_t  *algebra;
	struct symbol_s *mvec_sym;	///< null if single group
} multivector_t;

struct expr_s;
struct attribute_s;
bool is_algebra (const struct type_s *type) __attribute__((pure));
struct type_s *algebra_type (struct type_s *type, const struct expr_s *params);
struct type_s *algebra_subtype (struct type_s *type, struct attribute_s *attr);
struct type_s *algebra_mvec_type (algebra_t *algebra, pr_uint_t group_mask);
int algebra_count_flips (const algebra_t *alg, pr_uint_t a, pr_uint_t b) __attribute__((pure));
struct ex_value_s *algebra_blade_value (algebra_t *alg, const char *name);
struct symtab_s *algebra_scope (struct type_s *type, struct symtab_s *curscope);
void algebra_print_type_str (struct dstring_s *str, const struct type_s *type);
void algebra_encode_type (struct dstring_s *encoding,
						  const struct type_s *type);
int algebra_type_size (const struct type_s *type) __attribute__((pure));
int algebra_type_width (const struct type_s *type) __attribute__((pure));

int metric_apply (const metric_t *metric, pr_uint_t a, pr_uint_t b) __attribute__((pure));

algebra_t *algebra_get (const struct type_s *type) __attribute__((pure));
int algebra_type_assignable (const struct type_s *dst,
							 const struct type_s *src) __attribute__((pure));
struct type_s *algebra_base_type (const struct type_s *type) __attribute__((pure));
struct type_s *algebra_struct_type (const struct type_s *type) __attribute__((pure));
bool is_mono_grade (const struct type_s *type) __attribute__((pure));
int algebra_get_grade (const struct type_s *type) __attribute__((pure));
int algebra_blade_grade (basis_blade_t blade) __attribute__((const));

pr_uint_t get_group_mask (const struct type_s *type, algebra_t *algebra) __attribute__((pure));

const struct expr_s *algebra_binary_expr (int op, const struct expr_s *e1,
										  const struct expr_s *e2);
const struct expr_s *algebra_negate (const struct expr_s *e);
const struct expr_s *algebra_dual (const struct expr_s *e);
const struct expr_s *algebra_reverse (const struct expr_s *e);
const struct expr_s *algebra_cast_expr (struct type_s *dstType,
										const struct expr_s *e);
const struct expr_s *algebra_assign_expr (const struct expr_s *dst,
										  const struct expr_s *src);
const struct expr_s *algebra_field_expr (const struct expr_s *mvec,
										 const struct expr_s *field_name);
const struct expr_s *algebra_optimize (const struct expr_s *e);

const struct expr_s *mvec_expr (const struct expr_s *expr, algebra_t *algebra);
void mvec_scatter (const struct expr_s **components, const struct expr_s *mvec,
				   algebra_t *algebra);
const struct expr_s *mvec_gather (const struct expr_s **components,
								  algebra_t *algebra);



#endif//__algebra_h
