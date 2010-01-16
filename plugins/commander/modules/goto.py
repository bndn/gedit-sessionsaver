"""Goto specific places in the document"""
import os
import commander.commands as commands

__commander_module__ = True

def __default__(view, line, column=0):
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
		raise commands.exceptions.Execute('Please specify a valid line number')

	linnum = max(0, linnum)
	column = max(0, column)

	citer = buf.get_iter_at_line(linnum)
	citer.forward_chars(column)
	
	buf.move_mark(ins, citer)
	buf.move_mark(buf.get_selection_bound(), buf.get_iter_at_mark(ins))
	return False
