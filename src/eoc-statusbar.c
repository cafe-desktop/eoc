/* Eye of Cafe - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *	   Jens Finke <jens@gnome.org>
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
#include "config.h"
#endif

#include "eoc-statusbar.h"

#include <string.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>

struct _EocStatusbarPrivate
{
	CtkWidget *progressbar;
	CtkWidget *img_num_label;
};

G_DEFINE_TYPE_WITH_PRIVATE (EocStatusbar, eoc_statusbar, CTK_TYPE_STATUSBAR)

static void
eoc_statusbar_class_init (EocStatusbarClass *klass)
{
    /* empty */
}

static void
eoc_statusbar_init (EocStatusbar *statusbar)
{
	EocStatusbarPrivate *priv;
	CtkWidget *vbox;

	statusbar->priv = eoc_statusbar_get_instance_private (statusbar);
	priv = statusbar->priv;

	ctk_widget_set_margin_top (CTK_WIDGET (statusbar), 0);
	ctk_widget_set_margin_bottom (CTK_WIDGET (statusbar), 0);

	priv->img_num_label = ctk_label_new (NULL);
	ctk_widget_set_size_request (priv->img_num_label, 100, 10);
	ctk_widget_show (priv->img_num_label);

	ctk_box_pack_end (CTK_BOX (statusbar),
			  priv->img_num_label,
			  FALSE,
			  TRUE,
			  0);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	ctk_box_pack_end (CTK_BOX (statusbar),
			  vbox,
			  FALSE,
			  FALSE,
			  2);

	statusbar->priv->progressbar = ctk_progress_bar_new ();

	ctk_box_pack_end (CTK_BOX (vbox),
			  priv->progressbar,
			  TRUE,
			  TRUE,
			  2);

	ctk_widget_set_size_request (priv->progressbar, -1, 10);

	ctk_widget_show (vbox);

	ctk_widget_hide (statusbar->priv->progressbar);

}

CtkWidget *
eoc_statusbar_new (void)
{
	return CTK_WIDGET (g_object_new (EOC_TYPE_STATUSBAR, NULL));
}

void
eoc_statusbar_set_image_number (EocStatusbar *statusbar,
                                gint          num,
				gint          tot)
{
	gchar *msg;

	g_return_if_fail (EOC_IS_STATUSBAR (statusbar));

	/* Hide number display if values don't make sense */
	if (G_UNLIKELY (num <= 0 || tot <= 0))
		return;

	/* Translators: This string is displayed in the statusbar.
	 * The first token is the image number, the second is total image
	 * count.
	 *
	 * Translate to "%Id" if you want to use localized digits, or
	 * translate to "%d" otherwise.
	 *
	 * Note that translating this doesn't guarantee that you get localized
	 * digits. That needs support from your system and locale definition
	 * too.*/
	msg = g_strdup_printf (_("%d / %d"), num, tot);

	ctk_label_set_text (CTK_LABEL (statusbar->priv->img_num_label), msg);

      	g_free (msg);
}

void
eoc_statusbar_set_progress (EocStatusbar *statusbar,
			    gdouble       progress)
{
	g_return_if_fail (EOC_IS_STATUSBAR (statusbar));

	ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (statusbar->priv->progressbar),
				       progress);

	if (progress > 0 && progress < 1) {
		ctk_widget_show (statusbar->priv->progressbar);
		ctk_widget_hide (statusbar->priv->img_num_label);
	} else {
		ctk_widget_hide (statusbar->priv->progressbar);
		ctk_widget_show (statusbar->priv->img_num_label);
	}
}

