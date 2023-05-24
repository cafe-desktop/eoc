/* Eye Of Cafe - EOC Plugin Manager
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@gnome.org>
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
#include <config.h>
#endif

#include "eoc-plugin-engine.h"
#include "eoc-debug.h"
#include "eoc-config-keys.h"
#include "eoc-util.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>
#include <girepository.h>

#define USER_EOC_PLUGINS_LOCATION "plugins/"

struct _EocPluginEnginePrivate {
	GSettings *plugins_settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (EocPluginEngine, eoc_plugin_engine, PEAS_TYPE_ENGINE)

static void
eoc_plugin_engine_dispose (GObject *object)
{
	EocPluginEngine *engine = EOC_PLUGIN_ENGINE (object);

	if (engine->priv->plugins_settings != NULL)
	{
		g_object_unref (engine->priv->plugins_settings);
		engine->priv->plugins_settings = NULL;
	}

	G_OBJECT_CLASS (eoc_plugin_engine_parent_class)->dispose (object);
}

static void
eoc_plugin_engine_class_init (EocPluginEngineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eoc_plugin_engine_dispose;
}

static void
eoc_plugin_engine_init (EocPluginEngine *engine)
{
	eoc_debug (DEBUG_PLUGINS);

	engine->priv = eoc_plugin_engine_get_instance_private (engine);

	engine->priv->plugins_settings = g_settings_new (EOC_CONF_PLUGINS);
}

EocPluginEngine *
eoc_plugin_engine_new (void)
{
	EocPluginEngine *engine;
	gchar *user_plugin_path;
	gchar *private_path;
	GError *error = NULL;

	private_path = g_build_filename (LIBDIR, "girepository-1.0", NULL);

	/* This should be moved to libbean */
	if (g_irepository_require (g_irepository_get_default (),
	                           "Bean", "1.0", 0, &error) == NULL)
	{
		g_warning ("Error loading Bean typelib: %s\n",
		           error->message);
		g_clear_error (&error);
	}

	if (g_irepository_require (g_irepository_get_default (),
	                           "BeanCtk", "1.0", 0, &error) == NULL)
	{
		g_warning ("Error loading BeanCtk typelib: %s\n",
		           error->message);
		g_clear_error (&error);
	}

	if (g_irepository_require_private (g_irepository_get_default (),
	                                   private_path, "Eoc", "1.0", 0,
	                                   &error) == NULL)
	{
		g_warning ("Error loading Eoc typelib: %s\n",
		           error->message);
		g_clear_error (&error);
	}

	g_free (private_path);

	engine = EOC_PLUGIN_ENGINE (g_object_new (EOC_TYPE_PLUGIN_ENGINE, NULL));

	bean_engine_enable_loader (PEAS_ENGINE (engine), "python3");

	user_plugin_path = g_build_filename (eoc_util_dot_dir (),
	                                     USER_EOC_PLUGINS_LOCATION, NULL);

	bean_engine_add_search_path (PEAS_ENGINE (engine),
	                             user_plugin_path, user_plugin_path);

	bean_engine_add_search_path (PEAS_ENGINE (engine),
	                             EOC_PLUGIN_DIR, EOC_PLUGIN_DIR);

	g_settings_bind (engine->priv->plugins_settings,
	                 EOC_CONF_PLUGINS_ACTIVE_PLUGINS,
	                 engine,
	                 "loaded-plugins",
	                 G_SETTINGS_BIND_DEFAULT);

	g_free (user_plugin_path);

	return engine;
}
