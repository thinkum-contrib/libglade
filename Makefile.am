INCLUDES = $(GTK_CFLAGS) $(XML_CFLAGS)

EXTRA_DIST =			\
	libgladeConf.sh		\
	libglade.spec.in	\
	libglade.spec		\
	test.glade		\
	simple.glade		\
	example.glade		\
	custom.glade		\
	gnome-widgets.glade	\
	dialogs.glade		\
	libglade.m4		\
	libglade-xgettext	\
	libglade.pc.in		\
	libglade-gnome.pc.in	\
	libglade-bonobo.pc.in	\
	config.h.win32		\
	makefile.mingw

bin_SCRIPTS = libglade-xgettext
noinst_PROGRAMS = test-libglade
test_libglade_LDADD = glade/libglade-2.0.la $(GTK_LIBS) $(XML_LIBS)
SUBDIRS = intl po macros glade

#m4datadir = $(datadir)/aclocal
#m4data_DATA = libglade-2.0.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libglade-2.0.pc

test_libglade_SOURCES = test-libglade.c
