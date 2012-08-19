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

from gi.repository import GObject, Gtk, Gio, GLib, GdkPixbuf, GtkSource
import os
import urllib

thumb_cache = {}
pixbuf_cache = {}

ICON_SIZES = SIZE_NORMAL, SIZE_LARGE = ((128, 128), (256, 256))

# Thumbview and Timelineview sizes
SIZE_THUMBVIEW = [(48, 36), (96, 72), (150, 108), (256, 192)]
SIZE_TIMELINEVIEW = [(16, 12), (32, 24), (64, 48), (128, 96)]
SIZE_TEXT_TIMELINEVIEW = ["small", "medium", "large", "x-large"]
SIZE_TEXT_THUMBVIEW = ["x-small", "small", "large", "xx-large"]

GRID_ITEM_COUNT = 7

no_pixbuf = Gtk.IconTheme.get_default().load_icon("gtk-file", 48, 0)


def uri_exists(uri):
    return uri.startswith("note://") or \
        os.path.exists(urllib.unquote(str(uri[7:])))


def make_icon_frame(pixbuf, blend=False, border=1, color=0x000000ff):
    """creates a new Pixmap which contains 'pixbuf' with a border at given
    size around it."""
    if not pixbuf:
        return pixbuf
    result = GdkPixbuf.Pixbuf.new(pixbuf.get_colorspace(), True,
        pixbuf.get_bits_per_sample(), pixbuf.get_width(),
        pixbuf.get_height())
    result.fill(color)
    pixbuf.copy_area(border, border, pixbuf.get_width() - (border * 2),
        pixbuf.get_height() - (border * 2), result, border, border)
    return result


def create_text_thumb(gfile, size=None, threshold=2):
    """ tries to use source_view to get a thumbnail of a text file """
    content = gfile.get_content().split("\n")
    new_content = []
    last_line = ""
    for line in content:
        x = line.strip()
        if not (x.startswith("/*") or x.startswith("*")\
            or x.startswith("#") or x.startswith("import ")\
            or x.startswith("from ") or x.startswith("#include ")\
            or x.startswith("#define") or x.startswith("using")\
            or x == "") and not last_line.endswith("\\"):
            new_content.append(line)
        else:
            last_line = line
    missing_lines = 30 - len(new_content)
    if missing_lines > 0:
        for i in xrange(missing_lines):
            new_content.append(" \n")
    content = "\n".join(new_content)
    lang_manager = GtkSource.LanguageManager.get_default()
    lang = lang_manager.guess_language(gfile.subject.uri[7:], None)
    if lang:
        buf = GtkSource.Buffer.new_with_language(lang)
    else:
        buf = GtkSource.Buffer()
    buf.set_highlight_syntax(True)
    buf.set_text(content)
    source_view = GtkSource.View.new_with_buffer(buf)
    overlay = Gtk.Overlay()
    overlay.add(Gtk.EventBox())
    overlay.add_overlay(source_view)
    event_box = Gtk.EventBox()
    event_box.add(overlay)
    return event_box


def __crop_pixbuf(pixbuf, x, y, size=SIZE_LARGE):
    """ returns a part of the given pixbuf as new one """
    result = GdkPixbuf.Pixbuf.new(pixbuf.get_colorspace(),
        True,
        pixbuf.get_bits_per_sample(),
        size[0], size[1])
    pixbuf.copy_area(x, y, x + size[0], y + size[1], result, 0, 0)
    return result


def get_pixbuf_from_cache(gicon, callback, size=48):
    name = gicon.to_string()
    if name in pixbuf_cache:
        callback(pixbuf_cache[name])
    else:
        theme = Gtk.IconTheme.get_default()
        # blocks: do something with it
        gtk_icon = theme.lookup_by_gicon(gicon, size,
            Gtk.IconLookupFlags.FORCE_SIZE)
        if gtk_icon:
            icon = gtk_icon.load_icon()
        else:
            icon = None
        pixbuf_cache[name] = icon
        callback(icon)


def get_icon(item, callback, size=48):
    uri = item.uri
    f = Gio.file_new_for_uri(uri)

    def query_info_callback(gfile, result, cancellable):
        info = f.query_info_finish(result)

        def pixbuf_loaded_cb(pixbuf):
            if pixbuf is None:
                return
            icon = pixbuf.scale_simple(size * pixbuf.get_width()\
                / pixbuf.get_height(), size, 2)
            callback(icon)


        gicon = info.get_icon()
        get_pixbuf_from_cache(gicon, pixbuf_loaded_cb, size)


    if f.is_native():
        attr = Gio.FILE_ATTRIBUTE_STANDARD_ICON + ","\
            + Gio.FILE_ATTRIBUTE_THUMBNAIL_PATH

        def cancel_cb():
            pass

        f.query_info_async(attr, Gio.FileQueryInfoFlags.NONE,
            GLib.PRIORITY_DEFAULT, None, query_info_callback, None)



