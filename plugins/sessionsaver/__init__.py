# -*- coding: utf-8 -*-
# __init__.py
# This file is part of gedit Session Saver Plugin
#
# Copyright (C) 2006-2007 - Steve Frécinaux <code@istique.net>
# Copyright (C) 2010 - Kenny Meyer <knny.myer@gmail.com>
#
# gedit Session Saver Plugin is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# gedit Session Saver Plugin is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gedit Session Saver Plugin; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA  02110-1301  USA

from gi.repository import GObject, Gio, Gtk, Gedit
import os.path
import gettext
from store import XMLSessionStore
from dialogs import SaveSessionDialog, SessionManagerDialog
from gpdefs import *

try:
    gettext.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    _ = lambda s: gettext.dgettext(GETTEXT_PACKAGE, s);
except:
    _ = lambda s: s

ui_str = """
<ui>
  <menubar name="MenuBar">
    <menu name="FileMenu" action="File">
      <placeholder name="FileOps_2">
        <separator/>
        <menu name="FileSessionMenu" action="FileSession">
          <placeholder name="SessionPluginPlaceHolder"/>
          <separator/>
          <menuitem name="FileSessionSaveMenu" action="FileSessionSave"/>
          <menuitem name="FileSessionManageMenu" action="FileSessionManage"/>
        </menu>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class SessionSaverPlugin(GObject.Object, Gedit.WindowActivatable):
    __gtype_name__ = "SessionSaverPlugin"

    window = GObject.property(type = Gedit.Window)

    ACTION_HANDLER_DATA_KEY = "SessionSaverActionHandlerData"
    SESSION_MENU_PATH = '/MenuBar/FileMenu/FileOps_2/FileSessionMenu/SessionPluginPlaceHolder'

    def __init__(self):
        GObject.Object.__init__(self)
        self.sessions = XMLSessionStore()

    def do_activate(self):
        self._insert_menu()

    def do_deactivate(self):
        self._remove_menu()

    def do_update_state(self):
        pass

    def _update_session_menu(self):
        manager = self.window.get_ui_manager()

        self._menu_ui_id = manager.new_merge_id()

        for i, session in enumerate(self.sessions):
            action_name = 'SessionSaver%X' % i
            action = Gtk.Action(action_name,
                                session.name,
                                _("Recover '%s' session") % session.name,
                                "")
            handler = action.connect("activate", self.session_menu_action, session)

            action.set_data(self.ACTION_HANDLER_DATA_KEY, handler)
            # Add an action to the session list items.
            self._action_group.add_action(action)

            manager.add_ui(self._ui_id,
                           self.SESSION_MENU_PATH,
                           action_name,
                           action_name,
                           Gtk.UIManagerItemType.MENUITEM,
                           False)

    def _insert_menu(self):
        manager = self.window.get_ui_manager()
        self._menu_action_group = Gtk.ActionGroup("SessionSaverPluginActions")
        self._menu_action_group.add_actions(
            [("FileSession", None, _("Sa_ved sessions"), None, None),
             ("FileSessionSave", Gtk.STOCK_SAVE_AS,
              _("_Save current session"), None,
              _("Save the current document list as a new session"),
              self.on_save_session_action),
             ("FileSessionManage", None, _("_Manage saved sessions..."),
              None, _("Open the saved session manager"),
              self.on_manage_sessions_action)
            ], self.window)
        manager.insert_action_group(self._menu_action_group)

        self._ui_id = manager.add_ui_from_string(ui_str)
        self._action_group = Gtk.ActionGroup(name="SessionSaverPluginSessionActions")
        manager.insert_action_group(self._action_group)

        self._update_session_menu()

        manager.ensure_update()

    def _remove_session_menu(self):
        if not self._menu_ui_id:
            return

        manager = self.window.get_ui_manager()
        manager.remove_ui(self._menu_ui_id)

        for action in self._action_group.list_actions():
            handler = action.get_data(self.ACTION_HANDLER_DATA_KEY)
            if handler is not None:
                action.disconnect(handler)
            self._action_group.remove_action(action)

        manager.remove_action_group(self._action_group)

    def _remove_menu(self):
        manager = self.window.get_ui_manager()

        self._remove_session_menu()

        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._menu_action_group)

        manager.ensure_update()

    def _load_session(self, session, window):
        # Note: a session has to stand on its own window.
        tab = window.get_active_tab()
        if tab is not None and \
           not (tab.get_document().is_untouched() and \
                tab.get_state() == Gedit.TabState.STATE_NORMAL):
            # Create a new gedit window
            window = Gedit.App.get_default().create_window(None)
            window.show()

        Gedit.commands_load_locations(window, session.files, None, 0, 0)

    def on_save_session_action(self, action, window):
        SaveSessionDialog(window, self, self.sessions).run()

    def on_manage_sessions_action(self, action, window):
        SessionManagerDialog(self, self.sessions).run()

    def session_menu_action(self, action, session):
        self._load_session(session, self.window)

# ex:ts=4:et:
