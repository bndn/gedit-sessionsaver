# Reverse plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libreverse.la

libreverse_la_SOURCES = reverse.c

INCLUDES = -I$(top_srcdir)/src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)
