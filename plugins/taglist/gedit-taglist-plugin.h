/*
 * gedit-taglist-plugin.h
 * 
 * Copyright (C) 2002-2005 - Paolo Maggi
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 */
 
#ifndef __GEDIT_TAGLIST_PLUGIN_H__
#define __GEDIT_TAGLIST_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_TAGLIST_PLUGIN		(gedit_taglist_plugin_get_type ())
#define GEDIT_TAGLIST_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_TAGLIST_PLUGIN, GeditTaglistPlugin))
#define GEDIT_TAGLIST_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_TAGLIST_PLUGIN, GeditTaglistPluginClass))
#define GEDIT_IS_TAGLIST_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_TAGLIST_PLUGIN))
#define GEDIT_IS_TAGLIST_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_TAGLIST_PLUGIN))
#define GEDIT_TAGLIST_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_TAGLIST_PLUGIN, GeditTaglistPluginClass))

typedef struct _GeditTaglistPlugin		GeditTaglistPlugin;
typedef struct _GeditTaglistPluginPrivate	GeditTaglistPluginPrivate;
typedef struct _GeditTaglistPluginClass		GeditTaglistPluginClass;

struct _GeditTaglistPlugin
{
	PeasExtensionBase parent_instance;

	/*< private >*/
	GeditTaglistPluginPrivate *priv;
};

struct _GeditTaglistPluginClass
{
	PeasExtensionBaseClass parent_class;
};

GType			gedit_taglist_plugin_get_type	(void) G_GNUC_CONST;

G_MODULE_EXPORT void	peas_register_types		(PeasObjectModule *module);

G_END_DECLS

#endif /* __GEDIT_TAGLIST_PLUGIN_H__ */

/* ex:ts=8:noet: */
