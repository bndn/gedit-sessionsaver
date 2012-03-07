# Drawspaces plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libdrawspaces.la

BUILT_SOURCES = \
	gedit-drawspaces-resources.c

libdrawspaces_la_SOURCES = \
	gedit-drawspaces-plugin.h				\
	gedit-drawspaces-plugin.c				\
	$(BUILT_SOURCES)

libdrawspaces_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdrawspaces_la_LIBADD = $(GEDIT_LIBS)

gedit-drawspaces-resources.c: gedit-drawspaces.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gedit-drawspaces.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $(srcdir)/gedit-drawspaces.gresource.xml

# Plugin Info

plugin_in_files = drawspaces.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

gsettings_SCHEMAS = org.gnome.gedit.plugins.drawspaces.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files) \
	org.gnome.gedit.plugins.drawspaces.gschema.xml.in.in \
	gedit-drawspaces.gresource.xml \
	gedit-drawspaces-plugin.ui

CLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
