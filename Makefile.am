SUBDIRS = plugins po

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =			\
	autogen.sh		\
	ChangeLog  		\
	gnome-doc-utils.make	\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

DISTCLEANFILES = 		\
	gnome-doc-utils.make	\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = --with-plugins=really-all
