#ifndef _EOC_SAVE_AS_DIALOG_HELPER_H_
#define _EOC_SAVE_AS_DIALOG_HELPER_H_

#include <ctk/ctk.h>
#include <gio/gio.h>
#include "eoc-uri-converter.h"


G_BEGIN_DECLS

G_GNUC_INTERNAL
CtkWidget*    eoc_save_as_dialog_new       (CtkWindow *main, GList *images, GFile *base_file);

G_GNUC_INTERNAL
EocURIConverter* eoc_save_as_dialog_get_converter (CtkWidget *dlg);


G_END_DECLS

#endif /* _EOC_SAVE_DIALOG_HELPER_H_ */
