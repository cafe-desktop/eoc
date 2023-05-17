/* Eye of Mate - Main Window
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

typedef struct _EomWindow EomWindow;
typedef struct _EomWindowClass EomWindowClass;
typedef struct _EomWindowPrivate EomWindowPrivate;

#define EOC_TYPE_WINDOW            (eoc_window_get_type ())
#define EOC_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_WINDOW, EomWindow))
#define EOC_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOC_TYPE_WINDOW, EomWindowClass))
#define EOC_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_WINDOW))
#define EOC_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOC_TYPE_WINDOW))
#define EOC_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOC_TYPE_WINDOW, EomWindowClass))

#define EOC_WINDOW_ERROR           (eoc_window_error_quark ())

typedef enum {
	EOC_WINDOW_MODE_UNKNOWN,
	EOC_WINDOW_MODE_NORMAL,
	EOC_WINDOW_MODE_FULLSCREEN,
	EOC_WINDOW_MODE_SLIDESHOW
} EomWindowMode;

typedef enum {
	EOC_WINDOW_COLLECTION_POS_BOTTOM,
	EOC_WINDOW_COLLECTION_POS_LEFT,
	EOC_WINDOW_COLLECTION_POS_TOP,
	EOC_WINDOW_COLLECTION_POS_RIGHT
} EomWindowCollectionPos;

//TODO
typedef enum {
	EOC_WINDOW_ERROR_CONTROL_NOT_FOUND,
	EOC_WINDOW_ERROR_UI_NOT_FOUND,
	EOC_WINDOW_ERROR_NO_PERSIST_FILE_INTERFACE,
	EOC_WINDOW_ERROR_IO,
	EOC_WINDOW_ERROR_TRASH_NOT_FOUND,
	EOC_WINDOW_ERROR_GENERIC,
	EOC_WINDOW_ERROR_UNKNOWN
} EomWindowError;

typedef enum {
	EOC_STARTUP_FULLSCREEN         = 1 << 0,
	EOC_STARTUP_SLIDE_SHOW         = 1 << 1,
	EOC_STARTUP_DISABLE_COLLECTION = 1 << 2
} EomStartupFlags;

struct _EomWindow {
	GtkApplicationWindow win;

	EomWindowPrivate *priv;
};

struct _EomWindowClass {
	GtkApplicationWindowClass parent_class;

	void (* prepared) (EomWindow *window);
};

GType         eoc_window_get_type  	(void) G_GNUC_CONST;

GtkWidget    *eoc_window_new		(EomStartupFlags  flags);

EomWindowMode eoc_window_get_mode       (EomWindow       *window);

void          eoc_window_set_mode       (EomWindow       *window,
					 EomWindowMode    mode);

GtkUIManager *eoc_window_get_ui_manager (EomWindow       *window);

EomListStore *eoc_window_get_store      (EomWindow       *window);

GtkWidget    *eoc_window_get_view       (EomWindow       *window);

GtkWidget    *eoc_window_get_sidebar    (EomWindow       *window);

GtkWidget    *eoc_window_get_thumb_view (EomWindow       *window);

GtkWidget    *eoc_window_get_thumb_nav  (EomWindow       *window);

GtkWidget    *eoc_window_get_statusbar  (EomWindow       *window);

EomImage     *eoc_window_get_image      (EomWindow       *window);

void          eoc_window_open_file_list	(EomWindow       *window,
					 GSList          *file_list);

gboolean      eoc_window_is_empty 	(EomWindow       *window);

void          eoc_window_reload_image (EomWindow *window);
GtkWidget    *eoc_window_get_properties_dialog (EomWindow *window);
G_END_DECLS

#endif
