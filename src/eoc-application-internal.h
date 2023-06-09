/* Eye Of Gnome - Application Facade (internal)
 *
 * Copyright (C) 2006-2012 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-application.h) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __EOC_APPLICATION_INTERNAL_H__
#define __EOC_APPLICATION_INTERNAL_H__

#include <glib.h>
#include <glib-object.h>

#include <libbean/bean-extension-set.h>

#include "eoc-application.h"
#include "eoc-plugin-engine.h"
#include "egg-toolbars-model.h"
#include "eoc-window.h"

G_BEGIN_DECLS

struct _EocApplicationPrivate {
	EggToolbarsModel *toolbars_model;
	gchar            *toolbars_file;
	EocPluginEngine  *plugin_engine;

	EocStartupFlags   flags;

	BeanExtensionSet *extensions;
};


EggToolbarsModel *eoc_application_get_toolbars_model  (EocApplication *application);

void              eoc_application_save_toolbars_model (EocApplication *application);

void              eoc_application_reset_toolbars_model (EocApplication *app);

G_END_DECLS

#endif /* __EOC_APPLICATION_INTERNAL_H__ */

