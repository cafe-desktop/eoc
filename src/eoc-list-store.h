/* Eye Of Cafe - Image Store
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 *
 * Based on code by: Jens Finke <jens@triq.net>
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

#ifndef EOC_LIST_STORE_H
#define EOC_LIST_STORE_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#ifndef __EOC_IMAGE_DECLR__
#define __EOC_IMAGE_DECLR__
  typedef struct _EocImage EocImage;
#endif

typedef struct _EocListStore EocListStore;
typedef struct _EocListStoreClass EocListStoreClass;
typedef struct _EocListStorePrivate EocListStorePrivate;

#define EOC_TYPE_LIST_STORE            eoc_list_store_get_type()
#define EOC_LIST_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_LIST_STORE, EocListStore))
#define EOC_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOC_TYPE_LIST_STORE, EocListStoreClass))
#define EOC_IS_LIST_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_LIST_STORE))
#define EOC_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOC_TYPE_LIST_STORE))
#define EOC_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOC_TYPE_LIST_STORE, EocListStoreClass))

#define EOC_LIST_STORE_THUMB_SIZE 90

typedef enum {
	EOC_LIST_STORE_THUMBNAIL = 0,
	EOC_LIST_STORE_THUMB_SET,
	EOC_LIST_STORE_EOC_IMAGE,
	EOC_LIST_STORE_EOC_JOB,
	EOC_LIST_STORE_NUM_COLUMNS
} EocListStoreColumn;

struct _EocListStore {
        GtkListStore parent;
	EocListStorePrivate *priv;
};

struct _EocListStoreClass {
        GtkListStoreClass parent_class;

	/* Padding for future expansion */
	void (* _eoc_reserved1) (void);
	void (* _eoc_reserved2) (void);
	void (* _eoc_reserved3) (void);
	void (* _eoc_reserved4) (void);
};

GType           eoc_list_store_get_type 	     (void) G_GNUC_CONST;

GtkListStore   *eoc_list_store_new 		     (void);

GtkListStore   *eoc_list_store_new_from_glist 	     (GList *list);

void            eoc_list_store_append_image 	     (EocListStore *store,
						      EocImage     *image);

void            eoc_list_store_add_files 	     (EocListStore *store,
						      GList        *file_list);

void            eoc_list_store_remove_image 	     (EocListStore *store,
						      EocImage     *image);

gint            eoc_list_store_get_pos_by_image      (EocListStore *store,
						      EocImage     *image);

EocImage       *eoc_list_store_get_image_by_pos      (EocListStore *store,
						      gint   pos);

gint            eoc_list_store_get_pos_by_iter 	     (EocListStore *store,
						      GtkTreeIter  *iter);

gint            eoc_list_store_length                (EocListStore *store);

gint            eoc_list_store_get_initial_pos 	     (EocListStore *store);

void            eoc_list_store_thumbnail_set         (EocListStore *store,
						      GtkTreeIter *iter);

void            eoc_list_store_thumbnail_unset       (EocListStore *store,
						      GtkTreeIter *iter);

void            eoc_list_store_thumbnail_refresh     (EocListStore *store,
						      GtkTreeIter *iter);

G_END_DECLS

#endif
