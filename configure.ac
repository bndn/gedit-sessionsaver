dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
 
m4_define(gedit_plugins_major_version,  3)
m4_define(gedit_plugins_minor_version,  0)
m4_define(gedit_plugins_micro_version,  3)
m4_define(gedit_plugins_version, gedit_plugins_major_version.gedit_plugins_minor_version.gedit_plugins_micro_version)

AC_INIT([gedit-plugins],
        [gedit_plugins_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gedit],
        [gedit-plugins])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([plugins])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_PREFIX_PROGRAM([gedit])

AM_INIT_AUTOMAKE([1.11 tar-ustar dist-xz no-dist-gzip])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AM_PATH_PYTHON

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

dnl check for win32 platform
AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

# ================================================================
# Gettext stuff.
# ================================================================
IT_PROG_INTLTOOL([0.40.0])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=gedit-plugins
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

# ================================================================
# Start of pkg-config checks
# ================================================================

PKG_CHECK_MODULES(GEDIT, [
	glib-2.0 >= 2.26.0
	gio-2.0 >= 2.26.0
	gtk+-3.0 >= 3.0.0
	gtksourceview-3.0 >= 3.0.0
	libpeas-1.0 >= 0.7.3
	libpeas-gtk-1.0 >= 0.7.3
	gedit >= 3.0.0
])

dnl ================================================================
dnl GSettings stuff
dnl ================================================================

GLIB_GSETTINGS

# ================================================================
# Plugins
# ================================================================

#FIXME
GEDIT_HAS_PYTHON=1
#/FIXME

AC_MSG_CHECKING([which plugins to build])

ALL_PLUGINS="bookmarks drawspaces wordcompletion taglist"
USEFUL_PLUGINS="bookmarks drawspaces wordcompletion taglist"
DEFAULT_PLUGINS="bookmarks drawspaces wordcompletion taglist"

PYTHON_ALL_PLUGINS="bracketcompletion charmap codecomment colorpicker commander joinlines multiedit textsize sessionsaver smartspaces terminal synctex"
PYTHON_USEFUL_PLUGINS="bracketcompletion charmap codecomment colorpicker commander joinlines multiedit textsize sessionsaver smartspaces terminal synctex"
PYTHON_DEFAULT_PLUGINS="bracketcompletion charmap codecomment colorpicker commander joinlines multiedit textsize sessionsaver smartspaces terminal synctex"

DIST_PLUGINS="$ALL_PLUGINS $PYTHON_ALL_PLUGINS"

if test $GEDIT_HAS_PYTHON
then
	ALL_PLUGINS="$ALL_PLUGINS $PYTHON_ALL_PLUGINS"
	USEFUL_PLUGINS="$USEFUL_PLUGINS $PYTHON_USEFUL_PLUGINS"
	DEFAULT_PLUGINS="$DEFAULT_PLUGINS $PYTHON_DEFAULT_PLUGINS"
else
	AC_MSG_WARN([gedit compiled without python support, plugins $PYTHON_ALL_PLUGINS will not be available])
fi

# due to an autoconf bug, commas in the first arg to
# AS_HELP_STRING() cause problems.
#
# AS_HELP_STRING(--with-plugins=ext1,ext2,...,build the specified plugins)
AC_ARG_WITH([plugins],
[  --with-plugins=plugin1,plugin2,...
			  build the specified plugins. Available:
			  bracketcompletion, charmap, codecomment,
			  colorpicker, drawspaces, joinlines, multiedit,
			  textsize, sessionsaver, smartspaces, taglist,
			  terminal, wordcompletion, as well as the aliases
			  default, all, and really-all],
			  [plugins=$with_plugins],
			  [plugins="default"])

if test "x$with_plugins" = xyes
then
	plugins="default"
fi

need_python=no
explicit_plugins=
PLUGINS=
IFS="${IFS=	}"; geditpl_save_ifs="$IFS"; IFS=","
for pl in $plugins
do
	if echo "$ALL_PLUGINS" | egrep "(^| )$pl(\$| )" > /dev/null
	then
		PLUGINS="$PLUGINS $pl"
		explicit_plugins="$explicit_plugins $pl"
	elif test "x$pl" = "xdefault"
	then
		PLUGINS="$PLUGINS $DEFAULT_PLUGINS"
	elif test "x$pl" = "xall"
	then
		PLUGINS="$PLUGINS $USEFUL_PLUGINS"
	elif test "x$pl" = "xreally-all"
	then
		PLUGINS="$PLUGINS $ALL_PLUGINS"
	else
		AC_MSG_ERROR([the specified plugin $pl does not exist])
	fi
	
	if echo "$PYTHON_ALL_PLUGINS" | egrep "(^| )$pl(\$| )" > /dev/null
	then
		need_python=yes
	fi
done
IFS="$geditpl_save_ifs"

AC_MSG_RESULT([$PLUGINS])


# ================================================================
# Python
# ================================================================

AC_MSG_CHECKING([whether Python support is requested])
AC_ARG_ENABLE([python],
	      AS_HELP_STRING([--enable-python], [Enable python support]),
	      [enable_python=$enableval],
	      [enable_python="yes"])

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

disabled_plugins=
# ================================================================
# Plugin specific checks
# ================================================================
check_plugin_defined() {
	for p in $2
	do
		if test "$p" = "$1"
		then
			return 1
		fi
	done
	return 0
}

plugin_defined() {
	check_plugin_defined "$1" "$PLUGINS"
	return $?
}

plugin_defined_explicit() {
	check_plugin_defined "$1" "$explicit_plugins"
	return $?
}

undef_plugin() {
	if test -z "$disabled_plugins"
	then
		disabled_plugins="$1 ($2)"
	else
		disabled_plugins="$disabled_plugins, $1 ($2)"
	fi
	
	PLUGINS=$(echo "$PLUGINS" | sed -e "s/[[^a-zA-Z0-9_]]$1[[ \t\r\n\f]]\{1,\}/ /" -e "s/[[^a-zA-Z0-9_]]$1$//" -e "s/^$1[[ \t\r\n\f]]\{1,\}//")
}

# ================================================================
# disable all python plugins if there is no python support
# ================================================================
if test "x$enable_python" = "xno"
then
	for pl in $PYTHON_ALL_PLUGINS
	do
		undef_plugin "$pl" "no python support"
	done
fi

# ================================================================
# Synctex (dbus-python)
# ================================================================
plugin_defined synctex
if test "$?" = 1
then
	DBUSPYTHON_REQUIRED=0.82
	PKG_CHECK_MODULES([DBUSPYTHON],
		[dbus-python >= $DBUSPYTHON_REQUIRED],
		[have_synctex=yes],[have_synctex=no])

	AC_SUBST([DBUS_PYTHON_CFLAGS])
	AC_SUBST([DBUS_PYTHON_LIBS])

	if test "x$have_synctex" = "xno"; then
		plugin_defined_explicit synctex
		if test "$?" = 1
		then
			AC_MSG_ERROR([dbus-python could not be found, needed for synctex plugin])
		else
			AC_MSG_WARN([dbus-python could not be found, synctex plugin will be disabled])
			undef_plugin synctex "dbus-python not found"
		fi
	fi
fi

if test -z "$disabled_plugins"
then
	disabled_plugins="none"
fi

PLUGINS=$(echo $PLUGINS | tr ' ' '\n' | sort | uniq | tr '\n' ' ')
AC_SUBST([DIST_PLUGINS])
AC_SUBST([BUILD_PLUGINS],[$PLUGINS])

# ================================================================
# Misc
# ================================================================
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

AC_ARG_ENABLE(deprecations,
              [AS_HELP_STRING([--enable-deprecations],
                              [warn about deprecated usages [default=no]])],,
              [enable_deprecations=no])

if test "x$enable_deprecations" = "xyes"
then
	DISABLE_DEPRECATED_CFLAGS="\
		-DG_DISABLE_DEPRECATED		\
		-DGDK_DISABLE_DEPRECATED	\
		-DGTK_DISABLE_DEPRECATED	\
		-DGDK_PIXBUF_DISABLE_DEPRECATED	\
		-DGNOME_DISABLE_DEPRECATED"
	AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
fi

if test "$platform_win32" = yes; then
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version -no-undefined"
else
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
fi
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

GEDIT_PLUGINS_LIBS_DIR="$libdir/gedit/plugins"
AC_SUBST(GEDIT_PLUGINS_LIBS_DIR)

GEDIT_PLUGINS_DATA_DIR="$datadir/gedit/plugins"
AC_SUBST(GEDIT_PLUGINS_DATA_DIR)

AC_CONFIG_FILES([
Makefile
plugins/gpdefs.py
plugins/Makefile
plugins/bookmarks/bookmarks.plugin.desktop.in
plugins/bookmarks/Makefile
plugins/bracketcompletion/bracketcompletion.plugin.desktop.in
plugins/bracketcompletion/Makefile
plugins/charmap/charmap.plugin.desktop.in
plugins/charmap/Makefile
plugins/charmap/charmap/Makefile
plugins/codecomment/codecomment.plugin.desktop.in
plugins/codecomment/Makefile
plugins/colorpicker/colorpicker.plugin.desktop.in
plugins/colorpicker/Makefile
plugins/commander/commander/commands/Makefile
plugins/commander/commander.plugin.desktop.in
plugins/commander/commander/Makefile
plugins/commander/Makefile
plugins/commander/modules/find/Makefile
plugins/commander/modules/Makefile
plugins/drawspaces/drawspaces.plugin.desktop.in
plugins/drawspaces/Makefile
plugins/drawspaces/org.gnome.gedit.plugins.drawspaces.gschema.xml.in
plugins/joinlines/joinlines.plugin.desktop.in
plugins/joinlines/Makefile
plugins/multiedit/Makefile
plugins/multiedit/multiedit.plugin.desktop.in
plugins/multiedit/multiedit/Makefile
plugins/sessionsaver/Makefile
plugins/sessionsaver/sessionsaver.plugin.desktop.in
plugins/smartspaces/Makefile
plugins/smartspaces/smartspaces.plugin.desktop.in
plugins/synctex/Makefile
plugins/synctex/synctex.plugin.desktop.in
plugins/synctex/synctex/Makefile
plugins/taglist/taglist.plugin.desktop.in
plugins/taglist/Makefile
plugins/terminal/Makefile
plugins/terminal/org.gnome.gedit.plugins.terminal.gschema.xml.in
plugins/terminal/terminal.plugin.desktop.in
plugins/textsize/Makefile
plugins/textsize/textsize.plugin.desktop.in
plugins/textsize/textsize/Makefile
plugins/wordcompletion/Makefile
plugins/wordcompletion/wordcompletion.plugin.desktop.in
po/Makefile.in])

AC_OUTPUT

echo "

Configuration:

	Source code location:   ${srcdir}
	Compiler:               ${CC}
	Prefix:			${prefix}
	Python Plugins Support: $enable_python
	Plugins:                $PLUGINS
	
	Disabled plugins:       $disabled_plugins

Note: you have to install these plugins into the same prefix as your gedit
installation (probably /usr if you're using your distro packages, /usr/local
if you have compiled it on your own).
"

