if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = glade/libglade-gnome.la glade/libglade.la $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = glade/libglade.la $(GTK_LIBS)
endif

INCLUDES = $(THE_FLAGS) $(XML_CFLAGS)

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
	config.h.win32		\
	makefile.mingw

bin_SCRIPTS = libglade-config libglade-xgettext
noinst_PROGRAMS = test-libglade
test_libglade_LDADD = $(THE_LIBS) $(XML_LIBS)
SUBDIRS = intl po macros glade doc

m4datadir = $(datadir)/aclocal
m4data_DATA = libglade.m4

confexecdir = $(libdir)
confexec_DATA = libgladeConf.sh

test_libglade_SOURCES = test-libglade.c
