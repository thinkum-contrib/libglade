
if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

INCLUDES = -I$(srcdir)/.. $(THE_FLAGS) $(XML_CFLAGS) -I$(srcdir)/..

the_sources =		\
	glade-init.c 	\
	glade-xml.c 	\
	glade-gtk.c 	\
	glade-tree.c 	\
	glade-styles.c 	\
	glade-keys.c 	\
	glade-private.h

the_headers =		\
	glade.h 	\
	glade-xml.h 	\
	glade-build.h

if LIBGLADE_FULLDIST
lib_LTLIBRARIES      = libglade.la
libglade_la_SOURCES  = $(the_sources)
libglade_la_LIBADD   = $(THE_LIBS) $(XML_LIBS)
gladeincludedir      = $(includedir)/glade
gladeinclude_HEADERS = $(the_headers)
else
lib_LIBRARIES        = libglade.a
libglade_a_SOURCES   = $(the_sources)
extra_dist           = $(the_headers)
endif

BUILT_SOURCES = glade-keys.c
EXTRA_DIST = makekeys.awk $(the_headers)

glade-keys.c: makekeys.awk $(GDK_KEYSYMS)
	$(AWK) -f $(srcdir)/makekeys.awk < $(GDK_KEYSYMS) > $(srcdir)/glade-keys.c
