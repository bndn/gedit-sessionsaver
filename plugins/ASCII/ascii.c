/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * docinfo.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2001-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-help.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-utils.h>

#define MENU_ITEM_LABEL		N_("_ASCII Table")
#define MENU_ITEM_PATH		"/menu/View/ViewOps/"
#define MENU_ITEM_NAME		"ASCIITablePlugin"	
#define MENU_ITEM_TIP		N_("Pop-up a dialog containing an ASCII Table")

enum {
	CHAR_COLUMN,
	DEC_COLUMN,
	HEX_COLUMN,
	NAME_COLUMN,
	INDEX_COLUMN,
	NUM_COLUMNS
};

static char *names[33] = { 
	"NUL", 
	"SOH",
	"STX",
	"ETX",
	"EOT",
	"ENQ",
	"ACK",
	"BEL",
	"BS",
	"HT",
	"LF",
	"VT",
	"FF",
	"CR",
	"SO",
	"SI",
	"DLE",
	"DC1",
	"DC2",
	"DC3",
	"DC4",
	"NAK",
	"SYN",
	"ETB",
	"CAN",
	"EM",
	"SUB",
	"ESC",
	"FS",
	"GS",
	"RS",
	"US",
	"SPACE"
	};

typedef struct _ASCIITableDialog ASCIITableDialog;

struct _ASCIITableDialog {
	GtkWidget *dialog;

	GtkWidget *ascii_table;
};

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static ASCIITableDialog *get_dialog (void);
static void dialog_response_handler (GtkDialog *dlg, gint res_id,  ASCIITableDialog *dialog);
static void insert_char (gint i);
static void ascii_table_real (void);
static GtkTreeModel *create_model (void);
static void create_ASCII_table_list (ASCIITableDialog *dialog);
static void ASCII_table_row_activated_cb (GtkTreeView *ascii_table, GtkTreePath *path,
		  GtkTreeViewColumn *column, gpointer data);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);


static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  ASCIITableDialog *dialog)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gint index;
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->ascii_table));
			g_return_if_fail (model != NULL);

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->ascii_table));
			g_return_if_fail (selection != NULL);

			if (gtk_tree_selection_get_selected (selection, NULL, &iter))
			{
				gtk_tree_model_get (model, &iter, INDEX_COLUMN, &index, -1);

				gedit_debug (DEBUG_PLUGINS, "Index: %d", index);

				insert_char (index);
			}

			break;

		case GTK_RESPONSE_HELP:
			/* FIXME: Choose a better link id */
			gnome_help_display ("gedit.xml", "gedit-use-plugins", &error);
	
			if (error != NULL)
			{
				g_warning (error->message);

				g_error_free (error);
			}
			
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}


static void
insert_char (gint i)
{
	GeditDocument *doc;
	gchar *ch_utf8;
	
	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_get_active_document ();

	if (doc == NULL)
	     return;

	g_return_if_fail ((i >=0) && (i <= 0x7f));

	ch_utf8 = g_strdup_printf ("%c", i);
	
	gedit_document_insert_text_at_cursor (doc, ch_utf8, -1);	
	g_free (ch_utf8);
}

static GtkTreeModel*
create_model (void)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeIter iter;
	
	gchar ch[5];
	gchar dec[5];
	gchar hex[5];
	gchar *name;
	
	gedit_debug (DEBUG_PLUGINS, "");

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, 
				    G_TYPE_STRING, 
				    G_TYPE_STRING, 
				    G_TYPE_STRING, 
				    G_TYPE_STRING, 
				    G_TYPE_INT);

	/* add data to the list store */
	for (i = 0; i <= 0x7f; ++i)
	{
		if ((i < 0x21) || (i == 0x7f))
			sprintf (ch, "    ");
		else
			sprintf (ch, "   %c", i);
		
		sprintf (dec, "%3d", i);
		sprintf (hex, "%2.2X", i);

		if (i < 0x21)
			name = names[i];
		else
		{
			if (i == 0x7f)
				name = "DEL";
			else
				name = "";
		}
			
		gtk_list_store_append (store, &iter);
		
		gtk_list_store_set (store, &iter,
				    CHAR_COLUMN, ch,
				    DEC_COLUMN, dec,
				    HEX_COLUMN, hex,
				    NAME_COLUMN, name,
				    INDEX_COLUMN, i,
				    -1);

	}
	
	return GTK_TREE_MODEL (store);
}

static void
ASCII_table_row_activated_cb (GtkTreeView *ascii_table, GtkTreePath *path,
		  GtkTreeViewColumn *column, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint index;
	
	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ascii_table));
	g_return_if_fail (model != NULL);
	
	gtk_tree_model_get_iter (model, &iter, path);
	g_return_if_fail (&iter != NULL);

	gtk_tree_model_get (model, &iter, INDEX_COLUMN, &index, -1);

	gedit_debug (DEBUG_PLUGINS, "Index: %d", index);

	insert_char (index);
}

static void
create_ASCII_table_list (ASCIITableDialog *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (dialog != NULL);

	model = create_model ();

	/* Set tree view model*/
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->ascii_table), model);

	g_object_unref (G_OBJECT (model));
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dialog->ascii_table), TRUE);
	
	/* the Char formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Char"), cell, 
			"text", CHAR_COLUMN, NULL);
	gtk_tree_view_column_set_min_width (column, 60);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->ascii_table), column);

	/* the Dec# formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Dec#"), cell, 
			"text", DEC_COLUMN, NULL);
	gtk_tree_view_column_set_min_width (column, 60);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->ascii_table), column);

	/* the Hex# formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Hex#"), cell, 
			"text", HEX_COLUMN, NULL);
	gtk_tree_view_column_set_min_width (column, 60);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->ascii_table), column);

	/* the Name formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"), cell, 
			"text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_min_width (column, 60);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->ascii_table), column);

	g_signal_connect_after (G_OBJECT (dialog->ascii_table), "row_activated",
				G_CALLBACK (ASCII_table_row_activated_cb), NULL);

	gtk_widget_show (dialog->ascii_table);
}

static ASCIITableDialog*
get_dialog (void)
{
	static ASCIITableDialog *dialog = NULL;

	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				window);
	
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "asciitable.glade2",
			     "asciitable_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find asciitable.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (ASCIITableDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("ASCII Table"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add the update button */
	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("_Insert char"), GTK_STOCK_ADD, GTK_RESPONSE_OK);

	content			= glade_xml_get_widget (gui, "asciitable_dialog_content");
	g_return_val_if_fail (content != NULL, NULL);

	dialog->ascii_table 	= glade_xml_get_widget (gui, "ascii_table");
	g_return_val_if_fail (dialog->ascii_table  != NULL, NULL);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	create_ASCII_table_list (dialog);
	
	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			   G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			   G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	gtk_widget_show (dialog->dialog);

	return dialog;
}

static void
ascii_table_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	ascii_table_real ();
}

static void
ascii_table_real (void)
{
	ASCIITableDialog *dialog;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();
	if (dialog == NULL) 
	{
		g_warning ("Could not create the ASCII Table");
		return;
	}

}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	
	gedit_debug (DEBUG_PLUGINS, "");	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
        gedit_debug (DEBUG_PLUGINS, "");

        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP, NULL,
				     ascii_table_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
		
	return PLUGIN_OK;
}

