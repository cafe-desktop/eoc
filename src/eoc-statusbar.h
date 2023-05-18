/* Eye of Cafe - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *	   Jens Finke <jens@gnome.org>
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

#ifndef __EOC_STATUSBAR_H__
#define __EOC_STATUSBAR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EocStatusbar        EocStatusbar;
typedef struct _EocStatusbarPrivate EocStatusbarPrivate;
typedef struct _EocStatusbarClass   EocStatusbarClass;

#define EOC_TYPE_STATUSBAR            (eoc_statusbar_get_type ())
#define EOC_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_STATUSBAR, EocStatusbar))
#define EOC_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),   EOC_TYPE_STATUSBAR, EocStatusbarClass))
#define EOC_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_STATUSBAR))
#define EOC_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOC_TYPE_STATUSBAR))
#define EOC_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOC_TYPE_STATUSBAR, EocStatusbarClass))

struct _EocStatusbar
{
        GtkStatusbar parent;

        EocStatusbarPrivate *priv;
};

struct _EocStatusbarClass
{
        GtkStatusbarClass parent_class;
};

GType		 eoc_statusbar_get_type			(void) G_GNUC_CONST;

GtkWidget	*eoc_statusbar_new			(void);

void		 eoc_statusbar_set_image_number		(EocStatusbar   *statusbar,
							 gint           num,
							 gint           tot);

void		 eoc_statusbar_set_progress		(EocStatusbar   *statusbar,
							 gdouble        progress);

G_END_DECLS

#endif /* __EOC_STATUSBAR_H__ */
