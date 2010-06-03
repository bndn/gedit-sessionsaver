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
#include <gtk/gtk.h>
#include <gedit/gedit-debug.h>


#define WINDOW_DATA_KEY		"GeditShowTabberWindowData"
#define GSETTINGS_UI_SCHEMA	"org.gnome.gedit.preferences.ui"
#define GSETTINGS_TABBAR_SCHEMA	"org.gnome.gedit.plugins.showtabbar"
#define GSETTINGS_UI_KEY	"notebook-show-tabs-mode"
#define GSETTINGS_TABBAR_KEY	"show-tabs-mode"

#define GEDIT_SHOW_TABBAR_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SHOW_TABBAR_PLUGIN, GeditShowTabbarPluginPrivate))

GEDIT_PLUGIN_REGISTER_TYPE (GeditShowTabbarPlugin, gedit_show_tabbar_plugin)

typedef enum {
	SHOW_TABS_NEVER,
	SHOW_TABS_AUTO,
	SHOW_TABS_ALWAYS
} ShowTabsModeType;

struct _GeditShowTabbarPluginPrivate
{
	GSettings *ui_settings;
	GSettings *tabbar_settings;
};

typedef struct
{
	GeditShowTabbarPlugin *plugin;
	guint                  ui_id;
	ShowTabsModeType       show_tabs_mode;
	GtkActionGroup        *action_group;
	GtkRadioAction        *bound_action;
} WindowData;


static void	 gedit_show_tabbar_plugin_deactivate	(GeditPlugin *plugin,
							 GeditWindow *window);
static void	 gedit_show_tabbar_plugin_activate	(GeditPlugin *plugin,
							 GeditWindow *window);

const gchar *show_tabbar_mode_ui = " \
<ui> \
  <menubar name='MenuBar'> \
    <menu name='ViewMenu' action='View'> \
        <separator/> \
        <menu action='ShowTabbar'> \
          <menuitem action='ShowTabbarNever'/> \
          <menuitem action='ShowTabbarAuto'/> \
          <menuitem action='ShowTabbarAlways'/> \
        </menu> \
    </menu> \
  </menubar> \
</ui>";

static const GtkActionEntry menu_action_entries[] =
{
	{ "ShowTabbar", NULL, N_("Show Ta_bbar Mode") },
};

static const GtkRadioActionEntry radio_action_entries[] =
{
	{ "ShowTabbarNever", NULL, N_("_Never Show Tabbar"), NULL,
	  N_("_Never Show the Tabbar"), SHOW_TABS_NEVER },

	{ "ShowTabbarAuto", NULL, N_("A_utomatically Show Tabbar"), NULL,
	  N_("Show the tabbar when there is more than one tab"), SHOW_TABS_AUTO },

	{ "ShowTabbarAlways", NULL, N_("_Always Show Tabbar"), NULL,
	  N_("Always show the tabbar"), SHOW_TABS_ALWAYS }
};

static void
gedit_show_tabbar_plugin_dispose (GObject *object)
{
	GeditShowTabbarPlugin *plugin = GEDIT_SHOW_TABBAR_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditShowTabbarPlugin disposing");

	if (plugin->priv->tabbar_settings != NULL)
	{
		g_object_unref (plugin->priv->tabbar_settings);
		plugin->priv->tabbar_settings = NULL;
	}

	if (plugin->priv->ui_settings != NULL)
	{
		g_object_unref (plugin->priv->ui_settings);
		plugin->priv->ui_settings = NULL;
	}

	G_OBJECT_CLASS (gedit_show_tabbar_plugin_parent_class)->dispose (object);
}

static void
gedit_show_tabbar_plugin_class_init (GeditShowTabbarPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->dispose = gedit_show_tabbar_plugin_dispose;

	plugin_class->activate = gedit_show_tabbar_plugin_activate;
	plugin_class->deactivate = gedit_show_tabbar_plugin_deactivate;

	g_type_class_add_private (object_class, sizeof (GeditShowTabbarPluginPrivate));
}

static void
gedit_show_tabbar_plugin_init (GeditShowTabbarPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditShowTabbarPlugin initializing");

	plugin->priv = GEDIT_SHOW_TABBAR_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->ui_settings = g_settings_new (GSETTINGS_UI_SCHEMA);
	plugin->priv->tabbar_settings = g_settings_new (GSETTINGS_TABBAR_SCHEMA);
}

static ShowTabsModeType
string_to_show_tabs_mode (const gchar *str)
{
	ShowTabsModeType show_tabs_mode;

	if (g_strcmp0 (str, "NEVER") == 0)
	{
		show_tabs_mode = SHOW_TABS_NEVER;
	}
	else if (g_strcmp0 (str, "AUTO") == 0)
	{
		show_tabs_mode = SHOW_TABS_AUTO;
	}
	else /* ALWAYS */
	{
		show_tabs_mode = SHOW_TABS_ALWAYS;
	}

	return show_tabs_mode;
}

static gchar *
show_tabs_mode_to_string (ShowTabsModeType show_tabs_mode)
{
	const gchar *value;

	switch (show_tabs_mode)
	{
		case SHOW_TABS_NEVER:
			value = "NEVER";
			break;

		case SHOW_TABS_AUTO:
			value = "AUTO";
			break;

		default: /* SHOW_TABS_ALWAYS */
			value = "ALWAYS";
			break;
	}

	return g_strdup (value);
}

static GVariant *
show_tabs_mode_set_mapping (const GValue       *value,
			    const GVariantType *expected_type,
			    gpointer            user_data G_GNUC_UNUSED)
{
	return g_variant_new_string (show_tabs_mode_to_string (g_value_get_int (value)));
}

static gboolean
show_tabs_mode_get_mapping (GValue   *value,
			    GVariant *variant,
			    gpointer  user_data G_GNUC_UNUSED)
{
	const gchar *str;
	ShowTabsModeType show_tabs_mode;

	str = g_variant_get_string (variant, NULL);

	show_tabs_mode = string_to_show_tabs_mode (str);

	g_value_set_int (value, show_tabs_mode);

	return TRUE;
}

static void
on_action_changed (GtkRadioAction *action,
		   GtkRadioAction *current,
		   WindowData     *data)
{
	ShowTabsModeType show_tabs_mode;

	gedit_debug (DEBUG_PLUGINS);

	show_tabs_mode = gtk_radio_action_get_current_value (current);

	/* prevent loop */
	if (data->show_tabs_mode != show_tabs_mode)
	{
		gchar *string;

		string = show_tabs_mode_to_string (show_tabs_mode);

		g_settings_set_string (data->plugin->priv->ui_settings,
				       GSETTINGS_UI_KEY,
				       string);

		g_free (string);
	}
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);

	g_slice_free (WindowData, data);
}

static void
gedit_show_tabbar_plugin_activate (GeditPlugin *plugin,
				   GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;
	gchar *str;
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS);

	data = g_slice_new (WindowData);
	data->plugin = GEDIT_SHOW_TABBAR_PLUGIN (plugin);

	manager = gedit_window_get_ui_manager (window);


	data->action_group = gtk_action_group_new ("GeditShowTabbarPluginActions");
	gtk_action_group_set_translation_domain (data->action_group,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (data->action_group,
				      menu_action_entries,
				      G_N_ELEMENTS (menu_action_entries),
				      data);

	gtk_action_group_add_radio_actions (data->action_group,
					    radio_action_entries,
					    G_N_ELEMENTS (radio_action_entries),
					    SHOW_TABS_ALWAYS,
					    G_CALLBACK (on_action_changed),
					    data);

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_add_ui_from_string (manager,
							 show_tabbar_mode_ui,
							 -1,
							 &error);
	if (error != NULL)
	{
		g_warning ("Could not merge ShowTabber UI: %s", error->message);
		g_error_free (error);
	}

	gtk_ui_manager_ensure_update (manager);

	data->bound_action = GTK_RADIO_ACTION (gtk_action_group_get_action (data->action_group,
									    "ShowTabbarNever"));

	g_settings_bind_with_mapping (data->plugin->priv->ui_settings,
				      GSETTINGS_UI_KEY,
				      data->bound_action,
				      "current-value",
				      G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET,
				      show_tabs_mode_get_mapping,
				      show_tabs_mode_set_mapping,
				      NULL, NULL);

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);

	/* Reset the saved setting */
	str = g_settings_get_string (data->plugin->priv->tabbar_settings,
				      GSETTINGS_TABBAR_KEY);
	gtk_radio_action_set_current_value (data->bound_action,
					    string_to_show_tabs_mode (str));
	g_free (str);
}

static void
gedit_show_tabbar_plugin_deactivate (GeditPlugin *plugin,
				     GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;
	gchar *str;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	/* Save the setting */
	str = show_tabs_mode_to_string (gtk_radio_action_get_current_value (data->bound_action));
	g_settings_set_string (data->plugin->priv->tabbar_settings,
			       GSETTINGS_TABBAR_KEY,
			       str);
	g_free (str);

	/* Reset the gedit setting */
	g_settings_set_string (data->plugin->priv->ui_settings,
			       GSETTINGS_UI_KEY,
			       "ALWAYS"); 

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}
