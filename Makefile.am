SUBDIRS = intl po glade

INCLUDES = $(GTK_CFLAGS) $(XML_CFLAGS)

bin_SCRIPTS = libglade-xgettext libglade-convert

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libglade-2.0.pc

noinst_PROGRAMS = test-libglade

test_libglade_LDADD = glade/libglade-2.0.la $(GTK_LIBS) $(XML_LIBS)
test_libglade_SOURCES = test-libglade.c

EXTRA_DIST = \
  libglade.spec.in \
  libglade.spec \
  libglade.pc.in \
  libglade-xgettext \
  libglade-convert \
  examples/simple.glade \
  examples/first.glade \
  examples/example.glade \
  config.h.win32 \
  makefile.mingw

