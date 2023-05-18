/* Eye Of Cafe - EOC Preferences Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#ifndef __EOC_PREFERENCES_DIALOG_H__
#define __EOC_PREFERENCES_DIALOG_H__

#include "eoc-image.h"
#include "eoc-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _EocPreferencesDialog EocPreferencesDialog;
typedef struct _EocPreferencesDialogClass EocPreferencesDialogClass;
typedef struct _EocPreferencesDialogPrivate EocPreferencesDialogPrivate;

#define EOC_TYPE_PREFERENCES_DIALOG            (eoc_preferences_dialog_get_type ())
#define EOC_PREFERENCES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_PREFERENCES_DIALOG, EocPreferencesDialog))
#define EOC_PREFERENCES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_PREFERENCES_DIALOG, EocPreferencesDialogClass))
#define EOC_IS_PREFERENCES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_PREFERENCES_DIALOG))
#define EOC_IS_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_PREFERENCES_DIALOG))
#define EOC_PREFERENCES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_PREFERENCES_DIALOG, EocPreferencesDialogClass))

struct _EocPreferencesDialog {
	GtkDialog dialog;

	EocPreferencesDialogPrivate *priv;
};

struct _EocPreferencesDialogClass {
	GtkDialogClass parent_class;
};

G_GNUC_INTERNAL
GType	    eoc_preferences_dialog_get_type	  (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget    *eoc_preferences_dialog_get_instance	  (GtkWindow   *parent);

G_END_DECLS

#endif /* __EOC_PREFERENCES_DIALOG_H__ */
