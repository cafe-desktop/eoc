/* Eye of Cafe - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnu.org>
 *	- Jens Finke <jens@gnome.org>
 * Based on evince code (shell/ev-window.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#ifndef __EOC_WINDOW_H__
#define __EOC_WINDOW_H__

#include "eoc-list-store.h"
#include "eoc-image.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EocWindow EocWindow;
typedef struct _EocWindowClass EocWindowClass;
typedef struct _EocWindowPrivate EocWindowPrivate;

#define EOC_TYPE_WINDOW            (eoc_window_get_type ())
#define EOC_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_WINDOW, EocWindow))
#define EOC_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOC_TYPE_WINDOW, EocWindowClass))
#define EOC_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_WINDOW))
#define EOC_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOC_TYPE_WINDOW))
#define EOC_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOC_TYPE_WINDOW, EocWindowClass))

#define EOC_WINDOW_ERROR           (eoc_window_error_quark ())

typedef enum {
	EOC_WINDOW_MODE_UNKNOWN,
	EOC_WINDOW_MODE_NORMAL,
	EOC_WINDOW_MODE_FULLSCREEN,
	EOC_WINDOW_MODE_SLIDESHOW
} EocWindowMode;

typedef enum {
	EOC_WINDOW_COLLECTION_POS_BOTTOM,
	EOC_WINDOW_COLLECTION_POS_LEFT,
	EOC_WINDOW_COLLECTION_POS_TOP,
	EOC_WINDOW_COLLECTION_POS_RIGHT
} EocWindowCollectionPos;

//TODO
typedef enum {
	EOC_WINDOW_ERROR_CONTROL_NOT_FOUND,
	EOC_WINDOW_ERROR_UI_NOT_FOUND,
	EOC_WINDOW_ERROR_NO_PERSIST_FILE_INTERFACE,
	EOC_WINDOW_ERROR_IO,
	EOC_WINDOW_ERROR_TRASH_NOT_FOUND,
	EOC_WINDOW_ERROR_GENERIC,
	EOC_WINDOW_ERROR_UNKNOWN
} EocWindowError;

typedef enum {
	EOC_STARTUP_FULLSCREEN         = 1 << 0,
	EOC_STARTUP_SLIDE_SHOW         = 1 << 1,
	EOC_STARTUP_DISABLE_COLLECTION = 1 << 2
} EocStartupFlags;

struct _EocWindow {
	GtkApplicationWindow win;

	EocWindowPrivate *priv;
};

struct _EocWindowClass {
	GtkApplicationWindowClass parent_class;

	void (* prepared) (EocWindow *window);
};

GType         eoc_window_get_type  	(void) G_GNUC_CONST;

GtkWidget    *eoc_window_new		(EocStartupFlags  flags);

EocWindowMode eoc_window_get_mode       (EocWindow       *window);

void          eoc_window_set_mode       (EocWindow       *window,
					 EocWindowMode    mode);

GtkUIManager *eoc_window_get_ui_manager (EocWindow       *window);

EocListStore *eoc_window_get_store      (EocWindow       *window);

GtkWidget    *eoc_window_get_view       (EocWindow       *window);

GtkWidget    *eoc_window_get_sidebar    (EocWindow       *window);

GtkWidget    *eoc_window_get_thumb_view (EocWindow       *window);

GtkWidget    *eoc_window_get_thumb_nav  (EocWindow       *window);

GtkWidget    *eoc_window_get_statusbar  (EocWindow       *window);

EocImage     *eoc_window_get_image      (EocWindow       *window);

void          eoc_window_open_file_list	(EocWindow       *window,
					 GSList          *file_list);

gboolean      eoc_window_is_empty 	(EocWindow       *window);

void          eoc_window_reload_image (EocWindow *window);
GtkWidget    *eoc_window_get_properties_dialog (EocWindow *window);
G_END_DECLS

#endif
