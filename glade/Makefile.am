
# Before making a release, the LTVERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LTVERSION = 0:0:0

INCLUDES = -I$(top_srcdir) $(XML_CFLAGS) $(GTK_CFLAGS)

lib_LTLIBRARIES = \
	libglade-2.0.la

gladeincludedir      = $(includedir)/glade-2.0/glade
gladeinclude_HEADERS = \
  glade.h 	\
  glade-xml.h 	\
  glade-build.h	\
  glade-parser.h

libglade_2_0_la_SOURCES  = \
  glade-init.c 	 \
  glade-xml.c 	 \
  glade-parser.c \
  glade-gtk.c 	 \
  glade-private.h \
  $(gladeinclude_HEADERS)

libglade_2_0_la_LIBADD   = $(GTK_LIBS) $(XML_LIBS)
libglade_2_0_la_LDFLAGS  = -version-info $(LTVERSION)

EXTRA_DIST = $(gladeinclude_HEADERS) \
	makefile.mingw	\
	glade.def
