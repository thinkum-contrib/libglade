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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* this file contains functions that handle conversion of styles in the XML
 * tree to their equivalent RC styles, which are then parsed by
 * gtk_rc_parse_string() */

#include "glade-private.h"
#include <string.h>
#include <gtk/gtkrc.h>

static void fill_style(xmlNodePtr style_node, char *style_name) {
  GString *str;
  char buf[512];
  int r, g, b;
  xmlNodePtr node;

  str = g_string_new("style \"GLADE_");
  g_string_append(str, style_name);
  g_string_append(str, "_style\"\n{\n");
  for (node = style_node->childs; node != NULL; node = node->next) {
    char *content = xmlNodeGetContent(node);
    if (!strcmp(node->name, "style_font")) {
      g_string_append(str, "  font = \"");
      g_string_append(str, content);
      g_string_append(str, "\"\n");
    } else if (!strncmp(node->name, "fg-", 3)) {
      g_string_append(str, "  fg[");
      g_string_append(str, node->name + 3);
      sscanf(content, "%d,%d,%d",&r, &g, &b);
      g_snprintf(buf, 511, "] = { %.3f, %3f, %3f }\n",
		 CLAMP(r, 0, 255)/255.0,
		 CLAMP(g, 0, 255)/255.0,
		 CLAMP(b, 0, 255)/255.0);
      g_string_append(str, buf);
    } else if (!strncmp(node->name, "bg-", 3)) {
      g_string_append(str, "  bg[");
      g_string_append(str, node->name + 3);
      sscanf(content, "%d,%d,%d",&r, &g, &b);
      g_snprintf(buf, 511, "] = { %.3f, %3f, %3f }\n",
		 CLAMP(r, 0, 255)/255.0,
		 CLAMP(g, 0, 255)/255.0,
		 CLAMP(b, 0, 255)/255.0);
      g_string_append(str, buf);
    } else if (!strncmp(node->name, "text-", 5)) {
      g_string_append(str, "  text[");
      g_string_append(str, node->name + 5);
      sscanf(content, "%d,%d,%d",&r, &g, &b);
      g_snprintf(buf, 511, "] = { %.3f, %3f, %3f }\n",
		 CLAMP(r, 0, 255)/255.0,
		 CLAMP(g, 0, 255)/255.0,
		 CLAMP(b, 0, 255)/255.0);
      g_string_append(str, buf);
    } else if (!strncmp(node->name, "base-", 5)) {
      g_string_append(str, "  base[");
      g_string_append(str, node->name + 5);
      sscanf(content, "%d,%d,%d",&r, &g, &b);
      g_snprintf(buf, 511, "] = { %.3f, %3f, %3f }\n",
		 CLAMP(r, 0, 255)/255.0,
		 CLAMP(g, 0, 255)/255.0,
		 CLAMP(b, 0, 255)/255.0);
      g_string_append(str, buf);
    } else if (!strncmp(node->name, "bg_pixmap-", 10)) {
      g_string_append(str, "  bg_pixmap[");
      g_string_append(str, node->name + 10);
      g_string_append(str, "] = \"");
      g_string_append(str, content);
      g_string_append(str, "\"\n");
    }
    if (content) free(content);
  }
  g_string_append(str, "}\n");
  gtk_rc_parse_string(str->str);
  g_string_free(str, TRUE);
  if (!strcmp(style_name, "Default"))
    gtk_rc_parse_string("widget \"*\" style \"GLADE_Default_style\"\n");
}

void glade_style_parse(xmlDocPtr tree) {
  xmlNodePtr node;
  char *style_name;
  for (node = tree->root->childs; node != NULL; node = node->next) {
    if (strcmp(node->name, "style")) continue;
    style_name = xmlNodeGetContent(glade_tree_find_node(node, "style_name"));
    fill_style(node, style_name);
    free(style_name);
  }
}

void glade_style_attach(GtkWidget *widget, const char *style) {
  char *name = glade_get_widget_long_name(widget), buf[512];
  g_snprintf(buf, 511, "widget \"*%s\" style \"GLADE_%s_style\"\n",name,style);
  gtk_rc_parse_string(buf);
}
