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

#include <gucharmap/gucharmap.h>

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
	g_slice_free (WindowData, data);
}

static void
on_table_status_message (GucharmapChartable *chartable,
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

static void
on_table_sync_active_char (GucharmapChartable *chartable,
			   GParamSpec         *psepc,
			   GeditWindow        *window)
{
	GString *gs;
	const gchar **temps;
	gint i;
	gunichar wc;

	wc = gucharmap_chartable_get_active_character (chartable);

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
	GucharmapChartable *chartable;
	WindowData *data;

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_val_if_fail (data != NULL, FALSE);

	chartable = gedit_charmap_panel_get_chartable
					(GEDIT_CHARMAP_PANEL (data->panel));

	on_table_status_message (chartable, NULL, window);
	return FALSE;
}

static void
on_table_activate (GucharmapChartable *chartable,
		   GeditWindow *window)
{
	GtkTextView   *view;
	GtkTextBuffer *document;
	GtkTextIter start, end;
	gchar buffer[6];
	gchar length;
        gunichar wc;

        wc = gucharmap_chartable_get_active_character (chartable);

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
	GucharmapChartable *chartable;
	PangoFontDescription *font_desc;
	gchar *font;

	panel = gedit_charmap_panel_new ();

	/* Use the same font as the document */
	font = gedit_prefs_manager_get_editor_font ();

	chartable = gedit_charmap_panel_get_chartable (GEDIT_CHARMAP_PANEL (panel));
	font_desc = pango_font_description_from_string (font);
	gucharmap_chartable_set_font_desc (chartable, font_desc);
	pango_font_description_free (font_desc);

	g_free (font);

	g_signal_connect (chartable,
			  "notify::active-character",
			  G_CALLBACK (on_table_sync_active_char),
			  window);
	g_signal_connect (chartable,
			  "focus-out-event",
			  G_CALLBACK (on_table_focus_out_event),
			  window);
	g_signal_connect (chartable,
			  "status-message",
			  G_CALLBACK (on_table_status_message),
			  window);
	g_signal_connect (chartable,
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

	data = g_slice_new (WindowData);

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
	WindowData *data;
	GucharmapChartable *chartable;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	chartable = gedit_charmap_panel_get_chartable
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
