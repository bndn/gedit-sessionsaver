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

# GConf Schemas

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gedit-show-tabbar-plugin.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p ; \
	                done \
	        fi
else
install-data-local:
endif

EXTRA_DIST = $(plugin_in_files) $(schemas_in_files)

CLEANFILES = $(plugin_DATA) $(schemas_DATA)
DISTCLEANFILES = $(plugin_DATA) $(schemas_DATA)

-include $(top_srcdir)/git.mk
