/*
 * eoc-close-confirmation-dialog.h
 * This file is part of eoc
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.h) by gedit Team
 *
 * Copyright (C) 2004-2009 GNOME Foundation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __EOC_CLOSE_CONFIRMATION_DIALOG_H__
#define __EOC_CLOSE_CONFIRMATION_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>

#include <eoc-image.h>

#define EOC_TYPE_CLOSE_CONFIRMATION_DIALOG		(eoc_close_confirmation_dialog_get_type ())
#define EOC_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_CLOSE_CONFIRMATION_DIALOG, EocCloseConfirmationDialog))
#define EOC_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_CLOSE_CONFIRMATION_DIALOG, EocCloseConfirmationDialogClass))
#define EOC_IS_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define EOC_IS_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define EOC_CLOSE_CONFIRMATION_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),EOC_TYPE_CLOSE_CONFIRMATION_DIALOG, EocCloseConfirmationDialogClass))

typedef struct _EocCloseConfirmationDialog 		EocCloseConfirmationDialog;
typedef struct _EocCloseConfirmationDialogClass 	EocCloseConfirmationDialogClass;
typedef struct _EocCloseConfirmationDialogPrivate 	EocCloseConfirmationDialogPrivate;

struct _EocCloseConfirmationDialog
{
	GtkDialog parent;

	/*< private > */
	EocCloseConfirmationDialogPrivate *priv;
};

struct _EocCloseConfirmationDialogClass
{
	GtkDialogClass parent_class;
};

G_GNUC_INTERNAL
GType 		 eoc_close_confirmation_dialog_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*eoc_close_confirmation_dialog_new			(GtkWindow     *parent,
									 GList         *unsaved_documents);
G_GNUC_INTERNAL
GtkWidget 	*eoc_close_confirmation_dialog_new_single 		(GtkWindow     *parent,
									 EocImage      *image);

G_GNUC_INTERNAL
const GList	*eoc_close_confirmation_dialog_get_unsaved_images	(EocCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
GList		*eoc_close_confirmation_dialog_get_selected_images	(EocCloseConfirmationDialog *dlg);

#endif /* __EOC_CLOSE_CONFIRMATION_DIALOG_H__ */

