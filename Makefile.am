if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

INCLUDES = $(THE_FLAGS) $(XML_CFLAGS)

EXTRA_DIST =			\
	libglade.spec.in	\
	test.glade		\
	simple.glade		\
	example.glade		\
	libglade.m4

if LIBGLADE_FULLDIST
bin_SCRIPTS = libglade-config
noinst_PROGRAMS = test-libglade
test_libglade_LDADD = glade/libglade.la $(THE_LIBS) $(XML_LIBS)
extra_dirs = macros doc
m4datadir = $(datadir)/aclocal
m4data_DATA = libglade.m4
else
extra_dirs =
endif

SUBDIRS = glade $(extra_dirs)

test_libglade_SOURCES = test-libglade.c

dist-hook:
	cp libglade.spec $(distdir)
