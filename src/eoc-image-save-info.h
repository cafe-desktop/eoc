#ifndef _EOC_IMAGE_SAVE_INFO_H_
#define _EOC_IMAGE_SAVE_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
typedef struct _EocImage EocImage;
#endif

#define EOC_TYPE_IMAGE_SAVE_INFO            (eoc_image_save_info_get_type ())
#define EOC_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_IMAGE_SAVE_INFO, EocImageSaveInfo))
#define EOC_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_IMAGE_SAVE_INFO, EocImageSaveInfoClass))
#define EOC_IS_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_IMAGE_SAVE_INFO))
#define EOC_IS_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_IMAGE_SAVE_INFO))
#define EOC_IMAGE_SAVE_INFO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_IMAGE_SAVE_INFO, EocImageSaveInfoClass))

typedef struct _EocImageSaveInfo EocImageSaveInfo;
typedef struct _EocImageSaveInfoClass EocImageSaveInfoClass;

struct _EocImageSaveInfo {
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

struct _EocImageSaveInfoClass {
	GObjectClass parent_klass;
};

#define EOC_FILE_FORMAT_JPEG   "jpeg"

GType             eoc_image_save_info_get_type         (void) G_GNUC_CONST;

EocImageSaveInfo *eoc_image_save_info_new_from_image   (EocImage        *image);

EocImageSaveInfo *eoc_image_save_info_new_from_uri     (const char      *uri,
						       CdkPixbufFormat  *format);

EocImageSaveInfo *eoc_image_save_info_new_from_file    (GFile           *file,
						       CdkPixbufFormat  *format);

G_END_DECLS

#endif /* _EOC_IMAGE_SAVE_INFO_H_ */
