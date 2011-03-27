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
	gedit-bookmarks-plugin.h				\
	gedit-bookmarks-plugin.c				\
	messages/gedit-bookmarks-message-add.c			\
	messages/gedit-bookmarks-message-add.h			\
	messages/gedit-bookmarks-message-goto-next.c		\
	messages/gedit-bookmarks-message-goto-next.h		\
	messages/gedit-bookmarks-message-goto-previous.c	\
	messages/gedit-bookmarks-message-goto-previous.h	\
	messages/gedit-bookmarks-message-remove.c		\
	messages/gedit-bookmarks-message-remove.h		\
	messages/gedit-bookmarks-message-toggle.c		\
	messages/gedit-bookmarks-message-toggle.h		\
	messages/messages.h

libbookmarks_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libbookmarks_la_LIBADD = $(GEDIT_LIBS)

support_DATA = bookmark.png

# Plugin Info
plugin_in_files = bookmarks.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

BUILDFILES = $(plugin_in_files:.plugin.desktop.in=.plugin)

plugin_DATA = $(BUILDFILES)

EXTRA_DIST = $(plugin_in_files) $(support_DATA)

CLEANFILES = $(BUILDFILES) 
DISTCLEANFILES = $(BUILDFILES)

-include $(top_srcdir)/git.mk
