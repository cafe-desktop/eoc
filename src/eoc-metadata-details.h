/* Eye Of Cafe - EOC Image Exif Details
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

#ifndef __EOC_METADATA_DETAILS__
#define __EOC_METADATA_DETAILS__

#include <glib-object.h>
#include <ctk/ctk.h>
#if HAVE_EXIF
#include <libexif/exif-data.h>
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

G_BEGIN_DECLS

typedef struct _EocMetadataDetails EocMetadataDetails;
typedef struct _EocMetadataDetailsClass EocMetadataDetailsClass;
typedef struct _EocMetadataDetailsPrivate EocMetadataDetailsPrivate;

#define EOC_TYPE_METADATA_DETAILS            (eoc_metadata_details_get_type ())
#define EOC_METADATA_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_METADATA_DETAILS, EocMetadataDetails))
#define EOC_METADATA_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), EOC_TYPE_METADATA_DETAILS, EocMetadataDetailsClass))
#define EOC_IS_METADATA_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_METADATA_DETAILS))
#define EOC_IS_METADATA_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_METADATA_DETAILS))
#define EOC_METADATA_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EOC_TYPE_METADATA_DETAILS, EocMetadataDetailsClass))

struct _EocMetadataDetails {
        GtkTreeView parent;

	EocMetadataDetailsPrivate *priv;
};

struct _EocMetadataDetailsClass {
	GtkTreeViewClass parent_class;
};

G_GNUC_INTERNAL
GType               eoc_metadata_details_get_type    (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget          *eoc_metadata_details_new         (void);

#if HAVE_EXIF
G_GNUC_INTERNAL
void                eoc_metadata_details_update      (EocMetadataDetails *details,
                                                      ExifData       *data);
#endif
#if HAVE_EXEMPI
G_GNUC_INTERNAL
void                eoc_metadata_details_xmp_update  (EocMetadataDetails *details,
                                                      XmpPtr          xmp_data);
#endif

G_END_DECLS

#endif /* __EOC_METADATA_DETAILS__ */
