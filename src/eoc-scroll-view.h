#ifndef _EOC_SCROLL_VIEW_H_
#define _EOC_SCROLL_VIEW_H_

#include <ctk/ctk.h>
#include "eoc-image.h"

G_BEGIN_DECLS

typedef struct _EocScrollView EocScrollView;
typedef struct _EocScrollViewClass EocScrollViewClass;
typedef struct _EocScrollViewPrivate EocScrollViewPrivate;

#define EOC_TYPE_SCROLL_VIEW              (eoc_scroll_view_get_type ())
#define EOC_SCROLL_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOC_TYPE_SCROLL_VIEW, EocScrollView))
#define EOC_SCROLL_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EOC_TYPE_SCROLL_VIEW, EocScrollViewClass))
#define EOC_IS_SCROLL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOC_TYPE_SCROLL_VIEW))
#define EOC_IS_SCROLL_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EOC_TYPE_SCROLL_VIEW))


struct _EocScrollView {
	CtkGrid base_instance;

	EocScrollViewPrivate *priv;
};

struct _EocScrollViewClass {
	CtkGridClass parent_class;

	void (* zoom_changed) (EocScrollView *view, double zoom);
};

typedef enum {
	EOC_TRANSP_BACKGROUND,
	EOC_TRANSP_CHECKED,
	EOC_TRANSP_COLOR
} EocTransparencyStyle;

GType    eoc_scroll_view_get_type         (void) G_GNUC_CONST;
CtkWidget* eoc_scroll_view_new            (void);

/* loading stuff */
void     eoc_scroll_view_set_image        (EocScrollView *view, EocImage *image);
EocImage* eoc_scroll_view_get_image       (EocScrollView *view);

/* general properties */
void     eoc_scroll_view_set_scroll_wheel_zoom (EocScrollView *view, gboolean scroll_wheel_zoom);
void     eoc_scroll_view_set_zoom_upscale (EocScrollView *view, gboolean upscale);
void     eoc_scroll_view_set_zoom_multiplier (EocScrollView *view, gdouble multiplier);
void     eoc_scroll_view_set_antialiasing_in (EocScrollView *view, gboolean state);
void     eoc_scroll_view_set_antialiasing_out (EocScrollView *view, gboolean state);
void     eoc_scroll_view_set_transparency_color (EocScrollView *view, GdkRGBA *color);
void     eoc_scroll_view_set_transparency (EocScrollView *view, EocTransparencyStyle style);
gboolean eoc_scroll_view_scrollbars_visible (EocScrollView *view);
void	 eoc_scroll_view_set_popup (EocScrollView *view, CtkMenu *menu);
void	 eoc_scroll_view_set_background_color (EocScrollView *view,
					       const GdkRGBA *color);
void	 eoc_scroll_view_override_bg_color (EocScrollView *view,
					    const GdkRGBA *color);
void     eoc_scroll_view_set_use_bg_color (EocScrollView *view, gboolean use);
/* zoom api */
void     eoc_scroll_view_zoom_in          (EocScrollView *view, gboolean smooth);
void     eoc_scroll_view_zoom_out         (EocScrollView *view, gboolean smooth);
void     eoc_scroll_view_zoom_fit         (EocScrollView *view);
void     eoc_scroll_view_set_zoom         (EocScrollView *view, double zoom);
double   eoc_scroll_view_get_zoom         (EocScrollView *view);
gboolean eoc_scroll_view_get_zoom_is_min  (EocScrollView *view);
gboolean eoc_scroll_view_get_zoom_is_max  (EocScrollView *view);
void     eoc_scroll_view_show_cursor      (EocScrollView *view);
void     eoc_scroll_view_hide_cursor      (EocScrollView *view);

G_END_DECLS

#endif /* _EOC_SCROLL_VIEW_H_ */


