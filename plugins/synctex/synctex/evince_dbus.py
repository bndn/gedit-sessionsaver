#!/usr/bin/python
# -*- coding: utf-8 -*-

# This file is part of the Gedit Synctex plugin.
#
# Copyright (C) 2010 Jose Aliste <jose.aliste@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public Licence as published by the Free Software
# Foundation; either version 2 of the Licence, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public Licence for more
# details.
#
# You should have received a copy of the GNU General Public Licence along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA

import dbus, subprocess, time
from traceback import print_exc

RUNNING, CLOSED = range(2)

class EvinceWindowProxy:
	"""A DBUS proxy for an Evince Window."""
	daemon = None
	bus = None
	def __init__(self, uri, spawn = False):
		self.uri = uri
		self.spawn = spawn
		self.status = CLOSED
		self.source_handler = None
		try:
			if EvinceWindowProxy.bus is None:
				EvinceWindowProxy.bus = dbus.SessionBus()
			if EvinceWindowProxy.daemon is None:
				EvinceWindowProxy.daemon = EvinceWindowProxy.bus.get_object('org.gnome.evince.Daemon',
							   '/org/gnome/evince/Daemon')
			self._get_dbus_name(False)
		except dbus.DBusException:
			print_exc()

	def _get_dbus_name(self, spawn):
		try:
			self.dbus_name = self.daemon.FindDocument(self.uri,spawn, dbus_interface = "org.gnome.evince.Daemon")
			if self.dbus_name != '':
				self.status = RUNNING
				self.window = EvinceWindowProxy.bus.get_object(self.dbus_name, '/org/gnome/evince/Window/0')
				self.window.connect_to_signal("Closed", self.on_window_close, dbus_interface="org.gnome.evince.Window")
				self.window.connect_to_signal("SyncSource", self.on_sync_source, dbus_interface="org.gnome.evince.Window")
		except dbus.DBusException:
			print_exc()
	def set_source_handler (self, source_handler):
		self.source_handler = source_handler

	def on_window_close(self):
		self.status = CLOSED
		self.window = None

	def on_sync_source(self, input_file, source_link):
		if self.source_handler is not None:
			self.source_handler(input_file, source_link)

	def name_owner_changed_cb(self, service_name, old_owner, new_owner):
		if service_name == self.dbus_name and new_owner == '': 
			self.status = CLOSED

	def SyncView(self, input_file, data):
		if self.status == CLOSED:
			print "Evince is closed"
			if self.spawn:
				print "Spawning evince"
				self._get_dbus_name(True)
		# if self.status is still closed, it means there is a
		# problem running evince.
		if self.status == CLOSED: 
			return False
		self.window.SyncView(input_file, data, dbus_interface="org.gnome.evince.Window")
		return False

## This file can be used as a script to support forward search and backward search in vim.
## It should be easy to adapt to other editors. 
##  evince_dbus  pdf_file  line_source input_file
if __name__ == '__main__':
	import dbus.mainloop.glib, gobject, glib, sys, os
	
	def print_usage():
		print '''
The usage is evince_dbus output_file line_number input_file from the directory of output_file.
'''
		sys.exit(1)

	if len(sys.argv)!=4:
		print_usage()
	try:
		line_number = int(sys.argv[2])
	except ValueError:
		print_usage()
	
	output_file = sys.argv[1]
	input_file  = sys.argv[3]
	path_output  = os.getcwd() + '/' + output_file
	path_input   = os.getcwd() + '/' + input_file

	if not os.path.isfile(path_output):
		print_usage()

	dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
	bus = dbus.SessionBus()

	a = EvinceWindowProxy(bus, 'file://' + path_output, True )
	glib.timeout_add(1000, a.SyncView, path_input, (line_number,1))
	loop = gobject.MainLoop()
	loop.run() 
