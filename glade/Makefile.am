
if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

INCLUDES = $(THE_FLAGS) $(XML_CFLAGS)

lib_LTLIBRARIES = libglade.la
libglade_la_SOURCES = \
	glade-init.c \
	glade-xml.c \
	glade-gtk.c \
	glade-tree.c \
	glade-styles.c \
	glade-keys.c \
	glade-private.h

libglade_la_LIBADD = $(THE_LIBS) $(XML_LIBS)

gladeincludedir = $(includedir)/glade

gladeinclude_HEADERS = \
	glade.h \
	glade-xml.h \
	glade-build.h

BUILT_SOURCES = glade-keys.c
EXTRA_DIST = makekeys.awk

glade-keys.c: makekeys.awk $(GDK_KEYSYMS)
	$(AWK) -f $(srcdir)/makekeys.awk < $(GDK_KEYSYMS) > $(srcdir)/glade-keys.c