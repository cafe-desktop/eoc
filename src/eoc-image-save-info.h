#ifndef _EOC_IMAGE_SAVE_INFO_H_
#define _EOC_IMAGE_SAVE_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
typedef struct _EomImage EomImage;
#endif

#define EOC_TYPE_IMAGE_SAVE_INFO            (eoc_image_save_info_get_type ())
#define EOC_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_IMAGE_SAVE_INFO, EomImageSaveInfo))
#define EOC_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_IMAGE_SAVE_INFO, EomImageSaveInfoClass))
#define EOC_IS_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_IMAGE_SAVE_INFO))
#define EOC_IS_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_IMAGE_SAVE_INFO))
#define EOC_IMAGE_SAVE_INFO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_IMAGE_SAVE_INFO, EomImageSaveInfoClass))

typedef struct _EomImageSaveInfo EomImageSaveInfo;
typedef struct _EomImageSaveInfoClass EomImageSaveInfoClass;

struct _EomImageSaveInfo {
	GObject parent;

	GFile       *file;
	char        *format;
	gboolean     exists;
	gboolean     local;
	gboolean     has_metadata;
	gboolean     modified;
	gboolean     overwrite;

	float        jpeg_quality; /* valid range: [0.0 ... 1.0] */
};

struct _EomImageSaveInfoClass {
	GObjectClass parent_klass;
};

#define EOC_FILE_FORMAT_JPEG   "jpeg"

GType             eoc_image_save_info_get_type         (void) G_GNUC_CONST;

EomImageSaveInfo *eoc_image_save_info_new_from_image   (EomImage        *image);

EomImageSaveInfo *eoc_image_save_info_new_from_uri     (const char      *uri,
						       GdkPixbufFormat  *format);

EomImageSaveInfo *eoc_image_save_info_new_from_file    (GFile           *file,
						       GdkPixbufFormat  *format);

G_END_DECLS

#endif /* _EOC_IMAGE_SAVE_INFO_H_ */
