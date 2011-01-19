# -*- coding: utf-8 -*-
#
#  completion.py - commander
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

import commander.commands as commands
import bisect
import sys
import os
import re
import gio

from xml.sax import saxutils

__all__ = ['command', 'filename']

def _common_prefix_part(first, second):
	length = min(len(first), len(second))

	for i in range(0, length):
		if first[i] != second[i]:
			return first[:i]

	return first[:length]

def common_prefix(args, sep=None):
	# A common prefix can be something like
	# first: some-thing
	# second: sho-tar
	# res: s-t
	args = list(args)

	if not args:
		return ''

	if len(args) == 1:
		return str(args[0])

	first = str(args[0])
	second = str(args[1])

	if not sep:
		ret = _common_prefix_part(first, second)
	else:
		first = first.split(sep)
		second = second.split(sep)
		ret = []

		for i in range(0, min(len(first), len(second))):
			ret.append(_common_prefix_part(first[i], second[i]))

		ret = sep.join(ret)

	del args[0]
	args[0] = ret

	return common_prefix(args, sep)

def _expand_commands(cmds):
	if not cmds:
		cmds.extend(commands.Commands().modules())
		return

	old = list(cmds)
	del cmds[:]

	# Expand 'commands' to all the respective subcommands

	for cmd in old:
		for c in cmd.commands():
			bisect.insort(cmds, c)

def _filter_command(cmd, subs):
	parts = cmd.name.split('-')

	if len(subs) > len(parts):
		return False

	for i in xrange(0, len(subs)):
		if not parts[i].startswith(subs[i]):
			return False

	return True

def _filter_commands(cmds, subs):
	# See what parts of cmds still match the parts in subs
	idx = bisect.bisect_left(cmds, subs[0])
	ret = []

	while idx < len(cmds):
		if not cmds[idx].name.startswith(subs[0]):
			break

		if _filter_command(cmds[idx], subs):
			ret.append(cmds[idx])

		idx += 1

	return ret

def single_command(words, idx):
	ret = command(words, idx)

	if not ret:
		return None

	ret[0] = filter(lambda x: x.method, ret[0])

	if not ret[0]:
		return None

	return ret[0][0]

def command(words, idx):
	s = words[idx].strip()

	parts = s.split('.')
	cmds = []

	for i in parts:
		# Expand all the parents to their child commands
		_expand_commands(cmds)

		if not cmds:
			return None

		subs = i.split('-')
		cmds = _filter_commands(cmds, subs)

		if not cmds:
			return None

	if not cmds:
		return None

	if len(parts) == 1:
		completed = common_prefix(cmds)
	else:
		completed = '.'.join(parts[0:-1]) + '.' + common_prefix(cmds, '-')

	return [cmds, completed]

def _file_color(path):
	if os.path.isdir(path):
		format = '<span color="#799ec6">%s</span>'
	else:
		format = '%s'

	return format % (saxutils.escape(os.path.basename(path)),)

def _sort_nicely(l):
	convert = lambda text: int(text) if text.isdigit() else text
	alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]

	l.sort(key=alphanum_key)

def filename(words, idx, view):
	prefix = os.path.dirname(words[idx])
	partial = os.path.expanduser(words[idx])

	doc = view.get_buffer()

	if not doc.is_untitled():
		root = os.path.dirname(doc.get_location().get_path())
	else:
		root = os.path.expanduser('~/')

	if not os.path.isabs(partial):
		partial = os.path.join(root, partial)

	dirname = os.path.dirname(partial)

	try:
		files = os.listdir(dirname)
	except OSError:
		return None

	base = os.path.basename(partial)
	ret = []
	real = []

	for f in files:
		if f.startswith(base) and (base or not f.startswith('.')):
			real.append(os.path.join(dirname, f))
			ret.append(os.path.join(prefix, f))

	_sort_nicely(real)

	if len(ret) == 1:
		if os.path.isdir(real[0]):
			after = '/'
		else:
			after = ' '

		return ret, ret[0], after
	else:
		return map(lambda x: _file_color(x), real), common_prefix(ret)

def words(ret):
	def decorator(words, idx):
		rr = filter(lambda x: x.startswith(words[idx]), ret)
		return rr, common_prefix(rr)

	return decorator
