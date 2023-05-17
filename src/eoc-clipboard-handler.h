/*
 * eoc-clipboard-handler.h
 * This file is part of eoc
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2010 GNOME Foundation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __EOC_CLIPBOARD_HANDLER_H__
#define __EOC_CLIPBOARD_HANDLER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "eoc-image.h"

G_BEGIN_DECLS

#define EOC_TYPE_CLIPBOARD_HANDLER          (eoc_clipboard_handler_get_type ())
#define EOC_CLIPBOARD_HANDLER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_CLIPBOARD_HANDLER, EomClipboardHandler))
#define EOC_CLIPBOARD_HANDLER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_CLIPBOARD_HANDLER, EomClipboardHandlerClass))
#define EOC_IS_CLIPBOARD_HANDLER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_CLIPBOARD_HANDLER))
#define EOC_IS_CLIPBOARD_HANDLER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_CLIPBOARD_HANDLER))
#define EOC_CLIPBOARD_HANDLER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_CLIPBOARD_HANDLER, EomClipboardHandlerClass))

typedef struct _EomClipboardHandler EomClipboardHandler;
typedef struct _EomClipboardHandlerClass EomClipboardHandlerClass;
typedef struct _EomClipboardHandlerPrivate EomClipboardHandlerPrivate;

struct _EomClipboardHandler {
	GObject parent;

	EomClipboardHandlerPrivate *priv;
};

struct _EomClipboardHandlerClass {
	GObjectClass parent_klass;
};

GType eoc_clipboard_handler_get_type (void) G_GNUC_CONST;

EomClipboardHandler* eoc_clipboard_handler_new (EomImage *img);

void eoc_clipboard_handler_copy_to_clipboard (EomClipboardHandler *handler,
					      GtkClipboard *clipboard);

G_END_DECLS
#endif /* __EOC_CLIPBOARD_HANDLER_H__ */
