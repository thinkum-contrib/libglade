if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = glade/libglade-gnome.la glade/libglade.la $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = glade/libglade.la $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

INCLUDES = $(THE_FLAGS) $(XML_CFLAGS)

EXTRA_DIST =			\
	libglade.spec.in	\
	libgladeConf.sh		\
	test.glade		\
	simple.glade		\
	example.glade		\
	custom.glade		\
	gnome-widgets.glade	\
	libglade.m4

if LIBGLADE_FULLDIST
bin_SCRIPTS = libglade-config
noinst_PROGRAMS = test-libglade
test_libglade_LDADD = $(THE_LIBS) $(XML_LIBS)
extra_dirs = macros doc
m4datadir = $(datadir)/aclocal
m4data_DATA = libglade.m4
confexecdir = $(libdir)
confexec_DATA = libgladeConf.sh
else
extra_dirs =
endif

SUBDIRS = glade $(extra_dirs)

test_libglade_SOURCES = test-libglade.c

dist-hook:
	cp libglade.spec $(distdir)
