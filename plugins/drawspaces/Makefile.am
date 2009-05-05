# Drawspaces plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libdrawspaces.la

libdrawspaces_la_SOURCES = \
	gedit-drawspaces-plugin.h				\
	gedit-drawspaces-plugin.c				

libdrawspaces_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdrawspaces_la_LIBADD = $(GEDIT_LIBS)

uidir = $(GEDIT_PLUGINS_DATA_DIR)/drawspaces
ui_DATA = drawspaces.ui

# Plugin Info

plugin_in_files = drawspaces.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(plugin_in_files) $(ui_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
