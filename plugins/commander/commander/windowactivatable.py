# -*- coding: utf-8 -*-
#
#  windowhelper.py - commander
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

from gi.repository import GObject, Gtk, Gedit
from entry import Entry
from info import Info
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
        <menuitem name="CommanderEditMode" action="CommanderModeAction"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class WindowActivatable(GObject.Object, Gedit.WindowActivatable):
    __gtype_name__ = "CommanderWindowActivatable"

    window = GObject.property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self._entry = None
        self._view = None

        self.install_ui()

    def do_deactivate(self):
        self.uninstall_ui()

    def do_update_state(self):
        pass

    def install_ui(self):
        manager = self.window.get_ui_manager()

        self._action_group = Gtk.ActionGroup("GeditCommanderPluginActions")
        self._action_group.add_toggle_actions([('CommanderModeAction', None,
                                               _('Commander Mode'), '<Ctrl>period',
                                               _('Start commander mode'), self.on_commander_mode)])

        manager.insert_action_group(self._action_group, -1)
        self._merge_id = manager.add_ui_from_string(ui_str)

    def uninstall_ui(self):
        manager = self.window.get_ui_manager()
        manager.remove_ui(self._merge_id)
        manager.remove_action_group(self._action_group)

        manager.ensure_update()

    def on_commander_mode(self, action, user_data=None):
        view = self.window.get_active_view()

        if not view:
            return False

        if action.get_active():
            if not self._entry or view != self._view:
                self._entry = Entry(view)
                self._entry.connect('destroy', self.on_entry_destroy)

            self._entry.grab_focus()
            self._view = view
        elif self._entry:
            self._entry.destroy()
            self._view = None

        return True

    def on_entry_destroy(self, widget, user_data=None):
        self._entry = None
        self._action_group.get_action('CommanderModeAction').set_active(False)

# vi:ex:ts=4:et
