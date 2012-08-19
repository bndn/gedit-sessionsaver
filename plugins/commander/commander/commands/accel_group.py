# -*- coding: utf-8 -*-
#
#  accel_group.py - commander
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

from gi.repository import Gtk

class Accelerator:
    def __init__(self, accelerators, arguments={}):
        if not hasattr(accelerators, '__iter__'):
            accelerators = [accelerators]

        self.accelerators = accelerators
        self.arguments = arguments

class AccelCallback:
    def __init__(self, accel, callback, data):
        self.accelerator = accel
        self.callback = callback
        self.data = data

    def activate(self, state, entry):
        self.callback(self.accelerator, self.data, state, entry)

class AccelGroup:
    def __init__(self, parent=None, name='', accelerators={}):
        self.accelerators = dict(accelerators)
        self.parent = parent
        self.name = name

    def add(self, accel, callback, data=None):
        num = len(accel.accelerators)
        mapping = self.accelerators

        for i in range(num):
            parsed = Gtk.accelerator_parse(accel.accelerators[i])

            if not Gtk.accelerator_valid(*parsed):
                return

            named = Gtk.accelerator_name(*parsed)
            inmap = named in mapping

            if i == num - 1 and inmap:
                # Last one cannot be in the map
                return
            elif inmap and isinstance(mapping[named], AccelCallback):
                # It's already mapped...
                return
            else:
                if not inmap:
                    mapping[named] = {}

                if i == num - 1:
                    mapping[named] = AccelCallback(accel, callback, data)

                mapping = mapping[named]

    def remove_real(self, accelerators, accels):
        if not accels:
            return

        parsed = Gtk.accelerator_parse(accels[0])

        if not Gtk.accelerator_valid(*parsed):
            return

        named = Gtk.accelerator_name(*parsed)

        if not named in accelerators:
            return

        if len(accels) == 1:
            del accelerators[named]
        else:
            self.remove_real(accelerators[named], accels[1:])

            if not accelerators[named]:
                del accelerators[named]

    def remove(self, accel):
        self.remove_real(self.accelerators, accel.accelerators)

    def activate(self, key, mod):
        named = Gtk.accelerator_name(key, mod)

        if not named in self.accelerators:
            return None

        accel = self.accelerators[named]

        if isinstance(accel, AccelCallback):
            return accel
        else:
            return AccelGroup(self, named, accel)

    def full_name(self):
        name = ''

        if self.parent:
            name = self.parent.full_name()

        if self.name:
            if name:
                name += ', '

            name += self.name

        return name

# vi:ex:ts=4:et
