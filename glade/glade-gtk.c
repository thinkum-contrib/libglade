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
    { "GtkAccelLabel", glade_standard_build_widget, NULL,
      gtk_accel_label_get_type },
    { "GtkAlignment", glade_standard_build_widget, glade_standard_build_children,
      gtk_alignment_get_type },
    { "GtkArrow", glade_standard_build_widget, NULL,
      gtk_arrow_get_type },
    { "GtkAspectFrame", glade_standard_build_widget, glade_standard_build_children,
      gtk_aspect_frame_get_type },
    { "GtkButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_button_get_type },
    { "GtkCalendar", glade_standard_build_widget, NULL,
      gtk_calendar_get_type },
    { "GtkCheckButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_check_button_get_type },
    { "GtkCheckMenuItem", glade_standard_build_widget, glade_standard_build_children,
      gtk_check_menu_item_get_type },
    { "GtkCList", glade_standard_build_widget, glade_standard_build_children,
      gtk_clist_get_type },
    { "GtkColorSelection", glade_standard_build_widget, NULL,
      gtk_color_selection_get_type },
    { "GtkColorSelectionDialog", window_new, glade_standard_build_children,
      gtk_color_selection_dialog_get_type },
    { "GtkCombo", glade_standard_build_widget, glade_standard_build_children,
      gtk_combo_get_type },
    { "GtkCTree", glade_standard_build_widget, glade_standard_build_children,
      gtk_ctree_get_type },
    { "GtkCurve", glade_standard_build_widget, NULL,
      gtk_curve_get_type },
    { "GtkDialog", glade_standard_build_widget, glade_standard_build_children,
      gtk_dialog_get_type },
    { "GtkDrawingArea", glade_standard_build_widget, NULL,
      gtk_drawing_area_get_type },
    { "GtkEntry", glade_standard_build_widget, NULL,
      gtk_entry_get_type },
    { "GtkEventBox", glade_standard_build_widget, glade_standard_build_children,
      gtk_event_box_get_type },
    { "GtkFileSelection", window_new, glade_standard_build_children,
      gtk_file_selection_get_type },
    { "GtkFixed", glade_standard_build_widget, glade_standard_build_children,
      gtk_fixed_get_type },
    { "GtkFontSelection", glade_standard_build_widget, NULL,
      gtk_font_selection_get_type },
    { "GtkFontSelectionDialog", window_new, glade_standard_build_children,
      gtk_font_selection_dialog_get_type },
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
    { "GtkImageMenuItem", glade_standard_build_widget, glade_standard_build_children,
      gtk_image_menu_item_get_type },
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
    { "GtkMenuItem", glade_standard_build_widget, glade_standard_build_children,
      gtk_menu_item_get_type },
    { "GtkMessageDialog", window_new, glade_standard_build_children,
      gtk_message_dialog_get_type },
    { "GtkNotebook", glade_standard_build_widget, glade_standard_build_children,
      gtk_notebook_get_type },
    { "GtkOptionMenu", glade_standard_build_widget, glade_standard_build_children,
      gtk_option_menu_get_type },
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
    { "GtkRadioMenuItem", glade_standard_build_widget, glade_standard_build_children,
      gtk_radio_menu_item_get_type },
    { "GtkScrolledWindow", glade_standard_build_widget, glade_standard_build_children,
      gtk_scrolled_window_get_type },
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
    { "GtkTextView", glade_standard_build_widget, NULL,
      gtk_text_view_get_type },
    { "GtkTipsQuery", glade_standard_build_widget, NULL,
      gtk_tips_query_get_type },
    { "GtkToggleButton", glade_standard_build_widget, glade_standard_build_children,
      gtk_toggle_button_get_type },
    { "GtkToolbar", glade_standard_build_widget, glade_standard_build_children,
      gtk_toolbar_get_type },
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
