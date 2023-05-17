/* Eye Of MATE -- Metadata Reader Interface
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
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

#ifndef _EOC_METADATA_READER_H_
#define _EOC_METADATA_READER_H_

#include <glib-object.h>
#if HAVE_EXIF
#include "eoc-exif-util.h"
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif
#if HAVE_LCMS
#include <lcms2.h>
#endif

G_BEGIN_DECLS

#define EOC_TYPE_METADATA_READER	      (eoc_metadata_reader_get_type ())
#define EOC_METADATA_READER(o)		      (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_METADATA_READER, EomMetadataReader))
#define EOC_IS_METADATA_READER(o)	      (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_METADATA_READER))
#define EOC_METADATA_READER_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), EOC_TYPE_METADATA_READER, EomMetadataReaderInterface))

typedef struct _EomMetadataReader EomMetadataReader;
typedef struct _EomMetadataReaderInterface EomMetadataReaderInterface;

struct _EomMetadataReaderInterface {
	GTypeInterface parent;

	void		(*consume)		(EomMetadataReader *self,
						 const guchar *buf,
						 guint len);

	gboolean	(*finished)		(EomMetadataReader *self);

	void		(*get_raw_exif)		(EomMetadataReader *self,
						 guchar **data,
						 guint *len);

	gpointer	(*get_exif_data)	(EomMetadataReader *self);

	gpointer	(*get_icc_profile)	(EomMetadataReader *self);

	gpointer	(*get_xmp_ptr)		(EomMetadataReader *self);
};

typedef enum {
	EOC_METADATA_JPEG,
	EOC_METADATA_PNG
} EomMetadataFileType;

G_GNUC_INTERNAL
GType                eoc_metadata_reader_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
EomMetadataReader*   eoc_metadata_reader_new 		(EomMetadataFileType type);

G_GNUC_INTERNAL
void                 eoc_metadata_reader_consume	(EomMetadataReader *emr,
							 const guchar *buf,
							 guint len);

G_GNUC_INTERNAL
gboolean             eoc_metadata_reader_finished	(EomMetadataReader *emr);

G_GNUC_INTERNAL
void                 eoc_metadata_reader_get_exif_chunk (EomMetadataReader *emr,
							 guchar **data,
							 guint *len);

#ifdef HAVE_EXIF
G_GNUC_INTERNAL
ExifData*            eoc_metadata_reader_get_exif_data	(EomMetadataReader *emr);
#endif

#ifdef HAVE_EXEMPI
G_GNUC_INTERNAL
XmpPtr	     	     eoc_metadata_reader_get_xmp_data	(EomMetadataReader *emr);
#endif

#if 0
gpointer             eoc_metadata_reader_get_iptc_chunk	(EomMetadataReader *emr);
IptcData*            eoc_metadata_reader_get_iptc_data	(EomMetadataReader *emr);
#endif

#ifdef HAVE_LCMS
G_GNUC_INTERNAL
cmsHPROFILE          eoc_metadata_reader_get_icc_profile (EomMetadataReader *emr);
#endif

G_END_DECLS

#endif /* _EOC_METADATA_READER_H_ */
