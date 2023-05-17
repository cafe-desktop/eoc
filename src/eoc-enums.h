/* Eye of MATE - General enumerations.
 *
 * Copyright (C) 2007-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#ifndef __EOC_ENUMS__
#define __EOC_ENUMS__

typedef enum {
	EOC_IMAGE_DATA_IMAGE     = 1 << 0,
	EOC_IMAGE_DATA_DIMENSION = 1 << 1,
	EOC_IMAGE_DATA_EXIF      = 1 << 2,
	EOC_IMAGE_DATA_XMP       = 1 << 3
} EocImageData;

#define EOC_IMAGE_DATA_ALL  (EOC_IMAGE_DATA_IMAGE |     \
			     EOC_IMAGE_DATA_DIMENSION | \
			     EOC_IMAGE_DATA_EXIF |      \
			     EOC_IMAGE_DATA_XMP)

#endif
