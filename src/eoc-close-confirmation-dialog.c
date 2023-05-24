/*
 * eoc-close-confirmation-dialog.c
 * This file is part of eoc
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.c) by gedit Team
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "eoc-close-confirmation-dialog.h"
#include <eoc-window.h>

/* Properties */
enum
{
	PROP_0,
	PROP_UNSAVED_IMAGES
};

/* Mode */
enum
{
	SINGLE_IMG_MODE,
	MULTIPLE_IMGS_MODE
};

/* Columns */
enum
{
	SAVE_COLUMN,
	IMAGE_COLUMN,
	NAME_COLUMN,
	IMG_COLUMN, /* a handy pointer to the image */
	N_COLUMNS
};

struct _EocCloseConfirmationDialogPrivate
{
	GList       *unsaved_images;

	GList       *selected_images;

	CtkTreeModel *list_store;
	CtkCellRenderer *toggle_renderer;
};

#define GET_MODE(priv) (((priv->unsaved_images != NULL) && \
			 (priv->unsaved_images->next == NULL)) ? \
			  SINGLE_IMG_MODE : MULTIPLE_IMGS_MODE)

#define IMAGE_COLUMN_HEIGHT 40

G_DEFINE_TYPE_WITH_PRIVATE (EocCloseConfirmationDialog, eoc_close_confirmation_dialog, CTK_TYPE_DIALOG)

static void 	 set_unsaved_image 		(EocCloseConfirmationDialog *dlg,
						 const GList                  *list);

static GList 	*get_selected_imgs 		(CtkTreeModel                 *store);

static CdkPixbuf *
eoc_close_confirmation_dialog_get_icon (const gchar *icon_name)
{
	GError *error = NULL;
	CtkIconTheme *icon_theme;
	CdkPixbuf *pixbuf;

	icon_theme = ctk_icon_theme_get_default ();

	pixbuf = ctk_icon_theme_load_icon (icon_theme,
					   icon_name,
					   IMAGE_COLUMN_HEIGHT,
					   0,
					   &error);

	if (!pixbuf) {
		g_warning ("Couldn't load icon: %s", error->message);
		g_error_free (error);
	}

	return pixbuf;
}

static CdkPixbuf*
get_nothumb_pixbuf (void)
{
	static GOnce nothumb_once = G_ONCE_INIT;
	g_once (&nothumb_once, (GThreadFunc) eoc_close_confirmation_dialog_get_icon, "image-x-generic");
	return CDK_PIXBUF (g_object_ref (nothumb_once.retval));
}

/*  Since we connect in the costructor we are sure this handler will be called
 *  before the user ones
 */
static void
response_cb (EocCloseConfirmationDialog *dlg,
             gint                        response_id,
             gpointer                    data)
{
	EocCloseConfirmationDialogPrivate *priv;

	g_return_if_fail (EOC_IS_CLOSE_CONFIRMATION_DIALOG (dlg));

	priv = dlg->priv;

	if (priv->selected_images != NULL)
		g_list_free (priv->selected_images);

	if (response_id == CTK_RESPONSE_YES)
	{
		if (GET_MODE (priv) == SINGLE_IMG_MODE)
		{
			priv->selected_images =
				g_list_copy (priv->unsaved_images);
		}
		else
		{
			g_return_if_fail (priv->list_store);

			priv->selected_images =
				get_selected_imgs (priv->list_store);
		}
	}
	else
		priv->selected_images = NULL;
}

static void
add_buttons (EocCloseConfirmationDialog *dlg)
{
	ctk_dialog_add_button (CTK_DIALOG (dlg),
			       _("Close _without Saving"),
			       CTK_RESPONSE_NO);

	ctk_dialog_add_button (CTK_DIALOG (dlg),
			       "ctk-cancel", CTK_RESPONSE_CANCEL);

	ctk_dialog_add_button (CTK_DIALOG (dlg),
			       "ctk-save",
			       CTK_RESPONSE_YES);

	ctk_dialog_set_default_response	(CTK_DIALOG (dlg),
					 CTK_RESPONSE_YES);
}

static void
eoc_close_confirmation_dialog_init (EocCloseConfirmationDialog *dlg)
{
	AtkObject *atk_obj;

	dlg->priv = eoc_close_confirmation_dialog_get_instance_private (dlg);

	ctk_container_set_border_width (CTK_CONTAINER (dlg), 5);
	ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dlg))), 14);
	ctk_window_set_resizable (CTK_WINDOW (dlg), FALSE);
	ctk_window_set_skip_taskbar_hint (CTK_WINDOW (dlg), TRUE);

	ctk_window_set_title (CTK_WINDOW (dlg), "");

	ctk_window_set_modal (CTK_WINDOW (dlg), TRUE);
	ctk_window_set_destroy_with_parent (CTK_WINDOW (dlg), TRUE);

	atk_obj = ctk_widget_get_accessible (CTK_WIDGET (dlg));
	atk_object_set_role (atk_obj, ATK_ROLE_ALERT);
	atk_object_set_name (atk_obj, _("Question"));

	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (response_cb),
			  NULL);
}

static void
eoc_close_confirmation_dialog_finalize (GObject *object)
{
	EocCloseConfirmationDialogPrivate *priv;

	priv = EOC_CLOSE_CONFIRMATION_DIALOG (object)->priv;

	if (priv->unsaved_images != NULL)
		g_list_free (priv->unsaved_images);

	if (priv->selected_images != NULL)
		g_list_free (priv->selected_images);

	/* Call the parent's destructor */
	G_OBJECT_CLASS (eoc_close_confirmation_dialog_parent_class)->finalize (object);
}

static void
eoc_close_confirmation_dialog_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec)
{
	EocCloseConfirmationDialog *dlg;

	dlg = EOC_CLOSE_CONFIRMATION_DIALOG (object);

	switch (prop_id)
	{
		case PROP_UNSAVED_IMAGES:
			set_unsaved_image (dlg, g_value_get_pointer (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
eoc_close_confirmation_dialog_get_property (GObject    *object,
					      guint       prop_id,
					      GValue     *value,
					      GParamSpec *pspec)
{
	EocCloseConfirmationDialogPrivate *priv;

	priv = EOC_CLOSE_CONFIRMATION_DIALOG (object)->priv;

	switch( prop_id )
	{
		case PROP_UNSAVED_IMAGES:
			g_value_set_pointer (value, priv->unsaved_images);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
eoc_close_confirmation_dialog_class_init (EocCloseConfirmationDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = eoc_close_confirmation_dialog_set_property;
	gobject_class->get_property = eoc_close_confirmation_dialog_get_property;
	gobject_class->finalize = eoc_close_confirmation_dialog_finalize;

	g_object_class_install_property (gobject_class,
					 PROP_UNSAVED_IMAGES,
					 g_param_spec_pointer ("unsaved_images",
						 	       "Unsaved Images",
							       "List of Unsaved Images",
							       (G_PARAM_READWRITE |
							        G_PARAM_CONSTRUCT_ONLY)));
}

static GList *
get_selected_imgs (CtkTreeModel *store)
{
	GList       *list;
	gboolean     valid;
	CtkTreeIter  iter;

	list = NULL;
	valid = ctk_tree_model_get_iter_first (store, &iter);

	while (valid)
	{
		gboolean to_save;
		EocImage *img;

		ctk_tree_model_get (store, &iter,
				    SAVE_COLUMN, &to_save,
				    IMG_COLUMN, &img,
				    -1);
		if (to_save)
			list = g_list_prepend (list, img);

		valid = ctk_tree_model_iter_next (store, &iter);
	}

	list = g_list_reverse (list);

	return list;
}

GList *
eoc_close_confirmation_dialog_get_selected_images (EocCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (EOC_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return g_list_copy (dlg->priv->selected_images);
}

CtkWidget *
eoc_close_confirmation_dialog_new (CtkWindow *parent,
				   GList     *unsaved_images)
{
	CtkWidget *dlg;
	CtkWindowGroup *wg;

	g_return_val_if_fail (unsaved_images != NULL, NULL);

	dlg = CTK_WIDGET (g_object_new (EOC_TYPE_CLOSE_CONFIRMATION_DIALOG,
				        "unsaved_images", unsaved_images,
				        NULL));
	g_return_val_if_fail (dlg != NULL, NULL);

	if (parent != NULL)
	{
		wg = ctk_window_get_group (parent);

		/* ctk_window_get_group returns a default group when the given
		 * window doesn't have a group. Explicitly add the window to
		 * the group here to make sure it's actually in the returned
		 * group. It makes no difference if it is already. */
		ctk_window_group_add_window (wg, parent);
		ctk_window_group_add_window (wg, CTK_WINDOW (dlg));

		ctk_window_set_transient_for (CTK_WINDOW (dlg), parent);
	}

	return dlg;
}

CtkWidget *
eoc_close_confirmation_dialog_new_single (CtkWindow     *parent,
					  EocImage	*image)
{
	CtkWidget *dlg;
	GList *unsaved_images;
	g_return_val_if_fail (image != NULL, NULL);

	unsaved_images = g_list_prepend (NULL, image);

	dlg = eoc_close_confirmation_dialog_new (parent,
						 unsaved_images);

	g_list_free (unsaved_images);

	return dlg;
}

static gchar *
get_text_secondary_label (EocImage *image)
{
	gchar *secondary_msg;

	secondary_msg = g_strdup (_("If you don't save, your changes will be lost."));

	return secondary_msg;
}

static void
build_single_img_dialog (EocCloseConfirmationDialog *dlg)
{
	CtkWidget     *hbox;
	CtkWidget     *vbox;
	CtkWidget     *primary_label;
	CtkWidget     *secondary_label;
	CtkWidget     *image;
	EocImage      *img;
	const gchar   *image_name;
	gchar         *str;
	gchar         *markup_str;

	g_return_if_fail (dlg->priv->unsaved_images->data != NULL);
	img = EOC_IMAGE (dlg->priv->unsaved_images->data);

	/* Image */
	image = ctk_image_new_from_icon_name ("dialog-warning",
					  CTK_ICON_SIZE_DIALOG);
	ctk_widget_set_valign (image, CTK_ALIGN_START);

	/* Primary label */
	primary_label = ctk_label_new (NULL);
	ctk_label_set_line_wrap (CTK_LABEL (primary_label), TRUE);
	ctk_label_set_use_markup (CTK_LABEL (primary_label), TRUE);
	ctk_label_set_max_width_chars (CTK_LABEL (primary_label), 88);
	ctk_label_set_xalign (CTK_LABEL (primary_label), 0.0);
	ctk_label_set_selectable (CTK_LABEL (primary_label), TRUE);

	image_name = eoc_image_get_caption (img);

	str = g_markup_printf_escaped (_("Save changes to image \"%s\" before closing?"),
				       image_name);
	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);

	ctk_label_set_markup (CTK_LABEL (primary_label), markup_str);
	g_free (markup_str);

	/* Secondary label */
	str = get_text_secondary_label (img);

	secondary_label = ctk_label_new (str);
	g_free (str);
	ctk_label_set_line_wrap (CTK_LABEL (secondary_label), TRUE);
	ctk_label_set_xalign (CTK_LABEL (secondary_label), 0.0);
	ctk_label_set_selectable (CTK_LABEL (secondary_label), TRUE);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
	ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);

	ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);

	ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	ctk_box_pack_start (CTK_BOX (vbox), primary_label, FALSE, FALSE, 0);

	ctk_box_pack_start (CTK_BOX (vbox), secondary_label, FALSE, FALSE, 0);

	ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dlg))),
			    hbox,
	                    FALSE,
			    FALSE,
			    0);

	add_buttons (dlg);

	ctk_widget_show_all (hbox);
}

static void
populate_model (CtkTreeModel *store, GList *imgs)
{
	CtkTreeIter iter;

	while (imgs != NULL)
	{
		EocImage *img;
		const gchar *name;
		CdkPixbuf *buf = NULL;
		CdkPixbuf *buf_scaled = NULL;
		int width;
		double ratio;

		img = EOC_IMAGE (imgs->data);

		name = eoc_image_get_caption (img);
		buf = eoc_image_get_thumbnail (img);

		if (buf) {
			ratio = IMAGE_COLUMN_HEIGHT / (double) cdk_pixbuf_get_height (buf);
			width = (int) (cdk_pixbuf_get_width (buf) * ratio);
			buf_scaled = cdk_pixbuf_scale_simple (buf, width, IMAGE_COLUMN_HEIGHT, CDK_INTERP_BILINEAR);
		} else
			buf_scaled = get_nothumb_pixbuf ();

		ctk_list_store_append (CTK_LIST_STORE (store), &iter);
		ctk_list_store_set (CTK_LIST_STORE (store), &iter,
				    SAVE_COLUMN, TRUE,
				    IMAGE_COLUMN, buf_scaled,
				    NAME_COLUMN, name,
				    IMG_COLUMN, img,
			            -1);

		imgs = g_list_next (imgs);
		g_object_unref (buf_scaled);
	}
}

static void
save_toggled (CtkCellRendererToggle *renderer, gchar *path_str, CtkTreeModel *store)
{
	CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
	CtkTreeIter iter;
	gboolean active;

	ctk_tree_model_get_iter (store, &iter, path);
	ctk_tree_model_get (store, &iter, SAVE_COLUMN, &active, -1);

	active ^= 1;

	ctk_list_store_set (CTK_LIST_STORE (store), &iter,
			    SAVE_COLUMN, active, -1);

	ctk_tree_path_free (path);
}

static CtkWidget *
create_treeview (EocCloseConfirmationDialogPrivate *priv)
{
	CtkListStore *store;
	CtkWidget *treeview;
	CtkCellRenderer *renderer;
	CtkTreeViewColumn *column;

	treeview = ctk_tree_view_new ();
	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (treeview), FALSE);
	ctk_tree_view_set_enable_search (CTK_TREE_VIEW (treeview), FALSE);

	/* Create and populate the model */
	store = ctk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, CDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	populate_model (CTK_TREE_MODEL (store), priv->unsaved_images);

	/* Set model to the treeview */
	ctk_tree_view_set_model (CTK_TREE_VIEW (treeview), CTK_TREE_MODEL (store));
	g_object_unref (store);

	priv->list_store = CTK_TREE_MODEL (store);

	/* Add columns */
	priv->toggle_renderer = renderer = ctk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled",
			  G_CALLBACK (save_toggled), store);

	column = ctk_tree_view_column_new_with_attributes ("Save?",
							   renderer,
							   "active",
							   SAVE_COLUMN,
							   NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (treeview), column);

	renderer = ctk_cell_renderer_pixbuf_new ();

	column = ctk_tree_view_column_new_with_attributes ("Image",
							   renderer,
							   "pixbuf",
							   IMAGE_COLUMN,
							   NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (treeview), column);



	renderer = ctk_cell_renderer_text_new ();
	column = ctk_tree_view_column_new_with_attributes ("Name",
							   renderer,
							   "text",
							   NAME_COLUMN,
							   NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (treeview), column);

	return treeview;
}

static void
build_multiple_imgs_dialog (EocCloseConfirmationDialog *dlg)
{
	EocCloseConfirmationDialogPrivate *priv;
	CtkWidget *hbox;
	CtkWidget *image;
	CtkWidget *vbox;
	CtkWidget *primary_label;
	CtkWidget *vbox2;
	CtkWidget *select_label;
	CtkWidget *scrolledwindow;
	CtkWidget *treeview;
	CtkWidget *secondary_label;
	gchar     *str;
	gchar     *markup_str;

	priv = dlg->priv;

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
	ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
  	ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dlg))),
			    hbox, TRUE, TRUE, 0);

	/* Image */
	image = ctk_image_new_from_icon_name ("dialog-warning",
					  CTK_ICON_SIZE_DIALOG);
	ctk_widget_set_valign (image, CTK_ALIGN_START);
	ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
	ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	/* Primary label */
	primary_label = ctk_label_new (NULL);
	ctk_label_set_line_wrap (CTK_LABEL (primary_label), TRUE);
	ctk_label_set_use_markup (CTK_LABEL (primary_label), TRUE);
	ctk_label_set_max_width_chars (CTK_LABEL (primary_label), 88);
	ctk_label_set_xalign (CTK_LABEL (primary_label), 0.0);
	ctk_label_set_selectable (CTK_LABEL (primary_label), TRUE);

	str = g_strdup_printf (
			ngettext ("There is %d image with unsaved changes. "
				  "Save changes before closing?",
				  "There are %d images with unsaved changes. "
				  "Save changes before closing?",
				  g_list_length (priv->unsaved_images)),
			g_list_length (priv->unsaved_images));

	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);

	ctk_label_set_markup (CTK_LABEL (primary_label), markup_str);
	g_free (markup_str);
	ctk_box_pack_start (CTK_BOX (vbox), primary_label, FALSE, FALSE, 0);

	vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
	ctk_box_pack_start (CTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

	select_label = ctk_label_new_with_mnemonic (_("S_elect the images you want to save:"));

	ctk_box_pack_start (CTK_BOX (vbox2), select_label, FALSE, FALSE, 0);
	ctk_label_set_line_wrap (CTK_LABEL (select_label), TRUE);
	ctk_label_set_xalign (CTK_LABEL (select_label), 0.0);

	scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_box_pack_start (CTK_BOX (vbox2), scrolledwindow, TRUE, TRUE, 0);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolledwindow),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolledwindow),
					     CTK_SHADOW_IN);

	treeview = create_treeview (priv);
	ctk_container_add (CTK_CONTAINER (scrolledwindow), treeview);
	ctk_widget_set_size_request (scrolledwindow, 260, 120);

	/* Secondary label */
	secondary_label = ctk_label_new (_("If you don't save, "
					   "all your changes will be lost."));

	ctk_box_pack_start (CTK_BOX (vbox2), secondary_label, FALSE, FALSE, 0);
	ctk_label_set_line_wrap (CTK_LABEL (secondary_label), TRUE);
	ctk_label_set_xalign (CTK_LABEL (secondary_label), 0.0);
	ctk_label_set_selectable (CTK_LABEL (secondary_label), TRUE);

	ctk_label_set_mnemonic_widget (CTK_LABEL (select_label), treeview);

	add_buttons (dlg);

	ctk_widget_show_all (hbox);
}

static void
set_unsaved_image (EocCloseConfirmationDialog *dlg,
		   const GList                *list)
{
	EocCloseConfirmationDialogPrivate *priv;

	g_return_if_fail (list != NULL);

	priv = dlg->priv;
	g_return_if_fail (priv->unsaved_images == NULL);

	priv->unsaved_images = g_list_copy ((GList *)list);

	if (GET_MODE (priv) == SINGLE_IMG_MODE)
	{
		build_single_img_dialog (dlg);
	}
	else
	{
		build_multiple_imgs_dialog (dlg);
	}
}

const GList *
eoc_close_confirmation_dialog_get_unsaved_images (EocCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (EOC_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return dlg->priv->unsaved_images;
}

