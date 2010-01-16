import commander.commands as commands

__commander_module__ = True

def remove_trailing_spaces(view, removeall=False):
	"""Remove trailing spaces: format.remove-trailing-spaces

Remove trailing spaces in the selection. If there is no selection, trailing
spaces are removed from the whole document."""
 
	if removeall:
		buffers = view.get_toplevel().get_documents()
	else:
		buffers = [view.get_buffer()]

	for buf in buffers:
		bounds = buf.get_selection_bounds()

		if not bounds:
			bounds = buf.get_bounds()

		buf.begin_user_action()

		try:
			# For each line, remove trailing spaces
			if not bounds[1].ends_line():
				bounds[1].forward_to_line_end()

			until = buf.create_mark(None, bounds[1], False)
			start = bounds[0]
			start.set_line_offset(0)

			while start.compare(buf.get_iter_at_mark(until)) < 0:
				end = start.copy()
				end.forward_to_line_end()
				last = end.copy()

				if end.equal(buf.get_end_iter()):
					end.backward_char()

				while end.get_char().isspace() and end.compare(start) > 0:
					end.backward_char()

				if not end.ends_line():
					if not end.get_char().isspace():
						end.forward_char()

					buf.delete(end, last)

				start = end.copy()
				start.forward_line()

		except Exception, e:
			print e

		buf.delete_mark(until)
		buf.end_user_action()

	return commands.result.HIDE

def _transform(view, how):
	buf = view.get_buffer()
	bounds = buf.get_selection_bounds()

	if not bounds:
		start = buf.get_iter_at_mark(buf.get_insert())
		end = start.copy()

		if not end.ends_line():
			end.forward_to_line_end()

		bounds = [start, end]

	if not bounds[0].equal(bounds[1]):
		text = how(bounds[0].get_text(bounds[1]))

		buf.begin_user_action()
		buf.delete(bounds[0], bounds[1])
		buf.insert(bounds[0], text)
		buf.end_user_action()

	return commands.result.HIDE

def upper(view):
	"""Make upper case: format.upper

Transform text in selection to upper case."""
	return _transform(view, lambda x: x.upper())

def lower(view):
	"""Make lower case: format.lower

Transform text in selection to lower case."""
	return _transform(view, lambda x: x.lower())

def title(view):
	"""Make title case: format.title

Transform text in selection to title case."""
	return _transform(view, lambda x: x.title()).replace('_', '')
