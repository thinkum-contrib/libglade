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
#include <stdlib.h>
#include <ctype.h>
#include <glade/glade.h>
#include <glade/glade-build.h>
#include <glade/glade-private.h>

/* for GtkText et all */
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>

void _glade_init_gtk_widgets(void);

static void
set_tooltip(GladeXML *xml, GtkWidget *widget,
	    const gchar *prop_name, const gchar *prop_value)
{
    gtk_tooltips_set_tip(xml->priv->tooltips, widget, prop_value, NULL);
}

static GtkWidget *
build_window(GladeXML *xml, GType widget_type, GladeWidgetInfo *info)
{
    GtkWidget *window = glade_standard_build_widget(xml, widget_type, info);

    glade_xml_set_toplevel(xml, GTK_WINDOW(window));

    return window;
}

static GtkWidget *
placeholder_create (void)
{
    GtkWidget *pl = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_widget_show (pl);
    return pl;
}

static void
menuitem_build_children(GladeXML *self, GtkWidget *w,
			GladeWidgetInfo *info)
{
    gint i;

    g_object_ref(G_OBJECT(w));
    for (i = 0; i < info->n_children; i++) {
	GtkWidget       *child;
	GladeWidgetInfo *childinfo = info->children[i].child;

	if (info->children[i].internal_child) {
	    glade_xml_handle_internal_child(self, w, &info->children[i]);
	    continue;
	}

	child = glade_xml_build_widget(self, childinfo);

	if (GTK_IS_MENU(child))
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), child);
	else
	    gtk_container_add(GTK_CONTAINER(w), child);
    }
    g_object_unref(G_OBJECT(w));
}

static void
gtk_dialog_build_children(GladeXML *self, GtkWidget *w,
			  GladeWidgetInfo *info)

{
    GtkDialog *dialog = GTK_DIALOG (w);
    GList *children, *list;

    glade_standard_build_children (self, w, info);

    if (dialog->action_area == NULL)
	return;

    children = gtk_container_get_children (GTK_CONTAINER (dialog->action_area));
    for (list = children; list; list = list->next) {
	gtk_widget_ref (GTK_WIDGET (list->data));
	gtk_container_remove (GTK_CONTAINER (dialog->action_area), GTK_WIDGET (list->data));
    }

    for (list = children; list; list = list->next) {
	gint response_id;
	response_id = GPOINTER_TO_INT (g_object_steal_data (
	    G_OBJECT (list->data), "response_id"));
	gtk_dialog_add_action_widget (dialog, GTK_WIDGET (list->data), response_id);
    }
    g_list_foreach (children, (GFunc)gtk_widget_unref, NULL);
    g_list_free (children);
}

static void
notebook_build_children(GladeXML *self, GtkWidget *parent,
			GladeWidgetInfo *info)
{
    gint i, j, tab = 0;
    enum {
	PANE_ITEM,
	TAB_ITEM,
	MENU_ITEM
    } type;

    g_object_ref(G_OBJECT(parent));
    for (i = 0; i < info->n_children; i++) {
	GladeWidgetInfo *childinfo = info->children[i].child;
	GtkWidget *child = glade_xml_build_widget(self, childinfo);

	type = PANE_ITEM;
	for (j = 0; j < info->children[i].n_properties; j++) {
	    if (!strcmp (info->children[i].properties[j].name, "type")) {
		const char *value = info->children[i].properties[j].value;

		if (!strcmp (value, "tab"))
		    type = TAB_ITEM;
		break;
	    }
	}

	if (type == TAB_ITEM) { /* The GtkNotebook API blows */
	    GtkWidget *body;

	    body = gtk_notebook_get_nth_page (GTK_NOTEBOOK (parent), (tab - 1));
	    gtk_notebook_set_tab_label (GTK_NOTEBOOK (parent), body, child);
	} else {
	    gtk_notebook_append_page (GTK_NOTEBOOK (parent), child, NULL);
	    tab++;
	}
    }
    g_object_unref(G_OBJECT(parent));
}

static GtkWidget *
build_progress (GladeXML *xml, GType widget_type,
		GladeWidgetInfo *info)
{
    GtkWidget *pbar;
    char *format = NULL;
    int i;

    for (i = 0; i < info->n_properties; i++) {
	if (!strcmp (info->properties[i].name, "format")) {
	    format = info->properties[i].value;
	    break;
	}
    }

    g_message ("ignore warnings about the following: format");

    pbar = glade_standard_build_widget (xml, widget_type, info);

    if (format && *format)
	gtk_progress_set_format_string (GTK_PROGRESS (pbar), format);

    return pbar;
}

static GtkWidget *
build_option_menu (GladeXML *xml, GType widget_type,
		   GladeWidgetInfo *info)
{
    GtkWidget *option_menu;
    int i, history = 0;

    for (i = 0; i < info->n_properties; i++) {
	if (!strcmp (info->properties[i].name, "history")) {
	    history = strtol (info->properties[i].value, NULL, 10);
	    break;
	}
    }

    g_message ("ignore warnings about the following: history");

    option_menu = glade_standard_build_widget (xml, widget_type, info);

    if (history)
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), history);

    return option_menu;
}

static GtkWidget *
build_text_view (GladeXML *xml, GType widget_type,
		 GladeWidgetInfo *info)
{
    GtkWidget *text_view;
    char *text = NULL;
    int i;

    for (i = 0; i < info->n_properties; i++) {
	if (!strcmp (info->properties[i].name, "text")) {
	    text = info->properties[i].value;
	    break;
	}
    }

    g_message ("ignore warnings about the following: text");

    text_view = glade_standard_build_widget (xml, widget_type, info);

    if (text && *text) {
	GtkTextBuffer *buffy;

	buffy = gtk_text_buffer_new (NULL);
	gtk_text_buffer_set_text (buffy, text, strlen (text));
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (text_view), buffy);
	g_object_unref (G_OBJECT (buffy));
    }

    return text_view;
}

static GtkWidget *
build_clist (GladeXML *xml, GType widget_type,
	     GladeWidgetInfo *info)
{
    GtkWidget *clist;
    int cols = 1;
    int i;

    const char *name, *value;

    /* "fake" properties */
    char *column_widths = NULL;
    GtkSelectionMode selection_mode = GTK_SELECTION_SINGLE;
    GtkShadowType shadow_type = GTK_SHADOW_IN;
    gboolean show_titles = TRUE;

    /* collect our custom properties */
    for (i = 0; i < info->n_properties; i++) {
	name = info->properties[i].name;
	value = info->properties[i].value;

	if (!strcmp (name, "column_widths"))
	    column_widths = g_strdup (value);

	else if (!strcmp (name, "columns"))
	    cols = strtol (value, NULL, 0);

	else if (!strcmp (name, "selection_mode"))
	    selection_mode = glade_enum_from_string (
		GTK_TYPE_SELECTION_MODE, value);

	else if (!strcmp (name, "shadow_type"))
	    shadow_type = glade_enum_from_string (
		GTK_TYPE_SHADOW_TYPE, value);

	else if (!strcmp (name, "show_titles"))
	    show_titles = 
		(tolower(value[0]) == 't' || tolower(value[0]) == 'y' || atoi(value));
    }

    g_message ("ignore warnings about the following: column_widths, columns, selection_mode, shadow_type, show_titles");

    clist = glade_standard_build_widget (xml, widget_type, info);

    if (column_widths) {
	char *pos = column_widths;
	while (pos && *pos) {
	    int width = strtol (pos, &pos, 0);
	    if (*pos == ',') pos++;
	    gtk_clist_set_column_width (GTK_CLIST (clist), cols++, width);
	}
	g_free (column_widths);
    }

    gtk_clist_set_selection_mode (GTK_CLIST (clist), selection_mode);
    gtk_clist_set_shadow_type (GTK_CLIST (clist), shadow_type);

    if (show_titles)
	gtk_clist_column_titles_show (GTK_CLIST (clist));
    else
	gtk_clist_column_titles_hide (GTK_CLIST (clist));

    return clist;
}

static void
clist_build_children(GladeXML *self, GtkWidget *parent,
		     GladeWidgetInfo *info)
{
    int i;

    g_object_ref (G_OBJECT (parent));
    for (i = 0; i < info->n_children; i++) {
	GladeWidgetInfo *childinfo;
	GtkWidget *child = NULL;

	childinfo = info->children[i].child;

	/* treat GtkLabels specially */
	if (!strcmp (childinfo->class, "GtkLabel")) {
	    int j;
	    const char *label = NULL;

	    for (j = 0; j < childinfo->n_properties; j++) {
		if (!strcmp (childinfo->properties[j].name, "label")) {
		    label = childinfo->properties[j].value;
		    break;
		}
	    }

	    if (label) {
		/* FIXME: translate ? */
		gtk_clist_set_column_title (GTK_CLIST(parent), i, label);
		child = gtk_clist_get_column_widget (GTK_CLIST (parent), i);
		child = GTK_BIN(child)->child;
		glade_xml_set_common_params(self, child, childinfo);
	    }
	}

	if (!child) {
	    child = glade_xml_build_widget (self, childinfo);
	    gtk_clist_set_column_widget (GTK_CLIST (parent), i, child);
	}
    }

    g_object_unref (G_OBJECT (parent));
}

static GtkWidget *
build_button(GladeXML *xml, GType widget_type,
	     GladeWidgetInfo *info)
{
    GtkWidget *widget;
    gint i, response_id = 0;

    for (i = 0; i < info->n_properties; i++) {
	if (!strcmp (info->properties[i].name, "response_id")) {
	    response_id = strtol (info->properties[i].value, NULL, 10);
	    break;
	}
    }

    widget = glade_standard_build_widget (xml, widget_type, info);

    if (response_id)
	g_object_set_data (G_OBJECT (widget), "response_id",
			   GINT_TO_POINTER (response_id));

    return widget;
}

static GtkWidget *
dialog_find_internal_child(GladeXML *xml, GtkWidget *parent,
			   const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GTK_DIALOG(parent)->vbox;
    if (!strcmp(childname, "action_area"))
	return GTK_DIALOG(parent)->action_area;

    return NULL;
}

static GtkWidget *
image_menu_find_internal_child(GladeXML *xml, GtkWidget *parent,
			       const gchar *childname)
{
    if (!strcmp(childname, "image")) {
	GtkWidget *pl = placeholder_create ();

	gtk_image_menu_item_set_image (
	    GTK_IMAGE_MENU_ITEM (parent), pl);

	return pl;
    }
    return NULL;
}

static GtkWidget *
option_menu_find_internal_child(GladeXML *xml, GtkWidget *parent,
				const gchar *childname)
{
    if (!strcmp(childname, "menu")) {
	GtkWidget *ret;
	
	if ((ret = gtk_option_menu_get_menu (GTK_OPTION_MENU (parent))))
	    return ret;

	ret = gtk_menu_new ();
	gtk_widget_show (ret);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (parent), ret);

	return ret;
    }

    return NULL;
}

static GtkWidget *
scrolled_window_find_internal_child(GladeXML *xml, GtkWidget *parent,
				    const gchar *childname)
{
    if (!strcmp(childname, "vscrollbar"))
	return GTK_SCROLLED_WINDOW (parent)->vscrollbar;
    if (!strcmp(childname, "hscrollbar"))
	return GTK_SCROLLED_WINDOW (parent)->hscrollbar;
    return NULL;
}

static GtkWidget *
filesel_find_internal_child(GladeXML *xml, GtkWidget *parent,
			    const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GTK_DIALOG(parent)->vbox;
    if (!strcmp(childname, "action_area"))
	return GTK_DIALOG(parent)->action_area;
    if (!strcmp(childname, "ok_button"))
	return GTK_FILE_SELECTION(parent)->ok_button;
    if (!strcmp(childname, "cancel_button"))
	return GTK_FILE_SELECTION(parent)->cancel_button;
    if (!strcmp(childname, "help_button"))
	return GTK_FILE_SELECTION(parent)->help_button;
    return NULL;
}

static GtkWidget *
colorseldlg_find_internal_child(GladeXML *xml, GtkWidget *parent,
				const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GTK_DIALOG(parent)->vbox;
    if (!strcmp(childname, "action_area"))
	return GTK_DIALOG(parent)->action_area;
    if (!strcmp(childname, "ok_button"))
	return GTK_COLOR_SELECTION_DIALOG(parent)->ok_button;
    if (!strcmp(childname, "cancel_button"))
	return GTK_COLOR_SELECTION_DIALOG(parent)->cancel_button;
    if (!strcmp(childname, "help_button"))
	return GTK_COLOR_SELECTION_DIALOG(parent)->help_button;
    return NULL;
}

static GtkWidget *
fontseldlg_find_internal_child(GladeXML *xml, GtkWidget *parent,
			       const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GTK_DIALOG(parent)->vbox;
    if (!strcmp(childname, "action_area"))
	return GTK_DIALOG(parent)->action_area;
    if (!strcmp(childname, "ok_button"))
	return GTK_FONT_SELECTION_DIALOG(parent)->ok_button;
    if (!strcmp(childname, "cancel_button"))
	return GTK_FONT_SELECTION_DIALOG(parent)->cancel_button;
    if (!strcmp(childname, "apply_button"))
	return GTK_FONT_SELECTION_DIALOG(parent)->apply_button;
    return NULL;
}

static GtkWidget *
combo_find_internal_child(GladeXML *xml, GtkWidget *parent,
			  const gchar *childname)
{
    if (!strcmp(childname, "entry"))
	return GTK_COMBO(parent)->entry;
    if (!strcmp(childname, "button"))
	return GTK_COMBO(parent)->button;
    if (!strcmp(childname, "popup"))
	return GTK_COMBO(parent)->popup;
    if (!strcmp(childname, "pupwin"))
	return GTK_COMBO(parent)->popwin;
    if (!strcmp(childname, "list"))
	return GTK_COMBO(parent)->list;
    return NULL;
}

void
_glade_init_gtk_widgets(void)
{
    glade_register_custom_prop (GTK_TYPE_WIDGET, "tooltip", set_tooltip);

    glade_register_widget (GTK_TYPE_ACCEL_LABEL, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_ALIGNMENT, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_ARROW, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_ASPECT_FRAME, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_BUTTON, build_button,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_CALENDAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_CHECK_BUTTON, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_CHECK_MENU_ITEM, glade_standard_build_widget,
			   menuitem_build_children, NULL);
    glade_register_widget (GTK_TYPE_CLIST, build_clist,
			   clist_build_children, NULL);
    glade_register_widget (GTK_TYPE_COLOR_SELECTION, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_COLOR_SELECTION_DIALOG, build_window,
			   glade_standard_build_children, colorseldlg_find_internal_child);
    glade_register_widget (GTK_TYPE_COMBO, glade_standard_build_widget,
			   glade_standard_build_children, combo_find_internal_child);
    glade_register_widget (GTK_TYPE_CTREE, build_clist,
			   clist_build_children, NULL);
    glade_register_widget (GTK_TYPE_CURVE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_DIALOG, glade_standard_build_widget,
			   gtk_dialog_build_children, dialog_find_internal_child);
    glade_register_widget (GTK_TYPE_DRAWING_AREA, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_ENTRY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_EVENT_BOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_FILE_SELECTION, build_window,
			   glade_standard_build_children, filesel_find_internal_child);
    glade_register_widget (GTK_TYPE_FIXED, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_FONT_SELECTION, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_FONT_SELECTION_DIALOG, build_window,
			   glade_standard_build_children, fontseldlg_find_internal_child);
    glade_register_widget (GTK_TYPE_FRAME, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_GAMMA_CURVE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_HANDLE_BOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_HBUTTON_BOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_HBOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_HPANED, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_HRULER, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_HSCALE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_HSCROLLBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_HSEPARATOR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_IMAGE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_IMAGE_MENU_ITEM, glade_standard_build_widget,
			   menuitem_build_children, image_menu_find_internal_child);
    glade_register_widget (GTK_TYPE_INPUT_DIALOG, build_window,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_LABEL, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_LAYOUT, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_LIST, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_LIST_ITEM, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_MENU, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_MENU_BAR, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_MENU_ITEM, glade_standard_build_widget,
			   menuitem_build_children, NULL);
    glade_register_widget (GTK_TYPE_MESSAGE_DIALOG, build_window,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_NOTEBOOK, glade_standard_build_widget,
			   notebook_build_children, NULL);
    glade_register_widget (GTK_TYPE_OPTION_MENU, build_option_menu,
			   glade_standard_build_children, option_menu_find_internal_child);
    glade_register_widget (GTK_TYPE_PIXMAP, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_PLUG, build_window,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_PROGRESS, build_progress,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_PROGRESS_BAR, build_progress,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_RADIO_BUTTON, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_RADIO_MENU_ITEM, glade_standard_build_widget,
			   menuitem_build_children, NULL);
    glade_register_widget (GTK_TYPE_SCROLLED_WINDOW, glade_standard_build_widget,
			   glade_standard_build_children,
			   scrolled_window_find_internal_child);
    glade_register_widget (GTK_TYPE_SOCKET, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_SPIN_BUTTON, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_STATUSBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TABLE, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_TEAROFF_MENU_ITEM, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TEXT, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TEXT_VIEW, build_text_view,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TIPS_QUERY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TOGGLE_BUTTON, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_TOOLBAR, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_TREE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_TREE_VIEW, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_VBUTTON_BOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_VBOX, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_VPANED, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_VRULER, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_VSCALE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_VSCROLLBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_VSEPARATOR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GTK_TYPE_VIEWPORT, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GTK_TYPE_WINDOW, build_window,
			   glade_standard_build_children, NULL);

    glade_provide("gtk");
}
