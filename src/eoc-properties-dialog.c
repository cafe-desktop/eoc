/* Eye Of Cafe - Image Properties Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *         Hubert Figuiere <hub@figuiere.net> (XMP support)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eoc-properties-dialog.h"
#include "eoc-image.h"
#include "eoc-util.h"
#include "eoc-thumb-view.h"

#if HAVE_EXIF
#include "eoc-exif-util.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>
#endif
#if HAVE_EXIF || HAVE_EXEMPI
#define HAVE_METADATA 1
#endif

#if HAVE_METADATA
#include "eoc-metadata-details.h"
#endif

enum {
        PROP_0,
        PROP_THUMBVIEW,
        PROP_NETBOOK_MODE
};

struct _EocPropertiesDialogPrivate {
	EocThumbView   *thumbview;

	gboolean        update_page;
	EocPropertiesDialogPage current_page;

	GtkWidget      *notebook;
	GtkWidget      *close_button;
	GtkWidget      *next_button;
	GtkWidget      *previous_button;

	GtkWidget      *general_box;
	GtkWidget      *thumbnail_image;
	GtkWidget      *name_label;
	GtkWidget      *width_label;
	GtkWidget      *height_label;
	GtkWidget      *type_label;
	GtkWidget      *bytes_label;
	GtkWidget      *folder_button;
	gchar          *folder_button_uri;
#ifdef HAVE_EXIF
	GtkWidget      *exif_aperture_label;
	GtkWidget      *exif_exposure_label;
	GtkWidget      *exif_focal_label;
	GtkWidget      *exif_flash_label;
	GtkWidget      *exif_iso_label;
	GtkWidget      *exif_metering_label;
	GtkWidget      *exif_model_label;
	GtkWidget      *exif_date_label;
#endif
#ifdef HAVE_EXEMPI
	GtkWidget      *xmp_location_label;
	GtkWidget      *xmp_description_label;
	GtkWidget      *xmp_keywords_label;
	GtkWidget      *xmp_creator_label;
	GtkWidget      *xmp_rights_label;
#else
	GtkWidget      *xmp_box;
	GtkWidget      *xmp_box_label;
#endif
#if HAVE_METADATA
	GtkWidget      *metadata_box;
	GtkWidget      *metadata_details_expander;
	GtkWidget      *metadata_details;
	GtkWidget      *metadata_details_box;
	GtkWidget      *metadata_details_sw;
#endif

	gboolean        netbook_mode;
};

G_DEFINE_TYPE_WITH_PRIVATE (EocPropertiesDialog, eoc_properties_dialog, GTK_TYPE_DIALOG);

static void
parent_file_display_name_query_info_cb (GObject *source_object,
					GAsyncResult *res,
					gpointer user_data)
{
	EocPropertiesDialog *prop_dlg = EOC_PROPERTIES_DIALOG (user_data);
	GFile *parent_file = G_FILE (source_object);
	GFileInfo *file_info;
	gchar *display_name;


	file_info = g_file_query_info_finish (parent_file, res, NULL);
	if (file_info == NULL) {
		display_name = g_file_get_basename (parent_file);
	} else {
		display_name = g_strdup (
			g_file_info_get_display_name (file_info));
		g_object_unref (file_info);
	}
	ctk_button_set_label (GTK_BUTTON (prop_dlg->priv->folder_button),
			      display_name);
	ctk_widget_set_sensitive (prop_dlg->priv->folder_button, TRUE);

	g_free (display_name);
	g_object_unref (prop_dlg);
}

static void
pd_update_general_tab (EocPropertiesDialog *prop_dlg,
		       EocImage            *image)
{
	gchar *bytes_str, *dir_str_long;
	gchar *width_str, *height_str;
	GFile *file, *parent_file;
	GFileInfo *file_info;
	const char *mime_str;
	char *type_str;
	gint width, height;
	goffset bytes;

	g_object_set (G_OBJECT (prop_dlg->priv->thumbnail_image),
		      "pixbuf", eoc_image_get_thumbnail (image),
		      NULL);

	ctk_label_set_text (GTK_LABEL (prop_dlg->priv->name_label),
			    eoc_image_get_caption (image));

	eoc_image_get_size (image, &width, &height);

	width_str = g_strdup_printf ("%d %s", width,
				     ngettext ("pixel", "pixels", width));
	height_str = g_strdup_printf ("%d %s", height,
				      ngettext ("pixel", "pixels", height));

	ctk_label_set_text (GTK_LABEL (prop_dlg->priv->width_label), width_str);

	ctk_label_set_text (GTK_LABEL (prop_dlg->priv->height_label),
			    height_str);

	g_free (height_str);
	g_free (width_str);

	file = eoc_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL) {
		type_str = g_strdup (_("Unknown"));
	} else {
		mime_str = g_file_info_get_content_type (file_info);
		type_str = g_content_type_get_description (mime_str);
		g_object_unref (file_info);
	}

	ctk_label_set_text (GTK_LABEL (prop_dlg->priv->type_label), type_str);

	bytes = eoc_image_get_bytes (image);
	bytes_str = g_format_size (bytes);

	ctk_label_set_text (GTK_LABEL (prop_dlg->priv->bytes_label), bytes_str);

	parent_file = g_file_get_parent (file);
	if (parent_file == NULL) {
		/* file is root directory itself */
		parent_file = g_object_ref (file);
	}

	ctk_widget_set_sensitive (prop_dlg->priv->folder_button, FALSE);
	ctk_button_set_label (GTK_BUTTON (prop_dlg->priv->folder_button), NULL);

	dir_str_long = g_file_get_path (parent_file);
	ctk_widget_set_tooltip_text (GTK_WIDGET (prop_dlg->priv->folder_button),
	                             dir_str_long);

	g_free (prop_dlg->priv->folder_button_uri);
	prop_dlg->priv->folder_button_uri = g_file_get_uri (parent_file);

	g_file_query_info_async (parent_file,
				 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 NULL,
				 parent_file_display_name_query_info_cb,
				 g_object_ref (prop_dlg));

	g_object_unref (parent_file);

	g_free (type_str);
	g_free (bytes_str);
	g_free (dir_str_long);
}

#if HAVE_EXEMPI
static void
eoc_xmp_set_label (XmpPtr xmp,
		   const char *ns,
		   const char *propname,
		   GtkWidget *w)
{
	uint32_t options;

	XmpStringPtr value = xmp_string_new ();

	if (xmp_get_property (xmp, ns, propname, value, &options)) {
		if (XMP_IS_PROP_SIMPLE (options)) {
			ctk_label_set_text (GTK_LABEL (w), xmp_string_cstr (value));
		} else if (XMP_IS_PROP_ARRAY (options)) {
			XmpIteratorPtr iter = xmp_iterator_new (xmp,
							        ns,
								propname,
								XMP_ITER_JUSTLEAFNODES);

			GString *string = g_string_new ("");

			if (iter) {
				gboolean first = TRUE;

				while (xmp_iterator_next (iter, NULL, NULL, value, &options)
				       && !XMP_IS_PROP_QUALIFIER (options)) {

					if (!first) {
						g_string_append_printf(string, ", ");
					} else {
						first = FALSE;
					}

					g_string_append_printf (string,
								"%s",
								xmp_string_cstr (value));
				}

				xmp_iterator_free (iter);
			}

			ctk_label_set_text (GTK_LABEL (w), string->str);
			g_string_free (string, TRUE);
		}
	} else {
		/* Property was not found */
		/* Clear label so it won't show bogus data */
		ctk_label_set_text (GTK_LABEL (w), NULL);
	}

	xmp_string_free (value);
}
#endif

#if HAVE_METADATA
static void
pd_update_metadata_tab (EocPropertiesDialog *prop_dlg,
			EocImage            *image)
{
	EocPropertiesDialogPrivate *priv;
	GtkNotebook *notebook;
#if HAVE_EXIF
	ExifData    *exif_data;
#endif
#if HAVE_EXEMPI
	XmpPtr      xmp_data;
#endif

	g_return_if_fail (EOC_IS_PROPERTIES_DIALOG (prop_dlg));

	priv = prop_dlg->priv;

	notebook = GTK_NOTEBOOK (priv->notebook);

	if (TRUE
#if HAVE_EXIF
	    && !eoc_image_has_data (image, EOC_IMAGE_DATA_EXIF)
#endif
#if HAVE_EXEMPI
	    && !eoc_image_has_data (image, EOC_IMAGE_DATA_XMP)
#endif
	    ) {
		if (ctk_notebook_get_current_page (notebook) ==	EOC_PROPERTIES_DIALOG_PAGE_EXIF) {
			ctk_notebook_prev_page (notebook);
		} else if (ctk_notebook_get_current_page (notebook) == EOC_PROPERTIES_DIALOG_PAGE_DETAILS) {
			ctk_notebook_set_current_page (notebook, EOC_PROPERTIES_DIALOG_PAGE_GENERAL);
		}

		if (ctk_widget_get_visible (priv->metadata_box)) {
			ctk_widget_hide (priv->metadata_box);
		}
		if (ctk_widget_get_visible (priv->metadata_details_box)) {
			ctk_widget_hide (priv->metadata_details_box);
		}

		return;
	} else {
		if (!ctk_widget_get_visible (priv->metadata_box))
			ctk_widget_show_all (priv->metadata_box);
		if (priv->netbook_mode &&
		    !ctk_widget_get_visible (priv->metadata_details_box)) {
			ctk_widget_show_all (priv->metadata_details_box);
			ctk_widget_hide (priv->metadata_details_expander);
		}
	}

#if HAVE_EXIF
	exif_data = (ExifData *) eoc_image_get_exif_info (image);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_aperture_label),
				      exif_data, EXIF_TAG_FNUMBER);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_exposure_label),
				      exif_data, EXIF_TAG_EXPOSURE_TIME);

	eoc_exif_util_set_focal_length_label_text (GTK_LABEL (priv->exif_focal_label), exif_data);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_flash_label),
				      exif_data, EXIF_TAG_FLASH);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_iso_label),
				      exif_data, EXIF_TAG_ISO_SPEED_RATINGS);


	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_metering_label),
				      exif_data, EXIF_TAG_METERING_MODE);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_model_label),
				      exif_data, EXIF_TAG_MODEL);

	eoc_exif_util_set_label_text (GTK_LABEL (priv->exif_date_label),
				      exif_data, EXIF_TAG_DATE_TIME_ORIGINAL);

	eoc_metadata_details_update (EOC_METADATA_DETAILS (priv->metadata_details),
				 exif_data);

	/* exif_data_unref can handle NULL-values */
	exif_data_unref(exif_data);
#endif

#if HAVE_EXEMPI
	xmp_data = (XmpPtr) eoc_image_get_xmp_info (image);

 	if (xmp_data != NULL) {
		eoc_xmp_set_label (xmp_data,
				   NS_IPTC4XMP,
				   "Location",
				   priv->xmp_location_label);

		eoc_xmp_set_label (xmp_data,
				   NS_DC,
				   "description",
				   priv->xmp_description_label);

		eoc_xmp_set_label (xmp_data,
				   NS_DC,
				   "subject",
				   priv->xmp_keywords_label);

		eoc_xmp_set_label (xmp_data,
				   NS_DC,
        	                   "creator",
				   priv->xmp_creator_label);

		eoc_xmp_set_label (xmp_data,
				   NS_DC,
				   "rights",
				   priv->xmp_rights_label);

		eoc_metadata_details_xmp_update (EOC_METADATA_DETAILS (priv->metadata_details), xmp_data);

		xmp_free (xmp_data);
	} else {
		/* Image has no XMP data */

		/* Clear the labels so they won't display foreign data.*/

		ctk_label_set_text (GTK_LABEL (priv->xmp_location_label), NULL);
		ctk_label_set_text (GTK_LABEL (priv->xmp_description_label),
				    NULL);
		ctk_label_set_text (GTK_LABEL (priv->xmp_keywords_label), NULL);
		ctk_label_set_text (GTK_LABEL (priv->xmp_creator_label), NULL);
		ctk_label_set_text (GTK_LABEL (priv->xmp_rights_label), NULL);
	}
#endif
}

static gboolean
pd_resize_dialog (gpointer user_data)
{
	gint width, height;

	ctk_window_get_size (GTK_WINDOW (user_data),
			     &width,
			     &height);

	ctk_window_resize (GTK_WINDOW (user_data), width, 1);

	return FALSE;
}

static void
pd_exif_details_activated_cb (GtkExpander *expander,
			      GParamSpec *param_spec,
			      GtkWidget *dialog)
{
	gboolean expanded;

	expanded = ctk_expander_get_expanded (expander);

	/*FIXME: this is depending on the expander animation
         * duration. Need to find a safer way for doing that. */
	if (!expanded)
		g_timeout_add (150, pd_resize_dialog, dialog);
}
#endif

static void
pd_folder_button_clicked_cb (GtkButton *button, gpointer data)
{
	EocPropertiesDialogPrivate *priv = EOC_PROPERTIES_DIALOG (data)->priv;
	GtkWindow *window;
	guint32 timestamp;

	if (!priv->folder_button_uri)
		return;

	timestamp = ctk_get_current_event_time ();

	window = GTK_WINDOW (data);
	ctk_show_uri_on_window (window, priv->folder_button_uri, timestamp, NULL);
}

static gboolean
eoc_properties_dialog_page_switch (GtkNotebook     *notebook,
				   gpointer         page,
				   guint            page_index,
				   EocPropertiesDialog *prop_dlg)
{
	if (prop_dlg->priv->update_page)
		prop_dlg->priv->current_page = page_index;

	return TRUE;
}

void
eoc_properties_dialog_set_netbook_mode (EocPropertiesDialog *dlg,
					gboolean enable)
{
	EocPropertiesDialogPrivate *priv;

	g_return_if_fail (EOC_IS_PROPERTIES_DIALOG (dlg));

	priv = dlg->priv;

	if (priv->netbook_mode == enable)
		return;

	priv->netbook_mode = enable;

#ifdef HAVE_METADATA
	if (enable) {
		g_object_ref (priv->metadata_details_sw);
		ctk_container_remove (GTK_CONTAINER (ctk_widget_get_parent (priv->metadata_details_sw)),
				      priv->metadata_details_sw);
		ctk_container_add (GTK_CONTAINER (priv->metadata_details_box), priv->metadata_details_sw);
		g_object_unref (priv->metadata_details_sw);
		// Only show details box if metadata is being displayed
		if (ctk_widget_get_visible (priv->metadata_box))
			ctk_widget_show_all (priv->metadata_details_box);

		ctk_widget_hide (priv->metadata_details_expander);
	} else {
		g_object_ref (priv->metadata_details_sw);
		ctk_container_remove (GTK_CONTAINER (ctk_widget_get_parent (priv->metadata_details_sw)),
				      priv->metadata_details_sw);
		ctk_container_add (GTK_CONTAINER (priv->metadata_details_expander), priv->metadata_details_sw);
		g_object_unref (priv->metadata_details_sw);
		ctk_widget_show_all (priv->metadata_details_expander);

		if (ctk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == EOC_PROPERTIES_DIALOG_PAGE_DETAILS)
			ctk_notebook_prev_page (GTK_NOTEBOOK (priv->notebook));
		ctk_widget_hide (priv->metadata_details_box);
	}
#endif
}

static void
eoc_properties_dialog_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	EocPropertiesDialog *prop_dlg = EOC_PROPERTIES_DIALOG (object);

	switch (prop_id) {
		case PROP_THUMBVIEW:
			prop_dlg->priv->thumbview = g_value_get_object (value);
			break;
		case PROP_NETBOOK_MODE:
			eoc_properties_dialog_set_netbook_mode (prop_dlg,
						   g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
							   pspec);
			break;
	}
}

static void
eoc_properties_dialog_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	EocPropertiesDialog *prop_dlg = EOC_PROPERTIES_DIALOG (object);

	switch (prop_id) {
		case PROP_THUMBVIEW:
			g_value_set_object (value, prop_dlg->priv->thumbview);
			break;
		case PROP_NETBOOK_MODE:
			g_value_set_boolean (value,
					     prop_dlg->priv->netbook_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
							   pspec);
			break;
	}
}

static void
eoc_properties_dialog_dispose (GObject *object)
{
	EocPropertiesDialog *prop_dlg;
	EocPropertiesDialogPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (EOC_IS_PROPERTIES_DIALOG (object));

	prop_dlg = EOC_PROPERTIES_DIALOG (object);
	priv = prop_dlg->priv;

	if (priv->thumbview) {
		g_object_unref (priv->thumbview);
		priv->thumbview = NULL;
	}

	g_free (priv->folder_button_uri);
	priv->folder_button_uri = NULL;

	G_OBJECT_CLASS (eoc_properties_dialog_parent_class)->dispose (object);
}

static void
eoc_properties_dialog_class_init (EocPropertiesDialogClass *klass)
{
	GObjectClass *g_object_class = (GObjectClass *) klass;

	g_object_class->dispose = eoc_properties_dialog_dispose;
	g_object_class->set_property = eoc_properties_dialog_set_property;
	g_object_class->get_property = eoc_properties_dialog_get_property;

	g_object_class_install_property (g_object_class,
					 PROP_THUMBVIEW,
					 g_param_spec_object ("thumbview",
							      "Thumbview",
							      "Thumbview",
							      EOC_TYPE_THUMB_VIEW,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_NAME |
							      G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (g_object_class, PROP_NETBOOK_MODE,
					 g_param_spec_boolean ("netbook-mode",
					 		      "Netbook Mode",
							      "Netbook Mode",
							      FALSE,
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));

	GtkWidgetClass *wklass = (GtkWidgetClass*) klass;

	ctk_widget_class_set_template_from_resource (wklass,
	                                             "/org/cafe/eoc/ui/eoc-image-properties-dialog.ui");

	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     notebook);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     previous_button);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     next_button);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     close_button);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     thumbnail_image);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     general_box);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     name_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     width_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     height_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     type_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     bytes_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     folder_button);

#if HAVE_EXIF
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_aperture_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_exposure_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_focal_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_flash_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_iso_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_metering_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_model_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     exif_date_label);
#endif
#if HAVE_EXEMPI
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_location_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_description_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_keywords_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_creator_label);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_rights_label);
#else
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_box);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     xmp_box_label);
#endif
#ifdef HAVE_METADATA
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     metadata_box);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     metadata_details_expander);
	ctk_widget_class_bind_template_child_private(wklass,
						     EocPropertiesDialog,
						     metadata_details_box);

	ctk_widget_class_bind_template_callback(wklass,
						pd_exif_details_activated_cb);
#endif
	ctk_widget_class_bind_template_callback(wklass,
						eoc_properties_dialog_page_switch);
	ctk_widget_class_bind_template_callback(wklass,
						pd_folder_button_clicked_cb);
}

static void
eoc_properties_dialog_init (EocPropertiesDialog *prop_dlg)
{
	EocPropertiesDialogPrivate *priv;
#if HAVE_METADATA
	GtkWidget *sw;
#endif

	prop_dlg->priv = eoc_properties_dialog_get_instance_private (prop_dlg);

	priv = prop_dlg->priv;

	priv->update_page = FALSE;

	ctk_widget_init_template (GTK_WIDGET (prop_dlg));

	g_signal_connect (prop_dlg,
	                  "delete-event",
	                 G_CALLBACK (ctk_widget_hide_on_delete),
	                 prop_dlg);

	g_signal_connect_swapped (priv->close_button,
	                          "clicked",
	                          G_CALLBACK (ctk_widget_hide_on_delete),
	                          prop_dlg);

	ctk_widget_set_size_request (priv->thumbnail_image, 100, 100);

#ifdef HAVE_METADATA
 	sw = ctk_scrolled_window_new (NULL, NULL);

	ctk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);

	ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	priv->metadata_details = eoc_metadata_details_new ();
	ctk_widget_set_size_request (priv->metadata_details, -1, 170);
	ctk_container_set_border_width (GTK_CONTAINER (sw), 6);
	ctk_widget_set_vexpand (priv->metadata_details, TRUE);

	ctk_container_add (GTK_CONTAINER (sw), priv->metadata_details);
	ctk_widget_show_all (sw);

	priv->metadata_details_sw = sw;

	if (priv->netbook_mode) {
		ctk_widget_hide (priv->metadata_details_expander);
		ctk_box_pack_start (GTK_BOX (priv->metadata_details_box),
				    sw, TRUE, TRUE, 6);
	} else {
		ctk_container_add (GTK_CONTAINER (priv->metadata_details_expander),
				   sw);
	}

#ifndef HAVE_EXEMPI
	ctk_widget_hide (priv->xmp_box);
	ctk_widget_hide (priv->xmp_box_label);
#endif

#else
	/* Remove pages from back to front. Otherwise the page index
	 * needs to be adjusted when deleting the next page. */
	ctk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook),
				  EOC_PROPERTIES_DIALOG_PAGE_DETAILS);
	ctk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook),
				  EOC_PROPERTIES_DIALOG_PAGE_EXIF);
#endif
}

/**
 * eoc_properties_dialog_new:
 * @parent: the dialog's parent window
 * @thumbview:
 * @next_image_action:
 * @previous_image_action:
 *
 *
 *
 * Returns: (transfer full) (type EocPropertiesDialog): a new #EocPropertiesDialog
 **/

GtkWidget *
eoc_properties_dialog_new (GtkWindow    *parent,
			   EocThumbView *thumbview,
			   GtkAction    *next_image_action,
			   GtkAction    *previous_image_action)
{
	GObject *prop_dlg;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (EOC_IS_THUMB_VIEW (thumbview), NULL);
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	g_return_val_if_fail (GTK_IS_ACTION (next_image_action), NULL);
	g_return_val_if_fail (GTK_IS_ACTION (previous_image_action), NULL);

	prop_dlg = g_object_new (EOC_TYPE_PROPERTIES_DIALOG,
	                         "thumbview", thumbview,
	                         NULL);

	if (parent) {
		ctk_window_set_transient_for (GTK_WINDOW (prop_dlg), parent);
	}

	ctk_activatable_set_related_action (GTK_ACTIVATABLE (EOC_PROPERTIES_DIALOG (prop_dlg)->priv->next_button), next_image_action);

	ctk_activatable_set_related_action (GTK_ACTIVATABLE (EOC_PROPERTIES_DIALOG (prop_dlg)->priv->previous_button), previous_image_action);
	G_GNUC_END_IGNORE_DEPRECATIONS;

	return GTK_WIDGET (prop_dlg);
}

void
eoc_properties_dialog_update (EocPropertiesDialog *prop_dlg,
			      EocImage            *image)
{
	g_return_if_fail (EOC_IS_PROPERTIES_DIALOG (prop_dlg));

	prop_dlg->priv->update_page = FALSE;

	pd_update_general_tab (prop_dlg, image);

#ifdef HAVE_METADATA
	pd_update_metadata_tab (prop_dlg, image);
#endif
	ctk_notebook_set_current_page (GTK_NOTEBOOK (prop_dlg->priv->notebook),
				       prop_dlg->priv->current_page);

	prop_dlg->priv->update_page = TRUE;
}

void
eoc_properties_dialog_set_page (EocPropertiesDialog *prop_dlg,
			        EocPropertiesDialogPage page)
{
	g_return_if_fail (EOC_IS_PROPERTIES_DIALOG (prop_dlg));

	prop_dlg->priv->current_page = page;

	ctk_notebook_set_current_page (GTK_NOTEBOOK (prop_dlg->priv->notebook),
				       page);
}
