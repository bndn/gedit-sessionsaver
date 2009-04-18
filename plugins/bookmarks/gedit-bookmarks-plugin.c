/*
 * gedit-bookmarks-plugin.c - Bookmarking for gedit
 * 
 * Copyright (C) 2008 Jesse van den Kieboom
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

#include "gedit-bookmarks-plugin.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-panel.h>
#include <gedit/gedit-document.h>
#include <gedit/gedit-prefs-manager.h>

#define WINDOW_DATA_KEY	"GeditBookmarksPluginWindowData"

#define GEDIT_BOOKMARKS_PLUGIN_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_BOOKMARKS_PLUGIN,		\
				GeditBookmarksPluginPrivate))

#define BOOKMARKS_DATA(window) ((WindowData *)(g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY)))

#define BOOKMARK_CATEGORY "GeditBookmarksPluginBookmark"
#define INSERT_DATA_KEY "GeditBookmarksInsertData"

typedef struct
{
	gint previous_line;
	gboolean new_user_action;
} InsertData;

static void update_background_color		(GeditView   *view);
static void on_style_scheme_notify		(GObject     *object,
						 GParamSpec  *pspec,
						 GeditView   *view);

static void on_delete_range			(GtkTextBuffer *buffer,
						 GtkTextIter   *start,
						 GtkTextIter   *end,
						 gpointer       user_data);

static void on_insert_text_before		(GtkTextBuffer *buffer,
						 GtkTextIter   *location,
						 gchar         *text,
						 gint		len,
						 InsertData    *data);

static void on_insert_text_after		(GtkTextBuffer *buffer,
						 GtkTextIter   *location,
						 gchar         *text,
						 gint		len,
						 InsertData    *data);

static void on_begin_user_action		(GtkTextBuffer *buffer,
						 InsertData    *data);

static void on_end_user_action			(GtkTextBuffer *buffer,
						 InsertData    *data);

static void on_toggle_bookmark_activate 	(GtkAction   *action, 
						 GeditWindow *window);
static void on_next_bookmark_activate 		(GtkAction   *action, 
						 GeditWindow *window);
static void on_previous_bookmark_activate 	(GtkAction   *action, 
						 GeditWindow *window);
static void on_tab_added 			(GeditWindow *window, 
						 GeditTab    *tab, 
						 GeditPlugin *plugin);
static void on_tab_removed 			(GeditWindow *window, 
						 GeditTab    *tab, 
						 GeditPlugin *plugin);
			  
typedef struct
{
	GtkActionGroup * action_group;
	guint ui_id;
} WindowData;

GEDIT_PLUGIN_REGISTER_TYPE (GeditBookmarksPlugin, gedit_bookmarks_plugin)

static void
gedit_bookmarks_plugin_init (GeditBookmarksPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditBookmarksPlugin initializing");
}

static void
gedit_bookmarks_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditBookmarksPlugin finalizing");

	G_OBJECT_CLASS (gedit_bookmarks_plugin_parent_class)->finalize (object);
}

static void
free_window_data (WindowData *data)
{
	g_slice_free (WindowData, data);
}

static void
free_insert_data (InsertData *data)
{
	g_slice_free (InsertData, data);
}

static GtkActionEntry const action_entries[] = {
	{"ToggleBookmark", NULL, N_("Toggle Bookmark"), "<Control><Alt>B",
	 N_("Toggle bookmark status of the current line"), 
	 G_CALLBACK (on_toggle_bookmark_activate)},
	{"NextBookmark", NULL, N_("Goto Next Bookmark"), "<Control>B",
	 N_("Goto the next bookmark"),
	 G_CALLBACK (on_next_bookmark_activate)},
	{"PreviousBookmark", NULL, N_("Goto Previous Bookmark"), "<Control><Shift>B",
	 N_("Goto the previous bookmark"),
	 G_CALLBACK (on_previous_bookmark_activate)}
};

static gchar const uidefinition[] = ""
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='EditMenu' action='Edit'>"
"      <placeholder name='EditOps_6'>"
"        <menuitem action='ToggleBookmark'/>"
"        <menuitem action='PreviousBookmark'/>"
"        <menuitem action='NextBookmark'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";

static void
install_menu (GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data = BOOKMARKS_DATA (window);
	GError *error = NULL;

	manager = gedit_window_get_ui_manager (window);
	data->action_group = gtk_action_group_new ("GeditBookmarksPluginActions");
	
	gtk_action_group_set_translation_domain (data->action_group,
						 GETTEXT_PACKAGE);
	
	gtk_action_group_add_actions (data->action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);
				      
	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);
	data->ui_id = gtk_ui_manager_add_ui_from_string (manager, uidefinition, -1, &error);
	
	if (!data->ui_id)
	{
		g_warning ("Could not load UI: %s", error->message);
		g_error_free (error);
	}
}

static void
uninstall_menu (GeditWindow *window)
{
	WindowData *data = BOOKMARKS_DATA (window);
	GtkUIManager *manager;
	
	manager = gedit_window_get_ui_manager (window);
	
	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);
	
	g_object_unref (data->action_group);
}

static void
disable_bookmarks (GeditView *view)
{
	GtkTextIter start;
	GtkTextIter end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gpointer data;
	
	gtk_source_view_set_show_line_marks (GTK_SOURCE_VIEW (view), FALSE);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_source_buffer_remove_source_marks (GTK_SOURCE_BUFFER (buffer), 
					       &start, 
					       &end, 
					       BOOKMARK_CATEGORY);

	g_signal_handlers_disconnect_by_func (buffer, on_style_scheme_notify, view);
	g_signal_handlers_disconnect_by_func (buffer, on_delete_range, NULL);
	
	data = g_object_get_data (G_OBJECT (buffer), INSERT_DATA_KEY);

	g_signal_handlers_disconnect_by_func (buffer, on_insert_text_before, data);
	g_signal_handlers_disconnect_by_func (buffer, on_insert_text_after, data);
	g_signal_handlers_disconnect_by_func (buffer, on_begin_user_action, data);
	g_signal_handlers_disconnect_by_func (buffer, on_end_user_action, data);
	
	g_object_set_data (G_OBJECT (buffer), INSERT_DATA_KEY, NULL);
}

static GdkPixbuf *
get_bookmark_pixbuf (GeditPlugin *plugin)
{
	gchar *datadir;
	gchar *iconpath;
	GdkPixbuf *pixbuf;

	datadir = gedit_plugin_get_data_dir (plugin);
	iconpath = g_build_filename (datadir, "bookmark.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file (iconpath, NULL);

	g_free (datadir);
	g_free (iconpath);

	return pixbuf;
}

static void
enable_bookmarks (GeditView   *view,
		  GeditPlugin *plugin)
{
	GdkPixbuf *pixbuf;

	pixbuf = get_bookmark_pixbuf (plugin);

	/* Make sure the category pixbuf is set */
	if (pixbuf)
	{
		InsertData *data;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

		update_background_color (view);		
		gtk_source_view_set_mark_category_pixbuf (GTK_SOURCE_VIEW (view),
							  BOOKMARK_CATEGORY,
							  pixbuf);
		g_object_unref (pixbuf);

		gtk_source_view_set_show_line_marks (GTK_SOURCE_VIEW (view), TRUE);
		
		g_signal_connect (buffer,
				  "notify::style-scheme",
				  G_CALLBACK (on_style_scheme_notify),
				  view);

		g_signal_connect_after (buffer,
				        "delete-range",
				        G_CALLBACK (on_delete_range),
				        NULL);
				        
		data = g_slice_new (InsertData);
		data->new_user_action = FALSE;
		data->previous_line = -1;
		
		g_object_set_data_full (G_OBJECT (buffer), 
					INSERT_DATA_KEY, 
					data,
					(GDestroyNotify) free_insert_data);

		g_signal_connect (buffer,
				  "insert-text",
				  G_CALLBACK (on_insert_text_before),
				  data);
				  
		g_signal_connect_after (buffer,
				        "insert-text",
				        G_CALLBACK (on_insert_text_after),
				        data);
				        
		g_signal_connect (buffer,
				  "begin-user-action",
				  G_CALLBACK (on_begin_user_action),
				  data);
		g_signal_connect (buffer,
				  "end-user-action",
				  G_CALLBACK (on_end_user_action),
				  data);

	}
	else
	{
		g_warning ("Could not set bookmark icon!");
	}
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	WindowData *data;
	GList *views;
	GList *item;

	gedit_debug (DEBUG_PLUGINS);

	data = g_slice_new (WindowData);
	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
	
	views = gedit_window_get_views (window);
	for (item = views; item != NULL; item = item->next)
	{
		enable_bookmarks (GEDIT_VIEW (item->data), plugin);
	}

	g_list_free (views);

	g_signal_connect (window, "tab-added",
			  G_CALLBACK (on_tab_added), plugin);

	g_signal_connect (window, "tab-removed",
			  G_CALLBACK (on_tab_removed), plugin);

	install_menu (window);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	WindowData *data;
	GList *views;
	GList *item;

	gedit_debug (DEBUG_PLUGINS);

	uninstall_menu (window);
	
	views = gedit_window_get_views (window);

	for (item = views; item != NULL; item = item->next)
		disable_bookmarks (GEDIT_VIEW (item->data));
	
	g_list_free (views);
	
	data = BOOKMARKS_DATA (window);
	g_return_if_fail (data != NULL);
	
	g_signal_handlers_disconnect_by_func (window, on_tab_added, plugin);
	g_signal_handlers_disconnect_by_func (window, on_tab_removed, plugin);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
gedit_bookmarks_plugin_class_init (GeditBookmarksPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_bookmarks_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
update_background_color (GeditView *view)
{
	GtkSourceView *source_view = GTK_SOURCE_VIEW (view);
	GtkSourceStyle *style;
	GtkSourceStyleScheme *scheme;
	GtkTextBuffer *buffer;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	scheme = gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (buffer));
	style = gtk_source_style_scheme_get_style (scheme, "search-match");
	
	if (style)
	{
		gboolean bgset;
		gchar *bg;
		
		g_object_get (style, "background-set", &bgset, "background", &bg, NULL);
		
		if (bgset)
		{
			GdkColor color;
			gdk_color_parse (bg, &color);
			gtk_source_view_set_mark_category_background (source_view,
								      BOOKMARK_CATEGORY,
								      &color);
			g_free (bg);
			
			return;
		}
	}
	
	gtk_source_view_set_mark_category_background (source_view,
						      BOOKMARK_CATEGORY,
						      NULL);
}

static void 
on_style_scheme_notify (GObject     *object,
			GParamSpec  *pspec,
			GeditView   *view)
{
	update_background_color (view);
}

static void 
on_delete_range (GtkTextBuffer *buffer,
	     	 GtkTextIter   *start,
	     	 GtkTextIter   *end,
	     	 gpointer       user_data)
{
	GtkTextIter iter;
	GSList *marks;
	GSList *item;

	iter = *start;
	
	/* move to start of line */
	gtk_text_iter_set_line_offset (&iter, 0);
	
	/* remove any bookmarks that are collapsed on each other due to this */
	marks = gtk_source_buffer_get_source_marks_at_iter (GTK_SOURCE_BUFFER (buffer), 
							    &iter,
							    BOOKMARK_CATEGORY);

	if (marks == NULL)
		return;
	
	/* remove all but the first mark */
	for (item = marks->next; item; item = item->next)
		gtk_text_buffer_delete_mark (buffer, GTK_TEXT_MARK (item->data));

	g_slist_free (marks);
}

static void
on_begin_user_action (GtkTextBuffer *buffer,
		      InsertData    *data)
{
	data->new_user_action = TRUE;
}

static void
on_end_user_action (GtkTextBuffer *buffer,
		    InsertData    *data)
{
	data->previous_line = -1;
}

static void 
on_insert_text_before (GtkTextBuffer *buffer,
		       GtkTextIter   *location,
		       gchar         *text,
		       gint	      len,
		       InsertData    *data)
{
	if (data->new_user_action && !gtk_text_iter_starts_line (location))
	{
		data->previous_line = -1;
	}
	else if (data->new_user_action)
	{
		GSList *marks;
		marks = gtk_source_buffer_get_source_marks_at_iter (GTK_SOURCE_BUFFER (buffer),
							    	    location,
							    	    BOOKMARK_CATEGORY);

		if (marks == NULL)
		{
			data->previous_line = -1;
		}
		else
		{
			data->previous_line = gtk_text_iter_get_line (location);
			g_slist_free (marks);
		}
		
		data->new_user_action = FALSE;
	}
}

static void 
on_insert_text_after (GtkTextBuffer *buffer,
		      GtkTextIter   *location,
		      gchar         *text,
		      gint	     len,
		      InsertData    *data)
{
	gint current;
	
	if (data->previous_line == -1)
		return;
	
	current = gtk_text_iter_get_line (location);

	if (current != data->previous_line)
	{
		GtkTextIter iter = *location;
		GSList *marks;
		GSList *item;
		
		gtk_text_iter_set_line_offset (&iter, 0);
		
		marks = gtk_source_buffer_get_source_marks_at_line (GTK_SOURCE_BUFFER (buffer),
								    data->previous_line,
								    BOOKMARK_CATEGORY);

		for (item = marks; item; item = item->next)
			gtk_text_buffer_move_mark (buffer, GTK_TEXT_MARK (item->data), &iter);

		g_slist_free (marks);
		data->previous_line = current;
	}
}

static void
on_toggle_bookmark_activate (GtkAction   *action,
			     GeditWindow *window)
{
	GeditDocument *doc = gedit_window_get_active_document (window);
	GtkTextMark *insert;
	GtkSourceMark *bookmark = NULL;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GSList *marks;

	if (!doc)
		return;
	
	buffer = GTK_TEXT_BUFFER (doc);	
	insert = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert);
	
	/* Move the iter to the beginning of the line, where the bookmarks are */
	gtk_text_iter_set_line_offset (&iter, 0);
	
	marks = gtk_source_buffer_get_source_marks_at_iter (GTK_SOURCE_BUFFER (buffer), 
							    &iter, 
							    BOOKMARK_CATEGORY);
	
	if (marks)
		bookmark = GTK_SOURCE_MARK (marks->data);
	
	g_slist_free (marks);
	
	if (bookmark)
	{
		/* Remove the bookmark */
		gtk_text_buffer_delete_mark (buffer, GTK_TEXT_MARK (bookmark));
	}
	else
	{
		/* Add new bookmark */
		gtk_source_buffer_create_source_mark (GTK_SOURCE_BUFFER (buffer), 
						      NULL, 
						      BOOKMARK_CATEGORY, 
						      &iter);
	}
}

typedef gboolean (*IterSearchFunc)(GtkSourceBuffer *buffer, GtkTextIter *iter, const gchar *category);
typedef void (*CycleFunc)(GtkTextBuffer *buffer, GtkTextIter *iter);

static void
goto_bookmark (GeditWindow    *window,
	       IterSearchFunc  func,
	       CycleFunc       cycle_func)
{
	GeditView *view = gedit_window_get_active_view (window);
	GeditDocument *doc = gedit_window_get_active_document (window);
	GtkTextBuffer *buffer;
	GtkTextMark *insert;
	GtkTextIter iter;
	GtkTextIter end;
	
	if (doc == NULL)
		return;
	
	buffer = GTK_TEXT_BUFFER (doc);	
	insert = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert);
	
	/* Move the iter to the beginning of the line, where the bookmarks are */
	gtk_text_iter_set_line_offset (&iter, 0);

	/* Try to find the next bookmark */
	if (!func (GTK_SOURCE_BUFFER (buffer), &iter, BOOKMARK_CATEGORY))
	{
		GSList *marks;
		
		/* cycle through */
		cycle_func (buffer, &iter);
		gtk_text_iter_set_line_offset (&iter, 0);
		
		marks = gtk_source_buffer_get_source_marks_at_iter (GTK_SOURCE_BUFFER (buffer),
							    	    &iter,
							    	    BOOKMARK_CATEGORY);
		
		if (!marks && !func (GTK_SOURCE_BUFFER (buffer), &iter, BOOKMARK_CATEGORY))
			return;
		
		g_slist_free (marks);
	}
	
	end = iter;
		
	if (!gtk_text_iter_forward_visible_line (&end))
		gtk_text_buffer_get_end_iter (buffer, &end);
	else
		gtk_text_iter_backward_char (&end);
	
	gtk_text_buffer_select_range (buffer, &iter, &end);
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view), insert, 0.3, FALSE, 0, 0);
}

static void
on_next_bookmark_activate (GtkAction   *action,
			   GeditWindow *window)
{
	goto_bookmark (window, 
		       gtk_source_buffer_forward_iter_to_source_mark,
		       gtk_text_buffer_get_start_iter);
}

static void
on_previous_bookmark_activate (GtkAction   *action,
			       GeditWindow *window)
{
	goto_bookmark (window, 
		       gtk_source_buffer_backward_iter_to_source_mark,
		       gtk_text_buffer_get_end_iter);
}

static void
on_tab_added (GeditWindow *window,
	      GeditTab    *tab,
	      GeditPlugin *plugin)
{
	enable_bookmarks (gedit_tab_get_view (tab), plugin);
}

static void
on_tab_removed (GeditWindow *window,
	        GeditTab    *tab,
	        GeditPlugin *plugin)
{
	disable_bookmarks (gedit_tab_get_view (tab));
}
