noinst_PROGRAMS = \
	test-libglade-gtk	\
	test-libglade-gtk-noupgrade

INCLUDES = \
	-I$(top_srcdir)		\
	$(LIBGLADE_CFLAGS)

test-libglade-gtk.glade2: test-libglade-gtk.glade $(top_srcdir)/libglade-convert
	$(PYTHON) $(top_builddir)/libglade-convert \
		$(srcdir)/test-libglade-gtk.glade \
		> test-libglade-gtk.glade2.tmp \
	&& mv test-libglade-gtk.glade2.tmp \
		test-libglade-gtk.glade2

test-libglade-gtk-noupgrade.glade2: test-libglade-gtk.glade $(top_srcdir)/libglade-convert
	$(PYTHON) $(top_builddir)/libglade-convert --no-upgrade \
		$(srcdir)/test-libglade-gtk.glade \
		> test-libglade-gtk-noupgrade.glade2.tmp \
	&& mv test-libglade-gtk-noupgrade.glade2.tmp \
		test-libglade-gtk-noupgrade.glade2

LDADD = $(top_builddir)/glade/libglade-2.0.la

test_libglade_gtk_SOURCE = test-libglade-gtk.c

$(test_libglade_gtk_OBJECTS): test-libglade-gtk.glade2

test_libglade_gtk_noupgrade_SOURCE = test-libglade-gtk-noupgrade.c

$(test_libglade_gtk_noupgrade_OBJECTS): test-libglade-gtk-noupgrade.glade2

TESTS = test-libglade-gtk test-libglade-gtk-noupgrade

EXTRA_DIST = test-libglade-gtk.glade

distclean-local:
	rm -f test-libglade-gtk.glade2 test-libglade-gtk-noupgrade.glade2
