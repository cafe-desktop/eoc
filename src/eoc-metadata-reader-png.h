/* Eye Of MATE -- PNG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
 *
 * Based on the old EocMetadataReader code.
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

#ifndef _EOC_METADATA_READER_PNG_H_
#define _EOC_METADATA_READER_PNG_H_

G_BEGIN_DECLS

#define EOC_TYPE_METADATA_READER_PNG		(eoc_metadata_reader_png_get_type ())
#define EOC_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_METADATA_READER_PNG, EocMetadataReaderPng))
#define EOC_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_METADATA_READER_PNG, EocMetadataReaderPngClass))
#define EOC_IS_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_METADATA_READER_PNG))
#define EOC_IS_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_METADATA_READER_PNG))
#define EOC_METADATA_READER_PNG_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_METADATA_READER_PNG, EocMetadataReaderPngClass))

typedef struct _EocMetadataReaderPng EocMetadataReaderPng;
typedef struct _EocMetadataReaderPngClass EocMetadataReaderPngClass;
typedef struct _EocMetadataReaderPngPrivate EocMetadataReaderPngPrivate;

struct _EocMetadataReaderPng {
	GObject parent;

	EocMetadataReaderPngPrivate *priv;
};

struct _EocMetadataReaderPngClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType		      eoc_metadata_reader_png_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _EOC_METADATA_READER_PNG_H_ */
