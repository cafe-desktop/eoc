/* Eye Of Cafe - EOC Plugin Engine
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-plugins-engine.h) by:
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

#ifndef __EOC_PLUGIN_ENGINE_H__
#define __EOC_PLUGIN_ENGINE_H__

#include <glib.h>
#include <libpeas/peas-engine.h>

G_BEGIN_DECLS

typedef struct _EocPluginEngine EocPluginEngine;
typedef struct _EocPluginEngineClass EocPluginEngineClass;
typedef struct _EocPluginEnginePrivate EocPluginEnginePrivate;

#define EOC_TYPE_PLUGIN_ENGINE            eoc_plugin_engine_get_type()
#define EOC_PLUGIN_ENGINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_PLUGIN_ENGINE, EocPluginEngine))
#define EOC_PLUGIN_ENGINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_PLUGIN_ENGINE, EocPluginEngineClass))
#define EOC_IS_PLUGIN_ENGINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_PLUGIN_ENGINE))
#define EOC_IS_PLUGIN_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_PLUGIN_ENGINE))
#define EOC_PLUGIN_ENGINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_PLUGIN_ENGINE, EocPluginEngineClass))

struct _EocPluginEngine {
	PeasEngine parent;
	EocPluginEnginePrivate *priv;
};

struct _EocPluginEngineClass {
	PeasEngineClass parent_class;
};

GType eoc_plugin_engine_get_type (void) G_GNUC_CONST;

EocPluginEngine* eoc_plugin_engine_new (void);

G_END_DECLS

#endif  /* __EOC_PLUGIN_ENGINE_H__ */
