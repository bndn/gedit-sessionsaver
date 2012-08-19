# -*- coding: utf-8 -*-
#
#  goto.py - goto commander module
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

import os

import commander.commands as commands
import commander.commands.completion
import commander.commands.result
import commander.commands.exceptions

__commander_module__ = True

def __default__(view, line, column=1):
    """Goto line number"""

    buf = view.get_buffer()
    ins = buf.get_insert()
    citer = buf.get_iter_at_mark(ins)

    try:
        if line.startswith('+'):
            linnum = citer.get_line() + int(line[1:])
        elif line.startswith('-'):
            linnum = citer.get_line() - int(line[1:])
        else:
            linnum = int(line) - 1

        column = int(column) - 1
    except ValueError:
        raise commander.commands.exceptions.Execute('Please specify a valid line number')

    linnum = min(max(0, linnum), buf.get_line_count() - 1)
    citer = buf.get_iter_at_line(linnum)

    column = min(max(0, column), citer.get_chars_in_line() - 1)

    citer = buf.get_iter_at_line(linnum)
    citer.forward_chars(column)

    buf.place_cursor(citer)
    view.scroll_to_iter(citer, 0.0, True, 0, 0.5)

    return commander.commands.result.HIDE

# vi:ex:ts=4:et
