/* Eye Of Cafe - General Utilities
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
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

#ifndef __EOC_UTIL_H__
#define __EOC_UTIL_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
void     eoc_util_show_help                  (const gchar *section,
					      CtkWindow   *parent);

G_GNUC_INTERNAL
gchar   *eoc_util_make_valid_utf8            (const gchar *name);

G_GNUC_INTERNAL
GSList  *eoc_util_parse_uri_string_list_to_file_list (const gchar *uri_list);

G_GNUC_INTERNAL
GSList  *eoc_util_string_list_to_file_list    (GSList *string_list);

G_GNUC_INTERNAL
GSList  *eoc_util_strings_to_file_list        (gchar **strings);

G_GNUC_INTERNAL
GSList  *eoc_util_string_array_to_list       (const gchar **files,
	 				      gboolean create_uri);

G_GNUC_INTERNAL
gchar  **eoc_util_string_array_make_absolute (gchar **files);

G_GNUC_INTERNAL
const    gchar *eoc_util_dot_dir             (void);

G_GNUC_INTERNAL
char *  eoc_util_filename_get_extension      (const char * filename_with_extension);

G_GNUC_INTERNAL
gboolean eoc_util_file_is_persistent (GFile *file);

G_GNUC_INTERNAL
void     eoc_util_show_file_in_filemanager   (GFile *file,
                                              CtkWindow *toplevel);

G_END_DECLS

#endif /* __EOC_UTIL_H__ */
