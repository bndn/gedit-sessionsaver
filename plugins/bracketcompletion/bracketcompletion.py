# -*- coding: utf-8 -*-
#
#  bracketcompletion.py - Bracket completion plugin for gedit
#  
#  Copyright (C) 2006 - Steve Frécinaux
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

import gedit
import gtk
from gtk import gdk

common_brackets = {
    '(' : ')',
    '[' : ']',
    '{' : '}',
    '"' : '"',
    "'" : "'",
}

language_brackets = {
    'changelog': { '<' : '>' },
    'html': { '<' : '>' },
    'ruby': { '|' : '|', 'do': 'end' },
    'sh': { '`' : '`' },
    'xml': { '<' : '>' },
}


class BracketCompletionViewHelper(object):
    def __init__(self, view):
        self._view = view
        self._doc = view.get_buffer()
        self._last_iter = None
        self.update_language()

        self._handlers = [
            None,
            None,
            view.connect('notify::editable', self.on_notify_editable),
            self._doc.connect('notify::language', self.on_notify_language),
        ]
        self.update_active()

    def deactivate(self):
        if self._handlers[0]:
            self._view.disconnect(self._handlers[0])
            self._view.disconnect(self._handlers[1])
        self._view.disconnect(self._handlers[2])
        self._doc.disconnect(self._handlers[3])

    def update_active(self):
        # Don't activate the feature if the buffer isn't editable or if
        # there are no brackets for the language
        active = self._view.get_editable() and \
                 self._brackets is not None

        if active and self._handlers[0] is None:
            self._handlers[0] = self._view.connect('event-after',
                                                   self.on_event_after)
            self._handlers[1] = self._view.connect('key-press-event',
                                                   self.on_key_press_event)
        elif not active and self._handlers[0] is not None:
            self._view.disconnect(self._handlers[0])
            self._handlers[0] = None
            self._view.disconnect(self._handlers[1])
            self._handlers[1] = None

    def update_language(self):
        lang = self._doc.get_language()
        if lang is None:
            self._brackets = None
            return

        lang_id = lang.get_id()
        if lang_id in language_brackets:
            self._brackets = language_brackets[lang_id]
            # we populate the language-specific brackets with common ones lazily
            self._brackets.update(common_brackets)
        else:
            self._brackets = common_brackets

        # get the corresponding keyvals
        self._bracket_keyvals = set()
        for b in self._brackets:
            kv = gtk.gdk.unicode_to_keyval(ord(b[-1]))
            if (kv):
                self._bracket_keyvals.add(kv)

    def get_current_token(self):
        end = self._doc.get_iter_at_mark(self._doc.get_insert())
        start = end.copy()
        word = None

        if end.ends_word() or (end.inside_word() and not end.starts_word()):
            start.backward_word_start()
            word = self._doc.get_text(start, end)

        if not word and start.backward_char():
            word = start.get_char()
            if word.isspace():
                word = None

        if word:
            return word, start, end
        else:
            return None, None, None

    def compute_indentation (self, cur):
        """
        Compute indentation at the given iterator line
        view : gtk.TextView
        cur : gtk.TextIter
        """
        start = self._doc.get_iter_at_line(cur.get_line())
        end = start.copy();

        c = end.get_char()
        while c.isspace() and c not in ('\n', '\r') and end.compare(cur) < 0:
            if not end.forward_char():
                break
            c = end.get_char()

        if start.equal(end):
            return ''
        return start.get_slice(end)

    def on_notify_language(self, view, pspec):
        self.update_language()
        self.update_active()

    def on_notify_editable(self, view, pspec):
        self.update_active()

    def on_key_press_event(self, view, event):
        if not self._last_iter or \
           event.state & (gdk.CONTROL_MASK | gdk.MOD1_MASK):
            return False

        if event.keyval == gtk.keysyms.BackSpace:
            iter = self._doc.get_iter_at_mark(self._doc.get_insert())
            iter.backward_char()
            self._doc.begin_user_action()
            self._doc.delete(iter, self._last_iter)
            self._doc.end_user_action()
            self._last_iter = None
            return True

        if event.keyval in (gtk.keysyms.Return, gtk.keysyms.KP_Enter) and \
           view.get_auto_indent():
            print "return"
            # This code has barely been adapted from gtksourceview.c
            # Note: it might break IM!

            mark = self._doc.get_insert()
            iter = self._doc.get_iter_at_mark(mark)

            indent = self.compute_indentation(iter)
            indent = "\n" + indent
            
            # Insert new line and auto-indent.
            self._doc.begin_user_action()
            self._doc.insert(iter, indent)
            self._doc.insert(iter, indent)                
            self._doc.end_user_action()

            # Leave the cursor where we want it to be
            iter.backward_chars(len(indent))
            self._doc.place_cursor(iter)
            self._view.scroll_mark_onscreen(mark)

            self._last_iter = None
            return True

        self._last_iter = None
        return False

    def on_event_after(self, view, event):
        if event.type != gdk.KEY_PRESS or \
           event.state & (gdk.CONTROL_MASK | gdk.MOD1_MASK) or \
           event.keyval not in self._bracket_keyvals:
            return

        word, start, end = self.get_current_token()

        if word not in self._brackets:
            return

        bracket = self._brackets[word]

        # Insert the closing bracket
        self._doc.begin_user_action()
        self._doc.insert(end, bracket)
        self._doc.end_user_action()
        
        # Leave the cursor when we want it to be
        self._last_iter = end.copy()
        end.backward_chars(len(bracket))
        self._doc.place_cursor(end)


class BracketCompletionPlugin(gedit.Plugin):
    WINDOW_DATA_KEY = "BracketCompletionPluginWindowData"
    VIEW_DATA_KEY = "BracketCompletionPluginViewData"

    def __init__(self):
        super(BracketCompletionPlugin, self).__init__()

    def add_helper(self, view):
        helper = BracketCompletionViewHelper(view)
        view.set_data(self.VIEW_DATA_KEY, helper)
    
    def remove_helper(self, view):
        view.get_data(self.VIEW_DATA_KEY).deactivate()
        view.set_data(self.VIEW_DATA_KEY, None)
                    
    def activate(self, window):
        for view in window.get_views():
            self.add_helper(view)

        added_hid = window.connect("tab-added",
                                   lambda w, t: self.add_helper(t.get_view()))
        removed_hid = window.connect("tab-removed",
                                     lambda w, t: self.remove_helper(t.get_view()))
        window.set_data(self.WINDOW_DATA_KEY, (added_hid, removed_hid))

    def deactivate(self, window):
        handlers = window.get_data(self.WINDOW_DATA_KEY)
        for handler_id in handlers:
            window.disconnect(handler_id)
        window.set_data(self.WINDOW_DATA_KEY, None)

        for view in window.get_views():
            self.remove_helper(view)

    def update_ui(self, window):
        pass

# ex:ts=4:et:
