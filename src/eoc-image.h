/* Eye Of Cafe - Image
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
#include <cdk-pixbuf/cdk-pixbuf.h>

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
typedef struct _EocImage EocImage;
#endif
typedef struct _EocImageClass EocImageClass;
typedef struct _EocImagePrivate EocImagePrivate;

#define EOC_TYPE_IMAGE            (eoc_image_get_type ())
#define EOC_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_IMAGE, EocImage))
#define EOC_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_IMAGE, EocImageClass))
#define EOC_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_IMAGE))
#define EOC_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_IMAGE))
#define EOC_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_IMAGE, EocImageClass))

typedef enum {
	EOC_IMAGE_ERROR_SAVE_NOT_LOCAL,
	EOC_IMAGE_ERROR_NOT_LOADED,
	EOC_IMAGE_ERROR_VFS,
	EOC_IMAGE_ERROR_FILE_EXISTS,
	EOC_IMAGE_ERROR_TMP_FILE_FAILED,
	EOC_IMAGE_ERROR_GENERIC,
	EOC_IMAGE_ERROR_UNKNOWN
} EocImageError;

#define EOC_IMAGE_ERROR eoc_image_error_quark ()

typedef enum {
	EOC_IMAGE_STATUS_UNKNOWN,
	EOC_IMAGE_STATUS_LOADING,
	EOC_IMAGE_STATUS_LOADED,
	EOC_IMAGE_STATUS_SAVING,
	EOC_IMAGE_STATUS_FAILED
} EocImageStatus;

typedef enum {
  EOC_IMAGE_METADATA_NOT_READ,
  EOC_IMAGE_METADATA_NOT_AVAILABLE,
  EOC_IMAGE_METADATA_READY
} EocImageMetadataStatus;

struct _EocImage {
	GObject parent;

	EocImagePrivate *priv;
};

struct _EocImageClass {
	GObjectClass parent_class;

	void (* changed) 	   (EocImage *img);

	void (* size_prepared)     (EocImage *img,
				    int       width,
				    int       height);

	void (* thumbnail_changed) (EocImage *img);

	void (* save_progress)     (EocImage *img,
				    gfloat    progress);

	void (* next_frame)        (EocImage *img,
				    gint delay);

	void (* file_changed)      (EocImage *img);
};

GType	          eoc_image_get_type	             (void) G_GNUC_CONST;

GQuark            eoc_image_error_quark              (void);

EocImage         *eoc_image_new_file                 (GFile *file, const gchar *caption);

gboolean          eoc_image_load                     (EocImage   *img,
					              EocImageData data2read,
					              EocJob     *job,
					              GError    **error);

void              eoc_image_cancel_load              (EocImage   *img);

gboolean          eoc_image_has_data                 (EocImage   *img,
					              EocImageData data);

void              eoc_image_data_ref                 (EocImage   *img);

void              eoc_image_data_unref               (EocImage   *img);

void              eoc_image_set_thumbnail            (EocImage   *img,
					              GdkPixbuf  *pixbuf);

gboolean          eoc_image_save_as_by_info          (EocImage   *img,
		      			              EocImageSaveInfo *source,
		      			              EocImageSaveInfo *target,
		      			              GError    **error);

gboolean          eoc_image_save_by_info             (EocImage   *img,
					              EocImageSaveInfo *source,
					              GError    **error);

GdkPixbuf*        eoc_image_get_pixbuf               (EocImage   *img);

GdkPixbuf*        eoc_image_get_thumbnail            (EocImage   *img);

void              eoc_image_get_size                 (EocImage   *img,
					              gint       *width,
					              gint       *height);

goffset           eoc_image_get_bytes                (EocImage   *img);

gboolean          eoc_image_is_modified              (EocImage   *img);

void              eoc_image_modified                 (EocImage   *img);

const gchar*      eoc_image_get_caption              (EocImage   *img);

const gchar      *eoc_image_get_collate_key          (EocImage   *img);

#ifdef HAVE_EXIF
ExifData*         eoc_image_get_exif_info            (EocImage   *img);
#endif

gpointer          eoc_image_get_xmp_info             (EocImage   *img);

GFile*            eoc_image_get_file                 (EocImage   *img);

gchar*            eoc_image_get_uri_for_display      (EocImage   *img);

EocImageStatus    eoc_image_get_status               (EocImage   *img);

EocImageMetadataStatus eoc_image_get_metadata_status (EocImage   *img);

void              eoc_image_transform                (EocImage   *img,
						      EocTransform *trans,
						      EocJob     *job);

void              eoc_image_autorotate               (EocImage   *img);

#ifdef HAVE_LCMS
cmsHPROFILE       eoc_image_get_profile              (EocImage    *img);

void              eoc_image_apply_display_profile    (EocImage    *img,
						      cmsHPROFILE  display_profile);
#endif

void              eoc_image_undo                     (EocImage   *img);

GList		 *eoc_image_get_supported_mime_types (void);

gboolean          eoc_image_is_supported_mime_type   (const char *mime_type);

gboolean          eoc_image_is_animation             (EocImage *img);

gboolean          eoc_image_start_animation          (EocImage *img);

#ifdef HAVE_RSVG
gboolean          eoc_image_is_svg                   (EocImage *img);
RsvgHandle       *eoc_image_get_svg                  (EocImage *img);
#endif

EocTransform     *eoc_image_get_transform            (EocImage *img);
EocTransform     *eoc_image_get_autorotate_transform (EocImage *img);

gboolean          eoc_image_is_jpeg                  (EocImage *img);

void              eoc_image_file_changed             (EocImage *img);
gboolean          eoc_image_is_file_changed         (EocImage *img);

G_END_DECLS

#endif /* __EOC_IMAGE_H__ */
