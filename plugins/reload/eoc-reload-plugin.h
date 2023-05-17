#ifndef __EOC_RELOAD_PLUGIN_H__
#define __EOC_RELOAD_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

#include <eoc-window.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define EOC_TYPE_RELOAD_PLUGIN \
	(eoc_reload_plugin_get_type())
#define EOC_RELOAD_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOC_TYPE_RELOAD_PLUGIN, EomReloadPlugin))
#define EOC_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_RELOAD_PLUGIN, EomReloadPluginClass))
#define EOC_IS_RELOAD_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOC_TYPE_RELOAD_PLUGIN))
#define EOC_IS_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOC_TYPE_RELOAD_PLUGIN))
#define EOC_RELOAD_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOC_TYPE_RELOAD_PLUGIN, EomReloadPluginClass))

/* Private structure type */
typedef struct _EomReloadPluginPrivate EomReloadPluginPrivate;

/*
 * Main object structure
 */
typedef struct _EomReloadPlugin EomReloadPlugin;

struct _EomReloadPlugin {
	PeasExtensionBase parent_instance;

	EomWindow *window;
	GtkActionGroup *ui_action_group;
	guint ui_id;
};

/*
 * Class definition
 */
typedef struct _EomReloadPluginClass EomReloadPluginClass;

struct _EomReloadPluginClass {
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType eoc_reload_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __EOC_RELOAD_PLUGIN_H__ */
