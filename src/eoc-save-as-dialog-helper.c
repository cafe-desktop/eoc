#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctk/ctk.h>
#include "eoc-save-as-dialog-helper.h"
#include "eoc-pixbuf-util.h"
#include "eoc-file-chooser.h"

typedef struct {
	CtkWidget *dir_chooser;
	CtkWidget *token_entry;
	CtkWidget *replace_spaces_check;
	CtkWidget *counter_spin;
	CtkWidget *preview_label;
	CtkWidget *format_combobox;

	guint      idle_id;
	gint       n_images;
	EocImage  *image;
	gint       nth_image;
} SaveAsData;

static GdkPixbufFormat *
get_selected_format (CtkComboBox *combobox)
{
	GdkPixbufFormat *format;
	CtkTreeModel *store;
	CtkTreeIter iter;

	ctk_combo_box_get_active_iter (combobox, &iter);
	store = ctk_combo_box_get_model (combobox);
	ctk_tree_model_get (store, &iter, 1, &format, -1);

	return format;
}

static gboolean
update_preview (gpointer user_data)
{
	SaveAsData *data;
	char *preview_str = NULL;
	const char *token_str;
	gboolean convert_spaces;
	gulong   counter_start;
	GdkPixbufFormat *format;

	data = g_object_get_data (G_OBJECT (user_data), "data");
	g_assert (data != NULL);

	if (data->image == NULL) return FALSE;

	/* obtain required dialog data */
	token_str = ctk_entry_get_text (GTK_ENTRY (data->token_entry));
	convert_spaces = ctk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (data->replace_spaces_check));
	counter_start = ctk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data->counter_spin));

	format = get_selected_format (GTK_COMBO_BOX (data->format_combobox));

	if (token_str != NULL) {
		/* generate preview filename */
		preview_str = eoc_uri_converter_preview (token_str, data->image, format,
							 (counter_start + data->nth_image),
							 data->n_images,
							 convert_spaces, '_' /* FIXME: make this editable */);
	}

	ctk_label_set_text (GTK_LABEL (data->preview_label), preview_str);

	g_free (preview_str);

	data->idle_id = 0;

	return FALSE;
}

static void
request_preview_update (CtkWidget *dlg)
{
	SaveAsData *data;

	data = g_object_get_data (G_OBJECT (dlg), "data");
	g_assert (data != NULL);

	if (data->idle_id != 0)
		return;

	data->idle_id = g_idle_add (update_preview, dlg);
}

static void
on_format_combobox_changed (CtkComboBox *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
on_token_entry_changed (CtkWidget *widget, gpointer user_data)
{
	SaveAsData *data;
	gboolean enable_save;

	data = g_object_get_data (G_OBJECT (user_data), "data");
	g_assert (data != NULL);

	request_preview_update (GTK_WIDGET (user_data));

	enable_save = (strlen (ctk_entry_get_text (GTK_ENTRY (data->token_entry))) > 0);
	ctk_dialog_set_response_sensitive (GTK_DIALOG (user_data), GTK_RESPONSE_OK,
					   enable_save);
}

static void
on_replace_spaces_check_clicked (CtkWidget *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
on_counter_spin_changed (CtkWidget *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
prepare_format_combobox (SaveAsData *data)
{
	CtkComboBox *combobox;
	CtkCellRenderer *cell;
	GSList *formats;
	CtkListStore *store;
	GSList *it;
	CtkTreeIter iter;

	combobox = GTK_COMBO_BOX (data->format_combobox);

	store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	ctk_combo_box_set_model (combobox, GTK_TREE_MODEL (store));

	cell = ctk_cell_renderer_text_new ();
	ctk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), cell, TRUE);
	ctk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combobox), cell,
				 	"text", 0);

	formats = eoc_pixbuf_get_savable_formats ();
	for (it = formats; it != NULL; it = it->next) {
		GdkPixbufFormat *f;

		f = (GdkPixbufFormat*) it->data;

		ctk_list_store_append (store, &iter);
		ctk_list_store_set (store, &iter, 0, gdk_pixbuf_format_get_name (f), 1, f, -1);
	}
	g_slist_free (formats);

	ctk_list_store_append (store, &iter);
	ctk_list_store_set (store, &iter, 0, _("as is"), 1, NULL, -1);
	ctk_combo_box_set_active_iter (combobox, &iter);
	ctk_widget_show_all (GTK_WIDGET (combobox));
}

static void
destroy_data_cb (gpointer data)
{
	SaveAsData *sd;

	sd = (SaveAsData*) data;

	if (sd->image != NULL)
		g_object_unref (sd->image);

	if (sd->idle_id != 0)
		g_source_remove (sd->idle_id);

	g_slice_free (SaveAsData, sd);
}

static void
set_default_values (CtkWidget *dlg, GFile *base_file)
{
	SaveAsData *sd;

	sd = (SaveAsData*) g_object_get_data (G_OBJECT (dlg), "data");

	ctk_spin_button_set_value (GTK_SPIN_BUTTON (sd->counter_spin), 0.0);
	ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sd->replace_spaces_check),
				      FALSE);
	if (base_file != NULL) {
		ctk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (sd->dir_chooser), base_file, NULL);
	}

	/*ctk_dialog_set_response_sensitive (GTK_DIALOG (dlg), GTK_RESPONSE_OK, FALSE);*/

	request_preview_update (dlg);
}

CtkWidget*
eoc_save_as_dialog_new (CtkWindow *main, GList *images, GFile *base_file)
{
	CtkBuilder  *xml;
	CtkWidget *dlg;
	SaveAsData *data;
	CtkWidget *label;

	xml = ctk_builder_new_from_resource ("/org/gnome/eog/ui/eoc-multiple-save-as-dialog.ui");
	ctk_builder_set_translation_domain (xml, GETTEXT_PACKAGE);

	dlg = GTK_WIDGET (g_object_ref (ctk_builder_get_object (xml, "eoc_multiple_save_as_dialog")));
	ctk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (main));
	ctk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER_ON_PARENT);

	data = g_slice_new0 (SaveAsData);
	/* init widget references */
	data->dir_chooser = GTK_WIDGET (ctk_builder_get_object (xml,
								"dir_chooser"));
	data->token_entry = GTK_WIDGET (ctk_builder_get_object (xml,
								"token_entry"));
	data->replace_spaces_check = GTK_WIDGET (ctk_builder_get_object (xml,
						       "replace_spaces_check"));
	data->counter_spin = GTK_WIDGET (ctk_builder_get_object (xml,
							       "counter_spin"));
	data->preview_label = GTK_WIDGET (ctk_builder_get_object (xml,
							      "preview_label"));
	data->format_combobox = GTK_WIDGET (ctk_builder_get_object (xml,
							    "format_combobox"));

	/* init preview information */
	data->idle_id = 0;
	data->n_images = g_list_length (images);
	data->nth_image = (int) ((float) data->n_images * rand() / (float) (RAND_MAX+1.0));
	g_assert (data->nth_image >= 0 && data->nth_image < data->n_images);
	data->image = g_object_ref (EOC_IMAGE (g_list_nth_data (images, data->nth_image)));
	g_object_set_data_full (G_OBJECT (dlg), "data", data, destroy_data_cb);

	g_signal_connect (G_OBJECT (data->format_combobox), "changed",
			  (GCallback) on_format_combobox_changed, dlg);

	g_signal_connect (G_OBJECT (data->token_entry), "changed",
			  (GCallback) on_token_entry_changed, dlg);

	g_signal_connect (G_OBJECT (data->replace_spaces_check), "toggled",
			  (GCallback) on_replace_spaces_check_clicked, dlg);

	g_signal_connect (G_OBJECT (data->counter_spin), "changed",
			  (GCallback) on_counter_spin_changed, dlg);

	label = GTK_WIDGET (ctk_builder_get_object (xml, "preview_label_from"));
	ctk_label_set_text (GTK_LABEL (label), eoc_image_get_caption (data->image));

	prepare_format_combobox (data);

	set_default_values (dlg, base_file);
	g_object_unref (xml);
	return dlg;
}

EocURIConverter*
eoc_save_as_dialog_get_converter (CtkWidget *dlg)
{
	EocURIConverter *conv;

	SaveAsData *data;
	const char *format_str;
	gboolean convert_spaces;
	gulong   counter_start;
	GdkPixbufFormat *format;
	GFile *base_file;

	data = g_object_get_data (G_OBJECT (dlg), "data");
	g_assert (data != NULL);

	/* obtain required dialog data */
	format_str = ctk_entry_get_text (GTK_ENTRY (data->token_entry));

	convert_spaces = ctk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (data->replace_spaces_check));

	counter_start = ctk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data->counter_spin));

	format = get_selected_format (GTK_COMBO_BOX (data->format_combobox));

	base_file = ctk_file_chooser_get_file (GTK_FILE_CHOOSER (data->dir_chooser));

	/* create converter object */
	conv = eoc_uri_converter_new (base_file, format, format_str);

	/* set other properties */
	g_object_set (G_OBJECT (conv),
		      "convert-spaces", convert_spaces,
		      "space-character", '_',
		      "counter-start", counter_start,
		      "n-images", data->n_images,
		      NULL);

	g_object_unref (base_file);

	return conv;
}
