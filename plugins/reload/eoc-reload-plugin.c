#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eoc-reload-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>
#include <libpeas/peas-activatable.h>

#include <eoc-debug.h>
#include <eoc-window.h>
#include <eoc-window-activatable.h>

static void eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EocReloadPlugin,
                                eoc_reload_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (EOC_TYPE_WINDOW_ACTIVATABLE,
                                                               eoc_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static void
reload_cb (GtkAction *action,
           EocWindow *window)
{
	eoc_window_reload_image (window);
}

static const gchar* const ui_definition = "<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\"><separator/>"
	"<menuitem name=\"EocPluginReload\" action=\"EocPluginRunReload\"/>"
	"<separator/></menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
	"<menuitem action=\"EocPluginRunReload\"/><separator/>"
	"</popup></ui>";

static const GtkActionEntry action_entries[] = {
	{ "EocPluginRunReload", "view-refresh", N_("Reload Image"), "R", N_("Reload current image"), G_CALLBACK (reload_cb) }
};

static void
eoc_reload_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	EocReloadPlugin *plugin = EOC_RELOAD_PLUGIN (object);

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
eoc_reload_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	EocReloadPlugin *plugin = EOC_RELOAD_PLUGIN (object);

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
eoc_reload_plugin_init (EocReloadPlugin *plugin)
{
	eoc_debug_message (DEBUG_PLUGINS, "EocReloadPlugin initializing");
}

static void
eoc_reload_plugin_dispose (GObject *object)
{
	EocReloadPlugin *plugin = EOC_RELOAD_PLUGIN (object);

	eoc_debug_message (DEBUG_PLUGINS, "EocReloadPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eoc_reload_plugin_parent_class)->dispose (object);
}

static void
eoc_reload_plugin_activate (EocWindowActivatable *activatable)
{
	EocReloadPlugin *plugin = EOC_RELOAD_PLUGIN (activatable);
	GtkUIManager *manager;

	eoc_debug (DEBUG_PLUGINS);

	manager = eoc_window_get_ui_manager (plugin->window);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	plugin->ui_action_group = gtk_action_group_new ("EocReloadPluginActions");

	gtk_action_group_set_translation_domain (plugin->ui_action_group, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (plugin->ui_action_group, action_entries,
	                              G_N_ELEMENTS (action_entries), plugin->window);
	G_GNUC_END_IGNORE_DEPRECATIONS;

	gtk_ui_manager_insert_action_group (manager, plugin->ui_action_group, -1);

	plugin->ui_id = gtk_ui_manager_add_ui_from_string (manager, ui_definition, -1, NULL);
	g_warn_if_fail (plugin->ui_id != 0);
}

static void
eoc_reload_plugin_deactivate (EocWindowActivatable *activatable)
{
	EocReloadPlugin *plugin = EOC_RELOAD_PLUGIN (activatable);
	GtkUIManager *manager;

	eoc_debug (DEBUG_PLUGINS);

	manager = eoc_window_get_ui_manager (plugin->window);

	gtk_ui_manager_remove_ui (manager, plugin->ui_id);

	gtk_ui_manager_remove_action_group (manager, plugin->ui_action_group);

	gtk_ui_manager_ensure_update (manager);
}

static void
eoc_reload_plugin_class_init (EocReloadPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eoc_reload_plugin_dispose;
	object_class->set_property = eoc_reload_plugin_set_property;
	object_class->get_property = eoc_reload_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eoc_reload_plugin_class_finalize (EocReloadPluginClass *klass)
{
	/* dummy function - used by G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface)
{
	iface->activate = eoc_reload_plugin_activate;
	iface->deactivate = eoc_reload_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eoc_reload_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
	                                            EOC_TYPE_WINDOW_ACTIVATABLE,
	                                            EOC_TYPE_RELOAD_PLUGIN);
}
