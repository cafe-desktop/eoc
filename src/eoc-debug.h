/* Eye Of Mate - Debugging
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-debug.h) by:
 * 	- Alex Roberts <bse@error.fsnet.co.uk>
 *	- Evan Lawrence <evan@worldpath.net>
 *	- Chema Celorio <chema@celorio.com>
 *	- Paolo Maggi <paolo@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __EOC_DEBUG_H__
#define __EOC_DEBUG_H__

#include <glib.h>

typedef enum {
	EOC_DEBUG_NO_DEBUG           = 0,
	EOC_DEBUG_WINDOW       = 1 << 0,
	EOC_DEBUG_VIEW         = 1 << 1,
	EOC_DEBUG_JOBS         = 1 << 2,
	EOC_DEBUG_THUMBNAIL    = 1 << 3,
	EOC_DEBUG_IMAGE_DATA   = 1 << 4,
	EOC_DEBUG_IMAGE_LOAD   = 1 << 5,
	EOC_DEBUG_IMAGE_SAVE   = 1 << 6,
	EOC_DEBUG_LIST_STORE   = 1 << 7,
	EOC_DEBUG_PREFERENCES  = 1 << 8,
	EOC_DEBUG_PRINTING     = 1 << 9,
	EOC_DEBUG_LCMS         = 1 << 10,
	EOC_DEBUG_PLUGINS      = 1 << 11
} EomDebug;

#define	DEBUG_WINDOW		EOC_DEBUG_WINDOW,      __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_VIEW		EOC_DEBUG_VIEW,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_JOBS		EOC_DEBUG_JOBS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_THUMBNAIL		EOC_DEBUG_THUMBNAIL,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_DATA	EOC_DEBUG_IMAGE_DATA,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_LOAD	EOC_DEBUG_IMAGE_LOAD,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_SAVE	EOC_DEBUG_IMAGE_SAVE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LIST_STORE	EOC_DEBUG_LIST_STORE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFERENCES	EOC_DEBUG_PREFERENCES, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PRINTING		EOC_DEBUG_PRINTING,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LCMS 		EOC_DEBUG_LCMS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS 		EOC_DEBUG_PLUGINS,     __FILE__, __LINE__, G_STRFUNC

void   eoc_debug_init        (void);

void   eoc_debug             (EomDebug    section,
          	              const gchar       *file,
          	              gint               line,
          	              const gchar       *function);

void   eoc_debug_message     (EomDebug    section,
			      const gchar       *file,
			      gint               line,
			      const gchar       *function,
			      const gchar       *format, ...) G_GNUC_PRINTF(5, 6);

#endif /* __EOC_DEBUG_H__ */
