if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

SUBDIRS = glade

#CFLAGS += -Wall -Wpointer-arith -Wstrict-prototypes

INCLUDES = $(THE_FLAGS) $(XML_CFLAGS)

EXTRA_DIST = libglade.spec test.glade simple.glade example.glade

bin_SCRIPTS = libglade-config

noinst_PROGRAMS = test-libglade

test_libglade_SOURCES = test-libglade.c
test_libglade_LDADD = glade/libglade.la $(THE_LIBS) $(XML_LIBS)

