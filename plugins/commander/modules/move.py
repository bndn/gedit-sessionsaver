# -*- coding: utf-8 -*-
#
#  move.py - move commander module
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

import commander.commands as commands
from gi.repository import Gdk, Gtk
import re

__commander_module__ = True

def _move(view, what, num, modifier):
    try:
        num = int(num)
    except:
        raise commands.exceptions.Execute('Invalid number: ' + str(num))

    view.emit('move-cursor', what, num, modifier & Gdk.EventMask.CONTROL_MASK)
    return commands.result.HIDE

def word(view, modifier, num=1):
    """Move cursor per word: move.word &lt;num&gt;

Move the cursor per word (use negative num to move backwards)"""
    return _move(view, Gtk.MovementStep.WORDS, num, modifier)

def line(view, modifier, num=1):
    """Move cursor per line: move.line &lt;num&gt;

Move the cursor per line (use negative num to move backwards)"""
    return _move(view, Gtk.MovementStep.DISPLAY_LINES, num, modifier)

def char(view, modifier, num=1):
    """Move cursor per char: move.char &lt;num&gt;

Move the cursor per char (use negative num to move backwards)"""
    return _move(view, Gtk.MovementStep.VISUAL_POSITIONS, num, modifier)

def paragraph(view, modifier, num=1):
    """Move cursor per paragraph: move.paragraph &lt;num&gt;

Move the cursor per paragraph (use negative num to move backwards)"""
    return _move(view, Gtk.MovementStep.PARAGRAPHS, num, modifier)

def regex(view, modifier, regex, num=1):
    """Move cursor per regex: move.regex &lt;num&gt;

Move the cursor per regex (use negative num to move backwards)"""
    try:
        r = re.compile(regex, re.DOTALL | re.MULTILINE | re.UNICODE)
    except Exception, e:
        raise commands.exceptions.Execute('Invalid regular expression: ' + str(e))

    try:
        num = int(num)
    except Exception, e:
        raise commands.exceptions.Execute('Invalid number: ' + str(e))

    buf = view.get_buffer()
    start = buf.get_iter_at_mark(buf.get_insert())

    if num > 0:
        end = buf.get_end_iter()
    else:
        end = start.copy()
        start = buf.get_start_iter()

    text = start.get_text(end)
    ret = list(r.finditer(text))

    if num < 0:
        ret.reverse()

    idx = min(abs(num), len(ret))

    if idx > 0:
        found = ret[idx - 1]
        start = buf.get_iter_at_mark(buf.get_insert())

        if num < 0:
            start.backward_char(len(text) - found.start(0))
        else:
            start.forward_chars(found.start(0))

        if modifier & Gdk.EventMask.CONTROL_MASK:
            buf.move_mark(buf.get_selection_bound(), start)
        else:
            buf.move_mark(buf.get_insert(), start)
            buf.move_mark(buf.get_selection_bound(), start)

        visible = view.get_visible_rect()
        loc = view.get_iter_location(start)

        # Scroll there if needed
        if loc.y + loc.height < visible.y or loc.y > visible.y + visible.height:
            view.scroll_to_mark(buf.get_insert(), 0.2, True, 0, 0.5)

    return commands.result.HIDE

# vi:ex:ts=4:et
