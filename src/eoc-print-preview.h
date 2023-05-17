/* Eye of MATE -- Print Preview Widget
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

#ifndef _EOC_PRINT_PREVIEW_H_
#define _EOC_PRINT_PREVIEW_H_

G_BEGIN_DECLS

typedef struct _EocPrintPreview EocPrintPreview;
typedef struct _EocPrintPreviewClass EocPrintPreviewClass;
typedef struct _EocPrintPreviewPrivate EocPrintPreviewPrivate;

#define EOC_TYPE_PRINT_PREVIEW            (eoc_print_preview_get_type ())
#define EOC_PRINT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_PRINT_PREVIEW, EocPrintPreview))
#define EOC_PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_PRINT_PREVIEW, EocPrintPreviewClass))
#define EOC_IS_PRINT_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_PRINT_PREVIEW))
#define EOC_IS_PRINT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_PRINT_PREVIEW))

struct _EocPrintPreview {
	GtkAspectFrame aspect_frame;

	EocPrintPreviewPrivate *priv;
};

struct _EocPrintPreviewClass {
	GtkAspectFrameClass parent_class;

};

G_GNUC_INTERNAL
GType        eoc_print_preview_get_type            (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget   *eoc_print_preview_new                 (void);

G_GNUC_INTERNAL
GtkWidget   *eoc_print_preview_new_with_pixbuf     (GdkPixbuf       *pixbuf);

G_GNUC_INTERNAL
void         eoc_print_preview_set_page_margins    (EocPrintPreview *preview,
						    gfloat          l_margin,
						    gfloat          r_margin,
						    gfloat          t_margin,
						    gfloat          b_margin);

G_GNUC_INTERNAL
void         eoc_print_preview_set_from_page_setup (EocPrintPreview *preview,
						    GtkPageSetup    *setup);

G_GNUC_INTERNAL
void         eoc_print_preview_get_image_position  (EocPrintPreview *preview,
						    gdouble         *x,
						    gdouble         *y);

G_GNUC_INTERNAL
void         eoc_print_preview_set_image_position  (EocPrintPreview *preview,
						    gdouble          x,
						    gdouble          y);

G_GNUC_INTERNAL
void         eoc_print_preview_set_scale           (EocPrintPreview *preview,
						    gfloat           scale);

G_END_DECLS

#endif /* _EOC_PRINT_PREVIEW_H_ */
