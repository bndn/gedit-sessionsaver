/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-taglist-plugin.c
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

#include <string.h>

#include <libgnome/gnome-i18n.h>

#include <gedit/gedit-menus.h>
#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>

#include "gedit-taglist-plugin.h"
#include "gedit-taglist-plugin-window.h"
#include "gedit-taglist-plugin-parser.h"

#define MENU_ITEM_LABEL		N_("Tag _List")
#define MENU_ITEM_PATH		"/menu/View/ViewOps/"
/*
#define MENU_ITEM_NAME		"TagList"	
*/
#define MENU_ITEM_TIP		N_("Show the tag list window")

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);


static void
tag_list_cb (BonoboUIComponent           *ui_component,
	     const char                  *path,
	     Bonobo_UIComponent_EventType type,
	     const char                  *state,
	     gpointer               	  data)
{
	gboolean s;
	
	gedit_debug (DEBUG_PLUGINS, "%s toggled to '%s'", path, state);

	s = (strcmp (state, "1") == 0);

	if (s)
		taglist_window_show ();
	else
		taglist_window_close ();
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
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	free_taglist ();

	return PLUGIN_OK;
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
        gedit_debug (DEBUG_PLUGINS, "");

	if (taglist == NULL)
		if (create_taglist () == NULL)
			return PLUGIN_ERROR;
	
        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		BonoboUIComponent *ui_component;

		gedit_menus_add_menu_item_toggle (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP,
				     tag_list_cb, NULL);

		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));

		bonobo_ui_component_set_prop (
			ui_component, "/commands/" MENU_ITEM_NAME, "accel", "*Shift*F8", NULL);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_menus_remove_menu_item_toggle_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	taglist_window_close ();

	free_taglist ();
	
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


