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

static GtkWidget *
window_new(GladeXML *xml, GType widget_type, GladeWidgetInfo *info)
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

void
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

void
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

void
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
clist_new (GladeXML *xml, GType widget_type,
	   GladeWidgetInfo *info)
{
    GtkWidget *clist;
    int cols = 1;
    int i;

    const char *name, *value;

    static GArray *props_array = NULL;
    GObjectClass *oclass;
    GList *deferred_props = NULL, *tmp;

    /* "fake" properties */
    const char *column_widths = NULL;
    GtkSelectionMode selection_mode = GTK_SELECTION_SINGLE;
    GtkShadowType shadow_type = GTK_SHADOW_IN;
    gboolean show_titles = TRUE;

    if (!props_array)
	props_array = g_array_new(FALSE, FALSE, sizeof(GParameter));

    /* we ref the class here as a slight optimisation */
    oclass = g_type_class_ref(widget_type);

    /* collect properties */
    for (i = 0; i < info->n_properties; i++) {
	GParameter param = { NULL };
	GParamSpec *pspec;

	pspec = g_object_class_find_property(oclass, info->properties[i].name);

	name = info->properties[i].name;
	value = info->properties[i].value;

	if (!pspec) {
	    switch (name[0]) {
	    case 'c':
		if (!strcmp (name, "column_widths")) {
		    column_widths = g_strdup (value);
		    continue;
		} else 	if (!strcmp (name, "columns")) {
		    g_warning ("columns!");
		    cols = strtol (value, NULL, 0);
		    continue;
		}
		break;
	    case 's':
		if (!strcmp (name, "selection_mode")) {
		    selection_mode = glade_enum_from_string (
			GTK_TYPE_SELECTION_MODE, value);
		    continue;
		} else if (!strcmp (name, "shadow_type")) {
		    shadow_type = glade_enum_from_string (
			GTK_TYPE_SHADOW_TYPE, value);
		    continue;
		} else if (!strcmp (name, "show_titles")) {
		    show_titles = 
			(tolower(value[0]) == 't' || tolower(value[0]) == 'y' || atoi(value));
		    continue;
		}
		break;
	    }

	    g_warning("unknown property `%s' for class `%s'",
		      info->properties[i].name, g_type_name(widget_type));
	    continue;
	}

	/* this should catch all properties wanting a GtkWidget
         * subclass.  We also look for types that could hold a
         * GtkWidget in order to catch things like the
         * GtkAccelLabel::accel_object property.  Since we don't do
         * any handling of GObject or GtkObject directly in
         * glade_xml_set_value_from_string, this shouldn't be a
         * problem. */
	if (g_type_is_a(GTK_TYPE_WIDGET, G_PARAM_SPEC_VALUE_TYPE(pspec)) ||
	    g_type_is_a(G_PARAM_SPEC_VALUE_TYPE(pspec), GTK_TYPE_WIDGET)) {
	    deferred_props = g_list_prepend(deferred_props,
					    &info->properties[i]);
	    continue;
	}

	if (glade_xml_set_value_from_string(xml, pspec,
					    info->properties[i].value,
					    &param.value)) {
	    param.name = info->properties[i].name;
	    g_array_append_val(props_array, param);
	}
    }

    clist = g_object_newv(widget_type, props_array->len,
			   (GParameter *)props_array->data);

    /* clean up props_array */
    for (i = 0; i < props_array->len; i++) {
	g_array_index(props_array, GParameter, i).name = NULL;
	g_value_unset(&g_array_index(props_array, GParameter, i).value);
    }

    if (column_widths) {
	const char *pos = column_widths;
	while (pos && *pos) {
	    int width = strtol (pos, &pos, 0);
	    if (*pos == ',') pos++;
	    gtk_clist_set_column_width (GTK_CLIST (clist), cols++, width);
	}
    }

    gtk_clist_set_selection_mode (GTK_CLIST (clist), selection_mode);
    gtk_clist_set_shadow_type (GTK_CLIST (clist), shadow_type);

    if (show_titles)
	gtk_clist_column_titles_show (GTK_CLIST (clist));
    else
	gtk_clist_column_titles_hide (GTK_CLIST (clist));

    /* handle deferred properties */
    for (tmp = deferred_props; tmp; tmp = tmp->next) {
	GladeProperty *prop = tmp->data;

	glade_xml_handle_widget_prop(xml, clist, prop->name, prop->value);
    }
    g_list_free(tmp);

    g_array_set_size(props_array, 0);
    g_type_class_unref(oclass);

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

static GladeWidgetBuildData widget_data[] = {
    { "GtkAccelLabel", glade_standard_build_widget, NULL,
      gtk_accel_label_get_type },
    { "GtkAlignment", glade_standard_build_widget, glade_standard_build_children,
      gtk_alignment_get_type },
    { "GtkArrow", glade_standard_build_widget, NULL,
      gtk_arrow_get_type },
    { "GtkAspectFrame", glade_standard_build_widget, glade_standard_build_children,
      gtk_aspect_frame_get_type },
    { "GtkButton", build_button, glade_standard_build_children,
      gtk_button_get_type },
    { "GtkCalendar", glade_standard_build_widget, NULL,
      gtk_calendar_get_type },
    { "GtkCheckButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_check_button_get_type },
    { "GtkCheckMenuItem", glade_standard_build_widget, menuitem_build_children,
      gtk_check_menu_item_get_type },
    { "GtkCList", clist_new, clist_build_children,
      gtk_clist_get_type },
    { "GtkColorSelection", glade_standard_build_widget, NULL,
      gtk_color_selection_get_type },
    { "GtkColorSelectionDialog", window_new, glade_standard_build_children,
      gtk_color_selection_dialog_get_type, 0, colorseldlg_find_internal_child },
    { "GtkCombo", glade_standard_build_widget, glade_standard_build_children,
      gtk_combo_get_type, 0, combo_find_internal_child },
    { "GtkCTree", clist_new, clist_build_children,
      gtk_ctree_get_type },
    { "GtkCurve", glade_standard_build_widget, NULL,
      gtk_curve_get_type },
    { "GtkDialog", glade_standard_build_widget, gtk_dialog_build_children,
      gtk_dialog_get_type, 0, dialog_find_internal_child },
    { "GtkDrawingArea", glade_standard_build_widget, NULL,
      gtk_drawing_area_get_type },
    { "GtkEntry", glade_standard_build_widget, NULL,
      gtk_entry_get_type },
    { "GtkEventBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_event_box_get_type },
    { "GtkFileSelection", window_new, glade_standard_build_children,
      gtk_file_selection_get_type, 0, filesel_find_internal_child },
    { "GtkFixed", glade_standard_build_widget, glade_standard_build_children,
      gtk_fixed_get_type },
    { "GtkFontSelection", glade_standard_build_widget, NULL,
      gtk_font_selection_get_type },
    { "GtkFontSelectionDialog", window_new, glade_standard_build_children,
      gtk_font_selection_dialog_get_type, 0, fontseldlg_find_internal_child },
    { "GtkFrame", glade_standard_build_widget, glade_standard_build_children,
      gtk_frame_get_type },
    { "GtkGammaCurve", glade_standard_build_widget, NULL,
      gtk_gamma_curve_get_type },
    { "GtkHandleBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_handle_box_get_type },
    { "GtkHButtonBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_hbutton_box_get_type },
    { "GtkHBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_hbox_get_type },
    { "GtkHPaned", glade_standard_build_widget, glade_standard_build_children,
      gtk_hpaned_get_type },
    { "GtkHRuler", glade_standard_build_widget, NULL,
      gtk_hruler_get_type },
    { "GtkHScale", glade_standard_build_widget, NULL,
      gtk_hscale_get_type },
    { "GtkHScrollbar", glade_standard_build_widget, NULL,
      gtk_hscrollbar_get_type },
    { "GtkHSeparator", glade_standard_build_widget, NULL,
      gtk_hseparator_get_type },
    { "GtkImage", glade_standard_build_widget, NULL,
      gtk_image_get_type },
    { "GtkImageMenuItem", glade_standard_build_widget, menuitem_build_children,
      gtk_image_menu_item_get_type, 0, image_menu_find_internal_child },
    { "GtkInputDialog", window_new, glade_standard_build_children,
      gtk_input_dialog_get_type },
    { "GtkLabel", glade_standard_build_widget, NULL,
      gtk_label_get_type },
    { "GtkLayout", glade_standard_build_widget, glade_standard_build_children,
      gtk_layout_get_type },
    { "GtkList", glade_standard_build_widget, glade_standard_build_children,
      gtk_list_get_type },
    { "GtkListItem", glade_standard_build_widget, glade_standard_build_children,
      gtk_list_item_get_type },
    { "GtkMenu", glade_standard_build_widget, glade_standard_build_children,
      gtk_menu_get_type },
    { "GtkMenuBar", glade_standard_build_widget, glade_standard_build_children,
      gtk_menu_bar_get_type },
    { "GtkMenuItem", glade_standard_build_widget, menuitem_build_children,
      gtk_menu_item_get_type },
    { "GtkMessageDialog", window_new, glade_standard_build_children,
      gtk_message_dialog_get_type },
    { "GtkNotebook", glade_standard_build_widget, notebook_build_children,
      gtk_notebook_get_type },
    { "GtkOptionMenu", glade_standard_build_widget, glade_standard_build_children,
      gtk_option_menu_get_type, 0, option_menu_find_internal_child },
/*    { "GtkPacker", glade_standard_build_widget, glade_standard_build_children,
      gtk_packer_get_type }, */
    { "GtkPixmap", glade_standard_build_widget, NULL,
      gtk_pixmap_get_type },
    { "GtkPlug", window_new, glade_standard_build_children,
      gtk_plug_get_type },
    { "GtkProgress", glade_standard_build_widget, NULL,
      gtk_progress_get_type },
    { "GtkProgressBar", glade_standard_build_widget, NULL,
      gtk_progress_bar_get_type },
    { "GtkRadioButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_radio_button_get_type },
    { "GtkRadioMenuItem", glade_standard_build_widget, menuitem_build_children,
      gtk_radio_menu_item_get_type },
    { "GtkScrolledWindow", glade_standard_build_widget, glade_standard_build_children,
      gtk_scrolled_window_get_type, 0, scrolled_window_find_internal_child },
    { "GtkSeparatorMenuItem", glade_standard_build_widget, NULL,
      gtk_separator_menu_item_get_type },
    { "GtkSocket", glade_standard_build_widget, NULL,
      gtk_socket_get_type },
    { "GtkSpinButton", glade_standard_build_widget, NULL,
      gtk_spin_button_get_type },
    { "GtkStatusbar", glade_standard_build_widget, NULL,
      gtk_statusbar_get_type },
    { "GtkTable", glade_standard_build_widget, glade_standard_build_children,
      gtk_table_get_type },
    { "GtkTearoffMenuItem", glade_standard_build_widget, NULL,
      gtk_tearoff_menu_item_get_type },
    { "GtkText", glade_standard_build_widget, NULL,
      gtk_text_get_type },
    { "GtkTextView", glade_standard_build_widget, NULL,
      gtk_text_view_get_type },
    { "GtkTipsQuery", glade_standard_build_widget, NULL,
      gtk_tips_query_get_type },
    { "GtkToggleButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_toggle_button_get_type },
    { "GtkToolbar", glade_standard_build_widget, glade_standard_build_children,
      gtk_toolbar_get_type },
    { "GtkTree", glade_standard_build_widget, NULL,
      gtk_tree_get_type },
    { "GtkTreeView", glade_standard_build_widget, NULL,
      gtk_tree_view_get_type },
    { "GtkVButtonBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_vbutton_box_get_type },
    { "GtkVBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_vbox_get_type },
    { "GtkVPaned", glade_standard_build_widget, glade_standard_build_children,
      gtk_vpaned_get_type },
    { "GtkVRuler", glade_standard_build_widget, NULL,
      gtk_vruler_get_type },
    { "GtkVScale", glade_standard_build_widget, NULL,
      gtk_vscale_get_type },
    { "GtkVScrollbar", glade_standard_build_widget, NULL,
      gtk_vscrollbar_get_type },
    { "GtkVSeparator", glade_standard_build_widget, NULL,
      gtk_vseparator_get_type },
    { "GtkViewport", glade_standard_build_widget, glade_standard_build_children,
      gtk_viewport_get_type },
    { "GtkWindow", window_new, glade_standard_build_children,
      gtk_window_get_type },
    { NULL, NULL, NULL, 0, 0 }
};

void
_glade_init_gtk_widgets(void)
{
    glade_provide("gtk");
    glade_register_widgets(widget_data);
}
