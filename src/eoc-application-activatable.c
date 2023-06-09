/*
 * eoc-application-activatable.c
 * This file is part of eoc
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2012 Felix Riemann
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eoc-application-activatable.h"

#include <glib-object.h>
#include "eoc-application.h"

G_DEFINE_INTERFACE(EocApplicationActivatable, eoc_application_activatable, G_TYPE_OBJECT)

void
eoc_application_activatable_default_init (EocApplicationActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
         * EocApplicationActivatable:app:
		 *
         * This is the #EocApplication this #EocApplicationActivatable instance
		 * should be attached to.
		 */
		g_object_interface_install_property (iface,
                g_param_spec_object ("app", "Application",
                             "The EocApplication this instance it attached to",
                             EOC_TYPE_APPLICATION,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_STATIC_STRINGS));
		initialized = TRUE;
	}
}

void
eoc_application_activatable_activate (EocApplicationActivatable *activatable)
{
    EocApplicationActivatableInterface *iface;

    g_return_if_fail (EOC_IS_APPLICATION_ACTIVATABLE (activatable));

    iface = EOC_APPLICATION_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->activate != NULL))
		iface->activate (activatable);
}

void
eoc_application_activatable_deactivate (EocApplicationActivatable *activatable)
{
    EocApplicationActivatableInterface *iface;

    g_return_if_fail (EOC_IS_APPLICATION_ACTIVATABLE (activatable));

    iface = EOC_APPLICATION_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->deactivate != NULL))
		iface->deactivate (activatable);
}

