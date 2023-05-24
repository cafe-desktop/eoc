/* Eye Of Cafe - Jobs
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-jobs.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#include "eoc-uri-converter.h"
#include "eoc-jobs.h"
#include "eoc-job-queue.h"
#include "eoc-image.h"
#include "eoc-transform.h"
#include "eoc-list-store.h"
#include "eoc-thumbnail.h"
#include "eoc-pixbuf-util.h"

#include <cdk-pixbuf/cdk-pixbuf.h>

#define EOC_JOB_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOC_TYPE_JOB, EocJobPrivate))

G_DEFINE_TYPE (EocJob, eoc_job, G_TYPE_OBJECT);
G_DEFINE_TYPE (EocJobThumbnail, eoc_job_thumbnail, EOC_TYPE_JOB);
G_DEFINE_TYPE (EocJobLoad, eoc_job_load, EOC_TYPE_JOB);
G_DEFINE_TYPE (EocJobModel, eoc_job_model, EOC_TYPE_JOB);
G_DEFINE_TYPE (EocJobTransform, eoc_job_transform, EOC_TYPE_JOB);
G_DEFINE_TYPE (EocJobSave, eoc_job_save, EOC_TYPE_JOB);
G_DEFINE_TYPE (EocJobSaveAs, eoc_job_save_as, EOC_TYPE_JOB_SAVE);
G_DEFINE_TYPE (EocJobCopy, eoc_job_copy, EOC_TYPE_JOB);

enum
{
	SIGNAL_FINISHED,
	SIGNAL_PROGRESS,
	SIGNAL_LAST_SIGNAL
};

static guint job_signals[SIGNAL_LAST_SIGNAL] = { 0 };

static void eoc_job_copy_run      (EocJob *ejob);
static void eoc_job_load_run 	  (EocJob *ejob);
static void eoc_job_model_run     (EocJob *ejob);
static void eoc_job_save_run      (EocJob *job);
static void eoc_job_save_as_run   (EocJob *job);
static void eoc_job_thumbnail_run (EocJob *ejob);
static void eoc_job_transform_run (EocJob *ejob);

static void eoc_job_init (EocJob *job)
{
	/* NOTE: We need to allocate the mutex here so the ABI stays the same when it used to use g_mutex_new */
	job->mutex = g_malloc (sizeof (GMutex));
	g_mutex_init (job->mutex);
	job->progress = 0.0;
}

static void
eoc_job_dispose (GObject *object)
{
	EocJob *job;

	job = EOC_JOB (object);

	if (job->error) {
		g_error_free (job->error);
		job->error = NULL;
	}

	if (job->mutex) {
		g_mutex_clear (job->mutex);
		g_free (job->mutex);
	}

	(* G_OBJECT_CLASS (eoc_job_parent_class)->dispose) (object);
}

static void
eoc_job_run_default (EocJob *job)
{
	g_critical ("Class \"%s\" does not implement the required run action",
		    G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (job)));
}

static void
eoc_job_class_init (EocJobClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = eoc_job_dispose;

	class->run = eoc_job_run_default;

	job_signals [SIGNAL_FINISHED] =
		g_signal_new ("finished",
			      EOC_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EocJobClass, finished),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	job_signals [SIGNAL_PROGRESS] =
		g_signal_new ("progress",
			      EOC_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EocJobClass, progress),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 1,
			      G_TYPE_FLOAT);
}

void
eoc_job_finished (EocJob *job)
{
	g_return_if_fail (EOC_IS_JOB (job));

	g_signal_emit (job, job_signals[SIGNAL_FINISHED], 0);
}

/**
 * eoc_job_run:
 * @job: the job to execute.
 *
 * Executes the job passed as @job. Usually there is no need to call this
 * on your own. Jobs should be executed by using the EocJobQueue.
 **/
void
eoc_job_run (EocJob *job)
{
	EocJobClass *class;

	g_return_if_fail (EOC_IS_JOB (job));

	class = EOC_JOB_GET_CLASS (job);
	if (class->run)
		class->run (job);
	else
		eoc_job_run_default (job);
}
static gboolean
notify_progress (gpointer data)
{
	EocJob *job = EOC_JOB (data);

	g_signal_emit (job, job_signals[SIGNAL_PROGRESS], 0, job->progress);

	return FALSE;
}

void
eoc_job_set_progress (EocJob *job, float progress)
{
	g_return_if_fail (EOC_IS_JOB (job));
	g_return_if_fail (progress >= 0.0 && progress <= 1.0);

	g_mutex_lock (job->mutex);
	job->progress = progress;
	g_mutex_unlock (job->mutex);

	g_idle_add (notify_progress, job);
}

static void eoc_job_thumbnail_init (EocJobThumbnail *job) { /* Do Nothing */ }

static void
eoc_job_thumbnail_dispose (GObject *object)
{
	EocJobThumbnail *job;

	job = EOC_JOB_THUMBNAIL (object);

	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	if (job->thumbnail) {
		g_object_unref (job->thumbnail);
		job->thumbnail = NULL;
	}

	(* G_OBJECT_CLASS (eoc_job_thumbnail_parent_class)->dispose) (object);
}

static void
eoc_job_thumbnail_class_init (EocJobThumbnailClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = eoc_job_thumbnail_dispose;

	EOC_JOB_CLASS (class)->run = eoc_job_thumbnail_run;
}

EocJob *
eoc_job_thumbnail_new (EocImage *image)
{
	EocJobThumbnail *job;

	job = g_object_new (EOC_TYPE_JOB_THUMBNAIL, NULL);

	if (image) {
		job->image = g_object_ref (image);
	}

	return EOC_JOB (job);
}

static void
eoc_job_thumbnail_run (EocJob *ejob)
{
	gchar *orig_width, *orig_height;
	gint width, height;
	CdkPixbuf *pixbuf;
	EocJobThumbnail *job;

	g_return_if_fail (EOC_IS_JOB_THUMBNAIL (ejob));

	job = EOC_JOB_THUMBNAIL (ejob);

	if (ejob->error) {
	        g_error_free (ejob->error);
		ejob->error = NULL;
	}

	job->thumbnail = eoc_thumbnail_load (job->image,
					     &ejob->error);

	if (!job->thumbnail) {
		ejob->finished = TRUE;
		return;
	}

	orig_width = g_strdup (cdk_pixbuf_get_option (job->thumbnail, "tEXt::Thumb::Image::Width"));
	orig_height = g_strdup (cdk_pixbuf_get_option (job->thumbnail, "tEXt::Thumb::Image::Height"));

	pixbuf = eoc_thumbnail_fit_to_size (job->thumbnail, EOC_LIST_STORE_THUMB_SIZE);
	g_object_unref (job->thumbnail);
	job->thumbnail = eoc_thumbnail_add_frame (pixbuf);
	g_object_unref (pixbuf);

	if (orig_width) {
		sscanf (orig_width, "%i", &width);
		g_object_set_data (G_OBJECT (job->thumbnail),
				   EOC_THUMBNAIL_ORIGINAL_WIDTH,
				   GINT_TO_POINTER (width));
		g_free (orig_width);
	}
	if (orig_height) {
		sscanf (orig_height, "%i", &height);
		g_object_set_data (G_OBJECT (job->thumbnail),
				   EOC_THUMBNAIL_ORIGINAL_HEIGHT,
				   GINT_TO_POINTER (height));
		g_free (orig_height);
	}

	if (ejob->error) {
		g_warning ("%s", ejob->error->message);
	}

	ejob->finished = TRUE;
}

static void eoc_job_load_init (EocJobLoad *job) { /* Do Nothing */ }

static void
eoc_job_load_dispose (GObject *object)
{
	EocJobLoad *job;

	job = EOC_JOB_LOAD (object);

	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	(* G_OBJECT_CLASS (eoc_job_load_parent_class)->dispose) (object);
}

static void
eoc_job_load_class_init (EocJobLoadClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = eoc_job_load_dispose;
	EOC_JOB_CLASS (class)->run = eoc_job_load_run;
}

EocJob *
eoc_job_load_new (EocImage *image, EocImageData data)
{
	EocJobLoad *job;

	job = g_object_new (EOC_TYPE_JOB_LOAD, NULL);

	if (image) {
		job->image = g_object_ref (image);
	}
	job->data = data;

	return EOC_JOB (job);
}

static void
eoc_job_load_run (EocJob *job)
{
	g_return_if_fail (EOC_IS_JOB_LOAD (job));

	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	eoc_image_load (EOC_IMAGE (EOC_JOB_LOAD (job)->image),
			EOC_JOB_LOAD (job)->data,
			job,
			&job->error);

	job->finished = TRUE;
}

static void eoc_job_model_init (EocJobModel *job) { /* Do Nothing */ }

static void
eoc_job_model_class_init (EocJobModelClass *class)
{
	EOC_JOB_CLASS (class)->run = eoc_job_model_run;
}

/**
 * eoc_job_model_new:
 * @file_list: (element-type GFile): a #GFile list
 *
 * Creates a new #EocJob model.
 *
 * Returns: A #EocJob.
 */

EocJob *
eoc_job_model_new (GSList *file_list)
{
	EocJobModel *job;

	job = g_object_new (EOC_TYPE_JOB_MODEL, NULL);

	job->file_list = file_list;

	return EOC_JOB (job);
}

static void
filter_files (GSList *files, GList **file_list, GList **error_list)
{
	GSList *it;
	GFileInfo *file_info;

	for (it = files; it != NULL; it = it->next) {
		GFile *file;
		GFileType type = G_FILE_TYPE_UNKNOWN;

		file = (GFile *) it->data;

		if (file != NULL) {
			file_info = g_file_query_info (file,
						       G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
						       0, NULL, NULL);
			if (file_info == NULL) {
				type = G_FILE_TYPE_UNKNOWN;
			} else {
				type = g_file_info_get_file_type (file_info);

				/* Workaround for gvfs backends that
				   don't set the GFileType. */
				if (G_UNLIKELY (type == G_FILE_TYPE_UNKNOWN)) {
					const gchar *ctype;

					ctype = g_file_info_get_content_type (file_info);

					/* If the content type is supported
					   adjust the file_type */
					if (eoc_image_is_supported_mime_type (ctype))
						type = G_FILE_TYPE_REGULAR;
				}

				g_object_unref (file_info);
			}
		}

		switch (type) {
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_DIRECTORY:
			*file_list = g_list_prepend (*file_list, g_object_ref (file));
			break;
		default:
			*error_list = g_list_prepend (*error_list,
						      g_file_get_uri (file));
			break;
		}
	}

	*file_list  = g_list_reverse (*file_list);
	*error_list = g_list_reverse (*error_list);
}

static void
eoc_job_model_run (EocJob *ejob)
{
	GList *filtered_list = NULL;
	GList *error_list = NULL;
	EocJobModel *job;

	g_return_if_fail (EOC_IS_JOB_MODEL (ejob));

	job = EOC_JOB_MODEL (ejob);

	filter_files (job->file_list, &filtered_list, &error_list);

	job->store = EOC_LIST_STORE (eoc_list_store_new ());

	eoc_list_store_add_files (job->store, filtered_list);

	g_list_foreach (filtered_list, (GFunc) g_object_unref, NULL);
	g_list_free (filtered_list);

	g_list_foreach (error_list, (GFunc) g_free, NULL);
	g_list_free (error_list);

	ejob->finished = TRUE;
}

static void eoc_job_transform_init (EocJobTransform *job) { /* Do Nothing */ }

static void
eoc_job_transform_dispose (GObject *object)
{
	EocJobTransform *job;

	job = EOC_JOB_TRANSFORM (object);

	if (job->trans) {
		g_object_unref (job->trans);
		job->trans = NULL;
	}

	g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
	g_list_free (job->images);

	(* G_OBJECT_CLASS (eoc_job_transform_parent_class)->dispose) (object);
}

static void
eoc_job_transform_class_init (EocJobTransformClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = eoc_job_transform_dispose;

	EOC_JOB_CLASS (class)->run = eoc_job_transform_run;
}

/**
 * eoc_job_transform_new:
 * @images: (element-type EocImage) (transfer full): a #EocImage list
 * @trans: a #EocTransform
 *
 * Create a new #EocJob for image transformation.
 *
 * Returns: A #EocJob.
 */

EocJob *
eoc_job_transform_new (GList *images, EocTransform *trans)
{
	EocJobTransform *job;

	job = g_object_new (EOC_TYPE_JOB_TRANSFORM, NULL);

	if (trans) {
		job->trans = g_object_ref (trans);
	} else {
		job->trans = NULL;
	}

	job->images = images;

	return EOC_JOB (job);
}

static gboolean
eoc_job_transform_image_modified (gpointer data)
{
	g_return_val_if_fail (EOC_IS_IMAGE (data), FALSE);

	eoc_image_modified (EOC_IMAGE (data));
	g_object_unref (G_OBJECT (data));

	return FALSE;
}

void
eoc_job_transform_run (EocJob *ejob)
{
	EocJobTransform *job;
	GList *it;

	g_return_if_fail (EOC_IS_JOB_TRANSFORM (ejob));

	job = EOC_JOB_TRANSFORM (ejob);

	if (ejob->error) {
	        g_error_free (ejob->error);
		ejob->error = NULL;
	}

	for (it = job->images; it != NULL; it = it->next) {
		EocImage *image = EOC_IMAGE (it->data);

		if (job->trans == NULL) {
			eoc_image_undo (image);
		} else {
			eoc_image_transform (image, job->trans, ejob);
		}

		if (eoc_image_is_modified (image) || job->trans == NULL) {
			g_object_ref (image);
			g_idle_add (eoc_job_transform_image_modified, image);
		}
	}

	ejob->finished = TRUE;
}

static void eoc_job_save_init (EocJobSave *job) { /* do nothing */ }

static void
eoc_job_save_dispose (GObject *object)
{
	EocJobSave *job;

	job = EOC_JOB_SAVE (object);

	if (job->images) {
		g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
		g_list_free (job->images);
		job->images = NULL;
	}

	(* G_OBJECT_CLASS (eoc_job_save_parent_class)->dispose) (object);
}

static void
eoc_job_save_class_init (EocJobSaveClass *class)
{
	G_OBJECT_CLASS (class)->dispose = eoc_job_save_dispose;
	EOC_JOB_CLASS (class)->run = eoc_job_save_run;
}

/**
 * eoc_job_save_new:
 * @images: (element-type EocImage) (transfer full): a #EocImage list
 *
 * Creates a new #EocJob for image saving.
 *
 * Returns: A #EocJob.
 */

EocJob *
eoc_job_save_new (GList *images)
{
	EocJobSave *job;

	job = g_object_new (EOC_TYPE_JOB_SAVE, NULL);

	job->images = images;
	job->current_image = NULL;

	return EOC_JOB (job);
}

static void
save_progress_handler (EocImage *image, gfloat progress, gpointer data)
{
	EocJobSave *job = EOC_JOB_SAVE (data);
	guint n_images = g_list_length (job->images);
	gfloat job_progress;

	job_progress = (job->current_pos / (gfloat) n_images) + (progress / n_images);

	eoc_job_set_progress (EOC_JOB (job), job_progress);
}

static void
eoc_job_save_run (EocJob *ejob)
{
	EocJobSave *job;
	GList *it;

	g_return_if_fail (EOC_IS_JOB_SAVE (ejob));

	job = EOC_JOB_SAVE (ejob);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = it->next, job->current_pos++) {
		EocImage *image = EOC_IMAGE (it->data);
		EocImageSaveInfo *save_info = NULL;
		gulong handler_id = 0;
		gboolean success = FALSE;

		job->current_image = image;

		/* Make sure the image doesn't go away while saving */
		eoc_image_data_ref (image);

		if (!eoc_image_has_data (image, EOC_IMAGE_DATA_ALL)) {
			EocImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = eoc_image_get_metadata_status (image);
			if (!eoc_image_has_data (image, EOC_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = EOC_IMAGE_DATA_ALL;
			} else if (m_status == EOC_IMAGE_METADATA_NOT_READ) {
				// Load only if we haven't read it yet
				data2load = EOC_IMAGE_DATA_EXIF | EOC_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				eoc_image_load (image,
						data2load,
						NULL,
						&ejob->error);
			}
		}

		handler_id = g_signal_connect (G_OBJECT (image),
					       "save-progress",
				               G_CALLBACK (save_progress_handler),
					       job);

		save_info = eoc_image_save_info_new_from_image (image);

		success = eoc_image_save_by_info (image,
						  save_info,
						  &ejob->error);

		if (save_info)
			g_object_unref (save_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		eoc_image_data_unref (image);

		if (!success) break;
	}

	ejob->finished = TRUE;
}

static void eoc_job_save_as_init (EocJobSaveAs *job) { /* do nothing */ }

static void eoc_job_save_as_dispose (GObject *object)
{
	EocJobSaveAs *job = EOC_JOB_SAVE_AS (object);

	if (job->converter != NULL) {
		g_object_unref (job->converter);
		job->converter = NULL;
	}

	if (job->file != NULL) {
		g_object_unref (job->file);
		job->file = NULL;
	}

	(* G_OBJECT_CLASS (eoc_job_save_as_parent_class)->dispose) (object);
}

static void
eoc_job_save_as_class_init (EocJobSaveAsClass *class)
{
	G_OBJECT_CLASS (class)->dispose = eoc_job_save_as_dispose;
	EOC_JOB_CLASS (class)->run = eoc_job_save_as_run;
}

/**
 * eoc_job_save_as_new:
 * @images: (element-type EocImage) (transfer full): a #EocImage list
 * @converter: a URI converter
 * file: a #GFile
 *
 * Creates a new #EocJob for save as.
 *
 * Returns: A #EocJob.
 */

EocJob *
eoc_job_save_as_new (GList *images, EocURIConverter *converter, GFile *file)
{
	EocJobSaveAs *job;

	g_assert (converter != NULL || g_list_length (images) == 1);

	job = g_object_new (EOC_TYPE_JOB_SAVE_AS, NULL);

	EOC_JOB_SAVE(job)->images = images;

	job->converter = converter ? g_object_ref (converter) : NULL;
	job->file = file ? g_object_ref (file) : NULL;

	return EOC_JOB (job);
}

static void
eoc_job_save_as_run (EocJob *ejob)
{
	EocJobSave *job;
	EocJobSaveAs *saveas_job;
	GList *it;
	guint n_images;

	g_return_if_fail (EOC_IS_JOB_SAVE_AS (ejob));

	job = EOC_JOB_SAVE (ejob);

	n_images = g_list_length (job->images);

	saveas_job = EOC_JOB_SAVE_AS (job);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = it->next, job->current_pos++) {
		CdkPixbufFormat *format;
		EocImageSaveInfo *src_info, *dest_info;
		EocImage *image = EOC_IMAGE (it->data);
		gboolean success = FALSE;
		gulong handler_id = 0;

		job->current_image = image;

		eoc_image_data_ref (image);

		if (!eoc_image_has_data (image, EOC_IMAGE_DATA_ALL)) {
			EocImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = eoc_image_get_metadata_status (image);
			if (!eoc_image_has_data (image, EOC_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = EOC_IMAGE_DATA_ALL;
			} else if (m_status == EOC_IMAGE_METADATA_NOT_READ) {
				// Load only if we haven't read it yet
				data2load = EOC_IMAGE_DATA_EXIF | EOC_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				eoc_image_load (image,
						data2load,
						NULL,
						&ejob->error);
			}
		}

		g_assert (ejob->error == NULL);

		handler_id = g_signal_connect (G_OBJECT (image),
					       "save-progress",
				               G_CALLBACK (save_progress_handler),
					       job);

		src_info = eoc_image_save_info_new_from_image (image);

		if (n_images == 1) {
			g_assert (saveas_job->file != NULL);

			format = eoc_pixbuf_get_format (saveas_job->file);

			dest_info = eoc_image_save_info_new_from_file (saveas_job->file,
								   format);

		/* SaveAsDialog has already secured permission to overwrite */
			if (dest_info->exists) {
				dest_info->overwrite = TRUE;
			}
		} else {
			GFile *dest_file;
			gboolean result;

			result = eoc_uri_converter_do (saveas_job->converter,
						       image,
						       &dest_file,
						       &format,
						       NULL);

			g_assert (result);

			dest_info = eoc_image_save_info_new_from_file (dest_file,
								   format);
		}

		success = eoc_image_save_as_by_info (image,
						     src_info,
						     dest_info,
						     &ejob->error);

		if (src_info)
			g_object_unref (src_info);

		if (dest_info)
			g_object_unref (dest_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		eoc_image_data_unref (image);

		if (!success)
			break;
	}

	ejob->finished = TRUE;
}

static void eoc_job_copy_init (EocJobCopy *job) { /* do nothing */};

static void
eoc_job_copy_dispose (GObject *object)
{
	EocJobCopy *job = EOC_JOB_COPY (object);

	if (job->dest) {
		g_free (job->dest);
		job->dest = NULL;
	}

	(* G_OBJECT_CLASS (eoc_job_copy_parent_class)->dispose) (object);
}

static void
eoc_job_copy_class_init (EocJobCopyClass *class)
{
	G_OBJECT_CLASS (class)->dispose = eoc_job_copy_dispose;
	EOC_JOB_CLASS (class)->run = eoc_job_copy_run;
}

/**
 * eoc_job_copy_new:
 * @images: (element-type EocImage) (transfer full): a #EocImage list
 * @dest: destination path for the copy
 *
 * Creates a new #EocJob.
 *
 * Returns: A #EocJob.
 */

EocJob *
eoc_job_copy_new (GList *images, const gchar *dest)
{
	EocJobCopy *job;

	g_assert (images != NULL && dest != NULL);

	job = g_object_new (EOC_TYPE_JOB_COPY, NULL);

	job->images = images;
	job->dest = g_strdup (dest);

	return EOC_JOB (job);
}

static void
eoc_job_copy_progress_callback (goffset current_num_bytes,
				goffset total_num_bytes,
				gpointer user_data)
{
	gfloat job_progress;
	guint n_images;
	EocJobCopy *job;

	job = EOC_JOB_COPY (user_data);
	n_images = g_list_length (job->images);

	job_progress =  ((current_num_bytes / (gfloat) total_num_bytes) + job->current_pos)/n_images;

	eoc_job_set_progress (EOC_JOB (job), job_progress);
}

void
eoc_job_copy_run (EocJob *ejob)
{
	EocJobCopy *job;
	GList *it;
	GFile *src, *dest;
	gchar *filename, *dest_filename;

	g_return_if_fail (EOC_IS_JOB_COPY (ejob));

	job = EOC_JOB_COPY (ejob);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = g_list_next (it), job->current_pos++) {
		src = (GFile *) it->data;
		filename = g_file_get_basename (src);
		dest_filename = g_build_filename (job->dest, filename, NULL);
		dest = g_file_new_for_path (dest_filename);

		g_file_copy (src, dest,
			     G_FILE_COPY_OVERWRITE, NULL,
			     eoc_job_copy_progress_callback, job,
			     &ejob->error);
		g_free (filename);
		g_free (dest_filename);
	}

	ejob->finished = TRUE;
}
