/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-private.h: private datastructures for the GladeXML object.
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
#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkaccelgroup.h>
#include <glade/glade-xml.h>
#include <glade/glade-parser.h>

struct _GladeXMLPrivate {
    GladeInterface *tree; /* the tree for this GladeXML */

    GtkTooltips *tooltips; /* if not NULL, holds all tooltip info */

    /*
     * hash tables of widgets.  The keys are stored as widget data,
     * and get * freed with those widgets.
     */
    GHashTable *name_hash;
	
    /*
     * hash table of signals.  The Data is a GList of GladeSignalData
     * structures which get freed when the GladeXML object is
     * destroyed
     */
    GHashTable *signals;

    /*
     * This hash table contains GSLists that are the radio groups *
     * bound to each name.
     */
    GHashTable *radio_groups;

    /* the current toplevel being built */
    GtkWindow *toplevel;

    /*
     * These items are for handling accelerator groups.  The first *
     * is the main accelerator group for the current window.  The *
     * second is an slist of groups, which is used for the uline *
     * accel groups for menu entries.
     */
    GSList *accel_groups;

    /* a list of label uline accels for widgets that don't exist yet */
    GList *focus_ulines;

    /* these hold the focus and default widgets for a window until they
     * get packed into the window -- we can't call gtk_widget_grab_focus
     * or grab_default until this occurs */
    GtkWidget *focus_widget;
    GtkWidget *default_widget;
};

typedef struct _GladeFocusULine GladeFocusULine;
struct _GladeFocusULine {
    const gchar *widget_name;
    guint key;
};

typedef struct _GladeSignalData GladeSignalData;
struct _GladeSignalData {
    GtkObject *signal_object;
    char *signal_name;
    char *connect_object; /* or NULL if there is none */
    gboolean signal_after;
};

#endif

