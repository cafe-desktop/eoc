/* Eye Of Cafe - Thumbnail View
 *
 * Copyright (C) 2006-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@gnome.org>
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

#include "eoc-thumb-view.h"
#include "eoc-list-store.h"
#include "eoc-image.h"
#include "eoc-job-queue.h"

#ifdef HAVE_EXIF
#include "eoc-exif-util.h"
#include <libexif/exif-data.h>
#endif

#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

enum {
  PROP_0,
  PROP_ORIENTATION
};

#define EOC_THUMB_VIEW_SPACING 0

static EocImage* eoc_thumb_view_get_image_from_path (EocThumbView      *thumbview,
						     CtkTreePath       *path);

static void      eoc_thumb_view_popup_menu          (EocThumbView      *widget,
						     CdkEventButton    *event);

static void      eoc_thumb_view_update_columns (EocThumbView *view);

static gboolean
thumbview_on_query_tooltip_cb (CtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       CtkTooltip *tooltip,
			       gpointer    user_data);
static void
thumbview_on_parent_set_cb (CtkWidget *widget,
			    CtkWidget *old_parent,
			    gpointer   user_data);

static void
thumbview_on_drag_data_get_cb (CtkWidget        *widget,
			       CdkDragContext   *drag_context,
			       CtkSelectionData *data,
			       guint             info,
			       guint             time,
			       gpointer          user_data);

struct _EocThumbViewPrivate {
	gint start_thumb; /* the first visible thumbnail */
	gint end_thumb;   /* the last visible thumbnail  */
	CtkWidget *menu;  /* a contextual menu for thumbnails */
	CtkCellRenderer *pixbuf_cell;
	gint visible_range_changed_id;

	CtkOrientation orientation;
	gint n_images;
	gulong image_add_id;
	gulong image_removed_id;
};

G_DEFINE_TYPE_WITH_CODE (EocThumbView, eoc_thumb_view, CTK_TYPE_ICON_VIEW,
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL) \
			 G_ADD_PRIVATE (EocThumbView));

/* Drag 'n Drop */

static void
eoc_thumb_view_constructed (GObject *object)
{
	EocThumbView *thumbview;

	if (G_OBJECT_CLASS (eoc_thumb_view_parent_class)->constructed)
		G_OBJECT_CLASS (eoc_thumb_view_parent_class)->constructed (object);

	thumbview = EOC_THUMB_VIEW (object);

	thumbview->priv->pixbuf_cell = ctk_cell_renderer_pixbuf_new ();

	ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (thumbview),
				    thumbview->priv->pixbuf_cell,
				    FALSE);

	g_object_set (thumbview->priv->pixbuf_cell,
	              "height", 100,
	              "width", 115,
	              "yalign", 0.5,
	              "xalign", 0.5,
	              NULL);

	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (thumbview),
					thumbview->priv->pixbuf_cell,
					"pixbuf", EOC_LIST_STORE_THUMBNAIL,
					NULL);

	ctk_icon_view_set_selection_mode (CTK_ICON_VIEW (thumbview),
					  CTK_SELECTION_MULTIPLE);

	ctk_icon_view_set_column_spacing (CTK_ICON_VIEW (thumbview),
					  EOC_THUMB_VIEW_SPACING);

	ctk_icon_view_set_row_spacing (CTK_ICON_VIEW (thumbview),
				       EOC_THUMB_VIEW_SPACING);

	g_object_set (thumbview, "has-tooltip", TRUE, NULL);

	g_signal_connect (thumbview,
			  "query-tooltip",
			  G_CALLBACK (thumbview_on_query_tooltip_cb),
			  NULL);

	thumbview->priv->start_thumb = 0;
	thumbview->priv->end_thumb = 0;
	thumbview->priv->menu = NULL;

	g_signal_connect (G_OBJECT (thumbview), "parent-set",
			  G_CALLBACK (thumbview_on_parent_set_cb), NULL);

	ctk_icon_view_enable_model_drag_source (CTK_ICON_VIEW (thumbview), 0,
						NULL, 0,
						CDK_ACTION_COPY |
						CDK_ACTION_MOVE |
						CDK_ACTION_LINK |
						CDK_ACTION_ASK);
	ctk_drag_source_add_uri_targets (CTK_WIDGET (thumbview));

	g_signal_connect (G_OBJECT (thumbview), "drag-data-get",
			  G_CALLBACK (thumbview_on_drag_data_get_cb), NULL);
}

static void
eoc_thumb_view_dispose (GObject *object)
{
	EocThumbViewPrivate *priv = EOC_THUMB_VIEW (object)->priv;
	CtkTreeModel *model;

	if (priv->visible_range_changed_id != 0) {
		g_source_remove (priv->visible_range_changed_id);
		priv->visible_range_changed_id = 0;
	}

	model = ctk_icon_view_get_model (CTK_ICON_VIEW (object));

	if (model && priv->image_add_id != 0) {
		g_signal_handler_disconnect (model, priv->image_add_id);
		priv->image_add_id = 0;
	}

	if (model && priv->image_removed_id) {
		g_signal_handler_disconnect (model, priv->image_removed_id);
		priv->image_removed_id = 0;
	}

	G_OBJECT_CLASS (eoc_thumb_view_parent_class)->dispose (object);
}

static void
eoc_thumb_view_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	EocThumbView *view = EOC_THUMB_VIEW (object);

	switch (prop_id)
	{
	case PROP_ORIENTATION:
		g_value_set_enum (value, view->priv->orientation);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eoc_thumb_view_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	EocThumbView *view = EOC_THUMB_VIEW (object);

	switch (prop_id)
	{
	case PROP_ORIENTATION:
		view->priv->orientation = g_value_get_enum (value);
		eoc_thumb_view_update_columns (view);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eoc_thumb_view_class_init (EocThumbViewClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->constructed = eoc_thumb_view_constructed;
	gobject_class->dispose = eoc_thumb_view_dispose;
	gobject_class->get_property = eoc_thumb_view_get_property;
	gobject_class->set_property = eoc_thumb_view_set_property;

	g_object_class_override_property (gobject_class, PROP_ORIENTATION,
	                                  "orientation");
}

static void
eoc_thumb_view_clear_range (EocThumbView *thumbview,
			    const gint start_thumb,
			    const gint end_thumb)
{
	CtkTreePath *path;
	CtkTreeIter iter;
	EocListStore *store = EOC_LIST_STORE (ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = ctk_tree_path_new_from_indices (start_thumb, -1);
	for (result = ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter), thumb++) {
		eoc_list_store_thumbnail_unset (store, &iter);
	}
	ctk_tree_path_free (path);
}

static void
eoc_thumb_view_add_range (EocThumbView *thumbview,
			  const gint start_thumb,
			  const gint end_thumb)
{
	CtkTreePath *path;
	CtkTreeIter iter;
	EocListStore *store = EOC_LIST_STORE (ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = ctk_tree_path_new_from_indices (start_thumb, -1);
	for (result = ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter), thumb++) {
		eoc_list_store_thumbnail_set (store, &iter);
	}
	ctk_tree_path_free (path);
}

static void
eoc_thumb_view_update_visible_range (EocThumbView *thumbview,
				     const gint start_thumb,
				     const gint end_thumb)
{
	EocThumbViewPrivate *priv = thumbview->priv;
	int old_start_thumb, old_end_thumb;

	old_start_thumb= priv->start_thumb;
	old_end_thumb = priv->end_thumb;

	if (start_thumb == old_start_thumb &&
	    end_thumb == old_end_thumb) {
		return;
	}

	if (old_start_thumb < start_thumb)
		eoc_thumb_view_clear_range (thumbview, old_start_thumb, MIN (start_thumb - 1, old_end_thumb));

	if (old_end_thumb > end_thumb)
		eoc_thumb_view_clear_range (thumbview, MAX (end_thumb + 1, old_start_thumb), old_end_thumb);

	eoc_thumb_view_add_range (thumbview, start_thumb, end_thumb);

	priv->start_thumb = start_thumb;
	priv->end_thumb = end_thumb;
}

static gboolean
visible_range_changed_cb (EocThumbView *thumbview)
{
	CtkTreePath *path1, *path2;

	thumbview->priv->visible_range_changed_id = 0;

	if (!ctk_icon_view_get_visible_range (CTK_ICON_VIEW (thumbview), &path1, &path2)) {
		return FALSE;
	}

	if (path1 == NULL) {
		path1 = ctk_tree_path_new_first ();
	}
	if (path2 == NULL) {
		gint n_items = ctk_tree_model_iter_n_children (ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview)), NULL);
		path2 = ctk_tree_path_new_from_indices (n_items - 1 , -1);
	}

	eoc_thumb_view_update_visible_range (thumbview, ctk_tree_path_get_indices (path1) [0],
					     ctk_tree_path_get_indices (path2) [0]);

	ctk_tree_path_free (path1);
	ctk_tree_path_free (path2);

	return FALSE;
}

static void
eoc_thumb_view_visible_range_changed (EocThumbView *thumbview)
{
	if (thumbview->priv->visible_range_changed_id == 0) {
		g_idle_add ((GSourceFunc)visible_range_changed_cb, thumbview);
	}

}

static void
thumbview_on_visible_range_changed_cb (EocThumbView *thumbview,
				       gpointer      user_data G_GNUC_UNUSED)
{
	eoc_thumb_view_visible_range_changed (thumbview);
}

static void
thumbview_on_adjustment_changed_cb (EocThumbView *thumbview,
				    gpointer user_data G_GNUC_UNUSED)
{
	eoc_thumb_view_visible_range_changed (thumbview);
}

static void
thumbview_on_parent_set_cb (CtkWidget *widget,
			    CtkWidget *old_parent G_GNUC_UNUSED,
			    gpointer   user_data G_GNUC_UNUSED)
{
	EocThumbView *thumbview = EOC_THUMB_VIEW (widget);
	CtkScrolledWindow *sw;
	CtkAdjustment *hadjustment;
	CtkAdjustment *vadjustment;

	CtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (thumbview));
	if (!CTK_IS_SCROLLED_WINDOW (parent)) {
		return;
	}

	/* if we have been set to a ScrolledWindow, we connect to the callback
	   to set and unset thumbnails. */
	sw = CTK_SCROLLED_WINDOW (parent);
	hadjustment = ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (sw));
	vadjustment = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (sw));

	/* when scrolling */
	g_signal_connect_data (G_OBJECT (hadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when the adjustment is changed, ie. probably we have new images added. */
	g_signal_connect_data (G_OBJECT (hadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when resizing the scrolled window */
	g_signal_connect_swapped (G_OBJECT (sw), "size-allocate",
				  G_CALLBACK (thumbview_on_visible_range_changed_cb),
				  thumbview);
}

static gboolean
thumbview_on_button_press_event_cb (CtkWidget      *thumbview,
				    CdkEventButton *event,
				    gpointer        user_data G_GNUC_UNUSED)
{
	CtkTreePath *path;

	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == CDK_BUTTON_PRESS)
	{
		path = ctk_icon_view_get_path_at_pos (CTK_ICON_VIEW (thumbview),
						      (gint) event->x, (gint) event->y);
		if (path == NULL) {
			return FALSE;
		}

		if (!ctk_icon_view_path_is_selected (CTK_ICON_VIEW (thumbview), path) ||
		    eoc_thumb_view_get_n_selected (EOC_THUMB_VIEW (thumbview)) != 1) {
			ctk_icon_view_unselect_all (CTK_ICON_VIEW (thumbview));
			ctk_icon_view_select_path (CTK_ICON_VIEW (thumbview), path);
			ctk_icon_view_set_cursor (CTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		}
		eoc_thumb_view_popup_menu (EOC_THUMB_VIEW (thumbview), event);

		ctk_tree_path_free (path);

		return TRUE;
	}

	return FALSE;
}

static void
thumbview_on_drag_data_get_cb (CtkWidget        *widget,
			       CdkDragContext   *drag_context G_GNUC_UNUSED,
			       CtkSelectionData *data,
			       guint             info G_GNUC_UNUSED,
			       guint             time G_GNUC_UNUSED,
			       gpointer          user_data G_GNUC_UNUSED)
{
	GList *list;
	GList *node;
	EocImage *image;
	GFile *file;
	gchar **uris = NULL;
	gint i = 0, n_images;

	list = eoc_thumb_view_get_selected_images (EOC_THUMB_VIEW (widget));
	n_images = eoc_thumb_view_get_n_selected (EOC_THUMB_VIEW (widget));

	uris = g_new (gchar *, n_images + 1);

	for (node = list; node != NULL; node = node->next, i++) {
		image = EOC_IMAGE (node->data);
		file = eoc_image_get_file (image);
		uris[i] = g_file_get_uri (file);
		g_object_unref (image);
		g_object_unref (file);
	}
	uris[i] = NULL;

	ctk_selection_data_set_uris (data, uris);
	g_strfreev (uris);
	g_list_free (list);
}

static gchar *
thumbview_get_tooltip_string (EocImage *image)
{
	gchar *bytes;
	char *type_str;
	gint width, height;
	GFile *file;
	GFileInfo *file_info;
	const char *mime_str;
	gchar *tooltip_string;
#ifdef HAVE_EXIF
	ExifData *exif_data;
#endif

	bytes = g_format_size (eoc_image_get_bytes (image));

	eoc_image_get_size (image, &width, &height);

	file = eoc_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	g_object_unref (file);
	if (file_info == NULL) {
		g_free (bytes);
		return NULL;
	}

	mime_str = g_file_info_get_content_type (file_info);

	if (G_UNLIKELY (mime_str == NULL)) {
		g_free (bytes);
		g_object_unref (image);
		return NULL;
	}

	type_str = g_content_type_get_description (mime_str);
	g_object_unref (file_info);

	if (width > -1 && height > -1) {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%i x %i %s\n"
							  "%s\n"
							  "%s",
							  eoc_image_get_caption (image),
							  width,
							  height,
							  ngettext ("pixel",
								    "pixels",
								    height),
							  bytes,
							  type_str);
	} else {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%s\n"
							  "%s",
							  eoc_image_get_caption (image),
							  bytes,
							  type_str);

	}

#ifdef HAVE_EXIF
	exif_data = (ExifData *) eoc_image_get_exif_info (image);

	if (exif_data) {
		gchar *extra_info, *tmp, *date;
		/* The EXIF standard says that the DATE_TIME tag is
		 * 20 bytes long. A 32-byte buffer should be large enough. */
		gchar time_buffer[32];

		date = eoc_exif_util_format_date (
			eoc_exif_data_get_value (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL, time_buffer, 32));

		if (date) {
			extra_info = g_strdup_printf ("\n%s %s", _("Taken on"), date);

			tmp = g_strconcat (tooltip_string, extra_info, NULL);

			g_free (date);
			g_free (extra_info);
			g_free (tooltip_string);

			tooltip_string = tmp;
		}
		exif_data_unref (exif_data);
	}
#endif

	g_free (type_str);
	g_free (bytes);

	return tooltip_string;
}

static void
on_data_loaded_cb (EocJob  *job,
		   gpointer data G_GNUC_UNUSED)
{
	if (!job->error) {
		ctk_tooltip_trigger_tooltip_query (cdk_display_get_default());
	}
}

static gboolean
thumbview_on_query_tooltip_cb (CtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       CtkTooltip *tooltip,
			       gpointer    user_data G_GNUC_UNUSED)
{
	CtkTreePath *path;
	EocImage *image;
	gchar *tooltip_string;
	EocImageData data = 0;

	if (!ctk_icon_view_get_tooltip_context (CTK_ICON_VIEW (widget),
						&x, &y, keyboard_mode,
						NULL, &path, NULL)) {
		return FALSE;
	}

	image = eoc_thumb_view_get_image_from_path (EOC_THUMB_VIEW (widget),
						    path);
	ctk_tree_path_free (path);

	if (image == NULL) {
		return FALSE;
	}

	if (!eoc_image_has_data (image, EOC_IMAGE_DATA_EXIF) &&
            eoc_image_get_metadata_status (image) == EOC_IMAGE_METADATA_NOT_READ) {
		data = EOC_IMAGE_DATA_EXIF;
	}

	if (!eoc_image_has_data (image, EOC_IMAGE_DATA_DIMENSION)) {
		data |= EOC_IMAGE_DATA_DIMENSION;
	}

	if (data) {
		EocJob *job;

		job = eoc_job_load_new (image, data);
		g_signal_connect (G_OBJECT (job), "finished",
				  G_CALLBACK (on_data_loaded_cb),
				  widget);
		eoc_job_queue_add_job (job);
		g_object_unref (image);
		g_object_unref (job);
		return FALSE;
	}

	tooltip_string = thumbview_get_tooltip_string (image);
	g_object_unref (image);

	if (tooltip_string == NULL) {
		return FALSE;
	}

	ctk_tooltip_set_markup (tooltip, tooltip_string);
	g_free (tooltip_string);

	return TRUE;
}

static void
eoc_thumb_view_init (EocThumbView *thumbview)
{
	thumbview->priv = eoc_thumb_view_get_instance_private (thumbview);

	thumbview->priv->visible_range_changed_id = 0;
	thumbview->priv->image_add_id = 0;
	thumbview->priv->image_removed_id = 0;
}

/**
 * eoc_thumb_view_new:
 *
 * Creates a new #EocThumbView object.
 *
 * Returns: a newly created #EocThumbView.
 **/
CtkWidget *
eoc_thumb_view_new (void)
{
	EocThumbView *thumbview;

	thumbview = g_object_new (EOC_TYPE_THUMB_VIEW, NULL);

	return CTK_WIDGET (thumbview);
}

static void
eoc_thumb_view_update_columns (EocThumbView *view)
{
	EocThumbViewPrivate *priv;

	g_return_if_fail (EOC_IS_THUMB_VIEW (view));

	priv = view->priv;

	if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
			ctk_icon_view_set_columns (CTK_ICON_VIEW (view),
			                           priv->n_images);
}

static void
eoc_thumb_view_row_inserted_cb (CtkTreeModel    *tree_model G_GNUC_UNUSED,
				CtkTreePath     *path G_GNUC_UNUSED,
				CtkTreeIter     *iter G_GNUC_UNUSED,
				EocThumbView    *view)
{
	EocThumbViewPrivate *priv = view->priv;

	priv->n_images++;
	eoc_thumb_view_update_columns (view);
}

static void
eoc_thumb_view_row_deleted_cb (CtkTreeModel    *tree_model G_GNUC_UNUSED,
			       CtkTreePath     *path G_GNUC_UNUSED,
			       EocThumbView    *view)
{
	EocThumbViewPrivate *priv = view->priv;

	priv->n_images--;
	eoc_thumb_view_update_columns (view);
}

/**
 * eoc_thumb_view_set_model:
 * @thumbview: A #EocThumbView.
 * @store: A #EocListStore.
 *
 * Sets the #EocListStore to be used with @thumbview. If an initial image
 * was set during @store creation, its thumbnail will be selected and visible.
 *
 **/
void
eoc_thumb_view_set_model (EocThumbView *thumbview, EocListStore *store)
{
	gint index;
	EocThumbViewPrivate *priv;
	CtkTreeModel *existing;

	g_return_if_fail (EOC_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (EOC_IS_LIST_STORE (store));

	priv = thumbview->priv;

	existing = ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview));

	if (existing != NULL) {
		if (priv->image_add_id != 0) {
			g_signal_handler_disconnect (existing,
			                             priv->image_add_id);
		}
		if (priv->image_removed_id != 0) {
			g_signal_handler_disconnect (existing,
			                             priv->image_removed_id);

		}
	}

	priv->image_add_id = g_signal_connect (G_OBJECT (store), "row-inserted",
	                            G_CALLBACK (eoc_thumb_view_row_inserted_cb),
	                            thumbview);
	priv->image_removed_id = g_signal_connect (G_OBJECT (store),
	                             "row-deleted",
	                             G_CALLBACK (eoc_thumb_view_row_deleted_cb),
	                             thumbview);

	thumbview->priv->n_images = eoc_list_store_length (store);

	index = eoc_list_store_get_initial_pos (store);

	ctk_icon_view_set_model (CTK_ICON_VIEW (thumbview),
	                         CTK_TREE_MODEL (store));

	eoc_thumb_view_update_columns (thumbview);

	if (index >= 0) {
		CtkTreePath *path = ctk_tree_path_new_from_indices (index, -1);
		ctk_icon_view_select_path (CTK_ICON_VIEW (thumbview), path);
		ctk_icon_view_set_cursor (CTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		ctk_icon_view_scroll_to_path (CTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
		ctk_tree_path_free (path);
	}
}

/**
 * eoc_thumb_view_set_item_height:
 * @thumbview: A #EocThumbView.
 * @height: The desired height.
 *
 * Sets the height of each thumbnail in @thumbview.
 *
 **/
void
eoc_thumb_view_set_item_height (EocThumbView *thumbview, gint height)
{
	g_return_if_fail (EOC_IS_THUMB_VIEW (thumbview));

	g_object_set (thumbview->priv->pixbuf_cell,
	              "height", height,
	              NULL);
}

static void
eoc_thumb_view_get_n_selected_helper (CtkIconView *thumbview G_GNUC_UNUSED,
				      CtkTreePath *path G_GNUC_UNUSED,
				      gpointer     data)
{
	/* data is of type (guint *) */
	(*(guint *) data) ++;
}

/**
 * eoc_thumb_view_get_n_selected:
 * @thumbview: An #EocThumbView.
 *
 * Gets the number of images that are currently selected in @thumbview.
 *
 * Returns: the number of selected images in @thumbview.
 **/
guint
eoc_thumb_view_get_n_selected (EocThumbView *thumbview)
{
	guint count = 0;
	ctk_icon_view_selected_foreach (CTK_ICON_VIEW (thumbview),
					eoc_thumb_view_get_n_selected_helper,
					(&count));
	return count;
}

/**
 * eoc_thumb_view_get_image_from_path:
 * @thumbview: A #EocThumbView.
 * @path: A #CtkTreePath pointing to a #EocImage in the model for @thumbview.
 *
 * Gets the #EocImage stored in @thumbview's #EocListStore at the position indicated
 * by @path.
 *
 * Returns: (transfer full): A #EocImage.
 **/
static EocImage *
eoc_thumb_view_get_image_from_path (EocThumbView *thumbview, CtkTreePath *path)
{
	CtkTreeModel *model;
	CtkTreeIter iter;
	EocImage *image;

	model = ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview));
	ctk_tree_model_get_iter (model, &iter, path);

	ctk_tree_model_get (model, &iter,
			    EOC_LIST_STORE_EOC_IMAGE, &image,
			    -1);

	return image;
}

/**
 * eoc_thumb_view_get_first_selected_image:
 * @thumbview: A #EocThumbView.
 *
 * Returns the first selected image. Note that the returned #EocImage
 * is not ensured to be really the first selected image in @thumbview, but
 * generally, it will be.
 *
 * Returns: (transfer full): A #EocImage.
 **/
EocImage *
eoc_thumb_view_get_first_selected_image (EocThumbView *thumbview)
{
	/* The returned list is not sorted! We need to find the
	   smaller tree path value => tricky and expensive. Do we really need this?
	*/
	EocImage *image;
	CtkTreePath *path;
	GList *list = ctk_icon_view_get_selected_items (CTK_ICON_VIEW (thumbview));

	if (list == NULL) {
		return NULL;
	}

	path = (CtkTreePath *) (list->data);

	image = eoc_thumb_view_get_image_from_path (thumbview, path);

	g_list_foreach (list, (GFunc) ctk_tree_path_free , NULL);
	g_list_free (list);

	return image;
}

/**
 * eoc_thumb_view_get_selected_images:
 * @thumbview: A #EocThumbView.
 *
 * Gets a list with the currently selected images. Note that a new reference is
 * hold for each image and the list must be freed with g_list_free().
 *
 * Returns: (element-type EocImage) (transfer full): A newly allocated list of #EocImage's.
 **/
GList *
eoc_thumb_view_get_selected_images (EocThumbView *thumbview)
{
	GList *l, *item;
	GList *list = NULL;

	CtkTreePath *path;

	l = ctk_icon_view_get_selected_items (CTK_ICON_VIEW (thumbview));

	for (item = l; item != NULL; item = item->next) {
		path = (CtkTreePath *) item->data;
		list = g_list_prepend (list, eoc_thumb_view_get_image_from_path (thumbview, path));
		ctk_tree_path_free (path);
	}

	g_list_free (l);
	list = g_list_reverse (list);

	return list;
}

/**
 * eoc_thumb_view_set_current_image:
 * @thumbview: A #EocThumbView.
 * @image: The image to be selected.
 * @deselect_other: Whether to deselect currently selected images.
 *
 * Changes the status of a image, marking it as currently selected.
 * If @deselect_other is %TRUE, all other selected images will be
 * deselected.
 *
 **/
void
eoc_thumb_view_set_current_image (EocThumbView *thumbview, EocImage *image,
				  gboolean deselect_other)
{
	CtkTreePath *path;
	EocListStore *store;
	gint pos;

	store = EOC_LIST_STORE (ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview)));
	pos = eoc_list_store_get_pos_by_image (store, image);
	path = ctk_tree_path_new_from_indices (pos, -1);

	if (path == NULL) {
		return;
	}

	if (deselect_other) {
		ctk_icon_view_unselect_all (CTK_ICON_VIEW (thumbview));
	}

	ctk_icon_view_select_path (CTK_ICON_VIEW (thumbview), path);
	ctk_icon_view_set_cursor (CTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	ctk_icon_view_scroll_to_path (CTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);

	ctk_tree_path_free (path);
}

/**
 * eoc_thumb_view_select_single:
 * @thumbview: A #EocThumbView.
 * @change: A #EocThumbViewSelectionChange, describing the
 * desired selection change.
 *
 * Changes the current selection according to a single movement
 * described by #EocThumbViewSelectionChange. If there are no
 * thumbnails currently selected, one is selected according to the
 * natural selection according to the #EocThumbViewSelectionChange
 * used, p.g., when %EOC_THUMB_VIEW_SELECT_RIGHT is the selected change,
 * the first thumbnail will be selected.
 *
 **/
void
eoc_thumb_view_select_single (EocThumbView *thumbview,
			      EocThumbViewSelectionChange change)
{
  	CtkTreePath *path = NULL;
	CtkTreeModel *model;
	GList *list;
	gint n_items;

	g_return_if_fail (EOC_IS_THUMB_VIEW (thumbview));

	model = ctk_icon_view_get_model (CTK_ICON_VIEW (thumbview));

	n_items = eoc_list_store_length (EOC_LIST_STORE (model));

	if (n_items == 0) {
		return;
	}

	if (eoc_thumb_view_get_n_selected (thumbview) == 0) {
		switch (change) {
		case EOC_THUMB_VIEW_SELECT_CURRENT:
			break;
		case EOC_THUMB_VIEW_SELECT_RIGHT:
		case EOC_THUMB_VIEW_SELECT_FIRST:
			path = ctk_tree_path_new_first ();
			break;
		case EOC_THUMB_VIEW_SELECT_LEFT:
		case EOC_THUMB_VIEW_SELECT_LAST:
			path = ctk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case EOC_THUMB_VIEW_SELECT_RANDOM:
			path = ctk_tree_path_new_from_indices (g_random_int_range (0, n_items), -1);
			break;
		}
	} else {
		list = ctk_icon_view_get_selected_items (CTK_ICON_VIEW (thumbview));
		path = ctk_tree_path_copy ((CtkTreePath *) list->data);
		g_list_foreach (list, (GFunc) ctk_tree_path_free , NULL);
		g_list_free (list);

		ctk_icon_view_unselect_all (CTK_ICON_VIEW (thumbview));

		switch (change) {
		case EOC_THUMB_VIEW_SELECT_CURRENT:
			break;
		case EOC_THUMB_VIEW_SELECT_LEFT:
			if (!ctk_tree_path_prev (path)) {
				ctk_tree_path_free (path);
				path = ctk_tree_path_new_from_indices (n_items - 1, -1);
			}
			break;
		case EOC_THUMB_VIEW_SELECT_RIGHT:
			if (ctk_tree_path_get_indices (path) [0] == n_items - 1) {
				ctk_tree_path_free (path);
				path = ctk_tree_path_new_first ();
			} else {
				ctk_tree_path_next (path);
			}
			break;
		case EOC_THUMB_VIEW_SELECT_FIRST:
			ctk_tree_path_free (path);
			path = ctk_tree_path_new_first ();
			break;
		case EOC_THUMB_VIEW_SELECT_LAST:
			ctk_tree_path_free (path);
			path = ctk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case EOC_THUMB_VIEW_SELECT_RANDOM:
			ctk_tree_path_free (path);
			path = ctk_tree_path_new_from_indices (g_random_int_range (0, n_items), -1);
			break;
		}
	}

	ctk_icon_view_select_path (CTK_ICON_VIEW (thumbview), path);
	ctk_icon_view_set_cursor (CTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	ctk_icon_view_scroll_to_path (CTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
	ctk_tree_path_free (path);
}


/**
 * eoc_thumb_view_set_thumbnail_popup:
 * @thumbview: An #EocThumbView.
 * @menu: A #CtkMenu.
 *
 * Set the contextual menu to be used with the thumbnails in the
 * widget. This can be done only once.
 *
 **/
void
eoc_thumb_view_set_thumbnail_popup (EocThumbView *thumbview,
				    CtkMenu      *menu)
{
	g_return_if_fail (EOC_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (thumbview->priv->menu == NULL);

	thumbview->priv->menu = g_object_ref (CTK_WIDGET (menu));

	ctk_menu_attach_to_widget (CTK_MENU (thumbview->priv->menu),
				   CTK_WIDGET (thumbview),
				   NULL);

	g_signal_connect (G_OBJECT (thumbview), "button_press_event",
			  G_CALLBACK (thumbview_on_button_press_event_cb), NULL);

}


static void
eoc_thumb_view_popup_menu (EocThumbView *thumbview, CdkEventButton *event)
{
	g_return_if_fail (event != NULL);

	ctk_menu_popup_at_pointer (CTK_MENU (thumbview->priv->menu),
	                           (const CdkEvent*) event);
}
