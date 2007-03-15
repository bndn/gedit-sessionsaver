# Draw Spaces Plugin

plugindir = $(libdir)/gedit-2/plugins

plugin_in_files = drawspaces.gedit-plugin.desktop.in
plugin_file = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_PYTHON = drawspaces.py
plugin_DATA = drawspaces.glade $(plugin_file)

EXTRA_DIST = $(plugin_in_files) $(plugin_DATA)
CLEANFILES = $(plugin_file)
DISTCLEANFILES = $(plugin_file)
