# Process this file with autoconf to produce a configure script.

GEDIT_MAJOR=2.14
GEDIT_PLUGINS_MAJOR=2.15

AC_INIT(gedit-plugins, 2.15, http://bugzilla.gnome.org/enter_bug.cgi?product=gedit)

AC_CONFIG_SRCDIR([plugins])
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(GEDIT_PLUGINS_MAJOR)

AC_PREREQ(2.59)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

GNOME_DOC_INIT

AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])
AM_DISABLE_STATIC

AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.34.90])

AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")

# ================================================================
# Gettext stuff.
# ================================================================

GETTEXT_PACKAGE=gedit-plugins
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

# ================================================================
# Start of pkg-config checks
# ================================================================

PKG_CHECK_MODULES(GEDIT, [
	glib-2.0 >= 2.8.0
	gtk+-2.0 >= 2.8.0
	gtksourceview-1.0 >= 1.2.0
	gedit-2.14 >= 2.14.0
	libgnomeui-2.0 >= 2.13.0
	libglade-2.0 >= 2.4.0
	libgnomeprintui-2.2 >= 2.6.0
	gnome-vfs-2.0 >= 2.13.4
])
GEDIT_LIBS="${GEDIT_LIBS}"
AC_SUBST(GEDIT_LIBS)
AC_SUBST(GEDIT_CFLAGS)

# ================================================================
# Plugins
# ================================================================

#FIXME
GEDIT_HAS_PYTHON=1
#/FIXME

AC_MSG_CHECKING([which plugins to build])

ALL_PLUGINS="showtabbar"
USEFUL_PLUGINS="showtabbar"
DEFAULT_PLUGINS="showtabbar"

PYTHON_ALL_PLUGINS="joinlines terminal"
PYTHON_USEFUL_PLUGINS="joinlines terminal"
PYTHON_DEFAULT_PLUGINS="joinlines terminal"

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
			  $ALL_PLUGINS,
			  as well as the aliases default, all, and really-all],
			  [plugins=$with_plugins],
			  [plugins="default"])

if test "x$with_plugins" = xyes
then
	plugins="default"
fi

need_python=no
PLUGINS=
IFS="${IFS=	}"; geditpl_save_ifs="$IFS"; IFS=","
for pl in $plugins
do
	if echo "$ALL_PLUGINS" | egrep "(^| )$pl(\$| )" > /dev/null
	then
		PLUGINS="$PLUGINS $pl"
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

AC_SUBST([DIST_PLUGINS])
AC_SUBST([BUILD_PLUGINS],[`echo $PLUGINS | tr ' ' '\n' | sort | uniq | tr '\n' ' '`])

AC_MSG_RESULT([$PLUGINS])

# ================================================================
# GConf related settings
# ================================================================

AM_GCONF_SOURCE_2

# ================================================================
# GEDIT_DATA_DIR - Sets the location of the data directory, where we install
# source files added to projects (such as all the Gnome m4 macros).
# This will substitute GEDIT_DATA_DIR in config.h.
# ================================================================

if test "x${datadir}" = 'x${prefix}/share'
then
	if test "x${prefix}" = "xNONE";
	then
		AC_DEFINE_UNQUOTED(GEDIT_DATA_DIR,
				   "${ac_default_prefix}/share/${PACKAGE}",
				   [Directory of data files])
	else
		AC_DEFINE_UNQUOTED(GEDIT_DATA_DIR,
				   "${prefix}/share/${PACKAGE}",
				   [Directory of data files])
	fi
else
	AC_DEFINE_UNQUOTED(GEDIT_DATA_DIR,
			   "${datadir}/${PACKAGE}",
			   [Directory of data files])
fi

# ================================================================
# Python
# ================================================================

AC_MSG_CHECKING([whether Python support is requested])
AC_ARG_ENABLE([python],
	      AS_HELP_STRING([--enable-python], [Enable python support]),
	      [enable_python=$enableval have_python=$enableval],
	      [enable_python=autodetect have_python=yes])
AC_MSG_RESULT([$enable_python])

if test "x$have_python" != "xno"
then
	AM_PATH_PYTHON([2.3], [], [no])
	if test "x$PYTHON" = "x:"
	then
		have_python=no
	fi
fi

if test "x$have_python" != "xno"
then
	AM_CHECK_PYTHON_HEADERS([], [have_python=no])
fi

if test "x$have_python" != "xno"
then
	PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
	PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
	PYTHON_LIBS="-lpython$PYTHON_VERSION"
	PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
	PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
	PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
	PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_EXTRA_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
	AC_SUBST([PYTHON_LIBS])
	AC_SUBST([PYTHON_LIB_LOC])
	AC_SUBST([PYTHON_CFLAGS])
	AC_SUBST([PYTHON_EXTRA_LIBS])

	# FIXME: do we really need this test?
	AC_MSG_CHECKING([whether we can build a shared library depending on libpython])
	rm -rf testpython
	mkdir testpython
	cd testpython
	cat > testpython.c <<EOF
#include <Python.h>
int testpython (void)
{
Py_Exit (0);
}
EOF

	if /bin/sh ../libtool --mode=compile ${CC} $PYTHON_CFLAGS -c testpython.c >/dev/null 2>&1 && \
		/bin/sh ../libtool --mode=link ${CC} -o testpython.la -rpath `pwd` -module -avoid-version $PYTHON_LIB_LOC testpython.lo $PYTHON_LIBS $PYTHON_EXTRA_LIBS >/dev/null 2>&1 && \
		grep 'dlname.*testpython' testpython.la >/dev/null 2>&1;
	then
		result=yes
	else
		result=no
		have_python=no
	fi
	cd ..
	rm -rf testpython
	AC_MSG_RESULT([$result])
fi

if test "x$have_python" != "xno"
then
	PYGTK_REQUIRED=2.8.0
	GNOME_PYTHON_DESKTOP_REQUIRED=2.13.3

	PKG_CHECK_MODULES([PYGTK], [
		pygtk-2.0 >= $PYGTK_REQUIRED
		gnome-python-desktop-2.0 >= $GNOME_PYTHON_DESKTOP_REQUIRED],
		[],[have_python=no])

	AC_SUBST([PYGTK_CFLAGS])
	AC_SUBST([PYGTK_LIBS])
fi

if test "x$have_python" != "xno"
then
	AC_MSG_CHECKING([for pygtk defs])
	PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
	AC_MSG_RESULT([$PYGTK_DEFSDIR])

	AC_MSG_CHECKING([for pygtk codegen])
	PYGTK_CODEGEN="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/codegen.py"
	AC_MSG_RESULT([$PYGTK_CODEGEN])

	AC_MSG_CHECKING([for pygtk h2def])
	PYGTK_H2DEF="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/h2def.py"
	AC_MSG_RESULT([$PYGTK_H2DEF])

	AC_SUBST([PYGTK_DEFSDIR])
	AC_SUBST([PYGTK_CODEGEN])
	AC_SUBST([PYGTK_H2DEF])

	dnl Check for -fno-strict-aliasing
	FLAGS="-fno-strict-aliasing"
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $FLAGS"
	AC_MSG_CHECKING([whether [$]CC understands $FLAGS])
	AC_TRY_COMPILE([], [], [compiler_has_option=yes], [compiler_has_option=no])
	CFLAGS="$save_CFLAGS"
	AC_MSG_RESULT($compiler_has_option)
	if test $compiler_has_option = yes
	then
		NO_STRICT_ALIASING_CFLAGS="$FLAGS"
	fi
	AC_SUBST([NO_STRICT_ALIASING_CFLAGS])
fi

if test "x$have_python" != "xyes"
then
	if test "x$enable_python" = "xyes"
	then
		AC_MSG_ERROR([Python not found])
	elif test "x$enable_python" = "xautodetect"
	then
		enable_python=no
		AC_MSG_WARN([Python not found, disabling python support])
	fi
elif test "x$enable_python" != "xno"
then
	enable_python=yes
	AC_DEFINE([ENABLE_PYTHON],[1],[Define to compile with python support])
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

# ================================================================
# Misc
# ================================================================
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
			      
GNOME_COMPILE_WARNINGS(yes)

AC_ARG_ENABLE(deprecations,
              [AC_HELP_STRING([--enable-deprecations],
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

PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

AC_CONFIG_FILES([
Makefile
plugins/Makefile
plugins/joinlines/Makefile
plugins/showtabbar/Makefile
plugins/terminal/Makefile
po/Makefile.in])

AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	Python Plugins Support: $enable_python
	Plugins:               $PLUGINS

"
