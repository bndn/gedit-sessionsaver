/*
 * Copyright (C) 2008 Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id: gedit-drawspaces-plugin.h 137 2006-04-23 15:13:27Z sfre $
 */

#ifndef __GEDIT_DRAWSPACES_PLUGIN_H__
#define __GEDIT_DRAWSPACES_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_DRAWSPACES_PLUGIN		(gedit_drawspaces_plugin_get_type ())
#define GEDIT_DRAWSPACES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_DRAWSPACES_PLUGIN, GeditDrawspacesPlugin))
#define GEDIT_DRAWSPACES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_DRAWSPACES_PLUGIN, GeditDrawspacesPluginClass))
#define GEDIT_IS_DRAWSPACES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_DRAWSPACES_PLUGIN))
#define GEDIT_IS_DRAWSPACES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_DRAWSPACES_PLUGIN))
#define GEDIT_DRAWSPACES_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_DRAWSPACES_PLUGIN, GeditDrawspacesPluginClass))

typedef struct _GeditDrawspacesPlugin		GeditDrawspacesPlugin;
typedef struct _GeditDrawspacesPluginPrivate	GeditDrawspacesPluginPrivate;
typedef struct _GeditDrawspacesPluginClass	GeditDrawspacesPluginClass;

struct _GeditDrawspacesPlugin
{
	PeasExtensionBase parent_instance;

	/* private */
	GeditDrawspacesPluginPrivate *priv;
};

struct _GeditDrawspacesPluginClass
{
	PeasExtensionBaseClass parent_class;
};

GType			gedit_drawspaces_plugin_get_type	(void) G_GNUC_CONST;

G_MODULE_EXPORT void	peas_register_types			(PeasObjectModule *module);

G_END_DECLS

#endif /* __GEDIT_DRAWSPACES_PLUGIN_H__ */
