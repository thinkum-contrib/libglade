#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=Libglade
TEST_TYPE=-d
FILE=glade

DIE=0

have_libtool=false
if libtool --version < /dev/null > /dev/null 2>&1 ; then
	libtool_version=`libtoolize --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
	case $libtool_version in
	    1.[456789]*)
		have_libtool=true
		;;
	esac
fi
if $have_libtool ; then : ; else
	echo
	echo "You must have libtool 1.4 installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
fi

have_autoconf=false
if autoconf --version < /dev/null > /dev/null 2>&1 ; then
	autoconf_version=`autoconf --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
	case $autoconf_version in
	    2.5*)
		have_autoconf=true
		;;
	esac
fi
if $have_autoconf ; then : ; else
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "libtool the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
fi

if automake-1.8 --version < /dev/null > /dev/null 2>&1; then
  AUTOMAKE=automake-1.8
  ACLOCAL=aclocal-1.8
elif automake-1.7 --version < /dev/null > /dev/null 2>&1; then
  AUTOMAKE=automake-1.7
  ACLOCAL=aclocal-1.7
else
	echo
	echo "You must have automake >= 1.7 installed to compile $PROJECT."
	echo "Get http://ftp.gnu.org/gnu/automake/automake-1.8.4.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
fi

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        if test -z "$*"; then
                echo "I am going to run ./configure with no arguments - if you wish "
                echo "to pass any to it, please specify them on the $0 command line."
        fi
fi

if test -z "$ACLOCAL_FLAGS"; then

	acdir=`$ACLOCAL --print-ac-dir`
        m4list="glib-2.0.m4 glib-gettext.m4 gtk-2.0.m4"

	for file in $m4list
	do
		if [ ! -f "$acdir/$file" ]; then
			echo "WARNING: aclocal's directory is $acdir, but..."
			echo "         no file $acdir/$file"
			echo "         You may see fatal macro warnings below."
			echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
			echo "         environment variable to \"-I /some/dir\", or install"
			echo "         $acdir/$file."
			echo ""
		fi
	done
fi

libtoolize --force || exit 1
gtkdocize || exit 1

$ACLOCAL $ACLOCAL_FLAGS || exit 1

autoconf || exit 1

# optionally feature autoheader
autoheader || exit 1
test -f config.h.in && touch config.h.in

$AUTOMAKE --add-missing || exit 1

cd $ORIGDIR

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        $srcdir/configure --enable-maintainer-mode --enable-debug --enable-gtk-doc "$@" || exit 1

        echo 
        echo "Now type 'make' to compile $PROJECT."
fi
