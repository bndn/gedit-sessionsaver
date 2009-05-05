# Charmap Panel plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(GUCHARMAP_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libcharmap.la

libcharmap_la_SOURCES = \
	gedit-charmap-plugin.h				\
	gedit-charmap-plugin.c				\
	gedit-charmap-panel.h				\
	gedit-charmap-panel.c

libcharmap_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libcharmap_la_LIBADD = $(GEDIT_LIBS) $(GUCHARMAP_LIBS)

# Plugin Info

plugin_in_files = charmap.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
