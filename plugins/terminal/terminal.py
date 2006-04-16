# -*- coding: utf8 -*-

# terminal.py - Embeded VTE terminal for gedit
# This file is part of gedit
#
# Copyright (C) 2005-2006 - Paolo Borelli
#
# gedit is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# gedit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gedit; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, 
# Boston, MA  02110-1301  USA

import gedit
import pango, gtk, vte, gconf

# Defaults.
def_audible = 0
def_background = None
def_blink = 0
def_command = None
def_emulation = "xterm"
def_font_name = "monospace 10"
def_fgcol = "AAAAAA"
def_bgcol = "000000"
def_scrollback = 100
def_transparent = 0
def_visible = 0

class GeditTerminal(vte.Terminal):
	"""VTE terminal which follows gnome-terminal default profile options"""
	
	def __init__(self):
		vte.Terminal.__init__(self)

		self.gconf = gconf.client_get_default()

		# TODO: retrieve the profile from GConf

		self.gconf.add_dir("/apps/gnome-terminal/profiles/Default",
				   gconf.CLIENT_PRELOAD_RECURSIVE)

		if not self.gconf.get_bool("/apps/gnome-terminal/profiles/Default/use_system_font"):
			self.set_font(self.gconf.get_string("/apps/gnome-terminal/profiles/Default/font"))
		else:
			self.set_font(self.gconf.get_string("/desktop/gnome/interface/monospace_font"))

#TODO:
#		if not self.gconf.get_bool("/apps/gnome-terminal/profiles/Default/use_theme_colors"):
#			self.set_colors(gtk.gdk.color_parse (self.gconf.get_string("/apps/gnome-terminal/profiles/Default/foregound_color")),
#					gtk.gdk.color_parse (self.gconf.get_string("/apps/gnome-terminal/profiles/Default/backgound_color")))
#		else:
#			...

		self.set_colors(gtk.gdk.color_parse (self.gconf.get_string("/apps/gnome-terminal/profiles/Default/foreground_color")),
				gtk.gdk.color_parse (self.gconf.get_string("/apps/gnome-terminal/profiles/Default/background_color")))


		# TODO: more GConf getting
		self.set_cursor_blinks(def_blink)
		self.set_emulation(def_emulation)
		self.set_scrollback_lines(def_scrollback)
		self.set_audible_bell(def_audible)
		self.set_visible_bell(def_visible)

		# GConf notification
		self.gconf.notify_add("/apps/gnome-terminal/profiles/Default/font",
				      lambda x, y, z, a: self.set_font(z.value.get_string())) #is this bad if the value isn't a string?

		# TODO: add more notofications

		# set a reasonably small size... this is ugly but it seems
		# the only way to make vte behave.
		self.set_size(30, 5)
	        self.set_size_request(200, 50)

		# Start up the default command, the user's shell.
		self.fork_command()
		self.connect("child-exited", lambda t: t.fork_command())

	def set_font(self, font_name):
		try:
			font = pango.FontDescription(font_name)
		except:
			font = pango.FontDescription(def_font_name)
		vte.Terminal.set_font(self, font)

	def set_colors(self, fg_col, bg_col):
		vte.Terminal.set_colors(self, fg_col, bg_col, [])

class TerminalPlugin(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)

	def activate(self, window):
		terminal = GeditTerminal()

		window.terminal_pane = gtk.HBox(0)
		window.terminal_pane.set_resize_mode(gtk.RESIZE_IMMEDIATE)

		window.terminal_pane.pack_start(terminal)

		scrollbar = gtk.VScrollbar(terminal.get_adjustment())
		window.terminal_pane.pack_start(scrollbar, False, False, 0)

		window.terminal_pane.show_all()

		image = gtk.Image()
		image.set_from_icon_name("gnome-terminal", gtk.ICON_SIZE_MENU)
		bottom = window.get_bottom_panel()
		bottom.add_item(window.terminal_pane, "Terminal", image)

	def deactivate(self, window):
		bottom = window.get_bottom_panel()
		bottom.remove_item(window.terminal_pane)

	def update_ui(self, window):
		pass
