#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cdk/cdkkeysyms.h>
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#include "eoc-config-keys.h"
#include "eoc-enum-types.h"
#include "eoc-marshal.h"
#include "eoc-scroll-view.h"
#include "eoc-debug.h"
#include "zoom.h"

#include <cdk/cdk.h>

/* Maximum size of delayed repaint rectangles */
#define PAINT_RECT_WIDTH 128
#define PAINT_RECT_HEIGHT 128

/* Scroll step increment */
#define SCROLL_STEP_SIZE 32

/* Maximum zoom factor */
#define MAX_ZOOM_FACTOR 20
#define MIN_ZOOM_FACTOR 0.02

#define CHECK_MEDIUM 8
#define CHECK_BLACK "#000000"
#define CHECK_DARK "#555555"
#define CHECK_GRAY "#808080"
#define CHECK_LIGHT "#cccccc"
#define CHECK_WHITE "#ffffff"

/* Default increment for zooming.  The current zoom factor is multiplied or
 * divided by this amount on every zooming step.  For consistency, you should
 * use the same value elsewhere in the program.
 */
#define IMAGE_VIEW_ZOOM_MULTIPLIER 1.05

/* States for automatically adjusting the zoom factor */
typedef enum {
	ZOOM_MODE_FIT,		/* Image is fitted to scroll view even if the latter changes size */
	ZOOM_MODE_FREE		/* The image remains at its current zoom factor even if the scrollview changes size  */
} ZoomMode;

/* Signal IDs */
enum {
	SIGNAL_ZOOM_CHANGED,
	SIGNAL_LAST
};

static guint view_signals [SIGNAL_LAST] = { 0 };

typedef enum {
	EOC_SCROLL_VIEW_CURSOR_NORMAL,
	EOC_SCROLL_VIEW_CURSOR_HIDDEN,
	EOC_SCROLL_VIEW_CURSOR_DRAG
} EocScrollViewCursor;

/* Drag 'n Drop */
static CtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0},
};

enum {
	PROP_0,
	PROP_ANTIALIAS_IN,
	PROP_ANTIALIAS_OUT,
	PROP_BACKGROUND_COLOR,
	PROP_IMAGE,
	PROP_SCROLLWHEEL_ZOOM,
	PROP_TRANSP_COLOR,
	PROP_TRANSPARENCY_STYLE,
	PROP_USE_BG_COLOR,
	PROP_ZOOM_MULTIPLIER
};

/* Private part of the EocScrollView structure */
struct _EocScrollViewPrivate {
	/* some widgets we rely on */
	CtkWidget *display;
	CtkAdjustment *hadj;
	CtkAdjustment *vadj;
	CtkWidget *hbar;
	CtkWidget *vbar;
	CtkWidget *menu;

	/* actual image */
	EocImage *image;
	guint image_changed_id;
	guint frame_changed_id;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;

	/* scale factor */
	gint scale;

	/* zoom mode, either ZOOM_MODE_FIT or ZOOM_MODE_FREE */
	ZoomMode zoom_mode;

	/* whether to allow zoom > 1.0 on zoom fit */
	gboolean upscale;

	/* the actual zoom factor */
	double zoom;

	/* the minimum possible (reasonable) zoom factor */
	double min_zoom;

	/* Current scrolling offsets */
	int xofs, yofs;

	/* handler ID for paint idle callback */
	guint idle_id;

	/* Interpolation type when zoomed in*/
	cairo_filter_t interp_type_in;

	/* Interpolation type when zoomed out*/
	cairo_filter_t interp_type_out;

	/* Scroll wheel zoom */
	gboolean scroll_wheel_zoom;

	/* Scroll wheel zoom */
	gdouble zoom_multiplier;

	/* dragging stuff */
	int drag_anchor_x, drag_anchor_y;
	int drag_ofs_x, drag_ofs_y;
	guint dragging : 1;

	/* how to indicate transparency in images */
	EocTransparencyStyle transp_style;
	CdkRGBA transp_color;

	/* the type of the cursor we are currently showing */
	EocScrollViewCursor cursor;

	gboolean  use_bg_color;
	CdkRGBA *background_color;
	CdkRGBA *override_bg_color;

	cairo_surface_t *background_surface;

	/* Two-pass filtering */
	GSource *hq_redraw_timeout_source;
	gboolean force_unfiltered;
};

static void scroll_by (EocScrollView *view, int xofs, int yofs);
static void set_zoom_fit (EocScrollView *view);
/* static void request_paint_area (EocScrollView *view, CdkRectangle *area); */
static void set_minimum_zoom_factor (EocScrollView *view);
static void view_on_drag_begin_cb (CtkWidget *widget, CdkDragContext *context,
				   gpointer user_data);
static void view_on_drag_data_get_cb (CtkWidget *widget,
				      CdkDragContext*drag_context,
				      CtkSelectionData *data, guint info,
				      guint time, gpointer user_data);

static gboolean _eoc_cdk_rgba_equal0 (const CdkRGBA *a, const CdkRGBA *b);

G_DEFINE_TYPE_WITH_PRIVATE (EocScrollView, eoc_scroll_view, CTK_TYPE_GRID)

/*===================================
    widget size changing handler &
        util functions
  ---------------------------------*/

static cairo_surface_t *
create_surface_from_pixbuf (EocScrollView *view, GdkPixbuf *pixbuf)
{
	cairo_surface_t *surface;

	surface = cdk_cairo_surface_create_from_pixbuf (pixbuf,
			                                view->priv->scale,
			                                ctk_widget_get_window (view->priv->display));

	return surface;
}

/* Disconnects from the EocImage and removes references to it */
static void
free_image_resources (EocScrollView *view)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->image_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->image_changed_id);
		priv->image_changed_id = 0;
	}

	if (priv->frame_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->frame_changed_id);
		priv->frame_changed_id = 0;
	}

	if (priv->image != NULL) {
		eoc_image_data_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (priv->surface !=NULL) {
		cairo_surface_destroy (priv->surface);
		priv->surface = NULL;
	}
}

/* Computes the size in pixels of the scaled image */
static void
compute_scaled_size (EocScrollView *view, double zoom, int *width, int *height)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf) {
		*width = floor (gdk_pixbuf_get_width (priv->pixbuf) / priv->scale * zoom + 0.5);
		*height = floor (gdk_pixbuf_get_height (priv->pixbuf) / priv->scale * zoom + 0.5);
	} else
		*width = *height = 0;
}

/* Computes the offsets for the new zoom value so that they keep the image
 * centered on the view.
 */
static void
compute_center_zoom_offsets (EocScrollView *view,
			     double old_zoom, double new_zoom,
			     int width, int height,
			     double zoom_x_anchor, double zoom_y_anchor,
			     int *xofs, int *yofs)
{
	EocScrollViewPrivate *priv;
	int old_scaled_width, old_scaled_height;
	int new_scaled_width, new_scaled_height;
	double view_cx, view_cy;

	priv = view->priv;

	compute_scaled_size (view, old_zoom,
			     &old_scaled_width, &old_scaled_height);

	if (old_scaled_width < width)
		view_cx = (zoom_x_anchor * old_scaled_width) / old_zoom;
	else
		view_cx = (priv->xofs + zoom_x_anchor * width) / old_zoom;

	if (old_scaled_height < height)
		view_cy = (zoom_y_anchor * old_scaled_height) / old_zoom;
	else
		view_cy = (priv->yofs + zoom_y_anchor * height) / old_zoom;

	compute_scaled_size (view, new_zoom,
			     &new_scaled_width, &new_scaled_height);

	if (new_scaled_width < width)
		*xofs = 0;
	else {
		*xofs = floor (view_cx * new_zoom - zoom_x_anchor * width + 0.5);
		if (*xofs < 0)
			*xofs = 0;
	}

	if (new_scaled_height < height)
		*yofs = 0;
	else {
		*yofs = floor (view_cy * new_zoom - zoom_y_anchor * height + 0.5);
		if (*yofs < 0)
			*yofs = 0;
	}
}

/* Sets the scrollbar values based on the current scrolling offset */
static void
update_scrollbar_values (EocScrollView *view)
{
	EocScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	gdouble page_size,page_increment,step_increment;
	gdouble lower, upper;
	CtkAllocation allocation;

	priv = view->priv;

	if (!ctk_widget_get_visible (CTK_WIDGET (priv->hbar))
	    && !ctk_widget_get_visible (CTK_WIDGET (priv->vbar)))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);
	ctk_widget_get_allocation (CTK_WIDGET (priv->display), &allocation);

	if (ctk_widget_get_visible (CTK_WIDGET (priv->hbar))) {
		/* Set scroll increments */
		page_size = MIN (scaled_width, allocation.width);

		page_increment = allocation.width / 2;
		step_increment = SCROLL_STEP_SIZE;

		/* Set scroll bounds and new offsets */
		lower = 0;
		upper = scaled_width;
		priv->xofs = CLAMP (priv->xofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		ctk_adjustment_configure (priv->hadj, priv->xofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}

	if (ctk_widget_get_visible (CTK_WIDGET (priv->vbar))) {
		page_size = MIN (scaled_height, allocation.height);
		page_increment = allocation.height / 2;
		step_increment = SCROLL_STEP_SIZE;

		lower = 0;
		upper = scaled_height;
		priv->yofs = CLAMP (priv->yofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		ctk_adjustment_configure (priv->vadj, priv->yofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}
}

static void
eoc_scroll_view_set_cursor (EocScrollView *view, EocScrollViewCursor new_cursor)
{
	CdkCursor *cursor = NULL;
	CdkDisplay *display;
	CtkWidget *widget;

	if (view->priv->cursor == new_cursor) {
		return;
	}

	widget = ctk_widget_get_toplevel (CTK_WIDGET (view));
	display = ctk_widget_get_display (widget);
	view->priv->cursor = new_cursor;

	switch (new_cursor) {
		case EOC_SCROLL_VIEW_CURSOR_NORMAL:
			cdk_window_set_cursor (ctk_widget_get_window (widget), NULL);
			break;
                case EOC_SCROLL_VIEW_CURSOR_HIDDEN:
                        cursor = cdk_cursor_new_for_display (display, CDK_BLANK_CURSOR);
                        break;
		case EOC_SCROLL_VIEW_CURSOR_DRAG:
			cursor = cdk_cursor_new_for_display (display, CDK_FLEUR);
			break;
	}

	if (cursor) {
		cdk_window_set_cursor (ctk_widget_get_window (widget), cursor);
		g_object_unref (cursor);
		cdk_display_flush (display);
	}
}

/* Changes visibility of the scrollbars based on the zoom factor and the
 * specified allocation, or the current allocation if NULL is specified.
 */
static void
check_scrollbar_visibility (EocScrollView *view, CtkAllocation *alloc)
{
	EocScrollViewPrivate *priv;
	int bar_height;
	int bar_width;
	int img_width;
	int img_height;
	CtkRequisition req;
	int width, height;
	gboolean hbar_visible, vbar_visible;

	priv = view->priv;

	if (alloc) {
		width = alloc->width;
		height = alloc->height;
	} else {
		CtkAllocation allocation;

		ctk_widget_get_allocation (CTK_WIDGET (view), &allocation);
		width = allocation.width;
		height = allocation.height;
	}

	compute_scaled_size (view, priv->zoom, &img_width, &img_height);

	/* this should work fairly well in this special case for scrollbars */
	ctk_widget_get_preferred_size (priv->hbar, &req, NULL);
	bar_height = req.height;
	ctk_widget_get_preferred_size (priv->vbar, &req, NULL);
	bar_width = req.width;

	eoc_debug_message (DEBUG_WINDOW, "Widget Size allocate: %i, %i   Bar: %i, %i\n",
			   width, height, bar_width, bar_height);

	hbar_visible = vbar_visible = FALSE;
	if (priv->zoom_mode == ZOOM_MODE_FIT)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width <= width && img_height <= height)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width > width && img_height > height)
		hbar_visible = vbar_visible = TRUE;
	else if (img_width > width) {
		hbar_visible = TRUE;
		if (img_height <= (height - bar_height))
			vbar_visible = FALSE;
		else
			vbar_visible = TRUE;
	}
        else if (img_height > height) {
		vbar_visible = TRUE;
		if (img_width <= (width - bar_width))
			hbar_visible = FALSE;
		else
			hbar_visible = TRUE;
	}

	if (hbar_visible != ctk_widget_get_visible (CTK_WIDGET (priv->hbar)))
		g_object_set (G_OBJECT (priv->hbar), "visible", hbar_visible, NULL);

	if (vbar_visible != ctk_widget_get_visible (CTK_WIDGET (priv->vbar)))
		g_object_set (G_OBJECT (priv->vbar), "visible", vbar_visible, NULL);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)

/* Returns whether the image is zoomed in */
static gboolean
is_zoomed_in (EocScrollView *view)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;
	return priv->zoom - 1.0 > DOUBLE_EQUAL_MAX_DIFF;
}

/* Returns whether the image is zoomed out */
static gboolean
is_zoomed_out (EocScrollView *view)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL_MAX_DIFF + priv->zoom - 1.0 < 0.0;
}

/* Returns wether the image is movable, that means if it is larger then
 * the actual visible area.
 */
static gboolean
is_image_movable (EocScrollView *view)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	return (ctk_widget_get_visible (priv->hbar) || ctk_widget_get_visible (priv->vbar));
}

/*===================================
          drawing core
  ---------------------------------*/

static void
get_transparency_params (EocScrollView *view, int *size, CdkRGBA *color1, CdkRGBA *color2)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	/* Compute transparency parameters */
	switch (priv->transp_style) {
	case EOC_TRANSP_BACKGROUND: {
		/* Simply return fully transparent color */
		color1->red = color1->green = color1->blue = color1->alpha = 0.0;
		color2->red = color2->green = color2->blue = color2->alpha = 0.0;
		break;
	}

	case EOC_TRANSP_CHECKED:
		g_warn_if_fail (cdk_rgba_parse (color1, CHECK_GRAY));
		g_warn_if_fail (cdk_rgba_parse (color2, CHECK_LIGHT));
		break;

	case EOC_TRANSP_COLOR:
		*color1 = *color2 = priv->transp_color;
		break;

	default:
		g_assert_not_reached ();
	};

	*size = CHECK_MEDIUM;
}

static cairo_surface_t *
create_background_surface (EocScrollView *view)
{
	int check_size;
	CdkRGBA check_1;
	CdkRGBA check_2;
	cairo_surface_t *surface;

	get_transparency_params (view, &check_size, &check_1, &check_2);
	surface = cdk_window_create_similar_surface (ctk_widget_get_window (view->priv->display),
						     CAIRO_CONTENT_COLOR_ALPHA,
						     check_size * 2, check_size * 2);
	cairo_t* cr = cairo_create (surface);

	/* Use source operator to make fully transparent work */
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	cdk_cairo_set_source_rgba(cr, &check_1);
	cairo_rectangle (cr, 0, 0, check_size, check_size);
	cairo_rectangle (cr, check_size, check_size, check_size, check_size);
	cairo_fill (cr);

	cdk_cairo_set_source_rgba(cr, &check_2);
	cairo_rectangle (cr, 0, check_size, check_size, check_size);
	cairo_rectangle (cr, check_size, 0, check_size, check_size);
	cairo_fill (cr);

	cairo_destroy (cr);

	return surface;
}

/* =======================================

    scrolling stuff

    --------------------------------------*/


/* Scrolls the view to the specified offsets.  */
static void
scroll_to (EocScrollView *view, int x, int y, gboolean change_adjustments)
{
	EocScrollViewPrivate *priv;
	CtkAllocation allocation;
	int xofs, yofs;
	CdkWindow *window;

	priv = view->priv;

	/* Check bounds & Compute offsets */
	if (ctk_widget_get_visible (priv->hbar)) {
		x = CLAMP (x, 0, ctk_adjustment_get_upper (priv->hadj)
				 - ctk_adjustment_get_page_size (priv->hadj));
		xofs = x - priv->xofs;
	} else
		xofs = 0;

	if (ctk_widget_get_visible (priv->vbar)) {
		y = CLAMP (y, 0, ctk_adjustment_get_upper (priv->vadj)
				 - ctk_adjustment_get_page_size (priv->vadj));
		yofs = y - priv->yofs;
	} else
		yofs = 0;

	if (xofs == 0 && yofs == 0)
		return;

	priv->xofs = x;
	priv->yofs = y;

	if (!ctk_widget_is_drawable (priv->display))
		goto out;

	ctk_widget_get_allocation (CTK_WIDGET (priv->display), &allocation);

	if (abs (xofs) >= allocation.width || abs (yofs) >= allocation.height) {
		ctk_widget_queue_draw (CTK_WIDGET (priv->display));
		goto out;
	}

	window = ctk_widget_get_window (CTK_WIDGET (priv->display));

	cdk_window_scroll (window, -xofs, -yofs);

 out:
	if (!change_adjustments)
		return;

	g_signal_handlers_block_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_block_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);

	ctk_adjustment_set_value (priv->hadj, x);
	ctk_adjustment_set_value (priv->vadj, y);

	g_signal_handlers_unblock_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_unblock_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
}

/* Scrolls the image view by the specified offsets.  Notifies the adjustments
 * about their new values.
 */
static void
scroll_by (EocScrollView *view, int xofs, int yofs)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	scroll_to (view, priv->xofs + xofs, priv->yofs + yofs, TRUE);
}


/* Callback used when an adjustment is changed */
static void
adjustment_changed_cb (CtkAdjustment *adj, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	scroll_to (view, ctk_adjustment_get_value (priv->hadj),
		   ctk_adjustment_get_value (priv->vadj), FALSE);
}


/* Drags the image to the specified position */
static void
drag_to (EocScrollView *view, int x, int y)
{
	EocScrollViewPrivate *priv;
	int dx, dy;

	priv = view->priv;

	dx = priv->drag_anchor_x - x;
	dy = priv->drag_anchor_y - y;

	x = priv->drag_ofs_x + dx;
	y = priv->drag_ofs_y + dy;

	scroll_to (view, x, y, TRUE);
}

static void
set_minimum_zoom_factor (EocScrollView *view)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	view->priv->min_zoom = MAX (1.0 / gdk_pixbuf_get_width (view->priv->pixbuf) / view->priv->scale,
				    MAX(1.0 / gdk_pixbuf_get_height (view->priv->pixbuf) / view->priv->scale,
					MIN_ZOOM_FACTOR) );
	return;
}

/**
 * set_zoom:
 * @view: A scroll view.
 * @zoom: Zoom factor.
 * @have_anchor: Whether the anchor point specified by (@anchorx, @anchory)
 * should be used.
 * @anchorx: Horizontal anchor point in pixels.
 * @anchory: Vertical anchor point in pixels.
 *
 * Sets the zoom factor for an image view.  The anchor point can be used to
 * specify the point that stays fixed when the image is zoomed.  If @have_anchor
 * is %TRUE, then (@anchorx, @anchory) specify the point relative to the image
 * view widget's allocation that will stay fixed when zooming.  If @have_anchor
 * is %FALSE, then the center point of the image view will be used.
 **/
static void
set_zoom (EocScrollView *view, double zoom,
	  gboolean have_anchor, int anchorx, int anchory)
{
	EocScrollViewPrivate *priv;
	CtkAllocation allocation;
	int xofs, yofs;
	double x_rel, y_rel;

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return;

	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;

	if (DOUBLE_EQUAL (priv->zoom, zoom))
		return;
	if (DOUBLE_EQUAL (priv->zoom, priv->min_zoom) && zoom < priv->zoom)
		return;

	priv->zoom_mode = ZOOM_MODE_FREE;

	ctk_widget_get_allocation (CTK_WIDGET (priv->display), &allocation);

	/* compute new xofs/yofs values */
	if (have_anchor) {
		x_rel = (double) anchorx / allocation.width;
		y_rel = (double) anchory / allocation.height;
	} else {
		x_rel = 0.5;
		y_rel = 0.5;
	}

	compute_center_zoom_offsets (view, priv->zoom, zoom,
				     allocation.width, allocation.height,
				     x_rel, y_rel,
				     &xofs, &yofs);

	/* set new values */
	priv->xofs = xofs; /* (img_width * x_rel * zoom) - anchorx; */
	priv->yofs = yofs; /* (img_height * y_rel * zoom) - anchory; */
#if 0
	g_print ("xofs: %i  yofs: %i\n", priv->xofs, priv->yofs);
#endif
	if (zoom <= priv->min_zoom)
		priv->zoom = priv->min_zoom;
	else
		priv->zoom = zoom;

	/* we make use of the new values here */
	check_scrollbar_visibility (view, NULL);
	update_scrollbar_values (view);

	/* repaint the whole image */
	ctk_widget_queue_draw (CTK_WIDGET (priv->display));

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/* Zooms the image to fit the available allocation */
static void
set_zoom_fit (EocScrollView *view)
{
	EocScrollViewPrivate *priv;
	CtkAllocation allocation;
	double new_zoom;

	priv = view->priv;

	priv->zoom_mode = ZOOM_MODE_FIT;

	if (!ctk_widget_get_mapped (CTK_WIDGET (view)))
		return;

	if (priv->pixbuf == NULL)
		return;

	ctk_widget_get_allocation (CTK_WIDGET(priv->display), &allocation);

	new_zoom = zoom_fit_scale (allocation.width, allocation.height,
				   gdk_pixbuf_get_width (priv->pixbuf) / priv->scale,
				   gdk_pixbuf_get_height (priv->pixbuf) / priv->scale,
				   priv->upscale);

	if (new_zoom > MAX_ZOOM_FACTOR)
		new_zoom = MAX_ZOOM_FACTOR;
	else if (new_zoom < MIN_ZOOM_FACTOR)
		new_zoom = MIN_ZOOM_FACTOR;

	priv->zoom = new_zoom;
	priv->xofs = 0;
	priv->yofs = 0;

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/*===================================

   internal signal callbacks

  ---------------------------------*/

/* Key press event handler for the image view */
static gboolean
display_key_press_event (CtkWidget *widget, CdkEventKey *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;
	CtkAllocation allocation;
	gboolean do_zoom;
	double zoom;
	gboolean do_scroll;
	int xofs, yofs;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	do_zoom = FALSE;
	do_scroll = FALSE;
	xofs = yofs = 0;
	zoom = 1.0;

	ctk_widget_get_allocation (CTK_WIDGET (priv->display), &allocation);

	/* EocScrollView doesn't handle/have any Alt+Key combos */
	if (event->state & CDK_MOD1_MASK) {
		return FALSE;
	}

	switch (event->keyval) {
	case CDK_KEY_Up:
		do_scroll = TRUE;
		xofs = 0;
		yofs = -SCROLL_STEP_SIZE;
		break;

	case CDK_KEY_Page_Up:
		do_scroll = TRUE;
		if (event->state & CDK_CONTROL_MASK) {
			xofs = -(allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = -(allocation.height * 3) / 4;
		}
		break;

	case CDK_KEY_Down:
		do_scroll = TRUE;
		xofs = 0;
		yofs = SCROLL_STEP_SIZE;
		break;

	case CDK_KEY_Page_Down:
		do_scroll = TRUE;
		if (event->state & CDK_CONTROL_MASK) {
			xofs = (allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = (allocation.height * 3) / 4;
		}
		break;

	case CDK_KEY_Left:
		do_scroll = TRUE;
		xofs = -SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case CDK_KEY_Right:
		do_scroll = TRUE;
		xofs = SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case CDK_KEY_plus:
	case CDK_KEY_equal:
	case CDK_KEY_KP_Add:
		do_zoom = TRUE;
		zoom = priv->zoom * priv->zoom_multiplier;
		break;

	case CDK_KEY_minus:
	case CDK_KEY_KP_Subtract:
		do_zoom = TRUE;
		zoom = priv->zoom / priv->zoom_multiplier;
		break;

	case CDK_KEY_1:
		do_zoom = TRUE;
		zoom = 1.0;
		break;

	default:
		return FALSE;
	}

	if (do_zoom) {
		CdkSeat *seat;
		CdkDevice *device;
		gint x, y;

		seat = cdk_display_get_default_seat (ctk_widget_get_display (widget));
		device = cdk_seat_get_pointer (seat);

		cdk_window_get_device_position (ctk_widget_get_window (widget), device,
		                                &x, &y, NULL);
		set_zoom (view, zoom, TRUE, x, y);
	}

	if (do_scroll)
		scroll_by (view, xofs, yofs);

	return TRUE;
}


/* Button press event handler for the image view */
static gboolean
eoc_scroll_view_button_press_event (CtkWidget *widget, CdkEventButton *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	if (!ctk_widget_has_focus (priv->display))
		ctk_widget_grab_focus (CTK_WIDGET (priv->display));

	if (priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
                        if (event->button == 1 && !priv->scroll_wheel_zoom &&
			    !(event->state & CDK_CONTROL_MASK))
				break;

			if (is_image_movable (view)) {
				eoc_scroll_view_set_cursor (view, EOC_SCROLL_VIEW_CURSOR_DRAG);

				priv->dragging = TRUE;
				priv->drag_anchor_x = event->x;
				priv->drag_anchor_y = event->y;

				priv->drag_ofs_x = priv->xofs;
				priv->drag_ofs_y = priv->yofs;

				return TRUE;
			}
		default:
			break;
	}

	return FALSE;
}

/* Button release event handler for the image view */
static gboolean
eoc_scroll_view_button_release_event (CtkWidget *widget, CdkEventButton *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
			drag_to (view, event->x, event->y);
			priv->dragging = FALSE;

			eoc_scroll_view_set_cursor (view, EOC_SCROLL_VIEW_CURSOR_NORMAL);
			break;

		default:
			break;
	}

	return TRUE;
}

/* Scroll event handler for the image view.  We zoom with an event without
 * modifiers rather than scroll; we use the Shift modifier to scroll.
 * Rationale: images are not primarily vertical, and in EOC you scan scroll by
 * dragging the image with button 1 anyways.
 */
static gboolean
eoc_scroll_view_scroll_event (CtkWidget *widget, CdkEventScroll *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;
	double zoom_factor;
	int xofs, yofs;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	/* Compute zoom factor and scrolling offsets; we'll only use either of them */
	/* same as in ctkscrolledwindow.c */
	xofs = ctk_adjustment_get_page_increment (priv->hadj) / 2;
	yofs = ctk_adjustment_get_page_increment (priv->vadj) / 2;

	switch (event->direction) {
	case CDK_SCROLL_UP:
		zoom_factor = priv->zoom_multiplier;
		xofs = 0;
		yofs = -yofs;
		break;

	case CDK_SCROLL_LEFT:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = -xofs;
		yofs = 0;
		break;

	case CDK_SCROLL_DOWN:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = 0;
		yofs = yofs;
		break;

	case CDK_SCROLL_RIGHT:
		zoom_factor = priv->zoom_multiplier;
		xofs = xofs;
		yofs = 0;
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

        if (priv->scroll_wheel_zoom) {
		if (event->state & CDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & CDK_CONTROL_MASK)
			scroll_by (view, xofs, yofs);
		else
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
	} else {
		if (event->state & CDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & CDK_CONTROL_MASK)
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
		else
			scroll_by (view, xofs, yofs);
        }

	return TRUE;
}

/* Motion event handler for the image view */
static gboolean
eoc_scroll_view_motion_event (CtkWidget *widget, CdkEventMotion *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;
	gint x, y;
	CdkModifierType mods;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	if (event->is_hint)
		cdk_window_get_device_position (ctk_widget_get_window (CTK_WIDGET (priv->display)), event->device, &x, &y, &mods);
	else {
		x = event->x;
		y = event->y;
	}

	drag_to (view, x, y);
	return TRUE;
}

static void
display_map_event (CtkWidget *widget, CdkEvent *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	eoc_debug (DEBUG_WINDOW);

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	ctk_widget_queue_draw (CTK_WIDGET (priv->display));
}

static void
eoc_scroll_view_size_allocate (CtkWidget *widget, CtkAllocation *alloc)
{
	EocScrollView *view;

	view = EOC_SCROLL_VIEW (widget);
	check_scrollbar_visibility (view, alloc);

	CTK_WIDGET_CLASS (eoc_scroll_view_parent_class)->size_allocate (widget
									,alloc);
}

static void
display_size_change (CtkWidget *widget, CdkEventConfigure *event, gpointer data)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	if (priv->zoom_mode == ZOOM_MODE_FIT) {
		CtkAllocation alloc;

		alloc.width = event->width;
		alloc.height = event->height;

		set_zoom_fit (view);
		check_scrollbar_visibility (view, &alloc);
		ctk_widget_queue_draw (CTK_WIDGET (priv->display));
	} else {
		int scaled_width, scaled_height;
		int x_offset = 0;
		int y_offset = 0;

		compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

		if (priv->xofs + event->width > scaled_width)
			x_offset = scaled_width - event->width - priv->xofs;

		if (priv->yofs + event->height > scaled_height)
			y_offset = scaled_height - event->height - priv->yofs;

		scroll_by (view, x_offset, y_offset);
	}

	update_scrollbar_values (view);
}


static gboolean
eoc_scroll_view_focus_in_event (CtkWidget     *widget,
			    CdkEventFocus *event,
			    gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_in_event");
	return FALSE;
}

static gboolean
eoc_scroll_view_focus_out_event (CtkWidget     *widget,
			     CdkEventFocus *event,
			     gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_out_event");
	return FALSE;
}

static gboolean _hq_redraw_cb (gpointer user_data)
{
	EocScrollViewPrivate *priv = EOC_SCROLL_VIEW (user_data)->priv;

	priv->force_unfiltered = FALSE;
	ctk_widget_queue_draw (CTK_WIDGET (priv->display));

	priv->hq_redraw_timeout_source = NULL;
	return G_SOURCE_REMOVE;
}

static void
_clear_hq_redraw_timeout (EocScrollView *view)
{
	EocScrollViewPrivate *priv = view->priv;

	if (priv->hq_redraw_timeout_source != NULL) {
		g_source_unref (priv->hq_redraw_timeout_source);
		g_source_destroy (priv->hq_redraw_timeout_source);
	}

	priv->hq_redraw_timeout_source = NULL;
}

static void
_set_hq_redraw_timeout (EocScrollView *view)
{
	GSource *source;

	_clear_hq_redraw_timeout (view);

	source = g_timeout_source_new (200);
	g_source_set_callback (source, &_hq_redraw_cb, view, NULL);

	g_source_attach (source, NULL);

	view->priv->hq_redraw_timeout_source = source;
}

static gboolean
display_draw (CtkWidget *widget, cairo_t *cr, gpointer data)
{
	const CdkRGBA *background_color = NULL;
	EocScrollView *view;
	EocScrollViewPrivate *priv;
	CtkAllocation allocation;
	int scaled_width, scaled_height;
	int xofs, yofs;

	g_return_val_if_fail (CTK_IS_DRAWING_AREA (widget), FALSE);
	g_return_val_if_fail (EOC_IS_SCROLL_VIEW (data), FALSE);

	view = EOC_SCROLL_VIEW (data);

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return TRUE;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	ctk_widget_get_allocation (CTK_WIDGET (priv->display), &allocation);

	/* Compute image offsets with respect to the window */

	if (scaled_width <= allocation.width)
		xofs = (allocation.width - scaled_width) / 2;
	else
		xofs = -priv->xofs;

	if (scaled_height <= allocation.height)
		yofs = (allocation.height - scaled_height) / 2;
	else
		yofs = -priv->yofs;

	eoc_debug_message (DEBUG_WINDOW, "zoom %.2f, xofs: %i, yofs: %i scaled w: %i h: %i\n",
	priv->zoom, xofs, yofs, scaled_width, scaled_height);

	/* Paint the background */
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	if (priv->transp_style != EOC_TRANSP_BACKGROUND)
		cairo_rectangle (cr, MAX (0, xofs), MAX (0, yofs),
				 scaled_width, scaled_height);
	if (priv->override_bg_color != NULL)
		background_color = priv->override_bg_color;
	else if (priv->use_bg_color)
		background_color = priv->background_color;
	if (background_color != NULL)
		cairo_set_source_rgba (cr,
				       background_color->red,
				       background_color->green,
				       background_color->blue,
				       background_color->alpha);
	else
		cairo_set_source (cr, cdk_window_get_background_pattern (ctk_widget_get_window (priv->display)));
	cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_fill (cr);

	if (gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface == NULL) {
			priv->background_surface = create_background_surface (view);
		}
		cairo_set_source_surface (cr, priv->background_surface, xofs, yofs);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
		cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
		cairo_fill (cr);
	}

	/* Make sure the image is only drawn as large as needed.
	 * This is especially necessary for SVGs where there might
	 * be more image data available outside the image boundaries.
	 */
	cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
	cairo_clip (cr);

#ifdef HAVE_RSVG
	if (eoc_image_is_svg (view->priv->image)) {
		cairo_matrix_t matrix, translate, scale, original;
		EocTransform *transform = eoc_image_get_transform (priv->image);
		cairo_matrix_init_identity (&matrix);
		if (transform) {
			cairo_matrix_t affine;
			double image_offset_x = 0., image_offset_y = 0.;

			eoc_transform_get_affine (transform, &affine);
			cairo_matrix_multiply (&matrix, &affine, &matrix);

			switch (eoc_transform_get_transform_type (transform)) {
			case EOC_TRANSFORM_ROT_90:
			case EOC_TRANSFORM_FLIP_HORIZONTAL:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				break;
			case EOC_TRANSFORM_ROT_270:
			case EOC_TRANSFORM_FLIP_VERTICAL:
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case EOC_TRANSFORM_ROT_180:
			case EOC_TRANSFORM_TRANSPOSE:
			case EOC_TRANSFORM_TRANSVERSE:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case EOC_TRANSFORM_NONE:
				default:
				break;
			}
			cairo_matrix_init_translate (&translate, image_offset_x, image_offset_y);
			cairo_matrix_multiply (&matrix, &matrix, &translate);
		}
		/* Zoom factor for SVGs is already scaled, so scale back to application pixels. */
		cairo_matrix_init_scale (&scale, priv->zoom / priv->scale, priv->zoom / priv->scale);
		cairo_matrix_multiply (&matrix, &matrix, &scale);
		cairo_matrix_init_translate (&translate, xofs, yofs);
		cairo_matrix_multiply (&matrix, &matrix, &translate);

		cairo_get_matrix (cr, &original);
		cairo_matrix_multiply (&matrix, &matrix, &original);
		cairo_set_matrix (cr, &matrix);

		rsvg_handle_render_cairo (eoc_image_get_svg (priv->image), cr);

	} else
#endif /* HAVE_RSVG */
	{
		cairo_filter_t interp_type;

		if(!DOUBLE_EQUAL(priv->zoom, 1.0) && priv->force_unfiltered)
		{
			interp_type = CAIRO_FILTER_NEAREST;
			_set_hq_redraw_timeout(view);
		}
		else
		{
			if (is_zoomed_in (view))
				interp_type = priv->interp_type_in;
			else
				interp_type = priv->interp_type_out;

			_clear_hq_redraw_timeout (view);
			priv->force_unfiltered = TRUE;
		}
		cairo_scale (cr, priv->zoom, priv->zoom);
		cairo_set_source_surface (cr, priv->surface, xofs/priv->zoom, yofs/priv->zoom);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_PAD);
		if (is_zoomed_in (view) || is_zoomed_out (view))
			cairo_pattern_set_filter (cairo_get_source (cr), interp_type);

		cairo_paint (cr);
	}

	return TRUE;
}


/*==================================

   image loading callbacks

   -----------------------------------*/

/* Use when the pixbuf in the view is changed, to keep a
   reference to it and create its cairo surface. */
static void
update_pixbuf (EocScrollView *view, GdkPixbuf *pixbuf)
{
	EocScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	priv->pixbuf = pixbuf;

	if (priv->surface) {
		cairo_surface_destroy (priv->surface);
	}
	priv->surface = create_surface_from_pixbuf (view, priv->pixbuf);
}

static void
image_changed_cb (EocImage *img, gpointer data)
{
	EocScrollViewPrivate *priv;

	priv = EOC_SCROLL_VIEW (data)->priv;

	update_pixbuf (EOC_SCROLL_VIEW (data), eoc_image_get_pixbuf (img));

	set_zoom_fit (EOC_SCROLL_VIEW (data));
	check_scrollbar_visibility (EOC_SCROLL_VIEW (data), NULL);

	ctk_widget_queue_draw (CTK_WIDGET (priv->display));
}

/*===================================
         public API
  ---------------------------------*/

void
eoc_scroll_view_hide_cursor (EocScrollView *view)
{
       eoc_scroll_view_set_cursor (view, EOC_SCROLL_VIEW_CURSOR_HIDDEN);
}

void
eoc_scroll_view_show_cursor (EocScrollView *view)
{
       eoc_scroll_view_set_cursor (view, EOC_SCROLL_VIEW_CURSOR_NORMAL);
}

/* general properties */
void
eoc_scroll_view_set_zoom_upscale (EocScrollView *view, gboolean upscale)
{
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->upscale != upscale) {
		priv->upscale = upscale;

		if (priv->zoom_mode == ZOOM_MODE_FIT) {
			set_zoom_fit (view);
			ctk_widget_queue_draw (CTK_WIDGET (priv->display));
		}
	}
}

void
eoc_scroll_view_set_antialiasing_in (EocScrollView *view, gboolean state)
{
	EocScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_in != new_interp_type) {
		priv->interp_type_in = new_interp_type;
		ctk_widget_queue_draw (CTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-in");
	}
}

void
eoc_scroll_view_set_antialiasing_out (EocScrollView *view, gboolean state)
{
	EocScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_out != new_interp_type) {
		priv->interp_type_out = new_interp_type;
		ctk_widget_queue_draw (CTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-out");

	}
}

static void
_transp_background_changed (EocScrollView *view)
{
	EocScrollViewPrivate *priv = view->priv;

	if (priv->pixbuf != NULL && gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface) {
			cairo_surface_destroy (priv->background_surface);
			/* Will be recreated if needed during redraw */
			priv->background_surface = NULL;
		}
		ctk_widget_queue_draw (CTK_WIDGET (priv->display));
	}

}

void
eoc_scroll_view_set_transparency_color (EocScrollView *view, CdkRGBA *color)
{
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (!_eoc_cdk_rgba_equal0 (&priv->transp_color, color)) {
		priv->transp_color = *color;
		if (priv->transp_style == EOC_TRANSP_COLOR)
		    _transp_background_changed (view);

		g_object_notify (G_OBJECT (view), "transparency-color");
	}
}

void
eoc_scroll_view_set_transparency (EocScrollView        *view,
				  EocTransparencyStyle  style)
{
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->transp_style != style) {
		priv->transp_style = style;
		_transp_background_changed (view);
		g_object_notify (G_OBJECT (view), "transparency-style");
	}
}

/* zoom api */

static double preferred_zoom_levels[] = {
	1.0 / 100, 1.0 / 50, 1.0 / 20,
	1.0 / 10.0, 1.0 / 5.0, 1.0 / 3.0, 1.0 / 2.0, 1.0 / 1.5,
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0
};
static const gint n_zoom_levels = (sizeof (preferred_zoom_levels) / sizeof (double));

void
eoc_scroll_view_zoom_in (EocScrollView *view, gboolean smooth)
{
	EocScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom * priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = 0; i < n_zoom_levels; i++) {
			if (preferred_zoom_levels [i] - priv->zoom
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);

}

void
eoc_scroll_view_zoom_out (EocScrollView *view, gboolean smooth)
{
	EocScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom / priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = n_zoom_levels - 1; i >= 0; i--) {
			if (priv->zoom - preferred_zoom_levels [i]
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);
}

void
eoc_scroll_view_zoom_fit (EocScrollView *view)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	ctk_widget_queue_draw (CTK_WIDGET (view->priv->display));
}

void
eoc_scroll_view_set_zoom (EocScrollView *view, double zoom)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	set_zoom (view, zoom, FALSE, 0, 0);
}

double
eoc_scroll_view_get_zoom (EocScrollView *view)
{
	g_return_val_if_fail (EOC_IS_SCROLL_VIEW (view), 0.0);

	return view->priv->zoom;
}

gboolean
eoc_scroll_view_get_zoom_is_min (EocScrollView *view)
{
	g_return_val_if_fail (EOC_IS_SCROLL_VIEW (view), FALSE);

	set_minimum_zoom_factor (view);

	return DOUBLE_EQUAL (view->priv->zoom, MIN_ZOOM_FACTOR) ||
	       DOUBLE_EQUAL (view->priv->zoom, view->priv->min_zoom);
}

gboolean
eoc_scroll_view_get_zoom_is_max (EocScrollView *view)
{
	g_return_val_if_fail (EOC_IS_SCROLL_VIEW (view), FALSE);

	return DOUBLE_EQUAL (view->priv->zoom, MAX_ZOOM_FACTOR);
}

static void
display_next_frame_cb (EocImage *image, gint delay, gpointer data)
{
 	EocScrollViewPrivate *priv;
	EocScrollView *view;

	if (!EOC_IS_SCROLL_VIEW (data))
		return;

	view = EOC_SCROLL_VIEW (data);
	priv = view->priv;

	update_pixbuf (view, eoc_image_get_pixbuf (image));
	ctk_widget_queue_draw (CTK_WIDGET (priv->display));
}

void
eoc_scroll_view_set_image (EocScrollView *view, EocImage *image)
{
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->image == image) {
		return;
	}

	if (priv->image != NULL) {
		free_image_resources (view);
	}
	g_assert (priv->image == NULL);
	g_assert (priv->pixbuf == NULL);

	if (image != NULL) {
		eoc_image_data_ref (image);

		if (priv->pixbuf == NULL) {
			update_pixbuf (view, eoc_image_get_pixbuf (image));
			set_zoom_fit (view);
			check_scrollbar_visibility (view, NULL);
			ctk_widget_queue_draw (CTK_WIDGET (priv->display));

		}

		priv->image_changed_id = g_signal_connect (image, "changed",
							   (GCallback) image_changed_cb, view);
		if (eoc_image_is_animation (image) == TRUE ) {
			eoc_image_start_animation (image);
			priv->frame_changed_id = g_signal_connect (image, "next-frame",
								    (GCallback) display_next_frame_cb, view);
		}
	}

	priv->image = image;

	g_object_notify (G_OBJECT (view), "image");
}

/**
 * eoc_scroll_view_get_image:
 * @view: An #EocScrollView.
 *
 * Gets the the currently displayed #EocImage.
 *
 * Returns: (transfer full): An #EocImage.
 **/
EocImage*
eoc_scroll_view_get_image (EocScrollView *view)
{
	EocImage *img;

	g_return_val_if_fail (EOC_IS_SCROLL_VIEW (view), NULL);

	img = view->priv->image;

	if (img != NULL)
		g_object_ref (img);

	return img;
}

gboolean
eoc_scroll_view_scrollbars_visible (EocScrollView *view)
{
	if (!ctk_widget_get_visible (CTK_WIDGET (view->priv->hbar)) &&
	    !ctk_widget_get_visible (CTK_WIDGET (view->priv->vbar)))
		return FALSE;

	return TRUE;
}

/*===================================
    object creation/freeing
  ---------------------------------*/

static gboolean
sv_string_to_rgba_mapping (GValue   *value,
			    GVariant *variant,
			    gpointer  user_data)
{
	CdkRGBA color;

	g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING), FALSE);

	if (cdk_rgba_parse (&color, g_variant_get_string (variant, NULL))) {
		g_value_set_boxed (value, &color);
		return TRUE;
	}

	return FALSE;
}

static GVariant*
sv_rgba_to_string_mapping (const GValue       *value,
			    const GVariantType *expected_type,
			    gpointer            user_data)
{
	GVariant *variant = NULL;
	CdkRGBA *color;
	gchar *hex_val;

	g_return_val_if_fail (G_VALUE_TYPE (value) == CDK_TYPE_RGBA, NULL);
	g_return_val_if_fail (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING), NULL);

	color = g_value_get_boxed (value);
	hex_val = cdk_rgba_to_string(color);
	variant = g_variant_new_string (hex_val);
	g_free (hex_val);

	return variant;
}

static void
eoc_scroll_view_init (EocScrollView *view)
{
	GSettings *settings;
	EocScrollViewPrivate *priv;

	priv = view->priv = eoc_scroll_view_get_instance_private (view);
	settings = g_settings_new (EOC_CONF_VIEW);

	priv->zoom = 1.0;
	priv->min_zoom = MIN_ZOOM_FACTOR;
	priv->zoom_mode = ZOOM_MODE_FIT;
	priv->upscale = FALSE;
	priv->interp_type_in = CAIRO_FILTER_GOOD;
	priv->interp_type_out = CAIRO_FILTER_GOOD;
	priv->scroll_wheel_zoom = FALSE;
	priv->zoom_multiplier = IMAGE_VIEW_ZOOM_MULTIPLIER;
	priv->image = NULL;
	priv->pixbuf = NULL;
	priv->surface = NULL;
	priv->transp_style = EOC_TRANSP_BACKGROUND;
	g_warn_if_fail (cdk_rgba_parse(&priv->transp_color, CHECK_BLACK));
	priv->cursor = EOC_SCROLL_VIEW_CURSOR_NORMAL;
	priv->menu = NULL;
	priv->background_color = NULL;

	priv->hadj = CTK_ADJUSTMENT (ctk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->hadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

	priv->hbar = ctk_scrollbar_new (CTK_ORIENTATION_HORIZONTAL, priv->hadj);
	priv->vadj = CTK_ADJUSTMENT (ctk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->vadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

	priv->vbar = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, priv->vadj);
	priv->display = g_object_new (CTK_TYPE_DRAWING_AREA,
				      "can-focus", TRUE,
				      NULL);
	priv->scale = ctk_widget_get_scale_factor (CTK_WIDGET (priv->display));

	ctk_widget_add_events (CTK_WIDGET (priv->display),
			       CDK_EXPOSURE_MASK
			       | CDK_BUTTON_PRESS_MASK
			       | CDK_BUTTON_RELEASE_MASK
			       | CDK_POINTER_MOTION_MASK
			       | CDK_POINTER_MOTION_HINT_MASK
			       | CDK_SCROLL_MASK
			       | CDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (priv->display), "configure_event",
			  G_CALLBACK (display_size_change), view);
	g_signal_connect (G_OBJECT (priv->display), "draw", G_CALLBACK (display_draw), view);
	g_signal_connect (G_OBJECT (priv->display), "map_event",
			  G_CALLBACK (display_map_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_press_event",
			  G_CALLBACK (eoc_scroll_view_button_press_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "motion_notify_event",
			  G_CALLBACK (eoc_scroll_view_motion_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_release_event",
			  G_CALLBACK (eoc_scroll_view_button_release_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "scroll_event",
			  G_CALLBACK (eoc_scroll_view_scroll_event), view);
	g_signal_connect (G_OBJECT (priv->display), "focus_in_event",
			  G_CALLBACK (eoc_scroll_view_focus_in_event), NULL);
	g_signal_connect (G_OBJECT (priv->display), "focus_out_event",
			  G_CALLBACK (eoc_scroll_view_focus_out_event), NULL);

	g_signal_connect (G_OBJECT (view), "key_press_event",
			  G_CALLBACK (display_key_press_event), view);

	ctk_drag_source_set (priv->display, CDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     CDK_ACTION_COPY | CDK_ACTION_MOVE |
			     CDK_ACTION_LINK | CDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (priv->display), "drag-data-get",
			  G_CALLBACK (view_on_drag_data_get_cb), view);
	g_signal_connect (G_OBJECT (priv->display), "drag-begin",
			  G_CALLBACK (view_on_drag_begin_cb), view);

	ctk_grid_attach (CTK_GRID (view), priv->display,
					 0, 0, 1, 1);
	ctk_widget_set_hexpand (priv->display, TRUE);
	ctk_widget_set_vexpand (priv->display, TRUE);
	ctk_grid_attach (CTK_GRID (view), priv->hbar,
					 0, 1, 1, 1);
	ctk_widget_set_hexpand (priv->hbar, TRUE);
	ctk_grid_attach (CTK_GRID (view), priv->vbar,
					 1, 0, 1, 1);
	ctk_widget_set_vexpand (priv->vbar, TRUE);

	g_settings_bind (settings, EOC_CONF_VIEW_USE_BG_COLOR, view,
			 "use-background-color", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping (settings, EOC_CONF_VIEW_BACKGROUND_COLOR,
				      view, "background-color",
				      G_SETTINGS_BIND_DEFAULT,
				      sv_string_to_rgba_mapping,
				      sv_rgba_to_string_mapping, NULL, NULL);
	g_settings_bind (settings, EOC_CONF_VIEW_EXTRAPOLATE, view,
			 "antialiasing-in", G_SETTINGS_BIND_GET);
	g_settings_bind (settings, EOC_CONF_VIEW_INTERPOLATE, view,
			 "antialiasing-out", G_SETTINGS_BIND_GET);
	g_settings_bind_with_mapping (settings, EOC_CONF_VIEW_TRANS_COLOR,
				      view, "transparency-color",
				      G_SETTINGS_BIND_GET,
				      sv_string_to_rgba_mapping,
				      sv_rgba_to_string_mapping, NULL, NULL);
	g_settings_bind (settings, EOC_CONF_VIEW_TRANSPARENCY, view,
			 "transparency-style", G_SETTINGS_BIND_GET);

	g_object_unref (settings);

	priv->override_bg_color = NULL;
	priv->background_surface = NULL;
}

static void
eoc_scroll_view_dispose (GObject *object)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (object));

	view = EOC_SCROLL_VIEW (object);
	priv = view->priv;

	_clear_hq_redraw_timeout (view);

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	if (priv->background_color != NULL) {
		cdk_rgba_free (priv->background_color);
		priv->background_color = NULL;
	}

	if (priv->override_bg_color != NULL) {
		cdk_rgba_free (priv->override_bg_color);
		priv->override_bg_color = NULL;
	}

	if (priv->background_surface != NULL) {
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	free_image_resources (view);

	G_OBJECT_CLASS (eoc_scroll_view_parent_class)->dispose (object);
}

static void
eoc_scroll_view_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	EocScrollView *view;
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (object));

	view = EOC_SCROLL_VIEW (object);
	priv = view->priv;

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
	{
		gboolean filter = (priv->interp_type_in != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_ANTIALIAS_OUT:
	{
		gboolean filter = (priv->interp_type_out != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_USE_BG_COLOR:
		g_value_set_boolean (value, priv->use_bg_color);
		break;
	case PROP_BACKGROUND_COLOR:
		//FIXME: This doesn't really handle the NULL color.
		g_value_set_boxed (value, priv->background_color);
		break;
	case PROP_SCROLLWHEEL_ZOOM:
		g_value_set_boolean (value, priv->scroll_wheel_zoom);
		break;
	case PROP_TRANSPARENCY_STYLE:
		g_value_set_enum (value, priv->transp_style);
		break;
	case PROP_ZOOM_MULTIPLIER:
		g_value_set_double (value, priv->zoom_multiplier);
		break;
	case PROP_IMAGE:
		g_value_set_object (value, priv->image);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eoc_scroll_view_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
	EocScrollView *view;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (object));

	view = EOC_SCROLL_VIEW (object);

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
		eoc_scroll_view_set_antialiasing_in (view, g_value_get_boolean (value));
		break;
	case PROP_ANTIALIAS_OUT:
		eoc_scroll_view_set_antialiasing_out (view, g_value_get_boolean (value));
		break;
	case PROP_USE_BG_COLOR:
		eoc_scroll_view_set_use_bg_color (view, g_value_get_boolean (value));
		break;
	case PROP_BACKGROUND_COLOR:
	{
		const CdkRGBA *color = g_value_get_boxed (value);
		eoc_scroll_view_set_background_color (view, color);
		break;
	}
	case PROP_SCROLLWHEEL_ZOOM:
		eoc_scroll_view_set_scroll_wheel_zoom (view, g_value_get_boolean (value));
		break;
	case PROP_TRANSP_COLOR:
		eoc_scroll_view_set_transparency_color (view, g_value_get_boxed (value));
		break;
	case PROP_TRANSPARENCY_STYLE:
		eoc_scroll_view_set_transparency (view, g_value_get_enum (value));
		break;
	case PROP_ZOOM_MULTIPLIER:
		eoc_scroll_view_set_zoom_multiplier (view, g_value_get_double (value));
		break;
	case PROP_IMAGE:
		eoc_scroll_view_set_image (view, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
eoc_scroll_view_class_init (EocScrollViewClass *klass)
{
	GObjectClass *gobject_class;
	CtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) klass;
	widget_class = (CtkWidgetClass*) klass;

	gobject_class->dispose = eoc_scroll_view_dispose;
        gobject_class->set_property = eoc_scroll_view_set_property;
        gobject_class->get_property = eoc_scroll_view_get_property;

	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_IN,
		g_param_spec_boolean ("antialiasing-in", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_OUT,
		g_param_spec_boolean ("antialiasing-out", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:background-color:
	 *
	 * This is the default background color used for painting the background
	 * of the image view. If set to %NULL the color is determined by the
	 * active CTK theme.
	 */
	g_object_class_install_property (
		gobject_class, PROP_BACKGROUND_COLOR,
		g_param_spec_boxed ("background-color", NULL, NULL,
				    CDK_TYPE_RGBA,
				    G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_USE_BG_COLOR,
		g_param_spec_boolean ("use-background-color", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:zoom-multiplier:
	 *
	 * The current zoom factor is multiplied with this value + 1.0 when
	 * scrolling with the scrollwheel to determine the next zoom factor.
	 */
	g_object_class_install_property (
		gobject_class, PROP_ZOOM_MULTIPLIER,
		g_param_spec_double ("zoom-multiplier", NULL, NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE -1.0, 0.05,
				     G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:scrollwheel-zoom:
	 *
	 * If %TRUE the scrollwheel will zoom the view, otherwise it will be
	 * used for scrolling a zoomed image.
	 */
	g_object_class_install_property (
		gobject_class, PROP_SCROLLWHEEL_ZOOM,
		g_param_spec_boolean ("scrollwheel-zoom", NULL, NULL, TRUE,
		G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:image:
	 *
	 * This is the currently display #EocImage.
	 */
	g_object_class_install_property (
		gobject_class, PROP_IMAGE,
		g_param_spec_object ("image", NULL, NULL, EOC_TYPE_IMAGE,
				     G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:transparency-color:
	 *
	 * This is the color used to fill the transparent parts of an image
	 * if :transparency-style is set to use a custom color.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSP_COLOR,
		g_param_spec_boxed ("transparency-color", NULL, NULL,
				    CDK_TYPE_RGBA,
				    G_PARAM_WRITABLE | G_PARAM_STATIC_NAME));

	/**
	 * EocScrollView:transparency-style:
	 *
	 * Determines how to fill the shown image's transparent areas.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSPARENCY_STYLE,
		g_param_spec_enum ("transparency-style", NULL, NULL,
				   EOC_TYPE_TRANSPARENCY_STYLE,
				   EOC_TRANSP_CHECKED,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	view_signals [SIGNAL_ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      EOC_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EocScrollViewClass, zoom_changed),
			      NULL, NULL,
			      eoc_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);

	widget_class->size_allocate = eoc_scroll_view_size_allocate;
}

static void
view_on_drag_begin_cb (CtkWidget        *widget,
		       CdkDragContext   *context,
		       gpointer          user_data)
{
	EocScrollView *view;
	EocImage *image;
	GdkPixbuf *thumbnail;
	gint width, height;

	view = EOC_SCROLL_VIEW (user_data);
	image = view->priv->image;

	thumbnail = eoc_image_get_thumbnail (image);

	if  (thumbnail) {
		width = gdk_pixbuf_get_width (thumbnail) / view->priv->scale;
		height = gdk_pixbuf_get_height (thumbnail) / view->priv->scale;
		ctk_drag_set_icon_pixbuf (context, thumbnail, width/2, height/2);
		g_object_unref (thumbnail);
	}
}

static void
view_on_drag_data_get_cb (CtkWidget        *widget,
			  CdkDragContext   *drag_context,
			  CtkSelectionData *data,
			  guint             info,
			  guint             time,
			  gpointer          user_data)
{
	EocScrollView *view;
	EocImage *image;
	gchar *uris[2];
	GFile *file;

	view = EOC_SCROLL_VIEW (user_data);

	image = view->priv->image;

	file = eoc_image_get_file (image);
	uris[0] = g_file_get_uri (file);
	uris[1] = NULL;

	ctk_selection_data_set_uris (data, uris);

	g_free (uris[0]);
	g_object_unref (file);
}

CtkWidget*
eoc_scroll_view_new (void)
{
	CtkWidget *widget;

	widget = g_object_new (EOC_TYPE_SCROLL_VIEW,
			       "can-focus", TRUE,
			       "row-homogeneous", FALSE,
			       "column-homogeneous", FALSE,
			       NULL);


	return widget;
}

static gboolean
view_on_button_press_event_cb (CtkWidget *widget, CdkEventButton *event,
			       gpointer user_data)
{
    EocScrollView *view = EOC_SCROLL_VIEW (widget);

    /* Ignore double-clicks and triple-clicks */
    if (event->button == 3 && event->type == CDK_BUTTON_PRESS)
    {
	    ctk_menu_popup_at_pointer (CTK_MENU (view->priv->menu),
	                               (const CdkEvent*) event);

	    return TRUE;
    }

    return FALSE;
}

void
eoc_scroll_view_set_popup (EocScrollView *view,
			   CtkMenu *menu)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));
	g_return_if_fail (view->priv->menu == NULL);

	view->priv->menu = g_object_ref (CTK_WIDGET (menu));

	ctk_menu_attach_to_widget (CTK_MENU (view->priv->menu),
				   CTK_WIDGET (view),
				   NULL);

	g_signal_connect (G_OBJECT (view), "button_press_event",
			  G_CALLBACK (view_on_button_press_event_cb), NULL);
}

static gboolean
_eoc_cdk_rgba_equal0 (const CdkRGBA *a, const CdkRGBA *b)
{
	if (a == NULL || b == NULL)
		return (a == b);

	return cdk_rgba_equal (a, b);
}

static gboolean
_eoc_replace_cdk_rgba (CdkRGBA **dest, const CdkRGBA *src)
{
	CdkRGBA *old = *dest;

	if (_eoc_cdk_rgba_equal0 (old, src))
		return FALSE;

	if (old != NULL)
		cdk_rgba_free (old);

	*dest = (src) ? cdk_rgba_copy (src) : NULL;

	return TRUE;
}

static void
_eoc_scroll_view_update_bg_color (EocScrollView *view)
{
	EocScrollViewPrivate *priv = view->priv;

	if (priv->transp_style == EOC_TRANSP_BACKGROUND
	    && priv->background_surface != NULL) {
		/* Delete the SVG background to have it recreated with
		 * the correct color during the next SVG redraw */
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	ctk_widget_queue_draw (priv->display);
}

void
eoc_scroll_view_set_background_color (EocScrollView *view,
				      const CdkRGBA *color)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	if (_eoc_replace_cdk_rgba (&view->priv->background_color, color))
		_eoc_scroll_view_update_bg_color (view);
}

void
eoc_scroll_view_override_bg_color (EocScrollView *view,
				   const CdkRGBA *color)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	if (_eoc_replace_cdk_rgba (&view->priv->override_bg_color, color))
		_eoc_scroll_view_update_bg_color (view);
}

void
eoc_scroll_view_set_use_bg_color (EocScrollView *view, gboolean use)
{
	EocScrollViewPrivate *priv;

	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (use != priv->use_bg_color) {
		priv->use_bg_color = use;

		_eoc_scroll_view_update_bg_color (view);

		g_object_notify (G_OBJECT (view), "use-background-color");
	}
}

void
eoc_scroll_view_set_scroll_wheel_zoom (EocScrollView *view,
				       gboolean       scroll_wheel_zoom)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

	if (view->priv->scroll_wheel_zoom != scroll_wheel_zoom) {
		view->priv->scroll_wheel_zoom = scroll_wheel_zoom;
		g_object_notify (G_OBJECT (view), "scrollwheel-zoom");
	}
}

void
eoc_scroll_view_set_zoom_multiplier (EocScrollView *view,
				     gdouble        zoom_multiplier)
{
	g_return_if_fail (EOC_IS_SCROLL_VIEW (view));

        view->priv->zoom_multiplier = 1.0 + zoom_multiplier;

	g_object_notify (G_OBJECT (view), "zoom-multiplier");
}
