/*
	snd_oss.c

	(description)

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

	$Id$
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#if defined HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#elif defined HAVE_LINUX_SOUNDCARD_H
# include <linux/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
# include <machine/soundcard.h>
#endif

#include "QF/cmd.h"
#include "QF/console.h"
#include "QF/qargs.h"
#include "QF/sound.h"

#ifndef MAP_FAILED
# define MAP_FAILED ((void *) -1)
#endif

static int         audio_fd;
static int         snd_inited;
static char       *snd_dev = "/dev/dsp";

static int  tryrates[] = { 11025, 22050, 22051, 44100, 8000 };

qboolean
SNDDMA_Init (void)
{
	int         rc;
	int         fmt;
	int         tmp;
	int         i;
	struct audio_buf_info info;
	int         caps;
	int         retries = 3;
	int         omode = O_WRONLY;

	snd_inited = 0;

	// open snd_dev, confirm capability to mmap, and get size of dma buffer
	if (snd_device->string[0])
		snd_dev = snd_device->string;

	if (snd_oss_rw->int_val)
	    omode = O_RDWR;

	audio_fd = open (snd_dev, omode);
	if (audio_fd < 0) {					// Failed open, retry up to 3 times
		// if it's busy
		while ((audio_fd < 0) && retries-- &&
			   ((errno == EAGAIN) || (errno == EBUSY))) {
			sleep (1);
			audio_fd = open (snd_dev, O_RDWR);
		}
		if (audio_fd < 0) {
			perror (snd_dev);
			Con_Printf ("Could not open %s\n", snd_dev);
			return 0;
		}
	}

	if ((rc = ioctl (audio_fd, SNDCTL_DSP_RESET, 0)) < 0) {
		perror (snd_dev);
		Con_Printf ("Could not reset %s\n", snd_dev);
		close (audio_fd);
		return 0;
	}

	if (ioctl (audio_fd, SNDCTL_DSP_GETCAPS, &caps) == -1) {
		perror (snd_dev);
		Con_Printf ("Sound driver too old\n");
		close (audio_fd);
		return 0;
	}

	if (!(caps & DSP_CAP_TRIGGER) || !(caps & DSP_CAP_MMAP)) {
		Con_Printf ("Sound device can't do memory-mapped I/O.\n");
		close (audio_fd);
		return 0;
	}

	if (ioctl (audio_fd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
		perror ("GETOSPACE");
		Con_Printf ("Um, can't do GETOSPACE?\n");
		close (audio_fd);
		return 0;
	}

	shm = &sn;
	shm->splitbuffer = 0;

	// set sample bits & speed
	shm->samplebits = snd_bits->int_val;

	if (shm->samplebits != 16 && shm->samplebits != 8) {
		ioctl (audio_fd, SNDCTL_DSP_GETFMTS, &fmt);

		if (fmt & AFMT_S16_LE) {		// little-endian 16-bit signed
			shm->samplebits = 16;
		} else {
			if (fmt & AFMT_U8) {		// unsigned 8-bit ulaw
				shm->samplebits = 8;
			}
		}
	}

	if (snd_rate->int_val) {
		shm->speed = snd_rate->int_val;
	} else {
		for (i = 0; i < (sizeof (tryrates) / 4); i++)
			if (!ioctl (audio_fd, SNDCTL_DSP_SPEED, &tryrates[i]))
				break;
		shm->speed = tryrates[i];
	}

	if (!snd_stereo->int_val) {
		shm->channels = 1;
	} else {
		shm->channels = 2;
	}

	shm->samples = info.fragstotal * info.fragsize / (shm->samplebits / 8);
	shm->submission_chunk = 1;

	// memory map the dma buffer
	shm->buffer = (unsigned char *) mmap (NULL, info.fragstotal
										  * info.fragsize,
										  PROT_READ | PROT_WRITE,
										  MAP_FILE | MAP_SHARED, audio_fd, 0);

	if (shm->buffer == MAP_FAILED) {
		perror (snd_dev);
		Con_Printf ("Could not mmap %s\n", snd_dev);
		close (audio_fd);
		return 0;
	}

	tmp = 0;
	if (shm->channels == 2)
		tmp = 1;
	rc = ioctl (audio_fd, SNDCTL_DSP_STEREO, &tmp);
	if (rc < 0) {
		perror (snd_dev);
		Con_Printf ("Could not set %s to stereo=%d", snd_dev, shm->channels);
		close (audio_fd);
		return 0;
	}

	if (tmp)
		shm->channels = 2;
	else
		shm->channels = 1;

	rc = ioctl (audio_fd, SNDCTL_DSP_SPEED, &shm->speed);
	if (rc < 0) {
		perror (snd_dev);
		Con_Printf ("Could not set %s speed to %d", snd_dev, shm->speed);
		close (audio_fd);
		return 0;
	}

	if (shm->samplebits == 16) {
		rc = AFMT_S16_LE;
		rc = ioctl (audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0) {
			perror (snd_dev);
			Con_Printf ("Could not support 16-bit data.  Try 8-bit.\n");
			close (audio_fd);
			return 0;
		}
	} else if (shm->samplebits == 8) {
		rc = AFMT_U8;
		rc = ioctl (audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0) {
			perror (snd_dev);
			Con_Printf ("Could not support 8-bit data.\n");
			close (audio_fd);
			return 0;
		}
	} else {
		perror (snd_dev);
		Con_Printf ("%d-bit sound not supported.", shm->samplebits);
		close (audio_fd);
		return 0;
	}

// toggle the trigger & start her up

	tmp = 0;
	rc = ioctl (audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	if (rc < 0) {
		perror (snd_dev);
		Con_Printf ("Could not toggle.\n");
		close (audio_fd);
		return 0;
	}
	tmp = PCM_ENABLE_OUTPUT;
	rc = ioctl (audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	if (rc < 0) {
		perror (snd_dev);
		Con_Printf ("Could not toggle.\n");
		close (audio_fd);
		return 0;
	}

	shm->samplepos = 0;

	snd_inited = 1;
	return 1;

}

int
SNDDMA_GetDMAPos (void)
{

	struct count_info count;

	if (!snd_inited)
		return 0;

	if (ioctl (audio_fd, SNDCTL_DSP_GETOPTR, &count) == -1) {
		perror (snd_dev);
		Con_Printf ("Uh, sound dead.\n");
		close (audio_fd);
		snd_inited = 0;
		return 0;
	}
//  shm->samplepos = (count.bytes / (shm->samplebits / 8)) & (shm->samples-1);
//  fprintf(stderr, "%d \r", count.ptr);
	shm->samplepos = count.ptr / (shm->samplebits / 8);

	return shm->samplepos;

}

void
SNDDMA_Shutdown (void)
{
	if (snd_inited) {
		close (audio_fd);
		snd_inited = 0;
	}
}

/*
	SNDDMA_Submit

	Send sound to device if buffer isn't really the dma buffer
*/
void
SNDDMA_Submit (void)
{
}
