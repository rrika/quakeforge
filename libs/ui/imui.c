/*
	imui.c

	Immediate mode user inferface

	Copyright (C) 2023 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2023/07/01

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
#include <stdlib.h>
#include <string.h>

#include "QF/ecs.h"
#include "QF/hash.h"
#include "QF/mathlib.h"
#include "QF/progs.h"
#include "QF/quakeio.h"

#include "QF/input/event.h"

#include "QF/ui/canvas.h"
#include "QF/ui/font.h"
#include "QF/ui/imui.h"
#include "QF/ui/text.h"

typedef struct imui_state_s {
	struct imui_state_s *next;
	struct imui_state_s **prev;
	char       *label;
	uint32_t    label_len;
	int         key_offset;
	uint32_t    frame_count;
	uint32_t    entity;
} imui_state_t;

struct imui_ctx_s {
	canvas_system_t csys;
	uint32_t    canvas;
	ecs_system_t vsys;
	text_system_t tsys;
	view_t      root_view;
	hashctx_t  *hashctx;
	hashtab_t  *tab;
	PR_RESMAP (imui_state_t) state_map;
	imui_state_t *states;
	font_t     *font;

	int64_t     frame_start;
	int64_t     frame_draw;
	int64_t     frame_end;
	uint32_t    frame_count;

	uint32_t    hot;
	uint32_t    active;
	bool        mouse_pressed;
	bool        mouse_released;
	unsigned    mouse_buttons;
	view_pos_t  mouse_position;
	unsigned    shift;
	int         key_code;
	int         unicode;
};

static imui_state_t *
imui_state_new (imui_ctx_t *ctx)
{
	imui_state_t *state = PR_RESNEW (ctx->state_map);
	*state = (imui_state_t) {
		.next = ctx->states,
		.prev = &ctx->states,
		.entity = nullent,
	};
	if (ctx->states) {
		ctx->states->prev = &state->next;
	}
	ctx->states = state;
	return state;
}

static void
imui_state_free (imui_ctx_t *ctx, imui_state_t *state)
{
	if (state->next) {
		state->next->prev = state->prev;
	}
	*state->prev = state->next;
	PR_RESFREE (ctx->state_map, state);
}

static imui_state_t *
imui_get_state (imui_ctx_t *ctx, const char *label)
{
	int         key_offset = 0;
	uint32_t    label_len = ~0u;
	const char *key = strstr (label, "##");
	if (key) {
		// key is '###': hash only past this
		if (key[2] == '#') {
			key_offset = (key += 3) - label;
		}
		label_len = key - label;
	}
	imui_state_t *state = Hash_Find (ctx->tab, label + key_offset);
	if (state) {
		state->frame_count = ctx->frame_count;
		return state;
	}
	state = imui_state_new (ctx);
	state->label = strdup (label);
	state->label_len = label_len == ~0u ? strlen (label) : label_len;
	state->key_offset = key_offset;
	state->frame_count = ctx->frame_count;
	Hash_Add (ctx->tab, state);
	return state;
}

static const char *
imui_state_getkey (const void *obj, void *data)
{
	auto state = (const imui_state_t *) obj;
	return state->label + state->key_offset;
}

imui_ctx_t *
IMUI_NewContext (canvas_system_t canvas_sys, const char *font, float fontsize)
{
	imui_ctx_t *ctx = malloc (sizeof (imui_ctx_t));
	uint32_t canvas;
	*ctx = (imui_ctx_t) {
		.csys = canvas_sys,
		.canvas = canvas = Canvas_New (canvas_sys),
		.vsys = { canvas_sys.reg, canvas_sys.view_base },
		.tsys = { canvas_sys.reg, canvas_sys.view_base, canvas_sys.text_base },
		.root_view = Canvas_GetRootView (canvas_sys, canvas),
		.hot = nullent,
		.active = nullent,
		.mouse_position = {-1, -1},
	};
	ctx->tab = Hash_NewTable (511, imui_state_getkey, 0, ctx, &ctx->hashctx);

	auto fpath = Font_SystemFont (font);
	if (fpath) {
		QFile *file = Qopen (fpath, "rb");
		if (file) {
			ctx->font = Font_Load (file, fontsize);
			//Qclose (file); FIXME closed by QFS_LoadFile
		}
		free (fpath);
	}
	return ctx;
}

void
IMUI_DestroyContext (imui_ctx_t *ctx)
{
	for (auto s = ctx->states; s; s = s->next) {
		free (s->label);
	}
	PR_RESDELMAP (ctx->state_map);

	if (ctx->font) {
		Font_Free (ctx->font);
	}

	Hash_DelTable (ctx->tab);
	Hash_DelContext (ctx->hashctx);
	free (ctx);
}

void
IMUI_SetVisible (imui_ctx_t *ctx, bool visible)
{
	*Canvas_Visible (ctx->csys, ctx->canvas) = visible;
}

void
IMUI_SetSize (imui_ctx_t *ctx, int xlen, int ylen)
{
	View_SetLen (ctx->root_view, xlen, ylen);
	View_UpdateHierarchy (ctx->root_view);
}

void
IMUI_ProcessEvent (imui_ctx_t *ctx, const IE_event_t *ie_event)
{
	if (ie_event->type == ie_mouse) {
		auto m = &ie_event->mouse;
		ctx->mouse_position = (view_pos_t) { m->x, m->y };

		unsigned old = ctx->mouse_buttons & 1;
		unsigned new = m->buttons & 1;
		ctx->mouse_pressed = (old ^ new) & new;
		ctx->mouse_released = (old ^ new) & !new;
		ctx->mouse_buttons = m->buttons;
	} else {
		auto k = &ie_event->key;
		//printf ("imui: %d %d %x\n", k->code, k->unicode, k->shift);
		ctx->shift = k->shift;
		ctx->key_code = k->code;
		ctx->unicode = k->unicode;
	}
}

void
IMUI_BeginFrame (imui_ctx_t *ctx)
{
	uint32_t    root_ent = ctx->root_view.id;
	Ent_RemoveComponent (root_ent, ctx->root_view.comp, ctx->root_view.reg);
	ctx->root_view = View_AddToEntity (root_ent, ctx->vsys, nullview);
	ctx->frame_start = Sys_LongTime ();
	ctx->frame_count++;
}

static void
prune_objects (imui_ctx_t *ctx)
{
	for (auto s = &ctx->states; *s; ) {
		if ((*s)->frame_count == ctx->frame_count) {
			s = &(*s)->next;
		} else {
			View_Delete (View_FromEntity (ctx->vsys, (*s)->entity));
			Hash_Del (ctx->tab, (*s)->label + (*s)->key_offset);
			imui_state_free (ctx, *s);
		}
	}
}

//FIXME currently works properly only for grav_northwest
static void
layout_objects (imui_ctx_t *ctx)
{
	auto ref = View_GetRef (ctx->root_view);
	auto h = ref->hierarchy;

	byte       *modified = h->components[view_modified];
	view_pos_t *pos = h->components[view_pos];
	view_pos_t *len = h->components[view_len];
	viewcont_t *cont = h->components[view_control];
	uint32_t   *parent = h->parentIndex;
	struct boolpair {
		bool x, y;
	}          down_depend[h->num_objects];

	// the root view size is always explicity
	down_depend[0] = (struct boolpair) { false, false };
	for (uint32_t i = 1; i < h->num_objects; i++) {
//		printf ("%d %d %d [%d %d] [%d %d]\n", i, parent[i], h->childCount[i],
//				pos[i].x, pos[i].y, len[i].x, len[i].y);
		if (cont[i].semantic_x == IMUI_SizeKind_ChildrenSum) {
			down_depend[i].x = 1;
		} else if (!(down_depend[i].x = down_depend[parent[i]].x)
				   && cont[i].semantic_x == IMUI_SizeKind_PercentOfParent) {
			int x = (len[parent[i]].x * 100) / 100;	//FIXME precent
			modified[i] |= len[i].x != x;
			len[i].x = x;
		}
		if (cont[i].semantic_y == IMUI_SizeKind_ChildrenSum) {
			down_depend[i].y = 1;
		} else if (!(down_depend[i].y = down_depend[parent[i]].y)
				   && cont[i].semantic_y == IMUI_SizeKind_PercentOfParent) {
			int y = (len[parent[i]].y * 100) / 100;	//FIXME precent
			modified[i] |= len[i].y != y;
			len[i].y = y;
		}
	}
	for (uint32_t i = h->num_objects; --i > 0; ) {
		view_pos_t  clen = len[i];
		if (cont[i].semantic_x == IMUI_SizeKind_ChildrenSum) {
			clen.x = 0;
			if (cont[i].vertical) {
				for (uint32_t j = 0; j < h->childCount[i]; j++) {
					uint32_t child = h->childIndex[i] + j;
					clen.x = max (clen.x, len[child].x);
				}
			} else {
				for (uint32_t j = 0; j < h->childCount[i]; j++) {
					uint32_t child = h->childIndex[i] + j;
					clen.x += len[child].x;
				}
			}
		}
		if (cont[i].semantic_y == IMUI_SizeKind_ChildrenSum) {
			clen.y = 0;
			if (!cont[i].vertical) {
				for (uint32_t j = 0; j < h->childCount[i]; j++) {
					uint32_t child = h->childIndex[i] + j;
					clen.y = max (clen.y, len[child].y);
				}
			} else {
				for (uint32_t j = 0; j < h->childCount[i]; j++) {
					uint32_t child = h->childIndex[i] + j;
					clen.y += len[child].y;
				}
			}
		}
		modified[i] |= (len[i].x != clen.x) | (len[i].y != clen.y);
		len[i] = clen;
	}

	view_pos_t  cpos = {};
	uint32_t    cur_parent = 0;
	for (uint32_t i = 1; i < h->num_objects; i++) {
		if (parent[i] != cur_parent) {
			cur_parent = parent[i];
			cpos = (view_pos_t) {};
		}
		if (cont[i].semantic_x != IMUI_SizeKind_Null
			&& cont[i].semantic_y != IMUI_SizeKind_Null) {
			modified[i] |= (pos[i].x != cpos.x) | (pos[i].y != cpos.y);
			pos[i] = cpos;
		} else if (cont[i].semantic_x != IMUI_SizeKind_Null) {
			modified[i] |= pos[i].x != cpos.x;
			pos[i].x = cpos.x;
		} else if (cont[i].semantic_y != IMUI_SizeKind_Null) {
			modified[i] |= pos[i].y != cpos.y;
			pos[i].y = cpos.y;
		}
		if (cont[parent[i]].vertical) {
			cpos.y += cont[i].semantic_y == IMUI_SizeKind_Null ? 0 : len[i].y;
		} else {
			cpos.x += cont[i].semantic_x == IMUI_SizeKind_Null ? 0 : len[i].x;
		}
	}

	View_UpdateHierarchy (ctx->root_view);
}

static void
check_inside (imui_ctx_t *ctx)
{
	auto ref = View_GetRef (ctx->root_view);
	auto h = ref->hierarchy;

	uint32_t   *entity = h->ent;
	view_pos_t *abs = h->components[view_abs];
	view_pos_t *len = h->components[view_len];
	viewcont_t *cont = h->components[view_control];
	auto mp = ctx->mouse_position;

	ctx->hot = nullent;
	for (uint32_t i = 0; i < h->num_objects; i++) {
		if (cont[i].active
			&& mp.x >= abs[i].x && mp.y >= abs[i].y
			&& mp.x < abs[i].x + len[i].x && mp.y < abs[i].y + len[i].y) {
			if (ctx->active == entity[i] || ctx->active == nullent) {
				ctx->hot = entity[i];
			}
		}
	}
	//printf ("check_inside: %8x %8x\n", ctx->hot, ctx->active);
}

void
IMUI_Draw (imui_ctx_t *ctx)
{
	ctx->frame_draw = Sys_LongTime ();

	prune_objects (ctx);
	layout_objects (ctx);
	check_inside (ctx);

	ctx->frame_end = Sys_LongTime ();
}

static bool
check_button_state (imui_ctx_t *ctx, uint32_t entity)
{
	bool result = false;
	//printf ("check_button_state: h:%8x a:%8x e:%8x\n", ctx->hot, ctx->active, entity);
	if (ctx->active == entity) {
		if (ctx->mouse_released) {
			result = ctx->hot == entity;
			ctx->active = nullent;
		}
	} else if (ctx->hot == entity) {
		if (ctx->mouse_pressed) {
			ctx->active = entity;
		}
	}
	return result;
}

static view_t
add_text (view_t view, imui_state_t *state, imui_ctx_t *ctx)
{
	uint32_t c_glyphs = ctx->csys.base + canvas_glyphs;
	uint32_t c_passage_glyphs = ctx->csys.text_base + text_passage_glyphs;
	auto     reg = ctx->csys.reg;

	auto text = Text_StringView (ctx->tsys, view, ctx->font,
								 state->label, state->label_len, 0, 0);

	int ascender = ctx->font->face->size->metrics.ascender / 64;
	int descender = ctx->font->face->size->metrics.descender / 64;
	auto len = View_GetLen (text);
	View_SetLen (text, len.x, ascender - descender);
	// text is positioned such that 0 is the baseline, and +y offset moves
	// the text down. The font's global ascender is used to find the baseline
	// relative to the top of the view.
	auto pos = View_GetPos (text);
	View_SetPos (text, pos.x, pos.y - len.y + ascender);
	View_SetGravity (text, grav_northwest);

	View_SetVisible (text, 1);
	Ent_SetComponent (text.id, c_glyphs, reg,
					  Ent_GetComponent (text.id, c_passage_glyphs, reg));

	len = View_GetLen (text);
	View_SetLen (view, len.x, len.y);

	return text;
}

static void
update_hot_active (imui_ctx_t *ctx, uint32_t old_entity, uint32_t new_entity)
{
	if (old_entity != nullent) {
		if (ctx->hot == old_entity) {
			ctx->hot = new_entity;
		}
		if (ctx->active == old_entity) {
			ctx->active = new_entity;
		}
	}
}

static void
set_fill (imui_ctx_t *ctx, view_t view, byte color)
{
	uint32_t c_fill = ctx->csys.base + canvas_fill;
	*(byte*) Ent_AddComponent (view.id, c_fill, ctx->csys.reg) = color;
}

static void
set_control (imui_ctx_t *ctx, view_t view, bool active)
{
	*View_Control (view) = (viewcont_t) {
		.gravity = grav_northwest,
		.visible = 1,
		.semantic_x = IMUI_SizeKind_Pixels,
		.semantic_y = IMUI_SizeKind_Pixels,
		.active = active,
	};
}

bool
IMUI_Button (imui_ctx_t *ctx, const char *label)
{
	auto state = imui_get_state (ctx, label);
	uint32_t old_entity = state->entity;

	auto view = View_New (ctx->vsys, ctx->root_view);
	state->entity = view.id;
	update_hot_active (ctx, old_entity, state->entity);

	set_control (ctx, view, true);
	set_fill (ctx, view, 0);
	add_text (view, state, ctx);

	return check_button_state (ctx, state->entity);
}

bool
IMUI_Checkbox (imui_ctx_t *ctx, bool *flag, const char *label)
{
	auto state = imui_get_state (ctx, label);
	uint32_t old_entity = state->entity;

	auto view = View_New (ctx->vsys, ctx->root_view);
	state->entity = view.id;
	update_hot_active (ctx, old_entity, state->entity);

	set_control (ctx, view, true);
	View_Control (view)->semantic_x = IMUI_SizeKind_ChildrenSum;
	View_Control (view)->semantic_y = IMUI_SizeKind_ChildrenSum;

	set_fill (ctx, view, 0);

	auto checkbox = View_New (ctx->vsys, view);
	set_control (ctx, checkbox, false);
	View_SetLen (checkbox, 20, 20);
	set_fill (ctx, checkbox, 0xfe);
	if (!*flag) {
		auto punch = View_New (ctx->vsys, checkbox);
		set_control (ctx, punch, false);
		View_SetGravity (punch, grav_center);
		View_SetLen (punch, 14, 14);
		set_fill (ctx, punch, 0);
	}

	auto text = View_New (ctx->vsys, view);
	set_control (ctx, text, false);
	add_text (text, state, ctx);

	if (check_button_state (ctx, state->entity)) {
		*flag = !*flag;
	}
	return *flag;
}

void
IMUI_Radio (imui_ctx_t *ctx, int *state, int value, const char *label)
{
}

void
IMUI_Slider (imui_ctx_t *ctx, float *value, float minval, float maxval,
			 const char *label)
{
}
