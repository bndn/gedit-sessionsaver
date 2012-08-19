# -*- coding: utf-8 -*-
#
# Copyright (C) 2006 Steve Frécinaux <steve@istique.net>
#               2010 Ignacio Casal Quinteiro <icq@gnome.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

from gi.repository import Gtk, Gucharmap

class CharmapPanel(Gtk.Box):
    __gtype_name__ = "CharmapPanel"

    def __init__(self):
        Gtk.Box.__init__(self)

        paned = Gtk.Paned.new(Gtk.Orientation.VERTICAL)

        scrolled_window = Gtk.ScrolledWindow(None, None)
        scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scrolled_window.set_shadow_type(Gtk.ShadowType.ETCHED_IN)

        self.view = Gucharmap.ChaptersView()
        self.view.set_headers_visible (False)

        model = Gucharmap.ScriptChaptersModel()
        self.view.set_model(model)

        selection = self.view.get_selection()
        selection.connect("changed", self.on_chapter_view_selection_changed)

        scrolled_window.add(self.view)
        paned.pack1(scrolled_window, False, True)

        scrolled_window = Gtk.ScrolledWindow(None, None)
        scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scrolled_window.set_shadow_type(Gtk.ShadowType.ETCHED_IN)

        self.chartable = Gucharmap.Chartable()

        scrolled_window.add(self.chartable)
        paned.pack2(scrolled_window, True, True)

        paned.set_position(150)
        paned.show_all()

        self.pack_start(paned, True, True, 0)

    def on_chapter_view_selection_changed(self, selection):
        model, it = selection.get_selected()
        if it:
            codepoint_list = self.view.get_codepoint_list()
            self.chartable.set_codepoint_list(codepoint_list)

    def get_chartable(self):
        return self.chartable

# ex:et:ts=4:
