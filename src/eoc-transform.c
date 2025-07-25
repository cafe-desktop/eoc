/* Eye Of CAFE -- Affine Transformations
 *
 * Copyright (C) 2003-2009 The Free Software Foundation
 *
 * Portions based on code from libart_lgpl by Raph Levien.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <ctk/ctk.h>
#include <cairo/cairo.h>

#include "eoc-transform.h"
#include "eoc-jobs.h"

/* The number of progress updates per transformation */
#define EOC_TRANSFORM_N_PROG_UPDATES 20

struct _EocTransformPrivate {
	cairo_matrix_t affine;
};

typedef struct {
	gdouble x;
	gdouble y;
} EocPoint;

/* Convert degrees into radians */
#define EOC_DEG_TO_RAD(degree) ((degree) * (G_PI/180.0))

G_DEFINE_TYPE_WITH_PRIVATE (EocTransform, eoc_transform, G_TYPE_OBJECT)

static void
eoc_transform_init (EocTransform *trans)
{
	trans->priv = eoc_transform_get_instance_private (trans);
}

static void
eoc_transform_class_init (EocTransformClass *klass G_GNUC_UNUSED)
{

}

/**
 * eoc_transform_apply:
 * @trans: a #EocTransform
 * @pixbuf: a #GdkPixbuf
 * @job: a #EocJob
 *
 * Applies the transformation in @trans to @pixbuf, setting its progress in @job.
 *
 * Returns: (transfer full): A new #GdkPixbuf with the transformation applied.
 **/
GdkPixbuf*
eoc_transform_apply (EocTransform *trans, GdkPixbuf *pixbuf, EocJob *job)
{
	EocPoint dest_top_left;
	EocPoint dest_bottom_right;
	EocPoint vertices[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
	double r_det;
	int inverted [6];
	EocPoint dest;

	int src_width;
	int src_height;
	int src_rowstride;
	int src_n_channels;
	guchar *src_buffer;

	GdkPixbuf *dest_pixbuf;
	int dest_width;
	int dest_height;
	int dest_rowstride;
	int dest_n_channels;
	guchar *dest_buffer;

	guchar *src_pos;
	guchar *dest_pos;
	int dx, dy, sx, sy;
	int i, x, y;

	int progress_delta;

	g_return_val_if_fail (pixbuf != NULL, NULL);

	g_object_ref (pixbuf);

	src_width = gdk_pixbuf_get_width (pixbuf);
	src_height = gdk_pixbuf_get_height (pixbuf);
	src_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	src_n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	src_buffer = gdk_pixbuf_get_pixels (pixbuf);

	/* find out the dimension of the destination pixbuf */
	dest_top_left.x = 100000;
	dest_top_left.y = 100000;
	dest_bottom_right.x = -100000;
	dest_bottom_right.y = -100000;

	for (i = 0; i < 4; i++) {
		dest.x = vertices[i].x * (src_width - 1);
		dest.y = vertices[i].y * (src_height -1);

		cairo_matrix_transform_point (&trans->priv->affine,
					      &dest.x, &dest.y);

		dest_top_left.x = MIN (dest_top_left.x, dest.x);
		dest_top_left.y = MIN (dest_top_left.y, dest.y);

		dest_bottom_right.x = MAX (dest_bottom_right.x, dest.x);
		dest_bottom_right.y = MAX (dest_bottom_right.y, dest.y);
	}

	/* create the resulting pixbuf */
	dest_width = abs ((int) (dest_bottom_right.x - dest_top_left.x + 1));
	dest_height = abs ((int) (dest_bottom_right.y - dest_top_left.y + 1));

	dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			       gdk_pixbuf_get_has_alpha (pixbuf),
			       gdk_pixbuf_get_bits_per_sample (pixbuf),
			       dest_width,
			       dest_height);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest_pixbuf);
	dest_n_channels = gdk_pixbuf_get_n_channels (dest_pixbuf);
	dest_buffer = gdk_pixbuf_get_pixels (dest_pixbuf);

	/* invert the matrix so that we can compute the source pixel
	   from the target pixel and convert the values to integer
	   ones (faster!)  FIXME: Maybe we can do some more
	   improvements by using special mmx/3dnow features if
	   available.
	*/
	r_det = 1.0 / (trans->priv->affine.xx * trans->priv->affine.yy - trans->priv->affine.yx * trans->priv->affine.xy);
	inverted[0] =  trans->priv->affine.yy * r_det;
	inverted[1] = -trans->priv->affine.yx * r_det;
	inverted[2] = -trans->priv->affine.xy * r_det;
	inverted[3] =  trans->priv->affine.xx * r_det;
	inverted[4] = -trans->priv->affine.x0 * inverted[0] - trans->priv->affine.y0 * inverted[2];
	inverted[5] = -trans->priv->affine.x0 * inverted[1] - trans->priv->affine.y0 * inverted[3];

	progress_delta = MAX (1, dest_height / EOC_TRANSFORM_N_PROG_UPDATES);

	/*
	 * for every destination pixel (dx,dy) compute the source pixel (sx, sy)
	 * and copy the color values
	 */
	for (y = 0, dy = dest_top_left.y; y < dest_height; y++, dy++) {
		for (x = 0, dx = dest_top_left.x; x < dest_width; x++, dx++) {

			sx = dx * inverted[0] + dy * inverted[2] + inverted[4];
			sy = dx * inverted[1] + dy * inverted[3] + inverted[5];

			if (sx >= 0 && sx < src_width && sy >= 0 && sy < src_height) {
				src_pos  = src_buffer  + sy * src_rowstride  + sx * src_n_channels;
				dest_pos = dest_buffer +  y * dest_rowstride +  x * dest_n_channels;

				for (i = 0; i <  src_n_channels; i++) {
					dest_pos[i] = src_pos[i];
				}
			}
		}

		if (job != NULL && y % progress_delta == 0) {
			gfloat progress;

			progress = (gfloat) (y + 1.0) / (gfloat) dest_height;

			eoc_job_set_progress (job, progress);
		}
	}

	g_object_unref (pixbuf);

	if (job != NULL) {
		eoc_job_set_progress (job, 1.0);
	}

	return dest_pixbuf;
}

static void
_eoc_cairo_matrix_copy (const cairo_matrix_t *src, cairo_matrix_t *dest)
{
	cairo_matrix_init (dest, src->xx, src->yx, src->xy, src->yy, src->x0, src->y0);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)
/* art_affine_equal modified to work with cairo_matrix_t */
static gboolean
_eoc_cairo_matrix_equal (const cairo_matrix_t *a, const cairo_matrix_t *b)
{
	return (DOUBLE_EQUAL (a->xx, b->xx) && DOUBLE_EQUAL (a->yx, b->yx) &&
		DOUBLE_EQUAL (a->xy, b->xy) && DOUBLE_EQUAL (a->yy, b->yy) &&
		DOUBLE_EQUAL (a->x0, b->x0) && DOUBLE_EQUAL (a->y0, b->y0) );
}

/* art_affine_flip modified to work with cairo_matrix_t */
static void
_eoc_cairo_matrix_flip (cairo_matrix_t *dst, const cairo_matrix_t *src, gboolean horiz, gboolean vert)
{
	dst->xx = horiz ? -src->xx : src->xx;
	dst->yx = horiz ? -src->yx : src->yx;
	dst->xy = vert ? -src->xy : src->xy;
	dst->yy = vert ? -src->yy : src->yy;
	dst->x0 = horiz ? -src->x0 : src->x0;
	dst->y0 = vert ? -src->y0 : src->y0;
}

/**
 * eoc_transform_reverse:
 * @trans: a #EocTransform
 *
 * Creates the reverse transformation of @trans
 *
 * Returns: (transfer full): a new transformation
 **/
EocTransform*
eoc_transform_reverse (EocTransform *trans)
{
	EocTransform *reverse;

	g_return_val_if_fail (EOC_IS_TRANSFORM (trans), NULL);

	reverse = EOC_TRANSFORM (g_object_new (EOC_TYPE_TRANSFORM, NULL));

	_eoc_cairo_matrix_copy (&trans->priv->affine, &reverse->priv->affine);

	g_return_val_if_fail (cairo_matrix_invert (&reverse->priv->affine) == CAIRO_STATUS_SUCCESS, reverse);

	return reverse;
}

/**
 * eoc_transform_compose:
 * @trans: a #EocTransform
 * @compose: another #EocTransform
 *
 *
 *
 * Returns: (transfer full): a new transform
 **/
EocTransform*
eoc_transform_compose (EocTransform *trans, EocTransform *compose)
{
	EocTransform *composition;

	g_return_val_if_fail (EOC_IS_TRANSFORM (trans), NULL);
	g_return_val_if_fail (EOC_IS_TRANSFORM (compose), NULL);

	composition = EOC_TRANSFORM (g_object_new (EOC_TYPE_TRANSFORM, NULL));

	cairo_matrix_multiply (&composition->priv->affine,
			       &trans->priv->affine,
			       &compose->priv->affine);

	return composition;
}

gboolean
eoc_transform_is_identity (EocTransform *trans)
{
	static const cairo_matrix_t identity = { 1, 0, 0, 1, 0, 0 };

	g_return_val_if_fail (EOC_IS_TRANSFORM (trans), FALSE);

	return _eoc_cairo_matrix_equal (&identity, &trans->priv->affine);
}

EocTransform*
eoc_transform_identity_new (void)
{
	EocTransform *trans;

	trans = EOC_TRANSFORM (g_object_new (EOC_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_identity (&trans->priv->affine);

	return trans;
}

EocTransform*
eoc_transform_rotate_new (int degree)
{
	EocTransform *trans;

	trans = EOC_TRANSFORM (g_object_new (EOC_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_rotate (&trans->priv->affine, EOC_DEG_TO_RAD(degree));

	return trans;
}

EocTransform*
eoc_transform_flip_new   (EocTransformType type)
{
	EocTransform *trans;
	gboolean horiz, vert;

	trans = EOC_TRANSFORM (g_object_new (EOC_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_identity (&trans->priv->affine);

	horiz = (type == EOC_TRANSFORM_FLIP_HORIZONTAL);
	vert = (type == EOC_TRANSFORM_FLIP_VERTICAL);

	_eoc_cairo_matrix_flip (&trans->priv->affine,
				&trans->priv->affine,
				horiz, vert);

	return trans;
}

EocTransform*
eoc_transform_new (EocTransformType type)
{
	EocTransform *trans = NULL;
	EocTransform *temp1 = NULL, *temp2 = NULL;

	switch (type) {
	case EOC_TRANSFORM_NONE:
		trans = eoc_transform_identity_new ();
		break;
	case EOC_TRANSFORM_FLIP_HORIZONTAL:
		trans = eoc_transform_flip_new (EOC_TRANSFORM_FLIP_HORIZONTAL);
		break;
	case EOC_TRANSFORM_ROT_180:
		trans = eoc_transform_rotate_new (180);
		break;
	case EOC_TRANSFORM_FLIP_VERTICAL:
		trans = eoc_transform_flip_new (EOC_TRANSFORM_FLIP_VERTICAL);
		break;
	case EOC_TRANSFORM_TRANSPOSE:
		temp1 = eoc_transform_rotate_new (90);
		temp2 = eoc_transform_flip_new (EOC_TRANSFORM_FLIP_HORIZONTAL);
		trans = eoc_transform_compose (temp1, temp2);
		g_object_unref (temp1);
		g_object_unref (temp2);
		break;
	case EOC_TRANSFORM_ROT_90:
		trans = eoc_transform_rotate_new (90);
		break;
	case EOC_TRANSFORM_TRANSVERSE:
		temp1 = eoc_transform_rotate_new (90);
		temp2 = eoc_transform_flip_new (EOC_TRANSFORM_FLIP_VERTICAL);
		trans = eoc_transform_compose (temp1, temp2);
		g_object_unref (temp1);
		g_object_unref (temp2);
		break;
	case EOC_TRANSFORM_ROT_270:
		trans = eoc_transform_rotate_new (270);
		break;
	default:
		trans = eoc_transform_identity_new ();
		break;
	}

	return trans;
}

EocTransformType
eoc_transform_get_transform_type (EocTransform *trans)
{
	cairo_matrix_t affine, a1, a2;
	EocTransformPrivate *priv;

	g_return_val_if_fail (EOC_IS_TRANSFORM (trans), EOC_TRANSFORM_NONE);

	priv = trans->priv;

	cairo_matrix_init_rotate (&affine, EOC_DEG_TO_RAD(90));
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_ROT_90;
	}

	cairo_matrix_init_rotate (&affine, EOC_DEG_TO_RAD(180));
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_ROT_180;
	}

	cairo_matrix_init_rotate (&affine, EOC_DEG_TO_RAD(270));
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_ROT_270;
	}

	cairo_matrix_init_identity (&affine);
	_eoc_cairo_matrix_flip (&affine, &affine, TRUE, FALSE);
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_FLIP_HORIZONTAL;
	}

	cairo_matrix_init_identity (&affine);
	_eoc_cairo_matrix_flip (&affine, &affine, FALSE, TRUE);
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_FLIP_VERTICAL;
	}

	cairo_matrix_init_rotate (&a1, EOC_DEG_TO_RAD(90));
	cairo_matrix_init_identity (&a2);
	_eoc_cairo_matrix_flip (&a2, &a2, TRUE, FALSE);
	cairo_matrix_multiply(&affine, &a1, &a2);
	if (_eoc_cairo_matrix_equal (&affine, &priv->affine)) {
		return EOC_TRANSFORM_TRANSPOSE;
	}

	/* A transversion is a 180° rotation followed by a transposition */
	/* Reuse the transposition from the previous step for this. */
	cairo_matrix_init_rotate (&a1, EOC_DEG_TO_RAD(180));
	cairo_matrix_multiply(&a2, &a1, &affine);
	if (_eoc_cairo_matrix_equal (&a2, &priv->affine)) {
		return EOC_TRANSFORM_TRANSVERSE;
	}

	return EOC_TRANSFORM_NONE;
}

gboolean
eoc_transform_get_affine (EocTransform *trans, cairo_matrix_t *affine)
{
	g_return_val_if_fail (EOC_IS_TRANSFORM (trans), FALSE);

	_eoc_cairo_matrix_copy (&trans->priv->affine, affine);

	return TRUE;
}

