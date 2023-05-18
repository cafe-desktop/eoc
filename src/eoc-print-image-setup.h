/* Eye Of CAFE -- Print Dialog Custom Widget
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#include "eoc-image.h"

#ifndef EOC_PRINT_IMAGE_SETUP_H
#define EOC_PRINT_IMAGE_SETUP_H

G_BEGIN_DECLS

typedef struct _EocPrintImageSetup         EocPrintImageSetup;
typedef struct _EocPrintImageSetupClass    EocPrintImageSetupClass;
typedef struct _EocPrintImageSetupPrivate   EocPrintImageSetupPrivate;

#define EOC_TYPE_PRINT_IMAGE_SETUP            (eoc_print_image_setup_get_type ())
#define EOC_PRINT_IMAGE_SETUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_PRINT_IMAGE_SETUP, EocPrintImageSetup))
#define EOC_PRINT_IMAGE_SETUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_PRINT_IMAGE_SETUP, EocPrintImageSetupClass))
#define EOC_IS_PRINT_IMAGE_SETUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_PRINT_IMAGE_SETUP))
#define EOC_IS_PRINT_IMAGE_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_PRINT_IMAGE_SETUP))
#define EOC_PRINT_IMAGE_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_PRINT_IMAGE_SETUP, EocPrintImageSetupClass))

struct _EocPrintImageSetup {
	GtkGrid parent_instance;

	EocPrintImageSetupPrivate *priv;
};

struct _EocPrintImageSetupClass {
	GtkGridClass parent_class;
};

G_GNUC_INTERNAL
GType		  eoc_print_image_setup_get_type    (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget        *eoc_print_image_setup_new         (EocImage     *image,
						     GtkPageSetup *page_setup);

G_GNUC_INTERNAL
void              eoc_print_image_setup_get_options (EocPrintImageSetup *setup,
						     gdouble            *left,
						     gdouble            *top,
						     gdouble            *scale,
						     GtkUnit            *unit);
void              eoc_print_image_setup_update      (GtkPrintOperation *operation,
						     GtkWidget         *custom_widget,
						     GtkPageSetup      *page_setup,
						     GtkPrintSettings  *print_settings,
						     gpointer           user_data);

G_END_DECLS

#endif /* EOC_PRINT_IMAGE_SETUP_H */
