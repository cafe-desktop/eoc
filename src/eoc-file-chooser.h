/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _EOC_FILE_CHOOSER_H_
#define _EOC_FILE_CHOOSER_H_

#include <ctk/ctk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define EOC_TYPE_FILE_CHOOSER          (eoc_file_chooser_get_type ())
#define EOC_FILE_CHOOSER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOC_TYPE_FILE_CHOOSER, EocFileChooser))
#define EOC_FILE_CHOOSER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOC_TYPE_FILE_CHOOSER, EocFileChooserClass))

#define EOC_IS_FILE_CHOOSER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOC_TYPE_FILE_CHOOSER))
#define EOC_IS_FILE_CHOOSER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOC_TYPE_FILE_CHOOSER))
#define EOC_FILE_CHOOSER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOC_TYPE_FILE_CHOOSER, EocFileChooserClass))

typedef struct _EocFileChooser         EocFileChooser;
typedef struct _EocFileChooserClass    EocFileChooserClass;
typedef struct _EocFileChooserPrivate  EocFileChooserPrivate;

struct _EocFileChooser
{
	GtkFileChooserDialog  parent;

	EocFileChooserPrivate *priv;
};

struct _EocFileChooserClass
{
	GtkFileChooserDialogClass  parent_class;
};


GType		 eoc_file_chooser_get_type	(void) G_GNUC_CONST;

GtkWidget	*eoc_file_chooser_new		(GtkFileChooserAction action);

GdkPixbufFormat	*eoc_file_chooser_get_format	(EocFileChooser *chooser);


G_END_DECLS

#endif /* _EOC_FILE_CHOOSER_H_ */
