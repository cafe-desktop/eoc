#ifndef _EOC_URI_CONVERTER_H_
#define _EOC_URI_CONVERTER_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include "eoc-image.h"

G_BEGIN_DECLS

#define EOC_TYPE_URI_CONVERTER          (eoc_uri_converter_get_type ())
#define EOC_URI_CONVERTER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_URI_CONVERTER, EomURIConverter))
#define EOC_URI_CONVERTER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_URI_CONVERTER, EomURIConverterClass))
#define EOC_IS_URI_CONVERTER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_URI_CONVERTER))
#define EOC_IS_URI_CONVERTER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_URI_CONVERTER))
#define EOC_URI_CONVERTER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_URI_CONVERTER, EomURIConverterClass))

#ifndef __EOC_URI_CONVERTER_DECLR__
#define __EOC_URI_CONVERTER_DECLR__
typedef struct _EomURIConverter EomURIConverter;
#endif
typedef struct _EomURIConverterClass EomURIConverterClass;
typedef struct _EomURIConverterPrivate EomURIConverterPrivate;

typedef enum {
	EOC_UC_STRING,
	EOC_UC_FILENAME,
	EOC_UC_COUNTER,
	EOC_UC_COMMENT,
	EOC_UC_DATE,
	EOC_UC_TIME,
	EOC_UC_DAY,
	EOC_UC_MONTH,
	EOC_UC_YEAR,
	EOC_UC_HOUR,
	EOC_UC_MINUTE,
	EOC_UC_SECOND,
	EOC_UC_END
} EomUCType;

typedef struct {
	char     *description;
	char     *rep;
	gboolean req_exif;
} EomUCInfo;

typedef enum {
	EOC_UC_ERROR_INVALID_UNICODE,
	EOC_UC_ERROR_INVALID_CHARACTER,
	EOC_UC_ERROR_EQUAL_FILENAMES,
	EOC_UC_ERROR_UNKNOWN
} EomUCError;

#define EOC_UC_ERROR eoc_uc_error_quark ()


struct _EomURIConverter {
	GObject parent;

	EomURIConverterPrivate *priv;
};

struct _EomURIConverterClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType              eoc_uri_converter_get_type      (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GQuark             eoc_uc_error_quark              (void);

G_GNUC_INTERNAL
EomURIConverter*   eoc_uri_converter_new           (GFile *base_file,
                                                    GdkPixbufFormat *img_format,
						    const char *format_string);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_check         (EomURIConverter *converter,
                                                    GList *img_list,
                                                    GError **error);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_requires_exif (EomURIConverter *converter);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_do            (EomURIConverter *converter,
                                                    EomImage *image,
                                                    GFile **file,
                                                    GdkPixbufFormat **format,
                                                    GError **error);

G_GNUC_INTERNAL
char*              eoc_uri_converter_preview       (const char *format_str,
                                                    EomImage *img,
                                                    GdkPixbufFormat *format,
						    gulong counter,
						    guint n_images,
						    gboolean convert_spaces,
						    gunichar space_char);

/* for debugging purpose only */
G_GNUC_INTERNAL
void                eoc_uri_converter_print_list (EomURIConverter *conv);

G_END_DECLS

#endif /* _EOC_URI_CONVERTER_H_ */
