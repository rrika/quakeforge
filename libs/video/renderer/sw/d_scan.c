/*
	d_scan.c

	Portable C scan-level rasterization code, all pixel depths.

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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "QF/render.h"
#include "QF/ui/view.h"

#include "d_local.h"
#include "r_internal.h"
#include "vid_internal.h"
#include "vid_sw.h"

byte       *r_turb_pbase;
byte       *r_turb_pdest;
fixed16_t   r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
int        *r_turb_turb;
int         r_turb_spancount;

/*
	D_WarpScreen

	this performs a slight compression of the screen at the same time as
	the sine warp, to keep the edges from wrapping
*/
void
warp_screen_8 (void)
{
	int         w, h;
	int         u, v;
	int         scr_x = vr_data.scr_view->xpos;
	int         scr_y = vr_data.scr_view->ypos;
	int         scr_w = vr_data.scr_view->xlen;
	int         scr_h = vr_data.scr_view->ylen;
	byte       *dest;
	int        *turb;
	int        *col;
	byte      **row;

	/* FIXME: allocate these arrays properly */
	byte       *rowptr[MAXHEIGHT + AMP2 * 2];
	int         column[MAXWIDTH + AMP2 * 2];
	float       wratio, hratio;

	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;

	wratio = w / (float) scr_w;
	hratio = h / (float) scr_h;

	for (v = 0; v < scr_h + AMP2 * 2; v++) {
		rowptr[v] = d_viewbuffer + (r_refdef.vrect.y * screenwidth) +
			(screenwidth * (int) ((float) v * hratio * h / (h + AMP2 * 2)));
	}

	for (u = 0; u < scr_w + AMP2 * 2; u++) {
		column[u] = r_refdef.vrect.x +
			(int) ((float) u * wratio * w / (w + AMP2 * 2));
	}

	turb = intsintable + ((int) (vr_data.realtime * SPEED) & (CYCLE - 1));
	dest = ((byte*)vid.buffer) + scr_y * vid.rowbytes + scr_x;

	for (v = 0; v < scr_h; v++, dest += vid.rowbytes) {
		col = &column[turb[v]];
		row = &rowptr[v];
		for (u = 0; u < scr_w; u += 4) {
			dest[u + 0] = row[turb[u + 0]][col[u + 0]];
			dest[u + 1] = row[turb[u + 1]][col[u + 1]];
			dest[u + 2] = row[turb[u + 2]][col[u + 2]];
			dest[u + 3] = row[turb[u + 3]][col[u + 3]];
		}
	}
}

void
warp_screen_16 (void)
{
	int         w, h;
	int         u, v;
	int         scr_x = vr_data.scr_view->xpos;
	int         scr_y = vr_data.scr_view->ylen;
	int         scr_w = vr_data.scr_view->xpos;
	int         scr_h = vr_data.scr_view->ylen;
	short      *dest;
	int        *turb;
	int        *col;
	short     **row;
	short      *rowptr[MAXHEIGHT];
	int         column[MAXWIDTH];
	float       wratio, hratio;

	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;

	wratio = w / (float) scr_w;
	hratio = h / (float) scr_h;

	for (v = 0; v < scr_h + AMP2 * 2; v++) {
		rowptr[v] = (short *) d_viewbuffer +
			(r_refdef.vrect.y * screenwidth) +
			(screenwidth * (int) ((float) v * hratio * h /
								  (h + AMP2 * 2)));
	}

	for (u = 0; u < scr_w + AMP2 * 2; u++) {
		column[u] = r_refdef.vrect.x +
			(int) ((float) u * wratio * w / (w + AMP2 * 2));
	}

	turb = intsintable + ((int) (vr_data.realtime * SPEED) & (CYCLE - 1));
	dest = (short *) vid.buffer + scr_y * (vid.rowbytes >> 1) + scr_x;

	for (v = 0; v < scr_h; v++, dest += (vid.rowbytes >> 1)) {
		col = &column[turb[v]];
		row = &rowptr[v];
		for (u = 0; u < scr_w; u += 4) {
			dest[u + 0] = row[turb[u + 0]][col[u + 0]];
			dest[u + 1] = row[turb[u + 1]][col[u + 1]];
			dest[u + 2] = row[turb[u + 2]][col[u + 2]];
			dest[u + 3] = row[turb[u + 3]][col[u + 3]];
		}
	}
}

void
warp_screen_32 (void)
{
	int         w, h;
	int         u, v;
	int         scr_x = vr_data.scr_view->xpos;
	int         scr_y = vr_data.scr_view->ylen;
	int         scr_w = vr_data.scr_view->xpos;
	int         scr_h = vr_data.scr_view->ylen;
	int        *dest;
	int        *turb;
	int        *col;
	int       **row;
	int        *rowptr[MAXHEIGHT];
	int         column[MAXWIDTH];
	float       wratio, hratio;

	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;

	wratio = w / (float) scr_w;
	hratio = h / (float) scr_h;

	for (v = 0; v < scr_h + AMP2 * 2; v++) {
		rowptr[v] = (int *) d_viewbuffer +
			(r_refdef.vrect.y * screenwidth) +
			(screenwidth * (int) ((float) v * hratio * h /
								  (h + AMP2 * 2)));
	}

	for (u = 0; u < scr_w + AMP2 * 2; u++) {
		column[u] = r_refdef.vrect.x +
			(int) ((float) u * wratio * w / (w + AMP2 * 2));
	}

	turb = intsintable + ((int) (vr_data.realtime * SPEED) & (CYCLE - 1));
	dest = (int *) vid.buffer + scr_y * (vid.rowbytes >> 2) + scr_x;

	for (v = 0; v < scr_h; v++, dest += (vid.rowbytes >> 2)) {
		col = &column[turb[v]];
		row = &rowptr[v];
		for (u = 0; u < scr_w; u += 4) {
			dest[u + 0] = row[turb[u + 0]][col[u + 0]];
			dest[u + 1] = row[turb[u + 1]][col[u + 1]];
			dest[u + 2] = row[turb[u + 2]][col[u + 2]];
			dest[u + 3] = row[turb[u + 3]][col[u + 3]];
		}
	}
}

#ifdef PIC
#undef USE_INTEL_ASM //XXX asm pic hack
#endif

#ifndef USE_INTEL_ASM
void
draw_turbulent_span_8 (void)
{
	int         sturb, tturb;

	do {
		sturb =
			((r_turb_s + r_turb_turb[(r_turb_t >> 16) & (CYCLE - 1)]) >> 16) &
			63;
		tturb =
			((r_turb_t + r_turb_turb[(r_turb_s >> 16) & (CYCLE - 1)]) >> 16) &
			63;
		*r_turb_pdest++ = *(r_turb_pbase + (tturb << 6) + sturb);
		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
}
#endif // !USE_INTEL_ASM

void
draw_turbulent_span_16 (void)
{
	int         sturb, tturb;
	short *pdest = (short *) r_turb_pdest;

	do {
		sturb = ((r_turb_s + r_turb_turb[(r_turb_t >> 16) &
										 (CYCLE - 1)]) >> 16) & 63;
		tturb = ((r_turb_t + r_turb_turb[(r_turb_s >> 16) &
										 (CYCLE - 1)]) >> 16) & 63;
		*pdest++ = d_8to16table[r_turb_pbase[(tturb << 6) + sturb]];
		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
	r_turb_pdest = (byte *)pdest;
}

void
draw_turbulent_span_32 (void)
{
	int         sturb, tturb;
	int *pdest = (int *) r_turb_pdest;
	do {
		sturb = ((r_turb_s + r_turb_turb[(r_turb_t >> 16) &
										 (CYCLE - 1)]) >> 16) & 63;
			tturb = ((r_turb_t + r_turb_turb[(r_turb_s >> 16) &
											 (CYCLE - 1)]) >> 16) & 63;
			*pdest++ = d_8to24table[r_turb_pbase[(tturb << 6) + sturb]];
			r_turb_s += r_turb_sstep;
			r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
	r_turb_pdest = (byte *)pdest;
}

void
Turbulent (espan_t *pspan)
{
	int         count;
	fixed16_t   snext, tnext;
	float       sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float       sdivz16stepu, tdivz16stepu, zi16stepu;

	r_turb_turb = sintable + ((int) (vr_data.realtime * SPEED) & (CYCLE - 1));

	r_turb_sstep = 0;					// keep compiler happy
	r_turb_tstep = 0;					// ditto

	r_turb_pbase = (byte *) cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	do {
		r_turb_pdest = (byte *) d_viewbuffer + (screenwidth * pspan->v) +
												pspan->u;

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float) pspan->u;
		dv = (float) pspan->v;

		sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
		tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
		zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
		z = (float) 0x10000 / zi;		// prescale to 16.16 fixed-point

		r_turb_s = (int) (sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int) (tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do {
			// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count) {
				// calculate s/z, t/z, zi->fixed s and t at far end of span,
				// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float) 0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int) (sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;			// prevent round-off error on <0
										// steps from
				// from causing overstepping & running off the
				// edge of the texture

				tnext = (int) (tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;			// guard against round-off error on
										// <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			} else {
				// calculate s/z, t/z, zi->fixed s and t at last pixel in
				// span (so can't step off polygon), clamp, calculate s and t
				// steps across span by division, biasing steps low so we
				// don't run off the texture
				spancountminus1 = (float) (r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float) 0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int) (sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;			// prevent round-off error on <0 steps
										// from causing overstepping & running
										// off the edge of the texture

				tnext = (int) (tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;			// guard against round-off error on
										// <0 steps

				if (r_turb_spancount > 1) {
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE << 16) - 1);
			r_turb_t = r_turb_t & ((CYCLE << 16) - 1);

			sw_ctx->draw->draw_turbulent_span ();

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}

#ifndef USE_INTEL_ASM
void
draw_spans_8 (espan_t *pspan)
{
	int         count, spancount;
	unsigned char *pbase, *pdest;
	fixed16_t   s, t, snext, tnext, sstep, tstep;
	float       sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float       sdivz8stepu, tdivz8stepu, zi8stepu;

	sstep = 0;							// keep compiler happy
	tstep = 0;							// ditto

	pbase = (unsigned char *) cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

	do {
		pdest = (unsigned char *) ((byte *) d_viewbuffer +
								   (screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float) pspan->u;
		dv = (float) pspan->v;

		sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
		tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
		zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
		z = (float) 0x10000 / zi;		// prescale to 16.16 fixed-point

		s = (int) (sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int) (tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		do {
			// calculate s and t at the far end of the span
			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;

			if (count) {
				// calculate s/z, t/z, zi->fixed s and t at far end of span,
				// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = (float) 0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int) (sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;			// prevent round-off error on <0
										// steps from
				// from causing overstepping & running off the
				// edge of the texture

				tnext = (int) (tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;			// guard against round-off error on
										// <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;
			} else {
				// calculate s/z, t/z, zi->fixed s and t at last pixel in span
				// (so can't step off polygon), clamp, calculate s and t steps
				// across span by division, biasing steps low so we don't run
				// off the texture
				spancountminus1 = (float) (spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float) 0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int) (sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;			// prevent round-off error on <0 steps
										// from from causing overstepping &
										// running off the edge of the texture

				tnext = (int) (tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;			// guard against round-off error on
										// <0 steps

				if (spancount > 1) {
					sstep = (snext - s) / (spancount - 1);
					tstep = (tnext - t) / (spancount - 1);
				}
			}

			do {
				*pdest++ = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
				s += sstep;
				t += tstep;
			} while (--spancount > 0);

			s = snext;
			t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}
#endif

void
draw_spans_16 (espan_t *pspan)
{
	short      *pbase = (short *) cacheblock, *pdest;
	int         count;
	fixed16_t   s, t, snext, tnext, sstep, tstep;
	float       sdivz, tdivz, zi, z, du, dv;
	float       sdivz8stepu, tdivz8stepu, zi8stepu;

	sstep = 0;							// keep compiler happy
	tstep = 0;							// ditto

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8 * 65536;

	do {
		pdest = (short *) d_viewbuffer + (screenwidth * pspan->v) +
			pspan->u;

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float) pspan->u;
		dv = (float) pspan->v;

		sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
		tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
		zi = (d_ziorigin + dv * d_zistepv + du * d_zistepu) * 65536.0f;
		z = d_zitable[(unsigned short) zi];

		s = (int) (sdivz * z) + sadjust;
		s = bound(0, s, bbextents);
		t = (int) (tdivz * z) + tadjust;
		t = bound(0, t, bbextentt);

		while(count >= 8) {
			count -= 8;
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			sdivz += sdivz8stepu;
			tdivz += tdivz8stepu;
			zi += zi8stepu;
			z = d_zitable[(unsigned short) zi];

			// prevent round-off error on <0 steps from from causing
			// overstepping & running off the edge of the texture
			snext = (int) (sdivz * z) + sadjust;
			snext = bound(8, snext, bbextents);
			tnext = (int) (tdivz * z) + tadjust;
			tnext = bound(8, tnext, bbextentt);

			sstep = (snext - s) >> 3;
			tstep = (tnext - t) >> 3;

			pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[2] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[3] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[4] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[5] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[6] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[7] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s = snext;t = tnext;
			pdest += 8;
		}
		if (count)
		{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span
			// (so can't step off polygon), clamp, calculate s and t steps
			// across span by division, biasing steps low so we don't run
			// off the texture
			//countminus1 = (float) (count - 1);
			sdivz += d_sdivzstepu * count; //minus1;
			tdivz += d_tdivzstepu * count; //minus1;
			zi += d_zistepu * 65536.0f * count; //minus1;
			z = d_zitable[(unsigned short) zi];

			// prevent round-off error on <0 steps from from causing
			// overstepping & running off the edge of the texture
			snext = (int) (sdivz * z) + sadjust;
			snext = bound(count, snext, bbextents);
			tnext = (int) (tdivz * z) + tadjust;
			tnext = bound(count, tnext, bbextentt);

			if (count > 1) {
				sstep = (snext - s) / count; //(count - 1);
				tstep = (tnext - t) / count; //(count - 1);

				if (count & 4)
				{
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[2] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[3] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;t += tstep;
					pdest += 4;
				}
				if (count & 2)
				{
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest += 2;
				}
				if (count & 1)
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			}
			else
			{
				pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			}
		}
	} while ((pspan = pspan->pnext) != NULL);
}

void
draw_spans_32 (espan_t *pspan)
{
	int        *pbase = (int *) cacheblock, *pdest;
	int         count;
	fixed16_t   s, t, snext, tnext, sstep, tstep;
	float       sdivz, tdivz, zi, z, du, dv;
	float       sdivz8stepu, tdivz8stepu, zi8stepu;

	sstep = 0;							// keep compiler happy
	tstep = 0;							// ditto

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8 * 65536;

	do {
		pdest = (int *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u;

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float) pspan->u;
		dv = (float) pspan->v;

		sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
		tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
		zi = (d_ziorigin + dv * d_zistepv + du * d_zistepu) * 65536.0f;
		z = d_zitable[(unsigned short) zi];

		s = (int) (sdivz * z) + sadjust;
		s = bound(0, s, bbextents);
		t = (int) (tdivz * z) + tadjust;
		t = bound(0, t, bbextentt);

		while(count >= 8) {
			count -= 8;
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			sdivz += sdivz8stepu;
			tdivz += tdivz8stepu;
			zi += zi8stepu;
			z = d_zitable[(unsigned short) zi];

			// prevent round-off error on <0 steps from from causing
			// overstepping & running off the edge of the texture
			snext = (int) (sdivz * z) + sadjust;
			snext = bound(8, snext, bbextents);
			tnext = (int) (tdivz * z) + tadjust;
			tnext = bound(8, tnext, bbextentt);

			sstep = (snext - s) >> 3;
			tstep = (tnext - t) >> 3;

			pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[2] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[3] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[4] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[5] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[6] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s += sstep;
			t += tstep;
			pdest[7] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			s = snext;
			t = tnext;
			pdest += 8;
		}
		if (count)
		{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span
			// (so can't step off polygon), clamp, calculate s and t steps
			// across span by division, biasing steps low so we don't run
			// off the texture
			//countminus1 = (float) (count - 1);
			sdivz += d_sdivzstepu * count; //minus1;
			tdivz += d_tdivzstepu * count; //minus1;
			zi += d_zistepu * 65536.0f * count; //minus1;
			z = d_zitable[(unsigned short) zi];

			// prevent round-off error on <0 steps from from causing
			// overstepping & running off the edge of the texture
			snext = (int) (sdivz * z) + sadjust;
			snext = bound(count, snext, bbextents);
			tnext = (int) (tdivz * z) + tadjust;
			tnext = bound(count, tnext, bbextentt);

			if (count > 1) {
				sstep = (snext - s) / count; //(count - 1);
				tstep = (tnext - t) / count; //(count - 1);

				if (count & 4)
				{
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[2] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[3] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest += 4;
				}
				if (count & 2)
				{
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest[1] = pbase[(t >> 16) * cachewidth + (s >> 16)];
					s += sstep;
					t += tstep;
					pdest += 2;
				}
				if (count & 1)
					pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			}
			else
			{
				pdest[0] = pbase[(t >> 16) * cachewidth + (s >> 16)];
			}
		}
	} while ((pspan = pspan->pnext) != NULL);
}

#ifndef USE_INTEL_ASM
void
D_DrawZSpans (espan_t *pspan)
{
	int         count, doublecount, izistep;
	int         izi;
	short      *pdest;
	unsigned int ltemp;
	double      zi;
	float       du, dv;

	// FIXME: check for clamping/range problems
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int) (d_zistepu * 0x8000 * 0x10000);

	do {
		pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

		// calculate the initial 1/z
		du = (float) pspan->u;
		dv = (float) pspan->v;

		zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
		// we count on FP exceptions being turned off to avoid range problems
		izi = (int) (zi * 0x8000 * 0x10000);

		if ((intptr_t) pdest & 0x02) {
			*pdest++ = (short) (izi >> 16);
			izi += izistep;
			count--;
		}

		if ((doublecount = count >> 1) > 0) {
			do {
				ltemp = izi >> 16;
				izi += izistep;
				ltemp |= izi & 0xFFFF0000;
				izi += izistep;
				*(int *) pdest = ltemp;
				pdest += 2;
			} while (--doublecount > 0);
		}

		if (count & 1)
			*pdest = (short) (izi >> 16);

	} while ((pspan = pspan->pnext) != NULL);
}
#endif
