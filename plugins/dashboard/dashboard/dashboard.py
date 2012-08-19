#!/usr/bin/env python
# -- coding: utf-8 --
#
# Copyright © 2011 Collabora Ltd.
#            By Seif Lotfy <seif@lotfy.com>
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

from gi.repository import GObject, Gedit, Gtk, Gio, GLib, GdkPixbuf, GtkSource
from zeitgeist.client import ZeitgeistClient
from zeitgeist.datamodel import Event, TimeRange
from utils import *

import time
import os
import urllib
import dbus
import datetime

try:
    BUS = dbus.SessionBus()
except Exception:
    BUS = None

CLIENT = ZeitgeistClient()
version = [int(x) for x in CLIENT.get_version()]
MIN_VERSION = [0, 8, 0, 0]
if version < MIN_VERSION:
    print "PLEASE USE ZEITGEIST 0.8.0 or above"

class Item(Gtk.Button):

    def __init__(self, subject):

        Gtk.Button.__init__(self)
        self._file_object = Gio.file_new_for_uri(subject.uri)
        self._file_info =\
            self._file_object.query_info("standard::content-type",
            Gio.FileQueryInfoFlags.NONE, None)
        self.subject = subject
        SIZE_LARGE = (256, 160)
        self.thumb = create_text_thumb(self, SIZE_LARGE, 1)
        self.set_size_request(224, 196)
        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        vbox.pack_start(self.thumb, True, True, 0)
        self.label = Gtk.Label()
        self.label.set_markup("<span size='large'><b>%s</b></span>"\
            %subject.text)
        vbox.pack_start(self.label, False, False, 6)
        self.add(vbox)

    @property
    def mime_type(self):
        return self._file_info.get_attribute_string("standard::content-type")

    def get_content(self):
        f = open(self._file_object.get_path())
        try:
            content = f.read()
        finally:
            f.close()
        return content


class StockButton(Gtk.Button):

    def __init__(self, stock, label):
        Gtk.Button.__init__(self)
        self.set_size_request(256, 196)
        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        self.label = Gtk.Label()
        self.label.set_markup("<span size='large'><b>%s</b></span>"%label)
        vbox.pack_end(self.label, False, False, 6)
        self.add(vbox)


class DashView(Gtk.Box):

    def __init__(self, show_doc, window):
        Gtk.Box.__init__(self, orientation = Gtk.Orientation.VERTICAL)
        hbox = Gtk.Box()
        self._window = window
        self.pack_start(Gtk.Label(""), True, True, 0)
        self.pack_start(hbox, False, False, 0)
        self.pack_start(Gtk.Label(""), True, True, 0)

        self.grid = Gtk.Grid()
        self.grid.set_row_homogeneous(True)
        self.grid.set_column_homogeneous(True)

        self.grid.set_column_spacing(15)
        self.grid.set_row_spacing(15)
        hbox.pack_start(Gtk.Label(""), True, True, 0)
        hbox.pack_start(self.grid, False, False, 0)
        hbox.pack_start(Gtk.Label(""), True, True, 0)

        self.new_button = StockButton(Gtk.STOCK_NEW, _("Empty Document"))
        self.new_button.connect("clicked", lambda x: show_doc())

    def populate_grid(self, events):
        for child in self.grid.get_children():
            self.grid.remove(child)
        self.hide()
        subjects = []

        self.grid.add(self.new_button)
        self.show_all()

        for event in events:
            for subject in event.subjects:
                if uri_exists(subject.uri):
                    subjects.append(subject)
                if len(subjects) == GRID_ITEM_COUNT:
                    break
            if len(subjects) == GRID_ITEM_COUNT:
                break

        for i, subject in enumerate(subjects):
            item = Item(subjects[i])
            item.connect("clicked", self.open)
            self.grid.attach(item, (i + 1) % 4, (i + 1) / 4, 1, 1)
        self.show_all()

    def open(self, item):
        Gedit.commands_load_location(self._window,
            item._file_object, None, -1, -1)


class DashPanelButton (Gtk.Box):

    def __init__(self, label):
        Gtk.Box.__init__(self, orientation = Gtk.Orientation.VERTICAL)
        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        self.text = label
        self.label = Gtk.Label()
        self.label.set_markup("<b>%s</b>"%label)
        self.label.set_alignment(0.5, 0.5)
        self.button = Gtk.Button()
        self.button.add(vbox)
        self.button.set_relief(2)
        self.pack_start(self.button, True, True, 0)
        self.set_size_request(-1, 42)
        self._active = False
        self.connect("style-updated", self.change_style)
        self.button.set_can_focus(False)
        self.line = Gtk.EventBox()
        self.line.set_size_request(-1, 3)
        box = Gtk.Box()
        box.pack_start(self.line, True, True, 4)
        vbox.pack_start(Gtk.Box(), False, False, 6) #This is for styling
        vbox.pack_start(self.label, False, False, 0)
        vbox.pack_start(box, False, False, 1)

    def set_active(self, active):
        self._active = active
        if self._active == True:
            self.line.show()
        else:
            self.line.hide()

    def change_style(self, widget):
        self.style = self.get_style_context()
        color = self.style.get_background_color(Gtk.StateFlags.SELECTED)
        self.line.override_background_color(Gtk.StateFlags.NORMAL, color)


class ZeitgeistFTS(object):

    result_type_relevancy = 100

    def __init__(self):
        self.template = Event()
        self.template.actor = "application://gedit.desktop"
        self._fts = BUS.get_object('org.gnome.zeitgeist.Engine',
            '/org/gnome/zeitgeist/index/activity')
        self.fts = dbus.Interface(self._fts, 'org.gnome.zeitgeist.Index')

    def search(self, text, callback):
        events, count = self.fts.Search(text + "*", TimeRange.always(),
            [self.template], 0, 100, 2)
        callback(map(Event, events), count)

ZG_FTS = ZeitgeistFTS()


class SearchEntry(Gtk.Entry):

    __gsignals__ = {
        "clear": (GObject.SIGNAL_RUN_FIRST,
                   GObject.TYPE_NONE,
                   ()),
        "search": (GObject.SIGNAL_RUN_FIRST,
                    GObject.TYPE_NONE,
                    (GObject.TYPE_STRING,)),
        "close": (GObject.SIGNAL_RUN_FIRST,
                   GObject.TYPE_NONE,
                   ()),
    }

    search_timeout = 0

    def __init__(self, accel_group = None):
        Gtk.Entry.__init__(self)
        self.set_width_chars(40)
        self.set_placeholder_text(_("Type here to search..."))
        self.connect("changed", lambda w: self._queue_search())

        search_icon =\
            Gio.ThemedIcon.new_with_default_fallbacks("edit-find-symbolic")
        self.set_icon_from_gicon(Gtk.EntryIconPosition.PRIMARY, search_icon)
        clear_icon =\
            Gio.ThemedIcon.new_with_default_fallbacks("edit-clear-symbolic")
        self.set_icon_from_gicon(Gtk.EntryIconPosition.SECONDARY, clear_icon)
        self.connect("icon-press", self._icon_press)
        self.show_all()

    def _icon_press(self, widget, pos, event):
        if event.button == 1 and pos == 1:
            self.set_text("")
            self.emit("clear")

    def _queue_search(self):
        if self.search_timeout != 0:
            GObject.source_remove(self.search_timeout)
            self.search_timeout = 0

        if len(self.get_text()) == 0:
            self.emit("clear")
        else:
            self.search_timeout = GObject.timeout_add(200,
                self._typing_timeout)

    def _typing_timeout(self):
        if len(self.get_text()) > 0:
            self.emit("search", self.get_text())

        self.search_timeout = 0
        return False


class ListView(Gtk.TreeView):

    def __init__(self):
        Gtk.TreeView.__init__(self)
        self.icontheme = Gtk.IconTheme.get_default()
        self.model = Gtk.ListStore(str, GdkPixbuf.Pixbuf, str, str,
            GObject.TYPE_PYOBJECT)

        renderer_text = Gtk.CellRendererText()
        column_text = Gtk.TreeViewColumn("Text", renderer_text, markup=0)
        self.append_column(column_text)

        renderer_pixbuf = Gtk.CellRendererPixbuf()
        column_pixbuf = Gtk.TreeViewColumn("Image", renderer_pixbuf, pixbuf=1)
        self.append_column(column_pixbuf)

        renderer_text = Gtk.CellRendererText()
        renderer_text.set_alignment(0.0, 0.5)
        renderer_text.set_property('ellipsize', 1)
        column_text = Gtk.TreeViewColumn("Text", renderer_text, markup=2)
        column_text.set_expand(True)
        self.append_column(column_text)

        renderer_text = Gtk.CellRendererText()
        renderer_text.set_alignment(1.0, 0.5)
        column_text = Gtk.TreeViewColumn("Text", renderer_text, markup=3)
        self.append_column(column_text)

        self.style_get_property("horizontal-separator", 9)
        self.set_headers_visible(False)
        self.set_model(self.model)
        self.home_path = "file://%s" %os.getenv("HOME")
        self.now = time.time()

    def get_time_string(self, timestamp):
        timestamp = int(timestamp)/1000
        diff = int(self.now - timestamp)
        if diff < 60:
            return "   few moment ago   "
        diff = int(diff / 60)
        if diff < 60:
            if diff == 1:
                return "   %i minute ago   " %int(diff)
            return "   %i minutes ago   " %int(diff)
        diff = int(diff / 60)
        if diff < 24:
            if diff == 1:
                return "   %i hour ago   " %int(diff)
            return "   %i hours ago   " %int(diff)
        diff = int(diff / 24)
        if diff < 4:
            if diff == 1:
                return "   Yesterday   "
            return "   %i days ago   " %int(diff)

        return datetime.datetime.fromtimestamp(timestamp)\
            .strftime('   %d %B %Y   ')

    def clear(self):
        self.model.clear()
        self.now = time.time()

    def insert_results(self, events, match):
        self.clear()
        for event in events:
            self.add_item(event, 48)

    def add_item(self, event, icon_size):
        item = event.subjects[0]
        if uri_exists(item.uri):
            uri = item.uri.replace(self.home_path, "Home").split("/")
            uri = u" → ".join(uri[0:len(uri) - 1])
            text = """<span size='large'><b>
               %s\n</b></span><span color='darkgrey'>   %s
               </span>"""%(item.text, uri)
            iter = self.model.append(["   ", no_pixbuf, text,
                "<span color='darkgrey'>   %s</span>"\
                %self.get_time_string(event.timestamp), item])

            def callback(icon):
                self.model[iter][1] = icon if icon is not None else no_pixbuf
            get_icon(item, callback, icon_size)


class Dashboard (Gtk.Box):

    def __init__(self, window, show_doc):
        Gtk.Box.__init__(self, orientation = Gtk.Orientation.VERTICAL)
        self._window = window
        self._show_doc = show_doc
        self.last_used_button = None
        self.frequently_used_toggle_button = DashPanelButton(_("Most Used"))
        self.recently_used_toggle_button = DashPanelButton(_("Recently Used"))
        self._init_done = False

        hbox1 = Gtk.Box()
        hbox1.pack_start(self.recently_used_toggle_button, False, False, 0)
        hbox1.pack_start(self.frequently_used_toggle_button, False, False, 0)

        self.frequently_used_toggle_button.button.connect("clicked",
            lambda x: self._toggle_view(self.frequently_used_toggle_button))
        self.recently_used_toggle_button.button.connect("clicked",
            lambda x: self._toggle_view(self.recently_used_toggle_button))

        self.dash_panel_buttons = [self.frequently_used_toggle_button,
            self.recently_used_toggle_button]

        self.view = DashView(self._show_doc, window)

        hbox = Gtk.Box()
        hbox.pack_start(self.view, True, True, 9)

        self.scrolledwindow = scrolledwindow = Gtk.ScrolledWindow()
        scrolledwindow.add_with_viewport(hbox)
        self.connect("style-updated", self.change_style)
        self.search = SearchEntry()
        hbox = Gtk.Box()
        self.toolbar = toolbar = Gtk.Toolbar()
        toolitem = Gtk.ToolItem()
        toolitem.set_expand(True)
        toolbar.insert(toolitem, 0)

        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        vbox.pack_start(self.search, True, True, 6)

        hbox.pack_start(Gtk.Label(""), True, True, 24) # purely cosmetic
        hbox.pack_start(hbox1, True, True, 0)
        hbox.pack_end(vbox, False, False, 9)

        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        vbox.pack_start(hbox, False, False, 1)
        separator = Gtk.Separator(orientation = Gtk.Orientation.HORIZONTAL)
        vbox.pack_start(separator, False, False, 1)
        vbox.pack_start(Gtk.Box(), False, False, 0)
        self.pack_start(vbox, False, False, 0)

        self.dash_box = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        self.dash_box.pack_start(scrolledwindow, True, True, 0)
        self.pack_start(self.dash_box, True, True, 0)

        self.tree_view = ListView()
        self.search_results_scrolled_window = Gtk.ScrolledWindow()
        self.search_results_scrolled_window.add(self.tree_view)

        self.search_result_box = Gtk.Box(orientation =\
            Gtk.Orientation.VERTICAL)
        self.search_result_box.pack_start(self.search_results_scrolled_window,
            True, True, 0)
        self.pack_end(self.search_result_box, True, True, 0)

        self.get_recent()

        self.search.connect("search", self._on_search)
        self.search.connect("clear", self._on_clear)

        self.show_all()
        self.recently_used_toggle_button.set_active(True)
        self.frequently_used_toggle_button.set_active(False)
        self.last_used_button = self.recently_used_toggle_button

        def open_uri(widget, row, cell):
            Gedit.commands_load_location(self._window,
                Gio.file_new_for_uri(widget.model[row][4].uri), None, 0, 0)
        self.tree_view.connect("row-activated", open_uri)
        self.search_result_box.hide()

    def grab_focus(self):
        self.search.grab_focus()

    def _on_search(self, widget, query):
        self.search_result_box.show()
        self.dash_box.hide()
        print "Dashboard search for:", query
        ZG_FTS.search(query, self.tree_view.insert_results)
        self.last_used_button.set_active(False)

    def _on_clear(self, widget):
        self.search_result_box.hide()
        self.dash_box.show()
        self.tree_view.clear()
        self.last_used_button.set_active(True)

    def change_style(self, widget):
        self.style = self.get_style_context()
        viewport = self.scrolledwindow.get_children()[0]
        color = self.style.get_color(Gtk.StateFlags.SELECTED)
        viewport.override_background_color(Gtk.StateFlags.NORMAL, color)
        if self._init_done == False:
            self.search.grab_focus()
            self._init_done = True

    def get_recent(self):
        template = Event()
        template.actor = "application://gedit.desktop"
        CLIENT.find_events_for_templates([template], self.view.populate_grid,
            num_events = 100, result_type = 2)

    def get_frequent(self):
        template = Event()
        template.actor = "application://gedit.desktop"
        now = time.time() * 1000
        # 14 being the amount of days
        # and 86400000 the amount of milliseconds per day
        two_weeks_in_ms = 14 * 86400000
        CLIENT.find_events_for_templates([template], self.view.populate_grid,
            [now - two_weeks_in_ms, now], num_events = 100, result_type = 4)

    def _toggle_view(self, widget):
        for button in self.dash_panel_buttons:
            if button == widget:
                button.set_active(True)
                self.last_used_button = button
                self.search.set_text("")
            else:
                button.set_active(False)
        if self.frequently_used_toggle_button._active:
            self.get_frequent()
        elif self.recently_used_toggle_button._active:
            self.get_recent()

# ex:ts=4:et:
