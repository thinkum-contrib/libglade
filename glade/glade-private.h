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
#ifndef GLADE_PRIVATE_H
#define GLADE_PRIVATE_H
#include <stdio.h>
#include <tree.h>
#include <glib.h>
#include <gtk/gtkwidget.h>
#include "glade-xml.h"

/* from glade-tree.c */
typedef struct _GladeTreeData GladeTreeData;
struct _GladeTreeData {
  xmlDocPtr xml;
  GNode *tree;      /* a tree of the <widget> tags */
  GHashTable *hash; /* a hash of the GNodes */
};
/* parse an XML document, evaluating any styles found.  Uses a cached copy
 * of the xmlDocPtr structure if this file has been parsed previously.
 * It also extracts a tree of all the <widget> tags to make it easier to
 * build interfaces */
GladeTreeData *glade_tree_get(const char *filename);
/* may return NULL if no matching nodes are found */
xmlNodePtr glade_tree_find_node(xmlNodePtr parent, const char *childname);

/* from glade-styles.c */
/* evaluate the styles in the given XML tree */
void glade_style_parse(xmlDocPtr tree);
/* get a named style from a previously parsed XML file */
void glade_style_attach(GtkWidget *widget, const char *style);

/* from glade-xml.c */
/* get the widget name */
const char *glade_get_widget_name(GtkWidget *widget);
/* get the name of the widget (dot separated heirachy) */
const char *glade_get_widget_long_name(GtkWidget *widget);

/* from glade-keys.c */
/* perform string->int keysym conversion */
guint glade_key_get(char *str);

#endif

