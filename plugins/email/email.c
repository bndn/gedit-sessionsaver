/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * email.c
 * This file is part of gedit
 *
 * Copyright (C) 2001 Alex Roberts  <bse@error.fsnet.co.uk>
 * 		      Chema Celorio <chema@celorio.com>
 * Copyright (C) 2002 James Willcox <jwillcox@cs.indiana.edu>
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
 * Modified by the gedit Team, 2000-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-file-entry.h>
#include <gconf/gconf-client.h>
#include <gnome.h>

#include <gedit-menus.h>
#include <gedit-plugin.h>
#include <gedit-utils.h>
#include <gedit-debug.h>
#include <gedit-file.h>
#include <dialogs/gedit-dialogs.h>

#define EMAIL_BASE_KEY 		"/apps/gedit-2/plugins/email"
#define EMAIL_LOCATION_KEY	"/sendmail-program-location"
#define EMAIL_PROGRAM_NAME      "sendmail"

#define MENU_ITEM_LABEL		N_("E_mail...")
#define MENU_ITEM_PATH		"/menu/File/FileOps_2/"
#define MENU_ITEM_NAME		"Email"	
#define MENU_ITEM_TIP		N_("Email a file.")

#define PLUGIN_NAME 		_("Email")

typedef struct _EmailDialog EmailDialog;

struct _EmailDialog {
	GtkWidget *dialog;

	GtkWidget *to_entry;
	GtkWidget *from_entry;
	GtkWidget *subject_entry;
};


G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState configure (GeditPlugin *p, GtkWidget *parent);
G_MODULE_EXPORT GeditPluginState save_settings (GeditPlugin *pd);

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static void email_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
static void error_dialog (const gchar* str, GtkWindow *parent);
static gboolean email_execute (EmailDialog *dialog);
static gboolean configure_real (GtkWindow *parent);
static void email_real (void);

static gchar* email_program_location = NULL;

static GConfClient 	*email_gconf_client 	= NULL;	

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}

	gedit_debug (DEBUG_PLUGINS, "END");	
}

static EmailDialog *
get_email_dialog (GtkWindow* parent)
{
	static EmailDialog *dialog = NULL;

	GladeXML *gui;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				parent);
		
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "email.glade",
			     "dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find email.glade, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (EmailDialog, 1);

	/* Create the dialog */
	dialog->dialog = gtk_dialog_new_with_buttons (_("Email current document..."),
						      parent,
						      GTK_DIALOG_DESTROY_WITH_PARENT |
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("Se_nd"), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK);

	/* Load widgets */
	content	= glade_xml_get_widget (gui, "dialog_content");
	dialog->from_entry = glade_xml_get_widget (gui, "from_entry");
	dialog->subject_entry = glade_xml_get_widget (gui, "subject_entry");
	dialog->to_entry = glade_xml_get_widget (gui, "to_entry");

	
	g_return_val_if_fail (content != NULL, NULL);
	

	g_return_val_if_fail (dialog->from_entry            != NULL, NULL);
	g_return_val_if_fail (dialog->to_entry              != NULL, NULL);
	g_return_val_if_fail (dialog->subject_entry         != NULL, NULL);

	/* Insert the content in the dialog */
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	/* Set default response */
	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	/* Connect destroy signal */
	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);
	
	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;

}

static void
email_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	email_real ();	
}

static void
email_real (void)
{
	GError *error = NULL;
	GtkWindow *parent;
	EmailDialog *dialog = NULL;
	gint ret;

	gedit_debug (DEBUG_PLUGINS, "");

	parent = GTK_WINDOW (gedit_get_active_window ());
	
	if (email_program_location  == NULL)
		if (!configure_real (parent))
			return;	

	dialog = get_email_dialog (parent);
	if (dialog == NULL) 
	{
		g_warning ("Could not create the Email dialog");
		return;
	}

	do 
	{
		ret = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (ret) {
			case GTK_RESPONSE_OK:

				if (email_execute (dialog))
					gtk_widget_hide (dialog->dialog);

			break;

			case GTK_RESPONSE_HELP:
				gnome_help_display ("gedit.xml", "gedit-use-plugins", &error);
				if (error != NULL) {
					g_warning (error->message);
					g_error_free (error);
				}
			break;

			default:
				gtk_widget_hide (dialog->dialog);
			break;
		}
	} while (GTK_WIDGET_VISIBLE (dialog->dialog));
}

static gboolean
email_execute (EmailDialog *dialog)
{
	GeditView *view;
	GeditDocument *doc;
	gchar *command_line = NULL;
	FILE *sendmail;

	const gchar *to;
	const gchar *from;
	const gchar *subject;
	const gchar *body;

	gedit_debug (DEBUG_PLUGINS, "");

	to = gtk_entry_get_text (GTK_ENTRY (dialog->to_entry));
	from = gtk_entry_get_text (GTK_ENTRY (dialog->from_entry));
	subject = gtk_entry_get_text (GTK_ENTRY (dialog->subject_entry));

	if (!strcmp (to, "")) {
		error_dialog (_("Error sending email.\n\n"
				"Please provide a recipient."),
				GTK_WINDOW (dialog->dialog));
		return FALSE;
	}
	if (!strcmp (from, "")) {
		error_dialog (_("Error sending email.\n\n"
				"Please provide a from address."),
				GTK_WINDOW (dialog->dialog));
		return FALSE;
	}
	if (!strcmp (subject, "")) {
		error_dialog (_("Error sending email.\n\n"
				"Please provide a subject."),
				GTK_WINDOW (dialog->dialog));
		return FALSE;
	}

	view = gedit_get_active_view ();
	g_return_val_if_fail (view != NULL, FALSE);
	
	doc = gedit_view_get_document (view);
	g_return_val_if_fail (doc != NULL, FALSE);

	body = gedit_document_get_chars (doc, 0, -1);

	command_line = g_strdup_printf ("%s -x -t", email_program_location);

	if ((sendmail = popen (command_line, "w")) == NULL) {
		error_dialog (_("Error sending email.\n\n"
				"Error executing the sendmail command."),
			      GTK_WINDOW (dialog->dialog));
	
		g_free (command_line);
		return FALSE;
	}

	fprintf (sendmail, "To: %s\n", to);
	fprintf (sendmail, "From: %s\n", from);
	fprintf (sendmail, "Subject: %s\n", subject);
	fprintf (sendmail, "X-Mailer: gedit email plugin %s\n", VERSION);
	fflush (sendmail);

	fprintf (sendmail, "%s", body);
	fflush (sendmail);
	pclose (sendmail);
	
	
	g_free (command_line);
	return TRUE;
}

static void 
error_dialog (const gchar* str, GtkWindow *parent)
{
	GtkWidget *message_dlg;

	gedit_debug (DEBUG_PLUGINS, "");

	message_dlg = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK, 
			str);
		
	gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
	
	gtk_dialog_run (GTK_DIALOG (message_dlg));
  	gtk_widget_destroy (message_dlg);
}

static gboolean
configure_real (GtkWindow *parent)
{
	gchar *temp;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	temp = gedit_plugin_program_location_dialog (EMAIL_PROGRAM_NAME, 
					      	     PLUGIN_NAME, 
					      	     parent);

	if (temp != NULL)
	{
		if (email_program_location != NULL)
			g_free (email_program_location);
		
		email_program_location = temp;
	}
	
	return (email_program_location != NULL);
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
	
	if (doc) {
		if (gedit_document_get_char_count (doc) > 0)
			gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);
		else
			gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
		
	}
	else
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
configure (GeditPlugin *p, GtkWidget *parent)
{
	if (configure_real (GTK_WINDOW (parent)))
		return PLUGIN_OK;
	else
		return PLUGIN_ERROR;	
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->deactivate (plugin);

	g_object_unref (G_OBJECT (email_gconf_client));

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
save_settings (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (email_gconf_client != NULL, PLUGIN_ERROR);

	if (email_program_location != NULL)
		gconf_client_set_string (
				email_gconf_client,
				EMAIL_BASE_KEY EMAIL_LOCATION_KEY,
				email_program_location,
		      		NULL);

	gconf_client_suggest_sync (email_gconf_client, NULL);

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
				     email_cb);

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
     
	pd->name = PLUGIN_NAME;
	pd->desc = _("Sends the current document via email.  You must have sendmail installed for this to work.");
	pd->author = "James Willcox <jwillcox@cs.indiana.edu>";
	pd->copyright = "Copyright \xc2\xa9 2000-2002 Alex Roberts, Chema Celorio, James Willcox";
	
	pd->private_data = NULL;

	email_gconf_client = gconf_client_get_default ();
	g_return_val_if_fail (email_gconf_client != NULL, PLUGIN_ERROR);

	gconf_client_add_dir (email_gconf_client,
			      EMAIL_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	
	email_program_location = gconf_client_get_string (
				email_gconf_client,
				EMAIL_BASE_KEY EMAIL_LOCATION_KEY,
			      	NULL);

	return PLUGIN_OK;
}




