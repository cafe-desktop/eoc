#ifndef _EOC_PIXBUF_UTIL_H_
#define _EOC_PIXBUF_UTIL_H_

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <gio/gio.h>

G_GNUC_INTERNAL
GSList*          eoc_pixbuf_get_savable_formats (void);

G_GNUC_INTERNAL
GdkPixbufFormat* eoc_pixbuf_get_format_by_suffix (const char *suffix);

G_GNUC_INTERNAL
GdkPixbufFormat* eoc_pixbuf_get_format (GFile *file);

G_GNUC_INTERNAL
char*            eoc_pixbuf_get_common_suffix (GdkPixbufFormat *format);

#endif /* _EOC_PIXBUF_UTIL_H_ */

