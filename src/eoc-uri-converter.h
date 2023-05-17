#ifndef _EOC_URI_CONVERTER_H_
#define _EOC_URI_CONVERTER_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include "eoc-image.h"

G_BEGIN_DECLS

#define EOC_TYPE_URI_CONVERTER          (eoc_uri_converter_get_type ())
#define EOC_URI_CONVERTER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_URI_CONVERTER, EocURIConverter))
#define EOC_URI_CONVERTER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_URI_CONVERTER, EocURIConverterClass))
#define EOC_IS_URI_CONVERTER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_URI_CONVERTER))
#define EOC_IS_URI_CONVERTER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_URI_CONVERTER))
#define EOC_URI_CONVERTER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_URI_CONVERTER, EocURIConverterClass))

#ifndef __EOC_URI_CONVERTER_DECLR__
#define __EOC_URI_CONVERTER_DECLR__
typedef struct _EocURIConverter EocURIConverter;
#endif
typedef struct _EocURIConverterClass EocURIConverterClass;
typedef struct _EocURIConverterPrivate EocURIConverterPrivate;

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
} EocUCType;

typedef struct {
	char     *description;
	char     *rep;
	gboolean req_exif;
} EocUCInfo;

typedef enum {
	EOC_UC_ERROR_INVALID_UNICODE,
	EOC_UC_ERROR_INVALID_CHARACTER,
	EOC_UC_ERROR_EQUAL_FILENAMES,
	EOC_UC_ERROR_UNKNOWN
} EocUCError;

#define EOC_UC_ERROR eoc_uc_error_quark ()


struct _EocURIConverter {
	GObject parent;

	EocURIConverterPrivate *priv;
};

struct _EocURIConverterClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType              eoc_uri_converter_get_type      (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GQuark             eoc_uc_error_quark              (void);

G_GNUC_INTERNAL
EocURIConverter*   eoc_uri_converter_new           (GFile *base_file,
                                                    GdkPixbufFormat *img_format,
						    const char *format_string);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_check         (EocURIConverter *converter,
                                                    GList *img_list,
                                                    GError **error);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_requires_exif (EocURIConverter *converter);

G_GNUC_INTERNAL
gboolean           eoc_uri_converter_do            (EocURIConverter *converter,
                                                    EocImage *image,
                                                    GFile **file,
                                                    GdkPixbufFormat **format,
                                                    GError **error);

G_GNUC_INTERNAL
char*              eoc_uri_converter_preview       (const char *format_str,
                                                    EocImage *img,
                                                    GdkPixbufFormat *format,
						    gulong counter,
						    guint n_images,
						    gboolean convert_spaces,
						    gunichar space_char);

/* for debugging purpose only */
G_GNUC_INTERNAL
void                eoc_uri_converter_print_list (EocURIConverter *conv);

G_END_DECLS

#endif /* _EOC_URI_CONVERTER_H_ */
