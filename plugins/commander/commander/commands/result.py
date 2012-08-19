# -*- coding: utf-8 -*-
#
#  result.py - commander
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

class Result(object):
    HIDE = 1
    DONE = 2
    PROMPT = 3
    SUSPEND = 4

    def __init__(self, value):
        self._value = value

    def __int__(self):
        return self._value

    def __cmp__(self, other):
        if isinstance(other, int) or isinstance(other, Result):
            return cmp(int(self), int(other))
        else:
            return 1

# Easy shortcuts
HIDE = Result(Result.HIDE)
DONE = Result(Result.DONE)

class Prompt(Result):
    def __init__(self, prompt, autocomplete={}):
        Result.__init__(self, Result.PROMPT)

        self.prompt = prompt
        self.autocomplete = autocomplete

class Suspend(Result):
    def __init__(self):
        Result.__init__(self, Result.SUSPEND)
        self._callbacks = []

    def register(self, cb, *args):
        self._callbacks.append([cb, args])

    def resume(self):
        for cb in self._callbacks:
            args = cb[1]
            cb[0](*args)

# vi:ex:ts=4:et
