/* Eye Of Mate - Image Properties Dialog
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

#ifndef __EOC_PROPERTIES_DIALOG_H__
#define __EOC_PROPERTIES_DIALOG_H__

#include "eoc-image.h"
#include "eoc-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EomPropertiesDialog EomPropertiesDialog;
typedef struct _EomPropertiesDialogClass EomPropertiesDialogClass;
typedef struct _EomPropertiesDialogPrivate EomPropertiesDialogPrivate;

#define EOC_TYPE_PROPERTIES_DIALOG            (eoc_properties_dialog_get_type ())
#define EOC_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_PROPERTIES_DIALOG, EomPropertiesDialog))
#define EOC_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_PROPERTIES_DIALOG, EomPropertiesDialogClass))
#define EOC_IS_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_PROPERTIES_DIALOG))
#define EOC_IS_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_PROPERTIES_DIALOG))
#define EOC_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_PROPERTIES_DIALOG, EomPropertiesDialogClass))

typedef enum {
	EOC_PROPERTIES_DIALOG_PAGE_GENERAL = 0,
	EOC_PROPERTIES_DIALOG_PAGE_EXIF,
	EOC_PROPERTIES_DIALOG_PAGE_DETAILS,
	EOC_PROPERTIES_DIALOG_N_PAGES
} EomPropertiesDialogPage;

struct _EomPropertiesDialog {
	GtkDialog dialog;

	EomPropertiesDialogPrivate *priv;
};

struct _EomPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType	    eoc_properties_dialog_get_type	(void) G_GNUC_CONST;

GtkWidget   *eoc_properties_dialog_new	  	(GtkWindow               *parent,
                                             EomThumbView            *thumbview,
                                             GtkAction               *next_image_action,
                                             GtkAction               *previous_image_action);

void	    eoc_properties_dialog_update  	(EomPropertiesDialog     *prop,
						 EomImage                *image);

void	    eoc_properties_dialog_set_page  	(EomPropertiesDialog     *prop,
						 EomPropertiesDialogPage  page);

void	    eoc_properties_dialog_set_netbook_mode (EomPropertiesDialog *dlg,
						    gboolean enable);
G_END_DECLS

#endif /* __EOC_PROPERTIES_DIALOG_H__ */
