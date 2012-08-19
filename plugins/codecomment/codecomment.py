# -*- coding: utf-8 -*-
#  Code comment plugin
#  This file is part of gedit
#
#  Copyright (C) 2005-2006 Igalia
#  Copyright (C) 2006 Matthew Dugan
#  Copyrignt (C) 2007 Steve Frécinaux
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

from gi.repository import GObject, Gtk, GtkSource, Gedit
import copy
import gettext
from gpdefs import *

try:
    gettext.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    _ = lambda s: gettext.dgettext(GETTEXT_PACKAGE, s);
except:
    _ = lambda s: s

# If the language is listed here we prefer block comments over line comments.
# Maybe this list should be user configurable, but just C comes to my mind...
block_comment_languages = [
    'c',
]

ui_str = """
<ui>
  <menubar name="MenuBar">
    <menu name="EditMenu" action="Edit">
      <placeholder name="EditOps_4">
            <menuitem name="Comment" action="CodeComment"/>
            <menuitem name="Uncomment" action="CodeUncomment"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class CodeCommentPlugin(GObject.Object, Gedit.WindowActivatable):
    __gtype_name__ = "CodeCommentPlugin"

    window = GObject.property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self._insert_menu()

    def do_deactivate(self):
        self._remove_menu()

    def do_update_state(self):
        sensitive = False
        doc = self.window.get_active_document()
        if doc:
            lang = doc.get_language()
            if lang is not None:
                sensitive = self.get_comment_tags(lang) != (None, None)
        self._action_group.set_sensitive(sensitive)

    def _remove_menu(self):
        manager = self.window.get_ui_manager()
        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._action_group)
        manager.ensure_update()

    def _insert_menu(self):
        manager = self.window.get_ui_manager()
        self._action_group = Gtk.ActionGroup(name="GeditCodeCommentPluginActions")
        self._action_group.add_actions([("CodeComment",
                                         None,
                                         _("Co_mment Code"),
                                         "<control>M",
                                         _("Comment the selected code"),
                                         lambda a, w: self.do_comment (w.get_active_document())),
                                        ('CodeUncomment',
                                         None,
                                         _('U_ncomment Code'),
                                         "<control><shift>M",
                                         _("Uncomment the selected code"),
                                         lambda a, w: self.do_comment (w.get_active_document(), True))],
                                        self.window)

        manager.insert_action_group(self._action_group)
        self._ui_id = manager.add_ui_from_string(ui_str)

    def get_block_comment_tags(self, lang):
        start_tag = lang.get_metadata('block-comment-start')
        end_tag = lang.get_metadata('block-comment-end')
        if start_tag and end_tag:
            return (start_tag, end_tag)
        return (None, None)

    def get_line_comment_tags(self, lang):
        start_tag = lang.get_metadata('line-comment-start')
        if start_tag:
            return (start_tag, None)
        return (None, None)

    def get_comment_tags(self, lang):
        if lang.get_id() in block_comment_languages:
            (s, e) = self.get_block_comment_tags(lang)
            if (s, e) == (None, None):
                (s, e) = self.get_line_comment_tags(lang)
        else:
            (s, e) = self.get_line_comment_tags(lang)
            if (s, e) == (None, None):
                (s, e) = self.get_block_comment_tags(lang)
        return (s, e)

    def forward_tag(self, iter, tag):
        iter.forward_chars(len(tag))

    def backward_tag(self, iter, tag):
        iter.backward_chars(len(tag))

    def get_tag_position_in_line(self, tag, head_iter, iter):
        found = False
        while (not found) and (not iter.ends_line()):
            s = iter.get_slice(head_iter)
            if s == tag:
                found = True
            else:
                head_iter.forward_char()
                iter.forward_char()
        return found

    def add_comment_characters(self, document, start_tag, end_tag, start, end):
        smark = document.create_mark("start", start, False)
        imark = document.create_mark("iter", start, False)
        emark = document.create_mark("end", end, False)
        number_lines = end.get_line() - start.get_line() + 1

        document.begin_user_action()

        for i in range(0, number_lines):
            iter = document.get_iter_at_mark(imark)
            if not iter.ends_line():
                document.insert(iter, start_tag)
                if end_tag is not None:
                    if i != number_lines -1:
                        iter = document.get_iter_at_mark(imark)
                        iter.forward_to_line_end()
                        document.insert(iter, end_tag)
                    else:
                        iter = document.get_iter_at_mark(emark)
                        document.insert(iter, end_tag)
            iter = document.get_iter_at_mark(imark)
            iter.forward_line()
            document.delete_mark(imark)
            imark = document.create_mark("iter", iter, True)

        document.end_user_action()

        document.delete_mark(imark)
        new_start = document.get_iter_at_mark(smark)
        new_end = document.get_iter_at_mark(emark)
        if not new_start.ends_line():
            self.backward_tag(new_start, start_tag)
        document.select_range(new_start, new_end)
        document.delete_mark(smark)
        document.delete_mark(emark)

    def remove_comment_characters(self, document, start_tag, end_tag, start, end):
        smark = document.create_mark("start", start, False)
        emark = document.create_mark("end", end, False)
        number_lines = end.get_line() - start.get_line() + 1
        iter = start.copy()
        head_iter = iter.copy()
        self.forward_tag(head_iter, start_tag)

        document.begin_user_action()

        for i in range(0, number_lines):
            if self.get_tag_position_in_line(start_tag, head_iter, iter):
                dmark = document.create_mark("delete", iter, False)
                document.delete(iter, head_iter)
                if end_tag is not None:
                    iter = document.get_iter_at_mark(dmark)
                    head_iter = iter.copy()
                    self.forward_tag(head_iter, end_tag)
                    if self.get_tag_position_in_line(end_tag, head_iter, iter):
                        document.delete(iter, head_iter)
                document.delete_mark(dmark)
            iter = document.get_iter_at_mark(smark)
            iter.forward_line()
            document.delete_mark(smark)
            head_iter = iter.copy()
            self.forward_tag(head_iter, start_tag)
            smark = document.create_mark("iter", iter, True)

        document.end_user_action()

        document.delete_mark(smark)
        document.delete_mark(emark)

    def do_comment(self, document, unindent=False):
        sel = document.get_selection_bounds()
        currentPosMark = document.get_insert()
        deselect = False
        if sel != ():
            (start, end) = sel
            if start.ends_line():
                start.forward_line()
            elif not start.starts_line():
                start.set_line_offset(0)
            if end.starts_line():
                end.backward_char()
            elif not end.ends_line():
                end.forward_to_line_end()
        else:
            deselect = True
            start = document.get_iter_at_mark(currentPosMark)
            start.set_line_offset(0)
            end = start.copy()
            end.forward_to_line_end()

        lang = document.get_language()
        if lang is None:
            return

        (start_tag, end_tag) = self.get_comment_tags(lang)

        if not start_tag and not end_tag:
            return

        if unindent:       # Select the comment or the uncomment method
            new_code = self.remove_comment_characters(document,
                                                      start_tag,
                                                      end_tag,
                                                      start,
                                                      end)
        else:
            new_code = self.add_comment_characters(document,
                                                   start_tag,
                                                   end_tag,
                                                   start,
                                                   end)

        if deselect:
            oldPosIter = document.get_iter_at_mark(currentPosMark)
            document.select_range(oldPosIter,oldPosIter)
            document.place_cursor(oldPosIter)

# ex:ts=4:et:
