/* -*- Mode: C; c-basic-offset: 8 -*- */
/* libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998, 1999, 2000  James Henstridge <james@daa.com.au>
 *
 * glade-xml.h: public interface of libglade library
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

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */
	
#define GLADE_XML(obj) GTK_CHECK_CAST((obj), glade_xml_get_type(), GladeXML)
#define GLADE_XML_CLASS(klass) GTK_CHECK_CLASS_CAST((klass), glade_xml_get_type(), GladeXMLClass)
#define GLADE_IS_XML(obj) GTK_CHECK_TYPE((obj), glade_xml_get_type())

typedef struct _GladeXML GladeXML;
typedef struct _GladeXMLClass GladeXMLClass;
typedef struct _GladeXMLPrivate GladeXMLPrivate;

struct _GladeXML {
  /* <public> */
  GtkData parent;

  char *filename;
  char *textdomain;

  /* <private> */
  GladeXMLPrivate *priv;
};

struct _GladeXMLClass {
  GtkDataClass parent_class;
};

GtkType glade_xml_get_type    (void);
GladeXML *glade_xml_new       (const char *fname, const char *root);
GladeXML *glade_xml_new_with_domain (const char *fname, const char *root,
				     const char *domain);
GladeXML *glade_xml_new_from_memory (char *buffer, int size, const char *root,
				     const char *domain);
gboolean glade_xml_construct  (GladeXML *self, const char *fname,
			       const char *root, const char *domain);

void glade_xml_signal_connect (GladeXML *self, const char *handlername,
			       GtkSignalFunc func);
void glade_xml_signal_connect_data (GladeXML *self, const char *handlername,
				    GtkSignalFunc func, gpointer user_data);
/*
 * use gmodule to connect signals automatically.  Basically a symbol with
 * the name of the signal handler is searched for, and that is connected to
 * the associated symbols.  So setting gtk_main_quit as a signal handler
 * for the destroy signal of a window will do what you expect.
 */
void       glade_xml_signal_autoconnect      (GladeXML *self);

/* if the gtk_signal_connect_object behaviour is required, connect_object
 * will point to the object, otherwise it will be NULL.
 */
typedef void (*GladeXMLConnectFunc)          (const gchar *handler_name,
					      GtkObject *object,
					      const gchar *signal_name,
					      const gchar *signal_data,
					      GtkObject *connect_object,
					      gboolean after,
					      gpointer user_data);

/*
 * These two are to make it easier to use libglade with an interpreted
 * language binding.
 */
void       glade_xml_signal_connect_full     (GladeXML *self,
					      const gchar *handler_name,
					      GladeXMLConnectFunc func,
					      gpointer user_data);

void       glade_xml_signal_autoconnect_full (GladeXML *self,
					      GladeXMLConnectFunc func,
					      gpointer user_data);


GtkWidget *glade_xml_get_widget              (GladeXML *self,
					      const char *name);
GList     *glade_xml_get_widget_prefix       (GladeXML *self,
					      const char *name);
GtkWidget *glade_xml_get_widget_by_long_name (GladeXML *self,
					      const char *longname);

gchar     *glade_xml_relative_file           (GladeXML *self,
					      const gchar *filename);

/* don't free the results of these two ... */
const char *glade_get_widget_name      (GtkWidget *widget);
const char *glade_get_widget_long_name (GtkWidget *widget);

GladeXML   *glade_get_widget_tree      (GtkWidget *widget);

#ifdef __cplusplus
}
#endif /* __cplusplus */
	
#endif
