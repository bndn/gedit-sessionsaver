# Show tabbar plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libshowtabbar.la

libshowtabbar_la_SOURCES = \
	gedit-show-tabbar-plugin.h	\
	gedit-show-tabbar-plugin.c

libshowtabbar_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libshowtabbar_la_LIBADD = $(GEDIT_LIBS)

# Plugin Info

plugin_in_files = showtabbar.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

# GSettings Schemas

gsettings_SCHEMAS = org.gnome.gedit.showtabbar.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = $(plugin_in_files) $(gsettings_SCHEMAS)

CLEANFILES = $(plugin_DATA) $(schemas_DATA)
DISTCLEANFILES = $(plugin_DATA) $(schemas_DATA)

-include $(top_srcdir)/git.mk
