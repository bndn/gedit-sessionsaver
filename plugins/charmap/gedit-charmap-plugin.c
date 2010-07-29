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
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-panel.h>
#include <gedit/gedit-document.h>

#include <gucharmap/gucharmap.h>

#define GEDIT_CHARMAP_PLUGIN_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_CHARMAP_PLUGIN,		\
				GeditCharmapPluginPrivate))

struct _GeditCharmapPluginPrivate
{
	GeditWindow     *window;

	GtkWidget       *panel;
	guint            context_id;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditCharmapPlugin,
				gedit_charmap_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)	\
													\
				_gedit_charmap_panel_register_type (type_module);			\
)

static void
gedit_charmap_plugin_init (GeditCharmapPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditCharmapPlugin initializing");

	plugin->priv = GEDIT_CHARMAP_PLUGIN_GET_PRIVATE (plugin);
}

static void
gedit_charmap_plugin_dispose (GObject *object)
{
	GeditCharmapPlugin *plugin = GEDIT_CHARMAP_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditCharmapPlugin disposing");

	if (plugin->priv->window != NULL)
	{
		g_object_unref (plugin->priv->window);
		plugin->priv->window = NULL;
	}

	G_OBJECT_CLASS (gedit_charmap_plugin_parent_class)->dispose (object);
}

static void
gedit_charmap_plugin_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
	GeditCharmapPlugin *plugin = GEDIT_CHARMAP_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_charmap_plugin_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
	GeditCharmapPlugin *plugin = GEDIT_CHARMAP_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
on_table_status_message (GucharmapChartable *chartable,
			 const gchar        *message,
			 GeditCharmapPlugin *plugin)
{
	GtkStatusbar *statusbar;

	statusbar = GTK_STATUSBAR (gedit_window_get_statusbar (plugin->priv->window));

	gtk_statusbar_pop (statusbar, plugin->priv->context_id);

	if (message)
		gtk_statusbar_push (statusbar, plugin->priv->context_id, message);
}

static void
on_table_sync_active_char (GucharmapChartable *chartable,
			   GParamSpec         *psepc,
			   GeditCharmapPlugin *plugin)
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

	on_table_status_message (chartable, gs->str, plugin);
	g_string_free (gs, TRUE);
}

static gboolean
on_table_focus_out_event (GtkWidget          *drawing_area,
			  GdkEventFocus      *event,
			  GeditCharmapPlugin *plugin)
{
	GucharmapChartable *chartable;

	chartable = gedit_charmap_panel_get_chartable
					(GEDIT_CHARMAP_PANEL (plugin->priv->panel));

	on_table_status_message (chartable, NULL, plugin);
	return FALSE;
}

static void
on_table_activate (GucharmapChartable *chartable,
		   GeditWindow        *window)
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

static gchar *
get_document_font ()
{
	GSettings *editor;
	gboolean use_default_font;
	gchar *font;

	editor = g_settings_new ("org.gnome.gedit.preferences.editor");

	use_default_font = g_settings_get_boolean (editor, "use-default-font");

	if (use_default_font)
	{
		GSettings *system;

		system = g_settings_new ("org.gnome.Desktop.Interface");
		font = g_settings_get_string (system, "monospace-font-name");
		g_object_unref (system);
	}
	else
	{
		font = g_settings_get_string (editor, "editor-font");
	}

	g_object_unref (editor);

	return font;
}

static GtkWidget *
create_charmap_panel (GeditCharmapPlugin *plugin)
{
	GSettings *settings;
	GtkWidget *panel;
	GucharmapChartable *chartable;
	PangoFontDescription *font_desc;
	gchar *font;

	panel = gedit_charmap_panel_new ();

	/* Use the same font as the document */
	font = get_document_font ();

	chartable = gedit_charmap_panel_get_chartable (GEDIT_CHARMAP_PANEL (panel));
	font_desc = pango_font_description_from_string (font);
	gucharmap_chartable_set_font_desc (chartable, font_desc);
	pango_font_description_free (font_desc);

	g_free (font);

	g_signal_connect (chartable,
			  "notify::active-character",
			  G_CALLBACK (on_table_sync_active_char),
			  plugin);
	g_signal_connect (chartable,
			  "focus-out-event",
			  G_CALLBACK (on_table_focus_out_event),
			  plugin);
	g_signal_connect (chartable,
			  "status-message",
			  G_CALLBACK (on_table_status_message),
			  plugin);
	g_signal_connect (chartable,
			  "activate",
			  G_CALLBACK (on_table_activate),
			  plugin->priv->window);

	gtk_widget_show_all (panel);

	return panel;
}

static void
gedit_charmap_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditCharmapPluginPrivate *priv;
	GeditPanel *panel;
	GtkWidget *image;
	GtkIconTheme *theme;
	GtkStatusbar *statusbar;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_CHARMAP_PLUGIN (activatable)->priv;

	panel = gedit_window_get_side_panel (priv->window);

	theme = gtk_icon_theme_get_default ();

	if (gtk_icon_theme_has_icon (theme, "accessories-character-map"))
		image = gtk_image_new_from_icon_name ("accessories-character-map",
						      GTK_ICON_SIZE_MENU);
	else
		image = gtk_image_new_from_icon_name ("gucharmap",
						      GTK_ICON_SIZE_MENU);

	priv->panel = create_charmap_panel (GEDIT_CHARMAP_PLUGIN (activatable));

	gedit_panel_add_item (panel,
			      priv->panel,
			      "GeditCharmapPanel",
			      _("Character Map"),
			      image);

	gtk_object_sink (GTK_OBJECT (image));

	statusbar = GTK_STATUSBAR (gedit_window_get_statusbar (priv->window));
	priv->context_id = gtk_statusbar_get_context_id (statusbar,
							 "Character Description");
}

static void
gedit_charmap_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditCharmapPluginPrivate *priv;
	GeditPanel *panel;
	GucharmapChartable *chartable;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_CHARMAP_PLUGIN (activatable)->priv;

	chartable = gedit_charmap_panel_get_chartable
					(GEDIT_CHARMAP_PANEL (priv->panel));
	on_table_status_message (chartable, NULL,
				 GEDIT_CHARMAP_PLUGIN (activatable));

	panel = gedit_window_get_side_panel (priv->window);
	gedit_panel_remove_item (panel, priv->panel);
}

static void
gedit_charmap_plugin_class_init (GeditCharmapPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_charmap_plugin_dispose;
	object_class->set_property = gedit_charmap_plugin_set_property;
	object_class->get_property = gedit_charmap_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");

	g_type_class_add_private (object_class, sizeof (GeditCharmapPluginPrivate));
}

static void
gedit_charmap_plugin_class_finalize (GeditCharmapPluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_charmap_plugin_activate;
	iface->deactivate = gedit_charmap_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_charmap_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_CHARMAP_PLUGIN);
}
