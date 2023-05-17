#ifndef __EOC_FULLSCREEN_PLUGIN_H__
#define __EOC_FULLSCREEN_PLUGIN_H__

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
#define EOC_TYPE_FULLSCREEN_PLUGIN \
	(eoc_fullscreen_plugin_get_type())
#define EOC_FULLSCREEN_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOC_TYPE_FULLSCREEN_PLUGIN, EomFullscreenPlugin))
#define EOC_FULLSCREEN_PLUGIN_CLASS(k) \
	G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_FULLSCREEN_PLUGIN, EomFullscreenPluginClass))
#define EOC_IS_FULLSCREEN_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOC_TYPE_FULLSCREEN_PLUGIN))
#define EOC_IS_FULLSCREEN_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOC_TYPE_FULLSCREEN_PLUGIN))
#define EOC_FULLSCREEN_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOC_TYPE_FULLSCREEN_PLUGIN, EomFullscreenPluginClass))

/* Private structure type */
typedef struct _EomFullscreenPluginPrivate EomFullscreenPluginPrivate;

/*
 * Main object structure
 */
typedef struct _EomFullscreenPlugin EomFullscreenPlugin;

struct _EomFullscreenPlugin {
	PeasExtensionBase parent_instance;

	EomWindow *window;
	gulong signal_id;
};

/*
 * Class definition
 */
typedef struct _EomFullscreenPluginClass EomFullscreenPluginClass;

struct _EomFullscreenPluginClass {
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType eoc_fullscreen_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __EOC_FULLSCREEN_PLUGIN_H__ */
