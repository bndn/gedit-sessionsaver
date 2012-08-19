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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-word-completion-plugin.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-view-activatable.h>
#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#define WINDOW_PROVIDER "GeditWordCompletionPluginProvider"

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static void gedit_view_activatable_iface_init (GeditViewActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditWordCompletionPlugin,
                                gedit_word_completion_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
                                                               gedit_window_activatable_iface_init)
                                G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_VIEW_ACTIVATABLE,
                                                               gedit_view_activatable_iface_init))

struct _GeditWordCompletionPluginPrivate
{
	GtkWidget *window;
	GeditView *view;
	GtkSourceCompletionProvider *provider;
};

enum
{
	PROP_0,
	PROP_WINDOW,
	PROP_VIEW
};

static void
gedit_word_completion_plugin_init (GeditWordCompletionPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditWordCompletionPlugin initializing");

	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE (plugin,
	                                            GEDIT_TYPE_WORD_COMPLETION_PLUGIN,
	                                            GeditWordCompletionPluginPrivate);
}

static void
gedit_word_completion_plugin_dispose (GObject *object)
{
	GeditWordCompletionPlugin *plugin = GEDIT_WORD_COMPLETION_PLUGIN (object);

	if (plugin->priv->window != NULL)
	{
		g_object_unref (plugin->priv->window);
		plugin->priv->window = NULL;
	}

	if (plugin->priv->view != NULL)
	{
		g_object_unref (plugin->priv->view);
		plugin->priv->view = NULL;
	}

	if (plugin->priv->provider != NULL)
	{
		g_object_unref (plugin->priv->provider);
		plugin->priv->provider = NULL;
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
			plugin->priv->window = g_value_dup_object (value);
			break;
		case PROP_VIEW:
			plugin->priv->view = GEDIT_VIEW (g_value_dup_object (value));
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
		case PROP_VIEW:
			g_value_set_object (value, plugin->priv->view);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_word_completion_window_activate (GeditWindowActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;
	GtkSourceCompletionWords *provider;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	provider = gtk_source_completion_words_new (_("Document Words"),
	                                            NULL);

	g_object_set_data_full (G_OBJECT (priv->window),
	                        WINDOW_PROVIDER,
	                        provider,
	                        (GDestroyNotify)g_object_unref);
}

static void
gedit_word_completion_window_deactivate (GeditWindowActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	g_object_set_data (G_OBJECT (priv->window), WINDOW_PROVIDER, NULL);
}

static void
gedit_word_completion_view_activate (GeditViewActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;
	GtkSourceCompletion *completion;
	GtkSourceCompletionProvider *provider;
	GtkTextBuffer *buf;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	priv->window = gtk_widget_get_toplevel (GTK_WIDGET (priv->view));

	/* We are disposing the window */
	g_object_ref (priv->window);

	completion = gtk_source_view_get_completion (GTK_SOURCE_VIEW (priv->view));
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->view));

	provider = g_object_get_data (G_OBJECT (priv->window), WINDOW_PROVIDER);

	if (provider == NULL)
	{
		/* Standalone provider */
		provider = GTK_SOURCE_COMPLETION_PROVIDER (
			gtk_source_completion_words_new (_("Document Words"),
			                                 NULL));
	}

	priv->provider = g_object_ref (provider);

	gtk_source_completion_add_provider (completion, provider, NULL);
	gtk_source_completion_words_register (GTK_SOURCE_COMPLETION_WORDS (provider),
	                                      buf);
}

static void
gedit_word_completion_view_deactivate (GeditViewActivatable *activatable)
{
	GeditWordCompletionPluginPrivate *priv;
	GtkSourceCompletion *completion;
	GtkSourceCompletionProvider *provider;
	GtkTextBuffer *buf;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_WORD_COMPLETION_PLUGIN (activatable)->priv;

	completion = gtk_source_view_get_completion (GTK_SOURCE_VIEW (priv->view));
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->view));

	gtk_source_completion_remove_provider (completion,
	                                       priv->provider,
	                                       NULL);

	gtk_source_completion_words_unregister (GTK_SOURCE_COMPLETION_WORDS (priv->provider),
	                                        buf);
}

static void
gedit_word_completion_plugin_class_init (GeditWordCompletionPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_word_completion_plugin_dispose;
	object_class->set_property = gedit_word_completion_plugin_set_property;
	object_class->get_property = gedit_word_completion_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
	g_object_class_override_property (object_class, PROP_VIEW, "view");

	g_type_class_add_private (klass, sizeof (GeditWordCompletionPluginPrivate));
}

static void
gedit_word_completion_plugin_class_finalize (GeditWordCompletionPluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_word_completion_window_activate;
	iface->deactivate = gedit_word_completion_window_deactivate;
}

static void
gedit_view_activatable_iface_init (GeditViewActivatableInterface *iface)
{
	iface->activate = gedit_word_completion_view_activate;
	iface->deactivate = gedit_word_completion_view_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_word_completion_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
	                                            GEDIT_TYPE_WINDOW_ACTIVATABLE,
	                                            GEDIT_TYPE_WORD_COMPLETION_PLUGIN);

	peas_object_module_register_extension_type (module,
	                                            GEDIT_TYPE_VIEW_ACTIVATABLE,
	                                            GEDIT_TYPE_WORD_COMPLETION_PLUGIN);
}
