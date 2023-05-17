/* Eye Of MATE -- JPEG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
 *
 * Based on the original EomMetadataReader code.
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

#ifndef _EOC_METADATA_READER_JPG_H_
#define _EOC_METADATA_READER_JPG_H_

G_BEGIN_DECLS

#define EOC_TYPE_METADATA_READER_JPG		(eoc_metadata_reader_jpg_get_type ())
#define EOC_METADATA_READER_JPG(o)         	(G_TYPE_CHECK_INSTANCE_CAST ((o),EOC_TYPE_METADATA_READER_JPG, EomMetadataReaderJpg))
#define EOC_METADATA_READER_JPG_CLASS(k)   	(G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_METADATA_READER_JPG, EomMetadataReaderJpgClass))
#define EOC_IS_METADATA_READER_JPG(o)      	(G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_METADATA_READER_JPG))
#define EOC_IS_METADATA_READER_JPG_CLASS(k)   	(G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_METADATA_READER_JPG))
#define EOC_METADATA_READER_JPG_GET_CLASS(o)  	(G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_METADATA_READER_JPG, EomMetadataReaderJpgClass))

typedef struct _EomMetadataReaderJpg EomMetadataReaderJpg;
typedef struct _EomMetadataReaderJpgClass EomMetadataReaderJpgClass;
typedef struct _EomMetadataReaderJpgPrivate EomMetadataReaderJpgPrivate;

struct _EomMetadataReaderJpg {
	GObject parent;

	EomMetadataReaderJpgPrivate *priv;
};

struct _EomMetadataReaderJpgClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType		      eoc_metadata_reader_jpg_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _EOC_METADATA_READER_JPG_H_ */
