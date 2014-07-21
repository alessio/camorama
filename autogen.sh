#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="camorama"
REQUIRED_LIBTOOL_VERSION=1.4.3
REQUIRED_AUTOMAKE_VERSION=1.7
#ACLOCAL_FLAGS="-I macros $ACLOCAL_FLAGS"

(test -f $srcdir/configure.in \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

ifs_save="$IFS"; IFS=":"
for dir in $PATH ; do
  test -z "$dir" && dir=.
  if test -f $dir/gnome-autogen.sh ; then
    gnome_autogen="$dir/gnome-autogen.sh"
    gnome_datadir=`echo $dir | sed -e 's,/bin$,/share,'`
    break
  fi
done
IFS="$ifs_save"

if test -z "$gnome_autogen" ; then
  echo "You need to install the gnome-common module and make"
  echo "sure the gnome-autogen.sh script is in your \$PATH."
  exit 1
fi

NOCONFIGURE="yes" GNOME_DATADIR="$gnome_datadir" USE_GNOME2_MACROS=1 . $gnome_autogen

$srcdir/configure --enable-maintainer-mode `/bin/grep ^DISTCHECK configure.in | sed 's/^DISTCHECK_CONFIGURE_FLAGS="\(.*\)"$/\1/'` $@
