
if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
else
THE_FLAGS = $(GTK_CFLAGS)
endif

INCLUDES = -I$(srcdir)/.. $(THE_FLAGS) $(XML_CFLAGS) -I$(srcdir)/..

the_sources =		\
	glade-init.c 	\
	glade-sax.c	\
	glade-xml.c 	\
	glade-gtk.c 	\
	glade-tree.c 	\
	glade-keys.c 	\
	glade-private.h

the_headers =		\
	glade.h 	\
	glade-xml.h 	\
	glade-build.h	\
	glade-widget-tree.h

if LIBGLADE_FULLDIST

lib_LTLIBRARIES      = libglade.la $(build_glade_gnome)
if GNOME_SUPPORT
build_glade_gnome = libglade-gnome.la
libglade_gnome_la_SOURCES  = glade-gnome.c glade-build.h glade-xml.h
libglade_gnome_la_LIBADD   = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(XML_LIBS)
endif
libglade_la_SOURCES  = $(the_sources)
libglade_la_LIBADD   = $(GTK_LIBS) $(XML_LIBS)
gladeincludedir      = $(includedir)/glade
gladeinclude_HEADERS = $(the_headers)

else

lib_LIBRARIES        = libglade.a
if GNOME_SUPPORT
libglade_a_SOURCES   = $(the_sources) glade-gnome.c
else
libglade_a_SOURCES   = $(the_sources)
endif
extra_dist           = $(the_headers)

endif

BUILT_SOURCES = glade-keys.c
EXTRA_DIST = makekeys.awk $(the_headers)

$(srcdir)/glade-keys.c: makekeys.awk $(GDK_KEYSYMS)
	$(AWK) -f $(srcdir)/makekeys.awk < $(GDK_KEYSYMS) > $(srcdir)/glade-keys.c
