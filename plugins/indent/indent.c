/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * indent.c
 * This file is part of gedit
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

#include <string.h> /* For strlen */

#include <glib/gutils.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock-icons.h>

#include <gedit/gedit-menus.h>
#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-prefs-manager.h>

#define MENU_INDENT_LABEL		N_("_Indent")
#define MENU_INDENT_NAME		"Indent"	
#define MENU_INDENT_TIP			N_("Indent selected lines")

#define MENU_UNINDENT_LABEL		N_("U_nindent")
#define MENU_UNINDENT_NAME		"Unindent"	
#define MENU_UNINDENT_TIP		N_("Unindent selected lines")

#define MENU_ITEM_PATH		"/menu/Edit/EditOps_5/"

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);


static void
indent_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditView *view;
	GtkTextIter start, end, iter;
	gint i, start_line, end_line;
	gchar *tab_buffer = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	gedit_document_begin_user_action (doc);
	
	gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
			
	start_line = gtk_text_iter_get_line (&start);
	end_line = gtk_text_iter_get_line (&end);

	if (gedit_prefs_manager_get_insert_spaces ())
	{
		gint tabs_size;

		tabs_size = gedit_prefs_manager_get_tabs_size ();
		tab_buffer = g_strnfill (tabs_size, ' ');
	} else {
		tab_buffer = g_strdup("\t");
	}

	for (i = start_line; i <= end_line; i++) 
	{
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc), &iter, i);
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc), &iter, tab_buffer, -1);
	}
	
	gedit_document_end_user_action (doc);
	g_free(tab_buffer);
}

static void
unindent_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditView *view;
	GtkTextIter start, end, iter, iter2;
	gint i, start_line, end_line;
	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	gedit_document_begin_user_action (doc);
	
	gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
			
	start_line = gtk_text_iter_get_line (&start);
	end_line = gtk_text_iter_get_line (&end);

	for (i = start_line; i <= end_line; i++) 
	{
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc), &iter, i);

		if (gtk_text_iter_get_char (&iter) == '\t') 
		{
			iter2 = iter;
			gtk_text_iter_forward_char (&iter2);
			gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &iter, &iter2);
		} 
		else if (gtk_text_iter_get_char (&iter) == ' ') 
		{
			int spaces = 0;

			iter2 = iter;

			while (!gtk_text_iter_ends_line (&iter2)) 
			{
				if (gtk_text_iter_get_char (&iter2) == ' ')
					spaces++;
				else
					break;
			
				gtk_text_iter_forward_char (&iter2);
			}
						
			if (spaces > 0)
			{
				int tabs = 0;
				int tabs_size = gedit_prefs_manager_get_tabs_size ();

				tabs = spaces / tabs_size;
				spaces = spaces - (tabs * tabs_size);
					
				if (spaces == 0)
					spaces = tabs_size;
				
				iter2 = iter;
				gtk_text_iter_forward_chars (&iter2, spaces);
				gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &iter, &iter2);
			}
		}

	}
	
	gedit_document_end_user_action (doc);
}


G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))
	{		
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_INDENT_NAME, FALSE);
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_UNINDENT_NAME, FALSE);
	}
	else
	{
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_INDENT_NAME, TRUE);
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_UNINDENT_NAME, TRUE);
	}

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
	BonoboUIComponent *ui_component;

        gedit_debug (DEBUG_PLUGINS, "");

        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_INDENT_NAME,
				     MENU_INDENT_LABEL, MENU_INDENT_TIP, GNOME_STOCK_TEXT_INDENT,
				     indent_cb);

		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_UNINDENT_NAME,
				     MENU_UNINDENT_LABEL, MENU_UNINDENT_TIP, GNOME_STOCK_TEXT_UNINDENT,
				     unindent_cb);

		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));

		bonobo_ui_component_set_prop (
			ui_component, "/commands/" MENU_INDENT_NAME, "accel", "*Ctrl*T", NULL);
		bonobo_ui_component_set_prop (
			ui_component, "/commands/" MENU_UNINDENT_NAME, "accel", "*Shift**Ctrl*T", NULL);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_INDENT_NAME);
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_UNINDENT_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}




