/* Statusbar Date -- Shows the EXIF date in EOC's statusbar
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra  <csaavedra@gnome.org>
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

#ifndef __EOC_STATUSBAR_DATE_PLUGIN_H__
#define __EOC_STATUSBAR_DATE_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <ctk/ctk.h>
#include <libbean/bean-extension-base.h>
#include <libbean/bean-object-module.h>

#include <eoc-window.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define EOC_TYPE_STATUSBAR_DATE_PLUGIN \
	(eoc_statusbar_date_plugin_get_type())
#define EOC_STATUSBAR_DATE_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOC_TYPE_STATUSBAR_DATE_PLUGIN, EocStatusbarDatePlugin))
#define EOC_STATUSBAR_DATE_PLUGIN_CLASS(k) \
	G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_STATUSBAR_DATE_PLUGIN, EocStatusbarDatePluginClass))
#define EOC_IS_STATUSBAR_DATE_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOC_TYPE_STATUSBAR_DATE_PLUGIN))
#define EOC_IS_STATUSBAR_DATE_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOC_TYPE_STATUSBAR_DATE_PLUGIN))
#define EOC_STATUSBAR_DATE_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOC_TYPE_STATUSBAR_DATE_PLUGIN, EocStatusbarDatePluginClass))

/* Private structure type */
typedef struct _EocStatusbarDatePluginPrivate EocStatusbarDatePluginPrivate;

/*
 * Main object structure
 */
typedef struct _EocStatusbarDatePlugin EocStatusbarDatePlugin;

struct _EocStatusbarDatePlugin {
	PeasExtensionBase parent_instance;

	EocWindow *window;
	CtkWidget *statusbar_date;
	gulong signal_id;
};

/*
 * Class definition
 */
typedef struct _EocStatusbarDatePluginClass	EocStatusbarDatePluginClass;

struct _EocStatusbarDatePluginClass {
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType eoc_statusbar_date_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void bean_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __EOC_STATUSBAR_DATE_PLUGIN_H__ */
