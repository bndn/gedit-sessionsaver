/*
 * gedit-charmap-panel.c
 * 
 * Copyright (C) 2006 Steve Frécinaux
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

#include <gedit/gedit-plugin.h>
#include "gedit-charmap-panel.h"

#ifdef HAVE_GUCHARMAP_2
#include <gucharmap/gucharmap.h>
#else
#include <gucharmap/gucharmap-script-chapters.h>
#endif

#define GEDIT_CHARMAP_PANEL_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ( \
						 (object),		       \
						 GEDIT_TYPE_CHARMAP_PANEL,     \
						 GeditCharmapPanelPrivate))

struct _GeditCharmapPanelPrivate
{
#ifdef HAVE_GUCHARMAP_2
        GucharmapChaptersView *chapters_view;
        GucharmapChartable *chartable;
#else
	GtkWidget	*table;
	GtkWidget	*chapters;
#endif
};

GEDIT_PLUGIN_DEFINE_TYPE(GeditCharmapPanel, gedit_charmap_panel, GTK_TYPE_VBOX)

#ifdef HAVE_GUCHARMAP_2
static void
on_chapter_view_selection_changed (GtkTreeSelection *selection,
                                   GeditCharmapPanel *panel)
{
        GeditCharmapPanelPrivate *priv = panel->priv;
        GucharmapCodepointList *codepoint_list;
        GtkTreeIter iter;

        if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
                return;

        codepoint_list = gucharmap_chapters_view_get_codepoint_list (priv->chapters_view);
        gucharmap_chartable_set_codepoint_list (priv->chartable, codepoint_list);
        g_object_unref (codepoint_list);
}

#else

static void
on_chapter_changed (GucharmapChapters *chapters,
		    GeditCharmapPanel *panel)
{
	gucharmap_table_set_codepoint_list (GUCHARMAP_TABLE (panel->priv->table),
					    gucharmap_chapters_get_codepoint_list (chapters));
}
#endif /* HAVE_GUCHARMAP_2 */

static void
gedit_charmap_panel_init (GeditCharmapPanel *panel)
{
        GeditCharmapPanelPrivate *priv;
	GucharmapCodepointList *codepoint_list;
	GtkPaned *paned;
#ifdef HAVE_GUCHARMAP_2
        GtkWidget *scrolled_window, *view, *chartable;
        GtkTreeSelection *selection;
        GucharmapChaptersModel *model;
        GtkTreeIter iter;
#endif

	priv = panel->priv = GEDIT_CHARMAP_PANEL_GET_PRIVATE (panel);

	paned = GTK_PANED (gtk_vpaned_new ());

#ifdef HAVE_GUCHARMAP_2
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                            GTK_SHADOW_ETCHED_IN);

        view = gucharmap_chapters_view_new ();
        priv->chapters_view = GUCHARMAP_CHAPTERS_VIEW (view);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	model = gucharmap_script_chapters_model_new ();
        gucharmap_chapters_view_set_model (priv->chapters_view, model);
        g_object_unref (model);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
        g_signal_connect (selection, "changed",
                          G_CALLBACK (on_chapter_view_selection_changed), panel);

        gtk_container_add (GTK_CONTAINER (scrolled_window), view);
        gtk_widget_show (view);

	gtk_paned_pack1 (paned, scrolled_window, FALSE, TRUE);
        gtk_widget_show (scrolled_window);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                            GTK_SHADOW_ETCHED_IN);

        chartable = gucharmap_chartable_new ();
	priv->chartable = GUCHARMAP_CHARTABLE (chartable);
        gtk_container_add (GTK_CONTAINER (scrolled_window), chartable);
        gtk_widget_show (chartable);

	gtk_paned_pack2 (paned, scrolled_window, TRUE, TRUE);
        gtk_widget_show (scrolled_window);

        gucharmap_chapters_view_select_locale (priv->chapters_view);
#else
	priv->chapters = gucharmap_script_chapters_new ();
	g_signal_connect (priv->chapters,
			  "changed",
			  G_CALLBACK (on_chapter_changed),
			  panel);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (GUCHARMAP_CHAPTERS (priv->chapters)->tree_view),
					   FALSE);

	codepoint_list = gucharmap_chapters_get_codepoint_list 
				(GUCHARMAP_CHAPTERS (priv->chapters));
	
	priv->table = gucharmap_table_new ();

	gucharmap_table_set_codepoint_list (GUCHARMAP_TABLE (priv->table),
					    codepoint_list);

	gtk_paned_pack1 (paned, priv->chapters, FALSE, TRUE);
	gtk_paned_pack2 (paned, priv->table, TRUE, TRUE);
#endif /* HAVE_GUCHARMAP_2 */

	gtk_paned_set_position (paned, 150);
	
	gtk_box_pack_start (GTK_BOX (panel), GTK_WIDGET (paned), TRUE, TRUE, 0);
}

static void
gedit_charmap_panel_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_charmap_panel_parent_class)->finalize (object);
}

static void
gedit_charmap_panel_class_init (GeditCharmapPanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GeditCharmapPanelPrivate));

	object_class->finalize = gedit_charmap_panel_finalize;
}

GtkWidget *
gedit_charmap_panel_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_CHARMAP_PANEL, NULL));
}

#ifdef HAVE_GUCHARMAP_2
GucharmapChartable *
gedit_charmap_panel_get_chartable (GeditCharmapPanel *panel)
{
	return panel->priv->chartable;
}
#else
GucharmapTable *
gedit_charmap_panel_get_table (GeditCharmapPanel *panel)
{
	return GUCHARMAP_TABLE (panel->priv->table);
}
#endif
