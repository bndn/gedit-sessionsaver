# -*- coding: utf8 -*-
from os.path import join, dirname, abspath, expanduser, exists
import os, sys
import gedit
import gtk, gtk.glade, gtksourceview, gconf, pango

QUICKIES_DIR = abspath(dirname(__file__))
GLADE_UI = join(QUICKIES_DIR, "quickies.glade")
DATA_DIR = expanduser("~/.gnome2/gedit/plugins/")
QUICKIES_SCRIPTS = join(DATA_DIR, "quickies_scripts.py")

# We create the python file containing the various scripts
def init_scripts():	
	if not exists(DATA_DIR):
		os.mkdir(DATA_DIR)
		
	make_new = False
	if not exists(QUICKIES_SCRIPTS):
		make_new = True
	else:
		content = file(QUICKIES_SCRIPTS, "r").read()
		if content.strip() == "":
			make_new = True
			
	if make_new:
		file(QUICKIES_SCRIPTS, "w").write(
"""
SCRIPTS = {
"Sort": 
\"\"\"def process(text):
	lines = text.split("\\\\n")
	lines.sort()
	return '\\\\n'.join(lines)
\"\"\",
"Python Comment":
\"\"\"def process(text):
	lines = text.split("\\\\n")
	for i in range(len(lines)):
		lines[i] = "# "+lines[i]
	return '\\\\n'.join(lines)
\"\"\",
"Python Uncomment":
\"\"\"def process(text):
	lines = text.split("\\\\n")
	for i in range(len(lines)):
		if lines[i].startswith("# "):
			lines[i] = lines[i][2:]
	return '\\\\n'.join(lines)
\"\"\",
}
""")
init_scripts()
import quickies_scripts

class QuickiesPlugin(gedit.Plugin):

	def __init__(self):
		gedit.Plugin.__init__(self)
		lang_manager = gtksourceview.SourceLanguagesManager()
		self.lang = lang_manager.get_language_from_mime_type("text/x-python")
		self.gconf = gconf.client_get_default()
		self.gconf.add_dir("/apps/gedit-2/preferences/editor/font", gconf.CLIENT_PRELOAD_RECURSIVE)
		
	def activate(self, window):
		# Build the panel
		glade = gtk.glade.XML(GLADE_UI, root="panel")
		panel = glade.get_widget("panel")
		panel.show_all()
		
		buff = gtksourceview.SourceBuffer()
		props = {
			"language": self.lang,
			"highlight": True,
			"check-brackets": True,
		}
		for prop, val in props.items():
			buff.set_property(prop, val)
		buff._dirty = False

		textview = gtksourceview.SourceView(buff)
		props = {
			"auto-indent": True,
			"highlight-current-line" : True,
			"show-line-markers": True,
			"show-line-numbers": True,
			"smart-home-end": True,
		}
		for prop, val in props.items():
			textview.set_property(prop, val)
	
		self.gconf.notify_add("/apps/gedit-2/preferences/editor/font/editor_font",
				lambda x, y, z, a: self.on_font_change(z.value, textview))
		self.on_font_change(self.gconf.get_string("/apps/gedit-2/preferences/editor/font/editor_font"),
				textview)
		
		viewport = glade.get_widget("viewport")
		textview.show_all()
		viewport.add(textview)
		
		convert = glade.get_widget("convert")
		save = glade.get_widget("save")
		
		scripts = glade.get_widget("script")
		scripts.connect('changed', self.on_script_change, textview, convert, save)
		fill_scripts(scripts)
		
		cell = gtk.CellRendererText()
		scripts.pack_start(cell, True)
		scripts.add_attribute(cell, 'text', 0)

		image = gtk.Image()
		image.set_from_stock(gtk.STOCK_EXECUTE, gtk.ICON_SIZE_BUTTON)
		convert.set_image(image)
		
		image = gtk.Image()
		image.set_from_stock(gtk.STOCK_SAVE, gtk.ICON_SIZE_BUTTON)
		save.set_image(image)
		
		convert.connect('clicked', self.on_convert, window, textview, scripts, save)
		buff.connect('changed', self.on_script_modified, scripts, convert, save)
		
		remove = glade.get_widget("remove")
		remove.connect('clicked', self.on_remove_script, scripts)
		
		bottom = window.get_bottom_panel()
		bottom.add_item(panel, "Quick Scripts", gtk.STOCK_EXECUTE)
		
		
		save.connect('clicked', lambda button: write_scripts())
		window.connect('delete-event', lambda win, event: write_scripts())
		
		# Store data in the window object
		window._quickies_window_data = (panel)
	
	def deactivate(self, window):
		# Retreive the data of the window object
		panel = window._quickies_window_data
		del window._quickies_window_data
		
		# Remove the panel
		bottom = window.get_bottom_panel()
		bottom.remove_item(panel)
		
	def update_ui(self, window):
		pass
	
	def on_font_change(self, value, textview):
		if value == None:
			return
		
		font_name = None
		if not hasattr(value, 'type'):
			font_name = value
		elif value.type == gconf.VALUE_STRING:
			font_name = value.get_string()

		
		font = pango.FontDescription("Monospace 10")
		try:
			font = pango.FontDescription(font_name)
		except:
			pass
			
		textview.modify_font(font)
		
	def on_script_modified(self, buff, combo, button, save):
		title = "Unknown"
		try:
			title = buff.get_text(buff.get_start_iter(), buff.get_end_iter(), False).split('\n', 1)[0][2:]
		except:
			print "Error:on_script_modified:Couldn't extract script title"
		
		if title != combo.get_model()[combo.get_active_iter()][0]:
			button.set_label("Save As “%s” and Execute" % title)
			save.set_label("Save As “%s”" % title)
		else:
			button.set_label("Save and Execute")
			save.set_label("Save")
			
		save.set_sensitive(True)
		buff._dirty = True
		
	def on_script_change(self, combo, textview, button, save):
		row = combo.get_model()[combo.get_active()]
		textview.get_buffer().set_text(row[1])
		button.set_label("Execute")
		save.set_label("Save")
		save.set_sensitive(False)
		textview.get_buffer()._dirty = False
			
	def on_remove_script(self, button, combo):
		remove_script(combo.get_active_iter(), combo.get_model())
		combo.set_active_iter(combo.get_model().get_iter_first())
				
	def on_convert(self, button, window, textview, combo, save):
		buff = window.get_active_document()
		
		# Save the script
		button.set_label("Execute")
		save.set_label("Save")
		save.set_sensitive(False)
		body_buff = textview.get_buffer()
		if body_buff._dirty:
			body_buff._dirty = False
			script = body_buff.get_text(body_buff.get_start_iter(), body_buff.get_end_iter(), False)
			iter = save_script(script, combo.get_model())
			if iter != None:
				combo.set_active_iter(iter)
						
		start = None
		end = None
		selection = buff.get_selection_bounds()
		if len(selection) == 0:
			# No selection, we will replace the buffer
			start = buff.get_start_iter()
			end = buff.get_end_iter()
		else:
			# Selected text, replace it by processed result
			start, end = selection
		
		text = buff.get_text(start, end, False)
		text = self.process_text(combo, textview.get_buffer(), text)
		
		if len(selection) == 0:
			buff.set_text(text)
		else:		
			buff.delete_selection(False, True)
			buff.insert_at_cursor(text)
	
	def process_text(self, combo, buff, text):
		body = combo.get_model()[combo.get_active_iter()][1]
		try:
			# Evaluate the function
			exec body
			result = text
			if "process" in locals():
				result = process(text)
			
			# Remove the function from scope
			del process
			
			return result
		except Exception, msg:
			print 'Error:', msg

def fill_scripts(combo):
	"""
	Create a liststore with the scripts defined in the quickies_scripts module
	and set it as modelfor the passed combo.
	"""
	store = gtk.ListStore(str, str)
	for name, fun_str in quickies_scripts.SCRIPTS.items():
		store.append([name, "# %s\n%s" % (name, fun_str)])

	combo.set_model(store)
	combo.set_active(0)
	
def save_script(text, model):
	"""
	Save the given script (text) in the quickies_scripts module
	Update the passed model with the new or existing script.
	"""
	title = "Unknown"
	body = text
	try:
		title = text.split('\n', 1)[0][2:]
		body = text.split('\n', 1)[1]
	except:
		print "Error:save_script:Couldn't extract script title"
	
	quickies_scripts.SCRIPTS[title] = body
	for row in model:
		if row[0] == title:
			row[1] = text
			break
	else:
		if title != "" and body != "":
			return append([title, text])

def remove_script(iter, model):
	"""
	Remove the given script (pointed by iter in model) from the model
	and from the quickies_scripts module.
	"""
	name = model[iter][0]
	del quickies_scripts.SCRIPTS[name]
	del model[iter]
	
		
def write_scripts():
	"""
	Write the in-memory quickies_scripts to the file and reload the module
	"""
	f = file(QUICKIES_SCRIPTS, "w")
	
	f.write('SCRIPTS = {')
	for name, body in quickies_scripts.SCRIPTS.items():
		body = body.replace('\\', '\\\\')
		body = body.replace('"', '\\"')
		f.write('"%s":\n"""%s""",\n' % (name, body))
	f.write('}\n')
	f.close()
	
	reload(quickies_scripts)
		
