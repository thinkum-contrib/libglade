SUBDIRS = intl po glade doc

INCLUDES = $(GTK_CFLAGS) $(XML_CFLAGS)

bin_SCRIPTS = libglade-xgettext libglade-convert

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libglade-2.0.pc

noinst_PROGRAMS = test-libglade

test_libglade_LDADD = glade/libglade-2.0.la $(GTK_LIBS) $(XML_LIBS)
test_libglade_SOURCES = test-libglade.c

#small hack to get distcheck to work
clean-local:
	rm -f intl/po2tbl.sed

EXTRA_DIST = \
  glade-2.0.dtd \
  libglade.spec.in \
  libglade.spec \
  libglade-2.0.pc.in \
  libglade-xgettext \
  libglade-convert \
  examples/simple.glade \
  examples/first.glade \
  examples/example.glade \
  config.h.win32 \
  makefile.mingw

