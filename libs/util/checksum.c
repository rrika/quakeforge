/*
	checksum.c

	CRC and MD4-based checksum utility functions

	Copyright (C) 2000       Jeff Teunissen <d2deek@pmail.net>

	Author: Jeff Teunissen	<d2deek@pmail.net>
	Date: 01 Jan 2000

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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "QF/checksum.h"
#include "QF/crc.h"
#include "QF/mdfour.h"

static byte chktbl[1024 + 4] = {
	0x78, 0xd2, 0x94, 0xe3, 0x41, 0xec, 0xd6, 0xd5, 0xcb, 0xfc, 0xdb, 0x8a,
		0x4b, 0xcc, 0x85, 0x01,
	0x23, 0xd2, 0xe5, 0xf2, 0x29, 0xa7, 0x45, 0x94, 0x4a, 0x62, 0xe3, 0xa5,
		0x6f, 0x3f, 0xe1, 0x7a,
	0x64, 0xed, 0x5c, 0x99, 0x29, 0x87, 0xa8, 0x78, 0x59, 0x0d, 0xaa, 0x0f,
		0x25, 0x0a, 0x5c, 0x58,
	0xfb, 0x00, 0xa7, 0xa8, 0x8a, 0x1d, 0x86, 0x80, 0xc5, 0x1f, 0xd2, 0x28,
		0x69, 0x71, 0x58, 0xc3,
	0x51, 0x90, 0xe1, 0xf8, 0x6a, 0xf3, 0x8f, 0xb0, 0x68, 0xdf, 0x95, 0x40,
		0x5c, 0xe4, 0x24, 0x6b,
	0x29, 0x19, 0x71, 0x3f, 0x42, 0x63, 0x6c, 0x48, 0xe7, 0xad, 0xa8, 0x4b,
		0x91, 0x8f, 0x42, 0x36,
	0x34, 0xe7, 0x32, 0x55, 0x59, 0x2d, 0x36, 0x38, 0x38, 0x59, 0x9b, 0x08,
		0x16, 0x4d, 0x8d, 0xf8,
	0x0a, 0xa4, 0x52, 0x01, 0xbb, 0x52, 0xa9, 0xfd, 0x40, 0x18, 0x97, 0x37,
		0xff, 0xc9, 0x82, 0x27,
	0xb2, 0x64, 0x60, 0xce, 0x00, 0xd9, 0x04, 0xf0, 0x9e, 0x99, 0xbd, 0xce,
		0x8f, 0x90, 0x4a, 0xdd,
	0xe1, 0xec, 0x19, 0x14, 0xb1, 0xfb, 0xca, 0x1e, 0x98, 0x0f, 0xd4, 0xcb,
		0x80, 0xd6, 0x05, 0x63,
	0xfd, 0xa0, 0x74, 0xa6, 0x86, 0xf6, 0x19, 0x98, 0x76, 0x27, 0x68, 0xf7,
		0xe9, 0x09, 0x9a, 0xf2,
	0x2e, 0x42, 0xe1, 0xbe, 0x64, 0x48, 0x2a, 0x74, 0x30, 0xbb, 0x07, 0xcc,
		0x1f, 0xd4, 0x91, 0x9d,
	0xac, 0x55, 0x53, 0x25, 0xb9, 0x64, 0xf7, 0x58, 0x4c, 0x34, 0x16, 0xbc,
		0xf6, 0x12, 0x2b, 0x65,
	0x68, 0x25, 0x2e, 0x29, 0x1f, 0xbb, 0xb9, 0xee, 0x6d, 0x0c, 0x8e, 0xbb,
		0xd2, 0x5f, 0x1d, 0x8f,
	0xc1, 0x39, 0xf9, 0x8d, 0xc0, 0x39, 0x75, 0xcf, 0x25, 0x17, 0xbe, 0x96,
		0xaf, 0x98, 0x9f, 0x5f,
	0x65, 0x15, 0xc4, 0x62, 0xf8, 0x55, 0xfc, 0xab, 0x54, 0xcf, 0xdc, 0x14,
		0x06, 0xc8, 0xfc, 0x42,
	0xd3, 0xf0, 0xad, 0x10, 0x08, 0xcd, 0xd4, 0x11, 0xbb, 0xca, 0x67, 0xc6,
		0x48, 0x5f, 0x9d, 0x59,
	0xe3, 0xe8, 0x53, 0x67, 0x27, 0x2d, 0x34, 0x9e, 0x9e, 0x24, 0x29, 0xdb,
		0x69, 0x99, 0x86, 0xf9,
	0x20, 0xb5, 0xbb, 0x5b, 0xb0, 0xf9, 0xc3, 0x67, 0xad, 0x1c, 0x9c, 0xf7,
		0xcc, 0xef, 0xce, 0x69,
	0xe0, 0x26, 0x8f, 0x79, 0xbd, 0xca, 0x10, 0x17, 0xda, 0xa9, 0x88, 0x57,
		0x9b, 0x15, 0x24, 0xba,
	0x84, 0xd0, 0xeb, 0x4d, 0x14, 0xf5, 0xfc, 0xe6, 0x51, 0x6c, 0x6f, 0x64,
		0x6b, 0x73, 0xec, 0x85,
	0xf1, 0x6f, 0xe1, 0x67, 0x25, 0x10, 0x77, 0x32, 0x9e, 0x85, 0x6e, 0x69,
		0xb1, 0x83, 0x00, 0xe4,
	0x13, 0xa4, 0x45, 0x34, 0x3b, 0x40, 0xff, 0x41, 0x82, 0x89, 0x79, 0x57,
		0xfd, 0xd2, 0x8e, 0xe8,
	0xfc, 0x1d, 0x19, 0x21, 0x12, 0x00, 0xd7, 0x66, 0xe5, 0xc7, 0x10, 0x1d,
		0xcb, 0x75, 0xe8, 0xfa,
	0xb6, 0xee, 0x7b, 0x2f, 0x1a, 0x25, 0x24, 0xb9, 0x9f, 0x1d, 0x78, 0xfb,
		0x84, 0xd0, 0x17, 0x05,
	0x71, 0xb3, 0xc8, 0x18, 0xff, 0x62, 0xee, 0xed, 0x53, 0xab, 0x78, 0xd3,
		0x65, 0x2d, 0xbb, 0xc7,
	0xc1, 0xe7, 0x70, 0xa2, 0x43, 0x2c, 0x7c, 0xc7, 0x16, 0x04, 0xd2, 0x45,
		0xd5, 0x6b, 0x6c, 0x7a,
	0x5e, 0xa1, 0x50, 0x2e, 0x31, 0x5b, 0xcc, 0xe8, 0x65, 0x8b, 0x16, 0x85,
		0xbf, 0x82, 0x83, 0xfb,
	0xde, 0x9f, 0x36, 0x48, 0x32, 0x79, 0xd6, 0x9b, 0xfb, 0x52, 0x45, 0xbf,
		0x43, 0xf7, 0x0b, 0x0b,
	0x19, 0x19, 0x31, 0xc3, 0x85, 0xec, 0x1d, 0x8c, 0x20, 0xf0, 0x3a, 0xfa,
		0x80, 0x4d, 0x2c, 0x7d,
	0xac, 0x60, 0x09, 0xc0, 0x40, 0xee, 0xb9, 0xeb, 0x13, 0x5b, 0xe8, 0x2b,
		0xb1, 0x20, 0xf0, 0xce,
	0x4c, 0xbd, 0xc6, 0x04, 0x86, 0x70, 0xc6, 0x33, 0xc3, 0x15, 0x0f, 0x65,
		0x19, 0xfd, 0xc2, 0xd3,

// map checksum goes here
	0x00, 0x00, 0x00, 0x00
};

/*
	COM_BlockSequenceCRCByte

	For proxy protecting
*/
byte
COM_BlockSequenceCRCByte (byte * base, int length, int sequence)
{
	unsigned short crc;
	byte       *p;
	byte        chkb[60 + 4];

	p = chktbl + (sequence % (sizeof (chktbl) - 8));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length + 1] = p[1];
	chkb[length + 2] = ((sequence >> 8) & 0xff) ^ p[2];
	chkb[length + 3] = p[3];

	length += 4;

	crc = CRC_Block (chkb, length);

	crc &= 0xff;

	return (byte) crc;
}

unsigned int
Com_BlockChecksum (void *buffer, int length)
{
	int         digest[4];
	unsigned int val;

	mdfour ((unsigned char *) digest, (unsigned char *) buffer, length);

	val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];

	return val;
}

void
Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf)
{
	mdfour (outbuf, (unsigned char *) buffer, len);
}
