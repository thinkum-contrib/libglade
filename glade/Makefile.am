
# Before making a release, the LTVERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LTVERSION = 0:0:0

INCLUDES = -DG_LOG_DOMAIN=\"libglade\" -I$(top_srcdir) $(XML_CFLAGS) $(GTK_CFLAGS) -DGLADE_MODULE_DIR=\""$(glademoduledir)"\" $(LIBGLADE_BONOBO_CFLAGS)

glademoduledir = $(pkglibdir)/2.0

if BUILD_BONOBO
bonobo_la = bonobo.la
endif

lib_LTLIBRARIES = libglade-2.0.la
glademodule_LTLIBRARIES = $(bonobo_la)

gladeincludedir      = $(includedir)/libglade-2.0/glade
gladeinclude_HEADERS = \
  glade.h 	\
  glade-init.h	\
  glade-xml.h	\
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

bonobo_la_SOURCES = glade-bonobo.c
bonobo_la_LIBADD = libglade-2.0.la $(LIBGLADE_BONOBO_LIBS)
bonobo_la_LDFLAGS = -export-dynamic -module -avoid-version

EXTRA_DIST = $(gladeinclude_HEADERS) \
	makefile.mingw	\
	glade.def
