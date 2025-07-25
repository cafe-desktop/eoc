/* Eye Of CAFE -- JPEG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
 *
 * Based on the original EocMetadataReader code.
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

#include <string.h>
#include <cdk/cdkx.h>

#include "eoc-metadata-reader.h"
#include "eoc-metadata-reader-jpg.h"
#include "eoc-debug.h"

typedef enum {
	EMR_READ = 0,
	EMR_READ_SIZE_HIGH_BYTE,
	EMR_READ_SIZE_LOW_BYTE,
	EMR_READ_MARKER,
	EMR_SKIP_BYTES,
	EMR_READ_APP1,
	EMR_READ_EXIF,
	EMR_READ_XMP,
	EMR_READ_ICC,
	EMR_READ_IPTC,
	EMR_FINISHED
} EocMetadataReaderState;

typedef enum {
	EJA_EXIF = 0,
	EJA_XMP,
	EJA_OTHER
} EocJpegApp1Type;


#define EOC_JPEG_MARKER_START   0xFF
#define EOC_JPEG_MARKER_SOI     0xD8
#define EOC_JPEG_MARKER_APP1	0xE1
#define EOC_JPEG_MARKER_APP2	0xE2
#define EOC_JPEG_MARKER_APP14	0xED

#define IS_FINISHED(priv) (priv->state == EMR_READ  && \
                           priv->exif_chunk != NULL && \
                           priv->icc_chunk  != NULL && \
                           priv->iptc_chunk != NULL && \
                           priv->xmp_chunk  != NULL)

struct _EocMetadataReaderJpgPrivate {
	EocMetadataReaderState  state;

	/* data fields */
	guint    exif_len;
	gpointer exif_chunk;

	gpointer iptc_chunk;
	guint	 iptc_len;

	guint icc_len;
	gpointer icc_chunk;

	gpointer xmp_chunk;
	guint xmp_len;

	/* management fields */
	int      size;
	int      last_marker;
	int      bytes_read;
};

static void
eoc_metadata_reader_jpg_init_emr_iface (gpointer g_iface, gpointer iface_data);


G_DEFINE_TYPE_WITH_CODE (EocMetadataReaderJpg, eoc_metadata_reader_jpg,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (EOC_TYPE_METADATA_READER,
			                        eoc_metadata_reader_jpg_init_emr_iface) \
			                        G_ADD_PRIVATE (EocMetadataReaderJpg))


static void
eoc_metadata_reader_jpg_dispose (GObject *object)
{
	EocMetadataReaderJpg *emr = EOC_METADATA_READER_JPG (object);

	if (emr->priv->exif_chunk != NULL) {
		g_free (emr->priv->exif_chunk);
		emr->priv->exif_chunk = NULL;
	}

	if (emr->priv->iptc_chunk != NULL) {
		g_free (emr->priv->iptc_chunk);
		emr->priv->iptc_chunk = NULL;
	}

	if (emr->priv->xmp_chunk != NULL) {
		g_free (emr->priv->xmp_chunk);
		emr->priv->xmp_chunk = NULL;
	}

	if (emr->priv->icc_chunk != NULL) {
		g_free (emr->priv->icc_chunk);
		emr->priv->icc_chunk = NULL;
	}

	G_OBJECT_CLASS (eoc_metadata_reader_jpg_parent_class)->dispose (object);
}

static void
eoc_metadata_reader_jpg_init (EocMetadataReaderJpg *emr)
{
	EocMetadataReaderJpgPrivate *priv;

	priv = emr->priv =  eoc_metadata_reader_jpg_get_instance_private (emr);
	priv->exif_chunk = NULL;
	priv->exif_len = 0;
	priv->iptc_chunk = NULL;
	priv->iptc_len = 0;
	priv->icc_chunk = NULL;
	priv->icc_len = 0;
}

static void
eoc_metadata_reader_jpg_class_init (EocMetadataReaderJpgClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;

	object_class->dispose = eoc_metadata_reader_jpg_dispose;
}

static gboolean
eoc_metadata_reader_jpg_finished (EocMetadataReaderJpg *emr)
{
	g_return_val_if_fail (EOC_IS_METADATA_READER_JPG (emr), TRUE);

	return (emr->priv->state == EMR_FINISHED);
}


static EocJpegApp1Type
eoc_metadata_identify_app1 (gchar *buf, guint len)
{
 	if (len < 5) {
 		return EJA_OTHER;
 	}

 	if (len < 29) {
 		return (strncmp ("Exif", buf, 5) == 0 ? EJA_EXIF : EJA_OTHER);
 	}

 	if (strncmp ("Exif", buf, 5) == 0) {
 		return EJA_EXIF;
 	} else if (strncmp ("http://ns.adobe.com/xap/1.0/", buf, 29) == 0) {
 		return EJA_XMP;
 	}

 	return EJA_OTHER;
}

static void
eoc_metadata_reader_get_next_block (EocMetadataReaderJpgPrivate* priv,
				    guchar *chunk,
				    int* i,
				    const guchar *buf,
				    int len,
				    EocMetadataReaderState state)
{
	if (*i + priv->size < len) {
		/* read data in one block */
		memcpy ((guchar*) (chunk) + priv->bytes_read, &buf[*i], priv->size);
		priv->state = EMR_READ;
		*i = *i + priv->size - 1; /* the for-loop consumes the other byte */
	} else {
		int chunk_len = len - *i;
		memcpy ((guchar*) (chunk) + priv->bytes_read, &buf[*i], chunk_len);
		priv->bytes_read += chunk_len; /* bytes already read */
		priv->size = (*i + priv->size) - len; /* remaining data to read */
		*i = len - 1;
		priv->state = state;
	}
}

static void
eoc_metadata_reader_jpg_consume (EocMetadataReaderJpg *emr, const guchar *buf, guint len)
{
	EocMetadataReaderJpgPrivate *priv;
 	EocJpegApp1Type app1_type;
	int i;
	EocMetadataReaderState next_state = EMR_READ;
	guchar *chunk = NULL;

	g_return_if_fail (EOC_IS_METADATA_READER_JPG (emr));

	priv = emr->priv;

	if (priv->state == EMR_FINISHED) return;

	for (i = 0; (i < len) && (priv->state != EMR_FINISHED); i++) {

		switch (priv->state) {
		case EMR_READ:
			if (buf[i] == EOC_JPEG_MARKER_START) {
				priv->state = EMR_READ_MARKER;
			}
			else {
				priv->state = EMR_FINISHED;
			}
			break;

		case EMR_READ_MARKER:
			if ((buf [i] & 0xF0) == 0xE0 || buf[i] == 0xFE) {
			/* we are reading some sort of APPxx or COM marker */
				/* these are always followed by 2 bytes of size information */
				priv->last_marker = buf [i];
				priv->size = 0;
				priv->state = EMR_READ_SIZE_HIGH_BYTE;

				eoc_debug_message (DEBUG_IMAGE_DATA, "APPx or COM Marker Found: %x", priv->last_marker);
			}
			else {
				/* otherwise simply consume the byte */
				priv->state = EMR_READ;
			}
			break;

		case EMR_READ_SIZE_HIGH_BYTE:
			priv->size = (buf [i] & 0xff) << 8;
			priv->state = EMR_READ_SIZE_LOW_BYTE;
			break;

		case EMR_READ_SIZE_LOW_BYTE:
			priv->size |= (buf [i] & 0xff);

			if (priv->size > 2)  /* ignore the two size-bytes */
				priv->size -= 2;

			if (priv->size == 0) {
				priv->state = EMR_READ;
			} else if (priv->last_marker == EOC_JPEG_MARKER_APP1 &&
				   ((priv->exif_chunk == NULL) || (priv->xmp_chunk == NULL)))
			{
				priv->state = EMR_READ_APP1;
			} else if (priv->last_marker == EOC_JPEG_MARKER_APP2 &&
				   priv->icc_chunk == NULL && priv->size > 14)
			{
	 			/* Chunk has 14 bytes identification data */
				priv->state = EMR_READ_ICC;
			} else if (priv->last_marker == EOC_JPEG_MARKER_APP14 &&
				priv->iptc_chunk == NULL)
			{
				priv->state = EMR_READ_IPTC;
			} else {
				priv->state = EMR_SKIP_BYTES;
			}

			priv->last_marker = 0;
			break;

		case EMR_SKIP_BYTES:
			eoc_debug_message (DEBUG_IMAGE_DATA, "Skip bytes: %i", priv->size);

			if (i + priv->size < len) {
				i = i + priv->size - 1; /* the for-loop consumes the other byte */
				priv->size = 0;
			}
			else {
				priv->size = (i + priv->size) - len;
				i = len - 1;
			}
			if (priv->size == 0) { /* don't need to skip any more bytes */
				priv->state = EMR_READ;
			}
			break;

		case EMR_READ_APP1:
			eoc_debug_message (DEBUG_IMAGE_DATA, "Read APP1 data, Length: %i", priv->size);

			app1_type = eoc_metadata_identify_app1 ((gchar*) &buf[i], priv->size);

			switch (app1_type) {
			case EJA_EXIF:
				if (priv->exif_chunk == NULL) {
					priv->exif_chunk = g_new0 (guchar, priv->size);
					priv->exif_len = priv->size;
					priv->bytes_read = 0;
					chunk = priv->exif_chunk;
					next_state = EMR_READ_EXIF;
				} else {
					chunk = NULL;
					priv->state = EMR_SKIP_BYTES;
				}
				break;
			case EJA_XMP:
				if (priv->xmp_chunk == NULL) {
					priv->xmp_chunk = g_new0 (guchar, priv->size);
					priv->xmp_len = priv->size;
					priv->bytes_read = 0;
					chunk = priv->xmp_chunk;
					next_state = EMR_READ_XMP;
				} else {
					chunk = NULL;
					priv->state = EMR_SKIP_BYTES;
				}
				break;
			case EJA_OTHER:
			default:
				/* skip unknown data */
				chunk = NULL;
				priv->state = EMR_SKIP_BYTES;
				break;
			}

			if (chunk) {
				eoc_metadata_reader_get_next_block (priv, chunk,
								    &i, buf,
								    len,
								    next_state);
			}

			if (IS_FINISHED(priv))
				priv->state = EMR_FINISHED;
			break;

		case EMR_READ_EXIF:
			eoc_debug_message (DEBUG_IMAGE_DATA, "Read continuation of EXIF data, length: %i", priv->size);
			{
 				eoc_metadata_reader_get_next_block (priv, priv->exif_chunk,
 								    &i, buf, len, EMR_READ_EXIF);
			}
			if (IS_FINISHED(priv))
				priv->state = EMR_FINISHED;
			break;

		case EMR_READ_XMP:
			eoc_debug_message (DEBUG_IMAGE_DATA, "Read continuation of XMP data, length: %i", priv->size);
			{
				eoc_metadata_reader_get_next_block (priv, priv->xmp_chunk,
 								    &i, buf, len, EMR_READ_XMP);
			}
			if (IS_FINISHED (priv))
				priv->state = EMR_FINISHED;
			break;

		case EMR_READ_ICC:
			eoc_debug_message (DEBUG_IMAGE_DATA,
					   "Read continuation of ICC data, "
					   "length: %i", priv->size);

			if (priv->icc_chunk == NULL) {
				priv->icc_chunk = g_new0 (guchar, priv->size);
				priv->icc_len = priv->size;
				priv->bytes_read = 0;
			}

			eoc_metadata_reader_get_next_block (priv,
							    priv->icc_chunk,
							    &i, buf, len,
							    EMR_READ_ICC);

			/* Test that the chunk actually contains ICC data. */
			if (priv->state == EMR_READ && priv->icc_chunk) {
			    	const char* icc_chunk = priv->icc_chunk;
				gboolean valid = TRUE;

				/* Chunk should begin with the
				 * ICC_PROFILE\0 identifier */
				valid &= strncmp (icc_chunk,
						  "ICC_PROFILE\0",12) == 0;
				/* Make sure this is the first and only
				 * ICC chunk in the file as we don't
				 * support merging chunks yet. */
				valid &=  *(guint16*)(icc_chunk+12) == 0x101;

				if (!valid) {
					/* This no ICC data. Throw it away. */
					eoc_debug_message (DEBUG_IMAGE_DATA,
					"Supposed ICC chunk didn't validate. "
					"Ignoring.");
					g_free (priv->icc_chunk);
					priv->icc_chunk = NULL;
					priv->icc_len = 0;
				}
			}

			if (IS_FINISHED(priv))
				priv->state = EMR_FINISHED;
			break;

		case EMR_READ_IPTC:
			eoc_debug_message (DEBUG_IMAGE_DATA,
					   "Read continuation of IPTC data, "
					   "length: %i", priv->size);

			if (priv->iptc_chunk == NULL) {
				priv->iptc_chunk = g_new0 (guchar, priv->size);
				priv->iptc_len = priv->size;
				priv->bytes_read = 0;
			}

			eoc_metadata_reader_get_next_block (priv,
							    priv->iptc_chunk,
							    &i, buf, len,
							    EMR_READ_IPTC);

			if (IS_FINISHED(priv))
				priv->state = EMR_FINISHED;
			break;

		default:
			g_assert_not_reached ();
		}
	}
}

/* Returns the raw exif data. NOTE: The caller of this function becomes
 * the new owner of this piece of memory and is responsible for freeing it!
 */
static void
eoc_metadata_reader_jpg_get_exif_chunk (EocMetadataReaderJpg *emr, guchar **data, guint *len)
{
	EocMetadataReaderJpgPrivate *priv;

	g_return_if_fail (EOC_IS_METADATA_READER (emr));
	priv = emr->priv;

	*data = (guchar*) priv->exif_chunk;
	*len = priv->exif_len;

	priv->exif_chunk = NULL;
	priv->exif_len = 0;
}

#ifdef HAVE_EXIF
static gpointer
eoc_metadata_reader_jpg_get_exif_data (EocMetadataReaderJpg *emr)
{
	EocMetadataReaderJpgPrivate *priv;
	ExifData *data = NULL;

	g_return_val_if_fail (EOC_IS_METADATA_READER (emr), NULL);
	priv = emr->priv;

	if (priv->exif_chunk != NULL) {
		data = exif_data_new_from_data (priv->exif_chunk, priv->exif_len);
	}

	return data;
}
#endif


#ifdef HAVE_EXEMPI

/* skip the signature */
#define EOC_XMP_OFFSET (29)

static gpointer
eoc_metadata_reader_jpg_get_xmp_data (EocMetadataReaderJpg *emr )
{
	EocMetadataReaderJpgPrivate *priv;
	XmpPtr xmp = NULL;

	g_return_val_if_fail (EOC_IS_METADATA_READER (emr), NULL);

	priv = emr->priv;

	if (priv->xmp_chunk != NULL) {
		xmp = xmp_new (priv->xmp_chunk+EOC_XMP_OFFSET,
			       priv->xmp_len-EOC_XMP_OFFSET);
	}

	return (gpointer)xmp;
}
#endif

/*
 * FIXME: very broken, assumes the profile fits in a single chunk.  Change to
 * parse the sections and construct a single memory chunk, or maybe even parse
 * the profile.
 */
#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
static gpointer
eoc_metadata_reader_jpg_get_icc_profile (EocMetadataReaderJpg *emr)
{
	EocMetadataReaderJpgPrivate *priv;
	cmsHPROFILE profile = NULL;

	g_return_val_if_fail (EOC_IS_METADATA_READER (emr), NULL);

	priv = emr->priv;

	if (priv->icc_chunk) {
		if (CDK_IS_X11_DISPLAY (cdk_display_get_default ())) {
			profile = cmsOpenProfileFromMem(priv->icc_chunk + 14, priv->icc_len - 14);
		}

		if (profile) {
			eoc_debug_message (DEBUG_LCMS, "JPEG has ICC profile");
		} else {
			eoc_debug_message (DEBUG_LCMS, "JPEG has invalid ICC profile");
		}
	}

#ifdef HAVE_EXIF
	if (!profile && priv->exif_chunk != NULL) {
		ExifEntry *entry;
		ExifByteOrder o;
		gint color_space;
		ExifData *exif = eoc_metadata_reader_jpg_get_exif_data (emr);

		if (!exif) return NULL;

		o = exif_data_get_byte_order (exif);

		entry = exif_data_get_entry (exif, EXIF_TAG_COLOR_SPACE);

		if (entry == NULL) {
			exif_data_unref (exif);
			return NULL;
		}

		color_space = exif_get_short (entry->data, o);

		switch (color_space) {
		case 1:
			eoc_debug_message (DEBUG_LCMS, "JPEG is sRGB");

			profile = cmsCreate_sRGBProfile ();

			break;
		case 2:
			eoc_debug_message (DEBUG_LCMS, "JPEG is Adobe RGB (Disabled)");

			/* TODO: create Adobe RGB profile */
			//profile = cmsCreate_Adobe1998Profile ();

			break;
		case 0xFFFF:
		  	{
			cmsCIExyY whitepoint;
			cmsCIExyYTRIPLE primaries;
			cmsToneCurve *gamma[3];
			double gammaValue;
			ExifRational r;

			const int offset = exif_format_get_size (EXIF_FORMAT_RATIONAL);

			entry = exif_data_get_entry (exif, EXIF_TAG_WHITE_POINT);

			if (entry && entry->components == 2) {
				r = exif_get_rational (entry->data, o);
				whitepoint.x = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + offset, o);
				whitepoint.y = (double) r.numerator / r.denominator;
				whitepoint.Y = 1.0;
			} else {
				eoc_debug_message (DEBUG_LCMS, "No whitepoint found");
				break;
			}

			entry = exif_data_get_entry (exif, EXIF_TAG_PRIMARY_CHROMATICITIES);

			if (entry && entry->components == 6) {
				r = exif_get_rational (entry->data + 0 * offset, o);
				primaries.Red.x = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + 1 * offset, o);
				primaries.Red.y = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + 2 * offset, o);
				primaries.Green.x = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + 3 * offset, o);
				primaries.Green.y = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + 4 * offset, o);
				primaries.Blue.x = (double) r.numerator / r.denominator;

				r = exif_get_rational (entry->data + 5 * offset, o);
				primaries.Blue.y = (double) r.numerator / r.denominator;

				primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;
			} else {
				eoc_debug_message (DEBUG_LCMS, "No primary chromaticities found");
				break;
			}

			entry = exif_data_get_entry (exif, EXIF_TAG_GAMMA);

			if (entry) {
				r = exif_get_rational (entry->data, o);
				gammaValue = (double) r.numerator / r.denominator;
			} else {
				eoc_debug_message (DEBUG_LCMS, "No gamma found");
				gammaValue = 2.2;
			}

			gamma[0] = gamma[1] = gamma[2] = cmsBuildGamma (NULL, gammaValue);

			profile = cmsCreateRGBProfile (&whitepoint, &primaries, gamma);

			cmsFreeToneCurve(gamma[0]);

			eoc_debug_message (DEBUG_LCMS, "JPEG is calibrated");

			break;
			}
		}

		exif_data_unref (exif);
	}
#endif
	return profile;
}
#endif

static void
eoc_metadata_reader_jpg_init_emr_iface (gpointer g_iface,
					gpointer iface_data G_GNUC_UNUSED)
{
	EocMetadataReaderInterface *iface;

	iface = (EocMetadataReaderInterface*)g_iface;

	iface->consume =
		(void (*) (EocMetadataReader *self, const guchar *buf, guint len))
			eoc_metadata_reader_jpg_consume;
	iface->finished =
		(gboolean (*) (EocMetadataReader *self))
			eoc_metadata_reader_jpg_finished;
	iface->get_raw_exif =
		(void (*) (EocMetadataReader *self, guchar **data, guint *len))
			eoc_metadata_reader_jpg_get_exif_chunk;
#ifdef HAVE_EXIF
	iface->get_exif_data =
		(gpointer (*) (EocMetadataReader *self))
			eoc_metadata_reader_jpg_get_exif_data;
#endif
#if defined(HAVE_LCMS) && defined(CDK_WINDOWING_X11)
	iface->get_icc_profile =
		(gpointer (*) (EocMetadataReader *self))
			eoc_metadata_reader_jpg_get_icc_profile;
#endif
#ifdef HAVE_EXEMPI
	iface->get_xmp_ptr =
		(gpointer (*) (EocMetadataReader *self))
			eoc_metadata_reader_jpg_get_xmp_data;
#endif
}

