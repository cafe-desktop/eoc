/* Eye Of Cafe - EOC Preferences Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
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
#include "config.h"
#endif

#include "eoc-preferences-dialog.h"
#include "eoc-scroll-view.h"
#include "eoc-util.h"
#include "eoc-config-keys.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <libpeas-ctk/peas-ctk-plugin-manager.h>

#define GSETTINGS_OBJECT_KEY		"GSETTINGS_KEY"
#define GSETTINGS_OBJECT_VALUE		"GSETTINGS_VALUE"

struct _EocPreferencesDialogPrivate {
	GSettings    *view_settings;
	GSettings    *ui_settings;
	GSettings    *fullscreen_settings;

	CtkWidget     *interpolate_check;
	CtkWidget     *extrapolate_check;
	CtkWidget     *autorotate_check;
	CtkWidget     *bg_color_check;
	CtkWidget     *bg_color_button;
	CtkWidget     *color_radio;
	CtkWidget     *checkpattern_radio;
	CtkWidget     *background_radio;
	CtkWidget     *transp_color_button;

	CtkWidget     *upscale_check;
	CtkWidget     *random_check;
	CtkWidget     *loop_check;
	CtkWidget     *seconds_spin;

	CtkWidget     *plugin_manager;
};

static GObject *instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (EocPreferencesDialog, eoc_preferences_dialog, CTK_TYPE_DIALOG);

static gboolean
pd_string_to_rgba_mapping (GValue   *value,
			    GVariant *variant,
			    gpointer user_data)
{
	GdkRGBA color;

	g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING), FALSE);

	if (gdk_rgba_parse (&color, g_variant_get_string (variant, NULL))) {
		g_value_set_boxed (value, &color);
		return TRUE;
	}

	return FALSE;
}

static GVariant*
pd_rgba_to_string_mapping (const GValue       *value,
			    const GVariantType *expected_type,
			    gpointer            user_data)
{

	GVariant *variant = NULL;
	GdkRGBA *color;
	gchar *hex_val;

	g_return_val_if_fail (G_VALUE_TYPE (value) == GDK_TYPE_RGBA, NULL);
	g_return_val_if_fail (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING), NULL);

	color = g_value_get_boxed (value);
	hex_val = gdk_rgba_to_string(color);

	variant = g_variant_new_string (hex_val);
	g_free (hex_val);

	return variant;
}

static void
pd_transp_radio_toggle_cb (CtkWidget *widget, gpointer data)
{
	gpointer value = NULL;

	if (!ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)))
	    return;

	value = g_object_get_data (G_OBJECT (widget), GSETTINGS_OBJECT_VALUE);

	g_settings_set_enum (G_SETTINGS (data), EOC_CONF_VIEW_TRANSPARENCY,
			     GPOINTER_TO_INT (value));
}

static void
random_change_cb (GSettings *settings, gchar *key, CtkWidget *widget)
{
	ctk_widget_set_sensitive (widget, !g_settings_get_boolean (settings, key));
}

static void
eoc_preferences_response_cb (CtkDialog *dlg, gint res_id, gpointer data)
{
	switch (res_id) {
		case CTK_RESPONSE_HELP:
			eoc_util_show_help ("eoc-prefs", NULL);
			break;
		default:
			ctk_widget_destroy (CTK_WIDGET (dlg));
			instance = NULL;
	}
}

static void
eoc_preferences_dialog_class_init (EocPreferencesDialogClass *klass)
{
	CtkWidgetClass *widget_class = (CtkWidgetClass*) klass;

	/* This should make sure the libpeas-ctk dependency isn't
	 * dropped by aggressive linkers (#739618) */
	g_type_ensure (PEAS_CTK_TYPE_PLUGIN_MANAGER);

	ctk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/cafe/eoc/ui/eoc-preferences-dialog.ui");
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              interpolate_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              extrapolate_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              autorotate_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              bg_color_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              bg_color_button);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              color_radio);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              checkpattern_radio);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              background_radio);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              transp_color_button);

	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              upscale_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              random_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              loop_check);
	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              seconds_spin);

	ctk_widget_class_bind_template_child_private (widget_class,
	                                              EocPreferencesDialog,
	                                              plugin_manager);
}

static void
eoc_preferences_dialog_init (EocPreferencesDialog *pref_dlg)
{
	EocPreferencesDialogPrivate *priv;

	pref_dlg->priv = eoc_preferences_dialog_get_instance_private (pref_dlg);
	priv = pref_dlg->priv;

	ctk_widget_init_template (CTK_WIDGET (pref_dlg));

	priv->view_settings = g_settings_new (EOC_CONF_VIEW);
	priv->fullscreen_settings = g_settings_new (EOC_CONF_FULLSCREEN);

	g_signal_connect (G_OBJECT (pref_dlg),
	                  "response",
	                  G_CALLBACK (eoc_preferences_response_cb),
	                  pref_dlg);

	g_settings_bind (priv->view_settings,
	                 EOC_CONF_VIEW_INTERPOLATE,
	                 priv->interpolate_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings,
	                 EOC_CONF_VIEW_EXTRAPOLATE,
	                 priv->extrapolate_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings,
	                 EOC_CONF_VIEW_AUTOROTATE,
	                 priv->autorotate_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings,
	                 EOC_CONF_VIEW_USE_BG_COLOR,
	                 priv->bg_color_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);

	g_settings_bind_with_mapping (priv->view_settings,
	                              EOC_CONF_VIEW_BACKGROUND_COLOR,
	                              priv->bg_color_button, "rgba",
	                              G_SETTINGS_BIND_DEFAULT,
	                              pd_string_to_rgba_mapping,
	                              pd_rgba_to_string_mapping,
	                              NULL, NULL);
	g_object_set_data (G_OBJECT (priv->color_radio),
	                   GSETTINGS_OBJECT_VALUE,
	                   GINT_TO_POINTER (EOC_TRANSP_COLOR));

	g_signal_connect (G_OBJECT (priv->color_radio),
	                  "toggled",
	                  G_CALLBACK (pd_transp_radio_toggle_cb),
	                  priv->view_settings);

	g_object_set_data (G_OBJECT (priv->checkpattern_radio),
	                   GSETTINGS_OBJECT_VALUE,
	                   GINT_TO_POINTER (EOC_TRANSP_CHECKED));

	g_signal_connect (G_OBJECT (priv->checkpattern_radio),
	                  "toggled",
	                  G_CALLBACK (pd_transp_radio_toggle_cb),
	                  priv->view_settings);

	g_object_set_data (G_OBJECT (priv->background_radio),
	                   GSETTINGS_OBJECT_VALUE,
	                   GINT_TO_POINTER (EOC_TRANSP_BACKGROUND));

	g_signal_connect (G_OBJECT (priv->background_radio),
	                  "toggled",
	                  G_CALLBACK (pd_transp_radio_toggle_cb),
	                  priv->view_settings);

	switch (g_settings_get_enum (priv->view_settings,
				    EOC_CONF_VIEW_TRANSPARENCY))
	{
	case EOC_TRANSP_COLOR:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->color_radio), TRUE);
		break;
	case EOC_TRANSP_CHECKED:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->checkpattern_radio), TRUE);
		break;
	default:
		// Log a warning and use EOC_TRANSP_BACKGROUND as fallback
		g_warn_if_reached ();
	case EOC_TRANSP_BACKGROUND:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->background_radio), TRUE);
		break;
	}

	g_settings_bind_with_mapping (priv->view_settings,
	                              EOC_CONF_VIEW_TRANS_COLOR,
	                              priv->transp_color_button, "rgba",
	                              G_SETTINGS_BIND_DEFAULT,
	                              pd_string_to_rgba_mapping,
	                              pd_rgba_to_string_mapping,
	                              NULL, NULL);

	g_settings_bind (priv->fullscreen_settings, EOC_CONF_FULLSCREEN_UPSCALE,
	                 priv->upscale_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (priv->fullscreen_settings,
	                 EOC_CONF_FULLSCREEN_LOOP,
	                 priv->loop_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (priv->fullscreen_settings,
	                 EOC_CONF_FULLSCREEN_RANDOM,
	                 priv->random_check, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	g_signal_connect (priv->fullscreen_settings,
	                  "changed::" EOC_CONF_FULLSCREEN_RANDOM,
	                  G_CALLBACK (random_change_cb),
	                  priv->loop_check);
	random_change_cb (priv->fullscreen_settings,
	                  EOC_CONF_FULLSCREEN_RANDOM,
	                  priv->loop_check);

	g_settings_bind (priv->fullscreen_settings,
	                 EOC_CONF_FULLSCREEN_SECONDS,
	                 priv->seconds_spin, "value",
	                 G_SETTINGS_BIND_DEFAULT);

	ctk_widget_show_all (priv->plugin_manager);
}

CtkWidget *eoc_preferences_dialog_get_instance (CtkWindow *parent)
{
	if (instance == NULL) {
		instance = g_object_new (EOC_TYPE_PREFERENCES_DIALOG,
				 	 NULL);
	}

	if (parent)
		ctk_window_set_transient_for (CTK_WINDOW (instance), parent);

	return CTK_WIDGET(instance);
}
