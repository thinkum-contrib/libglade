#include <config.h>
#include <string.h>
#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#include <glade/glade.h>

char *filename = NULL, *rootnode = NULL;
gboolean no_connect = FALSE;

#ifdef ENABLE_GNOME
static poptContext ctx;

static const struct poptOption options [] = {
        { "no-connect", '\0', POPT_ARG_INT, &no_connect, 0,
          N_("Do not connect signals") },
        { NULL, '\0', 0, NULL, 0 }
};
#endif

int main (int argc, char **argv)
{
  int i;
  GladeXML *xml;

#ifdef ENABLE_GNOME
  char **list = NULL;
  
  gnome_init_with_popt_table ("test-libglade", VERSION, argc, argv, options, 0, &ctx);
  glade_gnome_init();

  list = poptGetArgs (ctx);
  if (list){
	  int i;

	  for (i = 0; list [i]; i++){
		  if (filename == NULL)
			  filename = list [i];
		  else if (rootnode == NULL)
			  rootnode = list [i];
	  }
  }
  if (filename == NULL){
      g_print("Usage: %s [--no-connect] filename [rootnode]\n", argv[0]);
      return 1;
  }
#else
  gtk_init(&argc, &argv);
  glade_init();

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--no-connect"))
      no_connect = TRUE;
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
#endif
  
  xml = glade_xml_new(filename, rootnode);

  if (!no_connect)
    glade_xml_signal_autoconnect(xml);
  gtk_main();
  return 0;
}
