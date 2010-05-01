import os
import sys

path = os.path.dirname(__file__)

if not path in sys.path:
	sys.path.insert(0, path)

import gedit
from windowhelper import WindowHelper
import commands

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
