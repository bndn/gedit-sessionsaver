/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-taglist-plugin-window.c
 * This file is part of the gedit taglist plugin
 *
 * Copyright (C) 2002 Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-help.h>
#include <gdk/gdkkeysyms.h>

#include <string.h>

#include "gedit-taglist-plugin.h"
#include "gedit-taglist-plugin-window.h"

#include <gedit/gedit-debug.h>
#include <gedit/gedit2.h>
#include <gedit/gedit-document.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-menus.h>

enum
{
	COLUMN_TAG_NAME,
	COLUMN_TAG_INDEX_IN_GROUP,
	NUM_COLUMNS
};

typedef struct _TagListWindow TagListWindow;

struct _TagListWindow
{
	GtkWindow *window;

	GtkWidget *tag_groups_combo;
	GtkWidget *tags_list;

	TagGroup *selected_tag_group;
};

static TagListWindow *tag_list_window = NULL;

static void window_destroyed (GtkObject *obj,  void **window_pointer);
static void populate_tag_groups_combo (void);
static void selected_group_changed (GtkEntry *entry, TagListWindow *w);
static TagGroup *find_tag_group (const gchar *name);
static void populate_tags_list (void);
static GtkTreeModel *create_model (void);
static void tag_list_row_activated_cb (GtkTreeView *tag_list, GtkTreePath *path,
		  GtkTreeViewColumn *column, gpointer data);
static gboolean tag_list_key_press_event_cb (GtkTreeView *tag_list, GdkEventKey *event);
static void insert_tag (Tag *tag, gboolean focus_to_document);
static gboolean tag_list_window_key_press_event_cb (GtkTreeView *tag_list, GdkEventKey *event);

static void
window_destroyed (GtkObject *obj,  void **window_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (window_pointer != NULL)
	{
		GList *top_windows;
        	gedit_debug (DEBUG_PLUGINS, "");

        	top_windows = gedit_get_top_windows ();

        	while (top_windows)
        	{
			BonoboUIComponent *ui_component;
			
			ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
			
			gedit_menus_set_verb_state (ui_component, "/commands/" MENU_ITEM_NAME, 0);

			top_windows = g_list_next (top_windows);
		}

		g_free (*window_pointer);
		*window_pointer = NULL;	
	}	
}

void taglist_window_show ()
{
	GtkWidget *vbox;
	GtkWidget *sw;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	gedit_debug (DEBUG_PLUGINS, "");

	if (tag_list_window != NULL)
	{
		gtk_window_set_transient_for (tag_list_window->window,
			GTK_WINDOW (gedit_get_active_window ()));
			
		gtk_window_present (tag_list_window->window);
		gtk_widget_grab_focus (tag_list_window->tags_list);
		
		return;
	}

	tag_list_window = g_new0 (TagListWindow, 1);
	
	tag_list_window->window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title (GTK_WINDOW (tag_list_window->window), 
			      _("Tag list plugin"));
	
	/* Connect destroy signal */
	
	g_signal_connect (G_OBJECT (tag_list_window->window), "destroy",
			  G_CALLBACK (window_destroyed), &tag_list_window);

	/* Build the window content */
	vbox = gtk_vbox_new (FALSE, 4);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

	gtk_container_add (GTK_CONTAINER (tag_list_window->window), vbox);

	tag_list_window->tag_groups_combo = gtk_combo_new ();

	gtk_tooltips_set_tip (gtk_tooltips_new (),
			GTK_COMBO (tag_list_window->tag_groups_combo)->entry,
			_("Select the group of tags you want to use"),
			NULL);
	
	gtk_editable_set_editable (GTK_EDITABLE (
			GTK_COMBO (tag_list_window->tag_groups_combo)->entry), 
			FALSE);

	gtk_box_pack_start (GTK_BOX (vbox), tag_list_window->tag_groups_combo, FALSE, TRUE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

	/* Create tree view */
	tag_list_window->tags_list = gtk_tree_view_new ();

	gedit_utils_set_atk_name_description (tag_list_window->tag_groups_combo, 
							_("Tag Groups Combo"), NULL);
	gedit_utils_set_atk_name_description (tag_list_window->tags_list, 
							_("Tags Name List"), NULL);
	gedit_utils_set_atk_relation (tag_list_window->tag_groups_combo, tag_list_window->tags_list, 
							ATK_RELATION_CONTROLLER_FOR);
	gedit_utils_set_atk_relation (tag_list_window->tags_list, tag_list_window->tag_groups_combo, 
							ATK_RELATION_CONTROLLED_BY);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tag_list_window->tags_list), FALSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tag_list_window->tags_list), FALSE);

	gtk_tooltips_set_tip (gtk_tooltips_new (),
			tag_list_window->tags_list,
			_("Double-click on a tag to insert it in the current document"),
			NULL);

	g_signal_connect_after (G_OBJECT (tag_list_window->tags_list), "row_activated",
			  G_CALLBACK (tag_list_row_activated_cb), NULL);

	g_signal_connect_after (G_OBJECT (tag_list_window->tags_list), "key_press_event",
			  G_CALLBACK (tag_list_key_press_event_cb), NULL);

	/* Add the tags column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Tags"), cell, 
			"text", COLUMN_TAG_NAME, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tag_list_window->tags_list), column);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (tag_list_window->tags_list),
			COLUMN_TAG_NAME);

	gtk_container_add (GTK_CONTAINER (sw), tag_list_window->tags_list);

	gtk_window_set_default_size (GTK_WINDOW (tag_list_window->window), 250, 450);

	g_signal_connect (G_OBJECT (GTK_COMBO (tag_list_window->tag_groups_combo)->entry), 
			  "changed",
			  G_CALLBACK (selected_group_changed), 
			  tag_list_window);

	g_signal_connect (G_OBJECT (tag_list_window->window), "key_press_event",
			  G_CALLBACK (tag_list_window_key_press_event_cb), NULL);

	/* Populate combo box */
	populate_tag_groups_combo ();

	gtk_window_set_transient_for (tag_list_window->window,
			GTK_WINDOW (gedit_get_active_window ()));
				
	gtk_widget_show_all (GTK_WIDGET (tag_list_window->window));
	gtk_widget_grab_focus (tag_list_window->tags_list);

}

void 
taglist_window_close ()
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (tag_list_window != NULL)
		gtk_widget_destroy (GTK_WIDGET (tag_list_window->window));
}

static void
populate_tag_groups_combo (void)
{
	GList *list;
	GList *cbitems = NULL;
	GtkCombo *combo;
	
	gedit_debug (DEBUG_PLUGINS, "");

	combo = GTK_COMBO (tag_list_window->tag_groups_combo);
	
	g_return_if_fail (taglist != NULL);
	g_return_if_fail (combo != NULL);

	list = taglist->tag_groups;

	/* Build cbitems */
	while (list)
	{
		cbitems = g_list_append (cbitems, ((TagGroup*)list->data)->name);

		list = g_list_next (list);
	}

	gtk_combo_set_popdown_strings (combo, cbitems);

	/* Free cbitems */
	g_list_free (cbitems);
	
	return;
}

static void 
selected_group_changed (GtkEntry *entry, TagListWindow *w)
{
	const gchar* group_name;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	group_name = gtk_entry_get_text (entry);

	if ((group_name == NULL) || (strlen (group_name) <= 0))
		return;

	if ((w->selected_tag_group == NULL) || 
	    (strcmp (group_name, w->selected_tag_group->name) != 0))
	{
		w->selected_tag_group = find_tag_group (group_name);
		g_return_if_fail (w->selected_tag_group != NULL);
		
		gedit_debug (DEBUG_PLUGINS, "New selected group: %s", 
				w->selected_tag_group->name);

		populate_tags_list ();
	}
}


static TagGroup *
find_tag_group (const gchar *name)
{
	GList *list;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (taglist != NULL, NULL);

	list = taglist->tag_groups;

	while (list)
	{
		if (strcmp (name, ((TagGroup*)list->data)->name) == 0)
			return (TagGroup*)list->data;

			list = g_list_next (list);
	}

	return NULL;
}

static void
populate_tags_list (void)
{
	GtkTreeModel* model;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (taglist != NULL);

	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (tag_list_window->tags_list), 
			         model);
	
	g_object_unref (G_OBJECT (model));
}

static GtkTreeModel*
create_model (void)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeIter iter;
	GList *list;
	
	gedit_debug (DEBUG_PLUGINS, "");

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	/* add data to the list store */
	list = tag_list_window->selected_tag_group->tags;
	
	while (list != NULL)
	{
		const gchar* tag_name;

		tag_name = ((Tag*)list->data)->name;

		gedit_debug (DEBUG_PLUGINS, "%d : %s", i, tag_name);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_TAG_NAME, tag_name,
				    COLUMN_TAG_INDEX_IN_GROUP, i,
				    -1);
		++i;

		list = g_list_next (list);
	}
	
	gedit_debug (DEBUG_PLUGINS, "Rows: %d ", 
			gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL));
	
	return GTK_TREE_MODEL (store);
}

static void
tag_list_row_activated_cb (GtkTreeView *tag_list, GtkTreePath *path,
		  GtkTreeViewColumn *column, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint index;
	
	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tag_list_window->tags_list));
	g_return_if_fail (model != NULL);
	
	gtk_tree_model_get_iter (model, &iter, path);
	g_return_if_fail (&iter != NULL);

	gtk_tree_model_get (model, &iter, COLUMN_TAG_INDEX_IN_GROUP, &index, -1);

	gedit_debug (DEBUG_PLUGINS, "Index: %d", index);

	insert_tag ((Tag*)g_list_nth_data (tag_list_window->selected_tag_group->tags, index),
		    TRUE);
}

static gboolean 
tag_list_key_press_event_cb (GtkTreeView *tag_list, GdkEventKey *event)
{
	if (event->keyval == GDK_Return)
	{
		GtkTreeModel *model;
		GtkTreeSelection *selection;
		GtkTreeIter iter;
		gint index;

		gedit_debug (DEBUG_PLUGINS, "RETURN Pressed");

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (tag_list_window->tags_list));
		g_return_val_if_fail (model != NULL, FALSE);

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tag_list_window->tags_list));
		g_return_val_if_fail (selection != NULL, FALSE);

		if (gtk_tree_selection_get_selected (selection, NULL, &iter))
		{
			gtk_tree_model_get (model, &iter, COLUMN_TAG_INDEX_IN_GROUP, &index, -1);

			gedit_debug (DEBUG_PLUGINS, "Index: %d", index);

			insert_tag ((Tag*)g_list_nth_data (tag_list_window->selected_tag_group->tags, index),
				    event->state & GDK_CONTROL_MASK);
		}

		return FALSE;
	}

	return FALSE;
}

static void
insert_tag (Tag *tag, gboolean focus_to_document)
{
	GeditView *view;
	GeditDocument *doc;
	gint cursor;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	view = gedit_get_active_view ();
	if (view == NULL)
		return;

	gtk_window_set_transient_for (tag_list_window->window,
			GTK_WINDOW (gedit_get_active_window ()));
			
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);
	
	gedit_document_begin_user_action (doc);
	
	if (tag->begin != NULL)
		gedit_document_insert_text_at_cursor (doc, tag->begin, -1);
	
	cursor = gedit_document_get_cursor (doc);
	
	if (tag->end != NULL)
		gedit_document_insert_text_at_cursor (doc, tag->end, -1);
	
	gedit_document_set_cursor (doc, cursor);
	 
	gedit_document_end_user_action (doc);

	if (focus_to_document)
	{
		gtk_window_present (GTK_WINDOW (gedit_get_active_window ()));
		gtk_widget_grab_focus (GTK_WIDGET (view));
	}
}

static gboolean 
tag_list_window_key_press_event_cb (GtkTreeView *tag_list, GdkEventKey *event)
{
	if ((event->keyval == 'w') && (event->state & GDK_CONTROL_MASK))
	{
		taglist_window_close ();
		return TRUE;
	}

	if (event->keyval == GDK_F1)
	{
		GError *error = NULL;

		gedit_debug (DEBUG_PLUGINS, "F1 Pressed");

		gnome_help_display ("gedit.xml", "gedit-use-plugins", &error);
	
		if (error != NULL)
		{
			g_warning (error->message);
	
			g_error_free (error);
		}

		return FALSE;
	}	

	return FALSE;
}

