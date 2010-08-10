# -*- coding: utf-8 -*-   

#  synctex.py - Synctex support with Gedit and Evince.
#  
#  Copyright (C) 2010 - José Aliste <jose.aliste@gmail.com>
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

import gtk, gedit, gio , gtk.gdk as gdk
from gettext import gettext as _
from evince_dbus import EvinceWindowProxy
import dbus.mainloop.glib

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


ui_str = """<ui>
  <menubar name="MenuBar">
    <menu name="ToolsMenu" action="Tools">
      <placeholder name="ToolsOps_2">
        <separator/>
        <menuitem name="Synctex" action="SynctexForwardSearch"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

VIEW_DATA_KEY = "SynctexPluginViewData"
WINDOW_DATA_KEY = "SynctexPluginWindowData"


class SynctexViewHelper:
    def __init__(self, view, window, tab):
        self._view = view
        self._window = window
        self._tab = tab
        self._doc = view.get_buffer()
        self.window_proxy = None
        self._handlers = [
            self._doc.connect('saved', self.on_saved_or_loaded),
            self._doc.connect('loaded', self.on_saved_or_loaded)
        ]
        self.active = False
        self.last_iters = None
        self.uri = self._doc.get_uri()
        self.update_uri()
        self._highlight_tag = self._doc.create_tag()
        self._highlight_tag.set_property("background", "#dddddd")

    def on_button_release(self, view, event):
        if event.button == 1 and event.state & gdk.CONTROL_MASK:
            self.sync_view()
    def on_saved_or_loaded(self, doc, data):
        self.update_uri()

    def on_cursor_moved(self, cur):
        self._unhighlight()

    def deactivate(self):
        for h in self._handlers:
            self._doc.disconnect(h)

    def update_uri(self):
        uri = self._doc.get_uri()
        if uri is not None and uri != self.uri:
            self._window.get_data(WINDOW_DATA_KEY).view_dict[uri] = self
            self.uri = uri
        if self.uri is not None:
            self.file = gio.File(self.uri)
        self.update_active()

    def _highlight(self):
        iter = self._doc.get_iter_at_mark(self._doc.get_insert())
        end_iter = iter.copy()
        end_iter.forward_to_line_end()
        self._doc.apply_tag(self._highlight_tag, iter, end_iter)
        self.last_iters = [iter, end_iter];

    def _unhighlight(self):
        if self.last_iters is not None:
            self._doc.remove_tag(self._highlight_tag, self.last_iters[0],self.last_iters[1])
        self.last_iters = None

    def goto_line (self, line):
        self._doc.goto_line(line) 
        self._view.scroll_to_cursor()
        self._window.set_active_tab(self._tab)
        self._highlight()

    def source_view_handler(self, input_file, source_link):
        if self.file.get_basename() == input_file:
            self.goto_line(source_link[0] - 1)
        else:
            uri_input = self.file.get_parent().get_child(input_file).get_uri()
            view_dict = self._window.get_data(WINDOW_DATA_KEY).view_dict
            if uri_input in view_dict:
                view_dict[uri_input].goto_line(source_link[0] - 1)
            else:
                self._window.create_tab_from_uri(uri_input,
                                                 None, source_link[0]-1, False, True)
        self._window.present()

    def sync_view(self):
        if self.active:
            cursor_iter =  self._doc.get_iter_at_mark(self._doc.get_insert())
            line = cursor_iter.get_line() + 1
            col = cursor_iter.get_line_offset()
            self.window_proxy.SyncView(self.file.get_path(), (line, col))

    def update_active(self):
        # Activate the plugin only if the doc is a LaTeX file. 
        self.active = (self._doc.get_language() is not None and self._doc.get_language().get_name() == 'LaTeX')

        if self.active and self.window_proxy is None:
            self._active_handlers = [
                        self._doc.connect('cursor-moved', self.on_cursor_moved),
                        self._view.connect('button-release-event',self.on_button_release)]

            self._window.get_data(WINDOW_DATA_KEY)._action_group.set_sensitive(True)
            filename = self.file.get_basename().partition('.')[0]
            uri_output = self.file.get_parent().get_child(filename + ".pdf").get_uri()
            self.window_proxy = EvinceWindowProxy (uri_output, True)
            self.window_proxy.set_source_handler (self.source_view_handler)
        elif not self.active and self.window_proxy is not None:
            # destroy the evince window proxy.
            self._doc.disconnect(self._active_handlers[0])
            self._view.disconnect(self._active_handlers[1])
            self._window.get_data(WINDOW_DATA_KEY)._action_group.get_sensitive(False)
            self.window_proxy = None


class SynctexWindowHelper:
    def __init__(self, plugin, window):
        self._window = window
        self._plugin = plugin
        self.view_dict = {}

        self._insert_menu()

        for view in window.get_views():
            self.add_helper(view)

        self.handlers = [
            window.connect("tab-added", lambda w, t: self.add_helper(t.get_view(),w, t)),
            window.connect("tab-removed", lambda w, t: self.remove_helper(t.get_view())),
            window.connect("active-tab-changed", self.on_active_tab_changed)
        ]

    def on_active_tab_changed(self, window,  tab):
        view_helper = tab.get_view().get_data(VIEW_DATA_KEY)
        if view_helper is None:
            active = False
        else:
            active = view_helper.active 
        self._action_group.set_sensitive(active)


    def add_helper(self, view, window, tab):
        helper = SynctexViewHelper(view, window, tab)
        if helper.uri is not None:
            self.view_dict[helper.uri] = helper
        view.set_data (VIEW_DATA_KEY, helper)

    def remove_helper(self, view):
        helper = view.get_data(VIEW_DATA_KEY)
        if helper.uri is not None:
            del self.view_dict[helper.uri]
        helper.deactivate()
        view.set_data(VIEW_DATA_KEY, None)

    def __del__(self):
        self._window = None
        self._plugin = None

    def deactivate(self):
        for h in self.handlers:
            self._window.disconnect(h)
        for view in self._window.get_views():
            self.remove_helper(view)
        self._remove_menu()

    def _remove_menu(self):
        manager = self._window.get_ui_manager()
        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._action_group)
        manager.ensure_update()

    def _insert_menu(self):
        # Get the GtkUIManager
        manager = self._window.get_ui_manager()

        # Create a new action group
        self._action_group = gtk.ActionGroup("SynctexPluginActions")
        self._action_group.add_actions([("SynctexForwardSearch", None, _("Forward Search"),
                                         "<Ctrl><Alt>F", _("Forward Search"),
                                         self.forward_search_cb)])

        # Insert the action group
        manager.insert_action_group(self._action_group, -1)

        # Merge the UI
        self._ui_id = manager.add_ui_from_string(ui_str)

    def forward_search_cb(self, action):
        self._window.get_active_view().get_data(VIEW_DATA_KEY).sync_view()

class SynctexPlugin(gedit.Plugin):

    def __init__(self):
        gedit.Plugin.__init__(self)

    def activate(self, window):
        helper = SynctexWindowHelper(self, window)
        window.set_data(WINDOW_DATA_KEY, helper)

    def deactivate(self, window):
        window.get_data(WINDOW_DATA_KEY).deactivate()
        window.set_data(WINDOW_DATA_KEY, None)

    def update_ui(self, window):
        pass
# ex:ts=4:et:
