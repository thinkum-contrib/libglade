
INCLUDES = $(GTK_CFLAGS) $(XML_CFLAGS)

#CFLAGS += -Wall -Wpointer-arith -Wstrict-prototypes

lib_LTLIBRARIES = libglade.la
libglade_la_SOURCES = \
	glade-init.c \
	glade-xml.c \
	glade-gtk.c \
	glade-tree.c \
	glade-styles.c \
	glade-keys.c \
	glade-private.h
libglade_la_LIBADD = $(GTK_LIBS) $(XML_LIBS)

gladeincludedir = $(includedir)/glade

gladeinclude_HEADERS = \
	glade.h \
	glade-xml.h \
	glade-build.h

BUILT_SOURCES = glade-keys.c
EXTRA_DIST = makekeys.awk

glade-keys.c: makekeys.awk $(GDK_KEYSYMS)
	$(AWK) -f $(srcdir)/makekeys.awk < $(GDK_KEYSYMS) > $(srcdir)/glade-keys.c