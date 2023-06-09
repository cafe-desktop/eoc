/* Eye Of Cafe - Image Properties Dialog
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
#include <ctk/ctk.h>

G_BEGIN_DECLS

typedef struct _EocPropertiesDialog EocPropertiesDialog;
typedef struct _EocPropertiesDialogClass EocPropertiesDialogClass;
typedef struct _EocPropertiesDialogPrivate EocPropertiesDialogPrivate;

#define EOC_TYPE_PROPERTIES_DIALOG            (eoc_properties_dialog_get_type ())
#define EOC_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_PROPERTIES_DIALOG, EocPropertiesDialog))
#define EOC_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_PROPERTIES_DIALOG, EocPropertiesDialogClass))
#define EOC_IS_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_PROPERTIES_DIALOG))
#define EOC_IS_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_PROPERTIES_DIALOG))
#define EOC_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_PROPERTIES_DIALOG, EocPropertiesDialogClass))

typedef enum {
	EOC_PROPERTIES_DIALOG_PAGE_GENERAL = 0,
	EOC_PROPERTIES_DIALOG_PAGE_EXIF,
	EOC_PROPERTIES_DIALOG_PAGE_DETAILS,
	EOC_PROPERTIES_DIALOG_N_PAGES
} EocPropertiesDialogPage;

struct _EocPropertiesDialog {
	CtkDialog dialog;

	EocPropertiesDialogPrivate *priv;
};

struct _EocPropertiesDialogClass {
	CtkDialogClass parent_class;
};

GType	    eoc_properties_dialog_get_type	(void) G_GNUC_CONST;

CtkWidget   *eoc_properties_dialog_new	  	(CtkWindow               *parent,
                                             EocThumbView            *thumbview,
                                             CtkAction               *next_image_action,
                                             CtkAction               *previous_image_action);

void	    eoc_properties_dialog_update  	(EocPropertiesDialog     *prop,
						 EocImage                *image);

void	    eoc_properties_dialog_set_page  	(EocPropertiesDialog     *prop,
						 EocPropertiesDialogPage  page);

void	    eoc_properties_dialog_set_netbook_mode (EocPropertiesDialog *dlg,
						    gboolean enable);
G_END_DECLS

#endif /* __EOC_PROPERTIES_DIALOG_H__ */
