# -*- coding: utf-8 -*-
#  Code comment plugin
#  This file is part of gedit
# 
#  Copyright (C) 2005-2006 Igalia
#  Copyright (C) 2006 Matthew Dugan
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
import copy
from gettext import gettext as _

# Most common types of comments (store them only once)
ccomment    = ('/* ', ' */')
cppcomment  = ('// ', None)
adacomment  = ('-- ', None)
xmlcomment  = ('<!-- ', ' -->')
shcomment   = ('# ', None)
texcomment  = ('% ', None)
pascomment  = ('(* ', ' *)')
lispcomment = ('; ', None)

tags_dict = { '@46@desktop'     : shcomment,
              '@46@ini'         : lispcomment,
              'Ada'             : adacomment,
              'Boo'             : shcomment,
              'C'               : ccomment,
              'C@35@'           : cppcomment,
              'C@43@@43@'       : cppcomment,
              'CSS'             : ccomment,
              'D'               : cppcomment,
              'Fortran@32@95'   : ('! ', None),
              'GtkRC'           : shcomment,
              'HTML'            : xmlcomment,
              'Haskell'         : adacomment,
              'IDL'             : cppcomment,
              'Java'            : cppcomment,
              'JavaScript'      : cppcomment,
              'LaTeX'           : texcomment,
              'Lua'             : adacomment,
              'MSIL'            : cppcomment,
              'Makefile'        : shcomment,
              'Nemerle'         : cppcomment,
              'Objective@32@Caml': pascomment,
              'Octave'          : texcomment,
              'PHP'             : shcomment,
              'Pascal'          : pascomment,
              'Perl'            : shcomment,
              'Python'          : shcomment,
              'R'               : shcomment,
              'Ruby'            : shcomment,
              'SQL'             : adacomment,
              'Scheme'          : lispcomment,
              'Tcl'             : shcomment,
              'VB@46@NET'       : ("' ", None),
              'VHDL'            : adacomment,
              'Verilog'         : cppcomment,
              'XML'             : xmlcomment,
              'gettext@32@translation': shcomment,
              'sh'              : shcomment 
            }



def forward_tag(iter, tag):
    iter.forward_chars(len(tag))

def backward_tag(iter, tag):
    iter.backward_chars(len(tag))

def get_tag_position_in_line(tag, head_iter, iter):
    found = False
    while (not iter.ends_line()) and (not found):
        s = iter.get_slice(head_iter)
        if s == tag:
           found = True
        else:
            head_iter.forward_char()
            iter.forward_char()
    return found

def add_comment_characters(document, start_tag, end_tag, start, end):
    smark = document.create_mark("start", start, False)    
    imark = document.create_mark("iter", start, False)
    emark = document.create_mark("end", end, False)
    number_lines = end.get_line() - start.get_line() + 1

    document.begin_user_action()

    for i in range(0, number_lines):
        iter = document.get_iter_at_mark(imark)
        if not iter.ends_line():
            document.insert(iter, start_tag, -1)
            if end_tag is not None:
                if i != number_lines -1:
                    iter = document.get_iter_at_mark(imark)                
                    iter.forward_to_line_end()
                    document.insert(iter, end_tag, -1)
                else:
                    iter = document.get_iter_at_mark(emark)
                    document.insert(iter, end_tag, -1)
        iter = document.get_iter_at_mark(imark)
        iter.forward_line()
        document.delete_mark(imark)
        imark = document.create_mark("iter", iter, True)

    document.end_user_action()

    document.delete_mark(imark)        
    new_start = document.get_iter_at_mark(smark)
    new_end = document.get_iter_at_mark(emark)
    if not new_start.ends_line():
        backward_tag(new_start, start_tag)
    document.select_range(new_start, new_end)
    document.delete_mark(smark)
    document.delete_mark(emark)    
    
def remove_comment_characters(document, start_tag, end_tag, start, end):
    smark = document.create_mark("start", start, False)
    emark = document.create_mark("end", end, False)
    number_lines = end.get_line() - start.get_line() + 1
    iter = start.copy()
    head_iter = iter.copy()
    forward_tag(head_iter, start_tag)

    document.begin_user_action()

    for i in range(0, number_lines):
        if get_tag_position_in_line(start_tag, head_iter, iter):
            dmark = document.create_mark("delete", iter, False)
            document.delete(iter, head_iter)
            if end_tag is not None:
                iter = document.get_iter_at_mark(dmark)
                head_iter = iter.copy()
                forward_tag(head_iter, end_tag)
                if get_tag_position_in_line(end_tag, head_iter, iter):
                    document.delete(iter, head_iter)
            document.delete_mark(dmark)
        iter = document.get_iter_at_mark(smark)
        iter.forward_line()
        document.delete_mark(smark)
        head_iter = iter.copy()
        forward_tag(head_iter, start_tag)
        smark = document.create_mark("iter", iter, True)

    document.end_user_action()

    document.delete_mark(smark)
    document.delete_mark(emark)

def do_comment(document, unindent=False):
    selection = document.get_selection_bounds()
    currentPosMark = document.get_insert()
    deselect = False
    if selection != ():
        (start, end) = selection
    else:
        deselect = True        
        start = document.get_iter_at_mark(currentPosMark)
        if start.ends_line():
            return
        end = start.copy()
        if not end.forward_to_line_end():
            return

    lang = document.get_language()
    if lang is None:
        return

    lang_id = lang.get_id()
    if tags_dict.has_key(lang):
        (start_tag, end_tag) = tags_dict[lang]
        if unindent:       # Select the comment or the uncomment method
            new_code = remove_comment_characters(document,
                                                 start_tag,
                                                 end_tag,
                                                 start,
                                                 end)
        else:
            new_code = add_comment_characters(document,
                                              start_tag,
                                              end_tag,
                                              start,
                                              end)

        if deselect:
            oldPosIter = document.get_iter_at_mark(currentPosMark)
            document.select_range(oldPosIter,oldPosIter)
            document.place_cursor(oldPosIter)    

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

class CodeCommentWindowHelper(object):
    def __init__(self, plugin, window):
        self._window = window
        self._plugin = plugin
        self._insert_menu()

    def deactivate(self):
        self._remove_menu()
        self._action_group = None
        self._window = None
        self._plugin = None

    def _insert_menu(self):
        manager = self._window.get_ui_manager()
        self._action_group = gtk.ActionGroup("CodeCommentActions")
        self._action_group.add_actions([("CodeComment",
                                         None,
                                         _("Co_mment Code"),
                                         "<control>M",
                                         _("Comment the selected code"),
                                         lambda a, w: do_comment (w.get_active_document())),
                                        ('CodeUncomment',
                                         None,
                                         _('U_ncomment Code'),
                                         "<control><shift>M",
                                         _("Uncomment the selected code"),
                                         lambda a, w: do_comment (w.get_active_document(), True))],
                                        self._window)

        manager.insert_action_group(self._action_group, -1)
        self._ui_id = manager.add_ui_from_string(ui_str)

    def _remove_menu(self):
        manager = self._window.get_ui_manager()
        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._action_group)
        manager.ensure_update()

    def update_ui(self):
        doc = self._window.get_active_document()
        if doc:
            lang = doc.get_language()
            if lang is not None:
                self._action_group.set_sensitive(tags_dict.has_key(lang.get_id()))
        else:
            self._action_group.set_sensitive(False)


class CodeCommentPlugin(gedit.Plugin):
    DATA_TAG = "CodeCommentPluginInstance"

    def __init__(self):
        gedit.Plugin.__init__(self)

    def activate(self, window):
        window.set_data(self.DATA_TAG, CodeCommentWindowHelper(self, window))

    def deactivate(self, window):
        window.get_data(self.DATA_TAG).deactivate()
        window.set_data(self.DATA_TAG, None)

    def update_ui(self, window):
        window.get_data(self.DATA_TAG).update_ui()

# ex:ts=4:et:
