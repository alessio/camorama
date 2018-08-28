AC_PREREQ(2.53)
AC_INIT(camorama,0.19, https://github.com/alessio/camorama)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_DEFINE(PACKAGE, AC_PACKAGE_NAME, [package name])
AC_DEFINE(VERSION, AC_PACKAGE_VERSION, [package version])

AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

# This is needed for gtk2+ to work with gettext
DATADIRNAME=share
AC_SUBST(DATADIRNAME)

AC_MSG_CHECKING(prefix)

if test "x${prefix}" = "xNONE"; then
  PACKAGE_PREFIX="${ac_default_prefix}"
else
  PACKAGE_PREFIX="${prefix}"
fi
AC_MSG_RESULT($PACKAGE_PREFIX)

AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${PACKAGE_PREFIX}/${DATADIRNAME}", "the data dir")
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${PACKAGE_PREFIX}/${DATADIRNAME}/locale", "the locale dir")

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h unistd.h linux/videodev.h png.h glade/glade.h])

GNOME_COMPILE_WARNINGS

pkg_modules="gdk-pixbuf-2.0 gdk-pixbuf-xlib-2.0 libgnomeui-2.0 gtk+-2.0 >= 2.10 gconf-2.0 libglade-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
AC_DEFINE(HAVE_GCONF,1,[Build with gconf support])

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2


AC_OUTPUT([
	Makefile
	src/Makefile
	data/Makefile
	data/camorama.schemas
	pixmaps/Makefile
	po/Makefile.in
])

