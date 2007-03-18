/*
	midi.c

	midi file loading for use with libWildMidi

	Copyright (C) 2003  Chris Ison

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

static __attribute__ ((used)) const char rcsid[] = 
	"$Id$";

#ifdef HAVE_WILDMIDI

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>
#include <wildmidi_lib.h>

#include "QF/cvar.h"
#include "QF/sound.h"
#include "QF/sys.h"
#include "QF/quakefs.h"

#include "snd_render.h"

static int midi_intiialized = 0;

static cvar_t  *wildmidi_volume;
static cvar_t  *wildmidi_config;

static int
midi_init ( void ) {
	wildmidi_volume = Cvar_Get ("wildmidi_volume", "100", CVAR_ARCHIVE, NULL,
								"Set the Master Volume");
	wildmidi_config = Cvar_Get ("wildmidi_config", "/etc/timidity.cfg",
								CVAR_ROM, NULL,
								"path/filename of timidity.cfg");

	if (WildMidi_Init (wildmidi_config->string, snd_shm->speed, 0) == -1)
		return 1;
	midi_intiialized = 1;
	return 0;
}

static wavinfo_t
get_info (void * handle) {
	wavinfo_t   info;
	struct _WM_Info *wm_info;

	memset (&info, 0, sizeof (info));

	if ((wm_info = WildMidi_GetInfo (handle)) == NULL) {
		Sys_Printf ("Could not obtain midi information\n");
		return info;
	}

	info.rate = snd_shm->speed;
	info.width = 2;
	info.channels = 2;
	info.loopstart = -1;
	info.samples = wm_info->approx_total_samples;
	info.dataofs = 0;
	info.datalen = info.samples * 4;
	return info;
}

static int
midi_stream_read (void *file, byte *buf, int count, wavinfo_t *info)
{
	return WildMidi_GetOutput (file, (char *)buf, (unsigned long int)count);
}

static int
midi_stream_seek (void *file, int pos, wavinfo_t *info)
{
	unsigned long int new_pos;
	pos *= info->width * info->channels;
	pos += info->dataofs;
	new_pos = pos;
	
	return WildMidi_SampledSeek(file, &new_pos);
}

static void
midi_stream_close (sfx_t *sfx)
{
	sfxstream_t *stream = (sfxstream_t *)sfx->data;

	WildMidi_Close (stream->file);
	free (stream);
	free (sfx);
}

/*
 * Note: we only set the QF stream up here.
 * The WildMidi stream was setup when SND_OpenMidi was called
 * so stream->file contains the WildMidi handle for the midi
 */

static sfx_t *
midi_stream_open (sfx_t *_sfx)
{
	sfx_t      *sfx;
	sfxstream_t *stream = (sfxstream_t *) _sfx->data;
	QFile	   *file;
	midi	   *handle;
	unsigned char *local_buffer;
	unsigned long int local_buffer_size;

	QFS_FOpenFile (stream->file, &file);

	local_buffer_size = Qfilesize (file);

	local_buffer = malloc (local_buffer_size);
	Qread (file, local_buffer, local_buffer_size);
	Qclose (file);

	handle = WildMidi_OpenBuffer(local_buffer, local_buffer_size);

	if (handle == NULL) 
		return NULL;	

	return SND_SFX_StreamOpen (sfx, handle, midi_stream_read, midi_stream_seek,
							   midi_stream_close);
}

void
SND_LoadMidi (QFile *file, sfx_t *sfx, char *realname)
{
	wavinfo_t   info;
	midi *handle;
	unsigned char *local_buffer;
	unsigned long int local_buffer_size = Qfilesize (file);

	if (!midi_intiialized) {
		if (midi_init ()) {
			return;
		}
	}
		
	
	local_buffer = malloc (local_buffer_size);
	Qread (file, local_buffer, local_buffer_size);
	Qclose (file);

	// WildMidi takes ownership, so be damned if you touch it
	handle = WildMidi_OpenBuffer (local_buffer, local_buffer_size);

	if (handle == NULL) 
		return;

	info = get_info (handle);

	WildMidi_Close (handle);

	Sys_DPrintf ("stream %s\n", realname);

	// we init stream here cause we will only ever stream
	SND_SFX_Stream (sfx, realname, info, midi_stream_open);
}
#endif // HAVE_WILDMIDI
