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
#ifndef GLADE_BUILD_H
#define GLADE_BUILD_H

#include <glade/glade-xml.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkadjustment.h>

/* create a new widget of some type.  Don't parse `standard' widget options */
typedef GtkWidget *(* GladeNewFunc) (GladeXML *xml,
				     GNode *node);
/* call glade_xml_build_widget on each child node, and pack in self */
typedef void (* GladeBuildChildrenFunc) (GladeXML *xml,
					 GtkWidget *w,
					 GNode *node,
					 const char *longname);
typedef struct {
  char *name;
  GladeNewFunc new;
  GladeBuildChildrenFunc build_children;
} GladeWidgetBuildData;

/* widgets is a static, NULL terminated array of GladeWidgetBuildData's.
 * They will be added to a hash table, with the name as the key, to be
 * used by glade_xml_build_widget */
void glade_register_widgets(const GladeWidgetBuildData *widgets);

/* this function is called to build the interface by GladeXML */
GtkWidget *glade_xml_build_widget(GladeXML *self, GNode *node,
				  const char *parent_long);

/* This function performs half of what glade_xml_build_widget does.  It is
 * useful when the widget has already been created.  Usually it would not
 * have any use at all. */
void       glade_xml_set_common_params(GladeXML *self,
				       GtkWidget *widget,
				       GNode *node,
				       const char *parent_long,
				       const char *widget_class);

/* A standard child building routine that can be used in widget builders */
void glade_standard_build_children(GladeXML *self, GtkWidget *w,
				   GNode *node, const char *longname);

/* create an adjustment object for a widget */
GtkAdjustment *glade_get_adjustment(GNode *gnode);

/* this is a wrapper for gtk_type_enum_find_value, that just returns the
 * integer value for the enum */
gint glade_enum_from_string(GtkType type, const char *string);

/* a wrapper for gettext */
char *glade_xml_gettext(GladeXML *xml, const char *msgid);

#endif
