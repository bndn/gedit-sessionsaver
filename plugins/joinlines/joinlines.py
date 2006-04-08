# -*- coding: utf-8 -*-
#  Join lines plugin
#  This file is part of gedit
# 
#  Copyright (C) 2006 Steve Frécinaux
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

# Bug 319938:
# It would be great if there were a menu function to allow joining of lines in 
# gedit. I've had use of this feature in kate, vim and TextPad and it's really 
# useful: 
# - ctrl-J, no text selected: remove leading whitespace from next line, 
#   replace newline at end of current line with a space. 
# - ctrl-J, with selected text: remove leading whitespace from all but the first
#   line, replace newlines on all but the last line with a space.
# - ctrl-shift-J, with selected text: remove all leading whitespace from lines
#   not preceded by a whitespace-only line. replace all newlines not followed
#   by a whitespace-only line with a space.

import gedit, gtk
from gettext import gettext as _

ui_str = """
<ui>
  <menubar name="MenuBar">
    <menu name="EditMenu" action="Edit">
      <placeholder name="EditOps_5">
        <menuitem name="JoinLines" action="JoinLines"/>
        <menuitem name="SplitLines" action="SplitLines"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class JoinLinesPlugin(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)
					
	def activate(self, window):
		manager = window.get_ui_manager()
		data = dict()

		data["action_group"] = gtk.ActionGroup("GeditJoinLinesPluginActions")
		data["action_group"].add_actions(
			[("JoinLines", None, _("_Join Lines"), "<Ctrl>J",
			  _("Join the selected lines"),
			  lambda a, w: join_lines(w)),
			 ("SplitLines", None, _('_Split Lines'), "<Shift><Ctrl>J",
			  _("Split the selected lines"),
			  lambda a, w: split_lines(w))],
			window)

		manager.insert_action_group(data["action_group"], -1)
		data["ui_id"] = manager.add_ui_from_string(ui_str)

		window.set_data("JoinLinesPluginInfo", data)
		update_sensitivity(window)
	
	def deactivate(self, window):
		data = window.get_data("JoinLinesPluginInfo")		
		manager = window.get_ui_manager()		
		manager.remove_ui(data["ui_id"])
		manager.remove_action_group(data["action_group"])
		manager.ensure_update()
		window.set_data("JoinLinesPluginInfo", None)
		
	def update_ui(self, window):
		update_sensitivity(window)
			
def update_sensitivity(window):
	data = window.get_data("JoinLinesPluginInfo")
	view = window.get_active_view()
	data["action_group"].set_sensitive(view is not None and \
	                                   view.get_editable())

def join_lines(window):
	document = window.get_active_document()
	if document is None:
		return
	
	document.begin_user_action()
	
	try:
		start, end = document.get_selection_bounds()
	except ValueError:
		start, end = document.get_bounds()
	
	end_mark = document.create_mark(None, end)
	
	while document.get_iter_at_mark(end_mark).compare(start) == 1:
		start.forward_to_line_end()

		end = start.copy()
		c = end.get_char()
		if c == '\r':
			end.forward_char()
			c = end.get_char()
		if c == '\n':
			end.forward_char()
			c = end.get_char()

		# remove blank chars		
		while end.get_char() in (' ', '\t'):
			end.forward_char()
		
		document.delete(start, end)

		# let the carriage return there if there are more than one:
		while end.get_char() in ('\r', '\n'):
			end.forward_char()
		else:
			document.insert(start, ' ')
	
	document.delete_mark(end_mark)
	document.end_user_action()			

def split_lines(window):
	view = window.get_active_view()
	if view is None:
		return

	document = view.get_buffer()
	tabsize = view.get_tabs_width()

	document.begin_user_action()

	try:
		start, end = document.get_selection_bounds()
	except ValueError:
		start, end = document.get_bounds()

	end_mark = document.create_mark(None, end)
	width = window.get_active_view().get_margin()

	# split lines
	pos = 0
	while document.get_iter_at_mark(end_mark).compare(start) == 1:
		start.forward_char()
		char = start.get_char()

		if char in ('\r','\n'):
			pos = 0
		elif char == '\t':
			pos = pos + tabsize
		else:
			pos = pos + 1

		if pos >= width:
			pos = 0
			if start.inside_word() and not start.starts_word():
				start.backward_word_start()
			document.insert(start, '\n')

	document.delete_mark(end_mark)
	document.end_user_action()
