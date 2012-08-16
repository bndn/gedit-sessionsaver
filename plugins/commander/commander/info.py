# -*- coding: utf-8 -*-
#
#  info.py - commander
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
#  Foundation, Inc., 59 Temple Place, Suite 330,
#  Boston, MA 02111-1307, USA.

from transparentwindow import TransparentWindow
from gi.repository import Pango, Gdk, Gtk
import math

class Info(TransparentWindow):
    __gtype_name__ = "CommanderInfo"

    def __init__(self, entry):
        super(Info, self).__init__(Gtk.WindowType.POPUP)

        self._entry = entry
        self._vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)

        self.set_transient_for(entry.get_toplevel())

        self._vw = Gtk.ScrolledWindow()
        self._vw.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.NEVER)
        self._vw.show()

        self._text = Gtk.TextView()

        font = self._entry.get_font()
        fgcolor = self._entry.get_foreground_color()
        bgcolor = self.background_color()

        self._text.override_font(font)
        self._text.override_color(Gtk.StateFlags.NORMAL, fgcolor)
        self._text.override_background_color(Gtk.StateFlags.NORMAL, bgcolor)

        self._text.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)

        buf = self._text.get_buffer()

        buf.connect_after('insert-text', self.on_text_insert_text)
        buf.connect_after('delete-range', self.on_text_delete_range)

        self._text.set_editable(False)

        self._vw.add(self._text)
        self._vbox.pack_end(self._vw, False, False, 0)
        self._vbox.show()
        self._button_bar = None

        self.add(self._vbox)
        self._text.show()
        self._status_label = None

        self.props.can_focus = False
        self.set_border_width(8)

        self.attach()
        self.show()

        self.max_lines = 10

        self.connect_after('size-allocate', self.on_size_allocate)
        self.connect_after('draw', self.on_draw)

        self._attr_map = {
            Pango.AttrType.STYLE: 'style',
            Pango.AttrType.WEIGHT: 'weight',
            Pango.AttrType.VARIANT: 'variant',
            Pango.AttrType.STRETCH: 'stretch',
            Pango.AttrType.SIZE: 'size',
            Pango.AttrType.FOREGROUND: 'foreground',
            Pango.AttrType.BACKGROUND: 'background',
            Pango.AttrType.UNDERLINE: 'underline',
            Pango.AttrType.STRIKETHROUGH: 'strikethrough',
            Pango.AttrType.RISE: 'rise',
            Pango.AttrType.SCALE: 'scale'
        }

    def empty(self):
        buf = self._text.get_buffer()
        return buf.get_start_iter().equal(buf.get_end_iter())

    def status(self, text=None):
        if self._status_label == None and text != None:
            self._status_label = Gtk.Label()

            context = self._text.get_style_context()
            state = context.get_state()
            font_desc = context.get_font(state)

            self._status_label.override_font(font_desc)

            self._status_label.override_color(Gtk.StateFlags.NORMAL, context.get_color(Gtk.StateFlags.NORMAL))
            self._status_label.show()
            self._status_label.set_alignment(0, 0.5)
            self._status_label.set_padding(10, 0)
            self._status_label.set_use_markup(True)

            self.ensure_button_bar()
            self._button_bar.pack_start(self._status_label, True, True, 0)

        if text != None:
            self._status_label.set_markup(text)
        elif self._status_label:
            self._status_label.destroy()

            if not self._button_bar and self.empty():
                self.destroy()

    def attrs_to_tags(self, attrs):
        buf = self._text.get_buffer()
        table = buf.get_tag_table()
        ret = []

        for attr in attrs:
            if not attr.type in self._attr_map:
                continue

            if attr.type == Pango.AttrType.FOREGROUND or attr.type == Pango.AttrType.BACKGROUND:
                value = attr.color
            else:
                value = attr.value

            tagname = str(attr.type) + ':' + str(value)

            tag = table.lookup(tagname)

            if not tag:
                tag = buf.create_tag(tagname)
                tag.set_property(self._attr_map[attr.type], value)

            ret.append(tag)

        return ret

    def add_lines(self, line, use_markup=False):
        buf = self._text.get_buffer()

        if not buf.get_start_iter().equal(buf.get_end_iter()):
            line = "\n" + line

        if not use_markup:
            buf.insert(buf.get_end_iter(), line)
            return

        try:
            ret = Pango.parse_markup(line, -1, u'\x00')
        except Exception, e:
            print 'Could not parse markup:', e
            buf.insert(buf.get_end_iter(), line)
            return

        # TODO: fix this when pango supports get_iterator
        text = ret[2]
        buf.insert(buf.get_end_iter(), text)

        return

        piter = ret[1].get_iterator()

        while piter:
            attrs = piter.get_attrs()
            begin, end = piter.range()

            tags = self.attrs_to_tags(attrs)
            buf.insert_with_tags(buf.get_end_iter(), text[begin:end], *tags)

            if not piter.next():
                break

    def toomany_lines(self):
        buf = self._text.get_buffer()
        piter = buf.get_start_iter()
        num = 0

        while self._text.forward_display_line(piter):
            num += 1

            if num > self.max_lines:
                return True

        return False

    def contents_changed(self):
        buf = self._text.get_buffer()

        if self.toomany_lines() and (self._vw.get_policy()[1] != Gtk.PolicyType.ALWAYS):
            self._vw.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.ALWAYS)

            layout = self._text.create_pango_layout('Some text to measure')
            extents = layout.get_pixel_extents()

            self._vw.set_min_content_height(extents[1].height * self.max_lines)
        elif not self.toomany_lines() and (self._vw.get_policy()[1] == Gtk.PolicyType.ALWAYS):
            self._vw.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.NEVER)
            self._vw.set_min_content_height(0)

        if not self.toomany_lines():
            size = self.get_size()
            self.resize(size[0], 1)

    def ensure_button_bar(self):
        if not self._button_bar:
            self._button_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=3)
            self._button_bar.show()
            self._vbox.pack_start(self._button_bar, False, False, 0)

    def add_action(self, stock, callback, data=None):
        image = Gtk.Image.new_from_stock(stock, Gtk.IconSize.MENU)
        image.show()

        image.commander_action_stock_item = (stock, Gtk.IconSize.MENU)

        self.ensure_button_bar()

        ev = Gtk.EventBox()
        ev.set_visible_window(False)
        ev.add(image)
        ev.show()

        self._button_bar.pack_end(ev, False, False, 0)

        ev.connect('button-press-event', self.on_action_activate, callback, data)
        ev.connect('enter-notify-event', self.on_action_enter_notify)
        ev.connect('leave-notify-event', self.on_action_leave_notify)

        ev.connect_after('destroy', self.on_action_destroy)
        return ev

    def on_action_destroy(self, widget):
        if self._button_bar and len(self._button_bar.get_children()) == 0:
            self._button_bar.destroy()
            self._button_bar = None

    def on_action_enter_notify(self, widget, evnt):
        img = widget.get_child()
        img.set_state(Gtk.StateType.PRELIGHT)
        widget.get_window().set_cursor(Gdk.Cursor.new(Gdk.HAND2))

        pix = img.render_icon(*img.commander_action_stock_item)
        img.set_from_pixbuf(pix)

    def on_action_leave_notify(self, widget, evnt):
        img = widget.get_child()
        img.set_state(Gtk.StateType.NORMAL)
        widget.get_window().set_cursor(None)

        pix = img.render_icon(*img.commander_action_stock_item)
        img.set_from_pixbuf(pix)

    def on_action_activate(self, widget, evnt, callback, data):
        if data:
            callback(data)
        else:
            callback()

    def clear(self):
        self._text.get_buffer().set_text('')

    def attach(self):
        vwwnd = self._entry._view.get_window(Gtk.TextWindowType.TEXT)
        origin = vwwnd.get_origin()
        geom = vwwnd.get_geometry()

        margin = 5

        self.realize()

        alloc = self.get_allocation()

        self.move(origin[1], origin[2] + geom[3] - alloc.height)
        self.resize(geom[2] - margin * 2, alloc.height)

    def on_text_insert_text(self, buf, piter, text, length):
        self.contents_changed()

    def on_text_delete_range(self, buf, start, end):
        self.contents_changed()

    def on_size_allocate(self, widget, allocation):
        vwwnd = self._entry.view().get_window(Gtk.TextWindowType.TEXT)
        origin = vwwnd.get_origin()
        geom = vwwnd.get_geometry()

        alloc = self.get_allocation()

        self.move(origin[1] + (geom[2] - alloc.width) / 2, origin[2] + geom[3] - alloc.height)

    def on_draw(self, widget, ct):
        color = self._entry.get_border_color()

        self.background_shape(ct, self.get_allocation())

        ct.set_source_rgba(color.red, color.green, color.blue, color.alpha)
        ct.stroke()

        return False

    def background_shape(self, ct, alloc):
        w = alloc.width
        h = alloc.height

        ct.set_line_width(1)
        radius = 10

        ct.move_to(0.5, h)

        if self.is_composited():
            ct.arc(radius + 0.5, radius + 0.5, radius, math.pi, math.pi * 1.5)
            ct.arc(w - radius - 0.5, radius + 0.5, radius, math.pi * 1.5, math.pi * 2)
        else:
            ct.line_to(0.5, 0)
            ct.line_to(w - 0.5, 0)

        ct.line_to(w - 0.5, h)

    def background_color(self):
        color = self._entry.get_background_color().copy()
        color.alpha = 0.9

        return color

    def on_text_size_allocate(self, widget, alloc):
        pass

# vi:ex:ts=4:et
