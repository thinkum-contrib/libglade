
SUBDIRS = glade

#CFLAGS += -Wall -Wpointer-arith -Wstrict-prototypes

INCLUDES = $(GTK_CFLAGS) $(XML_CFLAGS)

EXTRA_DIST = libglade.spec test.glade simple.glade example.glade

noinst_PROGRAMS = test-libglade

test_libglade_SOURCES = test-libglade.c
test_libglade_LDADD = glade/libglade.la $(GTK_LIBS) $(XML_LIBS)

