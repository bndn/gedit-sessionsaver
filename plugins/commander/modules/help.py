# -*- coding: utf-8 -*-
#
#  help.py - help commander module
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

import sys
import os
import types

import commander.commands as commands
import commander.commands.completion

from xml.sax import saxutils

__commander_module__ = True

def _name_match(first, second):
    first = first.split('-')
    second = second.split('-')

    if len(first) > len(second):
        return False

    for i in xrange(0, len(first)):
        if not second[i].startswith(first[i]):
            return False

    return True

def _doc_text(command, func):
    if not _name_match(command.split('.')[-1], func.name):
        prefix = '<i>(Alias):</i> '
    else:
        prefix = ''

    doc = func.doc()

    if not doc:
        doc = "<b>%s</b>\n\n<i>No documentation available</i>" % (func.name,)
    else:
        parts = doc.split("\n")
        parts[0] = prefix + '<b>' + parts[0] + '</b>'
        doc = "\n".join(parts)

    return doc

@commands.autocomplete(command=commander.commands.completion.command)
def __default__(entry, command='help'):
    """Show help on commands: help &lt;command&gt;

Show detailed information on how to use a certain command (if available)"""
    res = commander.commands.completion.command([command], 0)

    if not res:
        raise commander.commands.exceptions.Execute('Could not find command: ' + command)

    entry.info_show(_doc_text(command, res[0][0]), True)
    return commands.result.DONE

# vi:ex:ts=4:et
