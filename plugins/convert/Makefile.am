# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libconvert.la

libconvert_la_SOURCES = convert.c

INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)


gladedir = $(datadir)/gedit/glade
glade_DATA = convert.glade

EXTRA_DIST = $(glade_DATA)
