#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

REQUIRED_AUTOMAKE_VERSION=1.9

(test -f $srcdir/configure.ac \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
  echo "You need to install the gnome-common module and make"
  echo "sure the gnome-autogen.sh script is in your \$PATH."
  exit 1
}

. gnome-autogen.sh

GETTEXT_VER=$(autopoint --version|perl -ne 'print $1 if (m/gettext-tools\D*([\d\.]+)/)')
CUR_VER=$(cat po/Makefile.in.in|perl -ne 'print "$1\n" if (m/gettext-\D*(\d[\d\.]+)/)')

if [ "$GETTEXT_VER" != "$CUR_VER" ]; then
	autopoint --force
fi

$srcdir/configure `/bin/grep ^DISTCHECK configure.ac | sed 's/^DISTCHECK_CONFIGURE_FLAGS="\(.*\)"$/\1/'` $@
