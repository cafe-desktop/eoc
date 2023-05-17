/* Eye Of Mate - Image
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#ifndef __EOC_IMAGE_H__
#define __EOC_IMAGE_H__

#include "eoc-jobs.h"
#include "eoc-window.h"
#include "eoc-transform.h"
#include "eoc-image-save-info.h"
#include "eoc-enums.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#include "eoc-exif-util.h"
#endif

#ifdef HAVE_LCMS
#include <lcms2.h>
#endif

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
typedef struct _EomImage EomImage;
#endif
typedef struct _EomImageClass EomImageClass;
typedef struct _EomImagePrivate EomImagePrivate;

#define EOC_TYPE_IMAGE            (eoc_image_get_type ())
#define EOC_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_IMAGE, EomImage))
#define EOC_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_IMAGE, EomImageClass))
#define EOC_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_IMAGE))
#define EOC_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_IMAGE))
#define EOC_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_IMAGE, EomImageClass))

typedef enum {
	EOC_IMAGE_ERROR_SAVE_NOT_LOCAL,
	EOC_IMAGE_ERROR_NOT_LOADED,
	EOC_IMAGE_ERROR_VFS,
	EOC_IMAGE_ERROR_FILE_EXISTS,
	EOC_IMAGE_ERROR_TMP_FILE_FAILED,
	EOC_IMAGE_ERROR_GENERIC,
	EOC_IMAGE_ERROR_UNKNOWN
} EomImageError;

#define EOC_IMAGE_ERROR eoc_image_error_quark ()

typedef enum {
	EOC_IMAGE_STATUS_UNKNOWN,
	EOC_IMAGE_STATUS_LOADING,
	EOC_IMAGE_STATUS_LOADED,
	EOC_IMAGE_STATUS_SAVING,
	EOC_IMAGE_STATUS_FAILED
} EomImageStatus;

typedef enum {
  EOC_IMAGE_METADATA_NOT_READ,
  EOC_IMAGE_METADATA_NOT_AVAILABLE,
  EOC_IMAGE_METADATA_READY
} EomImageMetadataStatus;

struct _EomImage {
	GObject parent;

	EomImagePrivate *priv;
};

struct _EomImageClass {
	GObjectClass parent_class;

	void (* changed) 	   (EomImage *img);

	void (* size_prepared)     (EomImage *img,
				    int       width,
				    int       height);

	void (* thumbnail_changed) (EomImage *img);

	void (* save_progress)     (EomImage *img,
				    gfloat    progress);

	void (* next_frame)        (EomImage *img,
				    gint delay);

	void (* file_changed)      (EomImage *img);
};

GType	          eoc_image_get_type	             (void) G_GNUC_CONST;

GQuark            eoc_image_error_quark              (void);

EomImage         *eoc_image_new_file                 (GFile *file, const gchar *caption);

gboolean          eoc_image_load                     (EomImage   *img,
					              EomImageData data2read,
					              EomJob     *job,
					              GError    **error);

void              eoc_image_cancel_load              (EomImage   *img);

gboolean          eoc_image_has_data                 (EomImage   *img,
					              EomImageData data);

void              eoc_image_data_ref                 (EomImage   *img);

void              eoc_image_data_unref               (EomImage   *img);

void              eoc_image_set_thumbnail            (EomImage   *img,
					              GdkPixbuf  *pixbuf);

gboolean          eoc_image_save_as_by_info          (EomImage   *img,
		      			              EomImageSaveInfo *source,
		      			              EomImageSaveInfo *target,
		      			              GError    **error);

gboolean          eoc_image_save_by_info             (EomImage   *img,
					              EomImageSaveInfo *source,
					              GError    **error);

GdkPixbuf*        eoc_image_get_pixbuf               (EomImage   *img);

GdkPixbuf*        eoc_image_get_thumbnail            (EomImage   *img);

void              eoc_image_get_size                 (EomImage   *img,
					              gint       *width,
					              gint       *height);

goffset           eoc_image_get_bytes                (EomImage   *img);

gboolean          eoc_image_is_modified              (EomImage   *img);

void              eoc_image_modified                 (EomImage   *img);

const gchar*      eoc_image_get_caption              (EomImage   *img);

const gchar      *eoc_image_get_collate_key          (EomImage   *img);

#ifdef HAVE_EXIF
ExifData*         eoc_image_get_exif_info            (EomImage   *img);
#endif

gpointer          eoc_image_get_xmp_info             (EomImage   *img);

GFile*            eoc_image_get_file                 (EomImage   *img);

gchar*            eoc_image_get_uri_for_display      (EomImage   *img);

EomImageStatus    eoc_image_get_status               (EomImage   *img);

EomImageMetadataStatus eoc_image_get_metadata_status (EomImage   *img);

void              eoc_image_transform                (EomImage   *img,
						      EomTransform *trans,
						      EomJob     *job);

void              eoc_image_autorotate               (EomImage   *img);

#ifdef HAVE_LCMS
cmsHPROFILE       eoc_image_get_profile              (EomImage    *img);

void              eoc_image_apply_display_profile    (EomImage    *img,
						      cmsHPROFILE  display_profile);
#endif

void              eoc_image_undo                     (EomImage   *img);

GList		 *eoc_image_get_supported_mime_types (void);

gboolean          eoc_image_is_supported_mime_type   (const char *mime_type);

gboolean          eoc_image_is_animation             (EomImage *img);

gboolean          eoc_image_start_animation          (EomImage *img);

#ifdef HAVE_RSVG
gboolean          eoc_image_is_svg                   (EomImage *img);
RsvgHandle       *eoc_image_get_svg                  (EomImage *img);
#endif

EomTransform     *eoc_image_get_transform            (EomImage *img);
EomTransform     *eoc_image_get_autorotate_transform (EomImage *img);

gboolean          eoc_image_is_jpeg                  (EomImage *img);

void              eoc_image_file_changed             (EomImage *img);
gboolean          eoc_image_is_file_changed         (EomImage *img);

G_END_DECLS

#endif /* __EOC_IMAGE_H__ */
