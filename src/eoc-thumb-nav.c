/* Eye Of Mate - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#include "eoc-thumb-nav.h"
#include "eoc-thumb-view.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#define EOC_THUMB_NAV_SCROLL_INC      20
#define EOC_THUMB_NAV_SCROLL_MOVE     20
#define EOC_THUMB_NAV_SCROLL_TIMEOUT  20

enum
{
	PROP_0,
	PROP_SHOW_BUTTONS,
	PROP_THUMB_VIEW,
	PROP_MODE
};

struct _EomThumbNavPrivate {
	EomThumbNavMode   mode;

	gboolean          show_buttons;
	gboolean          scroll_dir;
	gint              scroll_pos;
	gint              scroll_id;

	GtkWidget        *button_left;
	GtkWidget        *button_right;
	GtkWidget        *sw;
	GtkWidget        *thumbview;
	GtkAdjustment    *adj;
};

G_DEFINE_TYPE_WITH_PRIVATE (EomThumbNav, eoc_thumb_nav, GTK_TYPE_BOX);

static gboolean
eoc_thumb_nav_scroll_event (GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	EomThumbNav *nav = EOC_THUMB_NAV (user_data);
	gint inc = EOC_THUMB_NAV_SCROLL_INC * 3;

	if (nav->priv->mode != EOC_THUMB_NAV_MODE_ONE_ROW)
		return FALSE;

	switch (event->direction) {
	case GDK_SCROLL_UP:
	case GDK_SCROLL_LEFT:
		inc *= -1;
		break;

	case GDK_SCROLL_DOWN:
	case GDK_SCROLL_RIGHT:
		break;

	case GDK_SCROLL_SMOOTH:
	{
		/* Compatibility code to catch smooth events from mousewheels */
		gdouble x_delta, y_delta;
		gboolean set = gdk_event_get_scroll_deltas ((GdkEvent*)event,
							    &x_delta, &y_delta);

		/* Propagate horizontal smooth scroll events further,
		   as well as non-mousewheel events. */
		if (G_UNLIKELY (!set) || x_delta != 0.0 || fabs(y_delta) != 1.0)
			return FALSE;

		/* The y_delta is either +1.0 or -1.0 here */
		inc *= (gint) y_delta;
	}
	break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

	if (inc < 0)
		gtk_adjustment_set_value (nav->priv->adj, MAX (0, gtk_adjustment_get_value (nav->priv->adj) + inc));
	else
		gtk_adjustment_set_value (nav->priv->adj, MIN (gtk_adjustment_get_upper (nav->priv->adj) - gtk_adjustment_get_page_size (nav->priv->adj), gtk_adjustment_get_value (nav->priv->adj) + inc));

	return TRUE;
}

static void
eoc_thumb_nav_adj_changed (GtkAdjustment *adj, gpointer user_data)
{
	EomThumbNav *nav;
	EomThumbNavPrivate *priv;
	gboolean ltr;

	nav = EOC_THUMB_NAV (user_data);
	priv = eoc_thumb_nav_get_instance_private (nav);
	ltr = gtk_widget_get_direction (priv->sw) == GTK_TEXT_DIR_LTR;

	gtk_widget_set_sensitive (ltr ? priv->button_right : priv->button_left,
				  gtk_adjustment_get_value (adj)
				   < gtk_adjustment_get_upper (adj)
				    - gtk_adjustment_get_page_size (adj));
}

static void
eoc_thumb_nav_adj_value_changed (GtkAdjustment *adj, gpointer user_data)
{
	EomThumbNav *nav;
	EomThumbNavPrivate *priv;
	gboolean ltr;

	nav = EOC_THUMB_NAV (user_data);
	priv = eoc_thumb_nav_get_instance_private (nav);
	ltr = gtk_widget_get_direction (priv->sw) == GTK_TEXT_DIR_LTR;

	gtk_widget_set_sensitive (ltr ? priv->button_left : priv->button_right,
				  gtk_adjustment_get_value (adj) > 0);

	gtk_widget_set_sensitive (ltr ? priv->button_right : priv->button_left,
				  gtk_adjustment_get_value (adj)
				   < gtk_adjustment_get_upper (adj)
				    - gtk_adjustment_get_page_size (adj));
}

static gboolean
eoc_thumb_nav_scroll_step (gpointer user_data)
{
	EomThumbNav *nav = EOC_THUMB_NAV (user_data);
	GtkAdjustment *adj = nav->priv->adj;
	gint delta;

	if (nav->priv->scroll_pos < 10)
		delta = EOC_THUMB_NAV_SCROLL_INC;
	else if (nav->priv->scroll_pos < 20)
		delta = EOC_THUMB_NAV_SCROLL_INC * 2;
	else if (nav->priv->scroll_pos < 30)
		delta = EOC_THUMB_NAV_SCROLL_INC * 2 + 5;
	else
		delta = EOC_THUMB_NAV_SCROLL_INC * 2 + 12;

	if (!nav->priv->scroll_dir)
		delta *= -1;

	if ((gint) (gtk_adjustment_get_value (adj) + (gdouble) delta) >= 0 &&
	    (gint) (gtk_adjustment_get_value (adj) + (gdouble) delta) <= gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj)) {
		gtk_adjustment_set_value(adj,
			gtk_adjustment_get_value (adj) + (gdouble) delta);
		nav->priv->scroll_pos++;
	} else {
		if (delta > 0)
		      gtk_adjustment_set_value (adj,
		      	gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
		else
		      gtk_adjustment_set_value (adj, 0);

		nav->priv->scroll_pos = 0;

		return FALSE;
	}

	return TRUE;
}

static void
eoc_thumb_nav_button_clicked (GtkButton *button, EomThumbNav *nav)
{
	nav->priv->scroll_pos = 0;

	nav->priv->scroll_dir = gtk_widget_get_direction (GTK_WIDGET (button)) == GTK_TEXT_DIR_LTR ?
		GTK_WIDGET (button) == nav->priv->button_right :
		GTK_WIDGET (button) == nav->priv->button_left;

	eoc_thumb_nav_scroll_step (nav);
}

static void
eoc_thumb_nav_start_scroll (GtkButton *button, EomThumbNav *nav)
{
	nav->priv->scroll_dir = gtk_widget_get_direction (GTK_WIDGET (button)) == GTK_TEXT_DIR_LTR ?
		GTK_WIDGET (button) == nav->priv->button_right :
		GTK_WIDGET (button) == nav->priv->button_left;

	nav->priv->scroll_id = g_timeout_add (EOC_THUMB_NAV_SCROLL_TIMEOUT,
					      eoc_thumb_nav_scroll_step,
					      nav);
}

static void
eoc_thumb_nav_stop_scroll (GtkButton *button, EomThumbNav *nav)
{
	if (nav->priv->scroll_id > 0) {
		g_source_remove (nav->priv->scroll_id);
		nav->priv->scroll_id = 0;
		nav->priv->scroll_pos = 0;
	}
}

static void
eoc_thumb_nav_get_property (GObject    *object,
			    guint       property_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	EomThumbNav *nav = EOC_THUMB_NAV (object);

	switch (property_id)
	{
	case PROP_SHOW_BUTTONS:
		g_value_set_boolean (value,
			eoc_thumb_nav_get_show_buttons (nav));
		break;

	case PROP_THUMB_VIEW:
		g_value_set_object (value, nav->priv->thumbview);
		break;

	case PROP_MODE:
		g_value_set_int (value,
			eoc_thumb_nav_get_mode (nav));
		break;
	}
}

static void
eoc_thumb_nav_set_property (GObject      *object,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	EomThumbNav *nav = EOC_THUMB_NAV (object);

	switch (property_id)
	{
	case PROP_SHOW_BUTTONS:
		eoc_thumb_nav_set_show_buttons (nav,
			g_value_get_boolean (value));
		break;

	case PROP_THUMB_VIEW:
		nav->priv->thumbview =
			GTK_WIDGET (g_value_get_object (value));
		break;

	case PROP_MODE:
		eoc_thumb_nav_set_mode (nav,
			g_value_get_int (value));
		break;
	}
}

static GObject *
eoc_thumb_nav_constructor (GType type,
			   guint n_construct_properties,
			   GObjectConstructParam *construct_params)
{
	GObject *object;
	EomThumbNavPrivate *priv;

	object = G_OBJECT_CLASS (eoc_thumb_nav_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	priv = EOC_THUMB_NAV (object)->priv;

	if (priv->thumbview != NULL) {
		gtk_container_add (GTK_CONTAINER (priv->sw), priv->thumbview);
		gtk_widget_show_all (priv->sw);
	}

	return object;
}

static void
eoc_thumb_nav_class_init (EomThumbNavClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->constructor  = eoc_thumb_nav_constructor;
	g_object_class->get_property = eoc_thumb_nav_get_property;
	g_object_class->set_property = eoc_thumb_nav_set_property;

	g_object_class_install_property (g_object_class,
	                                 PROP_SHOW_BUTTONS,
	                                 g_param_spec_boolean ("show-buttons",
	                                                       "Show Buttons",
	                                                       "Whether to show navigation buttons or not",
	                                                       TRUE,
	                                                       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property (g_object_class,
	                                 PROP_THUMB_VIEW,
	                                 g_param_spec_object ("thumbview",
	                                                       "Thumbnail View",
	                                                       "The internal thumbnail viewer widget",
	                                                       EOC_TYPE_THUMB_VIEW,
	                                                       (G_PARAM_CONSTRUCT_ONLY |
								G_PARAM_READABLE |
								G_PARAM_WRITABLE)));

	g_object_class_install_property (g_object_class,
	                                 PROP_MODE,
	                                 g_param_spec_int ("mode",
	                                                   "Mode",
	                                                   "Thumb navigator mode",
	                                                   EOC_THUMB_NAV_MODE_ONE_ROW,
							   EOC_THUMB_NAV_MODE_MULTIPLE_ROWS,
							   EOC_THUMB_NAV_MODE_ONE_ROW,
	                                                   (G_PARAM_READABLE | G_PARAM_WRITABLE)));
}

static void
eoc_thumb_nav_init (EomThumbNav *nav)
{
	EomThumbNavPrivate *priv;
	GtkWidget *arrow;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (nav),
					GTK_ORIENTATION_HORIZONTAL);

	nav->priv = eoc_thumb_nav_get_instance_private (nav);

	priv = nav->priv;

	priv->mode = EOC_THUMB_NAV_MODE_ONE_ROW;

	priv->show_buttons = TRUE;

	priv->button_left = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button_left), GTK_RELIEF_NONE);

	arrow = gtk_image_new_from_icon_name ("pan-start-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_container_add (GTK_CONTAINER (priv->button_left), arrow);

	gtk_widget_set_size_request (GTK_WIDGET (priv->button_left), 25, 0);

	gtk_box_pack_start (GTK_BOX (nav), priv->button_left, FALSE, FALSE, 0);

	g_signal_connect (priv->button_left,
			  "clicked",
			  G_CALLBACK (eoc_thumb_nav_button_clicked),
			  nav);

	g_signal_connect (priv->button_left,
			  "pressed",
			  G_CALLBACK (eoc_thumb_nav_start_scroll),
			  nav);

	g_signal_connect (priv->button_left,
			  "released",
			  G_CALLBACK (eoc_thumb_nav_stop_scroll),
			  nav);

	priv->sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_widget_set_name (gtk_scrolled_window_get_hscrollbar (GTK_SCROLLED_WINDOW (priv->sw)), "eoc-image-collection-scrollbar");

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->sw),
					     GTK_SHADOW_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_NEVER);

	g_signal_connect (priv->sw,
			  "scroll-event",
			  G_CALLBACK (eoc_thumb_nav_scroll_event),
			  nav);

	priv->adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->sw));

	g_signal_connect (priv->adj,
			  "changed",
			  G_CALLBACK (eoc_thumb_nav_adj_changed),
			  nav);

	g_signal_connect (priv->adj,
			  "value-changed",
			  G_CALLBACK (eoc_thumb_nav_adj_value_changed),
			  nav);

	gtk_box_pack_start (GTK_BOX (nav), priv->sw, TRUE, TRUE, 0);

	priv->button_right = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button_right), GTK_RELIEF_NONE);

	arrow = gtk_image_new_from_icon_name ("pan-end-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_container_add (GTK_CONTAINER (priv->button_right), arrow);

	gtk_widget_set_size_request (GTK_WIDGET (priv->button_right), 25, 0);

	gtk_box_pack_start (GTK_BOX (nav), priv->button_right, FALSE, FALSE, 0);

	g_signal_connect (priv->button_right,
			  "clicked",
			  G_CALLBACK (eoc_thumb_nav_button_clicked),
			  nav);

	g_signal_connect (priv->button_right,
			  "pressed",
			  G_CALLBACK (eoc_thumb_nav_start_scroll),
			  nav);

	g_signal_connect (priv->button_right,
			  "released",
			  G_CALLBACK (eoc_thumb_nav_stop_scroll),
			  nav);

	/* Update nav button states */
	eoc_thumb_nav_adj_value_changed (priv->adj, nav);
}

/**
 * eoc_thumb_nav_new:
 * @thumbview: an #EomThumbView to embed in the navigation widget.
 * @mode: The navigation mode.
 * @show_buttons: Whether to show the navigation buttons.
 *
 * Creates a new thumbnail navigation widget.
 *
 * Returns: a new #EomThumbNav object.
 **/
GtkWidget *
eoc_thumb_nav_new (GtkWidget       *thumbview,
		   EomThumbNavMode  mode,
		   gboolean         show_buttons)
{
	GObject *nav;

	nav = g_object_new (EOC_TYPE_THUMB_NAV,
		            "show-buttons", show_buttons,
		            "mode", mode,
		            "thumbview", thumbview,
		            "homogeneous", FALSE,
		            "spacing", 0,
			    NULL);

	return GTK_WIDGET (nav);
}

/**
 * eoc_thumb_nav_get_show_buttons:
 * @nav: an #EomThumbNav.
 *
 * Gets whether the navigation buttons are visible.
 *
 * Returns: %TRUE if the navigation buttons are visible,
 * %FALSE otherwise.
 **/
gboolean
eoc_thumb_nav_get_show_buttons (EomThumbNav *nav)
{
	g_return_val_if_fail (EOC_IS_THUMB_NAV (nav), FALSE);

	return nav->priv->show_buttons;
}

/**
 * eoc_thumb_nav_set_show_buttons:
 * @nav: an #EomThumbNav.
 * @show_buttons: %TRUE to show the buttons, %FALSE to hide them.
 *
 * Sets whether the navigation buttons to the left and right of the
 * widget should be visible.
 **/
void
eoc_thumb_nav_set_show_buttons (EomThumbNav *nav, gboolean show_buttons)
{
	g_return_if_fail (EOC_IS_THUMB_NAV (nav));
	g_return_if_fail (nav->priv->button_left  != NULL);
	g_return_if_fail (nav->priv->button_right != NULL);

	nav->priv->show_buttons = show_buttons;

	if (show_buttons &&
	    nav->priv->mode == EOC_THUMB_NAV_MODE_ONE_ROW) {
		gtk_widget_show_all (nav->priv->button_left);
		gtk_widget_show_all (nav->priv->button_right);
	} else {
		gtk_widget_hide (nav->priv->button_left);
		gtk_widget_hide (nav->priv->button_right);
	}
}

/**
 * eoc_thumb_nav_get_mode:
 * @nav: an #EomThumbNav.
 *
 * Gets the navigation mode in @nav.
 *
 * Returns: A value in #EomThumbNavMode.
 **/
EomThumbNavMode
eoc_thumb_nav_get_mode (EomThumbNav *nav)
{
	g_return_val_if_fail (EOC_IS_THUMB_NAV (nav), FALSE);

	return nav->priv->mode;
}

/**
 * eoc_thumb_nav_set_mode:
 * @nav: An #EomThumbNav.
 * @mode: One of #EomThumbNavMode.
 *
 * Sets the navigation mode in @nav. See #EomThumbNavMode for details.
 **/
void
eoc_thumb_nav_set_mode (EomThumbNav *nav, EomThumbNavMode mode)
{
	EomThumbNavPrivate *priv;

	g_return_if_fail (EOC_IS_THUMB_NAV (nav));

	priv = nav->priv;

	priv->mode = mode;

	switch (mode)
	{
	case EOC_THUMB_NAV_MODE_ONE_ROW:
		gtk_orientable_set_orientation (GTK_ORIENTABLE(priv->thumbview),
		                                GTK_ORIENTATION_HORIZONTAL);

		gtk_widget_set_size_request (priv->thumbview, -1, -1);
		eoc_thumb_view_set_item_height (EOC_THUMB_VIEW (priv->thumbview),
						115);

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_NEVER);

		eoc_thumb_nav_set_show_buttons (nav, priv->show_buttons);

		break;

	case EOC_THUMB_NAV_MODE_ONE_COLUMN:
		gtk_orientable_set_orientation (GTK_ORIENTABLE(priv->thumbview),
		                                GTK_ORIENTATION_VERTICAL);

		gtk_widget_set_size_request (priv->thumbview, -1, -1);
		eoc_thumb_view_set_item_height (EOC_THUMB_VIEW (priv->thumbview),
						-1);

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
						GTK_POLICY_NEVER,
						GTK_POLICY_AUTOMATIC);

		gtk_widget_hide (priv->button_left);
		gtk_widget_hide (priv->button_right);

		break;

	case EOC_THUMB_NAV_MODE_MULTIPLE_ROWS:
		gtk_orientable_set_orientation (GTK_ORIENTABLE(priv->thumbview),
		                                GTK_ORIENTATION_VERTICAL);

		gtk_widget_set_size_request (priv->thumbview, -1, 220);
		eoc_thumb_view_set_item_height (EOC_THUMB_VIEW (priv->thumbview),
						-1);

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
						GTK_POLICY_NEVER,
						GTK_POLICY_AUTOMATIC);

		gtk_widget_hide (priv->button_left);
		gtk_widget_hide (priv->button_right);

		break;

	case EOC_THUMB_NAV_MODE_MULTIPLE_COLUMNS:
		gtk_orientable_set_orientation (GTK_ORIENTABLE(priv->thumbview),
		                                GTK_ORIENTATION_VERTICAL);

		gtk_widget_set_size_request (priv->thumbview, 230, -1);
		eoc_thumb_view_set_item_height (EOC_THUMB_VIEW (priv->thumbview),
						-1);

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
						GTK_POLICY_NEVER,
						GTK_POLICY_AUTOMATIC);

		gtk_widget_hide (priv->button_left);
		gtk_widget_hide (priv->button_right);

		break;
	}
}
