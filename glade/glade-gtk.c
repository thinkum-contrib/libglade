/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-gtk.c: support for GTK+ widgets in libglade
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

#include <string.h>
#include <glade/glade.h>
#include <glade/glade-build.h>
#include <glade/glade-private.h>
#include <gtk/gtk.h>

void _glade_init_gtk_widgets(void);

static GtkWidget *
window_new(GladeXML *xml, GType widget_type, GladeWidgetInfo *info)
{
    GtkWidget *window = glade_standard_build_widget(xml, widget_type, info);

    glade_xml_set_toplevel(xml, GTK_WINDOW(window));
    return window;
}

static GladeWidgetBuildData widget_data[] = {
    { "GtkWindow", window_new, glade_standard_build_children,
      gtk_window_get_type },
    { "GtkButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_button_get_type },
    { "GtkLabel", glade_standard_build_widget, NULL,
      gtk_label_get_type },
    { NULL, NULL, NULL, 0, 0 }
};

void
_glade_init_gtk_widgets(void)
{
    glade_provide("gtk");
    glade_register_widgets(widget_data);
}
