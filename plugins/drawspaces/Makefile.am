# Drawspaces plugin
plugindir = $(libdir)/gedit-2/plugins

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	-DGEDIT_UIDIR=\""$(datadir)/gedit-2/plugins/drawspaces"\" \
	-DGEDIT_LOCALEDIR=\""$(datadir)/locale"\"

plugin_LTLIBRARIES = libdrawspaces.la

libdrawspaces_la_SOURCES = \
	gedit-drawspaces-plugin.h				\
	gedit-drawspaces-plugin.c				

libdrawspaces_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdrawspaces_la_LIBADD = $(GEDIT_LIBS)

uidir = $(datadir)/gedit-2/plugins/drawspaces
ui_DATA = drawspaces.ui

# Plugin Info

plugin_in_files = drawspaces.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(plugin_in_files) $(ui_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
