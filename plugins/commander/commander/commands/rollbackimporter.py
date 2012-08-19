# -*- coding: utf-8 -*-
#
#  rollbackimporter.py - commander
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
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.

import sys
import utils

class RollbackImporter:
    def __init__(self):
        "Creates an instance and installs as the global importer"
        self._new_modules = []
        self._original_import = __builtins__['__import__']

    def monitor(self):
        __builtins__['__import__'] = self._import

    def cancel(self):
        __builtins__['__import__'] = self._original_import

    def _import(self, name, globals=None, locals=None, fromlist=[], level=-1):
        maybe = not name in sys.modules

        mod = apply(self._original_import, (name, globals, locals, fromlist, level))

        if maybe and utils.is_commander_module(mod):
            self._new_modules.append(name)

        return mod

    def uninstall(self):
        self.cancel()

        for modname in self._new_modules:
            if modname in sys.modules:
                del sys.modules[modname]

        self._new_modules = []

# vi:ex:ts=4:et
