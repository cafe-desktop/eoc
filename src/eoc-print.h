/* Eye of Cafe - Print Operations
 *
 * Copyright (C) 2005-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@gnome.org>
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

#ifndef __EOC_PRINT_H__
#define __EOC_PRINT_H__

#include "eoc-image.h"
#include <ctk/ctk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
CtkPrintOperation*    eoc_print_operation_new (EocImage *image,
					       CtkPrintSettings *print_settings,
					       CtkPageSetup *page_setup);

G_GNUC_INTERNAL
CtkPageSetup*         eoc_print_get_page_setup (void);

G_GNUC_INTERNAL
void                  eoc_print_set_page_setup (CtkPageSetup *page_setup);

G_GNUC_INTERNAL
CtkPrintSettings *    eoc_print_get_print_settings (void);

G_GNUC_INTERNAL
void                  eoc_print_set_print_settings (CtkPrintSettings *print_settings);

G_END_DECLS

#endif /* __EOC_PRINT_H__ */
