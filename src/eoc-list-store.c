/* Eye Of Cafe - Image Store
 *
 * Copyright (C) 2006-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@gnome.org>
 *
 * Based on code by: Jens Finke <jens@triq.net>
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

#include "eoc-list-store.h"
#include "eoc-thumbnail.h"
#include "eoc-image.h"
#include "eoc-job-queue.h"
#include "eoc-jobs.h"

#include <string.h>

#define EOC_LIST_STORE_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOC_TYPE_LIST_STORE, EocListStorePrivate))

struct _EocListStorePrivate {
	GList *monitors;          /* Monitors for the directories */
	gint initial_image;       /* The image that should be selected firstly by the view. */
	GdkPixbuf *busy_image;    /* Loading image icon */
	GdkPixbuf *missing_image; /* Missing image icon */
	GMutex mutex;            /* Mutex for saving the jobs in the model */
};

G_DEFINE_TYPE_WITH_PRIVATE (EocListStore, eoc_list_store, GTK_TYPE_LIST_STORE);

static void
foreach_monitors_free (gpointer data, gpointer user_data)
{
	g_file_monitor_cancel (G_FILE_MONITOR (data));
}

static void
eoc_list_store_dispose (GObject *object)
{
	EocListStore *store = EOC_LIST_STORE (object);

	g_list_foreach (store->priv->monitors,
			foreach_monitors_free, NULL);

	g_list_free (store->priv->monitors);

	store->priv->monitors = NULL;

	if(store->priv->busy_image != NULL) {
		g_object_unref (store->priv->busy_image);
		store->priv->busy_image = NULL;
	}

	if(store->priv->missing_image != NULL) {
		g_object_unref (store->priv->missing_image);
		store->priv->missing_image = NULL;
	}

	g_mutex_clear (&store->priv->mutex);

	G_OBJECT_CLASS (eoc_list_store_parent_class)->dispose (object);
}

static void
eoc_list_store_class_init (EocListStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eoc_list_store_dispose;
}

/*
   Sorting functions
*/

static gint
eoc_list_store_compare_func (GtkTreeModel *model,
			     GtkTreeIter *a,
			     GtkTreeIter *b,
			     gpointer user_data)
{
	gint r_value;

	EocImage *image_a, *image_b;

	ctk_tree_model_get (model, a,
			    EOC_LIST_STORE_EOC_IMAGE, &image_a,
			    -1);

	ctk_tree_model_get (model, b,
			    EOC_LIST_STORE_EOC_IMAGE, &image_b,
			    -1);

	r_value = strcmp (eoc_image_get_collate_key (image_a),
			  eoc_image_get_collate_key (image_b));

	g_object_unref (G_OBJECT (image_a));
	g_object_unref (G_OBJECT (image_b));

	return r_value;
}

static GdkPixbuf *
eoc_list_store_get_icon (const gchar *icon_name)
{
	GError *error = NULL;
	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf;

	icon_theme = ctk_icon_theme_get_default ();

	pixbuf = ctk_icon_theme_load_icon (icon_theme,
					   icon_name,
					   EOC_LIST_STORE_THUMB_SIZE,
					   0,
					   &error);

	if (!pixbuf) {
		g_warning ("Couldn't load icon: %s", error->message);
		g_error_free (error);
	}

	return pixbuf;
}

static void
eoc_list_store_init (EocListStore *self)
{
	GType types[EOC_LIST_STORE_NUM_COLUMNS];

	types[EOC_LIST_STORE_THUMBNAIL] = GDK_TYPE_PIXBUF;
	types[EOC_LIST_STORE_EOC_IMAGE] = G_TYPE_OBJECT;
	types[EOC_LIST_STORE_THUMB_SET] = G_TYPE_BOOLEAN;
	types[EOC_LIST_STORE_EOC_JOB]   = G_TYPE_POINTER;

	ctk_list_store_set_column_types (GTK_LIST_STORE (self),
					 EOC_LIST_STORE_NUM_COLUMNS, types);

	self->priv = eoc_list_store_get_instance_private (self);

	self->priv->monitors = NULL;
	self->priv->initial_image = -1;

	self->priv->busy_image = eoc_list_store_get_icon ("image-loading");
	self->priv->missing_image = eoc_list_store_get_icon ("image-missing");

	g_mutex_init (&self->priv->mutex);

	ctk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (self),
						 eoc_list_store_compare_func,
						 NULL, NULL);

	ctk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);
}

/**
 * eoc_list_store_new:
 *
 * Creates a new and empty #EocListStore.
 *
 * Returns: a newly created #EocListStore.
 **/
GtkListStore*
eoc_list_store_new (void)
{
        return g_object_new (EOC_TYPE_LIST_STORE, NULL);
}

/*
   Searchs for a file in the store. If found and @iter_found is not NULL,
   then sets @iter_found to a #GtkTreeIter pointing to the file.
 */
static gboolean
is_file_in_list_store (EocListStore *store,
		       const gchar *info_uri,
		       GtkTreeIter *iter_found)
{
	gboolean found = FALSE;
	EocImage *image;
	GFile *file;
	gchar *str;
	GtkTreeIter iter;

	if (!ctk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
		return FALSE;
	}

	do {
		ctk_tree_model_get (GTK_TREE_MODEL (store), &iter,
				    EOC_LIST_STORE_EOC_IMAGE, &image,
				    -1);
		if (!image)
			continue;

		file = eoc_image_get_file (image);
		str = g_file_get_uri (file);

		found = (strcmp (str, info_uri) == 0)? TRUE : FALSE;

		g_object_unref (file);
		g_free (str);
		g_object_unref (G_OBJECT (image));

	} while (!found &&
		 ctk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));

	if (found && iter_found != NULL) {
		*iter_found = iter;
	}

	return found;
}

static gboolean
is_file_in_list_store_file (EocListStore *store,
			   GFile *file,
			   GtkTreeIter *iter_found)
{
	gchar *uri_str;
	gboolean result;

	uri_str = g_file_get_uri (file);

	result = is_file_in_list_store (store, uri_str, iter_found);

	g_free (uri_str);

	return result;
}

static void
eoc_job_thumbnail_cb (EocJobThumbnail *job, gpointer data)
{
	EocListStore *store;
	GtkTreeIter iter;
	EocImage *image;
	GdkPixbuf *thumbnail;
	GFile *file;

	g_return_if_fail (EOC_IS_LIST_STORE (data));

	store = EOC_LIST_STORE (data);

	file = eoc_image_get_file (job->image);

	if (is_file_in_list_store_file (store, file, &iter)) {
		ctk_tree_model_get (GTK_TREE_MODEL (store), &iter,
				    EOC_LIST_STORE_EOC_IMAGE, &image,
				    -1);

		if (job->thumbnail) {
			eoc_image_set_thumbnail (image, job->thumbnail);

			/* Getting the thumbnail, in case it needed
 			 * transformations */
			thumbnail = eoc_image_get_thumbnail (image);
		} else {
			thumbnail = g_object_ref (store->priv->missing_image);
		}

		ctk_list_store_set (GTK_LIST_STORE (store), &iter,
				    EOC_LIST_STORE_THUMBNAIL, thumbnail,
				    EOC_LIST_STORE_THUMB_SET, TRUE,
				    EOC_LIST_STORE_EOC_JOB, NULL,
				    -1);
		g_object_unref (image);
		g_object_unref (thumbnail);
	}

	g_object_unref (file);
}

static void
on_image_changed (EocImage *image, EocListStore *store)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gint pos;

	pos = eoc_list_store_get_pos_by_image (store, image);
	path = ctk_tree_path_new_from_indices (pos, -1);

	ctk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	eoc_list_store_thumbnail_refresh (store, &iter);
	ctk_tree_path_free (path);
}

/**
 * eoc_list_store_remove:
 * @store: An #EocListStore.
 * @iter: A #GtkTreeIter.
 *
 * Removes the image pointed by @iter from @store.
 **/
static void
eoc_list_store_remove (EocListStore *store, GtkTreeIter *iter)
{
	EocImage *image;

	ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    EOC_LIST_STORE_EOC_IMAGE, &image,
			    -1);

	g_signal_handlers_disconnect_by_func (image, on_image_changed, store);
	g_object_unref (image);

	ctk_list_store_remove (GTK_LIST_STORE (store), iter);
}

/**
 * eoc_list_store_append_image:
 * @store: An #EocListStore.
 * @image: An #EocImage.
 *
 * Adds an #EocImage to @store. The thumbnail of the image is not
 * loaded and will only be loaded if the thumbnail is made visible
 * or eoc_list_store_set_thumbnail() is called.
 *
 **/
void
eoc_list_store_append_image (EocListStore *store, EocImage *image)
{
	GtkTreeIter iter;

	g_signal_connect (image, "changed",
 			  G_CALLBACK (on_image_changed),
 			  store);

	ctk_list_store_append (GTK_LIST_STORE (store), &iter);
	ctk_list_store_set (GTK_LIST_STORE (store), &iter,
			    EOC_LIST_STORE_EOC_IMAGE, image,
			    EOC_LIST_STORE_THUMBNAIL, store->priv->busy_image,
			    EOC_LIST_STORE_THUMB_SET, FALSE,
			    -1);
}

static void
eoc_list_store_append_image_from_file (EocListStore *store,
				       GFile *file,
				       const gchar *caption)
{
	EocImage *image;

	g_return_if_fail (EOC_IS_LIST_STORE (store));

	image = eoc_image_new_file (file, caption);

	eoc_list_store_append_image (store, image);
}

static void
file_monitor_changed_cb (GFileMonitor *monitor,
			 GFile *file,
			 GFile *other_file,
			 GFileMonitorEvent event,
			 EocListStore *store)
{
	const char *mimetype;
	GFileInfo *file_info;
	GtkTreeIter iter;
	EocImage *image;

	switch (event) {
	case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
					       G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
					       0, NULL, NULL);
		if (file_info == NULL) {
			break;
		}
		mimetype = g_file_info_get_content_type (file_info);

		if (is_file_in_list_store_file (store, file, &iter)) {
			if (eoc_image_is_supported_mime_type (mimetype)) {
				ctk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						    EOC_LIST_STORE_EOC_IMAGE, &image,
						    -1);
				eoc_image_file_changed (image);
				g_object_unref (image);
				eoc_list_store_thumbnail_refresh (store, &iter);
			} else {
				eoc_list_store_remove (store, &iter);
			}
		} else {
			if (eoc_image_is_supported_mime_type (mimetype)) {
				const gchar *caption;

				caption = g_file_info_get_display_name (file_info);
				eoc_list_store_append_image_from_file (store, file, caption);
			}
		}
		g_object_unref (file_info);
		break;
	case G_FILE_MONITOR_EVENT_DELETED:
		if (is_file_in_list_store_file (store, file, &iter)) {
			EocImage *image;

			ctk_tree_model_get (GTK_TREE_MODEL (store), &iter,
					    EOC_LIST_STORE_EOC_IMAGE, &image,
					    -1);

			eoc_list_store_remove (store, &iter);
		}
		break;
	case G_FILE_MONITOR_EVENT_CREATED:
		if (!is_file_in_list_store_file (store, file, NULL)) {
			file_info = g_file_query_info (file,
						       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
						       G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
						       0, NULL, NULL);
			if (file_info == NULL) {
				break;
			}
			mimetype = g_file_info_get_content_type (file_info);

			if (eoc_image_is_supported_mime_type (mimetype)) {
				const gchar *caption;

				caption = g_file_info_get_display_name (file_info);
				eoc_list_store_append_image_from_file (store, file, caption);
			}
			g_object_unref (file_info);
		}
		break;
	case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					       0, NULL, NULL);
		if (file_info == NULL) {
			break;
		}
		mimetype = g_file_info_get_content_type (file_info);
		if (is_file_in_list_store_file (store, file, &iter) &&
		    eoc_image_is_supported_mime_type (mimetype)) {
			eoc_list_store_thumbnail_refresh (store, &iter);
		}
		g_object_unref (file_info);
		break;
	default:
		break;
	}
}

/*
 * Called for each file in a directory. Checks if the file is some
 * sort of image. If so, it creates an image object and adds it to the
 * list.
 */
static void
directory_visit (GFile *directory,
		 GFileInfo *children_info,
		 EocListStore *store)
{
	GFile *child;
	gboolean load_uri = FALSE;
	const char *mime_type, *name;

	mime_type = g_file_info_get_content_type (children_info);
	name = g_file_info_get_name (children_info);

	if (!g_str_has_prefix (name, ".")) {
		/* We support opening any image type, so let eoc to add any images in the current directory to the store */
		if (g_content_type_is_mime_type (mime_type, "image/*") || eoc_image_is_supported_mime_type (mime_type)) {
			load_uri = TRUE;
		}
	}

	if (load_uri) {
		const gchar *caption;

		child = g_file_get_child (directory, name);
		caption = g_file_info_get_display_name (children_info);
		eoc_list_store_append_image_from_file (store, child, caption);
	}
}

static void
eoc_list_store_append_directory (EocListStore *store,
				 GFile *file,
				 GFileType file_type)
{
	GFileMonitor *file_monitor;
	GFileEnumerator *file_enumerator;
	GFileInfo *file_info;

	g_return_if_fail (file_type == G_FILE_TYPE_DIRECTORY);

	file_monitor = g_file_monitor_directory (file,
						 0, NULL, NULL);

	if (file_monitor != NULL) {
		g_signal_connect (file_monitor, "changed",
				  G_CALLBACK (file_monitor_changed_cb), store);

		/* prepend seems more efficient to me, we don't need this list
		   to be sorted */
		store->priv->monitors = g_list_prepend (store->priv->monitors, file_monitor);
	}

	file_enumerator = g_file_enumerate_children (file,
						     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
						     G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
						     G_FILE_ATTRIBUTE_STANDARD_NAME,
						     0, NULL, NULL);
	file_info = g_file_enumerator_next_file (file_enumerator, NULL, NULL);

	while (file_info != NULL)
	{
		directory_visit (file, file_info, store);
		g_object_unref (file_info);
		file_info = g_file_enumerator_next_file (file_enumerator, NULL, NULL);
	}
	g_object_unref (file_enumerator);
}

/**
 * eoc_list_store_add_files:
 * @store: An #EocListStore.
 * @file_list: (element-type GFile): A %NULL-terminated list of #GFile's.
 *
 * Adds a list of #GFile's to @store. The given list
 * must be %NULL-terminated.
 *
 * If any of the #GFile's in @file_list is a directory, all the images
 * in that directory will be added to @store. If the list of files contains
 * only one file and this is a regular file, then all the images in the same
 * directory will be added as well to @store.
 *
 **/
void
eoc_list_store_add_files (EocListStore *store, GList *file_list)
{
	GList *it;
	GFileInfo *file_info;
	GFileType file_type;
	GFile *initial_file = NULL;
	GtkTreeIter iter;

	if (file_list == NULL) {
		return;
	}

	ctk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	for (it = file_list; it != NULL; it = it->next) {
		GFile *file = (GFile *) it->data;
		gchar *caption = NULL;

		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_TYPE","
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE","
					       G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
					       0, NULL, NULL);
		if (file_info == NULL) {
			continue;
		}

		caption = g_strdup (g_file_info_get_display_name (file_info));
		file_type = g_file_info_get_file_type (file_info);

		/* Workaround for gvfs backends that don't set the GFileType. */
		if (G_UNLIKELY (file_type == G_FILE_TYPE_UNKNOWN)) {
			const gchar *ctype;

			ctype = g_file_info_get_content_type (file_info);

			/* If the content type is supported adjust file_type */
			if (eoc_image_is_supported_mime_type (ctype))
				file_type = G_FILE_TYPE_REGULAR;
		}

		g_object_unref (file_info);

		if (file_type == G_FILE_TYPE_DIRECTORY) {
			eoc_list_store_append_directory (store, file, file_type);
		} else if (file_type == G_FILE_TYPE_REGULAR &&
			   g_list_length (file_list) == 1) {

			initial_file = g_file_dup (file);

			file = g_file_get_parent (file);
			file_info = g_file_query_info (file,
						       G_FILE_ATTRIBUTE_STANDARD_TYPE,
						       0, NULL, NULL);

			/* If we can't get a file_info,
			   file_type will stay as G_FILE_TYPE_REGULAR */
			if (file_info != NULL) {
				file_type = g_file_info_get_file_type (file_info);
				g_object_unref (file_info);
			}

			if (file_type == G_FILE_TYPE_DIRECTORY) {
				eoc_list_store_append_directory (store, file, file_type);

				if (!is_file_in_list_store_file (store,
								 initial_file,
								 &iter)) {
					eoc_list_store_append_image_from_file (store, initial_file, caption);
				}
			} else {
				eoc_list_store_append_image_from_file (store, initial_file, caption);
			}
			g_object_unref (file);
		} else if (file_type == G_FILE_TYPE_REGULAR &&
			   g_list_length (file_list) > 1) {
			eoc_list_store_append_image_from_file (store, file, caption);
		}

		g_free (caption);
	}

	ctk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	if (initial_file &&
	    is_file_in_list_store_file (store, initial_file, &iter)) {
		store->priv->initial_image = eoc_list_store_get_pos_by_iter (store, &iter);
		g_object_unref (initial_file);
	} else {
		store->priv->initial_image = 0;
	}
}

/**
 * eoc_list_store_remove_image:
 * @store: An #EocListStore.
 * @image: An #EocImage.
 *
 * Removes @image from @store.
 **/
void
eoc_list_store_remove_image (EocListStore *store, EocImage *image)
{
	GtkTreeIter iter;
	GFile *file;

	g_return_if_fail (EOC_IS_LIST_STORE (store));
	g_return_if_fail (EOC_IS_IMAGE (image));

	file = eoc_image_get_file (image);

	if (is_file_in_list_store_file (store, file, &iter)) {
		eoc_list_store_remove (store, &iter);
	}
	g_object_unref (file);
}

/**
 * eoc_list_store_new_from_glist:
 * @list: (element-type EocImage): a %NULL-terminated list of #EocImage's.
 *
 * Creates a new #EocListStore from a list of #EocImage's.
 * The given list must be %NULL-terminated.
 *
 * Returns: a new #EocListStore.
 **/
GtkListStore *
eoc_list_store_new_from_glist (GList *list)
{
	GList *it;

	GtkListStore *store = eoc_list_store_new ();

	for (it = list; it != NULL; it = it->next) {
		eoc_list_store_append_image (EOC_LIST_STORE (store),
					     EOC_IMAGE (it->data));
	}

	return store;
}

/**
 * eoc_list_store_get_pos_by_image:
 * @store: An #EocListStore.
 * @image: An #EocImage.
 *
 * Gets the position where @image is stored in @store. If @image
 * is not stored in @store, -1 is returned.
 *
 * Returns: the position of @image in @store or -1 if not found.
 **/
gint
eoc_list_store_get_pos_by_image (EocListStore *store, EocImage *image)
{
	GtkTreeIter iter;
	gint pos = -1;
	GFile *file;

	g_return_val_if_fail (EOC_IS_LIST_STORE (store), -1);
	g_return_val_if_fail (EOC_IS_IMAGE (image), -1);

	file = eoc_image_get_file (image);

	if (is_file_in_list_store_file (store, file, &iter)) {
		pos = eoc_list_store_get_pos_by_iter (store, &iter);
	}

	g_object_unref (file);
	return pos;
}

/**
 * eoc_list_store_get_image_by_pos:
 * @store: An #EocListStore.
 * @pos: the position of the required #EocImage.
 *
 * Gets the #EocImage in the position @pos of @store. If there is
 * no image at position @pos, %NULL is returned.
 *
 * Returns: (transfer full): the #EocImage in position @pos or %NULL.
 *
 **/
EocImage *
eoc_list_store_get_image_by_pos (EocListStore *store, gint pos)
{
	EocImage *image = NULL;
	GtkTreeIter iter;

	g_return_val_if_fail (EOC_IS_LIST_STORE (store), NULL);

	if (ctk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &iter, NULL, pos)) {
		ctk_tree_model_get (GTK_TREE_MODEL (store), &iter,
				    EOC_LIST_STORE_EOC_IMAGE, &image,
				    -1);
	}

	return image;
}

/**
 * eoc_list_store_get_pos_by_iter:
 * @store: An #EocListStore.
 * @iter: A #GtkTreeIter pointing to an image in @store.
 *
 * Gets the position of the image pointed by @iter.
 *
 * Returns: The position of the image pointed by @iter.
 **/
gint
eoc_list_store_get_pos_by_iter (EocListStore *store,
				GtkTreeIter *iter)
{
	gint *indices;
	GtkTreePath *path;
	gint pos;

	path = ctk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
	indices = ctk_tree_path_get_indices (path);
	pos = indices [0];
	ctk_tree_path_free (path);

	return pos;
}

/**
 * eoc_list_store_length:
 * @store: An #EocListStore.
 *
 * Returns the number of images in the store.
 *
 * Returns: The number of images in @store.
 **/
gint
eoc_list_store_length (EocListStore *store)
{
	g_return_val_if_fail (EOC_IS_LIST_STORE (store), -1);

	return ctk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);
}

/**
 * eoc_list_store_get_initial_pos:
 * @store: An #EocListStore.
 *
 * Gets the position of the #EocImage that should be loaded first.
 * If not set, it returns -1.
 *
 * Returns: the position of the image to be loaded first or -1.
 *
 **/
gint
eoc_list_store_get_initial_pos (EocListStore *store)
{
	g_return_val_if_fail (EOC_IS_LIST_STORE (store), -1);

	return store->priv->initial_image;
}

static void
eoc_list_store_remove_thumbnail_job (EocListStore *store,
				     GtkTreeIter *iter)
{
	EocJob *job;

	ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    EOC_LIST_STORE_EOC_JOB, &job,
			    -1);

	if (job != NULL) {
		g_mutex_lock (&store->priv->mutex);
		eoc_job_queue_remove_job (job);
		ctk_list_store_set (GTK_LIST_STORE (store), iter,
				    EOC_LIST_STORE_EOC_JOB, NULL,
				    -1);
		g_mutex_unlock (&store->priv->mutex);
	}


}

static void
eoc_list_store_add_thumbnail_job (EocListStore *store, GtkTreeIter *iter)
{
	EocImage *image;
	EocJob *job;

	ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    EOC_LIST_STORE_EOC_IMAGE, &image,
			    EOC_LIST_STORE_EOC_JOB, &job,
			    -1);

	if (job != NULL) {
		g_object_unref (image);
		return;
	}

	job = eoc_job_thumbnail_new (image);

	g_signal_connect (job,
			  "finished",
			  G_CALLBACK (eoc_job_thumbnail_cb),
			  store);

	g_mutex_lock (&store->priv->mutex);
	ctk_list_store_set (GTK_LIST_STORE (store), iter,
			    EOC_LIST_STORE_EOC_JOB, job,
			    -1);
	eoc_job_queue_add_job (job);
	g_mutex_unlock (&store->priv->mutex);
	g_object_unref (job);
	g_object_unref (image);
}

/**
 * eoc_list_store_thumbnail_set:
 * @store: An #EocListStore.
 * @iter: A #GtkTreeIter pointing to an image in @store.
 *
 * Sets the thumbnail for the image pointed by @iter.
 *
 **/
void
eoc_list_store_thumbnail_set (EocListStore *store,
			      GtkTreeIter *iter)
{
	gboolean thumb_set = FALSE;

	ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    EOC_LIST_STORE_THUMB_SET, &thumb_set,
			    -1);

	if (thumb_set) {
		return;
	}

	eoc_list_store_add_thumbnail_job (store, iter);
}

/**
 * eoc_list_store_thumbnail_unset:
 * @store: An #EocListStore.
 * @iter: A #GtkTreeIter pointing to an image in @store.
 *
 * Unsets the thumbnail for the image pointed by @iter, changing
 * it to a "busy" icon.
 *
 **/
void
eoc_list_store_thumbnail_unset (EocListStore *store,
				GtkTreeIter *iter)
{
	EocImage *image;

	eoc_list_store_remove_thumbnail_job (store, iter);

	ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    EOC_LIST_STORE_EOC_IMAGE, &image,
			    -1);
	eoc_image_set_thumbnail (image, NULL);
	g_object_unref (image);

	ctk_list_store_set (GTK_LIST_STORE (store), iter,
			    EOC_LIST_STORE_THUMBNAIL, store->priv->busy_image,
			    EOC_LIST_STORE_THUMB_SET, FALSE,
			    -1);
}

/**
 * eoc_list_store_thumbnail_refresh:
 * @store: An #EocListStore.
 * @iter: A #GtkTreeIter pointing to an image in @store.
 *
 * Refreshes the thumbnail for the image pointed by @iter.
 *
 **/
void
eoc_list_store_thumbnail_refresh (EocListStore *store,
				  GtkTreeIter *iter)
{
	eoc_list_store_remove_thumbnail_job (store, iter);
	eoc_list_store_add_thumbnail_job (store, iter);
}
