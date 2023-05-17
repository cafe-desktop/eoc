/*
 * eoc-window-activatable.h
 * This file is part of eoc
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2011 Felix Riemann
 *
 * Base on code by:
 * 	- Steve Frécinaux <code@istique.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __EOC_WINDOW_ACTIVATABLE_H__
#define __EOC_WINDOW_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EOC_TYPE_WINDOW_ACTIVATABLE	(eoc_window_activatable_get_type ())
#define EOC_WINDOW_ACTIVATABLE(obj) 	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 EOC_TYPE_WINDOW_ACTIVATABLE, \
					 EocWindowActivatable))
#define EOC_WINDOW_ACTIVATABLE_IFACE(obj) \
					(G_TYPE_CHECK_CLASS_CAST ((obj), \
					 EOC_TYPE_WINDOW_ACTIVATABLE, \
					 EocWindowActivatableInterface))
#define EOC_IS_WINDOW_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 EOC_TYPE_WINDOW_ACTIVATABLE))
#define EOC_WINDOW_ACTIVATABLE_GET_IFACE(obj) \
					(G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
					 EOC_TYPE_WINDOW_ACTIVATABLE, \
					 EocWindowActivatableInterface))

typedef struct _EocWindowActivatable		EocWindowActivatable;
typedef struct _EocWindowActivatableInterface	EocWindowActivatableInterface;

struct _EocWindowActivatableInterface
{
	GTypeInterface g_iface;

	/* vfuncs */

	void	(*activate)	(EocWindowActivatable *activatable);
	void	(*deactivate)	(EocWindowActivatable *activatable);
};

GType	eoc_window_activatable_get_type	(void) G_GNUC_CONST;

void	eoc_window_activatable_activate	    (EocWindowActivatable *activatable);
void	eoc_window_activatable_deactivate   (EocWindowActivatable *activatable);

G_END_DECLS
#endif /* __EOC_WINDOW_ACTIVATABLE_H__ */

