AC_PREREQ(2.53)
AC_INIT(camorama,0.20.7, https://github.com/alessio/camorama)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE([foreign])
AM_CONFIG_HEADER(config.h)

AC_DEFINE(PACKAGE, AC_PACKAGE_NAME, [package name])
AC_DEFINE(VERSION, AC_PACKAGE_VERSION, [package version])

AM_GNU_GETTEXT([external])
m4_ifdef(AM_GNU_GETTEXT_REQUIRE_VERSION,[AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.8])],[AM_GNU_GETTEXT_VERSION([0.19.8])])

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
AC_HEADER_MAJOR
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h unistd.h linux/videodev.h png.h glade/glade.h])

# Check for V4L2
AC_CHECK_LIB(v4l2, v4l2_open, LIBV4L="yes",,)
if test "$LIBV4L" != "yes"; then
    echo "Oops:	libv4l library not found."
    exit 1
fi

# Don't assume that gcc warnings are errors
AX_IS_RELEASE(always)
AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])

AC_ARG_ENABLE(gtk2, [AC_HELP_STRING([--enable-gtk2], [Compile against GTK 2.x instead of GTK 3.x])], [enable_gtk2=$enableval], [enable_gtk2="no"])
AC_ARG_ENABLE(gtk4, [AC_HELP_STRING([--enable-gtk4], [Compile against GTK 4.x])], [enable_gtk4=$enableval], [enable_gtk4="no"])
GLIB_GSETTINGS

pkg_modules="gdk-pixbuf-2.0"

if test "x$enable_gtk4" = xyes; then
	pkg_modules="$pkg_modules gtk+-4.0 >= 3.92"
	CAMORAMA_UI="camorama-gtk4.ui"
	GTK_VER="4"
elif test "x$enable_gtk2" = xyes; then
	pkg_modules="$pkg_modules gtk+-2.0 >= 2.24"
	CAMORAMA_UI="camorama-gtk2.ui"
	GTK_VER="2"
else
	pkg_modules="$pkg_modules gtk+-3.0 >= 3.10 cairo"
	CAMORAMA_UI="camorama-gtk3.ui"
	GTK_VER="3"
fi

PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
AC_SUBST(CAMORAMA_UI)
AC_DEFINE(HAVE_GCONF,1,[Build with gconf support])

AC_OUTPUT([
	Makefile
	src/Makefile
	data/Makefile
	po/Makefile.in
])

cat <<EOF

compile time options summary
============================

    Gtk version to use: $GTK_VER
EOF
