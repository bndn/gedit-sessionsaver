/* browse.c - web browse plugin.
 *
 * Copyright (C) 1998 Alex Roberts
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Converted to a gmodule plugin by Jason Leach <leach@wam.umd.edu>
 * Libgladify and better error recovery Chema Celorio <chema@celorio.com>
 *
 */


#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "window.h"
#include "document.h"
#include "plugin.h"
#include "view.h"
#include "utils.h"

#define GEDIT_PLUGIN_PROGRAM "lynx"
/* xgettext translators: !!!!!!!!!!!---------> the name of the plugin only.
   it is used to display "you can not use the [name] plugin without this program... */
#define GEDIT_PLUGIN_NAME  _("Browse")

static GtkWidget *location_label;
static GtkWidget *url_entry;

static void
gedit_plugin_destroy (PluginData *pd)
{
}

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG(widget));
}

/* WE need to make this plugin non-blocking and to improve the error
   reporting. !! It is very bad !
   Chema
*/
   
static void
gedit_plugin_execute (GtkWidget *widget, gint button, gpointer data)
{
	int fdpipe[2];
	gchar *url;
	GeditDocument *doc;
	guint length, pos;
	char buff[1025];
	int pid;
	gchar *lynx_location;

	if (button == 0)
	{
		url = g_strdup (gtk_entry_get_text (GTK_ENTRY (url_entry)));
		
		if (!url || strlen (url) == 0)
		{
			GnomeDialog *error_dialog;
			error_dialog = GNOME_DIALOG (gnome_error_dialog_parented ("Please provide a valid URL.",
								    gedit_window_active()));
			gnome_dialog_run_and_close (error_dialog);
			gdk_window_raise (widget->window);
			return;
		}
		
		lynx_location = g_strdup (GTK_LABEL(location_label)->label);
		
		if (pipe (fdpipe) == -1)
		{
			g_warning ("Could not open pipe\n");
			return;
		}
  
		pid = fork();
		if (pid == 0)
		{
			char *argv[4];
			
			close (1);
			dup (fdpipe[1]);
			close (fdpipe[0]);
			close (fdpipe[1]);
      
			argv[0] = "lynx";
			argv[1] = "-dump";
			argv[2] = url;
			argv[3] = NULL;
			execv (lynx_location, argv);

			/* don't use asserts not reached ! Chema.
			   why crash gedit when you can display an error
			   and give the user a chance to save it's work ?
			g_assert_not_reached ();
			*/
			g_warning ("A undetermined PIPE problem occurred");
			return;
		}
		close (fdpipe[1]);

		doc = gedit_document_new_with_title (url);

		length = 1;
		pos = 0;
		while (length > 0)
		{
			buff [ length = read (fdpipe[0], buff, 1024) ] = 0;
			if (length > 0)
			{
				gedit_document_insert_text (doc, buff, pos, FALSE);
				pos += length;
			}
		}

		gedit_view_set_position (gedit_view_active (), 0);
                /*
		  gnome_config_push_prefix ("/Editor_Plugins/Browse/");
		  gnome_config_set_string ("Url", url[0]);
		  gnome_config_pop_prefix ();
		  gnome_config_sync ();
		*/
		g_free (url);
		g_free (lynx_location);
	}
	else
	{
		if(button == 2)
		{
			static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html#browse" };
			gnome_help_display (NULL, &help_entry);

			return;
		}	
	}
	gnome_dialog_close (GNOME_DIALOG(widget));
}

static void
gedit_plugin_change_location (GtkWidget *button, gpointer userdata)
{
	GtkWidget *dialog;
	GtkWidget *label;
	gchar * new_location;

	gedit_debug (DEBUG_PLUGINS, "");
	dialog = userdata;

	new_location = gedit_plugin_program_location_change (GEDIT_PLUGIN_PROGRAM,
							     GEDIT_PLUGIN_NAME);

	if ( new_location == NULL)
	{
		gdk_window_raise (dialog->window);
		return;
	}

	/* We need to update the label */
	label  = gtk_object_get_data (GTK_OBJECT (dialog), "location_label");
	g_return_if_fail (label!=NULL);
	gtk_label_set_text (GTK_LABEL (label),
			    new_location);
	g_free (new_location);

	gdk_window_raise (dialog->window);	

}

static void
gedit_plugin_browse_create_dialog (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	GtkWidget *change_button;
	gchar *browser_location;

	gedit_debug (DEBUG_PLUGINS, "");
	
        /* xgettext translators: !!!!!!!!!!!---------> the name of the plugin only.
	 it is used to display "you can not use the [name] plugin without this program... */
	browser_location = gedit_plugin_program_location_get (GEDIT_PLUGIN_PROGRAM,
							      GEDIT_PLUGIN_NAME,
							      FALSE);
	
	g_return_if_fail(browser_location != NULL);
	    
	gui = glade_xml_new (GEDIT_GLADEDIR "/browse.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find %s", GEDIT_GLADEDIR "/browse.glade");
		return;
	}

	dialog         = glade_xml_get_widget (gui, "dialog");
	url_entry      = glade_xml_get_widget (gui, "url_entry");
	location_label = glade_xml_get_widget (gui, "location_label");
	change_button  = glade_xml_get_widget (gui, "change_button");

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (url_entry != NULL);
	g_return_if_fail (location_label != NULL);
	g_return_if_fail (change_button != NULL);
	
        /* Set the location label */
	gtk_object_set_data (GTK_OBJECT (dialog), "location_label", location_label);
	gtk_label_set_text (GTK_LABEL (location_label),
					browser_location);
	g_free (browser_location);

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_execute), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (change_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_change_location), dialog);

        /* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent 	(GNOME_DIALOG (dialog),	 gedit_window_active());
	gtk_window_set_modal 		(GTK_WINDOW (dialog), TRUE);
	gnome_dialog_set_default     	(GNOME_DIALOG (dialog), 0);
	gnome_dialog_editable_enters 	(GNOME_DIALOG (dialog), GTK_EDITABLE (url_entry));

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	pd->destroy_plugin = gedit_plugin_destroy;
	pd->name = _("Browse");
	pd->desc = _("Web browse plugin");
	pd->long_desc = _("Web browse plugin");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->needs_a_document = FALSE;

	pd->private_data = (gpointer)gedit_plugin_browse_create_dialog;
	pd->installed_by_default = TRUE;

	return PLUGIN_OK;
}
