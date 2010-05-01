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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-drawspaces-plugin.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-utils.h>

#include <gconf/gconf-client.h>

#define GCONF_KEY_BASE "/apps/gedit-2/plugins/drawspaces"
#define GCONF_KEY_ENABLE        GCONF_KEY_BASE "/enable"
#define GCONF_KEY_DRAW_TABS     GCONF_KEY_BASE "/draw_tabs"
#define GCONF_KEY_DRAW_SPACES   GCONF_KEY_BASE "/draw_spaces"
#define GCONF_KEY_DRAW_NEWLINE  GCONF_KEY_BASE "/draw_newline"
#define GCONF_KEY_DRAW_NBSP     GCONF_KEY_BASE "/draw_nbsp"
#define GCONF_KEY_DRAW_LEADING  GCONF_KEY_BASE "/draw_leading"
#define GCONF_KEY_DRAW_TEXT     GCONF_KEY_BASE "/draw_text"
#define GCONF_KEY_DRAW_TRAILING GCONF_KEY_BASE "/draw_trailing"

#define UI_FILE "drawspaces.ui"

#define WINDOW_DATA_KEY "GeditDrawspacesPluginWindowData"

#define GEDIT_DRAWSPACES_PLUGIN_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_DRAWSPACES_PLUGIN,		\
				GeditDrawspacesPluginPrivate))


GEDIT_PLUGIN_REGISTER_TYPE (GeditDrawspacesPlugin, gedit_drawspaces_plugin)

struct _GeditDrawspacesPluginPrivate
{
	GConfClient *gconf_client;
	guint connection_id;

	GtkSourceDrawSpacesFlags flags;
};

typedef struct
{
	GtkActionGroup *action_group;
	guint           ui_id;

	gboolean enable;
} WindowData;

typedef struct
{
	GeditWindow *window;
	GeditDrawspacesPlugin *plugin;
} ActionData;

typedef struct _DrawspacesConfigureDialog DrawspacesConfigureDialog;

struct _DrawspacesConfigureDialog
{
	GtkWidget *dialog;

	GtkWidget *draw_tabs;
	GtkWidget *draw_spaces;
	GtkWidget *draw_newline;
	GtkWidget *draw_nbsp;
	GtkWidget *draw_leading;
	GtkWidget *draw_text;
	GtkWidget *draw_trailing;
};

enum
{
	COLUMN_LABEL,
	COLUMN_LOCATION
};

static const gchar submenu [] = {
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='ViewMenu' action='View'>"
"      <separator />"
"      <menuitem name='DrawSpacesMenu' action='DrawSpaces'/>"
"    </menu>"
"  </menubar>"
"</ui>"
};

static void draw_spaces_in_window (GeditWindow *window, GeditDrawspacesPlugin *plugin);

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_slice_free (WindowData, data);
}

static void
free_action_data (gpointer data)
{
	g_slice_free (ActionData, data);
}

static void
set_draw_gconf (GeditDrawspacesPlugin *plugin,
                const gchar           *key,
                gboolean               value)
{
	GError *error = NULL;

	gconf_client_set_bool (plugin->priv->gconf_client,
			       key,
			       value,
			       &error);

	if (error != NULL)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}
}

static void
on_active_toggled (GtkToggleAction *action,
		   ActionData *action_data)
{
	WindowData *data;
	gboolean value;

	data = (WindowData *) g_object_get_data (G_OBJECT (action_data->window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	value = gtk_toggle_action_get_active (action);
	data->enable = value;

	set_draw_gconf (action_data->plugin, GCONF_KEY_ENABLE, value);

	draw_spaces_in_window (action_data->window, action_data->plugin);
}

static const GtkToggleActionEntry action_entries[] =
{
	{ "DrawSpaces", NULL, N_("Show _White Space"), NULL,
	 N_("Show spaces and tabs"),
	 G_CALLBACK (on_active_toggled)},
};

static void on_gconf_notify (GConfClient *client,
			     guint cnxn_id,
			     GConfEntry *entry,
			     gpointer user_data);

static void
gedit_drawspaces_plugin_init (GeditDrawspacesPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesPlugin initializing");

	plugin->priv = GEDIT_DRAWSPACES_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->gconf_client = gconf_client_get_default ();

	gconf_client_add_dir (plugin->priv->gconf_client,
			      GCONF_KEY_BASE,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	plugin->priv->connection_id = gconf_client_notify_add (plugin->priv->gconf_client,
							       GCONF_KEY_BASE,
							       on_gconf_notify,
							       plugin, NULL, NULL);
}

static void
gedit_drawspaces_plugin_dispose (GObject *object)
{
	GeditDrawspacesPlugin *plugin = GEDIT_DRAWSPACES_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesPlugin disposing");

	if (plugin->priv->connection_id != 0)
	{
		gconf_client_notify_remove (plugin->priv->gconf_client,
					    plugin->priv->connection_id);

		plugin->priv->connection_id = 0;
	}

	if (plugin->priv->gconf_client != NULL)
	{
		gconf_client_suggest_sync (plugin->priv->gconf_client, NULL);

		g_object_unref (G_OBJECT (plugin->priv->gconf_client));

		plugin->priv->gconf_client = NULL;
	}

	G_OBJECT_CLASS (gedit_drawspaces_plugin_parent_class)->dispose (object);
}

static void
draw_spaces_in_window (GeditWindow *window,
		       GeditDrawspacesPlugin *plugin)
{
	GList *views, *l;
	WindowData *data;

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	views = gedit_window_get_views (window);
	for (l = views; l != NULL; l = g_list_next (l))
	{
		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (l->data),
						 data->enable ? plugin->priv->flags : 0);
	}

	g_list_free (views);
}

static void
draw_spaces (GeditDrawspacesPlugin *plugin)
{
	const GList *windows, *l;

	windows = gedit_app_get_windows (gedit_app_get_default ());

	for (l = windows; l != NULL; l = g_list_next (l))
	{
		draw_spaces_in_window (l->data, plugin);
	}
}

static void
tab_added_cb (GeditWindow *window,
	      GeditTab *tab,
	      GeditDrawspacesPlugin *plugin)
{
	GeditView *view;
	WindowData *data;

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (data->enable)
	{
		view = gedit_tab_get_view (tab);

		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (view),
						 plugin->priv->flags);
	}
}

static gint
get_gconf_value_with_default_int (GeditDrawspacesPlugin *plugin,
			          const gchar           *key,
			          gint                   def)
{
	GConfValue *value;
	gint ret;

	value = gconf_client_get (plugin->priv->gconf_client,
				  key, NULL);

	if (value != NULL && value->type == GCONF_VALUE_INT)
	{
		ret = gconf_value_get_int (value);
	}
	else
	{
		ret = def;
	}

	if (value != NULL)
	{
		gconf_value_free (value);
	}

	return ret;
}

static gboolean
get_gconf_value_with_default (GeditDrawspacesPlugin *plugin,
			      const gchar           *key,
			      gboolean               def)
{
	GConfValue *value;
	gboolean ret;

	value = gconf_client_get (plugin->priv->gconf_client,
				  key, NULL);

	if (value != NULL && value->type == GCONF_VALUE_BOOL)
	{
		ret = gconf_value_get_bool (value);
	}
	else
	{
		ret = def;
	}

	if (value != NULL)
	{
		gconf_value_free (value);
	}

	return ret;
}

static void
get_config_options (WindowData *data,
		    GeditDrawspacesPlugin *plugin)
{
	gboolean tabs, spaces, newline, nbsp, leading, text, trailing;

	data->enable = get_gconf_value_with_default (plugin, GCONF_KEY_ENABLE,
						     TRUE);

	tabs = get_gconf_value_with_default (plugin, GCONF_KEY_DRAW_TABS,
					     TRUE);

	spaces = get_gconf_value_with_default (plugin, GCONF_KEY_DRAW_SPACES,
					       TRUE);

	newline = get_gconf_value_with_default (plugin, GCONF_KEY_DRAW_NEWLINE,
					        FALSE);

	nbsp = get_gconf_value_with_default (plugin, GCONF_KEY_DRAW_NBSP,
					     FALSE);

	leading = get_gconf_value_with_default (plugin,
	                                        GCONF_KEY_DRAW_LEADING,
	                                        TRUE);

	text = get_gconf_value_with_default (plugin,
	                                     GCONF_KEY_DRAW_TEXT,
	                                     TRUE);

	trailing = get_gconf_value_with_default (plugin,
	                                         GCONF_KEY_DRAW_TRAILING,
	                                         TRUE);

	if (tabs)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TAB;
	}

	if (spaces)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_SPACE;
	}

	if (newline)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	}

	if (nbsp)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NBSP;
	}

	if (leading)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_LEADING;
	}

	if (text)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TEXT;
	}

	if (trailing)
	{
		plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TRAILING;
	}
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GeditDrawspacesPlugin *ds_plugin;
	GtkUIManager *manager;
	GError *error = NULL;
	GtkAction *action;
	WindowData *data;
	ActionData *action_data;

	gedit_debug (DEBUG_PLUGINS);

	ds_plugin = GEDIT_DRAWSPACES_PLUGIN (plugin);

	data = g_slice_new (WindowData);
	action_data = g_slice_new (ActionData);

	action_data->window = window;
	action_data->plugin = ds_plugin;

	get_config_options (data, ds_plugin);

	manager = gedit_window_get_ui_manager (window);

	data->action_group = gtk_action_group_new ("GeditDrawspacesPluginActions");
	gtk_action_group_set_translation_domain (data->action_group,
						 GETTEXT_PACKAGE);
	gtk_action_group_add_toggle_actions_full (data->action_group,
						  action_entries,
						  G_N_ELEMENTS (action_entries),
						  action_data,
						  (GDestroyNotify) free_action_data);

	/* Lets set the default value */
	action = gtk_action_group_get_action (data->action_group,
					      "DrawSpaces");
	g_signal_handlers_block_by_func (action, on_active_toggled, action_data);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      data->enable);
	g_signal_handlers_unblock_by_func (action, on_active_toggled, action_data);

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_add_ui_from_string (manager,
							 submenu,
							 -1,
							 &error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);

	if (data->enable)
	{
		draw_spaces_in_window (window, ds_plugin);
	}

	g_signal_connect (window, "tab-added",
			  G_CALLBACK (tab_added_cb), ds_plugin);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GeditDrawspacesPlugin *ds_plugin = GEDIT_DRAWSPACES_PLUGIN (plugin);
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	manager = gedit_window_get_ui_manager (window);

	data->enable = FALSE;
	draw_spaces_in_window (window, ds_plugin);

	g_signal_handlers_disconnect_by_func (window, tab_added_cb, ds_plugin);

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
on_draw_tabs_toggled (GtkToggleButton       *button,
                      GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_TABS,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_spaces_toggled (GtkToggleButton       *button,
                        GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_SPACES,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_newline_toggled (GtkToggleButton       *button,
                         GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_NEWLINE,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_nbsp_toggled (GtkToggleButton       *button,
                      GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_NBSP,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_leading_toggled (GtkToggleButton       *button,
                         GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_LEADING,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_text_toggled (GtkToggleButton       *button,
                      GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_TEXT,
	                gtk_toggle_button_get_active (button));
}

static void
on_draw_trailing_toggled (GtkToggleButton       *button,
                          GeditDrawspacesPlugin *plugin)
{
	set_draw_gconf (plugin,
	                GCONF_KEY_DRAW_TRAILING,
	                gtk_toggle_button_get_active (button));
}

static void
dialog_destroyed (GtkObject *obj, gpointer dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS);

	g_slice_free (DrawspacesConfigureDialog, dialog_pointer);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static DrawspacesConfigureDialog *
get_configuration_dialog (GeditDrawspacesPlugin *plugin)
{
	DrawspacesConfigureDialog *dialog = NULL;
	gboolean ret;
	GtkWidget *error_widget;
	gchar *datadir;
	gchar *filename;

	gchar *root_objects[] = {
		"dialog_draw_spaces",
		NULL
	};

	dialog = g_slice_new (DrawspacesConfigureDialog);

	datadir = gedit_plugin_get_data_dir (GEDIT_PLUGIN (plugin));
	filename = g_build_filename (datadir, UI_FILE, NULL);

	ret = gedit_utils_get_ui_objects (filename,
					  root_objects,
					  &error_widget,
					  "dialog_draw_spaces", &dialog->dialog,
					  "check_button_draw_tabs", &dialog->draw_tabs,
					  "check_button_draw_spaces", &dialog->draw_spaces,
					  "check_button_draw_new_lines", &dialog->draw_newline,
					  "check_button_draw_nbsp", &dialog->draw_nbsp,
					  "check_button_draw_leading", &dialog->draw_leading,
					  "check_button_draw_text", &dialog->draw_text,
					  "check_button_draw_trailing", &dialog->draw_trailing,
					  NULL);

	g_free (datadir);
	g_free (filename);

	if (!ret)
	{
		GtkWidget *dialog_error;
		GtkWidget *content;

		dialog_error = gtk_dialog_new_with_buttons (_("Error dialog"),
							    NULL,
							    GTK_DIALOG_DESTROY_WITH_PARENT,
							    GTK_STOCK_CLOSE,
							    GTK_RESPONSE_CLOSE,
							    NULL);
		content = gtk_dialog_get_content_area (GTK_DIALOG (dialog_error));
		gtk_widget_show (error_widget);

		gtk_box_pack_start_defaults (GTK_BOX (content),
					     error_widget);
		gtk_widget_show (dialog_error);
		gtk_dialog_run (GTK_DIALOG (dialog_error));
		gtk_widget_destroy (dialog_error);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_tabs),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TAB);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_spaces),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_SPACE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_newline),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_NEWLINE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_nbsp),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_NBSP);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_leading),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_LEADING);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_text),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TEXT);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->draw_trailing),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TRAILING);

	g_signal_connect (dialog->draw_tabs, "toggled",
			  G_CALLBACK (on_draw_tabs_toggled), plugin);
	g_signal_connect (dialog->draw_spaces, "toggled",
			  G_CALLBACK (on_draw_spaces_toggled), plugin);
	g_signal_connect (dialog->draw_newline, "toggled",
			  G_CALLBACK (on_draw_newline_toggled), plugin);
	g_signal_connect (dialog->draw_nbsp, "toggled",
			  G_CALLBACK (on_draw_nbsp_toggled), plugin);
	g_signal_connect (dialog->draw_leading, "toggled",
			  G_CALLBACK (on_draw_leading_toggled), plugin);
	g_signal_connect (dialog->draw_text, "toggled",
			  G_CALLBACK (on_draw_text_toggled), plugin);
	g_signal_connect (dialog->draw_trailing, "toggled",
			  G_CALLBACK (on_draw_trailing_toggled), plugin);

	g_signal_connect (dialog->dialog, "destroy",
			  G_CALLBACK (dialog_destroyed), dialog);

	return dialog;
}

static GtkWidget *
impl_create_configure_dialog (GeditPlugin *plugin)
{
	DrawspacesConfigureDialog *dialog;

	dialog = get_configuration_dialog (GEDIT_DRAWSPACES_PLUGIN (plugin));

	g_signal_connect (dialog->dialog,
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  dialog->dialog);

	return dialog->dialog;
}

static void
on_gconf_notify (GConfClient *client,
		 guint cnxn_id,
		 GConfEntry *entry,
		 gpointer user_data)
{
	GeditDrawspacesPlugin *plugin = GEDIT_DRAWSPACES_PLUGIN (user_data);
	gboolean value;

	if (strcmp (entry->key, GCONF_KEY_DRAW_TABS) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TAB;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TAB;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_SPACES) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_SPACE;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_SPACE;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_NEWLINE) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_NEWLINE;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_NBSP) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NBSP;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_NBSP;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_LEADING) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_LEADING;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_LEADING;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_TEXT) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TEXT;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TEXT;
		}
	}
	else if (strcmp (entry->key, GCONF_KEY_DRAW_TRAILING) == 0)
	{
		value = gconf_value_get_bool (entry->value);

		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TRAILING;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TRAILING;
		}
	}

	draw_spaces (plugin);
}

static void
gedit_drawspaces_plugin_class_init (GeditDrawspacesPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (GeditDrawspacesPluginPrivate));

	object_class->dispose = gedit_drawspaces_plugin_dispose;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->create_configure_dialog = impl_create_configure_dialog;
}
