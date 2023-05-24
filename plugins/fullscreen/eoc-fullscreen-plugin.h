#ifndef __EOC_FULLSCREEN_PLUGIN_H__
#define __EOC_FULLSCREEN_PLUGIN_H__

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
#define EOC_TYPE_FULLSCREEN_PLUGIN \
	(eoc_fullscreen_plugin_get_type())
#define EOC_FULLSCREEN_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOC_TYPE_FULLSCREEN_PLUGIN, EocFullscreenPlugin))
#define EOC_FULLSCREEN_PLUGIN_CLASS(k) \
	G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_FULLSCREEN_PLUGIN, EocFullscreenPluginClass))
#define EOC_IS_FULLSCREEN_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOC_TYPE_FULLSCREEN_PLUGIN))
#define EOC_IS_FULLSCREEN_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOC_TYPE_FULLSCREEN_PLUGIN))
#define EOC_FULLSCREEN_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOC_TYPE_FULLSCREEN_PLUGIN, EocFullscreenPluginClass))

/* Private structure type */
typedef struct _EocFullscreenPluginPrivate EocFullscreenPluginPrivate;

/*
 * Main object structure
 */
typedef struct _EocFullscreenPlugin EocFullscreenPlugin;

struct _EocFullscreenPlugin {
	BeanExtensionBase parent_instance;

	EocWindow *window;
	gulong signal_id;
};

/*
 * Class definition
 */
typedef struct _EocFullscreenPluginClass EocFullscreenPluginClass;

struct _EocFullscreenPluginClass {
	BeanExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType eoc_fullscreen_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void bean_register_types (BeanObjectModule *module);

G_END_DECLS

#endif /* __EOC_FULLSCREEN_PLUGIN_H__ */
