/*
 * Sort plugin
 * Original author: Carlo Borreo borreo@softhome.net
 * Ported to Gedit2 by Lee Mallabone <gnome@fonicmonkey.net>
 *
 * Sorts the current document
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <string.h>
#include <glade/glade-xml.h>

#include <gedit/gedit-menus.h>
#include <gedit/gedit-document.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-plugin.h>

/* Key in case the plugin ever needs any settings */
#define SORT_BASE_KEY "/apps/gedit-2/plugins/sort"

#define MENU_ITEM_LABEL		N_("S_ort...")
#define MENU_ITEM_PATH		"/menu/Edit/EditOps_6/"
#define MENU_ITEM_NAME		"Sort"
#define MENU_ITEM_TIP		N_("Sort the current document or selection.")


typedef struct _SortDialog SortDialog;

struct _SortDialog {
	GtkWidget *dialog;

	GtkWidget *reverse_order_checkbutton;
	GtkWidget *ignore_case_checkbutton;
	GtkWidget *remove_dups_checkbutton;
	GtkWidget *col_num_spinbutton;
};

typedef struct _SortInfo SortInfo;

struct _SortInfo {
	gboolean ignore_case;
	gboolean reverse_order;
	gboolean remove_duplicates;

	gint starting_column;
};

static void dialog_destroyed (GtkObject * obj, void **dialog_pointer);
static SortDialog *get_dialog (void);
static void dialog_response_handler (GtkDialog * dlg,
				     gint res_id, SortDialog * dialog);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin * plugin,
					    BonoboWindow * window);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin * pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin * pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin * pd);

static void sort_document (SortDialog * dlg);

static void
dialog_destroyed (GtkObject * obj, void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL) {
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}
}

static void
dialog_response_handler (GtkDialog * dlg, gint res_id, SortDialog * dialog)
{
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
	case GTK_RESPONSE_OK:
		sort_document (dialog);
		gtk_widget_destroy (dialog->dialog);

		break;

	case GTK_RESPONSE_HELP:
		/* FIXME */
		gnome_help_display ("gedit.xml", "gedit-use-plugins",
				    &error);

		if (error != NULL) {
			g_warning (error->message);
			g_error_free (error);
		}

		break;
	default:
		gtk_widget_destroy (dialog->dialog);
	}
}


static SortDialog *
get_dialog ()
{
	static SortDialog *dialog = NULL;

	GladeXML *gui;
	GtkWindow *window;

	gedit_debug (DEBUG_PLUGINS, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL) {
		gtk_widget_grab_focus (dialog->dialog);

		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      window);

		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "sort.glade2",
			     "sort_dialog", NULL);

	if (!gui) {
		g_warning
		    ("Could not find sort.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (SortDialog, 1);

	/* Save some references to the main dialog widgets */
	dialog->dialog = glade_xml_get_widget (gui, "sort_dialog");
	dialog->reverse_order_checkbutton =
	    glade_xml_get_widget (gui, "reverse_order_checkbutton");
	dialog->col_num_spinbutton =
	    glade_xml_get_widget (gui, "col_num_spinbutton");
	dialog->ignore_case_checkbutton =
	    glade_xml_get_widget (gui, "ignore_case_checkbutton");
	dialog->remove_dups_checkbutton =
	    glade_xml_get_widget (gui, "remove_dups_checkbutton");

	g_return_val_if_fail (dialog->dialog, NULL);
	g_return_val_if_fail (dialog->reverse_order_checkbutton, NULL);
	g_return_val_if_fail (dialog->col_num_spinbutton, NULL);
	g_return_val_if_fail (dialog->ignore_case_checkbutton, NULL);
	g_return_val_if_fail (dialog->remove_dups_checkbutton, NULL);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_window_set_modal (GTK_WINDOW (dialog->dialog), TRUE);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      window);

	gtk_widget_show (dialog->dialog);

	return dialog;
}


static void
sort_cb (BonoboUIComponent * uic, gpointer user_data,
	 const gchar * verbname)
{
	SortDialog *dialog = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();

	if (dialog == NULL)
		g_warning ("Could not create the Word Count dialog");

}

/*
 * Compares two strings for the sorting algorithm. Uses the UTF-8 processing
 * functions in GLib to be as correct as possible.
 */
static int
my_compare (const void *s1, const void *s2, gpointer data)
{
	gint length1;
	gint length2; 
	gint ret;
	gchar *string1; 
	gchar *string2;
	gchar *substring1; 
	gchar *substring2;
	gchar *key1;
	gchar *key2;
	SortInfo *sort_info;
	
	gedit_debug (DEBUG_PLUGINS, "");

	sort_info = (SortInfo *)data;

	g_return_val_if_fail (sort_info != NULL, -1);

	if (!sort_info->ignore_case) {
		string1 = *((gchar **) s1);
		string2 = *((gchar **) s2);
	} else {
		/* If the user wants a case-insensitive sort, use the best functions
		 * GLib has for getting rid of locale-specific case. */
		string1 = g_utf8_casefold (*((gchar **) s1), -1);
		string2 = g_utf8_casefold (*((gchar **) s2), -1);
	}

	/* Figure out the UTF-8 character lengths */
	length1 = g_utf8_strlen (string1, -1);
	length2 = g_utf8_strlen (string2, -1);

	if ((length1 < sort_info->starting_column) &&
	    (length2 < sort_info->starting_column))
		ret = 0;
	else if (length1 < sort_info->starting_column)
		ret = -1;
	else if (length2 < sort_info->starting_column)
		ret = 1;

	/* Check the entire line of text */
	else if (sort_info->starting_column < 1) {
		key1 = g_utf8_collate_key (string1, -1);
		key2 = g_utf8_collate_key (string2, -1);
		
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	} else {
		/* A character column offset is required, so figure out 
		 * the correct offset into the UTF-8 string. */
		substring1 =
		    g_utf8_offset_to_pointer (string1,
					      sort_info->starting_column);
		substring2 =
		    g_utf8_offset_to_pointer (string2,
					      sort_info->starting_column);

		key1 = g_utf8_collate_key (substring1, -1);
		key2 = g_utf8_collate_key (substring2, -1);
		
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	}

	/* Do the necessary cleanup */
	if (sort_info->ignore_case) {
		g_free (string1);
		g_free (string2);
	}

	if (sort_info->reverse_order) {
		/* Reverse the order */
		ret = -1 * ret;
	}

	/*
	gedit_debug (DEBUG_PLUGINS, "\n%s\n%s\n%d\n\n", *((gchar **)s1), *((gchar **)s2), ret);
	*/

	return ret;
}

/* The function that actually does the work */

static void
sort_document (SortDialog * dlg)
{
	GeditDocument *doc;
	gchar *buffer;
	gchar *p;
	gint start;
	gint end;
	gunichar c;
	gchar *last_row = NULL;
	gint old_cur_pos;
	gpointer *lines;
	gint cont;
	SortInfo *sort_info;

	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_get_active_document ();
	if (doc == NULL)
		return;

	sort_info = g_new0 (SortInfo, 1);

	sort_info->ignore_case =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					  (dlg->
					   ignore_case_checkbutton));

	sort_info->reverse_order =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					   (dlg->reverse_order_checkbutton));

	sort_info->remove_duplicates =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					  (dlg->remove_dups_checkbutton));

	sort_info->starting_column =
	    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
					      (dlg->col_num_spinbutton)) - 1;

	if (!gedit_document_get_selection (doc, &start, &end)) {
		buffer = gedit_document_get_chars (doc, 0, -1);
		start = 0;
		end = -1;
	} else {
		buffer = gedit_document_get_chars (doc, start, end);
	}

	lines = g_new0 (gpointer, gedit_document_get_line_count (doc) + 1);

	gedit_debug (DEBUG_PLUGINS, "Building list...");

	cont = 0;

	p = buffer;
	c = g_utf8_get_char (p);

	while (c != '\0') {
		if (c == '\n') {
			gchar *old_p;
			
			old_p = p;

			p = g_utf8_next_char (p);

			*old_p = '\0';

			lines [cont] = p;
			++cont;
		} else
			p = g_utf8_next_char (p);

		c = g_utf8_get_char (p);
	}

	lines [cont] = buffer; 
	++cont;
	
	gedit_debug (DEBUG_PLUGINS, "Sort list...");

	g_qsort_with_data (lines, cont, sizeof (gpointer), my_compare, sort_info);

	gedit_debug (DEBUG_PLUGINS, "Rebuilding document...");

	old_cur_pos = gedit_document_get_cursor (doc);

	gedit_document_begin_not_undoable_action (doc);
	gedit_document_delete_text (doc, start, end);

	gedit_document_set_cursor (doc, start);

	cont = 0;

	while (lines [cont] != NULL) {
		gchar *current_row = lines [cont];

		/* Don't insert this row if it's the same as the last one
		 * and the user has specified to remove duplicates */
		if (!sort_info->remove_duplicates ||
		    last_row == NULL ||
		    (strcmp (last_row, current_row) != 0)) {
			gedit_document_insert_text_at_cursor (doc,
							      current_row,
							      -1);

			if (lines [cont + 1] != NULL)
				gedit_document_insert_text_at_cursor (doc,
								      "\n",
								      -1);
		}

		last_row = current_row;

		++cont;
	}

	gedit_document_set_cursor (doc, old_cur_pos);

	gedit_document_end_not_undoable_action (doc);

	g_free (lines);

	g_free (buffer);

	g_free (sort_info);

	gedit_debug (DEBUG_PLUGINS, "Done.");
}

/*
 * Activates the plugin. This adds a menu item onto the edit menu.
 * When clicked, the item will open a dialog window.
 */
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin * pd)
{
	GList *top_windows;

	gedit_debug (DEBUG_PLUGINS, "");
	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

	/* Add a menu item to each open window */
	while (top_windows) {
		gedit_menus_add_menu_item (BONOBO_WINDOW
					   (top_windows->data),
					   MENU_ITEM_PATH, MENU_ITEM_NAME,
					   MENU_ITEM_LABEL, MENU_ITEM_TIP,
					   GTK_STOCK_SORT_ASCENDING,
					   sort_cb);
		top_windows = g_list_next (top_windows);
	}

	return PLUGIN_OK;
}

/*
 * Removes any traces of the plugin from the window GUIs. Called when the
 * plugin is unloaded
 */
G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin * pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin * plugin, BonoboWindow * window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))
		gedit_menus_set_verb_sensitive (uic,
						"/commands/"
						MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (uic,
						"/commands/"
						MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}



/*
 * Initialise basic plugin information
 */
G_MODULE_EXPORT GeditPluginState
init (GeditPlugin * pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	pd->private_data = NULL;

	return PLUGIN_OK;
}
