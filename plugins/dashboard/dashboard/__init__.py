#!/usr/bin/env python
# -- coding: utf-8 --
#
# Copyright © 2011 Collabora Ltd.
#            By Seif Lotfy <seif@lotfy.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

from gi.repository import GObject, Gedit
from dashboard import Dashboard

class DashboardWindowActivatable(GObject.Object, Gedit.WindowActivatable):

    window = GObject.property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self.status_bar = self.window.get_statusbar()
        self._handlers = [self.window.connect_after("tab-added", self._add_tab),
            self.window.connect_after("active-tab-changed", self._toggle_status_bar),
            self.window.connect_after("destroy", lambda x: self.status_bar.show())]

    def _toggle_status_bar(self, window, tab):
        self.status_bar.show()
        for w in tab.get_children():
            if type(w) == Dashboard:
                self.status_bar.hide()
                break

    def _add_tab(self, window, tab):

        notebook = tab.get_parent()

        doc = tab.get_document()
        uri = doc.get_uri_for_display()

        if not uri.startswith("/"):
            notebook.get_tab_label(tab).get_children()\
                    [0].get_children()[0].get_children()[1].hide()
            self.status_bar.hide()
            d = None
            if str(type(notebook.get_tab_label(tab))) == \
                "<class 'gi.types.__main__.GeditTabLabel'>":
                label = notebook.get_tab_label(tab).get_children()\
                    [0].get_children()[0].get_children()[2]
                text = label.get_text()
                label.set_text("Getting Started")
                def show_doc():
                    notebook.get_tab_label(tab).get_children()\
                        [0].get_children()[0].get_children()[1].show()
                    tab.remove(d)
                    tab.get_children()[0].show()
                    tab.grab_focus()
                    if label.get_text() == "Getting Started":
                        label.set_text(text)
                    self.status_bar.show()
                tab.get_children()[0].hide()
                d = Dashboard(self.window, show_doc)
                tab.pack_start(d, True, True, 0)
                doc.connect("loaded", lambda x, y: show_doc())
                d.search.set_receives_default(True)
                self.window.set_default(d.search)
                self.window.activate_default()
                d.search.grab_focus()
        self._is_adding = False

    def do_deactivate(self):
        tab = self.window.get_active_tab()
        if tab:
            notebook = tab.get_parent()
            for tab in notebook.get_children():
                for w in tab.get_children():
                    if type(w) == Dashboard:
                        tab.remove(w)
                    else:
                        w.show()
                doc = tab.get_document()
                label = notebook.get_tab_label(tab).get_children()\
                    [0].get_children()[0].get_children()[2]
                label.set_text(doc.get_short_name_for_display())

        for handler in self._handlers:
           self.window.disconnect(handler)
        self.window = None

# ex:ts=4:et:
