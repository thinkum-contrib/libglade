/* libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998  James Henstridge <james@daa.com.au>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#ifndef GLADE_XML_H
#define GLADE_XML_H

#include <glib.h>
#include <gtk/gtkdata.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktooltips.h>

#define GLADE_XML(obj) GTK_CHECK_CAST((obj), glade_xml_get_type(), GladeXML)
#define GLADE_XML_CLASS(klass) GTK_CHECK_CLASS_CAST((klass), glade_xml_get_type(), GladeXMLClass)
#define GLADE_IS_XML(obj) GTK_CHECK_TYPE((obj), glade_xml_get_type())

typedef struct _GladeXML GladeXML;
typedef struct _GladeXMLClass GladeXMLClass;
typedef struct _GladeSignalData GladeSignalData;
typedef struct _GladeXMLPrivate GladeXMLPrivate;

struct _GladeXML {
  /* <public> */
  GtkData parent;

  char *filename;

  /* <private> */
  GladeXMLPrivate *priv;
};

struct _GladeXMLClass {
  GtkDataClass parent_class;
};

struct _GladeSignalData {
  GtkObject *signal_object;
  char *signal_name;
  char *signal_data; /* this isn't actually used, but it is in the XML files */
  char *connect_object; /* or NULL if there is none */
  gboolean signal_after;
};

GtkType glade_xml_get_type    (void);
GladeXML *glade_xml_new       (const char *fname, const char *root);
gboolean glade_xml_construct  (GladeXML *self, const char *fname,
			       const char *root);

void glade_xml_signal_connect (GladeXML *self, const char *handlername,
			       GtkSignalFunc func);
/*
 * use gmodule to connect signals automatically.  Basically a symbol with
 * the name of the signal handler is searched for, and that is connected to
 * the associated symbols.  So setting gtk_main_quit as a signal handler
 * for the destroy signal of a window will do what you expect.
 */
void       glade_xml_signal_autoconnect      (GladeXML *self);

GtkWidget *glade_xml_get_widget              (GladeXML *self,
					      const char *name);
GtkWidget *glade_xml_get_widget_by_long_name (GladeXML *self,
					      const char *longname);

gchar     *glade_xml_relative_file           (GladeXML *self,
					      const gchar *filename);

/* don't free the results of these two ... */
const char *glade_get_widget_name      (GtkWidget *widget);
const char *glade_get_widget_long_name (GtkWidget *widget);

GladeXML   *glade_get_widget_tree      (GtkWidget *widget);


#endif
