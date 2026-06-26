#ifndef _EOC_TRANSFORM_H_
#define _EOC_TRANSFORM_H_

#include <glib-object.h>
#include <cdk-pixbuf/cdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOC_JOB_DECLR__
#define __EOC_JOB_DECLR__
typedef struct _EocJob EocJob;
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
} EocTransformType;

#define EOC_TYPE_TRANSFORM          (eoc_transform_get_type ())
#define EOC_TRANSFORM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_TRANSFORM, EocTransform))
#define EOC_TRANSFORM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_TRANSFORM, EocTransformClass))
#define EOC_IS_TRANSFORM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_TRANSFORM))
#define EOC_IS_TRANSFORM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_TRANSFORM))
#define EOC_TRANSFORM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_TRANSFORM, EocTransformClass))

/* =========================================

    GObjecat wrapper around an affine transformation

   ----------------------------------------*/

typedef struct _EocTransform EocTransform;
typedef struct _EocTransformClass EocTransformClass;
typedef struct _EocTransformPrivate EocTransformPrivate;

struct _EocTransform {
	GObject parent;

	EocTransformPrivate *priv;
};

struct _EocTransformClass {
	GObjectClass parent_klass;
};

GType         eoc_transform_get_type (void) G_GNUC_CONST;

CdkPixbuf*    eoc_transform_apply   (EocTransform *trans, CdkPixbuf *pixbuf, EocJob *job);
EocTransform* eoc_transform_reverse (EocTransform *trans);
EocTransform* eoc_transform_compose (EocTransform *trans, EocTransform *compose);
gboolean      eoc_transform_is_identity (EocTransform *trans);

EocTransform* eoc_transform_identity_new (void);
EocTransform* eoc_transform_rotate_new (int degree);
EocTransform* eoc_transform_flip_new   (EocTransformType type /* only EOC_TRANSFORM_FLIP_* are valid */);
#if 0
EocTransform* eoc_transform_scale_new  (double sx, double sy);
#endif
EocTransform* eoc_transform_new (EocTransformType trans);

EocTransformType eoc_transform_get_transform_type (EocTransform *trans);

gboolean         eoc_transform_get_affine (EocTransform *trans, cairo_matrix_t *affine);

G_END_DECLS

#endif /* _EOC_TRANSFORM_H_ */


