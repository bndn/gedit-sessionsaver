# -*- coding: utf-8 -*-
#
#  entry.py - commander
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

from gi.repository import GObject, GLib, Gdk, Gtk
import cairo
import os
import re
import inspect
import sys

import commander.commands as commands
import commands.completion
import commands.module
import commands.method
import commands.exceptions
import commands.accel_group

import commander.utils as utils

from history import History
from info import Info
from xml.sax import saxutils
import traceback

class Entry(Gtk.EventBox):
	__gtype_name__ = "CommanderEntry"

	def __init__(self, view):
		Gtk.EventBox.__init__(self)
		self._view = view

		self.set_visible_window(False)

		hbox = Gtk.Box.new(Gtk.Orientation.HORIZONTAL, 3)
		hbox.show()
		hbox.set_border_width(3)

		# context for the view
		self._entry = Gtk.Entry()
		self._entry.set_has_frame(False)
		self._entry.set_name('gedit-commander-entry')
		self._entry.show()

		css = Gtk.CssProvider()
		css.load_from_data("""
@binding-set terminal-like-bindings {
	unbind "<Control>A";

	bind "<Control>W" { "delete-from-cursor" (word-ends, -1) };
	bind "<Control>A" { "move-cursor" (buffer-ends, -1, 0) };
	bind "<Control>U" { "delete-from-cursor" (display-line-ends, -1) };
	bind "<Control>K" { "delete-from-cursor" (display-line-ends, 1) };
	bind "<Control>E" { "move-cursor" (buffer-ends, 1, 0) };
	bind "Escape" { "delete-from-cursor" (display-lines, 1) };
};

GtkEntry#gedit-commander-entry {
	gtk-key-bindings: terminal-like-bindings
}
""")

		# FIXME: remove hardcopy of 600 (GTK_STYLE_PROVIDER_PRIORITY_APPLICATION)
		# https://bugzilla.gnome.org/show_bug.cgi?id=646860
		self._entry.get_style_context().add_provider(css, 600)

		self._prompt_label = Gtk.Label(label='<b>&gt;&gt;&gt;</b>', use_markup=True)
		self._prompt_label.show()

		self._entry.connect('focus-out-event', self.on_entry_focus_out)
		self._entry.connect('key-press-event', self.on_entry_key_press)

		self._history = History(os.path.join(GLib.get_user_config_dir(), 'gedit/commander/history'))
		self._prompt = None

		self._accel_group = None

		hbox.pack_start(self._prompt_label, False, False, 0)
		hbox.pack_start(self._entry, True, True, 0)

		self.copy_style_from_view()
		self.view_style_updated_id = self._view.connect('style-updated', self.on_view_style_updated)

		self.add(hbox)
		self.attach()
		self._entry.grab_focus()

		self._wait_timeout = 0
		self._info_window = None

		self.connect('destroy', self.on_destroy)
		self.connect_after('size-allocate', self.on_size_allocate)
		self.view_draw_id = self._view.connect_after('draw', self.on_draw)

		self._history_prefix = None
		self._suspended = None
		self._handlers = [
			[0, Gdk.KEY_Up, self.on_history_move, -1],
			[0, Gdk.KEY_Down, self.on_history_move, 1],
			[None, Gdk.KEY_Return, self.on_execute, None],
			[None, Gdk.KEY_KP_Enter, self.on_execute, None],
			[0, Gdk.KEY_Tab, self.on_complete, None],
			[0, Gdk.KEY_ISO_Left_Tab, self.on_complete, None]
		]

		self._re_complete = re.compile('("((?:\\\\"|[^"])*)"?|\'((?:\\\\\'|[^\'])*)\'?|[^\s]+)')
		self._command_state = commands.Commands.State()

	def on_view_style_updated(self, widget):
		self.copy_style_from_view()

	def get_border_color(self):
		color = self.get_background_color().copy()
		color.red = 1 - color.red
		color.green = 1 - color.green
		color.blue = 1 - color.blue
		color.alpha = 0.5

		return color

	def get_background_color(self):
		context = self._view.get_style_context()
		return context.get_background_color(Gtk.StateFlags.NORMAL)
	
	def get_foreground_color(self):
		context = self._view.get_style_context()
		return context.get_color(Gtk.StateFlags.NORMAL)

	def get_font(self):
		context = self._view.get_style_context()
		return context.get_font(Gtk.StateFlags.NORMAL)

	def copy_style_from_view(self, widget=None):
		if widget != None:
			context = self._view.get_style_context()
			font = context.get_font(Gtk.StateFlags.NORMAL)

			widget.override_color(Gtk.StateFlags.NORMAL, self.get_foreground_color())
			widget.override_background_color(Gtk.StateFlags.NORMAL, self.get_background_color())
			widget.override_font(self.get_font())
		else:
			if self._entry:
				self.copy_style_from_view(self._entry)

			if self._prompt_label:
				self.copy_style_from_view(self._prompt_label)

	def view(self):
		return self._view

	def on_size_allocate(self, widget, alloc):
		alloc = self.get_allocation()
		self._view.set_border_window_size(Gtk.TextWindowType.BOTTOM, alloc.height)

		win = self._view.get_window(Gtk.TextWindowType.BOTTOM)
		self.set_size_request(win.get_width(), -1)

		# NOTE: we need to do this explicitly somehow, otherwise the window
		# size will not be updated unless something else happens, not exactly
		# sure what. This might be caused by the multi notebook, or custom
		# animation layouting?
		self._view.get_parent().resize_children()

	def attach(self):
		# Attach ourselves in the text view, and position just above the
		# text window
		win = self._view.get_window(Gtk.TextWindowType.TEXT)
		alloc = self.get_allocation()

		self._view.set_border_window_size(Gtk.TextWindowType.BOTTOM, max(alloc.height, 1))
		self._view.add_child_in_window(self, Gtk.TextWindowType.BOTTOM, 0, 0)

		win = self._view.get_window(Gtk.TextWindowType.BOTTOM)

		# Set same color as gutter, i.e. bg color of the view
		context = self._view.get_style_context()
		color = context.get_background_color(Gtk.StateFlags.NORMAL)
		win.set_background_rgba(color)

		self.show()
		self.set_size_request(win.get_width(), -1)

	def on_entry_focus_out(self, widget, evnt):
		if self._entry.get_sensitive():
			self.destroy()

	def on_entry_key_press(self, widget, evnt):
		state = evnt.state & Gtk.accelerator_get_default_mod_mask()
		text = self._entry.get_text()

		if evnt.keyval == Gdk.KEY_Escape:
			if self._info_window:
				if self._suspended:
					self._suspended.resume()

				if self._info_window:
					self._info_window.destroy()

				self._entry.set_sensitive(True)
			elif self._accel_group:
				self._accel_group = self._accel_group.parent

				if not self._accel_group or not self._accel_group.parent:
					self._entry.set_editable(True)
					self._accel_group = None

				self.prompt()
			elif text:
				self._entry.set_text('')
			elif self._command_state:
				self._command_state.clear()
				self.prompt()
			else:
				self._view.grab_focus()
				self.destroy()

			return True

		if state or self._accel_group:
			# Check if it should be handled by the accel group
			group = self._accel_group

			if not self._accel_group:
				group = commands.Commands().accelerator_group()

			accel = group.activate(evnt.keyval, state)

			if isinstance(accel, commands.accel_group.AccelGroup):
				self._accel_group = accel
				self._entry.set_text('')
				self._entry.set_editable(False)
				self.prompt()

				return True
			elif isinstance(accel, commands.accel_group.AccelCallback):
				self._entry.set_editable(True)
				self.run_command(lambda: accel.activate(self._command_state, self))
				return True

		if not self._entry.get_editable():
			return True

		for handler in self._handlers:
			if (handler[0] == None or handler[0] == state) and evnt.keyval == handler[1] and handler[2](handler[3], state):
				return True

		if self._info_window and self._info_window.empty():
			self._info_window.destroy()

		self._history_prefix = None
		return False

	def on_history_move(self, direction, modifier):
		pos = self._entry.get_position()

		self._history.update(self._entry.get_text())

		if self._history_prefix == None:
			if len(self._entry.get_text()) == pos:
				self._history_prefix = self._entry.get_chars(0, pos)
			else:
				self._history_prefix = ''

		if self._history_prefix == None:
			hist = ''
		else:
			hist = self._history_prefix

		next = self._history.move(direction, hist)

		if next != None:
			self._entry.set_text(next)
			self._entry.set_position(-1)

		return True

	def prompt(self, pr=''):
		self._prompt = pr

		if self._accel_group != None:
			pr = '<i>%s</i>' % (saxutils.escape(self._accel_group.full_name()),)

		if not pr:
			pr = ''
		else:
			pr = ' ' + pr

		self._prompt_label.set_markup('<b>&gt;&gt;&gt;</b>%s' % pr)

	def make_info(self):
		if self._info_window == None:
			self._info_window = Info(self)
			self._info_window.show()

			self._info_window.connect('destroy', self.on_info_window_destroy)

	def on_info_window_destroy(self, widget):
		self._info_window = None

	def info_show(self, text='', use_markup=False):
		self.make_info()
		self._info_window.add_lines(text, use_markup)

	def info_status(self, text):
		self.make_info()
		self._info_window.status(text)

	def info_add_action(self, stock, callback, data=None):
		self.make_info()
		return self._info_window.add_action(stock, callback, data)

	def command_history_done(self):
		self._history.add(self._entry.get_text())
		self._history_prefix = None
		self._entry.set_text('')

	def on_wait_cancel(self):
		if self._suspended:
			self._suspended.resume()

		if self._cancel_button:
			self._cancel_button.destroy()

		if self._info_window and self._info_window.empty():
			self._info_window.destroy()
			self._entry.grab_focus()
			self._entry.set_sensitive(True)

	def _show_wait_cancel(self):
		self._cancel_button = self.info_add_action(Gtk.STOCK_STOP, self.on_wait_cancel)
		self.info_status('<i>Waiting to finish...</i>')

		self._wait_timeout = 0
		return False

	def _complete_word_match(self, match):
		for i in (3, 2, 0):
			if match.group(i) != None:
				return [match.group(i), match.start(i), match.end(i)]

	def on_suspend_resume(self):
		if self._wait_timeout:
			GObject.source_remove(self._wait_timeout)
			self._wait_timeout = 0
		else:
			self._cancel_button.destroy()
			self._cancel_button = None
			self.info_status(None)

		self._entry.set_sensitive(True)
		self.command_history_done()

		if self._entry.props.has_focus or (self._info_window and not self._info_window.empty()):
			self._entry.grab_focus()

		self.on_execute(None, 0)

	def ellipsize(self, s, size):
		if len(s) <= size:
			return s

		mid = (size - 4) / 2
		return s[:mid] + '...' + s[-mid:]

	def destroy(self):
		self.hide()
		Gtk.EventBox.destroy(self)

	def run_command(self, cb):
		self._suspended = None

		try:
			ret = cb()
		except Exception, e:
			self.command_history_done()
			self._command_state.clear()

			self.prompt()

			# Show error in info
			self.info_show('<b><span color="#f66">Error:</span></b> ' + saxutils.escape(str(e)), True)

			if not isinstance(e, commands.exceptions.Execute):
				self.info_show(self.format_trace(), False)

			return None

		if ret == commands.result.Result.SUSPEND:
			# Wait for it...
			self._suspended = ret
			ret.register(self.on_suspend_resume)

			self._wait_timeout = GObject.timeout_add(500, self._show_wait_cancel)
			self._entry.set_sensitive(False)
		else:
			self.command_history_done()
			self.prompt('')

			if ret == commands.result.Result.PROMPT:
				self.prompt(ret.prompt)
			elif (ret == None or ret == commands.result.HIDE) and not self._prompt and (not self._info_window or self._info_window.empty()):
				self._command_state.clear()
				self._view.grab_focus()
				self.destroy()
			else:
				self._entry.grab_focus()

		return ret

	def format_trace(self):
		tp, val, tb = sys.exc_info()

		origtb = tb

		thisdir = os.path.dirname(__file__)

		# Skip frames up until after the last entry.py...
		while True:
			filename = tb.tb_frame.f_code.co_filename

			dname = os.path.dirname(filename)

			if not dname.startswith(thisdir):
				break

			tb = tb.tb_next

		msg = traceback.format_exception(tp, val, tb)
		r = ''.join(msg[0:-1])

		# This is done to prevent cyclic references, see python
		# documentation on sys.exc_info
		del origtb

		return r

	def on_execute(self, dummy, modifier):
		if self._info_window and not self._suspended:
			self._info_window.destroy()

		text = self._entry.get_text().strip()
		words = list(self._re_complete.finditer(text))
		wordsstr = []

		for word in words:
			spec = self._complete_word_match(word)
			wordsstr.append(spec[0])

		if not wordsstr and not self._command_state:
			self._entry.set_text('')
			return

		self.run_command(lambda: commands.Commands().execute(self._command_state, text, words, wordsstr, self, modifier))

		return True

	def on_complete(self, dummy, modifier):
		# First split all the text in words
		text = self._entry.get_text()
		pos = self._entry.get_position()

		words = list(self._re_complete.finditer(text))
		wordsstr = []

		for word in words:
			spec = self._complete_word_match(word)
			wordsstr.append(spec[0])

		# Find out at which word the cursor actually is
		# Examples:
		#  * hello world|
		#  * hello| world
		#  * |hello world
		#  * hello wor|ld
		#  * hello  |  world
		#  * "hello world|"
		posidx = None

		for idx in xrange(0, len(words)):
			spec = self._complete_word_match(words[idx])

			if words[idx].start(0) > pos:
				# Empty space, new completion
				wordsstr.insert(idx, '')
				words.insert(idx, None)
				posidx = idx
				break
			elif spec[2] == pos:
				# At end of word, resume completion
				posidx = idx
				break
			elif spec[1] <= pos and spec[2] > pos:
				# In middle of word, do not complete
				return True

		if posidx == None:
			wordsstr.append('')
			words.append(None)
			posidx = len(wordsstr) - 1

		# First word completes a command, if not in any special 'mode'
		# otherwise, relay completion to the command, or complete by advice
		# from the 'mode' (prompt)
		cmds = commands.Commands()

		if not self._command_state and posidx == 0:
			# Complete the first command
			ret = commands.completion.command(words=wordsstr, idx=posidx)
		else:
			complete = None
			realidx = posidx

			if not self._command_state:
				# Get the command first
				cmd = commands.completion.single_command(wordsstr, 0)
				realidx -= 1

				ww = wordsstr[1:]
			else:
				cmd = self._command_state.top()
				ww = wordsstr

			if cmd:
				complete = cmd.autocomplete_func()

			if not complete:
				return True

			# 'complete' contains a dict with arg -> func to do the completion
			# of the named argument the command (or stack item) expects
			args, varargs = cmd.args()

			# Remove system arguments
			s = ['argstr', 'args', 'entry', 'view']
			args = filter(lambda x: not x in s, args)

			if realidx < len(args):
				arg = args[realidx]
			elif varargs:
				arg = '*'
			else:
				return True

			if not arg in complete:
				return True

			func = complete[arg]

			try:
				spec = utils.getargspec(func)

				if not ww:
					ww = ['']

				kwargs = {
					'words': ww,
					'idx': realidx,
					'view': self._view
				}

				if not spec.keywords:
					for k in kwargs.keys():
						if not k in spec.args:
							del kwargs[k]

				ret = func(**kwargs)
			except Exception, e:
				# Can be number of arguments, or return values or simply buggy
				# modules
				print e
				traceback.print_exc()
				return True

		if not ret or not ret[0]:
			return True

		res = ret[0]
		completed = ret[1]

		if len(ret) > 2:
			after = ret[2]
		else:
			after = ' '

		# Replace the word
		if words[posidx] == None:
			# At end of everything, just append
			spec = None

			self._entry.insert_text(completed, self._entry.get_text_length())
			self._entry.set_position(-1)
		else:
			spec = self._complete_word_match(words[posidx])

			self._entry.delete_text(spec[1], spec[2])
			self._entry.insert_text(completed, spec[1])
			self._entry.set_position(spec[1] + len(completed))

		if len(res) == 1:
			# Full completion
			lastpos = self._entry.get_position()

			if not isinstance(res[0], commands.module.Module) or not res[0].commands():
				if words[posidx] and after == ' ' and (words[posidx].group(2) != None or words[posidx].group(3) != None):
					lastpos = lastpos + 1

				self._entry.insert_text(after, lastpos)
				self._entry.set_position(lastpos + 1)
			elif completed == wordsstr[posidx] or not res[0].method:
				self._entry.insert_text('.', lastpos)
				self._entry.set_position(lastpos + 1)

			if self._info_window:
				self._info_window.destroy()
		else:
			# Show popup with completed items
			if self._info_window:
				self._info_window.clear()

			ret = []

			for x in res:
				if isinstance(x, commands.method.Method):
					ret.append('<b>' + saxutils.escape(x.name) + '</b> (<i>' + x.oneline_doc() + '</i>)')
				else:
					ret.append(str(x))

			self.info_show("\n".join(ret), True)

		return True

	def on_draw(self, widget, ct):
		win = widget.get_window(Gtk.TextWindowType.BOTTOM)

		if not Gtk.cairo_should_draw_window(ct, win):
			return False

		Gtk.cairo_transform_to_window(ct, widget, win)

		color = self.get_border_color()
		width = win.get_width()

		ct.set_source_rgba(color.red, color.green, color.blue, color.alpha)
		ct.move_to(0, 0)
		ct.line_to(width, 0)
		ct.stroke()

		return False

	def on_destroy(self, widget):
		self._view.set_border_window_size(Gtk.TextWindowType.BOTTOM, 0)
		self._view.disconnect(self.view_style_updated_id)
		self._view.disconnect(self.view_draw_id)

		if self._info_window:
			self._info_window.destroy()

		self._history.save()

# vi:ex:ts=4
