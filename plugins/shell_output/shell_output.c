/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * shell_output.c
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

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-vfs-extensions.h>
#include <libgnomeui/gnome-entry.h>
#include <libgnomeui/gnome-file-entry.h>

#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-file.h>
#include <gedit/gedit-mdi.h>
#include <gedit/gedit-output-window.h>

#define MENU_ITEM_LABEL		N_("_Run Command...")
#define MENU_ITEM_PATH		"/menu/Tools/ToolsOps_3/"
#define MENU_ITEM_NAME		"PluginShellOutput"	
#define MENU_ITEM_TIP		N_("Run a command")

typedef struct _ShellOutputDialog ShellOutputDialog;

typedef enum {
	NOT_RUNNING,
	RUNNING,
	MAKE_IT_STOP,
	MAKE_IT_CLOSE
} RunStatus;

struct _ShellOutputDialog {
	GtkWidget *dialog;

	GtkWidget *command;
	GtkWidget *command_list;
	GtkWidget *command_label;
	GtkWidget *directory_label;
	GtkWidget *directory;
	GtkWidget *directory_fileentry;
	GtkWidget *capture_output;

	GtkWidget *run_button;
	GtkWidget *stop_button;
	GtkWidget *close_button;

	GtkWindow *toplevel_window;
	GtkWidget *output_window;

	gchar *command_line;
	gint child_pid;
	GIOChannel *ioc_stdout;
	GIOChannel *ioc_stderr;

	gboolean 	is_capturing_output;
	gboolean	failed;
};


enum
{
	GEDIT_RESPONSE_STOP = 100
};

static RunStatus running;

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static ShellOutputDialog *get_dialog ();
static void dialog_response_handler (GtkDialog *dlg, gint res_id,  ShellOutputDialog *dialog);

static void	run_command_cb (BonoboUIComponent *uic, gpointer user_data, 
			       const gchar* verbname);
static gboolean	run_command_real (ShellOutputDialog *dialog);

#if 0
static void	stop_command (GIOChannel *ioc);
#endif

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);

static gchar *current_directory = NULL;


static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		ShellOutputDialog *dialog = (ShellOutputDialog*)*dialog_pointer;

		if ((running == RUNNING) && dialog->is_capturing_output)
		{	
			running = MAKE_IT_CLOSE;
		
			/*
			g_print ("Kill Child: %d\n", dialog->child_pid);
			*/

			kill (dialog->child_pid, SIGKILL);	
			wait (NULL);

			/*
			g_print ("Killed Child: %d\n", dialog->child_pid);
			*/
		}

		if (dialog->is_capturing_output)
		{		
			if (running == MAKE_IT_CLOSE)
			{
				gchar *line;

				if (GEDIT_IS_OUTPUT_WINDOW (dialog->output_window))
					gedit_output_window_append_line (
						GEDIT_OUTPUT_WINDOW (dialog->output_window), "", TRUE);

				line = g_strdup_printf ("<i>%s</i>.", _("Stopped"));
				
				if (GEDIT_IS_OUTPUT_WINDOW (dialog->output_window))
					gedit_output_window_append_line (
						GEDIT_OUTPUT_WINDOW (dialog->output_window), line, TRUE);

				g_free (line);
			}
		}

		g_free (dialog->command_line);
		
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  ShellOutputDialog *dialog)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			run_command_real (dialog);
			
			break;
			
		case GTK_RESPONSE_HELP:
			/* FIXME: choose a better link id */
			gnome_help_display ("gedit.xml", "gedit-pipe-output", &error);
	
			if (error != NULL)
			{
				g_warning (error->message);

				g_error_free (error);
			}

			break;
			
		case GEDIT_RESPONSE_STOP:
			g_return_if_fail (running == RUNNING);
				
			running = MAKE_IT_STOP;

			gtk_widget_set_sensitive (dialog->stop_button, FALSE);

			/*
			g_print ("Kill Child: %d\n", dialog->child_pid);
			*/

			kill (dialog->child_pid, SIGKILL);
			wait (NULL);
			
			/*
			g_print ("Killed: %d\n", dialog->child_pid);
			*/

			break;

		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_NONE:
			break;

		default:
			gtk_widget_destroy (dialog->dialog);

			break;

		}
}

static void
display_error_dialog (GtkWindow *parent)
{
	GtkWidget *err_dialog;
	
	err_dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			_("An error occurs while running the selected command."));
			
	gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (err_dialog));
	gtk_widget_destroy (err_dialog);
}

static ShellOutputDialog*
get_dialog (void)
{
	static ShellOutputDialog *dialog = NULL;

	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				window);

		dialog->toplevel_window = window;
		
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		gtk_widget_grab_focus (dialog->command);

		if (!GTK_WIDGET_VISIBLE (dialog->dialog))
			gtk_widget_show (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "shell_output.glade2",
			     "shell_output_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find shell_output.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (ShellOutputDialog, 1);

	running = NOT_RUNNING;
	dialog->failed = FALSE;

	dialog->toplevel_window = window;


	dialog->dialog = gtk_dialog_new_with_buttons (_("Run Command"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add the run button */
	dialog->close_button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	dialog->stop_button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
					GTK_STOCK_STOP, GEDIT_RESPONSE_STOP);
	gtk_widget_hide (dialog->stop_button);
	dialog->run_button =  gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("_Run"), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK);

	content			= glade_xml_get_widget (gui, "shell_output_dialog_content");

	g_return_val_if_fail (content != NULL, NULL);

	dialog->command    	= glade_xml_get_widget (gui, "command_entry");
	dialog->command_list   	= glade_xml_get_widget (gui, "command_entry_list");

	dialog->directory  	= glade_xml_get_widget (gui, "directory_entry");
	dialog->directory_fileentry = glade_xml_get_widget (gui, "directory_fileentry");
	dialog->capture_output  = glade_xml_get_widget (gui, "capture_ouput_checkbutton");

	dialog->command_label  	= glade_xml_get_widget (gui, "command_label");
	dialog->directory_label	= glade_xml_get_widget (gui, "directory_label");

	g_return_val_if_fail (dialog->command != NULL, NULL);
	g_return_val_if_fail (dialog->command_label != NULL, NULL);
	g_return_val_if_fail (dialog->command_list != NULL, NULL);
	g_return_val_if_fail (dialog->directory != NULL, NULL);
	g_return_val_if_fail (dialog->directory_fileentry != NULL, NULL);
	g_return_val_if_fail (dialog->directory_label != NULL, NULL);
	g_return_val_if_fail (dialog->capture_output != NULL, NULL);

	gtk_entry_set_text (GTK_ENTRY (dialog->directory), current_directory);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			   G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			   G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	gtk_widget_grab_focus (dialog->command);

	gtk_widget_show (dialog->dialog);

	return dialog;

}

static void
run_command_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	ShellOutputDialog *dialog;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Shell Output dialog");
		return;
	}
}


static gboolean
handle_command_output (GIOChannel *ioc, GIOCondition condition, gpointer data) 
{
	gboolean not_running = FALSE;

	ShellOutputDialog *dialog = (ShellOutputDialog *)data;

	gedit_debug (DEBUG_PLUGINS, "");

	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GError	*error = NULL;
		gchar   *string = NULL;
		gint	 len = 0;
		gint	 pos = 0;
		gchar 	*line;
	
		gedit_debug (DEBUG_PLUGINS, "1");
	
		if (!ioc->is_readable)
			return TRUE;

		gedit_debug (DEBUG_PLUGINS, "1.1");

		do 
		{
			gint  	 status;
			gchar 	*p;

			
			if (running != RUNNING) 
			{
				not_running = TRUE;
				break;
			}
				
			gedit_debug (DEBUG_PLUGINS, "1.2");
		
			do 
			{
				status = g_io_channel_read_line (ioc, &string, &len, &pos, &error);
				
				while (gtk_events_pending ()) 
				{
					if (running == MAKE_IT_CLOSE) 
						return FALSE;
						
					gtk_main_iteration (); 				
				}

				if (running == MAKE_IT_CLOSE) 
					return FALSE;

				
			} while (status == G_IO_STATUS_AGAIN);
			
			if (status != G_IO_STATUS_NORMAL) 
			{
				gedit_debug (DEBUG_PLUGINS, "1.2.1");

				if (error != NULL) 
				{
					g_warning ("Error reading stdout: %s", error->message);
					g_error_free (error);

					dialog->failed = TRUE;
				}
				
				continue;
			}		
			
			if (len <= 0)
				continue;
				
			gedit_debug (DEBUG_PLUGINS, "1.3");

			p = g_utf8_offset_to_pointer (string, pos);

			*p = '\0';
	
			line = g_markup_escape_text (string, -1);

			if (ioc == dialog->ioc_stdout)	
			{
				gedit_output_window_append_line (
						GEDIT_OUTPUT_WINDOW (dialog->output_window), line, TRUE);
			}
			else
			{
				gchar *markup;
				
				markup = g_strdup_printf ("<span foreground=\"red\">%s</span>", line);

				gedit_output_window_append_line (
						GEDIT_OUTPUT_WINDOW (dialog->output_window), markup, TRUE);
				
				g_free (markup);
			}
			
			g_free (line);				
			while (gtk_events_pending ()) 
			{
				if (running == MAKE_IT_CLOSE)
					return FALSE;
				
				gtk_main_iteration (); 				
			}

			if (running == MAKE_IT_CLOSE) 
				return FALSE;

			g_free (string);
						
		} while (g_io_channel_get_buffer_condition (ioc) == G_IO_IN);
		
	}

	if ((condition != G_IO_IN) || not_running) 
	{	
		gchar *line;
		gboolean done = FALSE;
		
		gedit_debug (DEBUG_PLUGINS, "2");

		g_io_channel_shutdown (ioc, TRUE, NULL);

		if (ioc != dialog->ioc_stdout)
			return FALSE;

		if (running == MAKE_IT_CLOSE)
			return FALSE;
	
		if ((running == MAKE_IT_STOP))
			line = g_strdup_printf ("<i>%s</i>.", _("Stopped"));
		else
		{
			done = TRUE;
			
			if (!dialog->failed)
				line = g_strdup_printf ("<i>%s</i>.", _("Done"));
			else
				line = g_strdup_printf ("<i>%s</i>.", _("Failed"));
		}
	
		gedit_output_window_append_line (GEDIT_OUTPUT_WINDOW (dialog->output_window), "", TRUE);

		gedit_output_window_append_line (GEDIT_OUTPUT_WINDOW (dialog->output_window), line, TRUE);
		
		g_free (line);

		while (gtk_events_pending ()) 
		{
			if (running == MAKE_IT_CLOSE)
				return FALSE;
				
			gtk_main_iteration (); 				
		}

		if (running == MAKE_IT_CLOSE)
			return FALSE;

		if (done)
			running = NOT_RUNNING;
	
		gtk_widget_destroy (dialog->dialog);
		
		return FALSE;
	}

	return TRUE;
}

static gchar *
unescape_command_string_real (const gchar *text, GeditDocument *doc)
{
	GString *str;
	gint length;
	gboolean drop_prev = FALSE;
	const gchar *cur;
	const gchar *end;
	const gchar *prev;

	gchar *filename = NULL;
	gchar *basename = NULL;

	gchar *tmp;

	g_return_val_if_fail (text != NULL, NULL);

	length = strlen (text);

	str = g_string_new ("");

	cur = text;
	end = text + length;
	prev = NULL;
	while (cur != end) {
		const gchar *next;
		next = g_utf8_next_char (cur);

		if (prev && (*prev == '%')) {
			switch (*cur) {
				case 'f':
				case 'F':
					if (filename == NULL)
					{
						tmp = gedit_document_get_raw_uri (doc);
						
						if (tmp != NULL)
						{
							filename = gnome_vfs_get_local_path_from_uri (tmp);

							if (filename == NULL)
								filename = tmp;
							else
								g_free (tmp);
						}
					}

					if (filename != NULL)
					{
						gchar *qf;

						qf = g_shell_quote (filename);
						str = g_string_append (str, qf);
						g_free (qf);
					}
				break;
				case 'n':
				case 'N':
					if (basename == NULL)
					{
						tmp = gedit_document_get_raw_uri (doc);
						
						if (tmp != NULL)
						{
							basename = eel_uri_get_basename (tmp);
							g_free (tmp);
						}
					}

					if (basename != NULL)
					{
						gchar *qf;

						qf = g_shell_quote (basename);
						str = g_string_append (str, qf);
						g_free (qf);
					}
				break;
				case '%':
					str = g_string_append (str, "%");
					drop_prev = TRUE;
				break;
				default:
					str = g_string_append (str, "%");
					str = g_string_append (str, cur);
				break;
			}
		} else if (*cur != '%') {
			str = g_string_append_len (str, cur, next - cur);
		}

		if (!drop_prev)
			prev = cur;
		else {
			prev = NULL;
			drop_prev = FALSE;
		}

		cur = next;
	}

	g_free (filename);
	g_free (basename);

	return g_string_free (str, FALSE);
}

static gchar *
unescape_command_string (const gchar *text, GeditDocument *doc)
{
	return unescape_command_string_real (text, doc);
}

static gboolean
run_command_real (ShellOutputDialog *dialog)
{
	const gchar *command_string   = NULL ;
	const gchar *directory_string = NULL ;
	gchar *unescaped_command_string = NULL;

	gboolean retval;
	gchar **argv = 0;
	gint standard_output;
	gint standard_error;
	gboolean capture_output;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (dialog != NULL, FALSE);

	command_string = gtk_entry_get_text (GTK_ENTRY (dialog->command));

	if (command_string == NULL || (strlen (command_string) == 0))
	{
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				_("The shell command entry is empty.\n\n"
				  "Please, insert a valid shell command."));
			
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (err_dialog));
  		gtk_widget_destroy (err_dialog);

		return FALSE;
	}

	directory_string = gtk_entry_get_text (GTK_ENTRY (dialog->directory));
	
	if (directory_string == NULL || (strlen (directory_string) == 0)) 
		directory_string = current_directory;

	unescaped_command_string = unescape_command_string (command_string, 
							    gedit_get_active_document ());
	g_return_val_if_fail (unescaped_command_string != NULL, FALSE);

	if (!g_shell_parse_argv (unescaped_command_string,
                           NULL, &argv,
                           NULL))
	{
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				_("Error parsing the shell command.\n\n"
				  "Please, insert a valid shell command."));
			
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (err_dialog));
  		gtk_widget_destroy (err_dialog);

		g_free (unescaped_command_string);
		
		return FALSE;
	}
	
	capture_output = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->capture_output));
	
	gtk_widget_set_sensitive (dialog->command, FALSE);
	gtk_widget_set_sensitive (dialog->command_list, FALSE);
	gtk_widget_set_sensitive (dialog->command_label, FALSE);
	gtk_widget_set_sensitive (dialog->directory_label, FALSE);
	gtk_widget_set_sensitive (dialog->directory_fileentry, FALSE);
	gtk_widget_set_sensitive (dialog->capture_output, FALSE);

	gtk_widget_set_sensitive (dialog->close_button, FALSE);

	gtk_widget_show (dialog->stop_button);
	gtk_widget_hide (dialog->run_button);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_STOP);

	if (capture_output)
		retval = g_spawn_async_with_pipes (directory_string,
				argv,
				NULL,
				G_SPAWN_SEARCH_PATH,
				NULL,
				NULL,
				&dialog->child_pid,
				NULL,
				&standard_output,
				&standard_error,
				NULL);
	else
		retval = g_spawn_async (directory_string,
				argv,
				NULL,
				G_SPAWN_SEARCH_PATH | 
				G_SPAWN_STDERR_TO_DEV_NULL | 
				G_SPAWN_STDOUT_TO_DEV_NULL,
				NULL,
				NULL,
				&dialog->child_pid,
				NULL);

	g_strfreev (argv);

	/*
	g_print ("Child pid: %d\n", dialog->child_pid);
	*/

	if (retval)
	{	
		running = RUNNING;

		dialog->is_capturing_output = capture_output;

		if (capture_output)
		{
			GIOChannel *ioc_stdout;
			GIOChannel *ioc_stderr;
			const gchar *encoding = NULL;
	
			gchar *markup;
			gchar *line;
		
			dialog->command_line = g_strdup (unescaped_command_string);
	
			dialog->output_window = 
				gedit_mdi_get_output_window_from_window (
						BONOBO_WINDOW (dialog->toplevel_window));
			gtk_widget_show (dialog->output_window);
			gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (dialog->output_window));

			line = g_markup_escape_text (dialog->command_line, -1);
			markup = g_strdup_printf ("<i>%s</i>: <b>%s</b>", _("Executing command"), line);
			gedit_output_window_append_line (GEDIT_OUTPUT_WINDOW (dialog->output_window), 
							 markup,
							 TRUE);
			gedit_output_window_append_line (GEDIT_OUTPUT_WINDOW (dialog->output_window), 
							 "",
							 TRUE);
		
			g_free (line);
			g_free (markup);

			ioc_stdout = g_io_channel_unix_new (standard_output);
			ioc_stderr =  g_io_channel_unix_new (standard_error);

			dialog->ioc_stdout = ioc_stdout;
			dialog->ioc_stderr = ioc_stderr;

			g_get_charset (&encoding);
			
			/* FIXME: check erros */
			g_io_channel_set_encoding (ioc_stdout, encoding, NULL);
			g_io_channel_set_encoding (ioc_stderr, encoding, NULL);

			g_io_channel_set_flags (ioc_stdout, G_IO_FLAG_NONBLOCK, NULL);
			g_io_channel_set_flags (ioc_stderr, G_IO_FLAG_NONBLOCK, NULL);

			g_io_add_watch (ioc_stdout,
					G_IO_IN | G_IO_HUP,
				    	handle_command_output,
			    		dialog);

			g_io_add_watch (ioc_stderr,
					G_IO_IN | G_IO_HUP,
				    	handle_command_output,
			    		dialog);


			g_io_channel_unref (ioc_stdout);
			g_io_channel_unref (ioc_stderr);
		}
		
		/* Save command and directory history */
		gnome_entry_prepend_history (GNOME_ENTRY (dialog->command_list),
				TRUE, command_string);
		
		if (directory_string != current_directory)
		{
			gnome_entry_prepend_history (GNOME_ENTRY (
					gnome_file_entry_gnome_entry (
						GNOME_FILE_ENTRY (dialog->directory_fileentry))),
				TRUE, directory_string);

			g_free (current_directory);
			current_directory = g_strdup (directory_string);
		}

		g_free (unescaped_command_string);
	
		if (!capture_output)
			gtk_widget_destroy (dialog->dialog);

		return TRUE;
	}
	else
	{
		running = NOT_RUNNING;

		display_error_dialog (GTK_WINDOW (dialog->dialog));
		
		g_free (unescaped_command_string);

		gtk_widget_destroy (dialog->dialog);

		return FALSE;
	}
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_free (current_directory);
	current_directory = NULL;

	return PLUGIN_OK;
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
				     MENU_ITEM_LABEL, MENU_ITEM_TIP,
				     GTK_STOCK_EXECUTE, run_command_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
	
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	pd->private_data = NULL;

	current_directory = g_get_current_dir ();
		
	return PLUGIN_OK;
}
