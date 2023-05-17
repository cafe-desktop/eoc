/*
 * eoc-metadata-sidebar.h
 * This file is part of eoc
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2011 GNOME Foundation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef EOC_METADATA_SIDEBAR_H
#define EOC_METADATA_SIDEBAR_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "eoc-window.h"

G_BEGIN_DECLS

#define EOC_TYPE_METADATA_SIDEBAR          (eoc_metadata_sidebar_get_type ())
#define EOC_METADATA_SIDEBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_METADATA_SIDEBAR, EocMetadataSidebar))
#define EOC_METADATA_SIDEBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_METADATA_SIDEBAR, EocMetadataSidebarClass))
#define EOC_IS_METADATA_SIDEBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_METADATA_SIDEBAR))
#define EOC_IS_METADATA_SIDEBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_METADATA_SIDEBAR))
#define EOC_METADATA_SIDEBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_METADATA_SIDEBAR, EocMetadataSidebarClass))

typedef struct _EocMetadataSidebar EocMetadataSidebar;
typedef struct _EocMetadataSidebarClass EocMetadataSidebarClass;
typedef struct _EocMetadataSidebarPrivate EocMetadataSidebarPrivate;

struct _EocMetadataSidebar {
	GtkScrolledWindow parent;

	EocMetadataSidebarPrivate *priv;
};

struct _EocMetadataSidebarClass {
	GtkScrolledWindowClass parent_klass;
};

GType eoc_metadata_sidebar_get_type (void) G_GNUC_CONST;

GtkWidget* eoc_metadata_sidebar_new (EocWindow *window);

G_END_DECLS

#endif /* EOC_METADATA_SIDEBAR_H */
