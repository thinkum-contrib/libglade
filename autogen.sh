#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="libGlade"

(test -f $srcdir/configure.in \
  && test -f $srcdir/glade/glade-gtk.c \
  && test -d $srcdir/glade) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level libglade directory"
    exit 1
}

. $srcdir/macros/autogen.sh
