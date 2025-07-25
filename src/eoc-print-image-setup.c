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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>

#include <glib/gi18n.h>
#include <glib/gprintf.h>

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include "eoc-print-image-setup.h"
#include "eoc-print-preview.h"

/**
 * SECTION:
 * @Title: Printing Custom Widget
 * @Short_Description: a custom widget to setup image prints.
 * @Stability_Level: Internal
 * @See_Also: #EocPrintPreview
 *
 * This widget is to be used as the custom widget in a #CtkPrintUnixDialog in
 * EOC. Through it, you can set the position and scaling of a image
 * interactively.
 */

struct _EocPrintImageSetupPrivate {
	CtkWidget *left;
	CtkWidget *right;
	CtkWidget *top;
	CtkWidget *bottom;

	CtkWidget *center;

	CtkWidget *width;
	CtkWidget *height;

	CtkWidget *scaling;
	CtkWidget *unit;

	CtkUnit current_unit;

	EocImage *image;
	CtkPageSetup *page_setup;

	CtkWidget *preview;
};

enum {
	PROP_0,
	PROP_IMAGE,
	PROP_PAGE_SETUP
};

enum {
	CENTER_NONE,
	CENTER_HORIZONTAL,
	CENTER_VERTICAL,
	CENTER_BOTH
};

enum {
	CHANGE_HORIZ,
	CHANGE_VERT
};

enum {
	UNIT_INCH,
	UNIT_MM
};

#define FACTOR_INCH_TO_MM 25.4
#define FACTOR_INCH_TO_PIXEL 72.
#define FACTOR_MM_TO_INCH 0.03937007874015748
#define FACTOR_MM_TO_PIXEL 2.834645669

static void eoc_print_image_setup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void eoc_print_image_setup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void on_left_value_changed   (CtkSpinButton *spinbutton, gpointer user_data);
static void on_right_value_changed  (CtkSpinButton *spinbutton, gpointer user_data);
static void on_top_value_changed    (CtkSpinButton *spinbutton, gpointer user_data);
static void on_bottom_value_changed (CtkSpinButton *spinbutton, gpointer user_data);

static void on_width_value_changed  (CtkSpinButton *spinbutton, gpointer user_data);
static void on_height_value_changed (CtkSpinButton *spinbutton, gpointer user_data);

G_DEFINE_TYPE_WITH_PRIVATE (EocPrintImageSetup, eoc_print_image_setup, CTK_TYPE_GRID);

static void
block_handlers (EocPrintImageSetup *setup)
{
	EocPrintImageSetupPrivate *priv = setup->priv;

	g_signal_handlers_block_by_func (priv->left, on_left_value_changed, setup);
	g_signal_handlers_block_by_func (priv->right, on_right_value_changed, setup);
	g_signal_handlers_block_by_func (priv->width, on_width_value_changed, setup);
	g_signal_handlers_block_by_func (priv->top, on_top_value_changed, setup);
	g_signal_handlers_block_by_func (priv->bottom, on_bottom_value_changed, setup);
	g_signal_handlers_block_by_func (priv->height, on_height_value_changed, setup);
}

static void
unblock_handlers (EocPrintImageSetup *setup)
{
	EocPrintImageSetupPrivate *priv = setup->priv;

	g_signal_handlers_unblock_by_func (priv->left, on_left_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->right, on_right_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->width, on_width_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->top, on_top_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->bottom, on_bottom_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->height, on_height_value_changed, setup);
}

static gdouble
get_scale_to_px_factor (EocPrintImageSetup *setup)
{
	gdouble factor = 0.;

	switch (setup->priv->current_unit) {
	case CTK_UNIT_MM:
		factor = FACTOR_MM_TO_PIXEL;
		break;
	case CTK_UNIT_INCH:
		factor = FACTOR_INCH_TO_PIXEL;
		break;
	default:
		g_assert_not_reached ();
	}

	return factor;
}

static gdouble
get_max_percentage (EocPrintImageSetup *setup)
{
	EocPrintImageSetupPrivate *priv = setup->priv;
	gdouble p_width, p_height;
	gdouble width, height;
	gint pix_width, pix_height;
	gdouble perc;

	p_width = ctk_page_setup_get_page_width (priv->page_setup, CTK_UNIT_INCH);
	p_height = ctk_page_setup_get_page_height (priv->page_setup, CTK_UNIT_INCH);

	eoc_image_get_size (priv->image, &pix_width, &pix_height);

	width  = (gdouble)pix_width/FACTOR_INCH_TO_PIXEL;
	height = (gdouble)pix_height/FACTOR_INCH_TO_PIXEL;

	if (p_width > width && p_height > height) {
		perc = 1.;
	} else {
		perc = MIN (p_width/width, p_height/height);
	}

	return perc;
}

static void
center (gdouble page_width,
	gdouble width,
	CtkSpinButton *s_left,
	CtkSpinButton *s_right)
{
	gdouble left, right;

	left = (page_width - width)/2;
	right = page_width - left - width;
	ctk_spin_button_set_value (s_left, left);
	ctk_spin_button_set_value (s_right, right);
}

static void
on_center_changed (CtkComboBox *combobox,
		   gpointer user_data)
{
	EocPrintImageSetup *setup;
	EocPrintImageSetupPrivate *priv;
	gint active;

	setup = EOC_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	active = ctk_combo_box_get_active (combobox);

	switch (active) {
	case CENTER_HORIZONTAL:
		center (ctk_page_setup_get_page_width (priv->page_setup,
						       priv->current_unit),
			ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->width)),
			CTK_SPIN_BUTTON (priv->left),
			CTK_SPIN_BUTTON (priv->right));
		break;
	case CENTER_VERTICAL:
		center (ctk_page_setup_get_page_height (priv->page_setup,
							priv->current_unit),
			ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->height)),
			CTK_SPIN_BUTTON (priv->top),
			CTK_SPIN_BUTTON (priv->bottom));
		break;
	case CENTER_BOTH:
		center (ctk_page_setup_get_page_width (priv->page_setup,
						       priv->current_unit),
			ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->width)),
			CTK_SPIN_BUTTON (priv->left),
			CTK_SPIN_BUTTON (priv->right));
		center (ctk_page_setup_get_page_height (priv->page_setup,
							priv->current_unit),
			ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->height)),
			CTK_SPIN_BUTTON (priv->top),
			CTK_SPIN_BUTTON (priv->bottom));
		break;
	case CENTER_NONE:
	default:
		break;
	}

	ctk_combo_box_set_active (combobox, active);
}

static void
update_image_pos_ranges (EocPrintImageSetup *setup,
			 gdouble page_width,
			 gdouble page_height,
			 gdouble width,
			 gdouble height)
{
	EocPrintImageSetupPrivate *priv;

	priv = setup->priv;

	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->left),
				   0, page_width - width);
	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->right),
				   0, page_width - width);
	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->top),
				   0, page_height - height);
	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->bottom),
				   0, page_height - height);
}

static gboolean
on_scale_changed (CtkRange     *range,
		  gpointer      user_data)
{
	gdouble scale;
	gdouble width, height;
	gint pix_width, pix_height;
	gdouble left, right, top, bottom;
	gdouble page_width, page_height;
	EocPrintImageSetupPrivate *priv;
	EocPrintImageSetup *setup;
	gdouble factor;
	EocImage *image;

	setup = EOC_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	ctk_combo_box_set_active (CTK_COMBO_BOX (priv->center), CENTER_NONE);

	image = priv->image;
	eoc_image_get_size (image, &pix_width, &pix_height);

	factor = get_scale_to_px_factor (setup);

	width = (gdouble)pix_width/factor;
	height = (gdouble)pix_height/factor;

	left = ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->left));
	top = ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->top));

	scale = CLAMP (0.01*ctk_range_get_value (range), 0, get_max_percentage (setup));

 	eoc_print_preview_set_scale (EOC_PRINT_PREVIEW (priv->preview), scale);

	width  *= scale;
	height *= scale;

	page_width = ctk_page_setup_get_page_width (priv->page_setup, priv->current_unit);
	page_height = ctk_page_setup_get_page_height (priv->page_setup, priv->current_unit);

	update_image_pos_ranges (setup, page_width, page_height, width, height);

	right = page_width - left - width;
	bottom = page_height - top - height;

	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->width), width);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->height), height);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->right), right);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->bottom), bottom);

	return FALSE;
}

static gchar *
on_scale_format_value (CtkScale *scale G_GNUC_UNUSED,
		       gdouble   value)
{
	return g_strdup_printf ("%i%%", (gint)value);
}

static void
position_values_changed (EocPrintImageSetup *setup,
			 CtkWidget *w_changed,
			 CtkWidget *w_to_update,
			 CtkWidget *w_size,
			 gdouble total_size,
			 gint change)
{
	EocPrintImageSetupPrivate *priv;
	gdouble changed, to_update, size;
	gdouble pos;

	priv = setup->priv;
	size = ctk_spin_button_get_value (CTK_SPIN_BUTTON (w_size));
	changed = ctk_spin_button_get_value (CTK_SPIN_BUTTON (w_changed));

	to_update = total_size - changed - size;
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (w_to_update), to_update);
	ctk_combo_box_set_active (CTK_COMBO_BOX (priv->center), CENTER_NONE);

	switch (change) {
	case CHANGE_HORIZ:
		pos = ctk_spin_button_get_value (CTK_SPIN_BUTTON (setup->priv->left));
		if (setup->priv->current_unit == CTK_UNIT_MM) {
			pos *= FACTOR_MM_TO_INCH;
		}
 		eoc_print_preview_set_image_position (EOC_PRINT_PREVIEW (priv->preview), pos, -1);
		break;
	case CHANGE_VERT:
		pos = ctk_spin_button_get_value (CTK_SPIN_BUTTON (setup->priv->top));
		if (setup->priv->current_unit == CTK_UNIT_MM) {
			pos *= FACTOR_MM_TO_INCH;
		}
		eoc_print_preview_set_image_position (EOC_PRINT_PREVIEW (priv->preview), -1, pos);
		break;
	}
}

static void
on_left_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
		       gpointer       user_data)
{
	EocPrintImageSetup *setup;
	EocPrintImageSetupPrivate *priv;

	setup = EOC_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	position_values_changed (setup,
				 priv->left, priv->right, priv->width,
				 ctk_page_setup_get_page_width (priv->page_setup,
								priv->current_unit),
				 CHANGE_HORIZ);
}

static void
on_right_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
			gpointer       user_data)
{
	EocPrintImageSetupPrivate *priv;

	priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOC_PRINT_IMAGE_SETUP (user_data),
				 priv->right, priv->left, priv->width,
				 ctk_page_setup_get_page_width (priv->page_setup,
								priv->current_unit),
				 CHANGE_HORIZ);
}

static void
on_top_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
		      gpointer       user_data)
{
	EocPrintImageSetupPrivate *priv;

	priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOC_PRINT_IMAGE_SETUP (user_data),
				 priv->top, priv->bottom, priv->height,
				 ctk_page_setup_get_page_height (priv->page_setup,
								 priv->current_unit),
				 CHANGE_VERT);
}

static void
on_bottom_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
			 gpointer       user_data)
{
	EocPrintImageSetupPrivate *priv;

	priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOC_PRINT_IMAGE_SETUP (user_data),
				 priv->bottom, priv->top, priv->height,
				 ctk_page_setup_get_page_height (priv->page_setup,
								 priv->current_unit),
				 CHANGE_VERT);
}

static void
size_changed (EocPrintImageSetup *setup,
	      CtkWidget *w_size_x,
	      CtkWidget *w_size_y,
	      CtkWidget *w_margin_x_1,
	      CtkWidget *w_margin_x_2,
	      CtkWidget *w_margin_y_1,
	      CtkWidget *w_margin_y_2,
	      gdouble page_size_x,
	      gdouble page_size_y,
	      gint change)
{
	EocPrintImageSetupPrivate *priv;
	gdouble margin_x_1, margin_x_2;
	gdouble margin_y_1, margin_y_2;
	gdouble orig_size_x = -1, orig_size_y = -1, scale;
	gdouble size_x, size_y;
	gint pix_width, pix_height;
	gdouble factor;

	priv = setup->priv;

	size_x = ctk_spin_button_get_value (CTK_SPIN_BUTTON (w_size_x));
	margin_x_1 = ctk_spin_button_get_value (CTK_SPIN_BUTTON (w_margin_x_1));
	margin_y_1 = ctk_spin_button_get_value (CTK_SPIN_BUTTON (w_margin_y_1));

	eoc_image_get_size (priv->image, &pix_width, &pix_height);

	factor = get_scale_to_px_factor (setup);

	switch (change) {
	case CHANGE_HORIZ:
		orig_size_x = (gdouble) pix_width / factor;
		orig_size_y = (gdouble) pix_height / factor;
		break;
	case CHANGE_VERT:
		orig_size_y = (gdouble) pix_width / factor;
		orig_size_x = (gdouble) pix_height / factor;
		break;
	}

	scale = CLAMP (size_x / orig_size_x, 0, 1);

	size_y = scale * orig_size_y;

	margin_x_2 = page_size_x - margin_x_1 - size_x;
	margin_y_2 = page_size_y - margin_y_1 - size_y;

 	eoc_print_preview_set_scale (EOC_PRINT_PREVIEW (priv->preview), scale);

	switch (change) {
	case CHANGE_HORIZ:
		update_image_pos_ranges (setup, page_size_x, page_size_y, size_x, size_y);
		break;
	case CHANGE_VERT:
		update_image_pos_ranges (setup, page_size_y, page_size_x, size_y, size_x);
		break;
	}

	ctk_range_set_value (CTK_RANGE (priv->scaling), 100*scale);

	ctk_spin_button_set_value (CTK_SPIN_BUTTON (w_margin_x_2), margin_x_2);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (w_size_y), size_y);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (w_margin_y_2), margin_y_2);

	ctk_combo_box_set_active (CTK_COMBO_BOX (priv->center), CENTER_NONE);
}

static void
on_width_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
			gpointer       user_data)
{
	EocPrintImageSetupPrivate *priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;

	size_changed (EOC_PRINT_IMAGE_SETUP (user_data),
		      priv->width, priv->height,
		      priv->left, priv->right,
		      priv->top, priv->bottom,
		      ctk_page_setup_get_page_width (priv->page_setup,
						     priv->current_unit),
		      ctk_page_setup_get_page_height (priv->page_setup,
						      priv->current_unit),
		      CHANGE_HORIZ);
}

static void
on_height_value_changed (CtkSpinButton *spinbutton G_GNUC_UNUSED,
			 gpointer       user_data)
{
	EocPrintImageSetupPrivate *priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;

	size_changed (EOC_PRINT_IMAGE_SETUP (user_data),
		      priv->height, priv->width,
		      priv->top, priv->bottom,
		      priv->left, priv->right,
		      ctk_page_setup_get_page_height (priv->page_setup,
						     priv->current_unit),
		      ctk_page_setup_get_page_width (priv->page_setup,
						     priv->current_unit),
		      CHANGE_VERT);
}

static void
change_unit (CtkSpinButton *spinbutton,
	     gdouble factor,
	     gint digits,
	     gdouble step,
	     gdouble page)
{
	gdouble value;
	gdouble range;

	ctk_spin_button_get_range (spinbutton, NULL, &range);
	range *= factor;

	value = ctk_spin_button_get_value (spinbutton);
	value *= factor;

	ctk_spin_button_set_range (spinbutton, 0, range);
	ctk_spin_button_set_value (spinbutton, value);
	ctk_spin_button_set_digits (spinbutton, digits);
	ctk_spin_button_set_increments  (spinbutton, step, page);
}

static void
set_scale_unit (EocPrintImageSetup *setup,
		CtkUnit unit)
{
	EocPrintImageSetupPrivate *priv = setup->priv;
	gdouble factor;
	gdouble step, page;
	gint digits;

	if (G_UNLIKELY (priv->current_unit == unit))
		return;

	switch (unit) {
	case CTK_UNIT_MM:
		factor = FACTOR_INCH_TO_MM;
		digits = 0;
		step = 1;
		page = 10;
		break;
	case CTK_UNIT_INCH:
		factor = FACTOR_MM_TO_INCH;
		digits = 2;
		step = 0.01;
		page = 0.1;
		break;
	default:
		g_assert_not_reached ();
	}

 	block_handlers (setup);

	change_unit (CTK_SPIN_BUTTON (priv->width), factor, digits, step, page);
	change_unit (CTK_SPIN_BUTTON (priv->height), factor, digits, step, page);
	change_unit (CTK_SPIN_BUTTON (priv->left), factor, digits, step, page);
	change_unit (CTK_SPIN_BUTTON (priv->right), factor, digits, step, page);
	change_unit (CTK_SPIN_BUTTON (priv->top), factor, digits, step, page);
	change_unit (CTK_SPIN_BUTTON (priv->bottom), factor, digits, step, page);

 	unblock_handlers (setup);

	priv->current_unit = unit;
}

static void
on_unit_changed (CtkComboBox *combobox,
		 gpointer user_data)
{
	CtkUnit unit = CTK_UNIT_INCH;

	switch (ctk_combo_box_get_active (combobox)) {
	case UNIT_INCH:
		unit = CTK_UNIT_INCH;
		break;
	case UNIT_MM:
		unit = CTK_UNIT_MM;
		break;
	default:
		g_assert_not_reached ();
	}

	set_scale_unit (EOC_PRINT_IMAGE_SETUP (user_data), unit);
}

static void
on_preview_image_moved (EocPrintPreview *preview,
			gpointer user_data)
{
	EocPrintImageSetupPrivate *priv = EOC_PRINT_IMAGE_SETUP (user_data)->priv;
	gdouble x, y;

	eoc_print_preview_get_image_position (preview, &x, &y);

	if (priv->current_unit == CTK_UNIT_MM) {
		x *= FACTOR_INCH_TO_MM;
		y *= FACTOR_INCH_TO_MM;
	}

	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->left), x);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->top), y);
}

/* Function taken from ctkprintunixdialog.c */
static CtkWidget *
wrap_in_frame (const gchar *label,
               CtkWidget   *child)
{
	CtkWidget *frame, *label_widget;
	gchar *bold_text;

	label_widget = ctk_label_new ("");
	ctk_widget_set_halign (label_widget, CTK_ALIGN_START);
	ctk_widget_show (label_widget);

	bold_text = g_markup_printf_escaped ("<b>%s</b>", label);
	ctk_label_set_markup (CTK_LABEL (label_widget), bold_text);
	g_free (bold_text);

	frame = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (frame), label_widget, FALSE, FALSE, 0);
	ctk_box_pack_start (CTK_BOX (frame), child, FALSE, FALSE, 0);

	ctk_widget_set_margin_start (child, 12);

	ctk_widget_show (frame);

	return frame;
}

static CtkWidget *
grid_attach_spin_button_with_label (CtkWidget *grid,
				     const gchar* text_label,
				     gint left, gint top)
{
	CtkWidget *label, *spin_button;

	label = ctk_label_new_with_mnemonic (text_label);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	spin_button = ctk_spin_button_new_with_range (0, 100, 0.01);
	ctk_spin_button_set_digits (CTK_SPIN_BUTTON (spin_button), 2);
	ctk_entry_set_width_chars (CTK_ENTRY (spin_button), 6);
	ctk_grid_attach (CTK_GRID (grid), label, left, top, 1, 1);
	ctk_grid_attach_next_to (CTK_GRID (grid), spin_button, label,
							 CTK_POS_RIGHT, 1, 1);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), spin_button);

	return spin_button;
}

static void
eoc_print_image_setup_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	EocPrintImageSetup *setup = EOC_PRINT_IMAGE_SETUP (object);
	EocPrintImageSetupPrivate *priv = setup->priv;
	GdkPixbuf *pixbuf;

	switch (prop_id) {
	case PROP_IMAGE:
		if (priv->image) {
			g_object_unref (priv->image);
		}
		priv->image = EOC_IMAGE (g_value_dup_object (value));
		if (EOC_IS_IMAGE (priv->image)) {
			pixbuf = eoc_image_get_pixbuf (priv->image);
			g_object_set (priv->preview, "image",
				      pixbuf, NULL);
			g_object_unref (pixbuf);
		}
		break;
	case PROP_PAGE_SETUP:
		priv->page_setup = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eoc_print_image_setup_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	EocPrintImageSetup *setup = EOC_PRINT_IMAGE_SETUP (object);
	EocPrintImageSetupPrivate *priv = setup->priv;

	switch (prop_id) {
	case PROP_IMAGE:
		g_value_set_object (value, priv->image);
		break;
	case PROP_PAGE_SETUP:
		g_value_set_object (value, priv->page_setup);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
set_initial_values (EocPrintImageSetup *setup)
{
	EocPrintImageSetupPrivate *priv;
	CtkPageSetup *page_setup;
	EocImage *image;
	gdouble page_width, page_height;
	gint pix_width, pix_height;
	gdouble factor;
	gdouble width, height;
	gdouble max_perc;

	priv = setup->priv;
	page_setup = priv->page_setup;
	image = priv->image;

	factor = get_scale_to_px_factor (setup);

	eoc_image_get_size (image, &pix_width, &pix_height);
	width = (gdouble)pix_width/factor;
	height = (gdouble)pix_height/factor;

	max_perc = get_max_percentage (setup);

	width *= max_perc;
	height *= max_perc;

	ctk_range_set_range (CTK_RANGE (priv->scaling), 1, 100*max_perc);
	ctk_range_set_increments (CTK_RANGE (priv->scaling), max_perc, 10*max_perc);
	ctk_range_set_value (CTK_RANGE (priv->scaling), 100*max_perc);

	eoc_print_preview_set_scale (EOC_PRINT_PREVIEW (priv->preview), max_perc);
	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->width), 0, width);
	ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->height), 0, height);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->width), width);
	ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->height), height);

	ctk_combo_box_set_active (CTK_COMBO_BOX (priv->center),
				  CENTER_BOTH);

	center (ctk_page_setup_get_page_width (priv->page_setup, priv->current_unit),
		ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->width)),
		CTK_SPIN_BUTTON (priv->left), CTK_SPIN_BUTTON (priv->right));
	center (ctk_page_setup_get_page_height (priv->page_setup, priv->current_unit),
		ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->height)),
		CTK_SPIN_BUTTON (priv->top), CTK_SPIN_BUTTON (priv->bottom));

	page_width = ctk_page_setup_get_page_width (page_setup, priv->current_unit);
	page_height = ctk_page_setup_get_page_height (page_setup, priv->current_unit);

	update_image_pos_ranges (setup, page_width, page_height, width, height);


}

static void
connect_signals (EocPrintImageSetup *setup)
{
	EocPrintImageSetupPrivate *priv;

	priv = setup->priv;

	g_signal_connect (G_OBJECT (priv->left), "value-changed",
			  G_CALLBACK (on_left_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->right), "value-changed",
			  G_CALLBACK (on_right_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->top), "value-changed",
			  G_CALLBACK (on_top_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->bottom), "value-changed",
			  G_CALLBACK (on_bottom_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->width), "value-changed",
			  G_CALLBACK (on_width_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->height), "value-changed",
			  G_CALLBACK (on_height_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->scaling), "value-changed",
			  G_CALLBACK (on_scale_changed), setup);
	g_signal_connect (G_OBJECT (priv->scaling), "format-value",
			  G_CALLBACK (on_scale_format_value), NULL);
	g_signal_connect (G_OBJECT (priv->preview), "image-moved",
			  G_CALLBACK (on_preview_image_moved), setup);
}

static void
eoc_print_image_setup_class_init (EocPrintImageSetupClass *class)
{
	GObjectClass *object_class = (GObjectClass *)class;

	object_class->set_property = eoc_print_image_setup_set_property;
	object_class->get_property = eoc_print_image_setup_get_property;

	g_object_class_install_property (object_class, PROP_IMAGE,
					 g_param_spec_object ("image",
							      _("Image"),
							      _("The image whose printing properties will be set up"),
							      EOC_TYPE_IMAGE,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_PAGE_SETUP,
					 g_param_spec_object ("page-setup",
							      _("Page Setup"),
							      _("The information for the page where the image will be printed"),
							      CTK_TYPE_PAGE_SETUP,
							      G_PARAM_READWRITE));
}

static void
eoc_print_image_setup_init (EocPrintImageSetup *setup)
{
	CtkWidget *frame;
	CtkWidget *grid;
	CtkWidget *label;
	CtkWidget *hscale;
	CtkWidget *combobox;
	EocPrintImageSetupPrivate *priv;

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
	gchar *locale_scale = NULL;
#endif

	priv = setup->priv = eoc_print_image_setup_get_instance_private (setup);

	priv->image = NULL;

	grid = ctk_grid_new ();
	ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
	ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
	frame = wrap_in_frame (_("Position"), grid);
	ctk_grid_attach (CTK_GRID (setup), frame, 0, 0, 1, 1);

	priv->left = grid_attach_spin_button_with_label (grid,
													 _("_Left:"), 0, 0);
	priv->right = grid_attach_spin_button_with_label (grid,
													  _("_Right:"), 0, 1);
	priv->top = grid_attach_spin_button_with_label (grid, _("_Top:"), 2, 0);
	priv->bottom = grid_attach_spin_button_with_label (grid, _("_Bottom:"),
													   2, 1);

	label = ctk_label_new_with_mnemonic (_("C_enter:"));
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);

	combobox = ctk_combo_box_text_new ();
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox),
				   CENTER_NONE, _("None"));
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox),
				   CENTER_HORIZONTAL, _("Horizontal"));
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox),
				   CENTER_VERTICAL, _("Vertical"));
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox),
				   CENTER_BOTH, _("Both"));
	ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), CENTER_NONE);
	/* Attach combobox below right margin spinbutton and span until end */
	ctk_grid_attach_next_to (CTK_GRID (grid), combobox, priv->right,
							 CTK_POS_BOTTOM, 3, 1);
	/* Attach the label to the left of the combobox */
	ctk_grid_attach_next_to (CTK_GRID (grid), label, combobox, CTK_POS_LEFT,
							 1, 1);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), combobox);
	priv->center = combobox;
	g_signal_connect (G_OBJECT (combobox), "changed",
			  G_CALLBACK (on_center_changed), setup);

	grid = ctk_grid_new ();
	ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
	ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
	frame = wrap_in_frame (_("Size"), grid);
	ctk_grid_attach (CTK_GRID (setup), frame, 0, 1, 1, 1);

	priv->width = grid_attach_spin_button_with_label (grid, _("_Width:"),
							   0, 0);
	priv->height = grid_attach_spin_button_with_label (grid, _("_Height:"),
							    2, 0);

	label = ctk_label_new_with_mnemonic (_("_Scaling:"));
	hscale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 1, 100, 1);
	ctk_scale_set_value_pos (CTK_SCALE (hscale), CTK_POS_RIGHT);
	ctk_range_set_value (CTK_RANGE (hscale), 100);
	ctk_grid_attach_next_to (CTK_GRID (grid), hscale, priv->width,
							 CTK_POS_BOTTOM, 3, 1);
	ctk_grid_attach_next_to (CTK_GRID (grid), label, hscale, CTK_POS_LEFT,
							 1, 1);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), hscale);
	priv->scaling = hscale;

	label = ctk_label_new_with_mnemonic (_("_Unit:"));
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);

	combobox = ctk_combo_box_text_new ();
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox), UNIT_MM,
				   _("Millimeters"));
	ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combobox), UNIT_INCH,
				   _("Inches"));

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
	locale_scale = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
	if (locale_scale && locale_scale[0] == 2) {
		ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), UNIT_INCH);
		set_scale_unit (setup, CTK_UNIT_INCH);
	} else
#endif
	{
		ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), UNIT_MM);
		set_scale_unit (setup, CTK_UNIT_MM);
	}

	ctk_grid_attach_next_to (CTK_GRID (grid), combobox, hscale,
							 CTK_POS_BOTTOM, 3, 1);
	ctk_grid_attach_next_to (CTK_GRID (grid), label, combobox, CTK_POS_LEFT,
							 1, 1);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), combobox);
	priv->unit = combobox;
	g_signal_connect (G_OBJECT (combobox), "changed",
			  G_CALLBACK (on_unit_changed), setup);

	priv->preview = eoc_print_preview_new ();

	/* FIXME: This shouldn't be set by hand */
	ctk_widget_set_size_request (priv->preview, 250, 250);

	frame = wrap_in_frame (_("Preview"), priv->preview);
	/* The preview widget needs to span the whole grid height */
	ctk_grid_attach (CTK_GRID (setup), frame, 1, 0, 1, 2);

	ctk_widget_show_all (CTK_WIDGET (setup));
}


/**
 * eoc_print_image_setup_new:
 * @image: the #EocImage to print
 * @page_setup: a #CtkPageSetup specifying the page where
 * the image will be print
 *
 * Creates a new #EocPrintImageSetup widget, to be used as a custom
 * widget in a #CtkPrintUnixDialog. This widgets allows to set
 * the image position and scale in a page.
 *
 * Returns: a new #EocPrintImageSetup
 **/
CtkWidget *
eoc_print_image_setup_new (EocImage *image, CtkPageSetup *page_setup)
{
	CtkWidget *setup;
	CtkWidget *preview;

	setup = g_object_new (EOC_TYPE_PRINT_IMAGE_SETUP,
			     "orientation", CTK_ORIENTATION_VERTICAL,
			     "row-spacing", 18,
			     "column-spacing", 18,
			     "border-width", 12,
			     "image", image,
			     "page-setup", page_setup,
			     NULL);

	set_initial_values (EOC_PRINT_IMAGE_SETUP (setup));

	preview = EOC_PRINT_IMAGE_SETUP (setup)->priv->preview;
	eoc_print_preview_set_from_page_setup (EOC_PRINT_PREVIEW (preview),
					       page_setup);

	connect_signals (EOC_PRINT_IMAGE_SETUP (setup));

	return setup;
}

/**
 * eoc_print_image_setup_get_options:
 * @setup: a #EocPrintImageSetup
 * @left: a pointer where to store the image's left position
 * @top: a pointer where to store the image's top position
 * @scale: a pointer where to store the image's scale
 * @unit: a pointer where to store the #CtkUnit used by the @left and @top values.
 *
 * Gets the options set by the #EocPrintImageSetup.
 **/
void
eoc_print_image_setup_get_options (EocPrintImageSetup *setup,
				   gdouble *left,
				   gdouble *top,
				   gdouble *scale,
				   CtkUnit *unit)
{
	EocPrintImageSetupPrivate *priv;

	g_return_if_fail (EOC_IS_PRINT_IMAGE_SETUP (setup));

	priv = setup->priv;

	*left = ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->left));
	*top = ctk_spin_button_get_value (CTK_SPIN_BUTTON (priv->top));
	*scale = ctk_range_get_value (CTK_RANGE (priv->scaling));
	*unit = priv->current_unit;
}

void
eoc_print_image_setup_update (CtkPrintOperation *operation G_GNUC_UNUSED,
			      CtkWidget         *custom_widget,
			      CtkPageSetup      *page_setup,
			      CtkPrintSettings  *print_settings G_GNUC_UNUSED,
			      gpointer           user_data G_GNUC_UNUSED)
{
	CtkWidget *preview;
	gdouble    pos_x;
	gdouble    pos_y;
	EocPrintImageSetup *setup;

	setup = EOC_PRINT_IMAGE_SETUP (custom_widget);

	setup->priv->page_setup = ctk_page_setup_copy (page_setup);

	set_initial_values (EOC_PRINT_IMAGE_SETUP (setup));

	preview = EOC_PRINT_IMAGE_SETUP (setup)->priv->preview;
	eoc_print_preview_set_from_page_setup (EOC_PRINT_PREVIEW (preview),
					       setup->priv->page_setup);

	pos_x = ctk_spin_button_get_value (CTK_SPIN_BUTTON (setup->priv->left));
	pos_y = ctk_spin_button_get_value (CTK_SPIN_BUTTON (setup->priv->top));
	if (setup->priv->current_unit == CTK_UNIT_MM) {
		pos_x *= FACTOR_MM_TO_INCH;
		pos_y *= FACTOR_MM_TO_INCH;
	}
	eoc_print_preview_set_image_position (EOC_PRINT_PREVIEW (setup->priv->preview), pos_x, pos_y);
}
