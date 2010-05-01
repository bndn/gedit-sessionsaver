/*
 * charmap-panel.h
 * This file is part of gedit
 *
 * Copyright (C) 2006 - Steve Frécinaux
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_CHARMAP_PANEL_H__
#define __GEDIT_CHARMAP_PANEL_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <gucharmap/gucharmap.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_CHARMAP_PANEL		(gedit_charmap_panel_get_type ())
#define GEDIT_CHARMAP_PANEL(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_CHARMAP_PANEL, GeditCharmapPanel))
#define GEDIT_CHARMAP_PANEL_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_CHARMAP_PANEL, GeditCharmapPanelClass))
#define GEDIT_IS_CHARMAP_PANEL(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_CHARMAP_PANEL))
#define GEDIT_IS_CHARMAP_PANEL_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_CHARMAP_PANEL))
#define GEDIT_CHARMAP_PANEL_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_CHARMAP_PANEL, GeditCharmapPanelClass))

/* Private structure type */
typedef struct _GeditCharmapPanelPrivate	GeditCharmapPanelPrivate;

/*
 * Main object structure
 */
typedef struct _GeditCharmapPanel		GeditCharmapPanel;

struct _GeditCharmapPanel
{
	GtkVBox parent_instance;

	/*< private > */
	GeditCharmapPanelPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditCharmapPanelClass	GeditCharmapPanelClass;

struct _GeditCharmapPanelClass
{
	GtkVBoxClass parent_class;
};

/*
 * Public methods
 */
GType		 gedit_charmap_panel_get_type	   (void) G_GNUC_CONST;
GType		 gedit_charmap_panel_register_type (GTypeModule * module);
GtkWidget	*gedit_charmap_panel_new	   (void);

GucharmapChartable *gedit_charmap_panel_get_chartable (GeditCharmapPanel *panel);

G_END_DECLS

#endif /* __CHARMAP_PANEL_H__ */
