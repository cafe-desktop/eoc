/* Eye Of Cafe - Erro Message Area
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-message-area.h) by:
 * 	- Paolo Maggi <paolo@gnome.org>
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
#include <config.h>
#endif

#include "eoc-error-message-area.h"
#include "eoc-image.h"
#include "eoc-util.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <ctk/ctk.h>

static void
set_message_area_text_and_icon (CtkInfoBar   *message_area,
				const gchar  *icon_name,
				const gchar  *primary_text,
				const gchar  *secondary_text)
{
	CtkWidget *hbox_content;
	CtkWidget *image;
	CtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	CtkWidget *primary_label;
	CtkWidget *secondary_label;

	hbox_content = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
	ctk_widget_show (hbox_content);

	image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_DIALOG);
	ctk_widget_show (image);
	ctk_box_pack_start (CTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	ctk_widget_set_valign (image, CTK_ALIGN_START);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_widget_show (vbox);
	ctk_box_pack_start (CTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	primary_markup = g_markup_printf_escaped ("<b>%s</b>", primary_text);
	primary_label = ctk_label_new (primary_markup);
	g_free (primary_markup);

	ctk_widget_show (primary_label);

	ctk_box_pack_start (CTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	ctk_label_set_use_markup (CTK_LABEL (primary_label), TRUE);
	ctk_label_set_line_wrap (CTK_LABEL (primary_label), FALSE);
	ctk_label_set_xalign (CTK_LABEL (primary_label), 0.0);

	ctk_widget_set_can_focus (primary_label, TRUE);

	ctk_label_set_selectable (CTK_LABEL (primary_label), TRUE);

	if (secondary_text != NULL) {
		secondary_markup = g_markup_printf_escaped ("<small>%s</small>",
		                                            secondary_text);
		secondary_label = ctk_label_new (secondary_markup);
		g_free (secondary_markup);

		ctk_widget_show (secondary_label);

		ctk_box_pack_start (CTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);

		ctk_widget_set_can_focus (secondary_label, TRUE);

		ctk_label_set_use_markup (CTK_LABEL (secondary_label), TRUE);
		ctk_label_set_line_wrap (CTK_LABEL (secondary_label), TRUE);
		ctk_label_set_selectable (CTK_LABEL (secondary_label), TRUE);
		ctk_label_set_xalign (CTK_LABEL (secondary_label), 0.0);
	}

	ctk_box_pack_start (CTK_BOX (ctk_info_bar_get_content_area (CTK_INFO_BAR (message_area))), hbox_content, TRUE, TRUE, 0);
}

static CtkWidget *
create_error_message_area (const gchar *primary_text,
			   const gchar *secondary_text,
			   gboolean     recoverable)
{
	CtkWidget *message_area;

	if (recoverable)
		message_area = ctk_info_bar_new_with_buttons (_("_Retry"),
							      CTK_RESPONSE_OK,
							      NULL);
	else
		message_area = ctk_info_bar_new ();

	ctk_info_bar_set_message_type (CTK_INFO_BAR (message_area),
				       CTK_MESSAGE_ERROR);

	set_message_area_text_and_icon (CTK_INFO_BAR (message_area),
					"dialog-error",
					primary_text,
					secondary_text);

	return message_area;
}

/**
 * eoc_image_load_error_message_area_new:
 * @caption:
 * @error:
 *
 *
 *
 * Returns: (transfer full): a new #CtkInfoArea
 **/
CtkWidget *
eoc_image_load_error_message_area_new (const gchar  *caption,
				       const GError *error)
{
	CtkWidget *message_area;
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *pango_escaped_caption = NULL;

	g_return_val_if_fail (caption != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);

	/* Escape the caption string with respect to pango markup.
	   This is necessary because otherwise characters like "&" will
	   be interpreted as the beginning of a pango entity inside
	   the message area CtkLabel. */
	pango_escaped_caption = g_markup_escape_text (caption, -1);
	error_message = g_strdup_printf (_("Could not load image '%s'."),
					 pango_escaped_caption);

	message_details = eoc_util_make_valid_utf8 (error->message);

	message_area = create_error_message_area (error_message,
						  message_details,
						  TRUE);

	g_free (pango_escaped_caption);
	g_free (error_message);
	g_free (message_details);

	return message_area;
}

/**
 * eoc_no_images_error_message_area_new:
 * @file:
 *
 *
 *
 * Returns: (transfer full): a new #CtkInfoBar
 **/
CtkWidget *
eoc_no_images_error_message_area_new (GFile *file)
{
	CtkWidget *message_area;
	gchar *error_message = NULL;

	if (file != NULL) {
		gchar *uri_str, *unescaped_str, *pango_escaped_str;

		uri_str = g_file_get_uri (file);
		/* Unescape URI with respect to rules defined in RFC 3986. */
		unescaped_str = g_uri_unescape_string (uri_str, NULL);

		/* Escape the URI string with respect to pango markup.
		   This is necessary because the URI string can contain
		   for example "&" which will otherwise be interpreted
		   as a pango markup entity when inserted into a CtkLabel. */
		pango_escaped_str = g_markup_escape_text (unescaped_str, -1);
		error_message = g_strdup_printf (_("No images found in '%s'."),
						 pango_escaped_str);

		g_free (pango_escaped_str);
		g_free (uri_str);
		g_free (unescaped_str);
	} else {
		error_message = g_strdup (_("The given locations contain no images."));
	}

	message_area = create_error_message_area (error_message,
						  NULL,
						  FALSE);

	g_free (error_message);

	return message_area;
}
