/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * cvschangelog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 James Willcox
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * AUTHORS :
 *   James Willcox <jwillcox@cs.indiana.edu>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-i18n.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-file.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-utils.h>

#define GEDIT_CVSCHANGELOG_PLUGIN_PATH_STRING_SIZE 1

/* AFAIK, all cvs commit messages start with this */
#define GEDIT_CVSCHANGELOG_START_HEADER "\nCVS: ------"


#define MENU_ITEM_LABEL		N_("Open CVS Chan_geLogs")
#define MENU_ITEM_PATH		"/menu/File/FileOps_2/"
#define MENU_ITEM_NAME		"CVSChangeLog"	
#define MENU_ITEM_TIP		N_("Searches for ChangeLogs in the current document and opens them")

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);


static gchar *
get_cwd (void)
{
	gint bufsize = GEDIT_CVSCHANGELOG_PLUGIN_PATH_STRING_SIZE * sizeof (gchar);
	gchar *buf = g_malloc (bufsize);
	
	while (!(getcwd (buf, bufsize))) {
		if (errno == ERANGE) {
			bufsize = bufsize * 2;
			buf = g_realloc (buf, bufsize);
		} else {
			/* what the heck happened? */
			g_free(buf);
			return NULL;
		}
	}
	
	return buf;
}

static gint
is_changelog (const gchar * path)
{
	gboolean ret = FALSE;
	gchar *base;

	base = g_path_get_basename (path);

	if (g_utf8_caselessnmatch (base, "changelog",
				   strlen (base), strlen ("changelog")))
		ret = TRUE;

	g_free (base);

	return ret;
}


static GList *
get_changelogs (const gchar *buf)
{
	GList *list=NULL;
	guchar *filename;
	gchar *current_dir;
	gchar *uri;
	gchar *local_path;
	gchar **split_file;
	gchar **split_line;
	gint i, j;

	/* split the file up line by line */
	split_file = g_strsplit (buf, "\n", 0);

	for (i = 0; split_file[i]; i++) {
		/* now split the line up by spaces */
		split_line = g_strsplit (split_file[i], " ", 0);

		for (j = 0; split_line[j]; j++) {
			filename = g_strstrip (split_line[j]);

			if (filename[strlen (filename) - 1] == '\n')
				filename[strlen (filename) - 1] = '\0';

			if (is_changelog (filename)) {
				current_dir = get_cwd();
				
				if(!current_dir)
					continue;

				local_path = g_strdup_printf("%s/%s", current_dir, filename);

				uri = gnome_vfs_get_uri_from_local_path (local_path);
				list = g_list_append (list, uri);

				/* cleanup. */
				g_free(current_dir);
				g_free (local_path);
			}
		}

		g_strfreev (split_line);
	}

	g_strfreev (split_file);

	return list;
}

static gboolean
is_commit_message (const char *buf)
{
	GList *list = NULL;

	if (!strncmp (buf, GEDIT_CVSCHANGELOG_START_HEADER,
		      strlen (GEDIT_CVSCHANGELOG_START_HEADER))) {
		list = get_changelogs (buf);
		if (list) {
			g_list_foreach (list, (GFunc)g_free, NULL);
			g_list_free (list);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void
cvs_changelogs_cb (BonoboUIComponent *uic, gpointer user_data, const gchar *verbname)
{
	GeditView *view = gedit_get_active_view ();
	GeditDocument *doc;
	gchar *buf;
	GList *list;
	GList *tmp;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (view);

	doc = gedit_view_get_document (view);

	buf = gedit_document_get_chars (doc, 0, -1);

	list = get_changelogs (buf);

	tmp = list;
	while (tmp) {
		gedit_file_open_single_uri ((gchar *)tmp->data, NULL);

		tmp = tmp->next;
	}

	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}


G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;
	gchar *buf;
	
	gedit_debug (DEBUG_PLUGINS, "");	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if (doc != NULL) {	 	
		buf = gedit_document_get_chars (doc, 0, -1);

		if (is_commit_message (buf))
			gedit_menus_set_verb_sensitive (uic,
							"/commands/" MENU_ITEM_NAME, TRUE);
		else
			gedit_menus_set_verb_sensitive (uic,
							"/commands/" MENU_ITEM_NAME, FALSE);
		g_free (buf);
	}
	else {
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
	}

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
				     GTK_STOCK_OPEN, cvs_changelogs_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}



G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *plugin)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin* plugin)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->private_data = NULL;

	return PLUGIN_OK;
}
