/* Eye Of Cafe - Thumbnail View
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#ifndef EOC_THUMB_VIEW_H
#define EOC_THUMB_VIEW_H

#include "eoc-image.h"
#include "eoc-list-store.h"

G_BEGIN_DECLS

#define EOC_TYPE_THUMB_VIEW            (eoc_thumb_view_get_type ())
#define EOC_THUMB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_THUMB_VIEW, EocThumbView))
#define EOC_THUMB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOC_TYPE_THUMB_VIEW, EocThumbViewClass))
#define EOC_IS_THUMB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_THUMB_VIEW))
#define EOC_IS_THUMB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOC_TYPE_THUMB_VIEW))
#define EOC_THUMB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOC_TYPE_THUMB_VIEW, EocThumbViewClass))

typedef struct _EocThumbView EocThumbView;
typedef struct _EocThumbViewClass EocThumbViewClass;
typedef struct _EocThumbViewPrivate EocThumbViewPrivate;

typedef enum {
	EOC_THUMB_VIEW_SELECT_CURRENT = 0,
	EOC_THUMB_VIEW_SELECT_LEFT,
	EOC_THUMB_VIEW_SELECT_RIGHT,
	EOC_THUMB_VIEW_SELECT_FIRST,
	EOC_THUMB_VIEW_SELECT_LAST,
	EOC_THUMB_VIEW_SELECT_RANDOM
} EocThumbViewSelectionChange;

struct _EocThumbView {
	CtkIconView icon_view;
	EocThumbViewPrivate *priv;
};

struct _EocThumbViewClass {
	 CtkIconViewClass icon_view_class;
};

GType       eoc_thumb_view_get_type 		    (void) G_GNUC_CONST;

CtkWidget  *eoc_thumb_view_new 			    (void);

void	    eoc_thumb_view_set_model 		    (EocThumbView *thumbview,
						     EocListStore *store);

void        eoc_thumb_view_set_item_height          (EocThumbView *thumbview,
						     gint          height);

guint	    eoc_thumb_view_get_n_selected 	    (EocThumbView *thumbview);

EocImage   *eoc_thumb_view_get_first_selected_image (EocThumbView *thumbview);

GList      *eoc_thumb_view_get_selected_images 	    (EocThumbView *thumbview);

void        eoc_thumb_view_select_single 	    (EocThumbView *thumbview,
						     EocThumbViewSelectionChange change);

void        eoc_thumb_view_set_current_image	    (EocThumbView *thumbview,
						     EocImage     *image,
						     gboolean     deselect_other);

void        eoc_thumb_view_set_thumbnail_popup      (EocThumbView *thumbview,
						     CtkMenu      *menu);

G_END_DECLS

#endif /* EOC_THUMB_VIEW_H */
