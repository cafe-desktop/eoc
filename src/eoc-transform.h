#ifndef _EOC_TRANSFORM_H_
#define _EOC_TRANSFORM_H_

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_JOB_DECLR__
#define __EOC_JOB_DECLR__
typedef struct _EomJob EomJob;
#endif

typedef enum {
	EOC_TRANSFORM_NONE,
	EOC_TRANSFORM_ROT_90,
	EOC_TRANSFORM_ROT_180,
	EOC_TRANSFORM_ROT_270,
	EOC_TRANSFORM_FLIP_HORIZONTAL,
	EOC_TRANSFORM_FLIP_VERTICAL,
	EOC_TRANSFORM_TRANSPOSE,
	EOC_TRANSFORM_TRANSVERSE
} EomTransformType;

#define EOC_TYPE_TRANSFORM          (eoc_transform_get_type ())
#define EOC_TRANSFORM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_TRANSFORM, EomTransform))
#define EOC_TRANSFORM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_TRANSFORM, EomTransformClass))
#define EOC_IS_TRANSFORM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_TRANSFORM))
#define EOC_IS_TRANSFORM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_TRANSFORM))
#define EOC_TRANSFORM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_TRANSFORM, EomTransformClass))

/* =========================================

    GObjecat wrapper around an affine transformation

   ----------------------------------------*/

typedef struct _EomTransform EomTransform;
typedef struct _EomTransformClass EomTransformClass;
typedef struct _EomTransformPrivate EomTransformPrivate;

struct _EomTransform {
	GObject parent;

	EomTransformPrivate *priv;
};

struct _EomTransformClass {
	GObjectClass parent_klass;
};

GType         eoc_transform_get_type (void) G_GNUC_CONST;

GdkPixbuf*    eoc_transform_apply   (EomTransform *trans, GdkPixbuf *pixbuf, EomJob *job);
EomTransform* eoc_transform_reverse (EomTransform *trans);
EomTransform* eoc_transform_compose (EomTransform *trans, EomTransform *compose);
gboolean      eoc_transform_is_identity (EomTransform *trans);

EomTransform* eoc_transform_identity_new (void);
EomTransform* eoc_transform_rotate_new (int degree);
EomTransform* eoc_transform_flip_new   (EomTransformType type /* only EOC_TRANSFORM_FLIP_* are valid */);
#if 0
EomTransform* eoc_transform_scale_new  (double sx, double sy);
#endif
EomTransform* eoc_transform_new (EomTransformType trans);

EomTransformType eoc_transform_get_transform_type (EomTransform *trans);

gboolean         eoc_transform_get_affine (EomTransform *trans, cairo_matrix_t *affine);

G_END_DECLS

#endif /* _EOC_TRANSFORM_H_ */


