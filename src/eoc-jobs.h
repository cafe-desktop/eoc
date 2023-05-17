/* Eye Of Mate - Jobs
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-jobs.h) by:
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

#ifndef __EOC_JOBS_H__
#define __EOC_JOBS_H__

#include "eoc-list-store.h"
#include "eoc-transform.h"
#include "eoc-enums.h"

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
  typedef struct _EomImage EomImage;
#endif

#ifndef __EOC_URI_CONVERTER_DECLR__
#define __EOC_URI_CONVERTER_DECLR__
typedef struct _EomURIConverter EomURIConverter;
#endif

#ifndef __EOC_JOB_DECLR__
#define __EOC_JOB_DECLR__
typedef struct _EomJob EomJob;
#endif
typedef struct _EomJobClass EomJobClass;

typedef struct _EomJobThumbnail EomJobThumbnail;
typedef struct _EomJobThumbnailClass EomJobThumbnailClass;

typedef struct _EomJobLoad EomJobLoad;
typedef struct _EomJobLoadClass EomJobLoadClass;

typedef struct _EomJobModel EomJobModel;
typedef struct _EomJobModelClass EomJobModelClass;

typedef struct _EomJobTransform EomJobTransform;
typedef struct _EomJobTransformClass EomJobTransformClass;

typedef struct _EomJobSave EomJobSave;
typedef struct _EomJobSaveClass EomJobSaveClass;

typedef struct _EomJobSaveAs EomJobSaveAs;
typedef struct _EomJobSaveAsClass EomJobSaveAsClass;

typedef struct _EomJobCopy EomJobCopy;
typedef struct _EomJobCopyClass EomJobCopyClass;

#define EOC_TYPE_JOB		       (eoc_job_get_type())
#define EOC_JOB(obj)		       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB, EomJob))
#define EOC_JOB_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB, EomJobClass))
#define EOC_IS_JOB(obj)	               (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB))
#define EOC_JOB_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_JOB, EomJobClass))

#define EOC_TYPE_JOB_THUMBNAIL	       (eoc_job_thumbnail_get_type())
#define EOC_JOB_THUMBNAIL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_THUMBNAIL, EomJobThumbnail))
#define EOC_JOB_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_THUMBNAIL, EomJobThumbnailClass))
#define EOC_IS_JOB_THUMBNAIL(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_THUMBNAIL))

#define EOC_TYPE_JOB_LOAD	       (eoc_job_load_get_type())
#define EOC_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_LOAD, EomJobLoad))
#define EOC_JOB_LOAD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_LOAD, EomJobLoadClass))
#define EOC_IS_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_LOAD))

#define EOC_TYPE_JOB_MODEL	       (eoc_job_model_get_type())
#define EOC_JOB_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_MODEL, EomJobModel))
#define EOC_JOB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_MODEL, EomJobModelClass))
#define EOC_IS_JOB_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_MODEL))

#define EOC_TYPE_JOB_TRANSFORM	       (eoc_job_transform_get_type())
#define EOC_JOB_TRANSFORM(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_TRANSFORM, EomJobTransform))
#define EOC_JOB_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_TRANSFORM, EomJobTransformClass))
#define EOC_IS_JOB_TRANSFORM(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_TRANSFORM))

#define EOC_TYPE_JOB_SAVE              (eoc_job_save_get_type())
#define EOC_JOB_SAVE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_SAVE, EomJobSave))
#define EOC_JOB_SAVE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), EOC_TYPE_JOB_SAVE, EomJobSaveClass))
#define EOC_IS_JOB_SAVE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_SAVE))
#define EOC_JOB_SAVE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_JOB_SAVE, EomJobSaveClass))

#define EOC_TYPE_JOB_SAVE_AS           (eoc_job_save_as_get_type())
#define EOC_JOB_SAVE_AS(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_SAVE_AS, EomJobSaveAs))
#define EOC_JOB_SAVE_AS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass), EOC_TYPE_JOB_SAVE_AS, EomJobSaveAsClass))
#define EOC_IS_JOB_SAVE_AS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_SAVE_AS))

#define EOC_TYPE_JOB_COPY	       (eoc_job_copy_get_type())
#define EOC_JOB_COPY(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_COPY, EomJobCopy))
#define EOC_JOB_COPY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_COPY, EomJobCopyClass))
#define EOC_IS_JOB_COPY(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_COPY))


struct _EomJob
{
	GObject  parent;

	GError   *error;
	GMutex   *mutex;
	float     progress;
	gboolean  finished;
};

struct _EomJobClass
{
	GObjectClass parent_class;

	void    (* finished) (EomJob *job);
	void    (* progress) (EomJob *job, float progress);
	void    (*run)       (EomJob *job);
};

struct _EomJobThumbnail
{
	EomJob       parent;
	EomImage    *image;
	GdkPixbuf   *thumbnail;
};

struct _EomJobThumbnailClass
{
	EomJobClass parent_class;
};

struct _EomJobLoad
{
	EomJob        parent;
	EomImage     *image;
	EomImageData  data;
};

struct _EomJobLoadClass
{
	EomJobClass parent_class;
};

struct _EomJobModel
{
	EomJob        parent;
	EomListStore *store;
	GSList       *file_list;
};

struct _EomJobModelClass
{
        EomJobClass parent_class;
};

struct _EomJobTransform
{
	EomJob        parent;
	GList        *images;
	EomTransform *trans;
};

struct _EomJobTransformClass
{
        EomJobClass parent_class;
};

typedef enum {
	EOC_SAVE_RESPONSE_NONE,
	EOC_SAVE_RESPONSE_RETRY,
	EOC_SAVE_RESPONSE_SKIP,
	EOC_SAVE_RESPONSE_OVERWRITE,
	EOC_SAVE_RESPONSE_CANCEL,
	EOC_SAVE_RESPONSE_LAST
} EomJobSaveResponse;

struct _EomJobSave
{
	EomJob    parent;
	GList	 *images;
	guint      current_pos;
	EomImage *current_image;
};

struct _EomJobSaveClass
{
	EomJobClass parent_class;
};

struct _EomJobSaveAs
{
	EomJobSave       parent;
	EomURIConverter *converter;
	GFile           *file;
};

struct _EomJobSaveAsClass
{
	EomJobSaveClass parent;
};

struct _EomJobCopy
{
	EomJob parent;
	GList *images;
	guint current_pos;
	gchar *dest;
};

struct _EomJobCopyClass
{
	EomJobClass parent_class;
};

/* base job class */
GType           eoc_job_get_type           (void) G_GNUC_CONST;
void            eoc_job_finished           (EomJob          *job);
void            eoc_job_run                (EomJob          *job);
void            eoc_job_set_progress       (EomJob          *job,
					    float            progress);

/* EomJobThumbnail */
GType           eoc_job_thumbnail_get_type (void) G_GNUC_CONST;
EomJob         *eoc_job_thumbnail_new      (EomImage     *image);

/* EomJobLoad */
GType           eoc_job_load_get_type      (void) G_GNUC_CONST;
EomJob 	       *eoc_job_load_new 	   (EomImage        *image,
					    EomImageData     data);

/* EomJobModel */
GType 		eoc_job_model_get_type     (void) G_GNUC_CONST;
EomJob 	       *eoc_job_model_new          (GSList          *file_list);

/* EomJobTransform */
GType 		eoc_job_transform_get_type (void) G_GNUC_CONST;
EomJob 	       *eoc_job_transform_new      (GList           *images,
					    EomTransform    *trans);

/* EomJobSave */
GType		eoc_job_save_get_type      (void) G_GNUC_CONST;
EomJob         *eoc_job_save_new           (GList           *images);

/* EomJobSaveAs */
GType		eoc_job_save_as_get_type   (void) G_GNUC_CONST;
EomJob         *eoc_job_save_as_new        (GList           *images,
					    EomURIConverter *converter,
					    GFile           *file);

/* EomJobCopy */
GType          eoc_job_copy_get_type      (void) G_GNUC_CONST;
EomJob        *eoc_job_copy_new           (GList            *images,
					   const gchar      *dest);

G_END_DECLS

#endif /* __EOC_JOBS_H__ */
