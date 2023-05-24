/*
 * eoc-metadata-sidebar.c
 * This file is part of eoc
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Portions based on code by: Lucas Rocha <lucasr@gnome.org>
 *                            Hubert Figuiere <hub@figuiere.net> (XMP support)
 *
 * Copyright (C) 2011 GNOME Foundation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "eoc-image.h"
#include "eoc-metadata-sidebar.h"
#include "eoc-properties-dialog.h"
#include "eoc-scroll-view.h"
#include "eoc-util.h"
#include "eoc-window.h"

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#include <libexif/exif-tag.h>
#include "eoc-exif-util.h"
#endif

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>
#endif

/* There's no exempi support in the sidebar yet */
#if HAVE_EXIF  /*|| HAVE_EXEMPI */
#define HAVE_METADATA 1
#endif

enum {
	PROP_0,
	PROP_IMAGE,
	PROP_PARENT_WINDOW
};

struct _EocMetadataSidebarPrivate {
	EocWindow *parent_window;
	EocImage *image;

	gulong image_changed_id;
	gulong thumb_changed_id;

	CtkWidget *size_label;
	CtkWidget *type_label;
	CtkWidget *filesize_label;
	CtkWidget *folder_label;

#if HAVE_EXIF
	CtkWidget *aperture_label;
	CtkWidget *exposure_label;
	CtkWidget *focallen_label;
	CtkWidget *iso_label;
	CtkWidget *metering_label;
	CtkWidget *model_label;
	CtkWidget *date_label;
	CtkWidget *time_label;
#else
	CtkWidget *metadata_grid;
#endif

#if HAVE_METADATA
	CtkWidget *details_button;
#endif
};

G_DEFINE_TYPE_WITH_PRIVATE(EocMetadataSidebar, eoc_metadata_sidebar, CTK_TYPE_SCROLLED_WINDOW)

static void
parent_file_display_name_query_info_cb (GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
	EocMetadataSidebar *sidebar = EOC_METADATA_SIDEBAR (user_data);
	GFile *parent_file = G_FILE (source_object);
	GFileInfo *file_info;
	gchar *baseuri;
	gchar *display_name;
	gchar *str;

	file_info = g_file_query_info_finish (parent_file, res, NULL);
	if (file_info == NULL) {
		display_name = g_file_get_basename (parent_file);
	} else {
		display_name = g_strdup (
			g_file_info_get_display_name (file_info));
		g_object_unref (file_info);
	}
	baseuri = g_file_get_uri (parent_file);
	str = g_markup_printf_escaped ("<a href=\"%s\">%s</a>",
				       baseuri,
				       display_name);
	ctk_label_set_markup (CTK_LABEL (sidebar->priv->folder_label), str);

	g_free (str);
	g_free (baseuri);
	g_free (display_name);

	str = g_file_get_path (parent_file);
	ctk_widget_set_tooltip_text (CTK_WIDGET (sidebar->priv->folder_label), str);
	g_free (str);

	g_object_unref (sidebar);
}

static void
eoc_metadata_sidebar_update_general_section (EocMetadataSidebar *sidebar)
{
	EocMetadataSidebarPrivate *priv = sidebar->priv;
	EocImage *img = priv->image;
	GFile *file, *parent_file;
	GFileInfo *file_info;
	gchar *str;
	goffset bytes;
	gint width, height;

	if (G_UNLIKELY (img == NULL)) {
		ctk_label_set_text (CTK_LABEL (priv->size_label), NULL);
		ctk_label_set_text (CTK_LABEL (priv->type_label), NULL);
		ctk_label_set_text (CTK_LABEL (priv->filesize_label), NULL);
		ctk_label_set_text (CTK_LABEL (priv->folder_label), NULL);
		return;
	}

	eoc_image_get_size (img, &width, &height);
	str = g_strdup_printf (ngettext("%i × %i pixel",
					"%i × %i pixels", height),
			       width, height);
	ctk_label_set_text (CTK_LABEL (priv->size_label), str);
	g_free (str);

	file = eoc_image_get_file (img);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL) {
		str = g_strdup (_("Unknown"));
	} else {
		const gchar *mime_str;

		mime_str = g_file_info_get_content_type (file_info);
		str = g_content_type_get_description (mime_str);
		g_object_unref (file_info);
	}
	ctk_label_set_text (CTK_LABEL (priv->type_label), str);
	g_free (str);

	bytes = eoc_image_get_bytes (img);
	str = g_format_size (bytes);
	ctk_label_set_text (CTK_LABEL (priv->filesize_label), str);
	g_free (str);

	parent_file = g_file_get_parent (file);
	if (parent_file == NULL) {
		/* file is root directory itself */
		parent_file = g_object_ref (file);
	}
	ctk_label_set_markup (CTK_LABEL (sidebar->priv->folder_label), NULL);
	g_file_query_info_async (parent_file,
				 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 NULL,
				 parent_file_display_name_query_info_cb,
				 g_object_ref (sidebar));

	g_object_unref (parent_file);
}

#if HAVE_METADATA
static void
eoc_metadata_sidebar_update_metadata_section (EocMetadataSidebar *sidebar)
{
	EocMetadataSidebarPrivate *priv = sidebar->priv;
#if HAVE_EXIF
	EocImage *img = priv->image;
	ExifData *exif_data = NULL;

	if (img) {
		exif_data = eoc_image_get_exif_info (img);
	}

	eoc_exif_util_set_label_text (CTK_LABEL (priv->aperture_label),
				      exif_data, EXIF_TAG_FNUMBER);
	eoc_exif_util_set_label_text (CTK_LABEL (priv->exposure_label),
				      exif_data,
				      EXIF_TAG_EXPOSURE_TIME);
	eoc_exif_util_set_focal_length_label_text (
				       CTK_LABEL (priv->focallen_label),
				       exif_data);
	eoc_exif_util_set_label_text (CTK_LABEL (priv->iso_label),
				      exif_data,
				      EXIF_TAG_ISO_SPEED_RATINGS);
	eoc_exif_util_set_label_text (CTK_LABEL (priv->metering_label),
				      exif_data,
				      EXIF_TAG_METERING_MODE);
	eoc_exif_util_set_label_text (CTK_LABEL (priv->model_label),
				      exif_data, EXIF_TAG_MODEL);
	eoc_exif_util_format_datetime_label (CTK_LABEL (priv->date_label),
				      exif_data,
				      EXIF_TAG_DATE_TIME_ORIGINAL,
				      _("%a, %d %B %Y"));
	eoc_exif_util_format_datetime_label (CTK_LABEL (priv->time_label),
				      exif_data,
				      EXIF_TAG_DATE_TIME_ORIGINAL,
				      _("%X"));

	/* exif_data_unref can handle NULL-values */
	exif_data_unref(exif_data);
#endif /* HAVE_EXIF */
}
#endif /* HAVE_METADATA */

static void
eoc_metadata_sidebar_update (EocMetadataSidebar *sidebar)
{
	g_return_if_fail (EOC_IS_METADATA_SIDEBAR (sidebar));

	eoc_metadata_sidebar_update_general_section (sidebar);
#if HAVE_METADATA
	eoc_metadata_sidebar_update_metadata_section (sidebar);
#endif
}

static void
_thumbnail_changed_cb (EocImage *image, gpointer user_data)
{
	eoc_metadata_sidebar_update (EOC_METADATA_SIDEBAR (user_data));
}

static void
eoc_metadata_sidebar_set_image (EocMetadataSidebar *sidebar, EocImage *image)
{
	EocMetadataSidebarPrivate *priv = sidebar->priv;

	if (image == priv->image)
		return;


	if (priv->thumb_changed_id != 0) {
		g_signal_handler_disconnect (priv->image,
					     priv->thumb_changed_id);
		priv->thumb_changed_id = 0;
	}

	if (priv->image)
		g_object_unref (priv->image);

	priv->image = image;

	if (priv->image) {
		g_object_ref (priv->image);
		priv->thumb_changed_id =
			g_signal_connect (priv->image, "thumbnail-changed",
					  G_CALLBACK (_thumbnail_changed_cb),
					  sidebar);
		eoc_metadata_sidebar_update (sidebar);
	}

	g_object_notify (G_OBJECT (sidebar), "image");
}

static void
_notify_image_cb (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	EocImage *image;

	g_return_if_fail (EOC_IS_METADATA_SIDEBAR (user_data));
	g_return_if_fail (EOC_IS_SCROLL_VIEW (gobject));

	image = eoc_scroll_view_get_image (EOC_SCROLL_VIEW (gobject));

	eoc_metadata_sidebar_set_image (EOC_METADATA_SIDEBAR (user_data),
					image);

	if (image)
		g_object_unref (image);
}

static void
_folder_label_clicked_cb (CtkLabel *label, const gchar *uri, gpointer user_data)
{
	EocMetadataSidebarPrivate *priv = EOC_METADATA_SIDEBAR(user_data)->priv;
	EocImage *img;
	CtkWidget *toplevel;
	CtkWindow *window;
	GFile *file;

	g_return_if_fail (priv->parent_window != NULL);

	img = eoc_window_get_image (priv->parent_window);
	file = eoc_image_get_file (img);

	toplevel = ctk_widget_get_toplevel (CTK_WIDGET (label));
	if (CTK_IS_WINDOW (toplevel))
		window = CTK_WINDOW (toplevel);
	else
		window = NULL;

	eoc_util_show_file_in_filemanager (file, window);

	g_object_unref (file);
}

#ifdef HAVE_METADATA
static void
_details_button_clicked_cb (CtkButton *button, gpointer user_data)
{
	EocMetadataSidebarPrivate *priv = EOC_METADATA_SIDEBAR(user_data)->priv;
	CtkWidget *dlg;

	g_return_if_fail (priv->parent_window != NULL);

	dlg = eoc_window_get_properties_dialog (
					EOC_WINDOW (priv->parent_window));
	g_return_if_fail (dlg != NULL);
	eoc_properties_dialog_set_page (EOC_PROPERTIES_DIALOG (dlg),
					EOC_PROPERTIES_DIALOG_PAGE_DETAILS);
	ctk_widget_show (dlg);
}
#endif /* HAVE_METADATA */

static void
eoc_metadata_sidebar_set_parent_window (EocMetadataSidebar *sidebar,
					EocWindow *window)
{
	EocMetadataSidebarPrivate *priv;
	CtkWidget *view;

	g_return_if_fail (EOC_IS_METADATA_SIDEBAR (sidebar));
	priv = sidebar->priv;
	g_return_if_fail (priv->parent_window == NULL);

	priv->parent_window = g_object_ref (window);
	eoc_metadata_sidebar_update (sidebar);
	view = eoc_window_get_view (window);
	priv->image_changed_id = g_signal_connect (view, "notify::image",
						  G_CALLBACK (_notify_image_cb),
						  sidebar);

	g_object_notify (G_OBJECT (sidebar), "parent-window");

}

static void
eoc_metadata_sidebar_init (EocMetadataSidebar *sidebar)
{
	EocMetadataSidebarPrivate *priv;

	priv = sidebar->priv = eoc_metadata_sidebar_get_instance_private (sidebar);

	ctk_widget_init_template (CTK_WIDGET (sidebar));

	g_signal_connect (priv->folder_label, "activate-link",
	                  G_CALLBACK (_folder_label_clicked_cb), sidebar);

#if HAVE_METADATA
	g_signal_connect (priv->details_button, "clicked",
			  G_CALLBACK (_details_button_clicked_cb), sidebar);
#endif /* HAVE_METADATA */

#ifndef HAVE_EXIF
	{
		/* Remove the lower 8 lines as they are empty without libexif*/
		guint i;

		for (i = 11; i > 3; i--)
		{
			ctk_grid_remove_row (CTK_GRID (priv->metadata_grid), i);
		}
	}
#endif /* !HAVE_EXIF */
}

static void
eoc_metadata_sidebar_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
	EocMetadataSidebar *sidebar;

	g_return_if_fail (EOC_IS_METADATA_SIDEBAR (object));

	sidebar = EOC_METADATA_SIDEBAR (object);

	switch (property_id) {
	case PROP_IMAGE:
	{
		g_value_set_object (value, sidebar->priv->image);
		break;
	}
	case PROP_PARENT_WINDOW:
		g_value_set_object (value, sidebar->priv->parent_window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eoc_metadata_sidebar_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
	EocMetadataSidebar *sidebar;

	g_return_if_fail (EOC_IS_METADATA_SIDEBAR (object));

	sidebar = EOC_METADATA_SIDEBAR (object);

	switch (property_id) {
	case PROP_IMAGE:
	{
		break;
	}
	case PROP_PARENT_WINDOW:
	{
		EocWindow *window;

		window = g_value_get_object (value);
		eoc_metadata_sidebar_set_parent_window (sidebar, window);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}

}
static void
eoc_metadata_sidebar_class_init (EocMetadataSidebarClass *klass)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (klass);
	CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

	g_obj_class->get_property = eoc_metadata_sidebar_get_property;
	g_obj_class->set_property = eoc_metadata_sidebar_set_property;
/*	g_obj_class->dispose = eoc_metadata_sidebar_dispose;*/

	g_object_class_install_property (
		g_obj_class, PROP_PARENT_WINDOW,
		g_param_spec_object ("parent-window", NULL, NULL,
				     EOC_TYPE_WINDOW, G_PARAM_READWRITE
				     | G_PARAM_CONSTRUCT_ONLY
				     | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		g_obj_class, PROP_IMAGE,
		g_param_spec_object ("image", NULL, NULL, EOC_TYPE_IMAGE,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
				    );

	ctk_widget_class_set_template_from_resource (widget_class,
						     "/org/cafe/eoc/ui/metadata-sidebar.ui");

	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      size_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      type_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      filesize_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      folder_label);
#if HAVE_EXIF
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      aperture_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      exposure_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      focallen_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      iso_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      metering_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      model_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      date_label);
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      time_label);
#else
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      metadata_grid);
#endif /* HAVE_EXIF */
#if HAVE_METADATA
	ctk_widget_class_bind_template_child_private (widget_class,
						      EocMetadataSidebar,
						      details_button);
#endif /* HAVE_METADATA */
}


CtkWidget*
eoc_metadata_sidebar_new (EocWindow *window)
{
	return ctk_widget_new (EOC_TYPE_METADATA_SIDEBAR,
			       "hadjustment", NULL,
			       "vadjustment", NULL,
			       "hscrollbar-policy", CTK_POLICY_NEVER,
			       "vscrollbar-policy", CTK_POLICY_AUTOMATIC,
			       "parent-window", window,
			       NULL);
}
