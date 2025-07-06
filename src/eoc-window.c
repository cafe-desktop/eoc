/* Eye Of Cafe - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnu.org>
 *	- Jens Finke <jens@gnome.org>
 * Based on evince code (shell/ev-window.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#include <math.h>

#include "eoc-window.h"
#include "eoc-scroll-view.h"
#include "eoc-debug.h"
#include "eoc-file-chooser.h"
#include "eoc-thumb-view.h"
#include "eoc-list-store.h"
#include "eoc-sidebar.h"
#include "eoc-statusbar.h"
#include "eoc-preferences-dialog.h"
#include "eoc-properties-dialog.h"
#include "eoc-print.h"
#include "eoc-error-message-area.h"
#include "eoc-application.h"
#include "eoc-application-internal.h"
#include "eoc-thumb-nav.h"
#include "eoc-config-keys.h"
#include "eoc-job-queue.h"
#include "eoc-jobs.h"
#include "eoc-util.h"
#include "eoc-save-as-dialog-helper.h"
#include "eoc-close-confirmation-dialog.h"
#include "eoc-clipboard-handler.h"
#include "eoc-window-activatable.h"
#include "eoc-metadata-sidebar.h"

#include "eoc-enum-types.h"

#include "egg-toolbar-editor.h"
#include "egg-editable-toolbar.h"
#include "egg-toolbars-model.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <cdk/cdkkeysyms.h>
#include <gio/gdesktopappinfo.h>
#include <ctk/ctk.h>

#include <libbean/bean-extension-set.h>
#include <libbean/bean-activatable.h>

#if HAVE_LCMS
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef CDK_WINDOWING_X11
#include <cdk/cdkx.h>
#endif
#include <lcms2.h>
#endif

#define CAFE_DESKTOP_USE_UNSTABLE_API
#include <libcafe-desktop/cafe-desktop-utils.h>

#define EOC_WINDOW_MIN_WIDTH  440
#define EOC_WINDOW_MIN_HEIGHT 350

#define EOC_WINDOW_DEFAULT_WIDTH  540
#define EOC_WINDOW_DEFAULT_HEIGHT 450

#define EOC_WINDOW_FULLSCREEN_TIMEOUT 5 * 1000
#define EOC_WINDOW_FULLSCREEN_POPUP_THRESHOLD 5

#define EOC_RECENT_FILES_GROUP  "Graphics"
#define EOC_RECENT_FILES_APP_NAME "Eye of CAFE Image Viewer"
#define EOC_RECENT_FILES_LIMIT  5

#define EOC_WALLPAPER_FILENAME "eoc-wallpaper"

#define is_rtl (ctk_widget_get_default_direction () == CTK_TEXT_DIR_RTL)

typedef enum {
	EOC_WINDOW_STATUS_UNKNOWN,
	EOC_WINDOW_STATUS_INIT,
	EOC_WINDOW_STATUS_NORMAL
} EocWindowStatus;

enum {
	PROP_0,
	PROP_COLLECTION_POS,
	PROP_COLLECTION_RESIZABLE,
	PROP_STARTUP_FLAGS
};

enum {
	SIGNAL_PREPARED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

struct _EocWindowPrivate {
	GSettings           *view_settings;
	GSettings           *ui_settings;
	GSettings           *fullscreen_settings;
	GSettings           *lockdown_settings;

	EocListStore        *store;
	EocImage            *image;
	EocWindowMode        mode;
	EocWindowStatus      status;

	CtkUIManager        *ui_mgr;
	CtkWidget           *box;
	CtkWidget           *layout;
	CtkWidget           *cbox;
	CtkWidget           *view;
	CtkWidget           *sidebar;
	CtkWidget           *thumbview;
	CtkWidget           *statusbar;
	CtkWidget           *nav;
	CtkWidget           *message_area;
	CtkWidget           *toolbar;
	CtkWidget           *properties_dlg;

	CtkActionGroup      *actions_window;
	CtkActionGroup      *actions_image;
	CtkActionGroup      *actions_collection;
	CtkActionGroup      *actions_recent;

	CtkWidget           *fullscreen_popup;
	GSource             *fullscreen_timeout_source;

	gboolean             slideshow_random;
	gboolean             slideshow_loop;
	gint                 slideshow_switch_timeout;
	GSource             *slideshow_switch_source;

	guint                fullscreen_idle_inhibit_cookie;

	guint                recent_menu_id;

	EocJob              *load_job;
	EocJob              *transform_job;
	EocJob              *save_job;
	GFile               *last_save_as_folder;
	EocJob              *copy_job;

	guint                image_info_message_cid;
	guint                tip_message_cid;
	guint                copy_file_cid;

	EocStartupFlags      flags;
	GSList              *file_list;

	EocWindowCollectionPos collection_position;
	gboolean             collection_resizable;

	CtkActionGroup      *actions_open_with;
	guint                open_with_menu_id;

	gboolean             save_disabled;
	gboolean             needs_reload_confirmation;

	CtkPageSetup        *page_setup;

	BeanExtensionSet    *extensions;

#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
	cmsHPROFILE         *display_profile;
#endif
};

G_DEFINE_TYPE_WITH_PRIVATE (EocWindow, eoc_window, CTK_TYPE_APPLICATION_WINDOW);

static void eoc_window_cmd_fullscreen (CtkAction *action, gpointer user_data);
static void eoc_window_run_fullscreen (EocWindow *window, gboolean slideshow);
static void eoc_window_cmd_slideshow (CtkAction *action, gpointer user_data);
static void eoc_window_cmd_pause_slideshow (CtkAction *action, gpointer user_data);
static void eoc_window_stop_fullscreen (EocWindow *window, gboolean slideshow);
static void eoc_job_load_cb (EocJobLoad *job, gpointer data);
static void eoc_job_save_progress_cb (EocJobSave *job, float progress, gpointer data);
static void eoc_job_progress_cb (EocJobLoad *job, float progress, gpointer data);
static void eoc_job_transform_cb (EocJobTransform *job, gpointer data);
static void fullscreen_set_timeout (EocWindow *window);
static void fullscreen_clear_timeout (EocWindow *window);
static void update_action_groups_state (EocWindow *window);
static void open_with_launch_application_cb (CtkAction *action, gpointer callback_data);
static void eoc_window_update_openwith_menu (EocWindow *window, EocImage *image);
static void eoc_window_list_store_image_added (CtkTreeModel *tree_model,
					       CtkTreePath  *path,
					       CtkTreeIter  *iter,
					       gpointer      user_data);
static void eoc_window_list_store_image_removed (CtkTreeModel *tree_model,
                 				 CtkTreePath  *path,
						 gpointer      user_data);
static void eoc_window_set_wallpaper (EocWindow *window, const gchar *filename, const gchar *visible_filename);
static gboolean eoc_window_save_images (EocWindow *window, GList *images);
static void disconnect_proxy_cb (CtkUIManager *manager,
                                 CtkAction *action,
                                 CtkWidget *proxy,
                                 EocWindow *window);
static void eoc_window_finish_saving (EocWindow *window);
static GAppInfo *get_appinfo_for_editor (EocWindow *window);

static GQuark
eoc_window_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("eoc-window-error-quark");

	return q;
}

static void
eoc_window_set_collection_mode (EocWindow *window, EocWindowCollectionPos position, gboolean resizable)
{
	EocWindowPrivate *priv;
	CtkWidget *hpaned;
	EocThumbNavMode mode = EOC_THUMB_NAV_MODE_ONE_ROW;

	eoc_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOC_IS_WINDOW (window));

	priv = window->priv;

	if (priv->collection_position == position &&
	    priv->collection_resizable == resizable)
		return;

	priv->collection_position = position;
	priv->collection_resizable = resizable;

	hpaned = ctk_widget_get_parent (priv->sidebar);

	g_object_ref (hpaned);
	g_object_ref (priv->nav);

	ctk_container_remove (CTK_CONTAINER (priv->layout), hpaned);
	ctk_container_remove (CTK_CONTAINER (priv->layout), priv->nav);

	ctk_widget_destroy (priv->layout);

	switch (position) {
	case EOC_WINDOW_COLLECTION_POS_BOTTOM:
	case EOC_WINDOW_COLLECTION_POS_TOP:
		if (resizable) {
			mode = EOC_THUMB_NAV_MODE_MULTIPLE_ROWS;

			priv->layout = ctk_paned_new (CTK_ORIENTATION_VERTICAL);

			if (position == EOC_WINDOW_COLLECTION_POS_BOTTOM) {
				ctk_paned_pack1 (CTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				ctk_paned_pack2 (CTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			} else {
				ctk_paned_pack1 (CTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				ctk_paned_pack2 (CTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			}
		} else {
			mode = EOC_THUMB_NAV_MODE_ONE_ROW;

			priv->layout = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);

			if (position == EOC_WINDOW_COLLECTION_POS_BOTTOM) {
				ctk_box_pack_start (CTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				ctk_box_pack_start (CTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			} else {
				ctk_box_pack_start (CTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				ctk_box_pack_start (CTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			}
		}
		break;

	case EOC_WINDOW_COLLECTION_POS_LEFT:
	case EOC_WINDOW_COLLECTION_POS_RIGHT:
		if (resizable) {
			mode = EOC_THUMB_NAV_MODE_MULTIPLE_COLUMNS;

			priv->layout = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);

			if (position == EOC_WINDOW_COLLECTION_POS_LEFT) {
				ctk_paned_pack1 (CTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				ctk_paned_pack2 (CTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			} else {
				ctk_paned_pack1 (CTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				ctk_paned_pack2 (CTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			}
		} else {
			mode = EOC_THUMB_NAV_MODE_ONE_COLUMN;

			priv->layout = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

			if (position == EOC_WINDOW_COLLECTION_POS_LEFT) {
				ctk_box_pack_start (CTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				ctk_box_pack_start (CTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			} else {
				ctk_box_pack_start (CTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				ctk_box_pack_start (CTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			}
		}

		break;
	}

	ctk_box_pack_end (CTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	eoc_thumb_nav_set_mode (EOC_THUMB_NAV (priv->nav), mode);

	if (priv->mode != EOC_WINDOW_MODE_UNKNOWN) {
		update_action_groups_state (window);
	}
}

static void
eoc_window_can_save_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
	EocWindowPrivate *priv;
	EocWindow *window;
	gboolean save_disabled = FALSE;
	CtkAction *action_save, *action_save_as;

	eoc_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = EOC_WINDOW (user_data)->priv;

	save_disabled = g_settings_get_boolean (settings, key);

	priv->save_disabled = save_disabled;

	action_save =
		ctk_action_group_get_action (priv->actions_image, "ImageSave");
	action_save_as =
		ctk_action_group_get_action (priv->actions_image, "ImageSaveAs");

	if (priv->save_disabled) {
		ctk_action_set_sensitive (action_save, FALSE);
		ctk_action_set_sensitive (action_save_as, FALSE);
	} else {
		EocImage *image = eoc_window_get_image (window);

		if (EOC_IS_IMAGE (image)) {
			ctk_action_set_sensitive (action_save,
						  eoc_image_is_modified (image));

			ctk_action_set_sensitive (action_save_as, TRUE);
		}
	}
}

#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
static cmsHPROFILE *
eoc_window_get_display_profile (CdkScreen *screen)
{
	Display *dpy;
	Atom icc_atom, type;
	int format;
	gulong nitems;
	gulong bytes_after;
	gulong length;
	guchar *str;
	int result;
	cmsHPROFILE *profile = NULL;
	char *atom_name;

	if (!CDK_IS_X11_SCREEN (screen)) {
		return NULL;
	}

	dpy = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));

	if (cdk_x11_screen_get_screen_number (screen) > 0)
		atom_name = g_strdup_printf ("_ICC_PROFILE_%d", cdk_x11_screen_get_screen_number (screen));
	else
		atom_name = g_strdup ("_ICC_PROFILE");

	icc_atom = cdk_x11_get_xatom_by_name_for_display (cdk_screen_get_display (screen), atom_name);

	g_free (atom_name);

	result = XGetWindowProperty (dpy,
				     CDK_WINDOW_XID (cdk_screen_get_root_window (screen)),
				     icc_atom,
				     0,
				     G_MAXLONG,
				     False,
				     XA_CARDINAL,
				     &type,
				     &format,
				     &nitems,
				     &bytes_after,
                                     (guchar **)&str);

	/* TODO: handle bytes_after != 0 */

	if ((result == Success) && (type == XA_CARDINAL) && (nitems > 0)) {
		switch (format)
		{
			case 8:
				length = nitems;
				break;
			case 16:
				length = sizeof(short) * nitems;
				break;
			case 32:
				length = sizeof(long) * nitems;
				break;
			default:
				eoc_debug_message (DEBUG_LCMS, "Unable to read profile, not correcting");

				XFree (str);
				return NULL;
		}

		profile = cmsOpenProfileFromMem (str, length);

		if (G_UNLIKELY (profile == NULL)) {
			eoc_debug_message (DEBUG_LCMS,
					   "Invalid display profile set, "
					   "not using it");
		}

		XFree (str);
	}

	if (profile == NULL) {
		profile = cmsCreate_sRGBProfile ();
		eoc_debug_message (DEBUG_LCMS,
				 "No valid display profile set, assuming sRGB");
	}

	return profile;
}
#endif

static void
update_image_pos (EocWindow *window)
{
	EocWindowPrivate *priv;
	gint pos = -1, n_images = 0;

	priv = window->priv;

	n_images = eoc_list_store_length (EOC_LIST_STORE (priv->store));

	if (n_images > 0) {
		pos = eoc_list_store_get_pos_by_image (EOC_LIST_STORE (priv->store),
						       priv->image);
	}
	/* Images: (image pos) / (n_total_images) */
	eoc_statusbar_set_image_number (EOC_STATUSBAR (priv->statusbar),
					pos + 1,
					n_images);

}

static void
update_status_bar (EocWindow *window)
{
	EocWindowPrivate *priv;
	char *str = NULL;

	g_return_if_fail (EOC_IS_WINDOW (window));

	eoc_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->image != NULL &&
	    eoc_image_has_data (priv->image, EOC_IMAGE_DATA_DIMENSION)) {
		int zoom, width, height;
		goffset bytes = 0;

		zoom = floor (100 * eoc_scroll_view_get_zoom (EOC_SCROLL_VIEW (priv->view)) + 0.5);

		eoc_image_get_size (priv->image, &width, &height);

		bytes = eoc_image_get_bytes (priv->image);

		if ((width > 0) && (height > 0)) {
			char *size_string;

				size_string = g_format_size (bytes);

			/* Translators: This is the string displayed in the statusbar
			 * The tokens are from left to right:
			 * - image width
			 * - image height
			 * - image size in bytes
			 * - zoom in percent */
			str = g_strdup_printf (ngettext("%i × %i pixel  %s    %i%%",
							"%i × %i pixels  %s    %i%%", height),
						width,
						height,
						size_string,
						zoom);

			g_free (size_string);
		}

		update_image_pos (window);
	}

	ctk_statusbar_pop (CTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	ctk_statusbar_push (CTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, str ? str : "");

	g_free (str);
}

static void
eoc_window_set_message_area (EocWindow *window,
		             CtkWidget *message_area)
{
	if (window->priv->message_area == message_area)
		return;

	if (window->priv->message_area != NULL)
		ctk_widget_destroy (window->priv->message_area);

	window->priv->message_area = message_area;

	if (message_area == NULL) return;

	ctk_box_pack_start (CTK_BOX (window->priv->cbox),
			    window->priv->message_area,
			    FALSE,
			    FALSE,
			    0);

	g_object_add_weak_pointer (G_OBJECT (window->priv->message_area),
				   (void *) &window->priv->message_area);
}

static void
update_action_groups_state (EocWindow *window)
{
	EocWindowPrivate *priv;
	CtkAction *action_collection;
	CtkAction *action_sidebar;
	CtkAction *action_fscreen;
	CtkAction *action_sshow;
	CtkAction *action_print;
	gboolean print_disabled = FALSE;
	gboolean show_image_collection = FALSE;
	gint n_images = 0;

	g_return_if_fail (EOC_IS_WINDOW (window));

	eoc_debug (DEBUG_WINDOW);

	priv = window->priv;

	action_collection =
		ctk_action_group_get_action (priv->actions_window,
					     "ViewImageCollection");

	action_sidebar =
		ctk_action_group_get_action (priv->actions_window,
					     "ViewSidebar");

	action_fscreen =
		ctk_action_group_get_action (priv->actions_image,
					     "ViewFullscreen");

	action_sshow =
		ctk_action_group_get_action (priv->actions_collection,
					     "ViewSlideshow");

	action_print =
		ctk_action_group_get_action (priv->actions_image,
					     "ImagePrint");

	g_assert (action_collection != NULL);
	g_assert (action_sidebar != NULL);
	g_assert (action_fscreen != NULL);
	g_assert (action_sshow != NULL);
	g_assert (action_print != NULL);

	if (priv->store != NULL) {
		n_images = eoc_list_store_length (EOC_LIST_STORE (priv->store));
	}

	if (n_images == 0) {
		ctk_widget_hide (priv->layout);

		ctk_action_group_set_sensitive (priv->actions_window,      TRUE);
		ctk_action_group_set_sensitive (priv->actions_image,       FALSE);
		ctk_action_group_set_sensitive (priv->actions_collection,  FALSE);

		ctk_action_set_sensitive (action_fscreen, FALSE);
		ctk_action_set_sensitive (action_sshow,   FALSE);

		/* If there are no images on model, initialization
 		   stops here. */
		if (priv->status == EOC_WINDOW_STATUS_INIT) {
			priv->status = EOC_WINDOW_STATUS_NORMAL;
		}
	} else {
		if (priv->flags & EOC_STARTUP_DISABLE_COLLECTION) {
			g_settings_set_boolean (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION, FALSE);

			show_image_collection = FALSE;
		} else {
			show_image_collection =
				g_settings_get_boolean (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION);
		}

		show_image_collection = show_image_collection &&
					n_images > 1 &&
					priv->mode != EOC_WINDOW_MODE_SLIDESHOW;

		ctk_widget_show (priv->layout);

		if (show_image_collection)
			ctk_widget_show (priv->nav);

		ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action_collection),
					      show_image_collection);

		ctk_action_group_set_sensitive (priv->actions_window, TRUE);
		ctk_action_group_set_sensitive (priv->actions_image,  TRUE);

		ctk_action_set_sensitive (action_fscreen, TRUE);

		if (n_images == 1) {
			ctk_action_group_set_sensitive (priv->actions_collection,  FALSE);
			ctk_action_set_sensitive (action_collection, FALSE);
			ctk_action_set_sensitive (action_sshow, FALSE);
		} else {
			ctk_action_group_set_sensitive (priv->actions_collection,  TRUE);
			ctk_action_set_sensitive (action_sshow, TRUE);
		}

		if (show_image_collection)
			ctk_widget_grab_focus (priv->thumbview);
		else
			ctk_widget_grab_focus (priv->view);
	}

	print_disabled = g_settings_get_boolean (priv->lockdown_settings,
						EOC_CONF_LOCKDOWN_CAN_PRINT);

	if (print_disabled) {
		ctk_action_set_sensitive (action_print, FALSE);
	}

	if (eoc_sidebar_is_empty (EOC_SIDEBAR (priv->sidebar))) {
		ctk_action_set_sensitive (action_sidebar, FALSE);
		ctk_widget_hide (priv->sidebar);
	}
}

static void
update_selection_ui_visibility (EocWindow *window)
{
	EocWindowPrivate *priv;
	CtkAction *wallpaper_action;
	gint n_selected;

	priv = window->priv;

	n_selected = eoc_thumb_view_get_n_selected (EOC_THUMB_VIEW (priv->thumbview));

	wallpaper_action =
		ctk_action_group_get_action (priv->actions_image,
					     "ImageSetAsWallpaper");

	if (n_selected == 1) {
		ctk_action_set_sensitive (wallpaper_action, TRUE);
	} else {
		ctk_action_set_sensitive (wallpaper_action, FALSE);
	}
}

static gboolean
add_file_to_recent_files (GFile *file)
{
	gchar *text_uri;
	GFileInfo *file_info;
	CtkRecentData *recent_data;
	static gchar *groups[2] = { EOC_RECENT_FILES_GROUP , NULL };

	if (file == NULL) return FALSE;

	/* The password gets stripped here because ~/.recently-used.xbel is
	 * readable by everyone (chmod 644). It also makes the workaround
	 * for the bug with ctk_recent_info_get_uri_display() easier
	 * (see the comment in eoc_window_update_recent_files_menu()). */
	text_uri = g_file_get_uri (file);

	if (text_uri == NULL)
		return FALSE;

	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL)
		return FALSE;

	recent_data = g_slice_new (CtkRecentData);
	recent_data->display_name = NULL;
	recent_data->description = NULL;
	recent_data->mime_type = (gchar *) g_file_info_get_content_type (file_info);
	recent_data->app_name = EOC_RECENT_FILES_APP_NAME;
	recent_data->app_exec = g_strjoin(" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;
	recent_data->is_private = FALSE;

	ctk_recent_manager_add_full (ctk_recent_manager_get_default (),
				     text_uri,
				     recent_data);

	g_free (recent_data->app_exec);
	g_free (text_uri);
	g_object_unref (file_info);

	g_slice_free (CtkRecentData, recent_data);

	return FALSE;
}

static void
image_thumb_changed_cb (EocImage *image, gpointer data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
	GdkPixbuf *thumb;

	g_return_if_fail (EOC_IS_WINDOW (data));

	window = EOC_WINDOW (data);
	priv = window->priv;

	thumb = eoc_image_get_thumbnail (image);

	if (thumb != NULL) {
		ctk_window_set_icon (CTK_WINDOW (window), thumb);

		if (window->priv->properties_dlg != NULL) {
			eoc_properties_dialog_update (EOC_PROPERTIES_DIALOG (priv->properties_dlg),
						      image);
		}

		g_object_unref (thumb);
	} else if (!ctk_widget_get_visible (window->priv->nav)) {
		gint img_pos = eoc_list_store_get_pos_by_image (window->priv->store, image);
		CtkTreePath *path = ctk_tree_path_new_from_indices (img_pos,-1);
		CtkTreeIter iter;

		ctk_tree_model_get_iter (CTK_TREE_MODEL (window->priv->store), &iter, path);
		eoc_list_store_thumbnail_set (window->priv->store, &iter);
		ctk_tree_path_free (path);
	}
}

static void
file_changed_info_bar_response (CtkInfoBar *info_bar G_GNUC_UNUSED,
				gint        response,
				EocWindow  *window)
{
	if (response == CTK_RESPONSE_YES) {
		eoc_window_reload_image (window);
	}

	window->priv->needs_reload_confirmation = TRUE;

	eoc_window_set_message_area (window, NULL);
}
static void
image_file_changed_cb (EocImage *img, EocWindow *window)
{
	CtkWidget *info_bar;
	gchar *text, *markup;
	CtkWidget *image;
	CtkWidget *label;
	CtkWidget *hbox;

	if (window->priv->needs_reload_confirmation == FALSE)
		return;

	if (!eoc_image_is_modified (img)) {
		/* Auto-reload when image is unmodified */
		eoc_window_reload_image (window);
		return;
	}

	window->priv->needs_reload_confirmation = FALSE;

	info_bar = ctk_info_bar_new_with_buttons (_("_Reload"),
						  CTK_RESPONSE_YES,
						  C_("MessageArea", "Hi_de"),
						  CTK_RESPONSE_NO, NULL);
	ctk_info_bar_set_message_type (CTK_INFO_BAR (info_bar),
				       CTK_MESSAGE_QUESTION);
	image = ctk_image_new_from_icon_name ("dialog-question",
					  CTK_ICON_SIZE_DIALOG);
	label = ctk_label_new (NULL);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been modified by an external application."
				  "\nWould you like to reload it?"), eoc_image_get_caption (img));
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	ctk_label_set_markup (CTK_LABEL (label), markup);
	g_free (text);
	g_free (markup);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
	ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);
	ctk_widget_set_halign (image, CTK_ALIGN_START);
	ctk_widget_set_valign (image, CTK_ALIGN_END);
	ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_box_pack_start (CTK_BOX (ctk_info_bar_get_content_area (CTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	ctk_widget_show_all (hbox);
	ctk_widget_show (info_bar);

	eoc_window_set_message_area (window, info_bar);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (file_changed_info_bar_response), window);
}

static void
eoc_window_display_image (EocWindow *window, EocImage *image)
{
	EocWindowPrivate *priv;
	GFile *file;

	g_return_if_fail (EOC_IS_WINDOW (window));
	g_return_if_fail (EOC_IS_IMAGE (image));

	eoc_debug (DEBUG_WINDOW);

	g_assert (eoc_image_has_data (image, EOC_IMAGE_DATA_IMAGE));

	priv = window->priv;

	if (image != NULL) {
		g_signal_connect (image,
				  "thumbnail_changed",
				  G_CALLBACK (image_thumb_changed_cb),
				  window);
		g_signal_connect (image, "file-changed",
				  G_CALLBACK (image_file_changed_cb),
				  window);

		image_thumb_changed_cb (image, window);
	}

	priv->needs_reload_confirmation = TRUE;

	eoc_scroll_view_set_image (EOC_SCROLL_VIEW (priv->view), image);

	ctk_window_set_title (CTK_WINDOW (window), eoc_image_get_caption (image));

	update_status_bar (window);

	file = eoc_image_get_file (image);
	g_idle_add_full (G_PRIORITY_LOW,
			 (GSourceFunc) add_file_to_recent_files,
			 file,
			 (GDestroyNotify) g_object_unref);

	eoc_window_update_openwith_menu (window, image);
}

static void
open_with_launch_application_cb (CtkAction *action, gpointer data) {
	EocImage *image;
	GAppInfo *app;
	GFile *file;
	GList *files = NULL;

	image = EOC_IMAGE (data);
	file = eoc_image_get_file (image);

	app = g_object_get_data (G_OBJECT (action), "app");
	files = g_list_append (files, file);
	g_app_info_launch (app,
			   files,
			   NULL, NULL);

	g_object_unref (file);
	g_list_free (files);
}

static void
eoc_window_update_openwith_menu (EocWindow *window, EocImage *image)
{
	gboolean edit_button_active;
	GAppInfo *editor_app;
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	gchar *label, *tip;
	const gchar *mime_type;
	CtkAction *action;
	EocWindowPrivate *priv;
	GList *apps;
	guint action_id = 0;
	GIcon *app_icon;
	char *path;
	CtkWidget *menuitem;

	priv = window->priv;

	edit_button_active = FALSE;
	editor_app = get_appinfo_for_editor (window);

	file = eoc_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);

	if (file_info == NULL)
		return;
	else {
		mime_type = g_file_info_get_content_type (file_info);
	}

	if (priv->open_with_menu_id != 0) {
		ctk_ui_manager_remove_ui (priv->ui_mgr, priv->open_with_menu_id);
		priv->open_with_menu_id = 0;
	}

	if (priv->actions_open_with != NULL) {
		ctk_ui_manager_remove_action_group (priv->ui_mgr, priv->actions_open_with);
		g_object_unref (priv->actions_open_with);
		priv->actions_open_with = NULL;
	}

	if (mime_type == NULL) {
		g_object_unref (file_info);
		return;
	}

	apps = g_app_info_get_all_for_type (mime_type);

	g_object_unref (file_info);

	if (!apps)
		return;

	priv->actions_open_with = ctk_action_group_new ("OpenWithActions");
	ctk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_open_with, -1);

	priv->open_with_menu_id = ctk_ui_manager_new_merge_id (priv->ui_mgr);

	for (iter = apps; iter; iter = iter->next) {
		GAppInfo *app = iter->data;
		gchar name[64];

		if (editor_app != NULL && g_app_info_equal (editor_app, app)) {
			edit_button_active = TRUE;
		}

		/* Do not include eoc itself */
		if (g_ascii_strcasecmp (g_app_info_get_executable (app),
				                g_get_prgname ()) == 0) {
			g_object_unref (app);
			continue;
		}

		g_snprintf (name, sizeof (name), "OpenWith%u", action_id++);

		label = g_strdup (g_app_info_get_name (app));
		tip = g_strdup_printf (_("Use \"%s\" to open the selected image"), g_app_info_get_name (app));

		action = ctk_action_new (name, label, tip, NULL);

		app_icon = g_app_info_get_icon (app);
		if (G_LIKELY (app_icon != NULL)) {
			g_object_ref (app_icon);
			ctk_action_set_gicon (action, app_icon);
			g_object_unref (app_icon);
		}

		g_free (label);
		g_free (tip);

		g_object_set_data_full (G_OBJECT (action), "app", app,
				                (GDestroyNotify) g_object_unref);

		g_signal_connect (action,
				          "activate",
				          G_CALLBACK (open_with_launch_application_cb),
				          image);

		ctk_action_group_add_action (priv->actions_open_with, action);
		g_object_unref (action);

		ctk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/MainMenu/Image/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        CTK_UI_MANAGER_MENUITEM,
				        FALSE);

		ctk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/ThumbnailPopup/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        CTK_UI_MANAGER_MENUITEM,
				        FALSE);
		ctk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/ViewPopup/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        CTK_UI_MANAGER_MENUITEM,
				        FALSE);

		path = g_strdup_printf ("/MainMenu/Image/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = ctk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);

		path = g_strdup_printf ("/ThumbnailPopup/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = ctk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);

		path = g_strdup_printf ("/ViewPopup/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = ctk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);
	}

	g_list_free (apps);

	action = ctk_action_group_get_action (window->priv->actions_image,
		                                  "OpenEditor");
	if (action != NULL) {
		ctk_action_set_sensitive (action, edit_button_active);
	}
}

static void
eoc_window_clear_load_job (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;

	if (priv->load_job != NULL) {
		if (!priv->load_job->finished)
			eoc_job_queue_remove_job (priv->load_job);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      eoc_job_progress_cb,
						      window);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      eoc_job_load_cb,
						      window);

		eoc_image_cancel_load (EOC_JOB_LOAD (priv->load_job)->image);

		g_object_unref (priv->load_job);
		priv->load_job = NULL;

		/* Hide statusbar */
		eoc_statusbar_set_progress (EOC_STATUSBAR (priv->statusbar), 0);
	}
}

static void
eoc_job_progress_cb (EocJobLoad *job G_GNUC_UNUSED,
		     float       progress,
		     gpointer    user_data)
{
	EocWindow *window;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

	eoc_statusbar_set_progress (EOC_STATUSBAR (window->priv->statusbar),
				    progress);
}

static void
eoc_job_save_progress_cb (EocJobSave *job, float progress, gpointer user_data)
{
	EocWindowPrivate *priv;
	EocWindow *window;

	static EocImage *image = NULL;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	eoc_statusbar_set_progress (EOC_STATUSBAR (priv->statusbar),
				    progress);

	if (image != job->current_image) {
		gchar *str_image, *status_message;
		guint n_images;

		image = job->current_image;

		n_images = g_list_length (job->images);

		str_image = eoc_image_get_uri_for_display (image);

		/* Translators: This string is displayed in the statusbar
		 * while saving images. The tokens are from left to right:
		 * - the original filename
		 * - the current image's position in the queue
		 * - the total number of images queued for saving */
		status_message = g_strdup_printf (_("Saving image \"%s\" (%u/%u)"),
					          str_image,
						  job->current_pos + 1,
						  n_images);
		g_free (str_image);

		ctk_statusbar_pop (CTK_STATUSBAR (priv->statusbar),
				   priv->image_info_message_cid);

		ctk_statusbar_push (CTK_STATUSBAR (priv->statusbar),
				    priv->image_info_message_cid,
				    status_message);

		g_free (status_message);
	}

	if (progress == 1.0)
		image = NULL;
}

static void
eoc_window_obtain_desired_size (EocImage  *image G_GNUC_UNUSED,
				gint       width,
				gint       height,
				EocWindow *window)
{
	CdkScreen *screen;
	CdkDisplay *display;
	CdkRectangle monitor;
	CtkAllocation allocation;
	gint final_width, final_height;
	gint screen_width, screen_height;
	gint window_width, window_height;
	gint img_width, img_height;
	gint view_width, view_height;
	gint deco_width, deco_height;

	update_action_groups_state (window);

	img_width = width;
	img_height = height;

	if (!ctk_widget_get_realized (window->priv->view)) {
		ctk_widget_realize (window->priv->view);
	}

	ctk_widget_get_allocation (window->priv->view, &allocation);
	view_width  = allocation.width;
	view_height = allocation.height;

	if (!ctk_widget_get_realized (CTK_WIDGET (window))) {
		ctk_widget_realize (CTK_WIDGET (window));
	}

	ctk_widget_get_allocation (CTK_WIDGET (window), &allocation);
	window_width  = allocation.width;
	window_height = allocation.height;

	screen = ctk_window_get_screen (CTK_WINDOW (window));
	display = cdk_screen_get_display (screen);

	cdk_monitor_get_geometry (cdk_display_get_monitor_at_window (display,
								     ctk_widget_get_window (CTK_WIDGET (window))),
				  &monitor);

	screen_width  = monitor.width;
	screen_height = monitor.height;

	deco_width = window_width - view_width;
	deco_height = window_height - view_height;

	if (img_width > 0 && img_height > 0) {
		if ((img_width + deco_width > screen_width) ||
		    (img_height + deco_height > screen_height))
		{
			double factor;

			if (img_width > img_height) {
				factor = (screen_width * 0.75 - deco_width) / (double) img_width;
			} else {
				factor = (screen_height * 0.75 - deco_height) / (double) img_height;
			}

			img_width = img_width * factor;
			img_height = img_height * factor;
		}
	}

	final_width = MAX (EOC_WINDOW_MIN_WIDTH, img_width + deco_width);
	final_height = MAX (EOC_WINDOW_MIN_HEIGHT, img_height + deco_height);

	eoc_debug_message (DEBUG_WINDOW, "Setting window size: %d x %d", final_width, final_height);

	ctk_window_set_default_size (CTK_WINDOW (window), final_width, final_height);

	g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
}

static void
eoc_window_error_message_area_response (CtkInfoBar *message_area G_GNUC_UNUSED,
					gint        response_id,
					EocWindow  *window)
{
	if (response_id != CTK_RESPONSE_OK) {
		eoc_window_set_message_area (window, NULL);

		return;
	}

	/* Trigger loading for current image again */
	eoc_thumb_view_select_single (EOC_THUMB_VIEW (window->priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_CURRENT);
}

static void
eoc_job_load_cb (EocJobLoad *job, gpointer data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
	CtkAction *action_undo, *action_save;

	g_return_if_fail (EOC_IS_WINDOW (data));

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (data);
	priv = window->priv;

	eoc_statusbar_set_progress (EOC_STATUSBAR (priv->statusbar), 0.0);

	ctk_statusbar_pop (CTK_STATUSBAR (window->priv->statusbar),
			   priv->image_info_message_cid);

	if (priv->image != NULL) {
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);

		g_object_unref (priv->image);
	}

	priv->image = g_object_ref (job->image);

	if (EOC_JOB (job)->error == NULL) {
#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
		eoc_image_apply_display_profile (job->image,
						 priv->display_profile);
#endif

		ctk_action_group_set_sensitive (priv->actions_image, TRUE);

		eoc_window_display_image (window, job->image);
	} else {
		CtkWidget *message_area;

		message_area = eoc_image_load_error_message_area_new (
					eoc_image_get_caption (job->image),
					EOC_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (eoc_window_error_message_area_response),
				  window);

		ctk_window_set_icon (CTK_WINDOW (window), NULL);
		ctk_window_set_title (CTK_WINDOW (window),
				      eoc_image_get_caption (job->image));

		eoc_window_set_message_area (window, message_area);

		ctk_info_bar_set_default_response (CTK_INFO_BAR (message_area),
						   CTK_RESPONSE_CANCEL);

		ctk_widget_show (message_area);

		update_status_bar (window);

		eoc_scroll_view_set_image (EOC_SCROLL_VIEW (priv->view), NULL);

        	if (window->priv->status == EOC_WINDOW_STATUS_INIT) {
			update_action_groups_state (window);

			g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
		}

		ctk_action_group_set_sensitive (priv->actions_image, FALSE);
	}

	eoc_window_clear_load_job (window);

	if (window->priv->status == EOC_WINDOW_STATUS_INIT) {
		window->priv->status = EOC_WINDOW_STATUS_NORMAL;

		g_signal_handlers_disconnect_by_func
			(job->image,
			 G_CALLBACK (eoc_window_obtain_desired_size),
			 window);
	}

	action_save = ctk_action_group_get_action (priv->actions_image, "ImageSave");
	action_undo = ctk_action_group_get_action (priv->actions_image, "EditUndo");

	/* Set Save and Undo sensitive according to image state.
	 * Respect lockdown in case of Save.*/
	ctk_action_set_sensitive (action_save, (!priv->save_disabled && eoc_image_is_modified (job->image)));
	ctk_action_set_sensitive (action_undo, eoc_image_is_modified (job->image));

	g_object_unref (job->image);
}

static void
eoc_window_clear_transform_job (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;

	if (priv->transform_job != NULL) {
		if (!priv->transform_job->finished)
			eoc_job_queue_remove_job (priv->transform_job);

		g_signal_handlers_disconnect_by_func (priv->transform_job,
						      eoc_job_transform_cb,
						      window);
		g_object_unref (priv->transform_job);
		priv->transform_job = NULL;
	}
}

static void
eoc_job_transform_cb (EocJobTransform *job G_GNUC_UNUSED,
		      gpointer         data)
{
	EocWindow *window;
	CtkAction *action_undo, *action_save;
	EocImage *image;

	g_return_if_fail (EOC_IS_WINDOW (data));

	window = EOC_WINDOW (data);

	eoc_window_clear_transform_job (window);

	action_undo =
		ctk_action_group_get_action (window->priv->actions_image, "EditUndo");
	action_save =
		ctk_action_group_get_action (window->priv->actions_image, "ImageSave");

	image = eoc_window_get_image (window);

	ctk_action_set_sensitive (action_undo, eoc_image_is_modified (image));

	if (!window->priv->save_disabled)
	{
		ctk_action_set_sensitive (action_save, eoc_image_is_modified (image));
	}
}

static void
apply_transformation (EocWindow *window, EocTransform *trans)
{
	EocWindowPrivate *priv;
	GList *images;

	g_return_if_fail (EOC_IS_WINDOW (window));

	priv = window->priv;

	images = eoc_thumb_view_get_selected_images (EOC_THUMB_VIEW (priv->thumbview));

	eoc_window_clear_transform_job (window);

	priv->transform_job = eoc_job_transform_new (images, trans);

	g_signal_connect (priv->transform_job,
			  "finished",
			  G_CALLBACK (eoc_job_transform_cb),
			  window);

	g_signal_connect (priv->transform_job,
			  "progress",
			  G_CALLBACK (eoc_job_progress_cb),
			  window);

	eoc_job_queue_add_job (priv->transform_job);
}

static void
handle_image_selection_changed_cb (EocThumbView *thumbview G_GNUC_UNUSED,
				   EocWindow    *window)
{
	EocWindowPrivate *priv;
	EocImage *image;
	gchar *status_message;
	gchar *str_image;

	priv = window->priv;

	if (eoc_list_store_length (EOC_LIST_STORE (priv->store)) == 0) {
		ctk_window_set_title (CTK_WINDOW (window),
				      g_get_application_name());
		ctk_statusbar_remove_all (CTK_STATUSBAR (priv->statusbar),
					  priv->image_info_message_cid);
		eoc_scroll_view_set_image (EOC_SCROLL_VIEW (priv->view),
					   NULL);
	}

	if (eoc_thumb_view_get_n_selected (EOC_THUMB_VIEW (priv->thumbview)) == 0)
		return;

	update_selection_ui_visibility (window);

	image = eoc_thumb_view_get_first_selected_image (EOC_THUMB_VIEW (priv->thumbview));

	g_assert (EOC_IS_IMAGE (image));

	eoc_window_clear_load_job (window);

	eoc_window_set_message_area (window, NULL);

	ctk_statusbar_pop (CTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	if (image == priv->image) {
		update_status_bar (window);
		return;
	}

	if (eoc_image_has_data (image, EOC_IMAGE_DATA_IMAGE)) {
		if (priv->image != NULL)
			g_object_unref (priv->image);

		priv->image = image;
		eoc_window_display_image (window, image);
		return;
	}

	if (priv->status == EOC_WINDOW_STATUS_INIT) {
		g_signal_connect (image,
				  "size-prepared",
				  G_CALLBACK (eoc_window_obtain_desired_size),
				  window);
	}

	priv->load_job = eoc_job_load_new (image, EOC_IMAGE_DATA_ALL);

	g_signal_connect (priv->load_job,
			  "finished",
			  G_CALLBACK (eoc_job_load_cb),
			  window);

	g_signal_connect (priv->load_job,
			  "progress",
			  G_CALLBACK (eoc_job_progress_cb),
			  window);

	eoc_job_queue_add_job (priv->load_job);

	str_image = eoc_image_get_uri_for_display (image);

	status_message = g_strdup_printf (_("Opening image \"%s\""),
				          str_image);

	g_free (str_image);

	ctk_statusbar_push (CTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, status_message);

	g_free (status_message);
}

static void
view_zoom_changed_cb (CtkWidget *widget G_GNUC_UNUSED,
		      double     zoom G_GNUC_UNUSED,
		      gpointer   user_data)
{
	EocWindow *window;
	CtkAction *action_zoom_in;
	CtkAction *action_zoom_out;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

	update_status_bar (window);

	action_zoom_in =
		ctk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomIn");

	action_zoom_out =
		ctk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomOut");

	ctk_action_set_sensitive (action_zoom_in,
			!eoc_scroll_view_get_zoom_is_max (EOC_SCROLL_VIEW (window->priv->view)));
	ctk_action_set_sensitive (action_zoom_out,
			!eoc_scroll_view_get_zoom_is_min (EOC_SCROLL_VIEW (window->priv->view)));
}

static void
eoc_window_open_recent_cb (CtkAction *action,
			   EocWindow *window G_GNUC_UNUSED)
{
	CtkRecentInfo *info;
	const gchar *uri;
	GSList *list = NULL;

	info = g_object_get_data (G_OBJECT (action), "ctk-recent-info");
	g_return_if_fail (info != NULL);

	uri = ctk_recent_info_get_uri (info);
	list = g_slist_prepend (list, g_strdup (uri));

	eoc_application_open_uri_list (EOC_APP,
				       list,
				       CDK_CURRENT_TIME,
				       0,
				       NULL);

	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}

static void
file_open_dialog_response_cb (CtkWidget *chooser,
			      gint       response_id,
			      EocWindow *ev_window G_GNUC_UNUSED)
{
	if (response_id == CTK_RESPONSE_OK) {
		GSList *uris;

		uris = ctk_file_chooser_get_uris (CTK_FILE_CHOOSER (chooser));

		eoc_application_open_uri_list (EOC_APP,
					       uris,
					       CDK_CURRENT_TIME,
					       0,
					       NULL);

		g_slist_foreach (uris, (GFunc) g_free, NULL);
		g_slist_free (uris);
	}

	ctk_widget_destroy (chooser);
}

static void
eoc_window_update_fullscreen_action (EocWindow *window)
{
	CtkAction *action;

	action = ctk_action_group_get_action (window->priv->actions_image,
					      "ViewFullscreen");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eoc_window_cmd_fullscreen), window);

	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
				      window->priv->mode == EOC_WINDOW_MODE_FULLSCREEN);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eoc_window_cmd_fullscreen), window);
}

static void
eoc_window_update_slideshow_action (EocWindow *window)
{
	CtkAction *action;

	action = ctk_action_group_get_action (window->priv->actions_collection,
					      "ViewSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eoc_window_cmd_slideshow), window);

	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
				      window->priv->mode == EOC_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eoc_window_cmd_slideshow), window);
}

static void
eoc_window_update_pause_slideshow_action (EocWindow *window)
{
	CtkAction *action;

	action = ctk_action_group_get_action (window->priv->actions_image,
					      "PauseSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eoc_window_cmd_pause_slideshow), window);

	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
				      window->priv->mode != EOC_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eoc_window_cmd_pause_slideshow), window);
}

static void
eoc_window_update_fullscreen_popup (EocWindow *window)
{
	CtkWidget *popup = window->priv->fullscreen_popup;
	CdkRectangle screen_rect;
	CdkScreen *screen;
	CdkDisplay *display;

	g_return_if_fail (popup != NULL);

	if (ctk_widget_get_window (CTK_WIDGET (window)) == NULL) return;

	screen = ctk_widget_get_screen (CTK_WIDGET (window));
	display = cdk_screen_get_display (screen);

	cdk_monitor_get_geometry (cdk_display_get_monitor_at_window (display,
								     ctk_widget_get_window (CTK_WIDGET (window))),
				  &screen_rect);

	ctk_widget_set_size_request (popup,
				     screen_rect.width,
				     -1);

	ctk_window_move (CTK_WINDOW (popup), screen_rect.x, screen_rect.y);
}

static void
screen_size_changed_cb (CdkScreen *screen G_GNUC_UNUSED,
			EocWindow *window)
{
	eoc_window_update_fullscreen_popup (window);
}

static gboolean
fullscreen_timeout_cb (gpointer data)
{
	EocWindow *window = EOC_WINDOW (data);

	ctk_widget_hide (window->priv->fullscreen_popup);

	eoc_scroll_view_hide_cursor (EOC_SCROLL_VIEW (window->priv->view));

	fullscreen_clear_timeout (window);

	return FALSE;
}

static gboolean
slideshow_is_loop_end (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;
	EocImage *image = NULL;
	gint pos;

	image = eoc_thumb_view_get_first_selected_image (EOC_THUMB_VIEW (priv->thumbview));

	pos = eoc_list_store_get_pos_by_image (priv->store, image);

	return (pos == (eoc_list_store_length (priv->store) - 1));
}

static gboolean
slideshow_switch_cb (gpointer data)
{
	EocWindow *window = EOC_WINDOW (data);
	EocWindowPrivate *priv = window->priv;

	eoc_debug (DEBUG_WINDOW);

	if (priv->slideshow_random) {
		eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
					      EOC_THUMB_VIEW_SELECT_RANDOM);
		return TRUE;
	}

	if (!priv->slideshow_loop && slideshow_is_loop_end (window)) {
		eoc_window_stop_fullscreen (window, TRUE);
		return FALSE;
	}

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_RIGHT);

	return TRUE;
}

static void
fullscreen_clear_timeout (EocWindow *window)
{
	eoc_debug (DEBUG_WINDOW);

	if (window->priv->fullscreen_timeout_source != NULL) {
		g_source_unref (window->priv->fullscreen_timeout_source);
		g_source_destroy (window->priv->fullscreen_timeout_source);
	}

	window->priv->fullscreen_timeout_source = NULL;
}

static void
fullscreen_set_timeout (EocWindow *window)
{
	GSource *source;

	eoc_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	source = g_timeout_source_new (EOC_WINDOW_FULLSCREEN_TIMEOUT);
	g_source_set_callback (source, fullscreen_timeout_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->fullscreen_timeout_source = source;

	eoc_scroll_view_show_cursor (EOC_SCROLL_VIEW (window->priv->view));
}

static void
slideshow_clear_timeout (EocWindow *window)
{
	eoc_debug (DEBUG_WINDOW);

	if (window->priv->slideshow_switch_source != NULL) {
		g_source_unref (window->priv->slideshow_switch_source);
		g_source_destroy (window->priv->slideshow_switch_source);
	}

	window->priv->slideshow_switch_source = NULL;
}

static void
slideshow_set_timeout (EocWindow *window)
{
	GSource *source;

	eoc_debug (DEBUG_WINDOW);

	slideshow_clear_timeout (window);

	if (window->priv->slideshow_switch_timeout <= 0)
		return;

	source = g_timeout_source_new (window->priv->slideshow_switch_timeout * 1000);
	g_source_set_callback (source, slideshow_switch_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->slideshow_switch_source = source;
}

static void
show_fullscreen_popup (EocWindow *window)
{
	eoc_debug (DEBUG_WINDOW);

	if (!ctk_widget_get_visible (window->priv->fullscreen_popup)) {
		ctk_widget_show_all (CTK_WIDGET (window->priv->fullscreen_popup));
	}

	fullscreen_set_timeout (window);
}

static gboolean
fullscreen_motion_notify_cb (CtkWidget      *widget G_GNUC_UNUSED,
			     CdkEventMotion *event,
			     gpointer        user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	eoc_debug (DEBUG_WINDOW);

	if (event->y < EOC_WINDOW_FULLSCREEN_POPUP_THRESHOLD) {
		show_fullscreen_popup (window);
	} else {
		fullscreen_set_timeout (window);
	}

	return FALSE;
}

static gboolean
fullscreen_leave_notify_cb (CtkWidget        *widget G_GNUC_UNUSED,
			    CdkEventCrossing *event G_GNUC_UNUSED,
			    gpointer          user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	eoc_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	return FALSE;
}

static void
exit_fullscreen_button_clicked_cb (CtkWidget *button G_GNUC_UNUSED,
				   EocWindow *window)
{
	CtkAction *action;

	eoc_debug (DEBUG_WINDOW);

	if (window->priv->mode == EOC_WINDOW_MODE_SLIDESHOW) {
		action = ctk_action_group_get_action (window->priv->actions_collection,
						      "ViewSlideshow");
	} else {
		action = ctk_action_group_get_action (window->priv->actions_image,
						      "ViewFullscreen");
	}
	g_return_if_fail (action != NULL);

	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), FALSE);
}

static CtkWidget *
eoc_window_get_exit_fullscreen_button (EocWindow *window)
{
	CtkWidget *button;

	button = ctk_button_new_with_mnemonic (_("Leave Fullscreen"));
	ctk_button_set_image (CTK_BUTTON (button), ctk_image_new_from_icon_name ("view-restore", CTK_ICON_SIZE_BUTTON));

	g_signal_connect (button, "clicked",
			  G_CALLBACK (exit_fullscreen_button_clicked_cb),
			  window);

	return button;
}

static CtkWidget *
eoc_window_create_fullscreen_popup (EocWindow *window)
{
	CtkWidget *popup;
	CtkWidget *hbox;
	CtkWidget *button;
	CtkWidget *toolbar;
	CdkScreen *screen;

	eoc_debug (DEBUG_WINDOW);

	popup = ctk_window_new (CTK_WINDOW_POPUP);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (popup), hbox);

	toolbar = ctk_ui_manager_get_widget (window->priv->ui_mgr,
					     "/FullscreenToolbar");
	g_assert (CTK_IS_WIDGET (toolbar));
	ctk_toolbar_set_style (CTK_TOOLBAR (toolbar), CTK_TOOLBAR_ICONS);
	ctk_box_pack_start (CTK_BOX (hbox), toolbar, TRUE, TRUE, 0);

	button = eoc_window_get_exit_fullscreen_button (window);
	ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

	ctk_window_set_resizable (CTK_WINDOW (popup), FALSE);

	screen = ctk_widget_get_screen (CTK_WIDGET (window));

	g_signal_connect_object (screen, "size-changed",
			         G_CALLBACK (screen_size_changed_cb),
				 window, 0);

	g_signal_connect (popup,
			  "enter-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	ctk_window_set_screen (CTK_WINDOW (popup), screen);

	return popup;
}

static void
update_ui_visibility (EocWindow *window)
{
	EocWindowPrivate *priv;

	CtkAction *action;
	CtkWidget *menubar;

	gboolean fullscreen_mode, visible;

	g_return_if_fail (EOC_IS_WINDOW (window));

	eoc_debug (DEBUG_WINDOW);

	priv = window->priv;

	fullscreen_mode = priv->mode == EOC_WINDOW_MODE_FULLSCREEN ||
			  priv->mode == EOC_WINDOW_MODE_SLIDESHOW;

	menubar = ctk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (CTK_IS_WIDGET (menubar));

	visible = g_settings_get_boolean (priv->ui_settings, EOC_CONF_UI_TOOLBAR);
	visible = visible && !fullscreen_mode;

	action = ctk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ToolbarToggle");
	g_assert (action != NULL);
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

	visible = g_settings_get_boolean (priv->ui_settings, EOC_CONF_UI_STATUSBAR);
	visible = visible && !fullscreen_mode;

	action = ctk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/StatusbarToggle");
	g_assert (action != NULL);
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

	if (priv->status != EOC_WINDOW_STATUS_INIT) {
		visible = g_settings_get_boolean (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION);
		visible = visible && priv->mode != EOC_WINDOW_MODE_SLIDESHOW;
		action = ctk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ImageCollectionToggle");
		g_assert (action != NULL);
		ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), visible);
		if (visible) {
			ctk_widget_show (priv->nav);
		} else {
			ctk_widget_hide (priv->nav);
		}
	}

	visible = g_settings_get_boolean (priv->ui_settings, EOC_CONF_UI_SIDEBAR);
	visible = visible && !fullscreen_mode;
	action = ctk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/SidebarToggle");
	g_assert (action != NULL);
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), visible);
	if (visible) {
		ctk_widget_show (priv->sidebar);
	} else {
		ctk_widget_hide (priv->sidebar);
	}

	if (priv->fullscreen_popup != NULL) {
		ctk_widget_hide (priv->fullscreen_popup);
	}
}

static void
eoc_window_inhibit_screensaver (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;

	g_return_if_fail (priv->fullscreen_idle_inhibit_cookie == 0);

	eoc_debug (DEBUG_WINDOW);

	window->priv->fullscreen_idle_inhibit_cookie =
		ctk_application_inhibit (CTK_APPLICATION (EOC_APP),
		                         CTK_WINDOW (window),
		                         CTK_APPLICATION_INHIBIT_IDLE,
		                         _("Viewing a slideshow"));
}

static void
eoc_window_uninhibit_screensaver (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;

	if (G_UNLIKELY (priv->fullscreen_idle_inhibit_cookie == 0))
		return;

	eoc_debug (DEBUG_WINDOW);

	ctk_application_uninhibit (CTK_APPLICATION (EOC_APP),
	                           priv->fullscreen_idle_inhibit_cookie);
	priv->fullscreen_idle_inhibit_cookie = 0;
}

static void
eoc_window_run_fullscreen (EocWindow *window, gboolean slideshow)
{
	static const CdkRGBA black = { 0., 0., 0., 1.};

	EocWindowPrivate *priv;
	CtkWidget *menubar;
	gboolean upscale;

	eoc_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (slideshow) {
		priv->mode = EOC_WINDOW_MODE_SLIDESHOW;
	} else {
		/* Stop the timer if we come from slideshowing */
		if (priv->mode == EOC_WINDOW_MODE_SLIDESHOW)
			slideshow_clear_timeout (window);

		priv->mode = EOC_WINDOW_MODE_FULLSCREEN;
	}

	if (window->priv->fullscreen_popup == NULL)
		priv->fullscreen_popup
			= eoc_window_create_fullscreen_popup (window);

	update_ui_visibility (window);

	menubar = ctk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (CTK_IS_WIDGET (menubar));
	ctk_widget_hide (menubar);

	g_signal_connect (priv->view,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->view,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	fullscreen_set_timeout (window);

	if (slideshow) {
		priv->slideshow_random =
				g_settings_get_boolean (priv->fullscreen_settings,
						       EOC_CONF_FULLSCREEN_RANDOM);

		priv->slideshow_loop =
				g_settings_get_boolean (priv->fullscreen_settings,
						       EOC_CONF_FULLSCREEN_LOOP);

		priv->slideshow_switch_timeout =
				g_settings_get_int (priv->fullscreen_settings,
						      EOC_CONF_FULLSCREEN_SECONDS);

		slideshow_set_timeout (window);
	}

	upscale = g_settings_get_boolean (priv->fullscreen_settings,
					 EOC_CONF_FULLSCREEN_UPSCALE);

	eoc_scroll_view_set_zoom_upscale (EOC_SCROLL_VIEW (priv->view),
					  upscale);

	ctk_widget_grab_focus (priv->view);

	eoc_scroll_view_override_bg_color (EOC_SCROLL_VIEW (window->priv->view),
	                                   &black);

	ctk_window_fullscreen (CTK_WINDOW (window));
	eoc_window_update_fullscreen_popup (window);

	eoc_window_inhibit_screensaver (window);

	/* Update both actions as we could've already been in one those modes */
	eoc_window_update_slideshow_action (window);
	eoc_window_update_fullscreen_action (window);
	eoc_window_update_pause_slideshow_action (window);
}

static void
eoc_window_stop_fullscreen (EocWindow *window, gboolean slideshow)
{
	EocWindowPrivate *priv;
	CtkWidget *menubar;

	eoc_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->mode != EOC_WINDOW_MODE_SLIDESHOW &&
	    priv->mode != EOC_WINDOW_MODE_FULLSCREEN) return;

	priv->mode = EOC_WINDOW_MODE_NORMAL;

	fullscreen_clear_timeout (window);

	if (slideshow) {
		slideshow_clear_timeout (window);
	}

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	update_ui_visibility (window);

	menubar = ctk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (CTK_IS_WIDGET (menubar));
	ctk_widget_show (menubar);

	eoc_scroll_view_set_zoom_upscale (EOC_SCROLL_VIEW (priv->view), FALSE);

	eoc_scroll_view_override_bg_color (EOC_SCROLL_VIEW (window->priv->view),
					   NULL);
	ctk_window_unfullscreen (CTK_WINDOW (window));

	if (slideshow) {
		eoc_window_update_slideshow_action (window);
	} else {
		eoc_window_update_fullscreen_action (window);
	}

	eoc_scroll_view_show_cursor (EOC_SCROLL_VIEW (priv->view));

	eoc_window_uninhibit_screensaver (window);
}

static void
eoc_window_print (EocWindow *window)
{
	CtkWidget *dialog;
	GError *error = NULL;
	CtkPrintOperation *print;
	CtkPrintOperationResult res;
	CtkPageSetup *page_setup;
	CtkPrintSettings *print_settings;
	gboolean page_setup_disabled = FALSE;

	eoc_debug (DEBUG_PRINTING);

	print_settings = eoc_print_get_print_settings ();

	/* Make sure the window stays valid while printing */
	g_object_ref (window);

	if (window->priv->page_setup != NULL)
		page_setup = g_object_ref (window->priv->page_setup);
	else
		page_setup = eoc_print_get_page_setup ();

	print = eoc_print_operation_new (window->priv->image,
					 print_settings,
					 page_setup);

	// Disable page setup options if they are locked down
	page_setup_disabled = g_settings_get_boolean (window->priv->lockdown_settings,
						      EOC_CONF_LOCKDOWN_CAN_SETUP_PAGE);
	if (page_setup_disabled)
		ctk_print_operation_set_embed_page_setup (print, FALSE);

	res = ctk_print_operation_run (print,
				       CTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       CTK_WINDOW (window), &error);

	if (res == CTK_PRINT_OPERATION_RESULT_ERROR) {
		dialog = ctk_message_dialog_new (CTK_WINDOW (window),
						 CTK_DIALOG_DESTROY_WITH_PARENT,
						 CTK_MESSAGE_ERROR,
						 CTK_BUTTONS_CLOSE,
						 _("Error printing file:\n%s"),
						 error->message);
		g_signal_connect (dialog, "response",
				  G_CALLBACK (ctk_widget_destroy), NULL);
		ctk_widget_show (dialog);
		g_error_free (error);
	} else if (res == CTK_PRINT_OPERATION_RESULT_APPLY) {
		CtkPageSetup *new_page_setup;
		eoc_print_set_print_settings (ctk_print_operation_get_print_settings (print));
		new_page_setup = ctk_print_operation_get_default_page_setup (print);
		if (window->priv->page_setup != NULL)
			g_object_unref (window->priv->page_setup);
		window->priv->page_setup = g_object_ref (new_page_setup);
		eoc_print_set_page_setup (window->priv->page_setup);
	}

	if (page_setup != NULL)
		g_object_unref (page_setup);
	g_object_unref (print_settings);
	g_object_unref (window);
}

static void
eoc_window_cmd_file_open (CtkAction *action G_GNUC_UNUSED,
			  gpointer   user_data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
        EocImage *current;
	CtkWidget *dlg;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

        priv = window->priv;

	dlg = eoc_file_chooser_new (CTK_FILE_CHOOSER_ACTION_OPEN);
	ctk_window_set_transient_for (CTK_WINDOW (dlg), CTK_WINDOW (window));

	current = eoc_thumb_view_get_first_selected_image (EOC_THUMB_VIEW (priv->thumbview));

	if (current != NULL) {
		gchar *dir_uri, *file_uri;

		file_uri = eoc_image_get_uri_for_display (current);
		dir_uri = g_path_get_dirname (file_uri);

	        ctk_file_chooser_set_current_folder_uri (CTK_FILE_CHOOSER (dlg),
                                                         dir_uri);
		g_free (file_uri);
		g_free (dir_uri);
		g_object_unref (current);
	} else {
		/* If desired by the user,
		   fallback to the XDG_PICTURES_DIR (if available) */
		const gchar *pics_dir;
		gboolean use_fallback;

		use_fallback = g_settings_get_boolean (priv->ui_settings,
					   EOC_CONF_UI_FILECHOOSER_XDG_FALLBACK);
		pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
		if (use_fallback && pics_dir) {
			ctk_file_chooser_set_current_folder (CTK_FILE_CHOOSER (dlg),
							     pics_dir);
		}
	}

	g_signal_connect (dlg, "response",
			  G_CALLBACK (file_open_dialog_response_cb),
			  window);

	ctk_widget_show_all (dlg);
}

static void
eoc_job_close_save_cb (EocJobSave *job, gpointer user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	g_signal_handlers_disconnect_by_func (job,
					      eoc_job_close_save_cb,
					      window);

	ctk_widget_destroy (CTK_WIDGET (window));
}

static void
close_confirmation_dialog_response_handler (EocCloseConfirmationDialog *dlg,
					    gint                        response_id,
					    EocWindow                  *window)
{
	GList *selected_images;
	EocWindowPrivate *priv;

	priv = window->priv;

	switch (response_id)
	{
		case CTK_RESPONSE_YES:
			/* save selected images */
			selected_images = eoc_close_confirmation_dialog_get_selected_images (dlg);
			if (eoc_window_save_images (window, selected_images)) {
				g_signal_connect (priv->save_job,
							  "finished",
							  G_CALLBACK (eoc_job_close_save_cb),
							  window);

				eoc_job_queue_add_job (priv->save_job);
			}

			break;

		case CTK_RESPONSE_NO:
			/* dont save */
			ctk_widget_destroy (CTK_WIDGET (window));
			break;

		default:
			/* Cancel */
			ctk_widget_destroy (CTK_WIDGET (dlg));
			break;
	}
}

static gboolean
eoc_window_unsaved_images_confirm (EocWindow *window)
{
	EocWindowPrivate *priv;
	gboolean disabled;
	CtkWidget *dialog;
	GList *list;
	EocImage *image;
	CtkTreeIter iter;

	priv = window->priv;

	disabled = g_settings_get_boolean(priv->ui_settings,
					EOC_CONF_UI_DISABLE_CLOSE_CONFIRMATION);
	disabled |= window->priv->save_disabled;
	if (disabled || !priv->store) {
		return FALSE;
	}

	list = NULL;
	if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (priv->store), &iter)) {
		do {
			ctk_tree_model_get (CTK_TREE_MODEL (priv->store), &iter,
					    EOC_LIST_STORE_EOC_IMAGE, &image,
					    -1);
			if (!image)
				continue;

			if (eoc_image_is_modified (image)) {
				list = g_list_prepend (list, image);
			}
		} while (ctk_tree_model_iter_next (CTK_TREE_MODEL (priv->store), &iter));
	}

	if (list) {
		list = g_list_reverse (list);
		dialog = eoc_close_confirmation_dialog_new (CTK_WINDOW (window),
							    list);

		g_list_free (list);
		g_signal_connect (dialog,
				  "response",
				  G_CALLBACK (close_confirmation_dialog_response_handler),
				  window);
		ctk_window_set_destroy_with_parent (CTK_WINDOW (dialog), TRUE);

		ctk_widget_show (dialog);
		return TRUE;

	}
	return FALSE;
}

static void
eoc_window_cmd_close_window (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
	EocWindow *window;
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	if (priv->save_job != NULL) {
		eoc_window_finish_saving (window);
	}

	if (!eoc_window_unsaved_images_confirm (window)) {
		ctk_widget_destroy (CTK_WIDGET (user_data));
	}
}

static void
eoc_window_cmd_preferences (CtkAction *action G_GNUC_UNUSED,
			    gpointer   user_data)
{
	EocWindow *window;
	CtkWidget *pref_dlg;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

	pref_dlg = eoc_preferences_dialog_get_instance (CTK_WINDOW (window));

	ctk_widget_show (pref_dlg);
}

#define EOC_TB_EDITOR_DLG_RESET_RESPONSE 128

static void
eoc_window_cmd_edit_toolbar_cb (CtkDialog *dialog, gint response, gpointer data)
{
	EocWindow *window = EOC_WINDOW (data);

	if (response == EOC_TB_EDITOR_DLG_RESET_RESPONSE) {
		EggToolbarsModel *model;
		EggToolbarEditor *editor;

		editor = g_object_get_data (G_OBJECT (dialog),
					    "EggToolbarEditor");

		g_return_if_fail (editor != NULL);

		egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		eoc_application_reset_toolbars_model (EOC_APP);
		model = eoc_application_get_toolbars_model (EOC_APP);
		egg_editable_toolbar_set_model
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), model);
		egg_toolbar_editor_set_model (editor, model);

		/* Toolbar would be uneditable now otherwise */
		egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);
	} else if (response == CTK_RESPONSE_HELP) {
		eoc_util_show_help ("eoc-toolbareditor", NULL);
	} else {
		egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		eoc_application_save_toolbars_model (EOC_APP);

		ctk_widget_destroy (CTK_WIDGET (dialog));
	}
}

static void
eoc_window_cmd_edit_toolbar (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
	EocWindow *window;
	CtkWidget *dialog;
	CtkWidget *editor;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

	dialog = ctk_dialog_new_with_buttons (_("Toolbar Editor"),
					      CTK_WINDOW (window),
				              CTK_DIALOG_DESTROY_WITH_PARENT,
					      _("_Reset to Default"),
					      EOC_TB_EDITOR_DLG_RESET_RESPONSE,
 					      "ctk-close",
					      CTK_RESPONSE_CLOSE,
					      "ctk-help",
					      CTK_RESPONSE_HELP,
					      NULL);

	ctk_dialog_set_default_response (CTK_DIALOG (dialog),
					 CTK_RESPONSE_CLOSE);

	ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);

	ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), 2);

	ctk_window_set_default_size (CTK_WINDOW (dialog), 500, 400);

	editor = egg_toolbar_editor_new (window->priv->ui_mgr,
					 eoc_application_get_toolbars_model (EOC_APP));

	ctk_container_set_border_width (CTK_CONTAINER (editor), 5);

	// Use as much vertical space as available
	ctk_widget_set_vexpand (CTK_WIDGET (editor), TRUE);

	ctk_box_set_spacing (CTK_BOX (EGG_TOOLBAR_EDITOR (editor)), 5);

	ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), editor);

	egg_editable_toolbar_set_edit_mode
		(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);

	g_object_set_data (G_OBJECT (dialog), "EggToolbarEditor", editor);

	g_signal_connect (dialog,
                          "response",
			  G_CALLBACK (eoc_window_cmd_edit_toolbar_cb),
			  window);

	ctk_widget_show_all (dialog);
}

static void
eoc_window_cmd_help (CtkAction *action G_GNUC_UNUSED,
		     gpointer   user_data)
{
	EocWindow *window;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);

	eoc_util_show_help (NULL, CTK_WINDOW (window));
}

#define ABOUT_GROUP "About"
#define EMAILIFY(string) (g_strdelimit ((string), "%", '@'))

static void
eoc_window_cmd_about (CtkAction *action G_GNUC_UNUSED,
		      gpointer   user_data)
{
	EocWindow *window;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	const char *license[] = {
		N_("This program is free software; you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation; either version 2 of the License, or "
		   "(at your option) any later version.\n"),
		N_("This program is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details.\n"),
		N_("You should have received a copy of the GNU General Public License "
		   "along with this program; if not, write to the Free Software "
		   "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.")
	};

	char *license_trans;
	GKeyFile *key_file;
	GBytes *bytes;
	const guint8 *data;
	gsize data_len;
	GError *error = NULL;
	char **authors, **documenters;
	gsize n_authors = 0, n_documenters = 0 , i;

	bytes = g_resources_lookup_data ("/org/cafe/eoc/ui/eoc.about", G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
	g_assert_no_error (error);

	data = g_bytes_get_data (bytes, &data_len);
	key_file = g_key_file_new ();
	g_key_file_load_from_data (key_file, (const char *) data, data_len, 0, &error);
	g_assert_no_error (error);

	authors = g_key_file_get_string_list (key_file, ABOUT_GROUP, "Authors", &n_authors, NULL);
	documenters = g_key_file_get_string_list (key_file, ABOUT_GROUP, "Documenters", &n_documenters, NULL);

	g_key_file_free (key_file);
	g_bytes_unref (bytes);

	for (i = 0; i < n_authors; ++i)
		authors[i] = EMAILIFY (authors[i]);
	for (i = 0; i < n_documenters; ++i)
		documenters[i] = EMAILIFY (documenters[i]);

	license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n", _(license[2]), "\n", NULL);

	window = EOC_WINDOW (user_data);

	ctk_show_about_dialog (CTK_WINDOW (window),
			       "program-name", _("Eye of CAFE"),
			       "title", _("About Eye of CAFE"),
			       "version", VERSION,
			       "copyright", _("Copyright \xc2\xa9 2000-2010 Free Software Foundation, Inc.\n"
			                      "Copyright \xc2\xa9 2011 Perberos\n"
			                      "Copyright \xc2\xa9 2012-2020 MATE developers\n"
			                      "Copyright \xc2\xa9 2023-2025 Pablo Barciela"),
			       "comments",_("The CAFE image viewer."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", _("translator-credits"),
			       "website", "http://www.cafe-desktop.org/",
			       "logo-icon-name", "eoc",
			       "wrap-license", TRUE,
			       "license", license_trans,
			       NULL);

	g_strfreev (authors);
	g_strfreev (documenters);
	g_free (license_trans);
}

static void
eoc_window_cmd_show_hide_bar (CtkAction *action, gpointer user_data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
	gboolean visible;
	const gchar *action_name;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	if (priv->mode != EOC_WINDOW_MODE_NORMAL &&
            priv->mode != EOC_WINDOW_MODE_FULLSCREEN) return;

	visible = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));
	action_name = ctk_action_get_name (action);

	if (g_ascii_strcasecmp (action_name, "ViewToolbar") == 0) {
		g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

		if (priv->mode == EOC_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings, EOC_CONF_UI_TOOLBAR, visible);

	} else if (g_ascii_strcasecmp (action_name, "ViewStatusbar") == 0) {
		g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

		if (priv->mode == EOC_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings, EOC_CONF_UI_STATUSBAR, visible);

	} else if (g_ascii_strcasecmp (action_name, "ViewImageCollection") == 0) {
		if (visible) {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events */
			if (!ctk_widget_get_realized (window->priv->thumbview))
				ctk_widget_realize (window->priv->thumbview);

			ctk_widget_show (priv->nav);
			ctk_widget_grab_focus (priv->thumbview);
		} else {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events.
			 * Don't do it during init phase or the view
			 * will get a bogus allocation. */
			if (!ctk_widget_get_realized (priv->view)
			    && priv->status == EOC_WINDOW_STATUS_NORMAL)
				ctk_widget_realize (priv->view);

			ctk_widget_hide (priv->nav);

			if (ctk_widget_get_realized (priv->view))
				ctk_widget_grab_focus (priv->view);
		}
		g_settings_set_boolean (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION, visible);

	} else if (g_ascii_strcasecmp (action_name, "ViewSidebar") == 0) {
		if (visible) {
			ctk_widget_show (priv->sidebar);
		} else {
			ctk_widget_hide (priv->sidebar);
		}
		g_settings_set_boolean (priv->ui_settings, EOC_CONF_UI_SIDEBAR, visible);
	}
}

static void
wallpaper_info_bar_response (CtkInfoBar *bar G_GNUC_UNUSED,
			     gint        response,
			     EocWindow  *window)
{
	if (response == CTK_RESPONSE_YES) {
		GAppInfo *app_info;
		GError *error = NULL;

		app_info = g_app_info_create_from_commandline ("cafe-appearance-properties --show-page=background",
							       "cafe-appearance-properties",
							       G_APP_INFO_CREATE_NONE,
							       &error);

		if (error != NULL) {
			g_warning ("%s%s", _("Error launching appearance preferences dialog: "),
				   error->message);
			g_error_free (error);
			error = NULL;
		}

		if (app_info != NULL) {
			CdkAppLaunchContext *context;
			CdkDisplay *display;

			display = ctk_widget_get_display (CTK_WIDGET (window));
			context = cdk_display_get_app_launch_context (display);
			g_app_info_launch (app_info, NULL, G_APP_LAUNCH_CONTEXT (context), &error);

			if (error != NULL) {
				g_warning ("%s%s", _("Error launching appearance preferences dialog: "),
					   error->message);
				g_error_free (error);
				error = NULL;
			}

			g_object_unref (context);
			g_object_unref (app_info);
		}
	}

	/* Close message area on every response */
	eoc_window_set_message_area (window, NULL);
}

static void
eoc_window_set_wallpaper (EocWindow *window, const gchar *filename, const gchar *visible_filename)
{
	CtkWidget *info_bar;
	CtkWidget *image;
	CtkWidget *label;
	CtkWidget *hbox;
	gchar *markup;
	gchar *text;
	gchar *basename;
	GSettings *wallpaper_settings;

	wallpaper_settings = g_settings_new (EOC_CONF_BACKGROUND_SCHEMA);
	g_settings_set_string (wallpaper_settings,
				 EOC_CONF_BACKGROUND_FILE,
				 filename);
	g_object_unref (wallpaper_settings);

	/* I18N: When setting mnemonics for these strings, watch out to not
	   clash with mnemonics from eoc's menubar */
	info_bar = ctk_info_bar_new_with_buttons (_("_Open Background Preferences"),
						  CTK_RESPONSE_YES,
						  C_("MessageArea","Hi_de"),
						  CTK_RESPONSE_NO, NULL);
	ctk_info_bar_set_message_type (CTK_INFO_BAR (info_bar),
				       CTK_MESSAGE_QUESTION);

	image = ctk_image_new_from_icon_name ("dialog-question",
					  CTK_ICON_SIZE_DIALOG);
	label = ctk_label_new (NULL);

	if (!visible_filename)
		basename = g_path_get_basename (filename);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been set as Desktop Background."
				  "\nWould you like to modify its appearance?"),
				visible_filename ? visible_filename : basename);
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	ctk_label_set_markup (CTK_LABEL (label), markup);
	g_free (markup);
	g_free (text);
	if (!visible_filename)
		g_free (basename);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
	ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);
	ctk_widget_set_halign (image, CTK_ALIGN_START);
	ctk_widget_set_valign (image, CTK_ALIGN_END);
	ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_box_pack_start (CTK_BOX (ctk_info_bar_get_content_area (CTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	ctk_widget_show_all (hbox);
	ctk_widget_show (info_bar);


	eoc_window_set_message_area (window, info_bar);
	ctk_info_bar_set_default_response (CTK_INFO_BAR (info_bar),
					   CTK_RESPONSE_YES);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (wallpaper_info_bar_response), window);
}

static void
eoc_job_save_cb (EocJobSave *job, gpointer user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);
	CtkAction *action_save;

	g_signal_handlers_disconnect_by_func (job,
					      eoc_job_save_cb,
					      window);

	g_signal_handlers_disconnect_by_func (job,
					      eoc_job_save_progress_cb,
					      window);

	g_object_unref (window->priv->save_job);
	window->priv->save_job = NULL;

	update_status_bar (window);
	action_save = ctk_action_group_get_action (window->priv->actions_image,
						   "ImageSave");
	ctk_action_set_sensitive (action_save, FALSE);
}

static void
eoc_job_copy_cb (EocJobCopy *job, gpointer user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);
	gchar *filepath, *basename, *filename, *extension;
	CtkAction *action;
	GFile *source_file, *dest_file;

	/* Create source GFile */
	basename = g_file_get_basename (job->images->data);
	filepath = g_build_filename (job->dest, basename, NULL);
	source_file = g_file_new_for_path (filepath);
	g_free (filepath);

	/* Create destination GFile */
	extension = eoc_util_filename_get_extension (basename);
	filename = g_strdup_printf  ("%s.%s", EOC_WALLPAPER_FILENAME, extension);
	filepath = g_build_filename (job->dest, filename, NULL);
	dest_file = g_file_new_for_path (filepath);
	g_free (filename);
	g_free (extension);

	/* Move the file */
	g_file_move (source_file, dest_file, G_FILE_COPY_OVERWRITE,
		     NULL, NULL, NULL, NULL);

	/* Set the wallpaper */
	eoc_window_set_wallpaper (window, filepath, basename);
	g_free (basename);
	g_free (filepath);

	ctk_statusbar_pop (CTK_STATUSBAR (window->priv->statusbar),
			   window->priv->copy_file_cid);
	action = ctk_action_group_get_action (window->priv->actions_image,
					      "ImageSetAsWallpaper");
	ctk_action_set_sensitive (action, TRUE);

	window->priv->copy_job = NULL;

	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_object_unref (G_OBJECT (job->images->data));
	g_list_free (job->images);
	g_object_unref (job);
}

static gboolean
eoc_window_save_images (EocWindow *window, GList *images)
{
	EocWindowPrivate *priv;

	priv = window->priv;

	if (window->priv->save_job != NULL)
		return FALSE;

	priv->save_job = eoc_job_save_new (images);

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (eoc_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (eoc_job_save_progress_cb),
			  window);

	return TRUE;
}

static void
eoc_window_cmd_save (CtkAction *action G_GNUC_UNUSED,
		     gpointer   user_data)
{
	EocWindowPrivate *priv;
	EocWindow *window;
	GList *images;

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = eoc_thumb_view_get_selected_images (EOC_THUMB_VIEW (priv->thumbview));

	if (eoc_window_save_images (window, images)) {
		eoc_job_queue_add_job (priv->save_job);
	}
}

static GFile*
eoc_window_retrieve_save_as_file (EocWindow *window, EocImage *image)
{
	CtkWidget *dialog;
	GFile *save_file = NULL;
	GFile *last_dest_folder;
	gint response;

	g_assert (image != NULL);

	dialog = eoc_file_chooser_new (CTK_FILE_CHOOSER_ACTION_SAVE);

	last_dest_folder = window->priv->last_save_as_folder;

	if (last_dest_folder && g_file_query_exists (last_dest_folder, NULL)) {
		ctk_file_chooser_set_current_folder_file (CTK_FILE_CHOOSER (dialog), last_dest_folder, NULL);
		ctk_file_chooser_set_current_name (CTK_FILE_CHOOSER (dialog),
						 eoc_image_get_caption (image));
	} else {
		GFile *image_file;

		image_file = eoc_image_get_file (image);
		/* Setting the file will also navigate to its parent folder */
		ctk_file_chooser_set_file (CTK_FILE_CHOOSER (dialog),
					   image_file, NULL);
		g_object_unref (image_file);
	}

	response = ctk_dialog_run (CTK_DIALOG (dialog));
	ctk_widget_hide (dialog);

	if (response == CTK_RESPONSE_OK) {
		save_file = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (dialog));
		if (window->priv->last_save_as_folder)
			g_object_unref (window->priv->last_save_as_folder);
		window->priv->last_save_as_folder = g_file_get_parent (save_file);
	}
	ctk_widget_destroy (dialog);

	return save_file;
}

static void
eoc_window_cmd_save_as (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
        EocWindowPrivate *priv;
        EocWindow *window;
	GList *images;
	guint n_images;

        window = EOC_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = eoc_thumb_view_get_selected_images (EOC_THUMB_VIEW (priv->thumbview));
	n_images = g_list_length (images);

	if (n_images == 1) {
		GFile *file;

		file = eoc_window_retrieve_save_as_file (window, images->data);

		if (!file) {
			g_list_free (images);
			return;
		}

		priv->save_job = eoc_job_save_as_new (images, NULL, file);

		g_object_unref (file);
	} else if (n_images > 1) {
		GFile *base_file;
		CtkWidget *dialog;
		gchar *basedir;
		EocURIConverter *converter;

		basedir = g_get_current_dir ();
		base_file = g_file_new_for_path (basedir);
		g_free (basedir);

		dialog = eoc_save_as_dialog_new (CTK_WINDOW (window),
						 images,
						 base_file);

		ctk_widget_show_all (dialog);

		if (ctk_dialog_run (CTK_DIALOG (dialog)) != CTK_RESPONSE_OK) {
			g_object_unref (base_file);
			g_list_free (images);
			ctk_widget_destroy (dialog);

			return;
		}

		converter = eoc_save_as_dialog_get_converter (dialog);

		g_assert (converter != NULL);

		priv->save_job = eoc_job_save_as_new (images, converter, NULL);

		ctk_widget_destroy (dialog);

		g_object_unref (converter);
		g_object_unref (base_file);
	} else {
		/* n_images = 0 -- No Image selected */
		return;
	}

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (eoc_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (eoc_job_save_progress_cb),
			  window);

	eoc_job_queue_add_job (priv->save_job);
}

static void
eoc_window_cmd_open_containing_folder (CtkAction *action G_GNUC_UNUSED,
				       gpointer   user_data)
{
	EocWindowPrivate *priv;

	GFile *file;
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	priv = EOC_WINDOW (user_data)->priv;

	g_return_if_fail (priv->image != NULL);

	file = eoc_image_get_file (priv->image);

	g_return_if_fail (file != NULL);

	eoc_util_show_file_in_filemanager (file,
	                                   CTK_WINDOW (user_data));
}

static void
eoc_window_cmd_print (CtkAction *action G_GNUC_UNUSED,
		      gpointer   user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	eoc_window_print (window);
}

/**
 * eoc_window_get_properties_dialog:
 * @window: a #EocWindow
 *
 * Gets the @window property dialog. The widget will be built on the first call to this function.
 *
 * Returns: (transfer none): a #CtkDialog.
 */

CtkWidget*
eoc_window_get_properties_dialog (EocWindow *window)
{
	EocWindowPrivate *priv;

	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	priv = window->priv;

	if (priv->properties_dlg == NULL) {
		CtkAction *next_image_action, *previous_image_action;

		next_image_action =
			ctk_action_group_get_action (priv->actions_collection,
						     "GoNext");

		previous_image_action =
			ctk_action_group_get_action (priv->actions_collection,
						     "GoPrevious");
		priv->properties_dlg =
			eoc_properties_dialog_new (CTK_WINDOW (window),
						   EOC_THUMB_VIEW (priv->thumbview),
						   next_image_action,
						   previous_image_action);

		eoc_properties_dialog_update (EOC_PROPERTIES_DIALOG (priv->properties_dlg),
					      priv->image);
		g_settings_bind (priv->ui_settings,
				 EOC_CONF_UI_PROPSDIALOG_NETBOOK_MODE,
				 priv->properties_dlg, "netbook-mode",
				 G_SETTINGS_BIND_GET);
	}

	return priv->properties_dlg;
}

static void
eoc_window_cmd_properties (CtkAction *action G_GNUC_UNUSED,
			   gpointer   user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);
	CtkWidget *dialog;

	dialog = eoc_window_get_properties_dialog (window);
	ctk_widget_show (dialog);
}

static void
eoc_window_cmd_undo (CtkAction *action G_GNUC_UNUSED,
		     gpointer   user_data)
{
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	apply_transformation (EOC_WINDOW (user_data), NULL);
}

static void
eoc_window_cmd_flip_horizontal (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	apply_transformation (EOC_WINDOW (user_data),
			      eoc_transform_flip_new (EOC_TRANSFORM_FLIP_HORIZONTAL));
}

static void
eoc_window_cmd_flip_vertical (CtkAction *action G_GNUC_UNUSED,
			      gpointer   user_data)
{
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	apply_transformation (EOC_WINDOW (user_data),
			      eoc_transform_flip_new (EOC_TRANSFORM_FLIP_VERTICAL));
}

static void
eoc_window_cmd_rotate_90 (CtkAction *action G_GNUC_UNUSED,
			  gpointer   user_data)
{
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	apply_transformation (EOC_WINDOW (user_data),
			      eoc_transform_rotate_new (90));
}

static void
eoc_window_cmd_rotate_270 (CtkAction *action G_GNUC_UNUSED,
			   gpointer   user_data)
{
	g_return_if_fail (EOC_IS_WINDOW (user_data));

	apply_transformation (EOC_WINDOW (user_data),
			      eoc_transform_rotate_new (270));
}

static void
eoc_window_cmd_wallpaper (CtkAction *action G_GNUC_UNUSED,
			  gpointer   user_data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
	EocImage *image;
	GFile *file;
	char *filename = NULL;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	/* If currently copying an image to set it as wallpaper, return. */
	if (priv->copy_job != NULL)
		return;

	image = eoc_thumb_view_get_first_selected_image (EOC_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (EOC_IS_IMAGE (image));

	file = eoc_image_get_file (image);

	filename = g_file_get_path (file);

	/* Currently only local files can be set as wallpaper */
	if (filename == NULL || !eoc_util_file_is_persistent (file))
	{
		GList *files = NULL;
		CtkAction *action;

		action = ctk_action_group_get_action (window->priv->actions_image,
						      "ImageSetAsWallpaper");
		ctk_action_set_sensitive (action, FALSE);

		priv->copy_file_cid = ctk_statusbar_get_context_id (CTK_STATUSBAR (priv->statusbar),
								    "copy_file_cid");
		ctk_statusbar_push (CTK_STATUSBAR (priv->statusbar),
				    priv->copy_file_cid,
				    _("Saving image locally…"));

		files = g_list_append (files, eoc_image_get_file (image));
		priv->copy_job = eoc_job_copy_new (files, g_get_user_data_dir ());
		g_signal_connect (priv->copy_job,
				  "finished",
				  G_CALLBACK (eoc_job_copy_cb),
				  window);
		g_signal_connect (priv->copy_job,
				  "progress",
				  G_CALLBACK (eoc_job_progress_cb),
				  window);
		eoc_job_queue_add_job (priv->copy_job);

		g_object_unref (file);
		g_free (filename);
		return;
	}

	g_object_unref (file);

	eoc_window_set_wallpaper (window, filename, NULL);

	g_free (filename);
}

static gboolean
eoc_window_all_images_trasheable (GList *images)
{
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	EocImage *image;
	gboolean can_trash = TRUE;

	for (iter = images; iter != NULL; iter = g_list_next (iter)) {
		image = (EocImage *) iter->data;
		file = eoc_image_get_file (image);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
					       0, NULL, NULL);
		can_trash = g_file_info_get_attribute_boolean (file_info,
							       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);

		g_object_unref (file_info);
		g_object_unref (file);

		if (can_trash == FALSE)
			break;
	}

	return can_trash;
}

static int
show_move_to_trash_confirm_dialog (EocWindow *window, GList *images, gboolean can_trash)
{
	CtkWidget *dlg;
	char *prompt;
	int response;
	int n_images;
	EocImage *image;
	static gboolean dontaskagain = FALSE;
	gboolean neverask = FALSE;
	CtkWidget* dontask_cbutton = NULL;

	/* Check if the user never wants to be bugged. */
	neverask = g_settings_get_boolean (window->priv->ui_settings,
					 EOC_CONF_UI_DISABLE_TRASH_CONFIRMATION);

	/* Assume agreement, if the user doesn't want to be
	 * asked and the trash is available */
	if (can_trash && (dontaskagain || neverask))
		return CTK_RESPONSE_OK;

	n_images = g_list_length (images);

	if (n_images == 1) {
		image = EOC_IMAGE (images->data);
		if (can_trash) {
			prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"),
						  eoc_image_get_caption (image));
		} else {
			prompt = g_strdup_printf (_("A trash for \"%s\" couldn't be found. Do you want to remove "
						    "this image permanently?"), eoc_image_get_caption (image));
		}
	} else {
		if (can_trash) {
			prompt = g_strdup_printf (ngettext("Are you sure you want to move\n"
							   "the %d selected image to the trash?",
							   "Are you sure you want to move\n"
							   "the %d selected images to the trash?", n_images), n_images);
		} else {
			prompt = g_strdup (_("Some of the selected images can't be moved to the trash "
					     "and will be removed permanently. Are you sure you want "
					     "to proceed?"));
		}
	}

	dlg = ctk_message_dialog_new_with_markup (CTK_WINDOW (window),
						  CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
						  CTK_MESSAGE_WARNING,
						  CTK_BUTTONS_NONE,
						  "<span weight=\"bold\" size=\"larger\">%s</span>",
						  prompt);
	g_free (prompt);

	ctk_dialog_add_button (CTK_DIALOG (dlg), "ctk-cancel", CTK_RESPONSE_CANCEL);

	if (can_trash) {
		ctk_dialog_add_button (CTK_DIALOG (dlg), _("Move to _Trash"), CTK_RESPONSE_OK);

		dontask_cbutton = ctk_check_button_new_with_mnemonic (_("_Do not ask again during this session"));
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (dontask_cbutton), FALSE);

		ctk_box_pack_end (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dlg))), dontask_cbutton, TRUE, TRUE, 0);
	} else {
		if (n_images == 1) {
			ctk_dialog_add_button (CTK_DIALOG (dlg), "ctk-delete", CTK_RESPONSE_OK);
		} else {
			ctk_dialog_add_button (CTK_DIALOG (dlg), "ctk-yes", CTK_RESPONSE_OK);
		}
	}

	ctk_dialog_set_default_response (CTK_DIALOG (dlg), CTK_RESPONSE_OK);
	ctk_window_set_title (CTK_WINDOW (dlg), "");
	ctk_widget_show_all (dlg);

	response = ctk_dialog_run (CTK_DIALOG (dlg));

	/* Only update the property if the user has accepted */
	if (can_trash && response == CTK_RESPONSE_OK)
		dontaskagain = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (dontask_cbutton));

	/* The checkbutton is destroyed together with the dialog */
	ctk_widget_destroy (dlg);

	return response;
}

static gboolean
move_to_trash_real (EocImage *image, GError **error)
{
	GFile *file;
	GFileInfo *file_info;
	gboolean can_trash, result;

	g_return_val_if_fail (EOC_IS_IMAGE (image), FALSE);

	file = eoc_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
				       0, NULL, NULL);
	if (file_info == NULL) {
		g_set_error (error,
			     EOC_WINDOW_ERROR,
			     EOC_WINDOW_ERROR_TRASH_NOT_FOUND,
			     _("Couldn't access trash."));
		return FALSE;
	}

	can_trash = g_file_info_get_attribute_boolean (file_info,
						       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);
	g_object_unref (file_info);
	if (can_trash)
	{
		result = g_file_trash (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     EOC_WINDOW_ERROR,
				     EOC_WINDOW_ERROR_TRASH_NOT_FOUND,
				     _("Couldn't access trash."));
		}
	} else {
		result = g_file_delete (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     EOC_WINDOW_ERROR,
				     EOC_WINDOW_ERROR_IO,
				     _("Couldn't delete file"));
		}
	}

        g_object_unref (file);

	return result;
}

static void
eoc_window_cmd_copy_image (CtkAction *action G_GNUC_UNUSED,
			   gpointer   user_data)
{
	CtkClipboard *clipboard;
	EocWindow *window;
	EocWindowPrivate *priv;
	EocImage *image;
	EocClipboardHandler *cbhandler;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;

	image = eoc_thumb_view_get_first_selected_image (EOC_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (EOC_IS_IMAGE (image));

	clipboard = ctk_clipboard_get (CDK_SELECTION_CLIPBOARD);

	cbhandler = eoc_clipboard_handler_new (image);
	// cbhandler will self-destruct when it's not needed anymore
	eoc_clipboard_handler_copy_to_clipboard (cbhandler, clipboard);

}

static void
eoc_window_cmd_move_to_trash (CtkAction *action, gpointer user_data)
{
	GList *images;
	GList *it;
	EocWindowPrivate *priv;
	EocListStore *list;
	int pos;
	EocImage *img;
	EocWindow *window;
	int response;
	int n_images;
	gboolean success;
	gboolean can_trash;
	const gchar *action_name;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	window = EOC_WINDOW (user_data);
	priv = window->priv;
	list = priv->store;

	n_images = eoc_thumb_view_get_n_selected (EOC_THUMB_VIEW (priv->thumbview));

	if (n_images < 1) return;

	/* save position of selected image after the deletion */
	images = eoc_thumb_view_get_selected_images (EOC_THUMB_VIEW (priv->thumbview));

	g_assert (images != NULL);

	/* HACK: eoc_list_store_get_n_selected return list in reverse order */
	images = g_list_reverse (images);

	can_trash = eoc_window_all_images_trasheable (images);

	action_name = ctk_action_get_name (action);

	if (g_ascii_strcasecmp (action_name, "Delete") == 0 ||
	    can_trash == FALSE) {
		response = show_move_to_trash_confirm_dialog (window, images, can_trash);

		if (response != CTK_RESPONSE_OK) return;
	}

	pos = eoc_list_store_get_pos_by_image (list, EOC_IMAGE (images->data));

	/* FIXME: make a nice progress dialog */
	/* Do the work actually. First try to delete the image from the disk. If this
	 * is successful, remove it from the screen. Otherwise show error dialog.
	 */
	for (it = images; it != NULL; it = it->next) {
		GError *error = NULL;
		EocImage *image;

		image = EOC_IMAGE (it->data);

		success = move_to_trash_real (image, &error);

		if (success) {
			eoc_list_store_remove_image (list, image);
		} else {
			char *header;
			CtkWidget *dlg;

			header = g_strdup_printf (_("Error on deleting image %s"),
						  eoc_image_get_caption (image));

			dlg = ctk_message_dialog_new (CTK_WINDOW (window),
						      CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
						      CTK_MESSAGE_ERROR,
						      CTK_BUTTONS_OK,
						      "%s", header);

			ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dlg),
								  "%s", error->message);

			ctk_dialog_run (CTK_DIALOG (dlg));

			ctk_widget_destroy (dlg);

			g_free (header);
		}
	}

	/* free list */
	g_list_foreach (images, (GFunc) g_object_unref, NULL);
	g_list_free (images);

	/* select image at previously saved position */
	pos = MIN (pos, eoc_list_store_length (list) - 1);

	if (pos >= 0) {
		img = eoc_list_store_get_image_by_pos (list, pos);

		eoc_thumb_view_set_current_image (EOC_THUMB_VIEW (priv->thumbview),
						  img,
						  TRUE);

		if (img != NULL) {
			g_object_unref (img);
		}
	}
}

static void
eoc_window_cmd_fullscreen (CtkAction *action, gpointer user_data)
{
	EocWindow *window;
	gboolean fullscreen;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (user_data);

	fullscreen = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));

	if (fullscreen) {
		eoc_window_run_fullscreen (window, FALSE);
	} else {
		eoc_window_stop_fullscreen (window, FALSE);
	}
}

static void
eoc_window_cmd_slideshow (CtkAction *action, gpointer user_data)
{
	EocWindow *window;
	gboolean slideshow;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (user_data);

	slideshow = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));

	if (slideshow) {
		eoc_window_run_fullscreen (window, TRUE);
	} else {
		eoc_window_stop_fullscreen (window, TRUE);
	}
}

static void
eoc_window_cmd_pause_slideshow (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
	EocWindow *window;
	gboolean slideshow;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (user_data);

	slideshow = window->priv->mode == EOC_WINDOW_MODE_SLIDESHOW;

	if (!slideshow && window->priv->mode != EOC_WINDOW_MODE_FULLSCREEN)
		return;

	eoc_window_run_fullscreen (window, !slideshow);
}

static void
eoc_window_cmd_zoom_in (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	if (priv->view) {
		eoc_scroll_view_zoom_in (EOC_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
eoc_window_cmd_zoom_out (CtkAction *action G_GNUC_UNUSED,
			 gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	if (priv->view) {
		eoc_scroll_view_zoom_out (EOC_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
eoc_window_cmd_zoom_normal (CtkAction *action G_GNUC_UNUSED,
			    gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	if (priv->view) {
		eoc_scroll_view_set_zoom (EOC_SCROLL_VIEW (priv->view), 1.0);
	}
}

static void
eoc_window_cmd_zoom_fit (CtkAction *action G_GNUC_UNUSED,
			 gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	if (priv->view) {
		eoc_scroll_view_zoom_fit (EOC_SCROLL_VIEW (priv->view));
	}
}

static void
eoc_window_cmd_go_prev (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_LEFT);
}

static void
eoc_window_cmd_go_next (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_RIGHT);
}

static void
eoc_window_cmd_go_first (CtkAction *action G_GNUC_UNUSED,
			 gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_FIRST);
}

static void
eoc_window_cmd_go_last (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_LAST);
}

static void
eoc_window_cmd_go_random (CtkAction *action G_GNUC_UNUSED,
			  gpointer   user_data)
{
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (user_data));

	eoc_debug (DEBUG_WINDOW);

	priv = EOC_WINDOW (user_data)->priv;

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_RANDOM);
}

static const CtkActionEntry action_entries_window[] = {
	{ "Image", NULL, N_("_Image") },
	{ "Edit",  NULL, N_("_Edit") },
	{ "View",  NULL, N_("_View") },
	{ "Go",    NULL, N_("_Go") },
	{ "Tools", NULL, N_("_Tools") },
	{ "Help",  NULL, N_("_Help") },

	{ "ImageOpen", "document-open",  N_("_Open…"), "<control>O",
	  N_("Open a file"),
	  G_CALLBACK (eoc_window_cmd_file_open) },
	{ "ImageClose", "window-close", N_("_Close"), "<control>W",
	  N_("Close window"),
	  G_CALLBACK (eoc_window_cmd_close_window) },
	{ "EditToolbar", NULL, N_("T_oolbar"), NULL,
	  N_("Edit the application toolbar"),
	  G_CALLBACK (eoc_window_cmd_edit_toolbar) },
	{ "EditPreferences", "preferences-desktop", N_("Prefere_nces"), NULL,
	  N_("Preferences for Eye of CAFE"),
	  G_CALLBACK (eoc_window_cmd_preferences) },
	{ "HelpManual", "help-browser", N_("_Contents"), "F1",
	  N_("Help on this application"),
	  G_CALLBACK (eoc_window_cmd_help) },
	{ "HelpAbout", "help-about", N_("_About"), NULL,
	  N_("About this application"),
	  G_CALLBACK (eoc_window_cmd_about) }
};

static const CtkToggleActionEntry toggle_entries_window[] = {
	{ "ViewToolbar", NULL, N_("_Toolbar"), NULL,
	  N_("Changes the visibility of the toolbar in the current window"),
	  G_CALLBACK (eoc_window_cmd_show_hide_bar), TRUE },
	{ "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
	  N_("Changes the visibility of the statusbar in the current window"),
	  G_CALLBACK (eoc_window_cmd_show_hide_bar), TRUE },
	{ "ViewImageCollection", "eoc-image-collection", N_("_Image Collection"), "<control>F9",
	  N_("Changes the visibility of the image collection pane in the current window"),
	  G_CALLBACK (eoc_window_cmd_show_hide_bar), TRUE },
	{ "ViewSidebar", NULL, N_("Side _Pane"), "F9",
	  N_("Changes the visibility of the side pane in the current window"),
	  G_CALLBACK (eoc_window_cmd_show_hide_bar), TRUE },
};

static const CtkActionEntry action_entries_image[] = {
	{ "ImageSave", "document-save", N_("_Save"), "<control>s",
	  N_("Save changes in currently selected images"),
	  G_CALLBACK (eoc_window_cmd_save) },
	{ "ImageOpenWith", NULL, N_("Open _with"), NULL,
	  N_("Open the selected image with a different application"),
	  NULL},
	{ "ImageSaveAs", "document-save-as", N_("Save _As…"), "<control><shift>s",
	  N_("Save the selected images with a different name"),
	  G_CALLBACK (eoc_window_cmd_save_as) },
	{ "ImageOpenContainingFolder", "folder", N_("Open Containing _Folder"), NULL,
	  N_("Show the folder which contains this file in the file manager"),
	  G_CALLBACK (eoc_window_cmd_open_containing_folder) },
	{ "ImagePrint", "document-print", N_("_Print…"), "<control>p",
	  N_("Print the selected image"),
	  G_CALLBACK (eoc_window_cmd_print) },
	{ "ImageProperties", "document-properties", N_("Prope_rties"), "<alt>Return",
	  N_("Show the properties and metadata of the selected image"),
	  G_CALLBACK (eoc_window_cmd_properties) },
	{ "EditUndo", "edit-undo", N_("_Undo"), "<control>z",
	  N_("Undo the last change in the image"),
	  G_CALLBACK (eoc_window_cmd_undo) },
	{ "EditFlipHorizontal", "object-flip-horizontal", N_("Flip _Horizontal"), NULL,
	  N_("Mirror the image horizontally"),
	  G_CALLBACK (eoc_window_cmd_flip_horizontal) },
	{ "EditFlipVertical", "object-flip-vertical", N_("Flip _Vertical"), NULL,
	  N_("Mirror the image vertically"),
	  G_CALLBACK (eoc_window_cmd_flip_vertical) },
	{ "EditRotate90",  "object-rotate-right",  N_("_Rotate Clockwise"), "<control>r",
	  N_("Rotate the image 90 degrees to the right"),
	  G_CALLBACK (eoc_window_cmd_rotate_90) },
	{ "EditRotate270", "object-rotate-left", N_("Rotate Counterc_lockwise"), "<ctrl><shift>r",
	  N_("Rotate the image 90 degrees to the left"),
	  G_CALLBACK (eoc_window_cmd_rotate_270) },
	{ "ImageSetAsWallpaper", NULL, N_("Set as _Desktop Background"),
	  "<control>F8", N_("Set the selected image as the desktop background"),
	  G_CALLBACK (eoc_window_cmd_wallpaper) },
	{ "EditMoveToTrash", "user-trash", N_("Move to _Trash"), NULL,
	  N_("Move the selected image to the trash folder"),
	  G_CALLBACK (eoc_window_cmd_move_to_trash) },
	{ "EditCopyImage", "edit-copy", N_("_Copy"), "<control>C",
	  N_("Copy the selected image to the clipboard"),
	  G_CALLBACK (eoc_window_cmd_copy_image) },
	{ "ViewZoomIn", "zoom-in", N_("_Zoom In"), "<control>plus",
	  N_("Enlarge the image"),
	  G_CALLBACK (eoc_window_cmd_zoom_in) },
	{ "ViewZoomOut", "zoom-out", N_("Zoom _Out"), "<control>minus",
	  N_("Shrink the image"),
	  G_CALLBACK (eoc_window_cmd_zoom_out) },
	{ "ViewZoomNormal", "zoom-original", N_("_Normal Size"), "<control>0",
	  N_("Show the image at its normal size"),
	  G_CALLBACK (eoc_window_cmd_zoom_normal) },
	{ "ViewZoomFit", "zoom-fit-best", N_("_Best Fit"), "F",
	  N_("Fit the image to the window"),
	  G_CALLBACK (eoc_window_cmd_zoom_fit) },
	{ "ControlEqual", "zoom-in", N_("_Zoom In"), "<control>equal",
	  N_("Enlarge the image"),
	  G_CALLBACK (eoc_window_cmd_zoom_in) },
	{ "ControlKpAdd", "zoom-in", N_("_Zoom In"), "<control>KP_Add",
	  N_("Shrink the image"),
	  G_CALLBACK (eoc_window_cmd_zoom_in) },
	{ "ControlKpSub", "zoom-out", N_("Zoom _Out"), "<control>KP_Subtract",
	  N_("Shrink the image"),
	  G_CALLBACK (eoc_window_cmd_zoom_out) },
	{ "Delete", NULL, N_("Move to _Trash"), "Delete",
	  NULL,
	  G_CALLBACK (eoc_window_cmd_move_to_trash) },
};

static const CtkToggleActionEntry toggle_entries_image[] = {
	{ "ViewFullscreen", "view-fullscreen", N_("_Fullscreen"), "F11",
	  N_("Show the current image in fullscreen mode"),
	  G_CALLBACK (eoc_window_cmd_fullscreen), FALSE },
	{ "PauseSlideshow", "media-playback-pause", N_("Pause Slideshow"),
	  NULL, N_("Pause or resume the slideshow"),
	  G_CALLBACK (eoc_window_cmd_pause_slideshow), FALSE },
};

static const CtkActionEntry action_entries_collection[] = {
	{ "GoPrevious", "go-previous", N_("_Previous Image"), "<Alt>Left",
	  N_("Go to the previous image of the collection"),
	  G_CALLBACK (eoc_window_cmd_go_prev) },
	{ "GoNext", "go-next", N_("_Next Image"), "<Alt>Right",
	  N_("Go to the next image of the collection"),
	  G_CALLBACK (eoc_window_cmd_go_next) },
	{ "GoFirst", "go-first", N_("_First Image"), "<Alt>Home",
	  N_("Go to the first image of the collection"),
	  G_CALLBACK (eoc_window_cmd_go_first) },
	{ "GoLast", "go-last", N_("_Last Image"), "<Alt>End",
	  N_("Go to the last image of the collection"),
	  G_CALLBACK (eoc_window_cmd_go_last) },
	{ "GoRandom", NULL, N_("_Random Image"), "<control>M",
	  N_("Go to a random image of the collection"),
	  G_CALLBACK (eoc_window_cmd_go_random) },
	{ "BackSpace", NULL, N_("_Previous Image"), "BackSpace",
	  NULL,
	  G_CALLBACK (eoc_window_cmd_go_prev) },
	{ "Home", NULL, N_("_First Image"), "Home",
	  NULL,
	  G_CALLBACK (eoc_window_cmd_go_first) },
	{ "End", NULL, N_("_Last Image"), "End",
	  NULL,
	  G_CALLBACK (eoc_window_cmd_go_last) },
};

static const CtkToggleActionEntry toggle_entries_collection[] = {
	{ "ViewSlideshow", "slideshow-play", N_("S_lideshow"), "F5",
	  N_("Start a slideshow view of the images"),
	  G_CALLBACK (eoc_window_cmd_slideshow), FALSE },
};

static void
menu_item_select_cb (CtkMenuItem *proxy, EocWindow *window)
{
	CtkAction *action;
	char *message;

	action = ctk_activatable_get_related_action (CTK_ACTIVATABLE (proxy));

	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

	if (message) {
		ctk_statusbar_push (CTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (CtkMenuItem *proxy G_GNUC_UNUSED,
		       EocWindow   *window)
{
	ctk_statusbar_pop (CTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (CtkUIManager *manager,
                  CtkAction *action,
                  CtkWidget *proxy,
                  EocWindow *window)
{
	if (CTK_IS_MENU_ITEM (proxy)) {
		disconnect_proxy_cb (manager, action, proxy, window);
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (CtkUIManager *manager G_GNUC_UNUSED,
		     CtkAction    *action G_GNUC_UNUSED,
		     CtkWidget    *proxy,
		     EocWindow    *window)
{
	if (CTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
set_action_properties (CtkActionGroup *window_group,
		       CtkActionGroup *image_group,
		       CtkActionGroup *collection_group)
{
        CtkAction *action;

        action = ctk_action_group_get_action (collection_group, "GoPrevious");
        g_object_set (action, "short_label", _("Previous"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = ctk_action_group_get_action (collection_group, "GoNext");
        g_object_set (action, "short_label", _("Next"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = ctk_action_group_get_action (image_group, "EditRotate90");
        g_object_set (action, "short_label", _("Right"), NULL);

        action = ctk_action_group_get_action (image_group, "EditRotate270");
        g_object_set (action, "short_label", _("Left"), NULL);

        action = ctk_action_group_get_action (image_group, "ImageOpenContainingFolder");
        g_object_set (action, "short_label", _("Open Folder"), NULL);

        action = ctk_action_group_get_action (image_group, "ViewZoomIn");
        g_object_set (action, "short_label", _("In"), NULL);

        action = ctk_action_group_get_action (image_group, "ViewZoomOut");
        g_object_set (action, "short_label", _("Out"), NULL);

        action = ctk_action_group_get_action (image_group, "ViewZoomNormal");
        g_object_set (action, "short_label", _("Normal"), NULL);

        action = ctk_action_group_get_action (image_group, "ViewZoomFit");
        g_object_set (action, "short_label", _("Fit"), NULL);

        action = ctk_action_group_get_action (window_group, "ViewImageCollection");
        g_object_set (action, "short_label", _("Collection"), NULL);

        action = ctk_action_group_get_action (image_group, "EditMoveToTrash");
        g_object_set (action, "short_label", C_("action (to trash)", "Trash"), NULL);
}

static gint
sort_recents_mru (CtkRecentInfo *a, CtkRecentInfo *b)
{
	gboolean has_eoc_a, has_eoc_b;

	/* We need to check this first as ctk_recent_info_get_application_info
	 * will treat it as a non-fatal error when the CtkRecentInfo doesn't
	 * have the application registered. */
	has_eoc_a = ctk_recent_info_has_application (a,
						     EOC_RECENT_FILES_APP_NAME);
	has_eoc_b = ctk_recent_info_has_application (b,
						     EOC_RECENT_FILES_APP_NAME);
	if (has_eoc_a && has_eoc_b) {
		GDateTime *time_a = NULL;
		GDateTime *time_b = NULL;

		/* These should not fail as we already checked that
		 * the application is registered with the info objects */
		ctk_recent_info_get_application_info (a,
						      EOC_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_a);
		ctk_recent_info_get_application_info (b,
						      EOC_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_b);

		return (g_date_time_to_unix (time_b) - g_date_time_to_unix (time_a));
	} else if (has_eoc_a) {
		return -1;
	} else if (has_eoc_b) {
		return 1;
	}

	return 0;
}

static void
eoc_window_update_recent_files_menu (EocWindow *window)
{
	EocWindowPrivate *priv;
	GList *actions = NULL, *li = NULL, *items = NULL;
	guint count_recent = 0;

	priv = window->priv;

	if (priv->recent_menu_id != 0)
		ctk_ui_manager_remove_ui (priv->ui_mgr, priv->recent_menu_id);

	actions = ctk_action_group_list_actions (priv->actions_recent);

	for (li = actions; li != NULL; li = li->next) {
		g_signal_handlers_disconnect_by_func (CTK_ACTION (li->data),
						      G_CALLBACK(eoc_window_open_recent_cb),
						      window);

		ctk_action_group_remove_action (priv->actions_recent,
						CTK_ACTION (li->data));
	}

	g_list_free (actions);

	priv->recent_menu_id = ctk_ui_manager_new_merge_id (priv->ui_mgr);
	items = ctk_recent_manager_get_items (ctk_recent_manager_get_default());
	items = g_list_sort (items, (GCompareFunc) sort_recents_mru);

	for (li = items; li != NULL && count_recent < EOC_RECENT_FILES_LIMIT; li = li->next) {
		gchar *action_name;
		gchar *label;
		gchar *tip;
		gchar **display_name;
		gchar *label_filename;
		CtkAction *action;
		CtkRecentInfo *info = li->data;

		/* Sorting moves non-EOC files to the end of the list.
		 * So no file of interest will follow if this test fails */
		if (!ctk_recent_info_has_application (info, EOC_RECENT_FILES_APP_NAME))
			break;

		count_recent++;

		action_name = g_strdup_printf ("recent-info-%d", count_recent);
		display_name = g_strsplit (ctk_recent_info_get_display_name (info), "_", -1);
		label_filename = g_strjoinv ("__", display_name);
		label = g_strdup_printf ("%s_%d. %s",
				(is_rtl ? "\xE2\x80\x8F" : ""), count_recent, label_filename);
		g_free (label_filename);
		g_strfreev (display_name);

		tip = ctk_recent_info_get_uri_display (info);

		/* This is a workaround for a bug (#351945) regarding
		 * ctk_recent_info_get_uri_display() and remote URIs.
		 * cafe_vfs_format_uri_for_display is sufficient here
		 * since the password gets stripped when adding the
		 * file to the recently used list. */
		if (tip == NULL)
			tip = g_uri_unescape_string (ctk_recent_info_get_uri (info), NULL);

		action = ctk_action_new (action_name, label, tip, NULL);
		ctk_action_set_always_show_image (action, TRUE);

		g_object_set_data_full (G_OBJECT (action), "ctk-recent-info",
					ctk_recent_info_ref (info),
					(GDestroyNotify) ctk_recent_info_unref);

		g_object_set (G_OBJECT (action), "icon-name", "image-x-generic", NULL);

		g_signal_connect (action, "activate",
				  G_CALLBACK (eoc_window_open_recent_cb),
				  window);

		ctk_action_group_add_action (priv->actions_recent, action);

		g_object_unref (action);

		ctk_ui_manager_add_ui (priv->ui_mgr, priv->recent_menu_id,
				       "/MainMenu/Image/RecentDocuments",
				       action_name, action_name,
				       CTK_UI_MANAGER_AUTO, FALSE);

		g_free (action_name);
		g_free (label);
		g_free (tip);
	}

	g_list_foreach (items, (GFunc) ctk_recent_info_unref, NULL);
	g_list_free (items);
}

static void
eoc_window_recent_manager_changed_cb (CtkRecentManager *manager G_GNUC_UNUSED,
				      EocWindow        *window)
{
	eoc_window_update_recent_files_menu (window);
}

static void
eoc_window_drag_data_received (CtkWidget        *widget,
			       CdkDragContext   *context,
			       gint              x G_GNUC_UNUSED,
			       gint              y G_GNUC_UNUSED,
			       CtkSelectionData *selection_data,
			       guint             info G_GNUC_UNUSED,
			       guint             time)
{
	GSList *file_list;
	EocWindow *window;
	CdkAtom target;
	CtkWidget *src;

	target = ctk_selection_data_get_target (selection_data);

	if (!ctk_targets_include_uri (&target, 1))
		return;

	/* if the request is from another process this will return NULL */
	src = ctk_drag_get_source_widget (context);

	/* if the drag request originates from the current eoc instance, ignore
	   the request if the source window is the same as the dest window */
	if (src &&
	    ctk_widget_get_toplevel (src) == ctk_widget_get_toplevel (widget))
	{
		cdk_drag_status (context, 0, time);
		return;
	}

	if (cdk_drag_context_get_suggested_action (context) == CDK_ACTION_COPY) {
		window = EOC_WINDOW (widget);

		file_list = eoc_util_parse_uri_string_list_to_file_list ((const gchar *) ctk_selection_data_get_data (selection_data));

		eoc_window_open_file_list (window, file_list);
	}
}

static void
eoc_window_set_drag_dest (EocWindow *window)
{
	ctk_drag_dest_set (CTK_WIDGET (window),
                           CTK_DEST_DEFAULT_MOTION | CTK_DEST_DEFAULT_DROP,
                           NULL, 0,
                           CDK_ACTION_COPY | CDK_ACTION_ASK);
	ctk_drag_dest_add_uri_targets (CTK_WIDGET (window));
}

static void
eoc_window_sidebar_visibility_changed (CtkWidget *widget G_GNUC_UNUSED,
				       EocWindow *window)
{
	CtkAction *action;
	gboolean visible;

	visible = ctk_widget_get_visible (window->priv->sidebar);

	action = ctk_action_group_get_action (window->priv->actions_window,
					      "ViewSidebar");

	if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)) != visible)
		ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), visible);

	/* Focus the image */
	if (!visible && window->priv->image != NULL)
		ctk_widget_grab_focus (window->priv->view);
}

static void
eoc_window_sidebar_page_added (EocSidebar  *sidebar,
			       CtkWidget   *main_widget G_GNUC_UNUSED,
			       EocWindow   *window)
{
	if (eoc_sidebar_get_n_pages (sidebar) == 1) {
		CtkAction *action;
		gboolean show;

		action = ctk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		ctk_action_set_sensitive (action, TRUE);

		show = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));

		if (show)
			ctk_widget_show (CTK_WIDGET (sidebar));
	}
}
static void
eoc_window_sidebar_page_removed (EocSidebar  *sidebar,
				 CtkWidget   *main_widget G_GNUC_UNUSED,
				 EocWindow   *window)
{
	if (eoc_sidebar_is_empty (sidebar)) {
		CtkAction *action;

		ctk_widget_hide (CTK_WIDGET (sidebar));

		action = ctk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		ctk_action_set_sensitive (action, FALSE);
	}
}

static void
eoc_window_finish_saving (EocWindow *window)
{
	EocWindowPrivate *priv = window->priv;

	ctk_widget_set_sensitive (CTK_WIDGET (window), FALSE);

	do {
		ctk_main_iteration ();
	} while (priv->save_job != NULL);
}

static GAppInfo *
get_appinfo_for_editor (EocWindow *window)
{
	/* We want this function to always return the same thing, not
	 * just for performance reasons, but because if someone edits
	 * GConf while eoc is running, the application could get into an
	 * inconsistent state.  If the editor exists once, it gets added
	 * to the "available" list of the EggToolbarsModel (for which
	 * there is no API to remove it).  If later the editor no longer
	 * existed when constructing a new window, we'd be unable to
	 * construct a CtkAction for the editor for that window, causing
	 * assertion failures when viewing the "Edit Toolbars" dialog
	 * (item is available, but can't find the CtkAction for it).
	 *
	 * By ensuring we keep the GAppInfo around, we avoid the
	 * possibility of that situation occurring.
	 */
	static GDesktopAppInfo *app_info = NULL;
	static gboolean initialised;

	if (!initialised) {
		gchar *editor;

		editor = g_settings_get_string (window->priv->ui_settings,
		                                EOC_CONF_UI_EXTERNAL_EDITOR);

		if (editor != NULL) {
			app_info = g_desktop_app_info_new (editor);
		}

		initialised = TRUE;
		g_free (editor);
	}

	return (GAppInfo *) app_info;
}

static void
eoc_window_open_editor (CtkAction *action G_GNUC_UNUSED,
			EocWindow *window)
{
	CdkAppLaunchContext *context;
	GAppInfo *app_info;
	GList files;

	app_info = get_appinfo_for_editor (window);

	if (app_info == NULL)
		return;

	context = cdk_display_get_app_launch_context (
	  ctk_widget_get_display (CTK_WIDGET (window)));
	cdk_app_launch_context_set_screen (context,
	  ctk_widget_get_screen (CTK_WIDGET (window)));
	cdk_app_launch_context_set_icon (context,
	  g_app_info_get_icon (app_info));
	cdk_app_launch_context_set_timestamp (context,
	  ctk_get_current_event_time ());

	{
		GList f = { eoc_image_get_file (window->priv->image) };
		files = f;
	}

	g_app_info_launch (app_info, &files,
                           G_APP_LAUNCH_CONTEXT (context), NULL);

	g_object_unref (files.data);
	g_object_unref (context);
}

static void
eoc_window_add_open_editor_action (EocWindow *window)
{
	EggToolbarsModel *model;
	GAppInfo *app_info;
	CtkAction *action;
        gchar *tooltip;

	app_info = get_appinfo_for_editor (window);

	if (app_info == NULL)
		return;

	model = eoc_application_get_toolbars_model (EOC_APP);
	egg_toolbars_model_set_name_flags (model, "OpenEditor",
	                                   EGG_TB_MODEL_NAME_KNOWN);

	tooltip = g_strdup_printf (_("Edit the current image using %s"),
	                           g_app_info_get_name (app_info));
	action = ctk_action_new ("OpenEditor", _("Edit Image"), tooltip, NULL);
	ctk_action_set_gicon (action, g_app_info_get_icon (app_info));
	ctk_action_set_is_important (action, TRUE);

	g_signal_connect (action, "activate",
	                  G_CALLBACK (eoc_window_open_editor), window);

	ctk_action_group_add_action (window->priv->actions_image, action);

	g_object_unref (action);
	g_free (tooltip);
}

static void
eoc_window_construct_ui (EocWindow *window)
{
	EocWindowPrivate *priv;

	GError *error = NULL;

	CtkWidget *menubar;
	CtkWidget *thumb_popup;
	CtkWidget *view_popup;
	CtkWidget *hpaned;
	CtkWidget *menuitem;

	g_return_if_fail (EOC_IS_WINDOW (window));

	priv = window->priv;

	priv->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_container_add (CTK_CONTAINER (window), priv->box);
	ctk_widget_show (priv->box);

	priv->ui_mgr = ctk_ui_manager_new ();

	priv->actions_window = ctk_action_group_new ("MenuActionsWindow");

	ctk_action_group_set_translation_domain (priv->actions_window,
						 GETTEXT_PACKAGE);

	ctk_action_group_add_actions (priv->actions_window,
				      action_entries_window,
				      G_N_ELEMENTS (action_entries_window),
				      window);

	ctk_action_group_add_toggle_actions (priv->actions_window,
					     toggle_entries_window,
					     G_N_ELEMENTS (toggle_entries_window),
					     window);

	ctk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_window, 0);

	priv->actions_image = ctk_action_group_new ("MenuActionsImage");
	ctk_action_group_set_translation_domain (priv->actions_image,
						 GETTEXT_PACKAGE);

	ctk_action_group_add_actions (priv->actions_image,
				      action_entries_image,
				      G_N_ELEMENTS (action_entries_image),
				      window);

	eoc_window_add_open_editor_action (window);

	ctk_action_group_add_toggle_actions (priv->actions_image,
					     toggle_entries_image,
					     G_N_ELEMENTS (toggle_entries_image),
					     window);

	ctk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_image, 0);

	priv->actions_collection = ctk_action_group_new ("MenuActionsCollection");
	ctk_action_group_set_translation_domain (priv->actions_collection,
						 GETTEXT_PACKAGE);

	ctk_action_group_add_actions (priv->actions_collection,
				      action_entries_collection,
				      G_N_ELEMENTS (action_entries_collection),
				      window);

	ctk_action_group_add_toggle_actions (priv->actions_collection,
					     toggle_entries_collection,
					     G_N_ELEMENTS (toggle_entries_collection),
					     window);

	set_action_properties (priv->actions_window,
			       priv->actions_image,
			       priv->actions_collection);

	ctk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_collection, 0);

	if (!ctk_ui_manager_add_ui_from_resource (priv->ui_mgr,
	                                          "/org/cafe/eoc/ui/eoc-ui.xml",
	                                          &error)) {
		g_warning ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_signal_connect (priv->ui_mgr, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_mgr, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), window);

	menubar = ctk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (CTK_IS_WIDGET (menubar));
	ctk_box_pack_start (CTK_BOX (priv->box), menubar, FALSE, FALSE, 0);
	ctk_widget_show (menubar);

	menuitem = ctk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipHorizontal");
	ctk_image_menu_item_set_always_show_image (
			CTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = ctk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipVertical");
	ctk_image_menu_item_set_always_show_image (
			CTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = ctk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate90");
	ctk_image_menu_item_set_always_show_image (
			CTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = ctk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate270");
	ctk_image_menu_item_set_always_show_image (
			CTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	priv->toolbar = CTK_WIDGET
		(g_object_new (EGG_TYPE_EDITABLE_TOOLBAR,
			       "ui-manager", priv->ui_mgr,
			       "popup-path", "/ToolbarPopup",
			       "model", eoc_application_get_toolbars_model (EOC_APP),
			       NULL));

	ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (priv->toolbar)),
				     CTK_STYLE_CLASS_PRIMARY_TOOLBAR);

	egg_editable_toolbar_show (EGG_EDITABLE_TOOLBAR (priv->toolbar),
				   "Toolbar");

	ctk_box_pack_start (CTK_BOX (priv->box),
			    priv->toolbar,
			    FALSE,
			    FALSE,
			    0);

	ctk_widget_show (priv->toolbar);

	ctk_window_add_accel_group (CTK_WINDOW (window),
				    ctk_ui_manager_get_accel_group (priv->ui_mgr));

	priv->actions_recent = ctk_action_group_new ("RecentFilesActions");
	ctk_action_group_set_translation_domain (priv->actions_recent,
						 GETTEXT_PACKAGE);

	g_signal_connect (ctk_recent_manager_get_default (), "changed",
			  G_CALLBACK (eoc_window_recent_manager_changed_cb),
			  window);

	eoc_window_update_recent_files_menu (window);

	ctk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_recent, 0);

	priv->cbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_box_pack_start (CTK_BOX (priv->box), priv->cbox, TRUE, TRUE, 0);
	ctk_widget_show (priv->cbox);

	priv->statusbar = eoc_statusbar_new ();
	ctk_box_pack_end (CTK_BOX (priv->box),
			  CTK_WIDGET (priv->statusbar),
			  FALSE, FALSE, 0);
	ctk_widget_show (priv->statusbar);

	priv->image_info_message_cid =
		ctk_statusbar_get_context_id (CTK_STATUSBAR (priv->statusbar),
					      "image_info_message");
	priv->tip_message_cid =
		ctk_statusbar_get_context_id (CTK_STATUSBAR (priv->statusbar),
					      "tip_message");

	priv->layout = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);

	hpaned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);

	priv->sidebar = eoc_sidebar_new ();
	/* The sidebar shouldn't be shown automatically on show_all(),
	   but only when the user actually wants it. */
	ctk_widget_set_no_show_all (priv->sidebar, TRUE);

	ctk_widget_set_size_request (priv->sidebar, 210, -1);

	g_signal_connect_after (priv->sidebar,
				"show",
				G_CALLBACK (eoc_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"hide",
				G_CALLBACK (eoc_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-added",
				G_CALLBACK (eoc_window_sidebar_page_added),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-removed",
				G_CALLBACK (eoc_window_sidebar_page_removed),
				window);

 	priv->view = eoc_scroll_view_new ();

	eoc_sidebar_add_page (EOC_SIDEBAR (priv->sidebar),
			      _("Properties"),
			      CTK_WIDGET (eoc_metadata_sidebar_new (window)));

	ctk_widget_set_size_request (CTK_WIDGET (priv->view), 100, 100);
	g_signal_connect (G_OBJECT (priv->view),
			  "zoom_changed",
			  G_CALLBACK (view_zoom_changed_cb),
			  window);

	g_settings_bind (priv->view_settings, EOC_CONF_VIEW_SCROLL_WHEEL_ZOOM,
			 priv->view, "scrollwheel-zoom", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->view_settings, EOC_CONF_VIEW_ZOOM_MULTIPLIER,
			 priv->view, "zoom-multiplier", G_SETTINGS_BIND_GET);

	view_popup = ctk_ui_manager_get_widget (priv->ui_mgr, "/ViewPopup");
	eoc_scroll_view_set_popup (EOC_SCROLL_VIEW (priv->view),
				   CTK_MENU (view_popup));

	ctk_paned_pack1 (CTK_PANED (hpaned),
			 priv->sidebar,
			 FALSE,
			 FALSE);

	ctk_paned_pack2 (CTK_PANED (hpaned),
			 priv->view,
			 TRUE,
			 FALSE);

	ctk_widget_show_all (hpaned);

	ctk_box_pack_start (CTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);

	priv->thumbview = g_object_ref (eoc_thumb_view_new ());

	/* giving shape to the view */
	ctk_icon_view_set_margin (CTK_ICON_VIEW (priv->thumbview), 4);
	ctk_icon_view_set_row_spacing (CTK_ICON_VIEW (priv->thumbview), 0);

	g_signal_connect (G_OBJECT (priv->thumbview), "selection_changed",
			  G_CALLBACK (handle_image_selection_changed_cb), window);

	priv->nav = eoc_thumb_nav_new (priv->thumbview,
				       EOC_THUMB_NAV_MODE_ONE_ROW,
				       g_settings_get_boolean (priv->ui_settings,
							      EOC_CONF_UI_SCROLL_BUTTONS));

	// Bind the scroll buttons to their GSettings key
	g_settings_bind (priv->ui_settings, EOC_CONF_UI_SCROLL_BUTTONS,
			 priv->nav, "show-buttons", G_SETTINGS_BIND_GET);

	thumb_popup = ctk_ui_manager_get_widget (priv->ui_mgr, "/ThumbnailPopup");
	eoc_thumb_view_set_thumbnail_popup (EOC_THUMB_VIEW (priv->thumbview),
					    CTK_MENU (thumb_popup));

	ctk_box_pack_start (CTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);

	ctk_box_pack_end (CTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	eoc_window_can_save_changed_cb (priv->lockdown_settings,
					EOC_CONF_LOCKDOWN_CAN_SAVE,
					window);
	g_settings_bind (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION_POSITION,
			 window, "collection-position", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->ui_settings, EOC_CONF_UI_IMAGE_COLLECTION_RESIZABLE,
			 window, "collection-resizable", G_SETTINGS_BIND_GET);

	update_action_groups_state (window);

	if ((priv->flags & EOC_STARTUP_FULLSCREEN) ||
	    (priv->flags & EOC_STARTUP_SLIDE_SHOW)) {
		eoc_window_run_fullscreen (window, (priv->flags & EOC_STARTUP_SLIDE_SHOW));
	} else {
		priv->mode = EOC_WINDOW_MODE_NORMAL;
		update_ui_visibility (window);
	}

	eoc_window_set_drag_dest (window);
}

static void
eoc_window_init (EocWindow *window)
{
	CdkGeometry hints;
	CdkScreen *screen;
	EocWindowPrivate *priv;

	eoc_debug (DEBUG_WINDOW);

	CtkStyleContext *context;

	context = ctk_widget_get_style_context (CTK_WIDGET (window));
	ctk_style_context_add_class (context, "eoc-window");

	hints.min_width  = EOC_WINDOW_MIN_WIDTH;
	hints.min_height = EOC_WINDOW_MIN_HEIGHT;

	screen = ctk_widget_get_screen (CTK_WIDGET (window));

	priv = window->priv = eoc_window_get_instance_private (window);

	priv->view_settings = g_settings_new (EOC_CONF_VIEW);
	priv->ui_settings = g_settings_new (EOC_CONF_UI);
	priv->fullscreen_settings = g_settings_new (EOC_CONF_FULLSCREEN);
	priv->lockdown_settings = g_settings_new (EOC_CONF_LOCKDOWN_SCHEMA);

	g_signal_connect (priv->lockdown_settings,
					  "changed::" EOC_CONF_LOCKDOWN_CAN_SAVE,
					  G_CALLBACK (eoc_window_can_save_changed_cb),
					  window);

	window->priv->store = NULL;
	window->priv->image = NULL;

	window->priv->fullscreen_popup = NULL;
	window->priv->fullscreen_timeout_source = NULL;
	window->priv->slideshow_random = FALSE;
	window->priv->slideshow_loop = FALSE;
	window->priv->slideshow_switch_timeout = 0;
	window->priv->slideshow_switch_source = NULL;
	window->priv->fullscreen_idle_inhibit_cookie = 0;

	ctk_window_set_geometry_hints (CTK_WINDOW (window),
				       CTK_WIDGET (window),
				       &hints,
				       CDK_HINT_MIN_SIZE);

	ctk_window_set_default_size (CTK_WINDOW (window),
				     EOC_WINDOW_DEFAULT_WIDTH,
				     EOC_WINDOW_DEFAULT_HEIGHT);

	ctk_window_set_position (CTK_WINDOW (window), CTK_WIN_POS_CENTER);

	window->priv->mode = EOC_WINDOW_MODE_UNKNOWN;
	window->priv->status = EOC_WINDOW_STATUS_UNKNOWN;

#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
	window->priv->display_profile =
		eoc_window_get_display_profile (screen);
#endif

	window->priv->recent_menu_id = 0;

	window->priv->collection_position = 0;
	window->priv->collection_resizable = FALSE;

	window->priv->save_disabled = FALSE;

	window->priv->page_setup = NULL;

	ctk_window_set_application (CTK_WINDOW (window), CTK_APPLICATION (EOC_APP));
}

static void
eoc_window_dispose (GObject *object)
{
	EocWindow *window;
	EocWindowPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (EOC_IS_WINDOW (object));

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (object);
	priv = window->priv;

	bean_engine_garbage_collect (BEAN_ENGINE (EOC_APP->priv->plugin_engine));

	if (priv->extensions != NULL) {
		g_object_unref (priv->extensions);
		priv->extensions = NULL;
		bean_engine_garbage_collect (BEAN_ENGINE (EOC_APP->priv->plugin_engine));
	}

	if (priv->page_setup != NULL) {
		g_object_unref (priv->page_setup);
		priv->page_setup = NULL;
	}

	if (priv->thumbview)
	{
		/* Disconnect so we don't get any unwanted callbacks
		 * when the thumb view is disposed. */
		g_signal_handlers_disconnect_by_func (priv->thumbview,
		                 G_CALLBACK (handle_image_selection_changed_cb),
		                 window);
		g_clear_object (&priv->thumbview);
	}

	if (priv->store != NULL) {
		g_signal_handlers_disconnect_by_func (priv->store,
					      eoc_window_list_store_image_added,
					      window);
		g_signal_handlers_disconnect_by_func (priv->store,
					    eoc_window_list_store_image_removed,
					    window);
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	if (priv->image != NULL) {
	  	g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);
		g_object_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->actions_window != NULL) {
		g_object_unref (priv->actions_window);
		priv->actions_window = NULL;
	}

	if (priv->actions_image != NULL) {
		g_object_unref (priv->actions_image);
		priv->actions_image = NULL;
	}

	if (priv->actions_collection != NULL) {
		g_object_unref (priv->actions_collection);
		priv->actions_collection = NULL;
	}

	if (priv->actions_recent != NULL) {
		g_object_unref (priv->actions_recent);
		priv->actions_recent = NULL;
	}

        if (priv->actions_open_with != NULL) {
                g_object_unref (priv->actions_open_with);
                priv->actions_open_with = NULL;
        }

	fullscreen_clear_timeout (window);

	if (window->priv->fullscreen_popup != NULL) {
		ctk_widget_destroy (priv->fullscreen_popup);
		priv->fullscreen_popup = NULL;
	}

	slideshow_clear_timeout (window);
	eoc_window_uninhibit_screensaver (window);

	g_signal_handlers_disconnect_by_func (ctk_recent_manager_get_default (),
					      G_CALLBACK (eoc_window_recent_manager_changed_cb),
					      window);

	priv->recent_menu_id = 0;

	eoc_window_clear_load_job (window);

	eoc_window_clear_transform_job (window);

	if (priv->view_settings) {
		g_object_unref (priv->view_settings);
		priv->view_settings = NULL;
	}
	if (priv->ui_settings) {
		g_object_unref (priv->ui_settings);
		priv->ui_settings = NULL;
	}
	if (priv->fullscreen_settings) {
		g_object_unref (priv->fullscreen_settings);
		priv->fullscreen_settings = NULL;
	}
	if (priv->lockdown_settings) {
		g_object_unref (priv->lockdown_settings);
		priv->lockdown_settings = NULL;
	}

	if (priv->file_list != NULL) {
		g_slist_foreach (priv->file_list, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->file_list);
		priv->file_list = NULL;
	}

#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
	if (priv->display_profile != NULL) {
		cmsCloseProfile (priv->display_profile);
		priv->display_profile = NULL;
	}
#endif

	if (priv->last_save_as_folder != NULL) {
		g_object_unref (priv->last_save_as_folder);
		priv->last_save_as_folder = NULL;
	}

	bean_engine_garbage_collect (BEAN_ENGINE (EOC_APP->priv->plugin_engine));

	G_OBJECT_CLASS (eoc_window_parent_class)->dispose (object);
}

static gint
eoc_window_delete (CtkWidget   *widget,
		   CdkEventAny *event G_GNUC_UNUSED)
{
	EocWindow *window;
	EocWindowPrivate *priv;

	g_return_val_if_fail (EOC_IS_WINDOW (widget), FALSE);

	window = EOC_WINDOW (widget);
	priv = window->priv;

	if (priv->save_job != NULL) {
		eoc_window_finish_saving (window);
	}

	if (eoc_window_unsaved_images_confirm (window)) {
		return TRUE;
	}

	ctk_widget_destroy (widget);

	return TRUE;
}

static gint
eoc_window_key_press (CtkWidget *widget, CdkEventKey *event)
{
	CtkContainer *tbcontainer = CTK_CONTAINER ((EOC_WINDOW (widget)->priv->toolbar));
	gint result = FALSE;
	gboolean handle_selection = FALSE;

	switch (event->keyval) {
	case CDK_KEY_space:
		if (event->state & CDK_CONTROL_MASK) {
			handle_selection = TRUE;
			break;
		}
	case CDK_KEY_Return:
		if (ctk_container_get_focus_child (tbcontainer) == NULL) {
			/* Image properties dialog case */
			if (event->state & CDK_MOD1_MASK) {
				result = FALSE;
				break;
			}

			if (event->state & CDK_SHIFT_MASK) {
				eoc_window_cmd_go_prev (NULL, EOC_WINDOW (widget));
			} else {
				eoc_window_cmd_go_next (NULL, EOC_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case CDK_KEY_p:
	case CDK_KEY_P:
		if (EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_FULLSCREEN || EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_SLIDESHOW) {
			gboolean slideshow;

			slideshow = EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_SLIDESHOW;
			eoc_window_run_fullscreen (EOC_WINDOW (widget), !slideshow);
		}
		break;
	case CDK_KEY_Q:
	case CDK_KEY_q:
	case CDK_KEY_Escape:
		if (EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_FULLSCREEN) {
			eoc_window_stop_fullscreen (EOC_WINDOW (widget), FALSE);
		} else if (EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_SLIDESHOW) {
			eoc_window_stop_fullscreen (EOC_WINDOW (widget), TRUE);
		} else {
			eoc_window_cmd_close_window (NULL, EOC_WINDOW (widget));
			return TRUE;
		}
		break;
	case CDK_KEY_Left:
		if (event->state & CDK_MOD1_MASK) {
			/* Alt+Left moves to previous image */
			if (is_rtl) { /* move to next in RTL mode */
				eoc_window_cmd_go_next (NULL, EOC_WINDOW (widget));
			} else {
				eoc_window_cmd_go_prev (NULL, EOC_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case CDK_KEY_Up:
		if (eoc_scroll_view_scrollbars_visible (EOC_SCROLL_VIEW (EOC_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (ctk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!ctk_widget_get_visible (EOC_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == CDK_KEY_Left) {
				/* handle RTL fall-through,
				 * need to behave like CDK_Down then */
				eoc_window_cmd_go_next (NULL,
							EOC_WINDOW (widget));
			} else {
				eoc_window_cmd_go_prev (NULL,
							EOC_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case CDK_KEY_Right:
		if (event->state & CDK_MOD1_MASK) {
			/* Alt+Right moves to next image */
			if (is_rtl) { /* move to previous in RTL mode */
				eoc_window_cmd_go_prev (NULL, EOC_WINDOW (widget));
			} else {
				eoc_window_cmd_go_next (NULL, EOC_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case CDK_KEY_Down:
		if (eoc_scroll_view_scrollbars_visible (EOC_SCROLL_VIEW (EOC_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (ctk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!ctk_widget_get_visible (EOC_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == CDK_KEY_Right) {
				/* handle RTL fall-through,
				 * need to behave like CDK_Up then */
				eoc_window_cmd_go_prev (NULL,
							EOC_WINDOW (widget));
			} else {
				eoc_window_cmd_go_next (NULL,
							EOC_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case CDK_KEY_Page_Up:
		if (!eoc_scroll_view_scrollbars_visible (EOC_SCROLL_VIEW (EOC_WINDOW (widget)->priv->view))) {
			if (!ctk_widget_get_visible (EOC_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * previous image manually as it won't handle
				 * the keypress then. */
				eoc_window_cmd_go_prev (NULL,
							EOC_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	case CDK_KEY_Page_Down:
		if (!eoc_scroll_view_scrollbars_visible (EOC_SCROLL_VIEW (EOC_WINDOW (widget)->priv->view))) {
			if (!ctk_widget_get_visible (EOC_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * next image manually as it won't handle
				 * the keypress then. */
				eoc_window_cmd_go_next (NULL,
							EOC_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	}

	/* Update slideshow timeout */
	if (result && (EOC_WINDOW (widget)->priv->mode == EOC_WINDOW_MODE_SLIDESHOW)) {
		slideshow_set_timeout (EOC_WINDOW (widget));
	}

	if (handle_selection == TRUE && result == FALSE) {
		ctk_widget_grab_focus (CTK_WIDGET (EOC_WINDOW (widget)->priv->thumbview));

		result = ctk_widget_event (CTK_WIDGET (EOC_WINDOW (widget)->priv->thumbview),
					   (CdkEvent *) event);
	}

	/* If the focus is not in the toolbar and we still haven't handled the
	   event, give the scrollview a chance to do it.  */
	if (!ctk_container_get_focus_child (tbcontainer) && result == FALSE &&
		ctk_widget_get_realized (CTK_WIDGET (EOC_WINDOW (widget)->priv->view))) {
			result = ctk_widget_event (CTK_WIDGET (EOC_WINDOW (widget)->priv->view),
						   (CdkEvent *) event);
	}

	if (result == FALSE && CTK_WIDGET_CLASS (eoc_window_parent_class)->key_press_event) {
		result = (* CTK_WIDGET_CLASS (eoc_window_parent_class)->key_press_event) (widget, event);
	}

	return result;
}

static gint
eoc_window_button_press (CtkWidget *widget, CdkEventButton *event)
{
	EocWindow *window = EOC_WINDOW (widget);
	gint result = FALSE;

	if (event->type == CDK_BUTTON_PRESS) {
		switch (event->button) {
		case 6:
			eoc_thumb_view_select_single (EOC_THUMB_VIEW (window->priv->thumbview),
						      EOC_THUMB_VIEW_SELECT_LEFT);
			result = TRUE;
		       	break;
		case 7:
			eoc_thumb_view_select_single (EOC_THUMB_VIEW (window->priv->thumbview),
						      EOC_THUMB_VIEW_SELECT_RIGHT);
			result = TRUE;
		       	break;
		}
	}

	if (result == FALSE && CTK_WIDGET_CLASS (eoc_window_parent_class)->button_press_event) {
		result = (* CTK_WIDGET_CLASS (eoc_window_parent_class)->button_press_event) (widget, event);
	}

	return result;
}

static gboolean
eoc_window_focus_out_event (CtkWidget *widget, CdkEventFocus *event)
{
	EocWindow *window = EOC_WINDOW (widget);
	EocWindowPrivate *priv = window->priv;
	gboolean fullscreen;

	eoc_debug (DEBUG_WINDOW);

	fullscreen = priv->mode == EOC_WINDOW_MODE_FULLSCREEN ||
		     priv->mode == EOC_WINDOW_MODE_SLIDESHOW;

	if (fullscreen) {
		ctk_widget_hide (priv->fullscreen_popup);
	}

	return CTK_WIDGET_CLASS (eoc_window_parent_class)->focus_out_event (widget, event);
}

static void
eoc_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	EocWindow *window;
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (object));

	window = EOC_WINDOW (object);
	priv = window->priv;

	switch (property_id) {
	case PROP_COLLECTION_POS:
		eoc_window_set_collection_mode (window, g_value_get_enum (value),
					     priv->collection_resizable);
		break;
	case PROP_COLLECTION_RESIZABLE:
		eoc_window_set_collection_mode (window, priv->collection_position,
					     g_value_get_boolean (value));
		break;
	case PROP_STARTUP_FLAGS:
		priv->flags = g_value_get_flags (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eoc_window_get_property (GObject    *object,
			 guint       property_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	EocWindow *window;
	EocWindowPrivate *priv;

	g_return_if_fail (EOC_IS_WINDOW (object));

	window = EOC_WINDOW (object);
	priv = window->priv;

	switch (property_id) {
	case PROP_COLLECTION_POS:
		g_value_set_enum (value, priv->collection_position);
		break;
	case PROP_COLLECTION_RESIZABLE:
		g_value_set_boolean (value, priv->collection_resizable);
		break;
	case PROP_STARTUP_FLAGS:
		g_value_set_flags (value, priv->flags);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
on_extension_added (BeanExtensionSet *set G_GNUC_UNUSED,
		    BeanPluginInfo   *info G_GNUC_UNUSED,
		    BeanExtension    *exten,
		    CtkWindow        *window)
{
	bean_extension_call (exten, "activate", window);
}

static void
on_extension_removed (BeanExtensionSet *set G_GNUC_UNUSED,
		      BeanPluginInfo   *info G_GNUC_UNUSED,
		      BeanExtension    *exten,
		      CtkWindow        *window)
{
	bean_extension_call (exten, "deactivate", window);
}

static GObject *
eoc_window_constructor (GType type,
			guint n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject *object;
	EocWindowPrivate *priv;

	object = G_OBJECT_CLASS (eoc_window_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	priv = EOC_WINDOW (object)->priv;

	eoc_window_construct_ui (EOC_WINDOW (object));

	priv->extensions = bean_extension_set_new (BEAN_ENGINE (EOC_APP->priv->plugin_engine),
	                                           EOC_TYPE_WINDOW_ACTIVATABLE,
	                                           "window",
	                                           EOC_WINDOW (object), NULL);

	bean_extension_set_call (priv->extensions, "activate");

	g_signal_connect (priv->extensions, "extension-added",
			  G_CALLBACK (on_extension_added), object);
	g_signal_connect (priv->extensions, "extension-removed",
			  G_CALLBACK (on_extension_removed), object);

	return object;
}

static void
eoc_window_class_init (EocWindowClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	CtkWidgetClass *widget_class = (CtkWidgetClass *) class;

	g_object_class->constructor = eoc_window_constructor;
	g_object_class->dispose = eoc_window_dispose;
	g_object_class->set_property = eoc_window_set_property;
	g_object_class->get_property = eoc_window_get_property;

	widget_class->delete_event = eoc_window_delete;
	widget_class->key_press_event = eoc_window_key_press;
	widget_class->button_press_event = eoc_window_button_press;
	widget_class->drag_data_received = eoc_window_drag_data_received;
	widget_class->focus_out_event = eoc_window_focus_out_event;

/**
 * EocWindow:collection-position:
 *
 * Determines the position of the image collection in the window
 * relative to the image.
 */
	g_object_class_install_property (
		g_object_class, PROP_COLLECTION_POS,
		g_param_spec_enum ("collection-position", NULL, NULL,
				   EOC_TYPE_WINDOW_COLLECTION_POS,
				   EOC_WINDOW_COLLECTION_POS_BOTTOM,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * EocWindow:collection-resizable:
 *
 * If %TRUE the collection will be resizable by the user otherwise it will be
 * in single column/row mode.
 */
	g_object_class_install_property (
		g_object_class, PROP_COLLECTION_RESIZABLE,
		g_param_spec_boolean ("collection-resizable", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * EocWindow:startup-flags:
 *
 * A bitwise OR of #EocStartupFlags elements, indicating how the window
 * should behave upon creation.
 */
	g_object_class_install_property (g_object_class,
					 PROP_STARTUP_FLAGS,
					 g_param_spec_flags ("startup-flags",
							     NULL,
							     NULL,
							     EOC_TYPE_STARTUP_FLAGS,
					 		     0,
					 		     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));

/**
 * EocWindow::prepared:
 * @window: the object which received the signal.
 *
 * The #EocWindow::prepared signal is emitted when the @window is ready
 * to be shown.
 */
	signals [SIGNAL_PREPARED] =
		g_signal_new ("prepared",
			      EOC_TYPE_WINDOW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EocWindowClass, prepared),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * eoc_window_new:
 * @flags: the initialization parameters for the new window.
 *
 *
 * Creates a new and empty #EocWindow. Use @flags to indicate
 * if the window should be initialized fullscreen, in slideshow mode,
 * and/or without the thumbnails collection visible. See #EocStartupFlags.
 *
 * Returns: a newly created #EocWindow.
 **/
CtkWidget*
eoc_window_new (EocStartupFlags flags)
{
	EocWindow *window;

	eoc_debug (DEBUG_WINDOW);

	window = EOC_WINDOW (g_object_new (EOC_TYPE_WINDOW,
	                                   "type", CTK_WINDOW_TOPLEVEL,
	                                   "application", EOC_APP,
	                                   "show-menubar", FALSE,
	                                   "startup-flags", flags,
	                                   NULL));

	return CTK_WIDGET (window);
}

static void
eoc_window_list_store_image_added (CtkTreeModel *tree_model G_GNUC_UNUSED,
				   CtkTreePath  *path G_GNUC_UNUSED,
				   CtkTreeIter  *iter G_GNUC_UNUSED,
				   gpointer      user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
eoc_window_list_store_image_removed (CtkTreeModel *tree_model G_GNUC_UNUSED,
				     CtkTreePath  *path G_GNUC_UNUSED,
				     gpointer      user_data)
{
	EocWindow *window = EOC_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
eoc_job_model_cb (EocJobModel *job, gpointer data)
{
	EocWindow *window;
	EocWindowPrivate *priv;
	gint n_images;

	eoc_debug (DEBUG_WINDOW);

#ifdef HAVE_EXIF
	int i;
	EocImage *image;
#endif

	g_return_if_fail (EOC_IS_WINDOW (data));

	window = EOC_WINDOW (data);
	priv = window->priv;

	if (priv->store != NULL) {
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	priv->store = g_object_ref (job->store);

	n_images = eoc_list_store_length (EOC_LIST_STORE (priv->store));

#ifdef HAVE_EXIF
	if (g_settings_get_boolean (priv->view_settings, EOC_CONF_VIEW_AUTOROTATE)) {
		for (i = 0; i < n_images; i++) {
			image = eoc_list_store_get_image_by_pos (priv->store, i);
			eoc_image_autorotate (image);
			g_object_unref (image);
		}
	}
#endif

	eoc_thumb_view_set_model (EOC_THUMB_VIEW (priv->thumbview), priv->store);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-inserted",
			  G_CALLBACK (eoc_window_list_store_image_added),
			  window);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-deleted",
			  G_CALLBACK (eoc_window_list_store_image_removed),
			  window);

	if (n_images == 0) {
		gint n_files;

		priv->status = EOC_WINDOW_STATUS_NORMAL;
		update_action_groups_state (window);

		n_files = g_slist_length (priv->file_list);

		if (n_files > 0) {
			CtkWidget *message_area;
			GFile *file = NULL;

			if (n_files == 1) {
				file = (GFile *) priv->file_list->data;
			}

			message_area = eoc_no_images_error_message_area_new (file);

			eoc_window_set_message_area (window, message_area);

			ctk_widget_show (message_area);
		}

		g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
	}
}

/**
 * eoc_window_open_file_list:
 * @window: An #EocWindow.
 * @file_list: (element-type GFile): A %NULL-terminated list of #GFile's.
 *
 * Opens a list of files, adding them to the collection in @window.
 * Files will be checked to be readable and later filtered according
 * with eoc_list_store_add_files().
 **/
void
eoc_window_open_file_list (EocWindow *window, GSList *file_list)
{
	EocJob *job;

	eoc_debug (DEBUG_WINDOW);

	window->priv->status = EOC_WINDOW_STATUS_INIT;

	g_slist_foreach (file_list, (GFunc) g_object_ref, NULL);
	window->priv->file_list = file_list;

	job = eoc_job_model_new (file_list);

	g_signal_connect (job,
			  "finished",
			  G_CALLBACK (eoc_job_model_cb),
			  window);

	eoc_job_queue_add_job (job);
	g_object_unref (job);
}

/**
 * eoc_window_get_ui_manager:
 * @window: An #EocWindow.
 *
 * Gets the #CtkUIManager that describes the UI of @window.
 *
 * Returns: (transfer none): A #CtkUIManager.
 **/
CtkUIManager *
eoc_window_get_ui_manager (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->ui_mgr;
}

/**
 * eoc_window_get_mode:
 * @window: An #EocWindow.
 *
 * Gets the mode of @window. See #EocWindowMode for details.
 *
 * Returns: An #EocWindowMode.
 **/
EocWindowMode
eoc_window_get_mode (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), EOC_WINDOW_MODE_UNKNOWN);

	return window->priv->mode;
}

/**
 * eoc_window_set_mode:
 * @window: an #EocWindow.
 * @mode: an #EocWindowMode value.
 *
 * Changes the mode of @window to normal, fullscreen, or slideshow.
 * See #EocWindowMode for details.
 **/
void
eoc_window_set_mode (EocWindow *window, EocWindowMode mode)
{
	g_return_if_fail (EOC_IS_WINDOW (window));

	if (window->priv->mode == mode)
		return;

	switch (mode) {
	case EOC_WINDOW_MODE_NORMAL:
		eoc_window_stop_fullscreen (window,
					    window->priv->mode == EOC_WINDOW_MODE_SLIDESHOW);
		break;
	case EOC_WINDOW_MODE_FULLSCREEN:
		eoc_window_run_fullscreen (window, FALSE);
		break;
	case EOC_WINDOW_MODE_SLIDESHOW:
		eoc_window_run_fullscreen (window, TRUE);
		break;
	case EOC_WINDOW_MODE_UNKNOWN:
		break;
	}
}

/**
 * eoc_window_get_store:
 * @window: An #EocWindow.
 *
 * Gets the #EocListStore that contains the images in the collection
 * of @window.
 *
 * Returns: (transfer none): an #EocListStore.
 **/
EocListStore *
eoc_window_get_store (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return EOC_LIST_STORE (window->priv->store);
}

/**
 * eoc_window_get_view:
 * @window: An #EocWindow.
 *
 * Gets the #EocScrollView in the window.
 *
 * Returns: (transfer none): the #EocScrollView.
 **/
CtkWidget *
eoc_window_get_view (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->view;
}

/**
 * eoc_window_get_sidebar:
 * @window: An #EocWindow.
 *
 * Gets the sidebar widget of @window.
 *
 * Returns: (transfer none): the #EocSidebar.
 **/
CtkWidget *
eoc_window_get_sidebar (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->sidebar;
}

/**
 * eoc_window_get_thumb_view:
 * @window: an #EocWindow.
 *
 * Gets the thumbnails view in @window.
 *
 * Returns: (transfer none): an #EocThumbView.
 **/
CtkWidget *
eoc_window_get_thumb_view (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->thumbview;
}

/**
 * eoc_window_get_thumb_nav:
 * @window: an #EocWindow.
 *
 * Gets the thumbnails navigation pane in @window.
 *
 * Returns: (transfer none): an #EocThumbNav.
 **/
CtkWidget *
eoc_window_get_thumb_nav (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->nav;
}

/**
 * eoc_window_get_statusbar:
 * @window: an #EocWindow.
 *
 * Gets the statusbar in @window.
 *
 * Returns: (transfer none): a #EocStatusBar.
 **/
CtkWidget *
eoc_window_get_statusbar (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->statusbar;
}

/**
 * eoc_window_get_image:
 * @window: an #EocWindow.
 *
 * Gets the image currently displayed in @window or %NULL if
 * no image is being displayed.
 *
 * Returns: (transfer none): an #EocImage.
 **/
EocImage *
eoc_window_get_image (EocWindow *window)
{
	g_return_val_if_fail (EOC_IS_WINDOW (window), NULL);

	return window->priv->image;
}

/**
 * eoc_window_is_empty:
 * @window: an #EocWindow.
 *
 * Tells whether @window is currently empty or not.
 *
 * Returns: %TRUE if @window has no images, %FALSE otherwise.
 **/
gboolean
eoc_window_is_empty (EocWindow *window)
{
	EocWindowPrivate *priv;
	gboolean empty = TRUE;

	eoc_debug (DEBUG_WINDOW);

	g_return_val_if_fail (EOC_IS_WINDOW (window), FALSE);

	priv = window->priv;

	if (priv->store != NULL) {
		empty = (eoc_list_store_length (EOC_LIST_STORE (priv->store)) == 0);
	}

	return empty;
}

void
eoc_window_reload_image (EocWindow *window)
{
	CtkWidget *view;

	g_return_if_fail (EOC_IS_WINDOW (window));

	if (window->priv->image == NULL)
		return;

	g_object_unref (window->priv->image);
	window->priv->image = NULL;

	view = eoc_window_get_view (window);
	eoc_scroll_view_set_image (EOC_SCROLL_VIEW (view), NULL);

	eoc_thumb_view_select_single (EOC_THUMB_VIEW (window->priv->thumbview),
				      EOC_THUMB_VIEW_SELECT_CURRENT);
}
