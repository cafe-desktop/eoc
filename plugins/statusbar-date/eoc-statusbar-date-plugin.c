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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eoc-statusbar-date-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>
#include <libpeas/peas-activatable.h>

#include <eoc-debug.h>
#include <eoc-image.h>
#include <eoc-thumb-view.h>
#include <eoc-exif-util.h>
#include <eoc-window.h>
#include <eoc-window-activatable.h>

static void eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EocStatusbarDatePlugin,
                                eoc_statusbar_date_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (EOC_TYPE_WINDOW_ACTIVATABLE,
                                                               eoc_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static void
statusbar_set_date (CtkStatusbar *statusbar,
                    EocThumbView *view)
{
	EocImage *image;
	gchar *date = NULL;
	gchar time_buffer[32];
	ExifData *exif_data;

	if (eoc_thumb_view_get_n_selected (view) == 0)
	{
		return;
	}

	image = eoc_thumb_view_get_first_selected_image (view);

	ctk_statusbar_pop (statusbar, 0);

	if (!eoc_image_has_data (image, EOC_IMAGE_DATA_EXIF))
	{
		if (!eoc_image_load (image, EOC_IMAGE_DATA_EXIF, NULL, NULL))
		{
			ctk_widget_hide (CTK_WIDGET (statusbar));
		}
	}

	exif_data = eoc_image_get_exif_info (image);

	if (exif_data)
	{
		date = eoc_exif_util_format_date (eoc_exif_data_get_value (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL, time_buffer, 32));
		eoc_exif_data_free (exif_data);
	}

	if (date)
	{
		ctk_statusbar_push (statusbar, 0, date);
		ctk_widget_show (CTK_WIDGET (statusbar));
		g_free (date);
	}
	else
	{
		ctk_widget_hide (CTK_WIDGET (statusbar));
	}
}

static void
selection_changed_cb (EocThumbView           *view,
                      EocStatusbarDatePlugin *plugin)
{
	statusbar_set_date (CTK_STATUSBAR (plugin->statusbar_date), view);
}

static void
eoc_statusbar_date_plugin_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	EocStatusbarDatePlugin *plugin = EOC_STATUSBAR_DATE_PLUGIN (object);

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
eoc_statusbar_date_plugin_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	EocStatusbarDatePlugin *plugin = EOC_STATUSBAR_DATE_PLUGIN (object);

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
eoc_statusbar_date_plugin_init (EocStatusbarDatePlugin *plugin)
{
	eoc_debug_message (DEBUG_PLUGINS, "EocStatusbarDatePlugin initializing");
}

static void
eoc_statusbar_date_plugin_dispose (GObject *object)
{
	EocStatusbarDatePlugin *plugin = EOC_STATUSBAR_DATE_PLUGIN (object);

	eoc_debug_message (DEBUG_PLUGINS, "EocStatusbarDatePlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eoc_statusbar_date_plugin_parent_class)->dispose (object);
}

static void
eoc_statusbar_date_plugin_activate (EocWindowActivatable *activatable)
{
	EocStatusbarDatePlugin *plugin = EOC_STATUSBAR_DATE_PLUGIN (activatable);
	EocWindow *window = plugin->window;
	CtkWidget *statusbar = eoc_window_get_statusbar (window);
	CtkWidget *thumbview = eoc_window_get_thumb_view (window);

	eoc_debug (DEBUG_PLUGINS);

	plugin->statusbar_date = ctk_statusbar_new ();
	ctk_widget_set_size_request (plugin->statusbar_date, 200, 10);
	ctk_widget_set_margin_top (CTK_WIDGET (plugin->statusbar_date), 0);
	ctk_widget_set_margin_bottom (CTK_WIDGET (plugin->statusbar_date), 0);
	ctk_box_pack_end (CTK_BOX (statusbar), plugin->statusbar_date, FALSE, FALSE, 0);

	plugin->signal_id = g_signal_connect_after (G_OBJECT (thumbview), "selection_changed",
	                                            G_CALLBACK (selection_changed_cb), plugin);

	statusbar_set_date (CTK_STATUSBAR (plugin->statusbar_date),
	                    EOC_THUMB_VIEW (eoc_window_get_thumb_view (window)));
}

static void
eoc_statusbar_date_plugin_deactivate (EocWindowActivatable *activatable)
{
	EocStatusbarDatePlugin *plugin = EOC_STATUSBAR_DATE_PLUGIN (activatable);
	EocWindow *window = plugin->window;
	CtkWidget *statusbar = eoc_window_get_statusbar (window);
	CtkWidget *view = eoc_window_get_thumb_view (window);

	g_signal_handler_disconnect (view, plugin->signal_id);

	ctk_container_remove (CTK_CONTAINER (statusbar), plugin->statusbar_date);
}

static void
eoc_statusbar_date_plugin_class_init (EocStatusbarDatePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eoc_statusbar_date_plugin_dispose;
	object_class->set_property = eoc_statusbar_date_plugin_set_property;
	object_class->get_property = eoc_statusbar_date_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eoc_statusbar_date_plugin_class_finalize (EocStatusbarDatePluginClass *klass)
{
	/* dummy function - used by G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
eoc_window_activatable_iface_init (EocWindowActivatableInterface *iface)
{
	iface->activate = eoc_statusbar_date_plugin_activate;
	iface->deactivate = eoc_statusbar_date_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eoc_statusbar_date_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
	                                            EOC_TYPE_WINDOW_ACTIVATABLE,
	                                            EOC_TYPE_STATUSBAR_DATE_PLUGIN);
}
