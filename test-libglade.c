#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

int main(int argc, char **argv) {
  int i;
  GladeXML *xml;
  char *filename = NULL, *rootnode = NULL;
  gboolean autoconnect = TRUE;

  gtk_init(&argc, &argv);
  glade_init();

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--no-connect"))
      autoconnect = FALSE;
    else if (filename == NULL)
      filename = argv[i];
    else if (rootnode == NULL)
      rootnode = argv[i];
    else {
      g_print("Usage: %s [--no-connect] filename [rootnode]\n", argv[0]);
      return 1;
    }
  }
  if (filename == NULL) {
    g_print("Usage: %s [--no-connect] filename [rootnode]\n", argv[0]);
    return 1;
  }

  xml = glade_xml_new(filename, rootnode);

  if (autoconnect)
    glade_xml_signal_autoconnect(xml);
  gtk_main();
  return 0;
}
