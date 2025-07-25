#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eoc-fullscreen-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>
#include <libbean/bean-activatable.h>

#include <eoc-debug.h>
#include <eoc-window.h>
#include <eoc-window-activatable.h>

static void eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EocFullscreenPlugin,
                                eoc_fullscreen_plugin,
                                BEAN_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (EOC_TYPE_WINDOW_ACTIVATABLE,
                                                               eoc_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static gboolean
on_button_press (CtkWidget      *button G_GNUC_UNUSED,
		 CdkEventButton *event,
		 EocWindow      *window)
{
	if (event->button == 1 && event->type == CDK_2BUTTON_PRESS)
	{
		EocWindowMode mode = eoc_window_get_mode (window);

		if (mode == EOC_WINDOW_MODE_SLIDESHOW || mode == EOC_WINDOW_MODE_FULLSCREEN)
		{
			eoc_window_set_mode (window, EOC_WINDOW_MODE_NORMAL);
		}
		else if (mode == EOC_WINDOW_MODE_NORMAL)
		{
			eoc_window_set_mode (window, EOC_WINDOW_MODE_FULLSCREEN);
		}

		return TRUE;
	}

	return FALSE;
}

static void
eoc_fullscreen_plugin_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	EocFullscreenPlugin *plugin = EOC_FULLSCREEN_PLUGIN (object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		plugin->window = EOC_WINDOW (g_value_dup_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eoc_fullscreen_plugin_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	EocFullscreenPlugin *plugin = EOC_FULLSCREEN_PLUGIN (object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		g_value_set_object (value, plugin->window);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eoc_fullscreen_plugin_init (EocFullscreenPlugin *plugin G_GNUC_UNUSED)
{
	eoc_debug_message (DEBUG_PLUGINS, "EocFullscreenPlugin initializing");
}

static void
eoc_fullscreen_plugin_dispose (GObject *object)
{
	EocFullscreenPlugin *plugin = EOC_FULLSCREEN_PLUGIN (object);

	eoc_debug_message (DEBUG_PLUGINS, "EocFullscreenPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eoc_fullscreen_plugin_parent_class)->dispose (object);
}

static void
eoc_fullscreen_plugin_activate (EocWindowActivatable *activatable)
{
	EocFullscreenPlugin *plugin = EOC_FULLSCREEN_PLUGIN (activatable);
	CtkWidget *view = eoc_window_get_view (plugin->window);

	eoc_debug (DEBUG_PLUGINS);

	plugin->signal_id = g_signal_connect (G_OBJECT (view),
	                                      "button-press-event",
	                                      G_CALLBACK (on_button_press),
	                                      plugin->window);
}

static void
eoc_fullscreen_plugin_deactivate (EocWindowActivatable *activatable)
{
	EocFullscreenPlugin *plugin = EOC_FULLSCREEN_PLUGIN (activatable);
	CtkWidget *view = eoc_window_get_view (plugin->window);

	g_signal_handler_disconnect (view, plugin->signal_id);
}

static void
eoc_fullscreen_plugin_class_init (EocFullscreenPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eoc_fullscreen_plugin_dispose;
	object_class->set_property = eoc_fullscreen_plugin_set_property;
	object_class->get_property = eoc_fullscreen_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eoc_fullscreen_plugin_class_finalize (EocFullscreenPluginClass *klass G_GNUC_UNUSED)
{
	/* dummy function - used by G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface)
{
	iface->activate = eoc_fullscreen_plugin_activate;
	iface->deactivate = eoc_fullscreen_plugin_deactivate;
}

G_MODULE_EXPORT void
bean_register_types (BeanObjectModule *module)
{
	eoc_fullscreen_plugin_register_type (G_TYPE_MODULE (module));
	bean_object_module_register_extension_type (module,
	                                            EOC_TYPE_WINDOW_ACTIVATABLE,
	                                            EOC_TYPE_FULLSCREEN_PLUGIN);
}
