/*
	vorbis.c

	ogg/vorbis support

	Copyright (C) 2001 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2002/6/14

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

static __attribute__ ((unused)) const char rcsid[] =
	"$Id$";

#ifdef HAVE_VORBIS

#ifdef HAVE_STRING_H
# include "string.h"
#endif
#ifdef HAVE_STRINGS_H
# include "strings.h"
#endif

#include <stdlib.h>
#include <vorbis/vorbisfile.h>

#include "QF/cvar.h"
#include "QF/quakefs.h"
#include "QF/sound.h"
#include "QF/sys.h"

#include "snd_render.h"

static size_t
read_func (void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return Qread (datasource, ptr, size * nmemb);
}

static int
seek_func (void *datasource, ogg_int64_t offset, int whence)
{
	return Qseek (datasource, offset, whence);
}

static int
close_func (void *datasource)
{
	Qclose (datasource);
	return 0;
}

static long
tell_func (void *datasource)
{
	return Qtell (datasource);
}

static ov_callbacks callbacks = {
	read_func,
	seek_func,
	close_func,
	tell_func,
};

static wavinfo_t
get_info (OggVorbis_File *vf)
{
	vorbis_info *vi;
	int         sample_start = -1, sample_count = 0;
	int         samples;
	char      **ptr;
	wavinfo_t   info;

	vi = ov_info (vf, -1);
	samples = ov_pcm_total (vf, -1);

	for (ptr = ov_comment (vf, -1)->user_comments; *ptr; ptr++) {
		Sys_DPrintf ("%s\n", *ptr);
		if (strncmp ("CUEPOINT=", *ptr, 9) == 0) {
			sscanf (*ptr + 9, "%d %d", &sample_start, &sample_count);
		}
	}

	if (sample_start != -1)
		samples = sample_start + sample_count;

	info.rate = vi->rate;
	info.width = 2;
	info.channels = vi->channels;
	info.loopstart = sample_start;
	info.samples = samples;
	info.dataofs = 0;
	info.datalen = samples * 2;

	if (developer->int_val) {
		Sys_Printf ("\nBitstream is %d channel, %dHz\n",
					info.channels, info.rate);
		Sys_Printf ("\nDecoded length: %d samples (%d bytes)\n",
					info.samples, info.samples * info.channels * 2);
		Sys_Printf ("Encoded by: %s\n\n", ov_comment (vf, -1)->vendor);
	}

	return info;
}

static int
read_ogg (OggVorbis_File *vf, byte *buf, int len)
{
	int         count = 0;
	int         current_section;

	while (len) {
		int         res = ov_read (vf, buf, len, 0, 2, 1, &current_section);
		if (res > 0) {
			count += res;
			len -= res;
			buf += res;
		} else if (res < 0) {
			Sys_Printf ("vorbis error %d\n", res);
			return -1;
		} else {
			Sys_Printf ("unexpected eof\n");
			break;
		}
	}
	return count;
}

static sfxbuffer_t *
load_ogg (OggVorbis_File *vf, sfxblock_t *block, cache_allocator_t allocator)
{
	byte       *data;
	sfxbuffer_t *sc = 0;
	sfx_t      *sfx = block->sfx;
	void       (*resample)(sfxbuffer_t *, byte *, int);
	wavinfo_t  *info = &block->wavinfo;

	switch (info->channels) {
		case 1:
			resample = SND_ResampleMono;
			break;
		case 2:
			resample = SND_ResampleStereo;
			break;
		default:
			Sys_Printf ("%s: unsupported channel count: %d\n",
						sfx->name, info->channels);
			return 0;
	}

	data = malloc (info->datalen);
	if (!data)
		goto bail;
	sc = SND_GetCache (info->samples, info->rate, info->width, info->channels,
					   block, allocator);
	if (!sc)
		goto bail;
	sc->sfx = sfx;
	if (read_ogg (vf, data, info->datalen) < 0)
		goto bail;
	resample (sc, data, info->samples);
	sc->length = sc->head = sfx->length;
  bail:
	if (data)
		free (data);
	ov_clear (vf);
	return sc;
}

static void
ogg_callback_load (void *object, cache_allocator_t allocator)
{
	QFile      *file;
	OggVorbis_File vf;

	sfxblock_t *block = (sfxblock_t *) object;
	
	QFS_FOpenFile (block->file, &file);
	if (!file)
		return; //FIXME Sys_Error?

	if (ov_open_callbacks (file, &vf, 0, 0, callbacks) < 0) {
		Sys_Printf ("Input does not appear to be an Ogg bitstream.\n");
		Qclose (file);
		return; //FIXME Sys_Error?
	}
	load_ogg (&vf, block, allocator);
}

static void
cache_ogg (sfx_t *sfx, char *realname, OggVorbis_File *vf, wavinfo_t info)
{
	sfxblock_t *block = calloc (1, sizeof (sfxblock_t));
	ov_clear (vf);
	sfx->data = block;
	sfx->wavinfo = SND_CacheWavinfo;
	sfx->touch = SND_CacheTouch;
	sfx->retain = SND_CacheRetain;
	sfx->release = SND_CacheRelease;

	block->sfx = sfx;
	block->file = realname;
	block->wavinfo = info;

	Cache_Add (&block->cache, block, ogg_callback_load);
}

static void
fill_buffer (sfxbuffer_t *buffer, int count)
{
	byte        data[65536];
	float       stepscale;
	int         bps, bytes, insamples, outsamples;
	sfx_t      *sfx = buffer->sfx;
	sfxstream_t *stream = (sfxstream_t *) sfx->data;
	wavinfo_t  *info = &stream->wavinfo;

	stepscale = (float) info->rate / shm->speed;
	bps = info->width * info->channels;
	insamples = sizeof (data) / bps;
	outsamples = insamples / stepscale;

	bytes = count * bps * stepscale;

	while (bytes > sizeof (data)) {
		read_ogg (stream->file, data, sizeof (data));
		stream->resample (buffer, data, insamples);
		buffer->head += outsamples;
		count -= outsamples;
		bytes -= sizeof (data);
	}

	if (bytes) {
		int         n = bytes / bps;
		read_ogg (stream->file, data, bytes);
		stream->resample (buffer, data, n);
		buffer->head += count;
	}
}

static void
ogg_advance (sfxbuffer_t *buffer, int count)
{
	int         headpos, samples;
	int         post_count = 0;
	sfx_t      *sfx = buffer->sfx;
	sfxstream_t *stream = (sfxstream_t *) sfx->data;
	wavinfo_t  *info = &stream->wavinfo;

	// find out how many samples the buffer currently holds
	samples = buffer->head - buffer->tail;
	if (samples < 0)
		samples += buffer->length;

	headpos = buffer->pos + samples;

	if (info->loopstart == -1) {
		// unlooped sound
		if (headpos == sfx->length)
			return;					// at end of sample, nothing to do
		if (headpos + count > sfx->length)
			count = sfx->length - headpos;	// only advance to end of sample
	} else {
		// looped sound
		if (headpos > sfx->length) {
			// already handled the loop, nothing to worry about
		} else {
			if (headpos + count > sfx->length) {
				post_count = headpos + count - sfx->length;
			}
		}
	}

	buffer->pos += count;
	if (samples < count) {
		buffer->tail = buffer->head = 0;
		ov_pcm_seek (stream->file, buffer->pos);
	} else {
		buffer->tail += count;
		if (buffer->tail >= buffer->length)
			buffer->tail -= buffer->length;
	}

	count -= post_count;

	// find out how many new samples we can fit into the buffer
	samples = buffer->tail - buffer->head - 1;
	if (samples < 0)
		samples += buffer->length;

	while (samples) {
		count = buffer->length - buffer->head;
		if (count > samples)
			count = samples;
		samples -= count;

		fill_buffer (buffer, count);

		if (buffer->head >= buffer->length)
			buffer->head = buffer->length;
	}
}

static void
stream_ogg (sfx_t *sfx, char *realname, OggVorbis_File *vf, wavinfo_t info)
{
	sfxstream_t *stream;
	int          samples;
	int          size;
	
	samples = size = shm->speed * 0.3;
	if (!snd_loadas8bit->int_val)
		size *= 2;
	if (info.channels == 2)
		size *= 2;
	stream = calloc (1, sizeof (sfxstream_t) + size);
	memcpy (stream->buffer.data + size, "\xde\xad\xbe\xef", 4);

	free (realname);

	sfx->data = stream;
	sfx->wavinfo = SND_CacheWavinfo;
	sfx->touch = sfx->retain = SND_StreamRetain;
	sfx->release = SND_StreamRelease;

	stream->sfx = sfx;
	stream->file = vf;
	stream->resample = info.channels == 2 ? SND_ResampleStereo
										  : SND_ResampleMono;
	stream->wavinfo = info;

	stream->buffer.length = samples;
	stream->buffer.advance = ogg_advance;
}

void
SND_LoadOgg (QFile *file, sfx_t *sfx, char *realname)
{
	OggVorbis_File vf;
	wavinfo_t   info;

	if (ov_open_callbacks (file, &vf, 0, 0, callbacks) < 0) {
		Sys_Printf ("Input does not appear to be an Ogg bitstream.\n");
		Qclose (file);
		free (realname);
		return;
	}
	info = get_info (&vf);
	if (info.channels < 1 || info.channels > 2) {
		Sys_Printf ("unsupported number of channels");
		return;
	}
	if (info.samples / info.rate < 3) {
		printf ("cache %s\n", realname);
		cache_ogg (sfx, realname, &vf, info);
	} else {
		printf ("stream %s\n", realname);
		stream_ogg (sfx, realname, &vf, info);
	}
}

#endif//HAVE_VORBIS
