/*
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
#include "config.h"
#endif

#include "eoc-file-chooser.h"
#include "eoc-config-keys.h"
#include "eoc-pixbuf-util.h"

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

/* We must define CAFE_DESKTOP_USE_UNSTABLE_API to be able
   to use CafeDesktopThumbnail */
#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#define CAFE_DESKTOP_USE_UNSTABLE_API
#endif
#include <libcafe-desktop/cafe-desktop-thumbnail.h>

static char *last_dir[] = { NULL, NULL, NULL, NULL };

#define FILE_FORMAT_KEY "file-format"

struct _EocFileChooserPrivate
{
	CafeDesktopThumbnailFactory *thumb_factory;

	CtkWidget *image;
	CtkWidget *size_label;
	CtkWidget *dim_label;
	CtkWidget *creator_label;
};

G_DEFINE_TYPE_WITH_PRIVATE (EocFileChooser, eoc_file_chooser, CTK_TYPE_FILE_CHOOSER_DIALOG)

static void
eoc_file_chooser_finalize (GObject *object)
{
	EocFileChooserPrivate *priv;

	priv = EOC_FILE_CHOOSER (object)->priv;

	if (priv->thumb_factory != NULL)
		g_object_unref (priv->thumb_factory);

	(* G_OBJECT_CLASS (eoc_file_chooser_parent_class)->finalize) (object);
}

static void
eoc_file_chooser_class_init (EocFileChooserClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = eoc_file_chooser_finalize;
}

static void
eoc_file_chooser_init (EocFileChooser *chooser)
{
	chooser->priv = eoc_file_chooser_get_instance_private (chooser);
}

static void
response_cb (CtkDialog *dlg, gint id, gpointer data)
{
	char *dir;
	CtkFileChooserAction action;

	if (id == CTK_RESPONSE_OK) {
		dir = ctk_file_chooser_get_current_folder (CTK_FILE_CHOOSER (dlg));
		action = ctk_file_chooser_get_action (CTK_FILE_CHOOSER (dlg));

		if (last_dir [action] != NULL)
			g_free (last_dir [action]);

		last_dir [action] = dir;
	}
}

static void
save_response_cb (CtkDialog *dlg, gint id, gpointer data)
{
	GFile *file;
	GdkPixbufFormat *format;

	if (id != CTK_RESPONSE_OK)
		return;

	file = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (dlg));
	format = eoc_pixbuf_get_format (file);
	g_object_unref (file);

	if (!format || !cdk_pixbuf_format_is_writable (format)) {
		CtkWidget *msg_dialog;

		msg_dialog = ctk_message_dialog_new (
						     CTK_WINDOW (dlg),
						     CTK_DIALOG_MODAL,
						     CTK_MESSAGE_ERROR,
						     CTK_BUTTONS_OK,
						     _("File format is unknown or unsupported"));

		ctk_message_dialog_format_secondary_text (
						CTK_MESSAGE_DIALOG (msg_dialog),
						"%s\n%s",
		 				_("Eye of CAFE could not determine a supported writable file format based on the filename."),
		  				_("Please try a different file extension like .png or .jpg."));

		ctk_dialog_run (CTK_DIALOG (msg_dialog));
		ctk_widget_destroy (msg_dialog);

		g_signal_stop_emission_by_name (dlg, "response");
	} else {
		response_cb (dlg, id, data);
	}
}

static void
eoc_file_chooser_add_filter (EocFileChooser *chooser)
{
	GSList *it;
	GSList *formats;
 	CtkFileFilter *all_file_filter;
	CtkFileFilter *all_img_filter;
	CtkFileFilter *filter;
	GSList *filters = NULL;
	gchar **mime_types, **pattern, *tmp;
	int i;
	CtkFileChooserAction action;

	action = ctk_file_chooser_get_action (CTK_FILE_CHOOSER (chooser));

	if (action != CTK_FILE_CHOOSER_ACTION_SAVE && action != CTK_FILE_CHOOSER_ACTION_OPEN) {
		return;
	}

	/* All Files Filter */
	all_file_filter = ctk_file_filter_new ();
	ctk_file_filter_set_name (all_file_filter, _("All Files"));
	ctk_file_filter_add_pattern (all_file_filter, "*");

	/* All Image Filter */
	all_img_filter = ctk_file_filter_new ();
	ctk_file_filter_set_name (all_img_filter, _("All Images"));

	if (action == CTK_FILE_CHOOSER_ACTION_SAVE) {
		formats = eoc_pixbuf_get_savable_formats ();
	}
	else {
		formats = cdk_pixbuf_get_formats ();
	}

	/* Image filters */
	for (it = formats; it != NULL; it = it->next) {
		char *filter_name;
		char *description, *extension;
		GdkPixbufFormat *format;
		filter = ctk_file_filter_new ();

		format = (GdkPixbufFormat*) it->data;
		description = cdk_pixbuf_format_get_description (format);
		extension = cdk_pixbuf_format_get_name (format);

		/* Filter name: First description then file extension, eg. "The PNG-Format (*.png)".*/
		filter_name = g_strdup_printf (_("%s (*.%s)"), description, extension);
		g_free (description);
		g_free (extension);

		ctk_file_filter_set_name (filter, filter_name);
		g_free (filter_name);

		mime_types = cdk_pixbuf_format_get_mime_types ((GdkPixbufFormat *) it->data);
		for (i = 0; mime_types[i] != NULL; i++) {
			ctk_file_filter_add_mime_type (filter, mime_types[i]);
			ctk_file_filter_add_mime_type (all_img_filter, mime_types[i]);
		}
		g_strfreev (mime_types);

		pattern = cdk_pixbuf_format_get_extensions ((GdkPixbufFormat *) it->data);
		for (i = 0; pattern[i] != NULL; i++) {
			tmp = g_strconcat ("*.", pattern[i], NULL);
			ctk_file_filter_add_pattern (filter, tmp);
			ctk_file_filter_add_pattern (all_img_filter, tmp);
			g_free (tmp);
		}
		g_strfreev (pattern);

		/* attach GdkPixbufFormat to filter, see also
		 * eoc_file_chooser_get_format. */
		g_object_set_data (G_OBJECT (filter),
				   FILE_FORMAT_KEY,
				   format);

		filters = g_slist_prepend (filters, filter);
	}
	g_slist_free (formats);

	/* Add filter to filechooser */
	ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (chooser), all_file_filter);
	ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (chooser), all_img_filter);
	ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (chooser), all_img_filter);

	for (it = filters; it != NULL; it = it->next) {
		ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (chooser), CTK_FILE_FILTER (it->data));
	}
	g_slist_free (filters);
}

static void
set_preview_label (CtkWidget *label, const char *str)
{
	if (str == NULL) {
		ctk_widget_hide (CTK_WIDGET (label));
	}
	else {
		ctk_label_set_text (CTK_LABEL (label), str);
		ctk_widget_show (CTK_WIDGET (label));
	}
}

/* Sets the pixbuf as preview thumbnail and tries to read and display
 * further information according to the thumbnail spec.
 */
static void
set_preview_pixbuf (EocFileChooser *chooser, GdkPixbuf *pixbuf, goffset size)
{
	EocFileChooserPrivate *priv;
	int bytes;
	int pixels;
	const char *bytes_str;
	const char *width;
	const char *height;
	const char *creator = NULL;
	char *size_str    = NULL;
	char *dim_str     = NULL;

	g_return_if_fail (EOC_IS_FILE_CHOOSER (chooser));

	priv = chooser->priv;

	ctk_image_set_from_pixbuf (CTK_IMAGE (priv->image), pixbuf);

	if (pixbuf != NULL) {
		/* try to read file size */
		bytes_str = cdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Size");
		if (bytes_str != NULL) {
			bytes = atoi (bytes_str);
			size_str = g_format_size (bytes);
		}
		else {
			size_str = g_format_size (size);
		}

		/* try to read image dimensions */
		width  = cdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
		height = cdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");

		if ((width != NULL) && (height != NULL)) {
			pixels = atoi (height);
			/* Pixel size of image: width x height in pixel */
			dim_str = g_strdup_printf ("%s x %s %s", width, height, ngettext ("pixel", "pixels", pixels));
		}

#if 0
		/* Not sure, if this is really useful, therefore its commented out for now. */

		/* try to read creator of the thumbnail */
		creator = cdk_pixbuf_get_option (pixbuf, "tEXt::Software");

		/* stupid workaround to display nicer string if the
		 * thumbnail is created through the cafe libraries.
		 */
		if (g_ascii_strcasecmp (creator, "Cafe::ThumbnailFactory") == 0) {
			creator = "CAFE Libs";
		}
#endif
	}

	set_preview_label (priv->size_label, size_str);
	set_preview_label (priv->dim_label, dim_str);
	set_preview_label (priv->creator_label, creator);

	if (size_str != NULL) {
		g_free (size_str);
	}

	if (dim_str != NULL) {
		g_free (dim_str);
	}
}

static void
update_preview_cb (CtkFileChooser *file_chooser, gpointer data)
{
	EocFileChooserPrivate *priv;
	char *uri;
	char *thumb_path = NULL;
	GFile *file;
	GFileInfo *file_info;
	GdkPixbuf *pixbuf = NULL;
	gboolean have_preview = FALSE;

	priv = EOC_FILE_CHOOSER (file_chooser)->priv;

	uri = ctk_file_chooser_get_preview_uri (file_chooser);
	if (uri == NULL) {
		ctk_file_chooser_set_preview_widget_active (file_chooser, FALSE);
		return;
	}

	file = g_file_new_for_uri (uri);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				       G_FILE_ATTRIBUTE_STANDARD_TYPE ","
				       G_FILE_ATTRIBUTE_STANDARD_SIZE ","
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	g_object_unref (file);

	if ((file_info != NULL) && (priv->thumb_factory != NULL)
	    && g_file_info_get_file_type (file_info) != G_FILE_TYPE_SPECIAL) {
		guint64 mtime;

		mtime = g_file_info_get_attribute_uint64 (file_info,
							  G_FILE_ATTRIBUTE_TIME_MODIFIED);
		thumb_path = cafe_desktop_thumbnail_factory_lookup (priv->thumb_factory, uri, mtime);

		if (thumb_path != NULL && g_file_test (thumb_path, G_FILE_TEST_EXISTS)) {
			/* try to load and display preview thumbnail */
			pixbuf = cdk_pixbuf_new_from_file (thumb_path, NULL);
		} else if (g_file_info_get_size (file_info) <= 100000) {
			/* read files smaller than 100kb directly */

			gchar *mime_type = g_content_type_get_mime_type (
						g_file_info_get_content_type (file_info));


			if (G_LIKELY (mime_type)) {
				gboolean can_thumbnail, has_failed;

				can_thumbnail = cafe_desktop_thumbnail_factory_can_thumbnail (
							priv->thumb_factory,
							uri, mime_type, mtime);
				has_failed = cafe_desktop_thumbnail_factory_has_valid_failed_thumbnail (
							priv->thumb_factory,
							uri, mtime);

				if (G_LIKELY (can_thumbnail && !has_failed))
					pixbuf = cafe_desktop_thumbnail_factory_generate_thumbnail (
							priv->thumb_factory, uri, mime_type);

				g_free (mime_type);
			}
		}

		if (pixbuf != NULL) {
			have_preview = TRUE;

			set_preview_pixbuf (EOC_FILE_CHOOSER (file_chooser), pixbuf,
					    g_file_info_get_size (file_info));

			if (pixbuf != NULL) {
				g_object_unref (pixbuf);
			}
		}
	}

	if (thumb_path != NULL) {
		g_free (thumb_path);
	}

	g_free (uri);
	g_object_unref (file_info);

	ctk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}

static void
eoc_file_chooser_add_preview (CtkWidget *widget)
{
	EocFileChooserPrivate *priv;
	CtkWidget *vbox;

	priv = EOC_FILE_CHOOSER (widget)->priv;

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_container_set_border_width (CTK_CONTAINER (vbox), 12);

	priv->image      = ctk_image_new ();
	/* 128x128 is maximum size of thumbnails */
	ctk_widget_set_size_request (priv->image, 128,128);

	priv->dim_label  = ctk_label_new (NULL);
	priv->size_label = ctk_label_new (NULL);
	priv->creator_label = ctk_label_new (NULL);

	ctk_box_pack_start (CTK_BOX (vbox), priv->image, FALSE, TRUE, 0);
	ctk_box_pack_start (CTK_BOX (vbox), priv->dim_label, FALSE, TRUE, 0);
	ctk_box_pack_start (CTK_BOX (vbox), priv->size_label, FALSE, TRUE, 0);
	ctk_box_pack_start (CTK_BOX (vbox), priv->creator_label, FALSE, TRUE, 0);

	ctk_widget_show_all (vbox);

	ctk_file_chooser_set_preview_widget (CTK_FILE_CHOOSER (widget), vbox);
	ctk_file_chooser_set_preview_widget_active (CTK_FILE_CHOOSER (widget), FALSE);

	priv->thumb_factory = cafe_desktop_thumbnail_factory_new (CAFE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

	g_signal_connect (widget, "update-preview",
			  G_CALLBACK (update_preview_cb), NULL);
}

CtkWidget *
eoc_file_chooser_new (CtkFileChooserAction action)
{
	CtkWidget *chooser;
	gchar *title = NULL;

	chooser = g_object_new (EOC_TYPE_FILE_CHOOSER,
				"action", action,
				"select-multiple", (action == CTK_FILE_CHOOSER_ACTION_OPEN),
				"local-only", FALSE,
				NULL);

	switch (action) {
	case CTK_FILE_CHOOSER_ACTION_OPEN:
		ctk_dialog_add_buttons (CTK_DIALOG (chooser),
					"ctk-cancel", CTK_RESPONSE_CANCEL,
					"ctk-open", CTK_RESPONSE_OK,
					NULL);
		title = _("Open Image");
		break;

	case CTK_FILE_CHOOSER_ACTION_SAVE:
		ctk_dialog_add_buttons (CTK_DIALOG (chooser),
					"ctk-cancel", CTK_RESPONSE_CANCEL,
					"ctk-save", CTK_RESPONSE_OK,
					NULL);
		title = _("Save Image");
		break;

	case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
		ctk_dialog_add_buttons (CTK_DIALOG (chooser),
					"ctk-cancel", CTK_RESPONSE_CANCEL,
					"ctk-open", CTK_RESPONSE_OK,
					NULL);
		title = _("Open Folder");
		break;

	default:
		g_assert_not_reached ();
	}

	if (action != CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
		eoc_file_chooser_add_filter (EOC_FILE_CHOOSER (chooser));
		eoc_file_chooser_add_preview (chooser);
	}

	if (last_dir[action] != NULL) {
		ctk_file_chooser_set_current_folder (CTK_FILE_CHOOSER (chooser), last_dir [action]);
	}

	g_signal_connect (chooser, "response",
			  G_CALLBACK ((action == CTK_FILE_CHOOSER_ACTION_SAVE) ?
				      save_response_cb : response_cb),
			  NULL);

 	ctk_window_set_title (CTK_WINDOW (chooser), title);
	ctk_dialog_set_default_response (CTK_DIALOG (chooser), CTK_RESPONSE_OK);

	ctk_file_chooser_set_do_overwrite_confirmation (CTK_FILE_CHOOSER (chooser), TRUE);

	return chooser;
}

GdkPixbufFormat *
eoc_file_chooser_get_format (EocFileChooser *chooser)
{
	CtkFileFilter *filter;
	GdkPixbufFormat* format;

	g_return_val_if_fail (EOC_IS_FILE_CHOOSER (chooser), NULL);

	filter = ctk_file_chooser_get_filter (CTK_FILE_CHOOSER (chooser));
	if (filter == NULL)
		return NULL;

	format = g_object_get_data (G_OBJECT (filter), FILE_FORMAT_KEY);

	return format;
}
