all: domp saxp

domp: domp.c
	$(CC) -o domp domp.c `glib-config --cflags --libs` `xml-config --cflags --libs`
saxp: saxp.c ../glade/glade-keys.c
	$(CC) -o saxp saxp.c ../glade/glade-keys.c `glib-config --cflags --libs` `xml-config --cflags --libs`
