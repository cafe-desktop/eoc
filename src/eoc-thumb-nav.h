/* Eye Of Mate - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#ifndef __EOC_THUMB_NAV_H__
#define __EOC_THUMB_NAV_H__

#include "eoc-thumb-view.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _EocThumbNav EocThumbNav;
typedef struct _EocThumbNavClass EocThumbNavClass;
typedef struct _EocThumbNavPrivate EocThumbNavPrivate;

#define EOC_TYPE_THUMB_NAV            (eoc_thumb_nav_get_type ())
#define EOC_THUMB_NAV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOC_TYPE_THUMB_NAV, EocThumbNav))
#define EOC_THUMB_NAV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOC_TYPE_THUMB_NAV, EocThumbNavClass))
#define EOC_IS_THUMB_NAV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOC_TYPE_THUMB_NAV))
#define EOC_IS_THUMB_NAV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOC_TYPE_THUMB_NAV))
#define EOC_THUMB_NAV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOC_TYPE_THUMB_NAV, EocThumbNavClass))

typedef enum {
	EOC_THUMB_NAV_MODE_ONE_ROW,
	EOC_THUMB_NAV_MODE_ONE_COLUMN,
	EOC_THUMB_NAV_MODE_MULTIPLE_ROWS,
	EOC_THUMB_NAV_MODE_MULTIPLE_COLUMNS
} EocThumbNavMode;

struct _EocThumbNav {
	GtkBox base_instance;

	EocThumbNavPrivate *priv;
};

struct _EocThumbNavClass {
	GtkBoxClass parent_class;
};

GType	         eoc_thumb_nav_get_type          (void) G_GNUC_CONST;

GtkWidget       *eoc_thumb_nav_new               (GtkWidget         *thumbview,
						  EocThumbNavMode    mode,
	             			          gboolean           show_buttons);

gboolean         eoc_thumb_nav_get_show_buttons  (EocThumbNav       *nav);

void             eoc_thumb_nav_set_show_buttons  (EocThumbNav       *nav,
                                                  gboolean           show_buttons);

EocThumbNavMode  eoc_thumb_nav_get_mode          (EocThumbNav       *nav);

void             eoc_thumb_nav_set_mode          (EocThumbNav       *nav,
                                                  EocThumbNavMode    mode);

G_END_DECLS

#endif /* __EOC_THUMB_NAV_H__ */
