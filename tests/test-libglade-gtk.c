
#include <glib/gmessages.h>
#include <glib-object.h>
#include <gtk/gtkmain.h>
#include <glade/glade-xml.h>

int
main (int argc, char *argv[])
{
	GladeXML *xml;

	gtk_init (&argc, &argv);

	xml = glade_xml_new ("test-libglade-gtk.glade2", NULL, NULL);

	g_assert (xml != NULL);

	g_object_unref (G_OBJECT (xml));

	return 0;
}
