/* Eye of Cafe - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.h) by:
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

#ifndef __EOC_SIDEBAR_H__
#define __EOC_SIDEBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EocSidebar EocSidebar;
typedef struct _EocSidebarClass EocSidebarClass;
typedef struct _EocSidebarPrivate EocSidebarPrivate;

#define EOC_TYPE_SIDEBAR	    (eoc_sidebar_get_type())
#define EOC_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_SIDEBAR, EocSidebar))
#define EOC_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_SIDEBAR, EocSidebarClass))
#define EOC_IS_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_SIDEBAR))
#define EOC_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_SIDEBAR))
#define EOC_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_SIDEBAR, EocSidebarClass))

struct _EocSidebar {
	GtkBox base_instance;

	EocSidebarPrivate *priv;
};

struct _EocSidebarClass {
	GtkBoxClass base_class;

	void (* page_added)   (EocSidebar *eoc_sidebar,
			       GtkWidget  *main_widget);

	void (* page_removed) (EocSidebar *eoc_sidebar,
			       GtkWidget  *main_widget);
};

GType      eoc_sidebar_get_type     (void);

GtkWidget *eoc_sidebar_new          (void);

void       eoc_sidebar_add_page     (EocSidebar  *eoc_sidebar,
				     const gchar *title,
				     GtkWidget   *main_widget);

void       eoc_sidebar_remove_page  (EocSidebar  *eoc_sidebar,
				     GtkWidget   *main_widget);

void       eoc_sidebar_set_page     (EocSidebar  *eoc_sidebar,
				     GtkWidget   *main_widget);

gint       eoc_sidebar_get_n_pages  (EocSidebar  *eoc_sidebar);

gboolean   eoc_sidebar_is_empty     (EocSidebar  *eoc_sidebar);

G_END_DECLS

#endif /* __EOC_SIDEBAR_H__ */


