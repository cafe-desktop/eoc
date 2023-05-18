/* Eye Of CAFE -- Metadata Reader Interface
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eoc-metadata-reader.h"
#include "eoc-metadata-reader-jpg.h"
#include "eoc-metadata-reader-png.h"
#include "eoc-debug.h"

G_DEFINE_INTERFACE (EocMetadataReader, eoc_metadata_reader, G_TYPE_INVALID)

EocMetadataReader*
eoc_metadata_reader_new (EocMetadataFileType type)
{
	EocMetadataReader *emr;

	switch (type) {
	case EOC_METADATA_JPEG:
		emr = EOC_METADATA_READER (g_object_new (EOC_TYPE_METADATA_READER_JPG, NULL));
		break;
	case EOC_METADATA_PNG:
		emr = EOC_METADATA_READER (g_object_new (EOC_TYPE_METADATA_READER_PNG, NULL));
		break;
	default:
		emr = NULL;
		break;
	}

	return emr;
}

gboolean
eoc_metadata_reader_finished (EocMetadataReader *emr)
{
	g_return_val_if_fail (EOC_IS_METADATA_READER (emr), TRUE);

	return EOC_METADATA_READER_GET_INTERFACE (emr)->finished (emr);
}


void
eoc_metadata_reader_consume (EocMetadataReader *emr, const guchar *buf, guint len)
{
	EOC_METADATA_READER_GET_INTERFACE (emr)->consume (emr, buf, len);
}

/* Returns the raw exif data. NOTE: The caller of this function becomes
 * the new owner of this piece of memory and is responsible for freeing it!
 */
void
eoc_metadata_reader_get_exif_chunk (EocMetadataReader *emr, guchar **data, guint *len)
{
	g_return_if_fail (data != NULL && len != NULL);

	EOC_METADATA_READER_GET_INTERFACE (emr)->get_raw_exif (emr, data, len);
}

#ifdef HAVE_EXIF
ExifData*
eoc_metadata_reader_get_exif_data (EocMetadataReader *emr)
{
	return EOC_METADATA_READER_GET_INTERFACE (emr)->get_exif_data (emr);
}
#endif

#ifdef HAVE_EXEMPI
XmpPtr
eoc_metadata_reader_get_xmp_data (EocMetadataReader *emr)
{
	return EOC_METADATA_READER_GET_INTERFACE (emr)->get_xmp_ptr (emr);
}
#endif

#if defined(HAVE_LCMS) && defined(GDK_WINDOWING_X11)
cmsHPROFILE
eoc_metadata_reader_get_icc_profile (EocMetadataReader *emr)
{
	return EOC_METADATA_READER_GET_INTERFACE (emr)->get_icc_profile (emr);
}
#endif

/* Default vfunc that simply clears the output if not overriden by the
   implementing class. This mimics the old behavour of get_exif_chunk(). */
static void
_eoc_metadata_reader_default_get_raw_exif (EocMetadataReader *emr,
					   guchar **data, guint *length)
{
	g_return_if_fail (data != NULL && length != NULL);
	*data = NULL;
	*length = 0;
}

/* Default vfunc that simply returns NULL if not overriden by the implementing
   class. Mimics the old fallback behaviour of the getter functions. */
static gpointer
_eoc_metadata_reader_default_get_null (EocMetadataReader *emr)
{
	return NULL;
}
static void
eoc_metadata_reader_default_init (EocMetadataReaderInterface *iface)
{
	/* consume and finished are required to be implemented */
	/* Not-implemented funcs return NULL by default */
	iface->get_raw_exif = _eoc_metadata_reader_default_get_raw_exif;
	iface->get_exif_data = _eoc_metadata_reader_default_get_null;
	iface->get_icc_profile = _eoc_metadata_reader_default_get_null;
	iface->get_xmp_ptr = _eoc_metadata_reader_default_get_null;
}
