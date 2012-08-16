# -*- coding: utf-8 -*-
#
#  metamodule.py
#
#  Copyright (C) 2012 - Jesse van den Kieboom
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
class MetaModule(object):
    def __init__(self, mod):
        object.__setattr__(self, '_mod', mod)

    def __getattribute__(self, name):
        return getattr(object.__getattribute__(self, "_mod"), name)

    def __delattr__(self, name):
        delattr(object.__getattribute__(self, "_mod"), name)

    def __setattr__(self, name, value):
        setattr(object.__getattribute__(self, "_mod"), name, value)

    def __getitem__(self, item):
        return object.__getattribute__(self, "_mod")[item]

    def __setitem__(self, item, value):
        object.__getattribute__(self, "_mod")[item] = value

    def __delitem__(self, item):
        del object.__getattribute__(self, "_mod")[item]

    def __iter__(self):
        return iter(object.__getattribute__(self, "_mod"))

    def __contains__(self, item):
        return item in object.__getattribute__(self, "_mod")

    def __call__(self, *args, **kw):
        object.__getattribute__(self, "_mod").__default__(*args, **kw)

# vi:ts=4:et
