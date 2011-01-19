# -*- coding: utf-8 -*-
#
#  __init__.py - commander
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

import os
import sys

path = os.path.dirname(__file__)

if not path in sys.path:
	sys.path.insert(0, path)

import gedit
from windowhelper import WindowHelper
import commander.commands as commands

class Commander(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)

		self._instances = {}
		self._path = os.path.dirname(__file__)

		if not self._path in sys.path:
			sys.path.insert(0, self._path)

		commands.Commands().set_dirs([
			os.path.expanduser('~/.gnome2/gedit/commander/modules'),
			os.path.join(self.get_data_dir(), 'modules')
		])

	def activate(self, window):
		self._instances[window] = WindowHelper(self, window)

	def deactivate(self, window):
		self._instances[window].deactivate()
		del self._instances[window]

		if len(self._instances) == 0:
			commands.Commands().stop()

			if self._path in sys.path:
				sys.path.remove(self._path)

	def update_ui(self, window):
		self._instances[window].update_ui()
