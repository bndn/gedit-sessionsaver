/*
 * gedit-show-tabbar-plugin.h
 *
 * Copyright (C) 2006 Steve Frécinaux
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifndef __GEDIT_SHOW_TABBAR_PLUGIN_H__
#define __GEDIT_SHOW_TABBAR_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gedit/gedit-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_SHOW_TABBAR_PLUGIN		(gedit_show_tabbar_plugin_get_type ())
#define GEDIT_SHOW_TABBAR_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_SHOW_TABBAR_PLUGIN, GeditShowTabbarPlugin))
#define GEDIT_SHOW_TABBAR_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_SHOW_TABBAR_PLUGIN, GeditShowTabbarPluginClass))
#define GEDIT_IS_SHOW_TABBAR_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_SHOW_TABBAR_PLUGIN))
#define GEDIT_IS_SHOW_TABBAR_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_SHOW_TABBAR_PLUGIN))
#define GEDIT_SHOW_TABBAR_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_SHOW_TABBAR_PLUGIN, GeditShowTabbarPluginClass))

/* Private structure type */
typedef struct _GeditShowTabbarPluginPrivate	GeditShowTabbarPluginPrivate;

/*
 * Main object structure
 */
typedef struct _GeditShowTabbarPlugin		GeditShowTabbarPlugin;

struct _GeditShowTabbarPlugin
{
	GeditPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _GeditShowTabbarPluginClass	GeditShowTabbarPluginClass;

struct _GeditShowTabbarPluginClass
{
	GeditPluginClass parent_class;
};

/*
 * Public methods
 */
GType	gedit_show_tabbar_plugin_get_type	(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_gedit_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __GEDIT_SHOW_TABBAR_PLUGIN_H__ */
