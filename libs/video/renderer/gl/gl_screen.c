/*
	gl_screen.c

	master for refresh, status bar, console, chat, notify, etc

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

#include <time.h>

#include "QF/console.h"
#include "QF/cvar.h"
#include "QF/draw.h"
#include "QF/quakefs.h"
#include "QF/render.h"
#include "QF/screen.h"
#include "QF/sys.h"
#include "QF/texture.h"
#include "QF/tga.h"
#include "QF/GL/defines.h"
#include "QF/GL/funcs.h"
#include "QF/GL/qf_rmain.h"
#include "QF/GL/qf_vid.h"

#include "compat.h"
#include "r_cvar.h"
#include "r_dynamic.h"
#include "r_local.h"
#include "r_screen.h"
#include "sbar.h"
#include "view.h"

int         glx, gly, glwidth, glheight;

/*
	SCR_CalcRefdef

	Must be called whenever vid changes
	Internal use only
*/
static void
SCR_CalcRefdef (void)
{
	float       size;
	int         h;
	qboolean    full = false;

	scr_fullupdate = 0;					// force a background redraw
	vid.recalc_refdef = 0;

	// force the status bar to redraw
	Sbar_Changed ();

	// bound viewsize
	Cvar_SetValue (scr_viewsize, bound (30, scr_viewsize->int_val, 120));

	// bound field of view
	Cvar_SetValue (scr_fov, bound (1, scr_fov->value, 170));

	if (scr_viewsize->int_val >= 120)
		sb_lines = 0;					// no status bar at all
	else if (scr_viewsize->int_val >= 110)
		sb_lines = 24;					// no inventory
	else
		sb_lines = 24 + 16 + 8;

	if (scr_viewsize->int_val >= 100) {
		full = true;
		size = 100.0;
	} else {
		size = scr_viewsize->int_val;
	}
	// intermission is always full screen
	if (r_force_fullscreen /* FIXME: better test */) {
		full = true;
		size = 100.0;
		sb_lines = 0;
	}
	size /= 100.0;

	h = vid.height - r_lineadj;

	r_refdef.vrect.width = vid.width * size + 0.5;
	if (r_refdef.vrect.width < 96) {
		size = 96.0 / r_refdef.vrect.width;
		r_refdef.vrect.width = 96;		// min for icons
	}

	r_refdef.vrect.height = vid.height * size + 0.5;
	if (r_refdef.vrect.height > h)
		r_refdef.vrect.height = h;
	r_refdef.vrect.x = (vid.width - r_refdef.vrect.width) / 2;
	if (full)
		r_refdef.vrect.y = 0;
	else
		r_refdef.vrect.y = (h - r_refdef.vrect.height) / 2;

	r_refdef.fov_x = scr_fov->value;
	r_refdef.fov_y =
		CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	scr_vrect = r_refdef.vrect;
}

/* SCREEN SHOTS */

tex_t *
SCR_ScreenShot (int width, int height)
{
	unsigned char *src, *dest, *snap;
	float          fracw, frach;
	int            count, dex, dey, dx, dy, nx, r, g, b, x, y, w, h;
	tex_t         *tex;

	snap = Hunk_TempAlloc (vid.width * vid.height * 3);

	qfglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE,
				    snap);

	w = (vid.width < width) ? vid.width : width;
	h = (vid.height < height) ? vid.height : height;

	fracw = (float) vid.width / (float) w;
	frach = (float) vid.height / (float) h;

	tex = malloc (field_offset (tex_t, data[w * h]));
	if (!tex)
		return 0;

	tex->width = w;
	tex->height = h;
	tex->palette = vid.palette;

	for (y = 0; y < h; y++) {
		dest = tex->data + (w * y);

		for (x = 0; x < w; x++) {
			r = g = b = 0;

			dx = x * fracw;
			dex = (x + 1) * fracw;
			if (dex == dx)
				dex++;					// at least one
			dy = y * frach;
			dey = (y + 1) * frach;
			if (dey == dy)
				dey++;					// at least one

			count = 0;
			for (; dy < dey; dy++) {
				src = snap + (vid.width * 3 * dy) + dx * 3;
				for (nx = dx; nx < dex; nx++) {
					r += *src++;
					g += *src++;
					b += *src++;
					count++;
				}
			}
			r /= count;
			g /= count;
			b /= count;
			*dest++ = MipColor (r, g, b);
		}
	}

	return tex;
}

void
SCR_ScreenShot_f (void)
{
	byte       *buffer;
	char        pcxname[MAX_OSPATH];

	// find a file name to save it to 
	if (!COM_NextFilename (pcxname, "qf", ".tga")) {
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a TGA file\n");
		return;
	}
	buffer = malloc (glwidth * glheight * 3);
	SYS_CHECKMEM (buffer);
	qfglReadPixels (glx, gly, glwidth, glheight, GL_BGR_EXT, GL_UNSIGNED_BYTE,
				  buffer);
	WriteTGAfile (pcxname, buffer, glwidth, glheight);
	free (buffer);
	Con_Printf ("Wrote %s\n", pcxname);
}

static void
SCR_TileClear (void)
{
	if (r_refdef.vrect.x > 0) {
		// left
		Draw_TileClear (0, 0, r_refdef.vrect.x, vid.height - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width, 0,
						vid.width - r_refdef.vrect.x + r_refdef.vrect.width,
						vid.height - sb_lines);
	}
	if (r_refdef.vrect.y > 0) {
		// top
		Draw_TileClear (r_refdef.vrect.x, 0,
						r_refdef.vrect.x + r_refdef.vrect.width,
						r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
						r_refdef.vrect.y + r_refdef.vrect.height,
						r_refdef.vrect.width,
						vid.height - sb_lines -
						(r_refdef.vrect.height + r_refdef.vrect.y));
	}
}

int		oldviewsize = 0;

/*
	SCR_UpdateScreen

	This is called every frame, and can also be called explicitly to flush
	text to the screen.

	WARNING: be very careful calling this from elsewhere, because the refresh
	needs almost the entire 256k of stack space!
*/
void
SCR_UpdateScreen (double realtime, SCR_Func *scr_funcs)
{
	double      time1 = 0, time2;

	if (block_drawing)
		return;

	GL_EndRendering ();

	r_realtime = realtime;

	vid.numpages = 2 + gl_triplebuffer->int_val;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (!scr_initialized)
		return;							// not initialized yet

	if (oldviewsize != scr_viewsize->int_val) {
		oldviewsize = scr_viewsize->int_val;
		vid.recalc_refdef = true;
	}

	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	if (r_speeds->int_val) {
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	if (oldfov != scr_fov->value) {		// determine size of refresh window
		oldfov = scr_fov->value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
		SCR_CalcRefdef ();

	// do 3D refresh drawing, and then update the screen
	V_RenderView ();

	SCR_SetUpToDrawConsole ();
	GL_Set2D ();

	// also makes polyblend apply to whole screen
	if (v_blend[3]) {
		qfglDisable (GL_TEXTURE_2D);
		qfglBegin (GL_QUADS);

		qfglColor4fv (v_blend);
		qfglVertex2f (0, 0);
		qfglVertex2f (vid.width, 0);
		qfglVertex2f (vid.width, vid.height);
		qfglVertex2f (0, vid.height);

		qfglEnd ();
		qfglColor3ubv (color_white);
		qfglEnable (GL_TEXTURE_2D);
	}

	// draw any areas not covered by the refresh
	SCR_TileClear ();

	if (r_force_fullscreen /*FIXME better test*/ == 1 && key_dest ==
		key_game) {
		Sbar_IntermissionOverlay ();
	} else if (r_force_fullscreen /*FIXME better test*/ == 2 &&
			   key_dest == key_game) {
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	} else {
		while (*scr_funcs) {
			(*scr_funcs)();
			scr_funcs++;
		}
	}

	if (r_speeds->int_val) {
//		qfglFinish ();
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly %4i parts\n",
					(int) ((time2 - time1) * 1000), c_brush_polys,
					c_alias_polys, numparticles);
	}

	GL_FlushText ();
	qfglFlush ();
}
