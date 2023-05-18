/* Eye Of Mate - GSettings Keys and Schemas definitions
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *         Stefano Karapetsas <stefano@karapetsas.com>
 *
 * Based on code by:
 *  - Federico Mena-Quintero <federico@gnu.org>
 *  - Jens Finke <jens@gnome.org>
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

#ifndef __EOC_CONFIG_KEYS_H__
#define __EOC_CONFIG_KEYS_H__

#define EOC_CONF_DOMAIN				"org.cafe.eoc"
#define EOC_CONF_FULLSCREEN			EOC_CONF_DOMAIN".full-screen"
#define EOC_CONF_PLUGINS			EOC_CONF_DOMAIN".plugins"
#define EOC_CONF_UI				EOC_CONF_DOMAIN".ui"
#define EOC_CONF_VIEW				EOC_CONF_DOMAIN".view"

#define EOC_CONF_BACKGROUND_SCHEMA              "org.cafe.background"
#define EOC_CONF_BACKGROUND_FILE                "picture-filename"

#define EOC_CONF_LOCKDOWN_SCHEMA                "org.cafe.lockdown"
#define EOC_CONF_LOCKDOWN_CAN_SAVE              "disable-save-to-disk"
#define EOC_CONF_LOCKDOWN_CAN_PRINT             "disable-printing"
#define EOC_CONF_LOCKDOWN_CAN_SETUP_PAGE        "disable-print-setup"

#define EOC_CONF_VIEW_BACKGROUND_COLOR          "background-color"
#define EOC_CONF_VIEW_INTERPOLATE               "interpolate"
#define EOC_CONF_VIEW_EXTRAPOLATE               "extrapolate"
#define EOC_CONF_VIEW_SCROLL_WHEEL_ZOOM         "scroll-wheel-zoom"
#define EOC_CONF_VIEW_ZOOM_MULTIPLIER           "zoom-multiplier"
#define EOC_CONF_VIEW_AUTOROTATE                "autorotate"
#define EOC_CONF_VIEW_TRANSPARENCY              "transparency"
#define EOC_CONF_VIEW_TRANS_COLOR               "trans-color"
#define EOC_CONF_VIEW_USE_BG_COLOR              "use-background-color"

#define EOC_CONF_FULLSCREEN_RANDOM              "random"
#define EOC_CONF_FULLSCREEN_LOOP                "loop"
#define EOC_CONF_FULLSCREEN_UPSCALE             "upscale"
#define EOC_CONF_FULLSCREEN_SECONDS             "seconds"

#define EOC_CONF_UI_TOOLBAR                     "toolbar"
#define EOC_CONF_UI_STATUSBAR                   "statusbar"
#define EOC_CONF_UI_IMAGE_COLLECTION            "image-collection"
#define EOC_CONF_UI_IMAGE_COLLECTION_POSITION   "image-collection-position"
#define EOC_CONF_UI_IMAGE_COLLECTION_RESIZABLE  "image-collection-resizable"
#define EOC_CONF_UI_SIDEBAR                     "sidebar"
#define EOC_CONF_UI_SCROLL_BUTTONS              "scroll-buttons"
#define EOC_CONF_UI_DISABLE_CLOSE_CONFIRMATION	"disable-close-confirmation"
#define EOC_CONF_UI_DISABLE_TRASH_CONFIRMATION  "disable-trash-confirmation"
#define EOC_CONF_UI_FILECHOOSER_XDG_FALLBACK    "filechooser-xdg-fallback"
#define EOC_CONF_UI_PROPSDIALOG_NETBOOK_MODE    "propsdialog-netbook-mode"
#define EOC_CONF_UI_EXTERNAL_EDITOR             "external-editor"

#define EOC_CONF_PLUGINS_ACTIVE_PLUGINS         "active-plugins"

#endif /* __EOC_CONFIG_KEYS_H__ */
