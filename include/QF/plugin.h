/*
	plugin.h

	QuakeForge plugin API structures and prototypes

	Copyright (C) 2001 Jeff Teunissen <deek@dusknet.dhs.org>

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

#ifndef __QF_plugin_h_
#define __QF_plugin_h_

#define QFPLUGIN_VERSION	"1.0"

#ifdef WIN32
# ifdef DLL_EXPORT
#  define QFPLUGIN __declspec(dllexport)
# else
#  define QFPLUGIN
# endif
#else
# define QFPLUGIN
#endif

#include <QF/qtypes.h>
#include <QF/plugin/cd.h>
#include <QF/plugin/console.h>
#include <QF/plugin/general.h>
#include <QF/plugin/input.h>
#include <QF/plugin/snd_output.h>
#include <QF/plugin/snd_render.h>

#ifdef STATIC_PLUGINS
#define PLUGIN_INFO(type,name) type##_##name##_PluginInfo
#else
#define PLUGIN_INFO(type,name) PluginInfo
#endif

typedef enum {
	qfp_null = 0,	// Not real
	qfp_input,		// Input (pointing devices, joysticks, etc)
	qfp_cd,			// CD Audio
	qfp_console,	// Console `driver'
	qfp_snd_output,	// Sound output (OSS, ALSA, Win32)
	qfp_snd_render,	// Sound mixing
} plugin_type_t;

typedef struct plugin_funcs_s {
	general_funcs_t *general;
	input_funcs_t	*input;
	cd_funcs_t		*cd;
	console_funcs_t	*console;
	snd_output_funcs_t	*snd_output;
	snd_render_funcs_t	*snd_render;
} plugin_funcs_t;

typedef struct plugin_data_s {
	general_data_t	*general;
	input_data_t	*input;
	cd_data_t		*cd;
	console_data_t	*console;
	snd_output_data_t	*snd_output;
	snd_render_data_t	*snd_render;
} plugin_data_t;

typedef struct plugin_s {
	plugin_type_t	type;
	void			*handle;
	const char		*api_version;
	const char		*plugin_version;
	const char		*description;
	const char		*copyright;
	plugin_funcs_t	*functions;
	plugin_data_t	*data;
} plugin_t;

/*
	General plugin info return function type
*/
typedef plugin_t * (*P_PluginInfo) (void);

typedef struct plugin_list_s {
	const char		*name;
	P_PluginInfo	info;
} plugin_list_t;

/*
	Plugin system variables
*/
extern struct cvar_s	*fs_pluginpath;

/*
	Function prototypes
*/
plugin_t *PI_LoadPlugin (const char *, const char *);
qboolean PI_UnloadPlugin (plugin_t *);
void PI_RegisterPlugins (plugin_list_t *);
void PI_Init (void);
void PI_Shutdown (void);

// FIXME: we need a generic function to initialize unused fields

#endif	// __QF_plugin_h_
