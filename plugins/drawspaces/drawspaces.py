# -*- coding: utf-8 -*-
#
#  drawspaces.py - Draw Spaces Plugin for gedit
#
#  Copyright (C) 2006 - Paolo Borelli
#  Copyright (C) 2007 - Steve Frécinaux
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330,
#  Boston, MA 02111-1307, USA.

import gtk
import gtk.glade
import gedit
import gconf
import os
from math import pi

try:
    from gpdefs import *
    import gettext, locale
    gettext.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    gtk.glade.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    gtk.glade.textdomain(GETTEXT_PACKAGE)
    locale.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    _ = lambda s: gettext.dgettext(GETTEXT_PACKAGE, s)
except:
    _ = lambda s: s

GCONF_KEY_BASE = '/apps/gedit-2/plugins/drawspaces'
GCONF_KEY_COLOR = '/apps/gedit-2/plugins/drawspaces/color'
GCONF_KEY_DRAW_TABS = '/apps/gedit-2/plugins/drawspaces/draw_tabs'
GCONF_KEY_DRAW_SPACES = '/apps/gedit-2/plugins/drawspaces/draw_spaces'


class DrawSpacesViewHelper(object):
    def __init__(self, plugin, view):
        self._plugin = plugin
        self._view = view
        self._handler_id = view.connect('event-after', self.on_event_after)

    def deactivate(self):
        self._view.disconnect(self._handler_id)

    def on_event_after(self, view, event):
        if event.type != gtk.gdk.EXPOSE or \
           event.window != view.get_window(gtk.TEXT_WINDOW_TEXT):
            return

        y = view.window_to_buffer_coords(gtk.TEXT_WINDOW_TEXT, event.area.x, event.area.y)[1]
        start = view.get_line_at_y(y)[0]
        end = view.get_line_at_y(y + event.area.height)[0]
        end.forward_to_line_end()
        self.draw_tabs_and_spaces(event, start, end)

    def draw_space_at_iter(self, cr, iter):
        if not self._plugin._draw_spaces:
            return

        rect = self._view.get_iter_location(iter)
        x, y = self._view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT,
                                                  rect.x + rect.width / 2,
                                                  rect.y + rect.height * 2 / 3)

        cr.save()
        cr.move_to(x, y)
        cr.arc(x, y, 0.8, 0, 2 * pi)
        cr.restore()

    def draw_tab_at_iter(self, cr, iter):
        if not self._plugin._draw_tabs:
            return

        rect = self._view.get_iter_location(iter)
        x, y = self._view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT,
                                                  rect.x,
                                                  rect.y + rect.height * 2 / 3)

        cr.save()
        cr.move_to(x + 4, y)
        cr.rel_line_to(rect.width - 8, 0)
        cr.rel_line_to(-3,-3)
        cr.rel_move_to(+3,+3)
        cr.rel_line_to(-3,+3)
        cr.restore()

    def draw_tabs_and_spaces(self, event, iter, end):
        cr = event.window.cairo_create()
        cr.set_source_color(self._plugin._color)
        cr.set_line_width(0.8)
        while iter.compare(end) <= 0:
            c = iter.get_char()
            if c == '\t':
                self.draw_tab_at_iter(cr, iter)
            elif c == ' ':
                self.draw_space_at_iter(cr, iter)
            if not iter.forward_char():
                break
        cr.stroke()


class DrawSpacesConfigDialog(object):
    GLADE_FILE = os.path.join(os.path.dirname(__file__), "drawspaces.glade")

    def __init__(self, plugin):
        object.__init__(self)

        self._plugin = plugin

        self.ui = gtk.glade.XML(self.GLADE_FILE, 'config-dialog')
        self.dialog = self.ui.get_widget('config-dialog')

        self['draw-spaces'].set_active(plugin._draw_spaces)
        self['draw-tabs'].set_active(plugin._draw_tabs)
        self['color'].set_color(plugin._color)

        handlers = {
            'on_color_color_set': self.on_color_color_set,
            'on_draw_spaces_toggled': self.on_draw_spaces_toggled,
            'on_draw_tabs_toggled': self.on_draw_tabs_toggled
        }
        self.ui.signal_autoconnect(handlers)

        self.dialog.connect('response', self.on_response)

    def __getitem__(self, item):
        return self.ui.get_widget(item)

    def __del__(self):
        self.__class__._instance = None

    def on_response(self, dialog, response_id):
        self.dialog.destroy()

    def on_color_color_set(self, colorbutton):
        color = colorbutton.get_color()
        value = "#%04x%04x%04x" % (color.red, color.green, color.blue)
        gconf_set_str(GCONF_KEY_COLOR, value)

    def on_draw_spaces_toggled(self, checkbox):
        value = checkbox.get_active()
        gconf_set_bool(GCONF_KEY_DRAW_SPACES, value)

    def on_draw_tabs_toggled(self, checkbox):
        value = checkbox.get_active()
        gconf_set_bool(GCONF_KEY_DRAW_TABS, value)


class DrawSpacesPlugin(gedit.Plugin):
    WINDOW_DATA_KEY = "DrawSpacesPluginWindowData"
    VIEW_DATA_KEY = "DrawSpacesPluginViewData"

    def __init__(self):
        gedit.Plugin.__init__(self)

        # TODO: there should be a GUI to config those
        self._color = gtk.gdk.color_parse(gconf_get_str(GCONF_KEY_COLOR, '#CCCCCC'))
        self._draw_tabs = gconf_get_bool(GCONF_KEY_DRAW_TABS, True)
        self._draw_spaces = gconf_get_bool(GCONF_KEY_DRAW_SPACES, True)
        gconf.client_get_default().notify_add(GCONF_KEY_BASE, self.on_gconf_notify)

    def on_gconf_notify(self, client, id, entry, data):
        key = entry.get_key()
        if key == GCONF_KEY_COLOR:
            self._color = gtk.gdk.color_parse(entry.get_value().get_string())
        elif key == GCONF_KEY_DRAW_TABS:
            self._draw_tabs = entry.get_value().get_bool()
        elif key == GCONF_KEY_DRAW_SPACES:
            self._draw_spaces = entry.get_value().get_bool()

        # redraw as needed
        for window in gedit.app_get_default().get_windows():
            window.get_active_view().queue_draw()

    def add_helper(self, view):
        helper = DrawSpacesViewHelper(self, view)
        view.set_data(self.VIEW_DATA_KEY, helper)
    
    def remove_helper(self, view):
        view.get_data(self.VIEW_DATA_KEY).deactivate()
        view.set_data(self.VIEW_DATA_KEY, None)

    def activate(self, window):
        for view in window.get_views():
            self.add_helper(view)
            
        handler_id = window.connect("tab-added",
                                    lambda w, t: self.add_helper(t.get_view()))
        window.set_data(self.WINDOW_DATA_KEY, handler_id)

    def deactivate(self, window):
        handler_id = window.get_data(self.WINDOW_DATA_KEY)
        window.disconnect(handler_id)
        window.set_data(self.WINDOW_DATA_KEY, None)
        
        for view in window.get_views():
            self.remove_helper(view)

    def update_ui(self, window):
        pass

    def create_configure_dialog(self):
        return DrawSpacesConfigDialog(self).dialog


def gconf_get_bool(key, default = False):
    val = gconf.client_get_default().get(key)
    if val is not None and val.type == gconf.VALUE_BOOL:
        return val.get_bool()
    else:
        return default

def gconf_set_bool(key, value):
    v = gconf.Value(gconf.VALUE_BOOL)
    v.set_bool(value)
    gconf.client_get_default().set(key, v)

def gconf_get_str(key, default = ''):
    val = gconf.client_get_default().get(key)
    if val is not None and val.type == gconf.VALUE_STRING:
        return val.get_string()
    else:
        return default

def gconf_set_str(key, value):
    v = gconf.Value(gconf.VALUE_STRING)
    v.set_string(value)
    gconf.client_get_default().set(key, v)

# ex:ts=4:et:
