# Advanced Editing plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)
supportdir = $(GEDIT_PLUGINS_DATA_DIR)/bookmarks

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libbookmarks.la

libbookmarks_la_SOURCES = \
	gedit-bookmarks-plugin.h			\
	gedit-bookmarks-plugin.c	

libbookmarks_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)				\
libbookmarks_la_LDADD =	$(GEDIT_LIBS)

support_DATA = bookmark.png

# Plugin Info
plugin_in_files = bookmarks.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

BUILDFILES = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

plugin_DATA = $(BUILDFILES)

EXTRA_DIST = $(plugin_in_files) $(support_DATA)

CLEANFILES = $(BUILDFILES) 
DISTCLEANFILES = $(BUILDFILES)

-include $(top_srcdir)/git.mk
