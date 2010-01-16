import math
import cairo
import gtk

def _draw_transparent_background(ct, region):
	ct.set_operator(cairo.OPERATOR_CLEAR)
	ct.region(region)
	ct.fill()

def _on_widget_expose(widget, evnt):
	ct = evnt.window.cairo_create()
	ct.save()
	
	# Basicly just clear the background
	_draw_transparent_background(ct, evnt.region)

	ct.restore()
	return False
	
def _on_parent_expose(parent, evnt, widget):
	#if evnt.window != widget.window.get_parent():
	#	return False
	
	# Composite child window back onto parent
	ct = evnt.window.cairo_create()
	ct.set_source_pixmap(widget.window, widget.allocation.x, widget.allocation.y)
	region = gtk.gdk.region_rectangle(widget.allocation)
	
	region.intersect(evnt.region)
	ct.region(region)
	ct.clip()
	
	# Composite it now
	ct.set_operator(cairo.OPERATOR_OVER)
	ct.paint_with_alpha(0.5)
	
	return True

def _on_widget_realize(widget):
	widget.window.set_back_pixmap(None, False)

	if widget.is_composited():
		widget.window.set_composited(True)
		widget.get_parent().connect_after('expose-event', _on_parent_expose, widget)

def transparent_background(widget):
	#widget.set_name('transparent-background')
	widget.connect_after('realize', _on_widget_realize)
	widget.set_app_paintable(True)

	cmap = widget.get_screen().get_rgba_colormap()
	
	if cmap:
		widget.set_colormap(cmap)

	widget.connect('expose-event', _on_widget_expose)

def set_rounded_rectangle_path(ct, x, y, width, height, radius):
	ct.move_to(x + radius, y)

	ct.arc(x + width - radius, y + radius, radius, math.pi * 1.5, math.pi * 2)
	ct.arc(x + width - radius, y + height - radius, radius, 0, math.pi * 0.5)
	ct.arc(x + radius, y + height - radius, radius, math.pi * 0.5, math.pi)
	ct.arc(x + radius, y + radius, radius, math.pi, math.pi * 1.5)

gtk.rc_parse_string("""
style "OverrideBackground" {
	engine "pixmap" {
		image {
			function = FLAT_BOX
		}
		image {
			function = BOX
		}
	}
}

widget "*.transparent-background" style "OverrideBackground"
""")
