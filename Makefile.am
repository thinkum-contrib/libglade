SUBDIRS = glade doc tests

INCLUDES = $(LIBGLADE_CFLAGS)

if HAVE_PYTHON
bin_SCRIPTS = libglade-convert
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libglade-2.0.pc

noinst_PROGRAMS = test-libglade

test_libglade_LDADD = glade/libglade-2.0.la $(LIBGLADE_LIBS)
test_libglade_SOURCES = test-libglade.c

#small hack to get distcheck to work
clean-local:
	rm -f intl/po2tbl.sed

EXTRA_DIST = \
  glade-2.0.dtd \
  libglade.spec.in \
  libglade.spec \
  libglade-2.0.pc.in \
  libglade-convert.in \
  examples/simple.glade \
  examples/first.glade \
  examples/example.glade \
  examples/bonobo.glade \
  config.h.win32 \
  makefile.mingw

TESTS_ENVIRONMENT = \
	LIBGLADE_REGRESSION=1
TESTS = test-libglade
