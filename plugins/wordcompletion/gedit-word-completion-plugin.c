/*
 * Copyright (C) 2009 Ignacio Casal Quinteiro <icq@gnome.org>
 *               2009 Jesse van den Kieboom <jesse@gnome.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-word-completion-plugin.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditWordCompletionPlugin,
				gedit_word_completion_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init))

struct _GeditWordCompletionPluginPrivate
{
	GeditWindow *window;
	GtkSourceCompletionWords *provider;

	gulong tab_added_id;
	gulong tab_removed_id;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void
gedit_word_completion_plugin_init (GeditWordCompletionPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditWordCompletionPlugin initializing");

	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE (plugin,
						    GEDIT_TYPE_WORD_COMPLETION_PLUGIN,
						    GeditWordCompletionPluginPrivate);

	plugin->priv->provider = gtk_source_completion_words_new (_("Document Words"),
								  NULL);
}

static void
gedit_word_completion_plugin_dispose (GObject *object)
{
	GeditWordCompletionPlugin *plugin = GEDIT_WORD_COMPLETION_PLUGIN (object);

	if (plugin->priv->provider != NULL)
	{
		g_object_unref (plugin->priv->provider);
		plugin->priv->provider = NULL;
	}

	if (plugin->priv->window != NULL)
	{
		g_object_unref (plugin->priv->window);
		plugin->priv->window = NULL;
	}

	G_OBJECT_CLASS (gedit_word_completion_plugin_parent_class)->dispose (object);
}

static void
gedit_word_completion_plugin_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
	GeditWordCompletionPlugin *plugin = GEDIT_WORD_COMPLETION_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_word_completion_plugin_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
	GeditWordCompletionPlugin *plugin = GEDIT_WORD_COMPLETION_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
add_view (GeditWordCompletionPlugin *plugin,
	  GtkSourceView             *view)
{
	GtkSourceCompletion *completion;
	GtkTextBuffer *buf;

	completion = gtk_source_view_get_completion (view);
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_source_completion_add_provider (completion,
					    GTK_SOURCE_COMPLETION_PROVIDER (plugin->priv->provider),
					    NULL);
	gtk_source_completion_words_register (plugin->priv->provider,
					      buf);
}

static void
remove_view (GeditWordCompletionPlugin *plugin,
	     GtkSourceView             *view)
{
	GtkSourceCompletion *completion;
	GtkTextBuffer *buf;

	completion = gtk_source_view_get_completion (view);
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_source_completion_remove_provider (completion,
					       GTK_SOURCE_COMPLETION_PROVIDER (plugin->priv->provider),
					       NULL);
	gtk_source_completion_words_unregister (plugin->priv->provider,
						buf);
}

static void
tab_added_cb (GeditWindow               *window,
	      GeditTab                  *tab,
	      GeditWordCompletionPlugin *plugin)
{
	GeditView *view;

	view = gedit_tab_get_view (tab);

	add_view (plugin, GTK_SOURCE_VIEW (view));
}

static void
tab_removed_cb (GeditWindow               *window,
		GeditTab                  *tab,
		GeditWordCompletionPlugin *plugin)
{
	GeditView *view;

	view = gedit_tab_get_view (tab);

	remove_view (plugin, GTK_SOURCE_VIEW (view));
}

static void
gedit_word_completion_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;
	GList *views, *l;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = g_list_next (l))
	{
		add_view (GEDIT_WORD_COMPLETION_PLUGIN (activatable),
			  GTK_SOURCE_VIEW (l->data));
	}

	priv->tab_added_id =
		g_signal_connect (priv->window, "tab-added",
				  G_CALLBACK (tab_added_cb),
				  activatable);
	priv->tab_removed_id =
		g_signal_connect (priv->window, "tab-removed",
				  G_CALLBACK (tab_removed_cb),
				  activatable);
}

static void
gedit_word_completion_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;
	GList *views, *l;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = g_list_next (l))
	{
		remove_view (GEDIT_WORD_COMPLETION_PLUGIN (activatable),
			     GTK_SOURCE_VIEW (l->data));
	}

	g_signal_handler_disconnect (priv->window, priv->tab_added_id);
	g_signal_handler_disconnect (priv->window, priv->tab_removed_id);
}

static void
gedit_word_completion_plugin_class_init (GeditWordCompletionPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_word_completion_plugin_dispose;
	object_class->set_property = gedit_word_completion_plugin_set_property;
	object_class->get_property = gedit_word_completion_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");

	g_type_class_add_private (klass, sizeof (GeditWordCompletionPluginPrivate));
}

static void
gedit_word_completion_plugin_class_finalize (GeditWordCompletionPluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_word_completion_plugin_activate;
	iface->deactivate = gedit_word_completion_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_word_completion_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_WORD_COMPLETION_PLUGIN);
}
