# -*- coding: utf-8 -*-
#
#  multiedit.py - Multi Edit
#
#  Copyright (C) 2009 - Jesse van den Kieboom
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

from gi.repository import GObject, Gtk, Gedit
import constants
from signals import Signals
from documenthelper import DocumentHelper
import gettext
from gpdefs import *

try:
    gettext.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    _ = lambda s: gettext.dgettext(GETTEXT_PACKAGE, s);
except:
    _ = lambda s: s

ui_str = """
<ui>
  <menubar name="MenuBar">
    <menu name="EditMenu" action="Edit">
      <placeholder name="EditOps_5">
        <menuitem name="MultiEditMode" action="MultiEditModeAction"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class MultiEditPlugin(GObject.Object, Gedit.WindowActivatable, Signals):
    __gtype_name__ = "MultiEditPlugin"

    window = GObject.property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)
        Signals.__init__(self)

    def do_activate(self):
        for view in self.window.get_views():
            self.add_document_helper(view)

        self.connect_signal(self.window, 'tab-added', self.on_tab_added)
        self.connect_signal(self.window, 'tab-removed', self.on_tab_removed)
        self.connect_signal(self.window, 'active-tab-changed', self.on_active_tab_changed)

        self._insert_menu()

    def do_deactivate(self):
        self.disconnect_signals(self.window)

        for view in self.window.get_views():
            self.remove_document_helper(view)

        self._remove_menu()

    def _insert_menu(self):
        manager = self.window.get_ui_manager()

        self._action_group = Gtk.ActionGroup("GeditMultiEditPluginActions")
        self._action_group.add_toggle_actions([('MultiEditModeAction',
                                                None,
                                                _('Multi Edit Mode'),
                                                '<Ctrl><Shift>C',
                                                _('Start multi edit mode'),
                                                self.on_multi_edit_mode)])

        manager.insert_action_group(self._action_group)
        self._ui_id = manager.add_ui_from_string(ui_str)

    def _remove_menu(self):
        manager = self.window.get_ui_manager()
        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._action_group)
        manager.ensure_update()

    def do_update_state(self):
        pass

    def get_helper(self, view):
        return view.get_data(constants.DOCUMENT_HELPER_KEY)

    def add_document_helper(self, view):
        if self.get_helper(view) != None:
            return

        helper = DocumentHelper(view)
        helper.set_toggle_callback(self.on_multi_edit_toggled, helper)

    def remove_document_helper(self, view):
        helper = self.get_helper(view)

        if helper != None:
            helper.stop()

    def get_action(self):
        return self._action_group.get_action('MultiEditModeAction')

    def on_multi_edit_toggled(self, helper):
        if helper.get_view() == self.window.get_active_view():
            self.get_action().set_active(helper.enabled())

    def on_tab_added(self, window, tab):
        self.add_document_helper(tab.get_view())

    def on_tab_removed(self, window, tab):
        self.remove_document_helper(tab.get_view())

    def on_active_tab_changed(self, window, tab):
        view = tab.get_view()
        helper = view.get_data(constants.DOCUMENT_HELPER_KEY)

        self.get_action().set_active(helper != None and helper.enabled())

    def on_multi_edit_mode(self, action):
        view = self.window.get_active_view()
        helper = view.get_data(constants.DOCUMENT_HELPER_KEY)

        if helper != None:
            helper.toggle_multi_edit(self.get_action().get_active())

# ex:ts=4:et:
