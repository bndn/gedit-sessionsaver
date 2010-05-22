/*
 * gedit-show-tabbar-plugin.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-show-tabbar-plugin.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>


#define WINDOW_DATA_KEY		"GeditShowTabbarWindowData"
#define MENU_PATH 		"/MenuBar/ViewMenu"
#define GSETTINGS_BASE_KEY	"org.gnome.gedit.plugins.showtabbar"
#define GSETTINGS_KEY_TABBAR	"tabbar-visible"

#define GEDIT_SHOW_TABBAR_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SHOW_TABBAR_PLUGIN, GeditShowTabbarPluginPrivate))

GEDIT_PLUGIN_REGISTER_TYPE (GeditShowTabbarPlugin, gedit_show_tabbar_plugin)

struct _GeditShowTabbarPluginPrivate
{
	GSettings *settings;
};

typedef struct
{
	GeditShowTabbarPlugin *plugin;
	GtkActionGroup        *action_group;
	guint                  ui_id;
	gulong		       signal_handler_id;
} WindowData;

static void
gedit_show_tabbar_plugin_init (GeditShowTabbarPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditShowTabbarPlugin initializing");

	plugin->priv = GEDIT_SHOW_TABBAR_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->settings = g_settings_new (GSETTINGS_BASE_KEY);
}

static void
gedit_show_tabbar_plugin_dispose (GObject *object)
{
	GeditShowTabbarPlugin *plugin = GEDIT_SHOW_TABBAR_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditShowTabbarPlugin disposing");

	if (plugin->priv->settings != NULL)
	{
		g_object_unref (plugin->priv->settings);
		plugin->priv->settings = NULL;
	}

	G_OBJECT_CLASS (gedit_show_tabbar_plugin_parent_class)->dispose (object);
}

static GtkNotebook *
get_notebook (GeditWindow *window)
{
	GList *list;
	GtkContainer *container;
	GtkNotebook *notebook;

	g_return_val_if_fail (window != NULL, NULL);

	container = GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window)));
								/* VBox   */

	list = gtk_container_get_children (container);
	container = GTK_CONTAINER (g_list_nth_data (list, 2));	/* HPaned */
	g_list_free (list);

	list = gtk_container_get_children (container);
	container = GTK_CONTAINER (g_list_nth_data (list, 1));	/* VPaned */
	g_list_free (list);

	list = gtk_container_get_children (container);
	notebook = GTK_NOTEBOOK (g_list_nth_data (list, 0));	/* Notebook */
	g_list_free (list);

	return notebook;
}

static void
on_notebook_show_tabs_changed (GtkNotebook	*notebook,
			       GParamSpec	*pspec,
			       WindowData	*data)
{
	GtkAction *action;
	gboolean visible;

#if 0
	/* this works quite bad due to update_tabs_visibility in
	   gedit-notebook.c */
	visible = gtk_notebook_get_show_tabs (notebook);

	if (gtk_toggle_action_get_active (action) != visible)
		gtk_toggle_action_set_active (action, visible);
#endif

	action = gtk_action_group_get_action (data->action_group,
					      "ShowTabbar");
	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	/* this is intendend to avoid the GeditNotebook to show the tabs again
	   (it does so everytime a new tab is added) */
	if (visible != gtk_notebook_get_show_tabs (notebook))
		gtk_notebook_set_show_tabs (notebook, visible);

	g_settings_set_boolean (data->plugin->priv->settings,
				GSETTINGS_KEY_TABBAR,
				visible);
}

static void
on_view_tabbar_toggled (GtkToggleAction	*action,
			GeditWindow	*window)
{
	gtk_notebook_set_show_tabs (get_notebook (window),
				    gtk_toggle_action_get_active (action));
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);
	g_slice_free (WindowData, data);
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;
	GtkToggleAction *action;
	GtkNotebook *notebook;
	gboolean visible;

	gedit_debug (DEBUG_PLUGINS);

	data = g_slice_new (WindowData);
	data->plugin = GEDIT_SHOW_TABBAR_PLUGIN (plugin);

	visible = g_settings_get_boolean (data->plugin->priv->settings,
					  GSETTINGS_KEY_TABBAR);

	notebook = get_notebook (window);

	gtk_notebook_set_show_tabs (notebook, visible);

	manager = gedit_window_get_ui_manager (window);

	data->action_group = gtk_action_group_new ("GeditHideTabbarPluginActions");
	gtk_action_group_set_translation_domain (data->action_group,
						 GETTEXT_PACKAGE);

	action = gtk_toggle_action_new ("ShowTabbar",
					_("Tab_bar"),
					_("Show or hide the tabbar in the current window"),
					NULL);

	gtk_toggle_action_set_active (action, visible);

	g_signal_connect (action,
			  "toggled",
			  G_CALLBACK (on_view_tabbar_toggled),
			  window);

	gtk_action_group_add_action (data->action_group, GTK_ACTION (action));

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_new_merge_id (manager);

	gtk_ui_manager_add_ui (manager,
			       data->ui_id,
			       MENU_PATH,
			       "ShowTabbar",
			       "ShowTabbar",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	data->signal_handler_id =
		g_signal_connect (notebook,
				  "notify::show-tabs",
				  G_CALLBACK (on_notebook_show_tabs_changed),
				  data);

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_signal_handler_disconnect (get_notebook (window),
				     data->signal_handler_id);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
gedit_show_tabbar_plugin_class_init (GeditShowTabbarPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (GeditShowTabbarPluginPrivate));

	object_class->dispose = gedit_show_tabbar_plugin_dispose;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}
