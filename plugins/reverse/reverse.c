/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Reverse plugin
 * Roberto Majadas <phoenix@nova.es>
 *
 * Reverse text
 */

#include <config.h>
#include <gnome.h>

#include "document.h"
#include "view.h"
#include "plugin.h"


static void
destroy_plugin (PluginData *pd)
{
}

static void
reverse (void)
{
	GeditDocument *doc;
	GeditView *view = gedit_view_active();
	gchar *buffer ;
	gchar tmp ;
	gint buffer_length ;
	gint i;
	gint *start = g_new(gint, 1);
	gint *end = g_new(gint, 1);
	
	if (view == NULL)
		return;

	doc = view->doc;

	buffer_length = gedit_document_get_buffer_length (doc);
	buffer = gedit_document_get_buffer (doc);

	if (!gedit_view_get_selection (view, start, end))
	{
		*start = 0;
		*end = buffer_length;
	}

	for (i=*start; i < ( *start + (*end - *start) / 2 ); i++)
	{
		tmp = buffer [i];
		buffer [i] = buffer [*end + *start - i - 1];
		buffer [*end + *start - i - 1] = tmp;
	}
     
	gedit_document_delete_text (doc, 0, buffer_length, TRUE);
	gedit_document_insert_text (doc, buffer, 0, TRUE);

	g_free (start);
	g_free (end);
	g_free (buffer);
     
}
gint
init_plugin (PluginData *pd)
{
	/* initialize */
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Reverse");
	pd->desc = _("Reverse text");
	pd->long_desc = _("Reverse text");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer)reverse;
	
	return PLUGIN_OK;
}

