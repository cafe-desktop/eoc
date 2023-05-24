#ifndef __EOC_RELOAD_PLUGIN_H__
#define __EOC_RELOAD_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <ctk/ctk.h>
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
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOC_TYPE_RELOAD_PLUGIN, EocReloadPlugin))
#define EOC_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_RELOAD_PLUGIN, EocReloadPluginClass))
#define EOC_IS_RELOAD_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOC_TYPE_RELOAD_PLUGIN))
#define EOC_IS_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOC_TYPE_RELOAD_PLUGIN))
#define EOC_RELOAD_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOC_TYPE_RELOAD_PLUGIN, EocReloadPluginClass))

/* Private structure type */
typedef struct _EocReloadPluginPrivate EocReloadPluginPrivate;

/*
 * Main object structure
 */
typedef struct _EocReloadPlugin EocReloadPlugin;

struct _EocReloadPlugin {
	PeasExtensionBase parent_instance;

	EocWindow *window;
	CtkActionGroup *ui_action_group;
	guint ui_id;
};

/*
 * Class definition
 */
typedef struct _EocReloadPluginClass EocReloadPluginClass;

struct _EocReloadPluginClass {
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
