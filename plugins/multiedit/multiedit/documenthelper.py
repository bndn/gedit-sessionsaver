# -*- coding: utf-8 -*-
#
#  documenthelper.py - Multi Edit
#
#  Copyright (C) 2009 - Jesse van den Kieboom
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

import gedit
import gtksourceview2 as gsv
import gtk
import glib

from signals import Signals
import constants
import time
import pango
import xml.sax.saxutils

from gpdefs import *

try:
    gettext.bindtextdomain(GETTEXT_PACKAGE, GP_LOCALEDIR)
    _ = lambda s: gettext.dgettext(GETTEXT_PACKAGE, s);
except:
    _ = lambda s: s

class DocumentHelper(Signals):
    def __init__(self, view):
        Signals.__init__(self)

        view.set_data(constants.DOCUMENT_HELPER_KEY, self)

        self._view = view
        self._buffer = None
        self._in_mode = False
        self._column_mode = None

        self._edit_points = []
        self._multi_edited = False
        self._status = None
        self._status_timeout = 0
        self._delete_mode_id = 0

        self.connect_signal(self._view, 'notify::buffer', self.on_notify_buffer)
        self.connect_signal(self._view, 'key-press-event', self.on_key_press_event)
        self.connect_signal(self._view, 'expose-event', self.on_view_expose_event)
        self.connect_signal(self._view, 'style-set', self.on_view_style_set)
        self.connect_signal(self._view, 'undo', self.on_view_undo)
        self.connect_signal(self._view, 'copy-clipboard', self.on_copy_clipboard)
        self.connect_signal(self._view, 'cut-clipboard', self.on_cut_clipboard)
        self.connect_signal(self._view, 'query-tooltip', self.on_query_tooltip)
        
        self._view.props.has_tooltip = True

        self.reset_buffer(self._view.get_buffer())

        self.initialize_event_handlers()

    def _update_selection_tag(self):
        style = self._view.get_style()

        fg = style.text[gtk.STATE_SELECTED]
        bg = style.base[gtk.STATE_SELECTED]

        self._selection_tag.props.foreground_gdk = fg
        self._selection_tag.props.background_gdk = bg

    def reset_buffer(self, newbuf):
        if self._buffer:
            self.disable_multi_edit()

            self.disconnect_signals(self._buffer)
            self._buffer.get_tag_table().remove(self._selection_tag)
            self._buffer.delete_mark(self._last_insert)

        self._buffer = None

        if newbuf == None or not isinstance(newbuf, gedit.Document):
            return

        if newbuf != None:
            self.connect_signal_after(newbuf, 'insert-text', self.on_insert_text)

            self.connect_signal(newbuf, 'delete-range', self.on_delete_range_before)
            self.connect_signal_after(newbuf, 'delete-range', self.on_delete_range)

            self.connect_signal_after(newbuf, 'mark-set', self.on_mark_set)
            self.connect_signal(newbuf, 'notify::style-scheme', self.on_notify_style_scheme)

            self._selection_tag = newbuf.create_tag(None)
            self._selection_tag.set_priority(newbuf.get_tag_table().get_size() - 1)
            self._last_insert = newbuf.create_mark(None, newbuf.get_iter_at_mark(newbuf.get_insert()), True)

            self._update_selection_tag()

        self._buffer = newbuf

    def stop(self):
        self._cancel_column_mode()
        self.reset_buffer(None)

        self._view.set_data(constants.DOCUMENT_HELPER_KEY, None)

        self.disconnect_signals(self._view)
        self._view = None
        
        if self._status_timeout != 0:
            glib.source_remove(self._status_timeout)
            self._status_timeout = 0
        
        if self._delete_mode_id != 0:
            glib.source_remove(self._delete_mode_id)
            self._delete_mode_id = 0

    def initialize_event_handlers(self):
        self._event_handlers = [
            [('Escape',), 0, self.do_escape_mode, True],
            [('Return',), 0, self.do_column_edit, True],
            [('Home',), gtk.gdk.CONTROL_MASK, self.do_mark_start, True],
            [('End',), gtk.gdk.CONTROL_MASK, self.do_mark_end, True],
            [('e',), gtk.gdk.CONTROL_MASK, self.do_toggle_edit_point, True]
        ]

        for handler in self._event_handlers:
            handler[0] = map(lambda x: gtk.gdk.keyval_from_name(x), handler[0])

    def enable_multi_edit(self):
        self._view.set_border_window_size(gtk.TEXT_WINDOW_TOP, 20)

        if self._in_mode:
            return

        self._in_mode = True

    def remove_edit_points(self):
        buf = self._buffer

        for mark in self._edit_points:
            buf.delete_mark(mark)

        self._edit_points = []
        self._multi_edited = False
        self._view.queue_draw()

    def disable_multi_edit(self):
        if self._column_mode:
            self._cancel_column_mode()

        self._in_mode = False

        self._view.set_border_window_size(gtk.TEXT_WINDOW_TOP, 0)
        self.remove_edit_points()

    def do_escape_mode(self, event):
        if self._column_mode:
            self._cancel_column_mode()
            return True

        if self._edit_points:
            self.remove_edit_points()
            return True

        self.disable_multi_edit()
        return True

    def iter_to_offset(self, piter):
        tw = self._view.get_tab_width()
        start = piter.copy()
        start.set_line_offset(0)

        offset = 0

        while not start.equal(piter):
            if start.get_char() == "\t":
                offset += tw - (offset % tw)
            else:
                offset += 1

            start.forward_char()

        return offset

    def get_visible_iter(self, line, offset):
        piter = self._buffer.get_iter_at_line(line)
        tw = self._view.get_tab_width()
        visiblepos = 0

        while visiblepos < offset:
            if piter.get_char() == "\t":
                visiblepos += (tw - (visiblepos % tw))
            else:
                visiblepos += 1

            if not piter.forward_char() or piter.get_line() != line:
                if piter.get_line() != line:
                    piter.backward_char()
                    visiblepos -= 1

                return piter, offset - visiblepos

        return piter, offset - visiblepos

    def _delete_columns(self):
        # Delete the text currently selected in column mode
        # If a line ends before the column selection, simply don't do anything.
        # Convert any tabs in the column selection to spaces
        # Remove all characters on each line within the column selection
        mode = self._column_mode
        self._column_mode = None

        buf = self._buffer
        start = mode[0]
        end = mode[1]

        buf.begin_user_action()

        while start <= end:
            start_iter, start_offset = self.get_visible_iter(start, mode[2])
            start += 1

            if start_offset > 0:
                # Only insert spaces
                buf.insert(start_iter, ' ' * start_offset)
                continue

            prefix = ''

            if start_offset < 0:
                # We went one tab over the start, go one back
                start_iter.backward_char()
                prefix = ' ' * (-start_offset)

            # Get the end one
            end_iter, end_offset = self.get_visible_iter(start - 1, mode[3])
            suffix = ''

            if end_offset > 0:
                # Delete until end of line
                end_iter = start_iter.copy()

                if not end_iter.ends_line():
                    end_iter.forward_to_line_end()
            elif end_offset < 0:
                # Within tab
                suffix = ' ' * (-end_offset)

            buf.delete(start_iter, end_iter)
            buf.insert(start_iter, prefix + suffix)

        buf.end_user_action()

    def _add_edit_point(self, piter):
        # Check if there is already an edit point here
        marks = piter.get_marks()
        
        for mark in marks:
            if mark in self._edit_points:
                return
        
        buf = self._buffer
        mark = buf.create_mark(None, piter, True)
        mark.set_visible(True)

        self._edit_points.append(mark)
        self.status('<i>%s</i>' % (xml.sax.saxutils.escape(_('Added edit point...'),)))

    def _remove_duplicate_edit_points(self):
        buf = self._buffer
        
        for mark in list(self._edit_points):
            if mark.get_deleted():
                continue

            piter = buf.get_iter_at_mark(mark)

            others = piter.get_marks()
            others.remove(mark)
            
            for other in others:
                if other in self._edit_points:
                    buf.delete_mark(other)
                    self._edit_points.remove(other)
        
        marks = buf.get_iter_at_mark(buf.get_insert()).get_marks()
        
        for mark in marks:
            if mark in self._edit_points:
                buf.delete_mark(mark)
                self._edit_points.remove(mark)

    def _invalidate_status(self):
        if not self._in_mode:
            return

        window = self._view.get_window(gtk.TEXT_WINDOW_TOP)
        geom = window.get_geometry()
        window.invalidate_rect(gtk.gdk.Rectangle(0, 0, geom[2], geom[3]), False)

    def _remove_status(self):
        self._status = None
        self._invalidate_status()
        
        self._status_timeout = 0
        return False

    def status(self, text):
        if not self._in_mode:
            self._status = None
            return
            
        self._status = text
        self._invalidate_status()
        
        if self._status_timeout != 0:
            glib.source_remove(self._status_timeout)
        
        self._status_timeout = glib.timeout_add(3000, self._remove_status)

    def _apply_column_mode(self):
        mode = self._column_mode

        # Delete the columns
        self._delete_columns()

        # Insert insertion marks at the start column
        start = mode[0]
        end = mode[1]
        column = mode[2]
        buf = self._buffer

        while start <= end:
            piter, offset = self.get_visible_iter(start, column)

            if offset != 0:
                sys.stderr.write('Wrong offset in applying column mode, should never happen: %d, %d' % (start, offset))
            elif start != end:
                # Add edit point for all lines, except last one
                self._add_edit_point(piter)
            else:
                # For last line, just move the insertion point there
                buf.move_mark(buf.get_insert(), piter)
                buf.move_mark(buf.get_selection_bound(), piter)

            start += 1

        self._view.queue_draw()

    def line_column_edit(self, piter, soff, eoff):
        start, soff = self.get_visible_iter(piter.get_line(), soff)
        end, eof = self.get_visible_iter(piter.get_line(), eoff)

        if eof < 0:
            end.backward_char()

        # Apply tag to start -> end
        if start.compare(end) < 0:
            self._buffer.apply_tag(self._selection_tag, start, end)

    def do_column_edit(self, event):
        buf = self._buffer

        bounds = buf.get_selection_bounds()

        if not bounds or bounds[0].get_line() == bounds[1].get_line():
            return False

        # Determine the column edit range in character positions, for each line
        # in the selection, determine where to put the edit with respect to tabs
        # that might be in the selection. Set selection tags on normal text.
        # If the column starts or stops in a tab, do custom overlay drawing
        bounds[0].order(bounds[1])
        start = bounds[0]
        end = bounds[1]

        tw = self._view.get_tab_width()
        soff = self.iter_to_offset(start)
        eoff = self.iter_to_offset(end)

        if eoff < soff:
            tmp = soff
            soff = eoff
            eoff = soff

        # Apply tags where possible
        start_line = start.get_line()
        end_line = end.get_line()

        while start.get_line() <= end.get_line():
            self.line_column_edit(start, soff, eoff)

            if not start.forward_line():
                break

        # Remove official selection
        insert = buf.get_iter_at_mark(buf.get_insert())
        buf.move_mark(buf.get_selection_bound(), insert)
        
        # Remove previous marks
        self.remove_edit_points()

        # Set the column mode
        self._column_mode = (start_line, end_line, soff, eoff)
        self.status('<i>%s</i>' % (xml.sax.saxutils.escape(_('Column Mode...')),))

        return True

    def _draw_column_mode(self, event):
        if not self._column_mode:
            return False

        start = self._column_mode[0]
        end = self._column_mode[1]
        buf = self._buffer

        col = self._view.get_style().base[gtk.STATE_SELECTED]
        layout = self._view.create_pango_layout('W')
        width = layout.get_pixel_extents()[1][2]

        ctx = event.window.cairo_create()
        ctx.rectangle(event.area)
        ctx.clip()

        ctx.set_source_color(col)

        cstart = self._column_mode[2]
        cend = self._column_mode[3]

        while start <= end:
            # Get the line range, convert to window coords, and see if it needs
            # rendering
            piter = buf.get_iter_at_line(start)
            y, height = self._view.get_line_yrange(piter)

            x_, y = self._view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT, 0, y)
            start += 1

            # Check in visible area
            if y >= event.area.y + event.area.height or \
               y + height <= event.area.y:
                continue

            # Check where to possible draw fake selection
            start_iter, soff = self.get_visible_iter(start - 1, cstart)
            end_iter, eoff = self.get_visible_iter(start - 1, cend)

            if soff == 0 and eoff == 0 and not start_iter.equal(end_iter):
                continue

            rx = cstart * width + self._view.get_left_margin()
            rw = (cend - cstart) * width

            if rw == 0:
                rw = 1

            ctx.rectangle(rx, y, rw, height)
            ctx.fill()
            
        return False

    def do_mark_start_end(self, test, move):
        buf = self._buffer
        bounds = buf.get_selection_bounds()

        if bounds:
            start = bounds[0]
            end = bounds[1]
        else:
            start = buf.get_iter_at_mark(buf.get_insert())
            end = start.copy()

        start.order(end)
        orig = start.copy()

        while start.get_line() <= end.get_line():
            if not test or not test(start):
                move(start)

            self._add_edit_point(start)

            if not start.forward_line():
                break
        
        return orig, end

    def do_mark_start(self, event):
        start, end = self.do_mark_start_end(None, lambda x: x.set_line_offset(0))
        
        start.backward_line()
        
        buf = self._buffer
        buf.move_mark(buf.get_insert(), start)
        buf.move_mark(buf.get_selection_bound(), start)
        
        return True

    def do_mark_end(self, event):
        start, end = self.do_mark_start_end(lambda x: x.ends_line(), lambda x: x.forward_to_line_end())
        
        end.forward_line()
        
        if not end.ends_line():
            end.forward_to_line_end()
        
        buf = self._buffer
        buf.move_mark(buf.get_insert(), end)
        buf.move_mark(buf.get_selection_bound(), end)

        return True
        
    def do_toggle_edit_point(self, event):
        buf = self._buffer
        piter = buf.get_iter_at_mark(buf.get_insert())
        
        marks = piter.get_marks()
        
        for mark in marks:
            if mark in self._edit_points:
                buf.delete_mark(mark)
                self._edit_points.remove(mark)
                
                self.status('<i>%s</i>' % (xml.sax.saxutils.escape(_('Removed edit point...'),)))
                return
        
        self._add_edit_point(piter)
        return True
        
    def on_key_press_event(self, view, event):
        defmod = gtk.accelerator_get_default_mod_mask() & event.state

        for handler in self._event_handlers:
            if (not handler[3] or self._in_mode) and event.keyval in handler[0] and (defmod == handler[1]):
                return handler[2](event)

        return False

    def on_notify_style_scheme(self, buf, spec):
        self._update_selection_tag()

    def on_view_style_set(self, view, prev):
        self._update_selection_tag()

    def on_notify_buffer(self, view, spec):
        self.reset_buffer(view.get_buffer())

    def on_insert_text(self, buf, where, text, length):
        if not self._in_mode:
            return

        self._remove_duplicate_edit_points()

        self.block_signal(buf, 'insert-text')
        buf.begin_user_action()

        insert = buf.get_iter_at_mark(buf.get_insert())
        atinsert = where.equal(insert)
        wasat = buf.create_mark(None, where, True)

        if self._column_mode:
            self._apply_column_mode()

        if self._edit_points and atinsert:
            # Insert the text at all the edit points
            for mark in self._edit_points:
                piter = buf.get_iter_at_mark(mark)
                
                if not buf.get_iter_at_mark(buf.get_insert()).equal(piter):
                    self._multi_edited = True
                    buf.insert(piter, text)
        else:
            self.remove_edit_points()

        iterwas = buf.get_iter_at_mark(wasat)

        if hasattr(where, 'assign'):
            where.assign(iterwas)
        
        if atinsert:
            buf.move_mark(buf.get_insert(), iterwas)
            buf.move_mark(buf.get_selection_bound(), iterwas)
            
        buf.delete_mark(wasat)
        buf.end_user_action()
        self.unblock_signal(buf, 'insert-text')

    def on_delete_range_before(self, buf, start, end):
        if not self._in_mode:
            return

        self._remove_duplicate_edit_points()
        self._delete_text = start.get_text(end)
        self._delete_length = abs(end.get_offset() - start.get_offset())

        start.order(end)
        self._is_backspace = start.compare(buf.get_iter_at_mark(buf.get_insert())) < 0

    def handle_column_mode_delete(self, mark):
        buf = self._buffer
        start = buf.get_iter_at_mark(mark)
        buf.delete_mark(mark)

        self._view.set_editable(True)

        # Reinsert what was deleted, and apply column mode
        self.block_signal(buf, 'insert-text')

        buf.begin_user_action()
        buf.insert(start, self._delete_text)
        self._apply_column_mode()
        buf.end_user_action()

        self.unblock_signal(buf, 'insert-text')
        self._delete_mode_id = 0
        return False

    def on_delete_range(self, buf, start, end):
        if self._column_mode:
            # Ooooh, what a hack to be able to work with the undo manager
            self._view.set_editable(False)
            mark = buf.create_mark(None, start, True)
            self._delete_mode_id = glib.timeout_add(0, self.handle_column_mode_delete, mark)
        elif self._edit_points:
            if start.equal(buf.get_iter_at_mark(buf.get_insert())):
                self.block_signal(buf, 'delete-range')
                buf.begin_user_action()
                orig = buf.create_mark(None, start, True)

                for mark in self._edit_points:
                    piter = buf.get_iter_at_mark(mark)
                    other = piter.copy()
                    
                    if self._is_backspace:
                        # Remove 'delete_length' chars _before_ piter
                        if not other.backward_chars(self._delete_length):
                            continue
                    else:
                        # Remove 'delete_text' chars _after_ piter
                        if not other.forward_chars(self._delete_length):
                            continue

                    if piter.equal(other):
                        continue

                    piter.order(other)
                    buf.delete(piter, other)
                    self._multi_edited = True

                buf.end_user_action()
                self.unblock_signal(buf, 'delete-range')
                
                piter = buf.get_iter_at_mark(orig)
                buf.delete_mark(orig)
                
                # To be able to have it not crash with old pygtk
                if hasattr(start, 'assign'):
                    start.assign(piter)
                    end.assign(piter)
            else:
                self.remove_edit_points()

    def _cancel_column_mode(self):
        if not self._column_mode:
            return

        self._column_mode = None

        buf = self._buffer
        bounds = buf.get_bounds()

        buf.remove_tag(self._selection_tag, bounds[0], bounds[1])
        
        self.status('<i>%s</i>' % (xml.sax.saxutils.escape(_('Cancelled column mode...'),)))
        self._view.queue_draw()

    def _column_text(self):
        if not self._column_mode:
            return ''
        
        start = self._column_mode[0]
        end = self._column_mode[1]
        buf = self._buffer

        cstart = self._column_mode[2]
        cend = self._column_mode[3]
        
        lines = []
        width = cend - cstart

        while start <= end:
            start_iter, soff = self.get_visible_iter(start, cstart)
            end_iter, eoff = self.get_visible_iter(start, cend)
            
            if soff == 0 and eoff == 0:
                # Just text
                lines.append(start_iter.get_text(end_iter))
            elif (soff < 0 and eoff < 0) or soff > 0:
                # Only spaces
                lines.append(' ' * width)
            elif soff < 0:
                # start to end_iter
                lines.append((' ' * abs(soff)) + start_iter.get_text(end_iter))
            elif eoff != 0:
                # Draw from start_iter to end
                if eoff < 0:
                    end_iter.backward_char()
                    eoff = self._view.get_tab_width() + eoff

                lines.append(start_iter.get_text(end_iter) + (' ' * abs(eoff)))
            else:
                lines.append('')
            
            start += 1
        
        return "\n".join(lines)

    def on_copy_clipboard(self, view):
        if not self._column_mode:
            return
        
        text = self._column_text()

        clipboard = gtk.Clipboard(self._view.get_display())
        clipboard.set_text(text)
        
        view.stop_emission('copy-clipboard')
    
    def on_cut_clipboard(self, view):
        if not self._column_mode:
            return
        
        text = self._column_text()
        clipboard = gtk.Clipboard(self._view.get_display())
        clipboard.set_text(text)
        
        view.stop_emission('cut-clipboard')
        
        self._apply_column_mode()

    def on_mark_set(self, buf, where, mark):
        if not mark == buf.get_insert():
            return

        if self._in_mode:
            if self._column_mode != None:
                # Cancel column mode when cursor moves
                self._cancel_column_mode()
            elif self._edit_points and self._multi_edited:
                # Detect moving up or down a line
                
                diff = where.get_offset() - buf.get_iter_at_mark(self._last_insert).get_offset()

                for point in self._edit_points:
                    piter = buf.get_iter_at_mark(point)
                    piter.set_offset(piter.get_offset() + diff)
                    buf.move_mark(point, piter)

                self._remove_duplicate_edit_points()

        self._buffer.move_mark(self._last_insert, where)

    def on_view_undo(self, view):
        self._cancel_column_mode()
        self.remove_edit_points()
    
    def make_label(self, text):
        lbl = gtk.Label(text)
        lbl.set_alignment(0, 0.5)
        lbl.show()
        
        return lbl
    
    def on_query_tooltip(self, view, x, y, keyboard_mode, tooltip):
        if not self._in_mode:
            return False
        
        geom = view.get_window(gtk.TEXT_WINDOW_TEXT).get_geometry()
        geom2 = view.get_window(gtk.TEXT_WINDOW_LEFT).get_geometry()
        
        if y < geom[3] or x < geom2[2]:
            return False

        table = gtk.Table(4, 2)
        table.set_row_spacings(3)
        table.set_col_spacings(6)

        table.attach(self.make_label('<Enter>:'), 0, 1, 0, 1, gtk.SHRINK | gtk.FILL, gtk.SHRINK | gtk.FILL)
        table.attach(self.make_label('<Ctrl>+E:'), 0, 1, 1, 2, gtk.SHRINK | gtk.FILL, gtk.SHRINK | gtk.FILL)
        table.attach(self.make_label('<Ctrl><Home>:'), 0, 1, 2, 3, gtk.SHRINK | gtk.FILL, gtk.SHRINK | gtk.FILL)
        table.attach(self.make_label('<Ctrl><End>:'), 0, 1, 3, 4, gtk.SHRINK | gtk.FILL, gtk.SHRINK | gtk.FILL)
        
        table.attach(self.make_label(_('Enter column edit mode using selection')), 1, 2, 0, 1)
        table.attach(self.make_label(_('Toggle edit point')), 1, 2, 1, 2)
        table.attach(self.make_label(_('Add edit point at beginning of line/selection')), 1, 2, 2, 3)
        table.attach(self.make_label(_('Add edit point at end of line/selection')), 1, 2, 3, 4)
        
        table.show_all()
        tooltip.set_custom(table)
        return True

    def from_color(self, col):
        return [col.red / float(0x10000), col.green / float(0x10000), col.blue / float(0x10000)]

    def _background_color(self):
        col = self.from_color(self._view.get_style().base[self._view.state])
        if col[2] > 0.8:
            col[2] -= 0.2
        else:
            col[2] += 0.2

        return col

    def on_view_expose_event(self, view, event):
        if event.window == view.get_window(gtk.TEXT_WINDOW_TEXT):
            return self._draw_column_mode(event)

        if event.window != view.get_window(gtk.TEXT_WINDOW_TOP):
            return False

        if not self._in_mode:
            return False

        ctx = event.window.cairo_create()
        col = self._background_color()

        ctx.set_source_rgb(col[0], col[1], col[2])
        ctx.rectangle(event.area)
        ctx.fill_preserve()
        ctx.clip()

        layout = view.create_pango_layout(_('Multi Edit Mode'))

        layout.set_font_description(pango.FontDescription('Sans 10'))
        extents = layout.get_pixel_extents()

        geom = event.window.get_geometry()

        ctx.translate(0.5, 0.5)
        ctx.set_line_width(1)

        col = self.from_color(view.get_style().text[view.state])
        
        ctx.set_source_rgba(col[0], col[1], col[2], 0.6)
        ctx.move_to(geom[0], geom[1] + geom[3] - 1)
        ctx.rel_line_to(geom[2], 0)
        ctx.stroke()

        ctx.set_source_rgb(col[0], col[1], col[2])
        ctx.move_to(geom[2] - extents[1][2] - 3, (geom[3] - extents[1][3]) / 2)
        ctx.show_layout(layout)
        
        if not self._status:
            status = ''
        else:
            status = str(self._status)

        if status:
            layout.set_markup(status)
            
            ctx.move_to(3, (geom[3] - extents[1][3]) / 2)
            ctx.show_layout(layout)

        return False

# ex:ts=4:et:
