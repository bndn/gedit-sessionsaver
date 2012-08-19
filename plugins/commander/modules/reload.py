# -*- coding: utf-8 -*-
#
#  reload.py - reload commander module
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

import commander.commands
import commander.commands.completion
import commander.commands.exceptions
import commander.commands.result
import commander.utils as utils
import commander.commands.module

__commander_module__ = True

@commander.commands.autocomplete(command=commander.commands.completion.command)
def __default__(command):
    """Force reload of a module: reload &lt;module&gt;

Force a reload of a module. This is mostly useful on systems where file monitoring
does not work correctly."""

    # Get the command
    res = commander.commands.completion.command([command], 0)

    if not res:
        raise commander.commands.exceptions.Execute('Could not find command: ' + command)

    mod = res[0][0]

    while not isinstance(mod, commander.commands.module.Module):
        mod = mod.parent

    commander.commands.Commands().reload_module(mod)
    return commander.commands.result.DONE

# vi:ex:ts=4:et
