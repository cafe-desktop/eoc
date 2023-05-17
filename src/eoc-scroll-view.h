#ifndef _EOC_SCROLL_VIEW_H_
#define _EOC_SCROLL_VIEW_H_

#include <gtk/gtk.h>
#include "eoc-image.h"

G_BEGIN_DECLS

typedef struct _EomScrollView EomScrollView;
typedef struct _EomScrollViewClass EomScrollViewClass;
typedef struct _EomScrollViewPrivate EomScrollViewPrivate;

#define EOC_TYPE_SCROLL_VIEW              (eoc_scroll_view_get_type ())
#define EOC_SCROLL_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_SCROLL_VIEW, EomScrollView))
#define EOC_SCROLL_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_SCROLL_VIEW, EomScrollViewClass))
#define EOC_IS_SCROLL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_SCROLL_VIEW))
#define EOC_IS_SCROLL_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_SCROLL_VIEW))


struct _EomScrollView {
	GtkGrid base_instance;

	EomScrollViewPrivate *priv;
};

struct _EomScrollViewClass {
	GtkGridClass parent_class;

	void (* zoom_changed) (EomScrollView *view, double zoom);
};

typedef enum {
	EOC_TRANSP_BACKGROUND,
	EOC_TRANSP_CHECKED,
	EOC_TRANSP_COLOR
} EomTransparencyStyle;

GType    eoc_scroll_view_get_type         (void) G_GNUC_CONST;
GtkWidget* eoc_scroll_view_new            (void);

/* loading stuff */
void     eoc_scroll_view_set_image        (EomScrollView *view, EomImage *image);
EomImage* eoc_scroll_view_get_image       (EomScrollView *view);

/* general properties */
void     eoc_scroll_view_set_scroll_wheel_zoom (EomScrollView *view, gboolean scroll_wheel_zoom);
void     eoc_scroll_view_set_zoom_upscale (EomScrollView *view, gboolean upscale);
void     eoc_scroll_view_set_zoom_multiplier (EomScrollView *view, gdouble multiplier);
void     eoc_scroll_view_set_antialiasing_in (EomScrollView *view, gboolean state);
void     eoc_scroll_view_set_antialiasing_out (EomScrollView *view, gboolean state);
void     eoc_scroll_view_set_transparency_color (EomScrollView *view, GdkRGBA *color);
void     eoc_scroll_view_set_transparency (EomScrollView *view, EomTransparencyStyle style);
gboolean eoc_scroll_view_scrollbars_visible (EomScrollView *view);
void	 eoc_scroll_view_set_popup (EomScrollView *view, GtkMenu *menu);
void	 eoc_scroll_view_set_background_color (EomScrollView *view,
					       const GdkRGBA *color);
void	 eoc_scroll_view_override_bg_color (EomScrollView *view,
					    const GdkRGBA *color);
void     eoc_scroll_view_set_use_bg_color (EomScrollView *view, gboolean use);
/* zoom api */
void     eoc_scroll_view_zoom_in          (EomScrollView *view, gboolean smooth);
void     eoc_scroll_view_zoom_out         (EomScrollView *view, gboolean smooth);
void     eoc_scroll_view_zoom_fit         (EomScrollView *view);
void     eoc_scroll_view_set_zoom         (EomScrollView *view, double zoom);
double   eoc_scroll_view_get_zoom         (EomScrollView *view);
gboolean eoc_scroll_view_get_zoom_is_min  (EomScrollView *view);
gboolean eoc_scroll_view_get_zoom_is_max  (EomScrollView *view);
void     eoc_scroll_view_show_cursor      (EomScrollView *view);
void     eoc_scroll_view_hide_cursor      (EomScrollView *view);

G_END_DECLS

#endif /* _EOC_SCROLL_VIEW_H_ */


