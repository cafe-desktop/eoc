/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2002 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* based upon file transupp.h from the libjpeg package, original copyright
 * note follows:
 *
 * transupp.h
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for image transformation routines and
 * other utility code used by the jpegtran sample application.  These are
 * NOT part of the core JPEG library.  But we keep these routines separate
 * from jpegtran.c to ease the task of maintaining jpegtran-like programs
 * that have other user interfaces.
 *
 * NOTE: all the routines declared here have very specific requirements
 * about when they are to be executed during the reading and writing of the
 * source and destination files.  See the comments in transupp.c, or see
 * jpegtran.c for an example of correct usage.
 */

/*
 * Codes for supported types of image transformations.
 */

#ifdef HAVE_LIBJPEG


#ifndef TRANSUPP_H
#define TRANSUPP_H

#include <jpeglib.h>

#define GNUC_UNUSED \
  __attribute__ ((__unused__))


typedef enum {
	JXFORM_NONE,		/* no transformation */
	JXFORM_FLIP_H,		/* horizontal flip */
	JXFORM_FLIP_V,		/* vertical flip */
	JXFORM_TRANSPOSE,	/* transpose across UL-to-LR axis */
	JXFORM_TRANSVERSE,	/* transpose across UR-to-LL axis */
	JXFORM_ROT_90,		/* 90-degree clockwise rotation */
	JXFORM_ROT_180,		/* 180-degree rotation */
	JXFORM_ROT_270		/* 270-degree clockwise (or 90 ccw) */
} JXFORM_CODE;

/*
 * Although rotating and flipping data expressed as DCT coefficients is not
 * hard, there is an asymmetry in the JPEG format specification for images
 * whose dimensions aren't multiples of the iMCU size.  The right and bottom
 * image edges are padded out to the next iMCU boundary with junk data; but
 * no padding is possible at the top and left edges.  If we were to flip
 * the whole image including the pad data, then pad garbage would become
 * visible at the top and/or left, and real pixels would disappear into the
 * pad margins --- perhaps permanently, since encoders & decoders may not
 * bother to preserve DCT blocks that appear to be completely outside the
 * nominal image area.  So, we have to exclude any partial iMCUs from the
 * basic transformation.
 *
 * Transpose is the only transformation that can handle partial iMCUs at the
 * right and bottom edges completely cleanly.  flip_h can flip partial iMCUs
 * at the bottom, but leaves any partial iMCUs at the right edge untouched.
 * Similarly flip_v leaves any partial iMCUs at the bottom edge untouched.
 * The other transforms are defined as combinations of these basic transforms
 * and process edge blocks in a way that preserves the equivalence.
 *
 * The "trim" option causes untransformable partial iMCUs to be dropped;
 * this is not strictly lossless, but it usually gives the best-looking
 * result for odd-size images.  Note that when this option is active,
 * the expected mathematical equivalences between the transforms may not hold.
 * (For example, -rot 270 -trim trims only the bottom edge, but -rot 90 -trim
 * followed by -rot 180 -trim trims both edges.)
 *
 * We also offer a "force to grayscale" option, which simply discards the
 * chrominance channels of a YCbCr image.  This is lossless in the sense that
 * the luminance channel is preserved exactly.  It's not the same kind of
 * thing as the rotate/flip transformations, but it's convenient to handle it
 * as part of this package, mainly because the transformation routines have to
 * be aware of the option to know how many components to work on.
 */

typedef struct {
  /* Options: set by caller */
  JXFORM_CODE transform;	/* image transform operator */
  boolean trim;			/* if TRUE, trim partial MCUs as needed */
  boolean force_grayscale;	/* if TRUE, convert color image to grayscale */

  /* Internal workspace: caller should not touch these */
  int num_components;		/* # of components in workspace */
  jvirt_barray_ptr * workspace_coef_arrays; /* workspace for transformations */
} jpeg_transform_info;


/* Request any required workspace */
void jtransform_request_workspace   (j_decompress_ptr     srcinfo,
				     jpeg_transform_info *info);

/* Adjust output image parameters */

jvirt_barray_ptr * jtransform_adjust_parameters (j_decompress_ptr     srcinfo,
						 j_compress_ptr       dstinfo,
						 jvirt_barray_ptr    *src_coef_arrays,
						 jpeg_transform_info *info);

/* Execute the actual transformation, if any */
void jtransform_execute_transformation (j_decompress_ptr     srcinfo,
					j_compress_ptr       dstinfo,
					jvirt_barray_ptr    *src_coef_arrays,
					jpeg_transform_info *info);


/*
 * Support for copying optional markers from source to destination file.
 */

typedef enum {
	JCOPYOPT_NONE,		/* copy no optional markers */
	JCOPYOPT_COMMENTS,	/* copy only comment (COM) markers */
	JCOPYOPT_ALL		/* copy all optional markers */
} JCOPY_OPTION;

#define JCOPYOPT_DEFAULT  JCOPYOPT_COMMENTS	/* recommended default */

/* Setup decompression object to save desired markers in memory */
void jcopy_markers_setup (j_decompress_ptr srcinfo,
			  JCOPY_OPTION     option);

/* Copy markers saved in the given source object to the destination object */
void jcopy_markers_execute (j_decompress_ptr srcinfo,
			    j_compress_ptr   dstinfo,
			    JCOPY_OPTION     option);


#endif /* TRANSUPP_H */

#endif /* HAVE_LIBJPEG */
