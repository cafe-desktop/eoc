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
#define EOC_CLIPBOARD_HANDLER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_CLIPBOARD_HANDLER, EocClipboardHandler))
#define EOC_CLIPBOARD_HANDLER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_CLIPBOARD_HANDLER, EocClipboardHandlerClass))
#define EOC_IS_CLIPBOARD_HANDLER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_CLIPBOARD_HANDLER))
#define EOC_IS_CLIPBOARD_HANDLER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_CLIPBOARD_HANDLER))
#define EOC_CLIPBOARD_HANDLER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_CLIPBOARD_HANDLER, EocClipboardHandlerClass))

typedef struct _EocClipboardHandler EocClipboardHandler;
typedef struct _EocClipboardHandlerClass EocClipboardHandlerClass;
typedef struct _EocClipboardHandlerPrivate EocClipboardHandlerPrivate;

struct _EocClipboardHandler {
	GObject parent;

	EocClipboardHandlerPrivate *priv;
};

struct _EocClipboardHandlerClass {
	GObjectClass parent_klass;
};

GType eoc_clipboard_handler_get_type (void) G_GNUC_CONST;

EocClipboardHandler* eoc_clipboard_handler_new (EocImage *img);

void eoc_clipboard_handler_copy_to_clipboard (EocClipboardHandler *handler,
					      GtkClipboard *clipboard);

G_END_DECLS
#endif /* __EOC_CLIPBOARD_HANDLER_H__ */
