/* Eye Of Cafe - Jobs
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
#include <ctk/ctk.h>
#include <cdk-pixbuf/cdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
  typedef struct _EocImage EocImage;
#endif

#ifndef __EOC_URI_CONVERTER_DECLR__
#define __EOC_URI_CONVERTER_DECLR__
typedef struct _EocURIConverter EocURIConverter;
#endif

#ifndef __EOC_JOB_DECLR__
#define __EOC_JOB_DECLR__
typedef struct _EocJob EocJob;
#endif
typedef struct _EocJobClass EocJobClass;

typedef struct _EocJobThumbnail EocJobThumbnail;
typedef struct _EocJobThumbnailClass EocJobThumbnailClass;

typedef struct _EocJobLoad EocJobLoad;
typedef struct _EocJobLoadClass EocJobLoadClass;

typedef struct _EocJobModel EocJobModel;
typedef struct _EocJobModelClass EocJobModelClass;

typedef struct _EocJobTransform EocJobTransform;
typedef struct _EocJobTransformClass EocJobTransformClass;

typedef struct _EocJobSave EocJobSave;
typedef struct _EocJobSaveClass EocJobSaveClass;

typedef struct _EocJobSaveAs EocJobSaveAs;
typedef struct _EocJobSaveAsClass EocJobSaveAsClass;

typedef struct _EocJobCopy EocJobCopy;
typedef struct _EocJobCopyClass EocJobCopyClass;

#define EOC_TYPE_JOB		       (eoc_job_get_type())
#define EOC_JOB(obj)		       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB, EocJob))
#define EOC_JOB_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB, EocJobClass))
#define EOC_IS_JOB(obj)	               (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB))
#define EOC_JOB_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_JOB, EocJobClass))

#define EOC_TYPE_JOB_THUMBNAIL	       (eoc_job_thumbnail_get_type())
#define EOC_JOB_THUMBNAIL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_THUMBNAIL, EocJobThumbnail))
#define EOC_JOB_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_THUMBNAIL, EocJobThumbnailClass))
#define EOC_IS_JOB_THUMBNAIL(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_THUMBNAIL))

#define EOC_TYPE_JOB_LOAD	       (eoc_job_load_get_type())
#define EOC_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_LOAD, EocJobLoad))
#define EOC_JOB_LOAD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_LOAD, EocJobLoadClass))
#define EOC_IS_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_LOAD))

#define EOC_TYPE_JOB_MODEL	       (eoc_job_model_get_type())
#define EOC_JOB_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_MODEL, EocJobModel))
#define EOC_JOB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_MODEL, EocJobModelClass))
#define EOC_IS_JOB_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_MODEL))

#define EOC_TYPE_JOB_TRANSFORM	       (eoc_job_transform_get_type())
#define EOC_JOB_TRANSFORM(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_TRANSFORM, EocJobTransform))
#define EOC_JOB_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_TRANSFORM, EocJobTransformClass))
#define EOC_IS_JOB_TRANSFORM(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_TRANSFORM))

#define EOC_TYPE_JOB_SAVE              (eoc_job_save_get_type())
#define EOC_JOB_SAVE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_SAVE, EocJobSave))
#define EOC_JOB_SAVE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), EOC_TYPE_JOB_SAVE, EocJobSaveClass))
#define EOC_IS_JOB_SAVE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_SAVE))
#define EOC_JOB_SAVE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_JOB_SAVE, EocJobSaveClass))

#define EOC_TYPE_JOB_SAVE_AS           (eoc_job_save_as_get_type())
#define EOC_JOB_SAVE_AS(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_SAVE_AS, EocJobSaveAs))
#define EOC_JOB_SAVE_AS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass), EOC_TYPE_JOB_SAVE_AS, EocJobSaveAsClass))
#define EOC_IS_JOB_SAVE_AS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_SAVE_AS))

#define EOC_TYPE_JOB_COPY	       (eoc_job_copy_get_type())
#define EOC_JOB_COPY(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_JOB_COPY, EocJobCopy))
#define EOC_JOB_COPY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_JOB_COPY, EocJobCopyClass))
#define EOC_IS_JOB_COPY(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_JOB_COPY))


struct _EocJob
{
	GObject  parent;

	GError   *error;
	GMutex   *mutex;
	float     progress;
	gboolean  finished;
};

struct _EocJobClass
{
	GObjectClass parent_class;

	void    (* finished) (EocJob *job);
	void    (* progress) (EocJob *job, float progress);
	void    (*run)       (EocJob *job);
};

struct _EocJobThumbnail
{
	EocJob       parent;
	EocImage    *image;
	CdkPixbuf   *thumbnail;
};

struct _EocJobThumbnailClass
{
	EocJobClass parent_class;
};

struct _EocJobLoad
{
	EocJob        parent;
	EocImage     *image;
	EocImageData  data;
};

struct _EocJobLoadClass
{
	EocJobClass parent_class;
};

struct _EocJobModel
{
	EocJob        parent;
	EocListStore *store;
	GSList       *file_list;
};

struct _EocJobModelClass
{
        EocJobClass parent_class;
};

struct _EocJobTransform
{
	EocJob        parent;
	GList        *images;
	EocTransform *trans;
};

struct _EocJobTransformClass
{
        EocJobClass parent_class;
};

typedef enum {
	EOC_SAVE_RESPONSE_NONE,
	EOC_SAVE_RESPONSE_RETRY,
	EOC_SAVE_RESPONSE_SKIP,
	EOC_SAVE_RESPONSE_OVERWRITE,
	EOC_SAVE_RESPONSE_CANCEL,
	EOC_SAVE_RESPONSE_LAST
} EocJobSaveResponse;

struct _EocJobSave
{
	EocJob    parent;
	GList	 *images;
	guint      current_pos;
	EocImage *current_image;
};

struct _EocJobSaveClass
{
	EocJobClass parent_class;
};

struct _EocJobSaveAs
{
	EocJobSave       parent;
	EocURIConverter *converter;
	GFile           *file;
};

struct _EocJobSaveAsClass
{
	EocJobSaveClass parent;
};

struct _EocJobCopy
{
	EocJob parent;
	GList *images;
	guint current_pos;
	gchar *dest;
};

struct _EocJobCopyClass
{
	EocJobClass parent_class;
};

/* base job class */
GType           eoc_job_get_type           (void) G_GNUC_CONST;
void            eoc_job_finished           (EocJob          *job);
void            eoc_job_run                (EocJob          *job);
void            eoc_job_set_progress       (EocJob          *job,
					    float            progress);

/* EocJobThumbnail */
GType           eoc_job_thumbnail_get_type (void) G_GNUC_CONST;
EocJob         *eoc_job_thumbnail_new      (EocImage     *image);

/* EocJobLoad */
GType           eoc_job_load_get_type      (void) G_GNUC_CONST;
EocJob 	       *eoc_job_load_new 	   (EocImage        *image,
					    EocImageData     data);

/* EocJobModel */
GType 		eoc_job_model_get_type     (void) G_GNUC_CONST;
EocJob 	       *eoc_job_model_new          (GSList          *file_list);

/* EocJobTransform */
GType 		eoc_job_transform_get_type (void) G_GNUC_CONST;
EocJob 	       *eoc_job_transform_new      (GList           *images,
					    EocTransform    *trans);

/* EocJobSave */
GType		eoc_job_save_get_type      (void) G_GNUC_CONST;
EocJob         *eoc_job_save_new           (GList           *images);

/* EocJobSaveAs */
GType		eoc_job_save_as_get_type   (void) G_GNUC_CONST;
EocJob         *eoc_job_save_as_new        (GList           *images,
					    EocURIConverter *converter,
					    GFile           *file);

/* EocJobCopy */
GType          eoc_job_copy_get_type      (void) G_GNUC_CONST;
EocJob        *eoc_job_copy_new           (GList            *images,
					   const gchar      *dest);

G_END_DECLS

#endif /* __EOC_JOBS_H__ */
