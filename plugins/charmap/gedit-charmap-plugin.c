/*
 * gedit-charmap-plugin.c - Character map side-pane for gedit
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-charmap-plugin.h"
#include "gedit-charmap-panel.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-panel.h>
#include <gedit/gedit-document.h>
#include <gedit/gedit-prefs-manager.h>
#include <gucharmap/gucharmap-table.h>
#include <gucharmap/gucharmap-unicode-info.h>

#define WINDOW_DATA_KEY	"GeditCharmapPluginWindowData"

#define GEDIT_CHARMAP_PLUGIN_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_CHARMAP_PLUGIN,		\
				GeditCharmapPluginPrivate))

typedef struct
{
	GtkWidget	*panel;
	guint		 context_id;
} WindowData;

GEDIT_PLUGIN_REGISTER_TYPE_WITH_CODE (GeditCharmapPlugin, gedit_charmap_plugin,
		gedit_charmap_panel_register_type (module);
)

static void
gedit_charmap_plugin_init (GeditCharmapPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditCharmapPlugin initializing");
}

static void
gedit_charmap_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditCharmapPlugin finalizing");

	G_OBJECT_CLASS (gedit_charmap_plugin_parent_class)->finalize (object);
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);
	
	g_free (data);
}

static void
on_table_status_message (GucharmapTable *chartable,
			 const gchar    *message,
			 GeditWindow    *window)
{
	GtkStatusbar *statusbar;
	WindowData *data;

	statusbar = GTK_STATUSBAR (gedit_window_get_statusbar (window));
	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_statusbar_pop (statusbar, data->context_id);

	if (message)
		gtk_statusbar_push (statusbar, data->context_id, message);
}

on_table_set_active_char (GucharmapTable *chartable,
			  gunichar        wc,
			  GeditWindow    *window)
{
	GString *gs;
	const gchar *temp;
	const gchar **temps;
	gint i;

	gs = g_string_new (NULL);
	g_string_append_printf (gs, "U+%4.4X %s", wc, 
				gucharmap_get_unicode_name (wc));

	temps = gucharmap_get_nameslist_equals (wc);
	if (temps)
	{
		g_string_append_printf (gs, "   = %s", temps[0]);
		for (i = 1;  temps[i];  i++)
			g_string_append_printf (gs, "; %s", temps[i]);
		g_free (temps);
	}

	temps = gucharmap_get_nameslist_stars (wc);
	if (temps)
	{
		g_string_append_printf (gs, "   \342\200\242 %s", temps[0]);
		for (i = 1;  temps[i];  i++)
			g_string_append_printf (gs, "; %s", temps[i]);
		g_free (temps);
	}

	on_table_status_message (chartable, gs->str, window);
	g_string_free (gs, TRUE);
}

static gboolean
on_table_focus_out_event (GtkWidget      *drawing_area,
			  GdkEventFocus  *event,
			  GeditWindow    *window)
{
	GucharmapTable *chartable;
	WindowData *data;
	
	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_val_if_fail (data != NULL, FALSE);

	chartable = gedit_charmap_panel_get_table
					(GEDIT_CHARMAP_PANEL (data->panel));

	on_table_status_message (chartable, NULL, window);
	return FALSE;
}

static void
on_table_activate (GucharmapTable *chartable, 
		   gunichar        wc, 
		   GeditWindow    *window)
{
	GtkTextView   *view;
	GtkTextBuffer *document;
	GtkTextIter start, end;
	gchar buffer[6];
	gchar length;
	
	g_return_if_fail (gucharmap_unichar_validate (wc));
	
	view = GTK_TEXT_VIEW (gedit_window_get_active_view (window));
	
	if (!view || !gtk_text_view_get_editable (view))
		return;
	
	document = gtk_text_view_get_buffer (view);
	
	g_return_if_fail (document != NULL);
	
	length = g_unichar_to_utf8 (wc, buffer);

	gtk_text_buffer_begin_user_action (document);
		
	gtk_text_buffer_get_selection_bounds (document, &start, &end);

	gtk_text_buffer_delete_interactive (document, &start, &end, TRUE);
	if (gtk_text_iter_editable (&start, TRUE))
		gtk_text_buffer_insert (document, &start, buffer, length);
	
	gtk_text_buffer_end_user_action (document);
}

static GtkWidget *
create_charmap_panel (GeditWindow *window)
{
	GtkWidget      *panel;
	GucharmapTable *table;
	gchar          *font;

	panel = gedit_charmap_panel_new ();
	table = gedit_charmap_panel_get_table (GEDIT_CHARMAP_PANEL (panel));

	/* Use the same font as the document */
	font = gedit_prefs_manager_get_editor_font ();
	gucharmap_table_set_font (table, font);
	g_free (font);

	g_signal_connect (table,
			  "status-message",
			  G_CALLBACK (on_table_status_message),
			  window);

	g_signal_connect (table,
			  "set-active-char",
			  G_CALLBACK (on_table_set_active_char),
			  window);

	/* Note: GucharmapTable does not provide focus-out-event ... */
	g_signal_connect (table->drawing_area,
			  "focus-out-event",
			  G_CALLBACK (on_table_focus_out_event),
			  window);

	g_signal_connect (table,
			  "activate", 
			  G_CALLBACK (on_table_activate),
			  window);

	gtk_widget_show_all (panel);

	return panel;
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GeditPanel *panel;
	GtkWidget *image;
	GtkIconTheme *theme;
	GtkStatusbar *statusbar;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	panel = gedit_window_get_side_panel (window);

	data = g_new (WindowData, 1);

	theme = gtk_icon_theme_get_default ();
	
	if (gtk_icon_theme_has_icon (theme, "accessories-character-map"))
		image = gtk_image_new_from_icon_name ("accessories-character-map",
						      GTK_ICON_SIZE_MENU);
	else
		image = gtk_image_new_from_icon_name ("gucharmap",
						      GTK_ICON_SIZE_MENU);

	data->panel = create_charmap_panel (window);
	
	gedit_panel_add_item (panel,
			      data->panel,
			      _("Character Map"),
			      image);

	gtk_object_sink (GTK_OBJECT (image));

	statusbar = GTK_STATUSBAR (gedit_window_get_statusbar (window));
	data->context_id = gtk_statusbar_get_context_id (statusbar,
							 "Character Description");

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GeditPanel *panel;
	GucharmapTable *chartable;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	chartable = gedit_charmap_panel_get_table
					(GEDIT_CHARMAP_PANEL (data->panel));
	on_table_status_message (chartable, NULL, window);

	panel = gedit_window_get_side_panel (window);
	gedit_panel_remove_item (panel, data->panel);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
gedit_charmap_plugin_class_init (GeditCharmapPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_charmap_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}
