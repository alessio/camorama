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

$srcdir/configure `/bin/grep ^DISTCHECK configure.ac | sed 's/^DISTCHECK_CONFIGURE_FLAGS="\(.*\)"$/\1/'` $@
