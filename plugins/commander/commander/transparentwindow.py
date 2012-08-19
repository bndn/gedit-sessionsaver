# -*- coding: utf-8 -*-
#
#  transparentwindow.py - commander
#
#  Copyright (C) 2010 - Jesse van den Kieboom
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
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.

from gi.repository import GObject, Gdk, Gtk, Gedit
import cairo

class TransparentWindow(Gtk.Window):
    __gtype_name__ = "CommanderTransparentWindow"

    def __init__(self, lvl=Gtk.WindowType.TOPLEVEL):
        Gtk.Window.__init__(self,
                            type=lvl,
                            decorated=False,
                            app_paintable=True,
                            skip_pager_hint=True,
                            skip_taskbar_hint=True)

        self.set_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.set_rgba()

    def set_rgba(self):
        visual = self.get_screen().get_rgba_visual()

        if not visual:
            visual = self.get_screen().get_system_visual()

        self.set_visual(visual)

    def do_screen_changed(self, prev):
        super(TransparentWindow, self).do_screen_changed(prev)

        self.set_rgba()

    def background_color(self):
        return Gdk.RGBA(0, 0, 0, 0.8)

    def background_shape(self, ct, alloc):
        ct.rectangle(0, 0, alloc.width, alloc.height)

    def draw_background(self, ct, widget=None):
        if widget == None:
            widget = self

        ct.set_operator(cairo.OPERATOR_SOURCE)
        alloc = widget.get_allocation()

        ct.rectangle(0, 0, alloc.width, alloc.height)
        ct.set_source_rgba(0, 0, 0, 0)
        ct.fill()

        color = self.background_color()
        self.background_shape(ct, alloc)

        ct.set_source_rgba(color.red, color.green, color.blue, color.alpha)
        ct.fill()

    def do_draw(self, ct):
        self.draw_background(ct)
        Gtk.Window.do_draw(self, ct)

        return False

# vi:ex:ts=4:et
