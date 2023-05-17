/* Eye of Mate - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.c) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "eoc-sidebar.h"

enum {
	PROP_0,
	PROP_CURRENT_PAGE
};

enum {
	PAGE_COLUMN_TITLE,
	PAGE_COLUMN_MENU_ITEM,
	PAGE_COLUMN_MAIN_WIDGET,
	PAGE_COLUMN_NOTEBOOK_INDEX,
	PAGE_COLUMN_NUM_COLS
};

enum {
	SIGNAL_PAGE_ADDED,
	SIGNAL_PAGE_REMOVED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

struct _EocSidebarPrivate {
	GtkWidget *notebook;
	GtkWidget *select_button;
	GtkWidget *menu;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *arrow;

	GtkTreeModel *page_model;
};

G_DEFINE_TYPE_WITH_PRIVATE (EocSidebar, eoc_sidebar, GTK_TYPE_BOX)

static void
eoc_sidebar_destroy (GtkWidget *object)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (object);

	if (eoc_sidebar->priv->menu) {
		gtk_menu_detach (GTK_MENU (eoc_sidebar->priv->menu));
		eoc_sidebar->priv->menu = NULL;
	}

	if (eoc_sidebar->priv->page_model) {
		g_object_unref (eoc_sidebar->priv->page_model);
		eoc_sidebar->priv->page_model = NULL;
	}

	(* GTK_WIDGET_CLASS (eoc_sidebar_parent_class)->destroy) (object);
}

static void
eoc_sidebar_select_page (EocSidebar *eoc_sidebar, GtkTreeIter *iter)
{
	gchar *title;
	gint index;

	gtk_tree_model_get (eoc_sidebar->priv->page_model, iter,
			    PAGE_COLUMN_TITLE, &title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (eoc_sidebar->priv->notebook), index);
	gtk_label_set_text (GTK_LABEL (eoc_sidebar->priv->label), title);

	g_free (title);
}

void
eoc_sidebar_set_page (EocSidebar   *eoc_sidebar,
		     GtkWidget   *main_widget)
{
	GtkTreeIter iter;
	gboolean valid;

	valid = gtk_tree_model_get_iter_first (eoc_sidebar->priv->page_model, &iter);

	while (valid) {
		GtkWidget *widget;

		gtk_tree_model_get (eoc_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			eoc_sidebar_select_page (eoc_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (eoc_sidebar->priv->page_model, &iter);
		}

		g_object_unref (widget);
	}

	g_object_notify (G_OBJECT (eoc_sidebar), "current-page");
}

static GtkWidget *
eoc_sidebar_get_current_page (EocSidebar *sidebar)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (sidebar->priv->notebook);

	return gtk_notebook_get_nth_page
		(notebook, gtk_notebook_get_current_page (notebook));
}

static void
eoc_sidebar_set_property (GObject     *object,
		         guint         prop_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	EocSidebar *sidebar = EOC_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		eoc_sidebar_set_page (sidebar, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eoc_sidebar_get_property (GObject    *object,
		          guint       prop_id,
		          GValue     *value,
		          GParamSpec *pspec)
{
	EocSidebar *sidebar = EOC_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		g_value_set_object (value, eoc_sidebar_get_current_page (sidebar));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eoc_sidebar_class_init (EocSidebarClass *eoc_sidebar_class)
{
	GObjectClass *g_object_class;
	GtkWidgetClass *widget_class;

	g_object_class = G_OBJECT_CLASS (eoc_sidebar_class);
	widget_class = GTK_WIDGET_CLASS (eoc_sidebar_class);

	widget_class->destroy = eoc_sidebar_destroy;
	g_object_class->get_property = eoc_sidebar_get_property;
	g_object_class->set_property = eoc_sidebar_set_property;

	g_object_class_install_property (g_object_class,
					 PROP_CURRENT_PAGE,
					 g_param_spec_object ("current-page",
							      "Current page",
							      "The currently visible page",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE));

	signals[SIGNAL_PAGE_ADDED] =
		g_signal_new ("page-added",
			      EOC_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EocSidebarClass, page_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);

	signals[SIGNAL_PAGE_REMOVED] =
		g_signal_new ("page-removed",
			      EOC_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EocSidebarClass, page_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);
}

static gboolean
eoc_sidebar_select_button_press_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (user_data);

	if (event->button == 1) {
		GtkRequisition requisition;
		GtkAllocation allocation;

		gtk_widget_get_allocation (widget, &allocation);

		gtk_widget_set_size_request (eoc_sidebar->priv->menu, -1, -1);
		gtk_widget_get_preferred_size (eoc_sidebar->priv->menu, &requisition, NULL);
		gtk_widget_set_size_request (eoc_sidebar->priv->menu,
					     MAX (allocation.width,
						  requisition.width), -1);

		gtk_widget_grab_focus (widget);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup_at_widget (GTK_MENU (eoc_sidebar->priv->menu),
		                          widget,
		                          GDK_GRAVITY_SOUTH_WEST,
		                          GDK_GRAVITY_NORTH_WEST,
		                          (const GdkEvent*) event);

		return TRUE;
	}

	return FALSE;
}

static gboolean
eoc_sidebar_select_button_key_press_cb (GtkWidget   *widget,
				        GdkEventKey *event,
				        gpointer     user_data)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (user_data);

	if (event->keyval == GDK_KEY_space ||
	    event->keyval == GDK_KEY_KP_Space ||
	    event->keyval == GDK_KEY_Return ||
	    event->keyval == GDK_KEY_KP_Enter) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup_at_widget (GTK_MENU (eoc_sidebar->priv->menu),
		                          widget,
		                          GDK_GRAVITY_SOUTH_WEST,
		                          GDK_GRAVITY_NORTH_WEST,
		                          (const GdkEvent*) event);

		return TRUE;
	}

	return FALSE;
}

static void
eoc_sidebar_close_clicked_cb (GtkWidget *widget,
 			      gpointer   user_data)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (user_data);

	gtk_widget_hide (GTK_WIDGET (eoc_sidebar));
}

static void
eoc_sidebar_menu_deactivate_cb (GtkWidget *widget,
			       gpointer   user_data)
{
	GtkWidget *menu_button;

	menu_button = GTK_WIDGET (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menu_button), FALSE);
}

static void
eoc_sidebar_menu_detach_cb (GtkWidget *widget,
			   GtkMenu   *menu)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (widget);

	eoc_sidebar->priv->menu = NULL;
}

static void
eoc_sidebar_menu_item_activate_cb (GtkWidget *widget,
				   gpointer   user_data)
{
	EocSidebar *eoc_sidebar = EOC_SIDEBAR (user_data);
	GtkTreeIter iter;
	GtkWidget *menu_item, *item;
	gboolean valid;

	menu_item = gtk_menu_get_active (GTK_MENU (eoc_sidebar->priv->menu));
	valid = gtk_tree_model_get_iter_first (eoc_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (eoc_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MENU_ITEM, &item,
				    -1);

		if (item == menu_item) {
			eoc_sidebar_select_page (eoc_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (eoc_sidebar->priv->page_model, &iter);
		}

		g_object_unref (item);
	}

	g_object_notify (G_OBJECT (eoc_sidebar), "current-page");
}

static void
eoc_sidebar_update_arrow_visibility (EocSidebar *sidebar)
{
	EocSidebarPrivate *priv = sidebar->priv;
	const gint n_pages = eoc_sidebar_get_n_pages (sidebar);

	gtk_widget_set_visible (GTK_WIDGET (priv->arrow),
				n_pages > 1);
}

static void
eoc_sidebar_init (EocSidebar *eoc_sidebar)
{
	GtkWidget *hbox;
	GtkWidget *close_button;
	GtkWidget *select_hbox;
	GtkWidget *arrow;
	GtkWidget *image;

	eoc_sidebar->priv = eoc_sidebar_get_instance_private (eoc_sidebar);

	gtk_style_context_add_class (
	        gtk_widget_get_style_context (GTK_WIDGET (eoc_sidebar)),
	                                      GTK_STYLE_CLASS_SIDEBAR);

	/* data model */
	eoc_sidebar->priv->page_model = (GtkTreeModel *)
			gtk_list_store_new (PAGE_COLUMN_NUM_COLS,
					    G_TYPE_STRING,
					    GTK_TYPE_WIDGET,
					    GTK_TYPE_WIDGET,
					    G_TYPE_INT);

	/* top option menu */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	g_object_set (hbox, "border-width", 6, NULL);
	eoc_sidebar->priv->hbox = hbox;
	gtk_box_pack_start (GTK_BOX (eoc_sidebar), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	eoc_sidebar->priv->select_button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (eoc_sidebar->priv->select_button),
			       GTK_RELIEF_NONE);

	g_signal_connect (eoc_sidebar->priv->select_button, "button_press_event",
			  G_CALLBACK (eoc_sidebar_select_button_press_cb),
			  eoc_sidebar);

	g_signal_connect (eoc_sidebar->priv->select_button, "key_press_event",
			  G_CALLBACK (eoc_sidebar_select_button_key_press_cb),
			  eoc_sidebar);

	select_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	eoc_sidebar->priv->label = gtk_label_new ("");
	gtk_widget_set_name (eoc_sidebar->priv->label, "eoc-sidebar-title");

	gtk_box_pack_start (GTK_BOX (select_hbox),
			    eoc_sidebar->priv->label,
			    FALSE, FALSE, 0);

	gtk_widget_show (eoc_sidebar->priv->label);

	arrow = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_end (GTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);
	eoc_sidebar->priv->arrow = arrow;
	gtk_widget_set_visible (arrow, FALSE);

	gtk_container_add (GTK_CONTAINER (eoc_sidebar->priv->select_button), select_hbox);
	gtk_widget_show (select_hbox);

	gtk_box_set_center_widget (GTK_BOX (hbox), eoc_sidebar->priv->select_button);
	gtk_widget_show (eoc_sidebar->priv->select_button);

	close_button = gtk_button_new ();

	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);

	g_signal_connect (close_button, "clicked",
			  G_CALLBACK (eoc_sidebar_close_clicked_cb),
			  eoc_sidebar);

	image = gtk_image_new_from_icon_name ("window-close",
					  GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_widget_show (image);

	gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show (close_button);

	eoc_sidebar->priv->menu = gtk_menu_new ();

	g_signal_connect (eoc_sidebar->priv->menu, "deactivate",
			  G_CALLBACK (eoc_sidebar_menu_deactivate_cb),
			  eoc_sidebar->priv->select_button);

	gtk_menu_attach_to_widget (GTK_MENU (eoc_sidebar->priv->menu),
				   GTK_WIDGET (eoc_sidebar),
				   eoc_sidebar_menu_detach_cb);

	gtk_widget_show (eoc_sidebar->priv->menu);

	eoc_sidebar->priv->notebook = gtk_notebook_new ();

	gtk_notebook_set_show_border (GTK_NOTEBOOK (eoc_sidebar->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (eoc_sidebar->priv->notebook), FALSE);

	gtk_box_pack_start (GTK_BOX (eoc_sidebar), eoc_sidebar->priv->notebook,
			    TRUE, TRUE, 0);

	gtk_widget_show (eoc_sidebar->priv->notebook);
}

GtkWidget *
eoc_sidebar_new (void)
{
	GtkWidget *eoc_sidebar;

	eoc_sidebar = g_object_new (EOC_TYPE_SIDEBAR,
	                            "orientation", GTK_ORIENTATION_VERTICAL,
	                            NULL);

	return eoc_sidebar;
}

void
eoc_sidebar_add_page (EocSidebar   *eoc_sidebar,
		      const gchar  *title,
		      GtkWidget    *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *menu_item;
	gchar *label_title;
	gint index;

	g_return_if_fail (EOC_IS_SIDEBAR (eoc_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	index = gtk_notebook_append_page (GTK_NOTEBOOK (eoc_sidebar->priv->notebook),
					  main_widget, NULL);

	menu_item = gtk_menu_item_new_with_label (title);

	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (eoc_sidebar_menu_item_activate_cb),
			  eoc_sidebar);

	gtk_widget_show (menu_item);

	gtk_menu_shell_append (GTK_MENU_SHELL (eoc_sidebar->priv->menu),
			       menu_item);

	/* Insert and move to end */
	gtk_list_store_insert_with_values (GTK_LIST_STORE (eoc_sidebar->priv->page_model),
					   &iter, 0,
					   PAGE_COLUMN_TITLE, title,
					   PAGE_COLUMN_MENU_ITEM, menu_item,
					   PAGE_COLUMN_MAIN_WIDGET, main_widget,
					   PAGE_COLUMN_NOTEBOOK_INDEX, index,
					   -1);

	gtk_list_store_move_before (GTK_LIST_STORE(eoc_sidebar->priv->page_model),
				    &iter,
				    NULL);

	/* Set the first item added as active */
	gtk_tree_model_get_iter_first (eoc_sidebar->priv->page_model, &iter);
	gtk_tree_model_get (eoc_sidebar->priv->page_model,
			    &iter,
			    PAGE_COLUMN_TITLE, &label_title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_menu_set_active (GTK_MENU (eoc_sidebar->priv->menu), index);

	gtk_label_set_text (GTK_LABEL (eoc_sidebar->priv->label), label_title);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (eoc_sidebar->priv->notebook),
				       index);

	g_free (label_title);

	eoc_sidebar_update_arrow_visibility (eoc_sidebar);

	g_signal_emit (G_OBJECT (eoc_sidebar),
		       signals[SIGNAL_PAGE_ADDED], 0, main_widget);
}

void
eoc_sidebar_remove_page (EocSidebar *eoc_sidebar, GtkWidget *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *widget, *menu_item;
	gboolean valid;
	gint index;

	g_return_if_fail (EOC_IS_SIDEBAR (eoc_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	valid = gtk_tree_model_get_iter_first (eoc_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (eoc_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
				    PAGE_COLUMN_MENU_ITEM, &menu_item,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			break;
		} else {
			valid = gtk_tree_model_iter_next (eoc_sidebar->priv->page_model,
							  &iter);
		}

		g_object_unref (menu_item);
		g_object_unref (widget);
	}

	if (valid) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (eoc_sidebar->priv->notebook),
					  index);

		gtk_container_remove (GTK_CONTAINER (eoc_sidebar->priv->menu), menu_item);

		gtk_list_store_remove (GTK_LIST_STORE (eoc_sidebar->priv->page_model),
				       &iter);

		eoc_sidebar_update_arrow_visibility (eoc_sidebar);

		g_signal_emit (G_OBJECT (eoc_sidebar),
			       signals[SIGNAL_PAGE_REMOVED], 0, main_widget);
	}
}

gint
eoc_sidebar_get_n_pages (EocSidebar *eoc_sidebar)
{
	g_return_val_if_fail (EOC_IS_SIDEBAR (eoc_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (eoc_sidebar->priv->page_model), NULL);
}

gboolean
eoc_sidebar_is_empty (EocSidebar *eoc_sidebar)
{
	g_return_val_if_fail (EOC_IS_SIDEBAR (eoc_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (eoc_sidebar->priv->page_model), NULL) == 0;
}
