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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade-build.h>
#include <glade/glade-private.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <tree.h>

/* functions to actually build the widgets */

static void
menushell_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
			  const char *longname)
{
	GNode *childnode;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget(xml, childnode, longname);
		gtk_menu_shell_append(GTK_MENU_SHELL(w), child);
	}
}

static void
menuitem_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
			 const char *longname)
{
	GtkWidget *menu;
	if (!node->children) return;
	menu = glade_xml_build_widget(xml, node->children, longname);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_widget_hide(menu); /* wierd things happen if menu is initially visible */
}

static void
box_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
		    const char *longname)
{
	GNode *childnode;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget (xml, childnode, longname);
		xmlNodePtr xmlnode = childnode->data;
		gboolean expand = TRUE, fill = TRUE, start = TRUE;
		gint padding = 0;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next)
			if (!strcmp (xmlnode->name, "child"))
				break;

		/* catch cases where child node doesn't exist */
		if (!xmlnode){
			gtk_box_pack_start_defaults (GTK_BOX(w), child);
			continue;
		}

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next) {
			char *content = xmlNodeGetContent (xmlnode);

			switch (xmlnode->name [0]) {
			case 'e':
				if (!strcmp (xmlnode->name, "expand"))
					expand = content[0] == 'T';
				break;
			case 'f':
				if (!strcmp (xmlnode->name, "fill"))
					fill = content[0] == 'T';
				break;
			case 'p':
				if (!strcmp (xmlnode->name, "padding"))
					padding = strtol(content, NULL, 0);
				else if (!strcmp (xmlnode->name, "pack"))
					start = strcmp (content, "GTK_PACK_START") == 0;
				break;
			}
			if (content)
				free (content);
		}
		if (start)
			gtk_box_pack_start (GTK_BOX(w), child, expand, fill, padding);
		else
			gtk_box_pack_end (GTK_BOX(w), child, expand, fill, padding);
	}
}

static void
table_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
		      const char *longname) {
	GNode *childnode;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget (xml, childnode, longname);
		xmlNodePtr xmlnode = childnode->data;
		gint left_attach=0, right_attach=1, top_attach=0, bottom_attach=1;
		gint xpad=0, ypad=0, xoptions=0, yoptions=0;
    
		/* find the child xmlNode for this widget */
		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child"))
				break;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next) {
			char *content = xmlNodeGetContent (xmlnode);
			switch (xmlnode->name[0]) {
			case 'b':
				if (!strcmp(xmlnode->name, "bottom_attach"))
					bottom_attach = strtol(content, NULL, 0);
				break;
			case 'l':
				if (!strcmp(xmlnode->name, "left_attach"))
					left_attach = strtol(content, NULL, 0);
				break;
			case 'r':
				if (!strcmp(xmlnode->name, "right_attach"))
					right_attach = strtol(content, NULL, 0);
				break;
			case 't':
				if (!strcmp(xmlnode->name, "top_attach"))
					top_attach = strtol(content, NULL, 0);
				break;
			case 'x':
				switch (xmlnode->name[1]) {
				case 'e':
					if (!strcmp(xmlnode->name, "xexpand") && content[0] == 'T')
						xoptions |= GTK_EXPAND;
					break;
				case 'f':
					if (!strcmp(xmlnode->name, "xfill") && content[0] == 'T')
						xoptions |= GTK_FILL;
					break;
				case 'p':
					if (!strcmp(xmlnode->name, "xpad"))
						xpad = strtol(content, NULL, 0);
					break;
				case 's':
					if (!strcmp(xmlnode->name, "xshrink") && content[0] == 'T')
						xoptions |= GTK_SHRINK;
					break;
				}
				break;
			case 'y':
				switch (xmlnode->name[1]) {
				case 'e':
					if (!strcmp(xmlnode->name, "yexpand") && content[0] == 'T')
						yoptions |= GTK_EXPAND;
					break;
				case 'f':
					if (!strcmp(xmlnode->name, "yfill") && content[0] == 'T')
						yoptions |= GTK_FILL;
					break;
				case 'p':
					if (!strcmp(xmlnode->name, "ypad"))
						ypad = strtol(content, NULL, 0);
					break;
				case 's':
					if (!strcmp(xmlnode->name, "yshrink") && content[0] == 'T')
						yoptions |= GTK_SHRINK;
					break;
				}
				break;
			}

			if (content)
				free (content);
		}
		gtk_table_attach(GTK_TABLE(w), child, left_attach, right_attach,
				 top_attach, bottom_attach, xoptions, yoptions, xpad, ypad);
	}  
}

static void
fixed_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
		      const char *longname)
{
	GNode *childnode;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget(xml, childnode, longname);
		xmlNodePtr xmlnode;
		gint xpos = 0, ypos = 0;

		for (xmlnode = childnode->data; xmlnode; xmlnode = xmlnode->next) {
			char *content = xmlNodeGetContent (xmlnode);
			
			if (xmlnode->name[0] == 'x' && xmlnode->name[1] == '\0')
				xpos = strtol(content, NULL, 0);
			else if (xmlnode->name[0] == 'y' && xmlnode->name[1] == '\0')
				ypos = strtol(content, NULL, 0);

			if (content)
				free(content);
		}
		gtk_fixed_put(GTK_FIXED(w), child, xpos, ypos);
	}
}

static void
clist_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
		      const char *longname)
{
	GNode *childnode;
	gint col = 0;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget (xml, childnode, longname);

		gtk_clist_set_column_widget (GTK_CLIST(w), col, child);
		col++;
	}
}

static void paned_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
				  const char *longname)
{
	GtkWidget *child;

	node = node->children;
	if (!node)
		return;
	
	child = glade_xml_build_widget (xml, node, longname);
	gtk_paned_add1 (GTK_PANED(w), child);

	node = node->next;
	if (! node)
		return;
	
	child = glade_xml_build_widget (xml, node, longname);
	gtk_paned_add2 (GTK_PANED(w), child);
}

static void
notebook_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
			 const char *longname)
{

	/*
	 * the notebook tabs are listed after the pages, and have
	 * child_name set to Notebook:tab.  We store pages in a GList
	 * while waiting for tabs
	 */
	
	GList *pages = NULL;
	GNode *childnode;

	for (childnode = node->children; childnode; childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget (xml, childnode, longname);
		xmlNodePtr xmlnode = (xmlNodePtr)childnode->data;
		char *content;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next)
			if (!strcmp (xmlnode->name, "child_name"))
				break;

		content = xmlNodeGetContent (xmlnode);
		if (xmlnode == NULL || strcmp (content, "Notebook:tab") != 0)
			pages = g_list_append (pages, child);
		else {
			GList *head = pages;
			GtkWidget *page = head->data;
			pages = g_list_remove_link (pages, head);
			g_list_free (head);
			gtk_notebook_append_page (GTK_NOTEBOOK(w), page, child);
		}

		if (content)
			free(content);
	}
}

static void
dialog_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
		       const char *longname)
{
	xmlNodePtr info;
	GNode *childnode;
	char *vboxname, *content;

	for (info = ((xmlNodePtr)node->children->data)->childs;
	     info; info = info->next)
		if (!strcmp(info->name, "name"))
			break;

	g_assert(info != NULL);

	content = xmlNodeGetContent (info);
	vboxname = g_strconcat (longname, ".", content, NULL);

	if (content)
		free(content);

	for (childnode = node->children->children; childnode; childnode = childnode->next) {
		GtkWidget *child;
		xmlNodePtr xmlnode = childnode->data;
		gboolean expand = TRUE, fill = TRUE, start = TRUE;
		gint padding = 0;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child_name"))
				break;

		content = xmlNodeGetContent (xmlnode);
		if (xmlnode && !strcmp (content, "Dialog:action_area")) {
			char *parent_name;

			if (content)
				free (content);

			/* we got the action area -- call box_build_children on it */
			for (xmlnode = ((xmlNodePtr)childnode->data)->childs;
			     xmlnode; xmlnode = xmlnode->next)
				if (!strcmp(xmlnode->name, "name"))
					break;

			g_assert(xmlnode != NULL);

			content = xmlNodeGetContent (xmlnode);
			parent_name = g_strconcat (vboxname, ".", content, NULL);
			box_build_children (xml, GTK_DIALOG(w)->action_area,
					    childnode, parent_name);
			g_free(parent_name);

			if (content)
				free(content);
			continue;
		}
		if (content)
			free(content);

		child = glade_xml_build_widget (xml, childnode, vboxname);
		for (xmlnode = ((xmlNodePtr)childnode->data)->childs;
		     xmlnode; xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child"))
				break;
		
		/* catch cases where child node doesn't exist */
		if (!xmlnode) {
			gtk_box_pack_start_defaults (
					GTK_BOX(GTK_DIALOG(w)->vbox), child);
			continue;
		}
		
		for (xmlnode = xmlnode->childs; xmlnode; xmlnode = xmlnode->next) {
			content = xmlNodeGetContent (xmlnode);
			switch (xmlnode->name[0]) {
			case 'e':
				if (!strcmp(xmlnode->name, "expand"))
					expand = content[0] == 'T';
				break;
			case 'f':
				if (!strcmp(xmlnode->name, "fill"))
					fill = content[0] == 'T';
				break;
			case 'p':
				if (!strcmp(xmlnode->name, "padding"))
					padding = strtol(content, NULL, 0);
				else if (!strcmp(xmlnode->name, "pack"))
					start = strcmp(content, "GTK_PACK_START") == 0;
				break;
			}
			if (content)
				free(content);
		}
		if (start)
			gtk_box_pack_start (GTK_BOX(GTK_DIALOG(w)->vbox),
					    child, expand, fill, padding);
		else
			gtk_box_pack_end (GTK_BOX(GTK_DIALOG(w)->vbox),
					  child, expand, fill, padding);
	}
	g_free (vboxname);
}

static void
misc_set (GtkMisc *misc, xmlNodePtr info)
{
	for (info = info->childs; info; info = info->next){
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'x':
			if (!strcmp (info->name, "xalign")){
				gfloat align = atof (content);
				gtk_misc_set_alignment(misc, align, misc->yalign);
			} else if (!strcmp(info->name, "xpad")){
				gint pad = strtol(content, NULL, 0);
				gtk_misc_set_padding(misc, pad, misc->ypad);
			} break;
			
		case 'y':
			if (!strcmp(info->name, "yalign")){
				gfloat align = atof (content);
				gtk_misc_set_alignment(misc, misc->xalign, align);
			} else if (!strcmp(info->name, "ypad")){
				gint pad = strtol(content, NULL, 0);
				gtk_misc_set_padding(misc, misc->xpad, pad);
			}
			break;
		}
		if (content) free(content);
	}
}

static GtkWidget *
label_new (GladeXML *xml, GNode *node)
{
	GtkWidget *label;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
	GtkJustification just = GTK_JUSTIFY_CENTER;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		} else if (!strcmp(info->name, "justify"))
			just = glade_enum_from_string(GTK_TYPE_JUSTIFICATION, content);

		if (content)
			free (content);
	}

	label = gtk_label_new(string);
	if (string)
		g_free(string);
	if (just != GTK_JUSTIFY_CENTER)
		gtk_label_set_justify(GTK_LABEL(label), just);
	misc_set (GTK_MISC(label), node->data);
	return label;
}

static GtkWidget *
accellabel_new (GladeXML *xml, GNode *node)
{
	GtkWidget *label;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
	GtkJustification just = GTK_JUSTIFY_CENTER;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		} else if (!strcmp(info->name, "justify"))
			just = glade_enum_from_string(GTK_TYPE_JUSTIFICATION, content);

		if (content)
			free(content);
	}
	label = gtk_accel_label_new(string);
	if (string)
		g_free(string);
	if (just != GTK_JUSTIFY_CENTER)
		gtk_label_set_justify(GTK_LABEL(label), just);

	misc_set(GTK_MISC(label), node->data);

	return label;
}

static GtkWidget *
entry_new (GladeXML *xml, GNode *node)
{
	GtkWidget *entry;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *text = NULL;
	gint text_max_length = -1;
	gboolean editable = TRUE, text_visible = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'e':
			if (!strcmp(info->name, "editable"))
				editable = content[0] == 'T';
			break;
		case 't':
			if (!strcmp(info->name, "text")) {
				if (text) g_free(text);
				text = g_strdup(content);
			} else if (!strcmp(info->name, "text_visible"))
				text_visible = content[0] == 'T';
			else if (!strcmp(info->name, "text_max_length"))
				text_max_length = strtol(content, NULL, 0);
			break;
		}
		if (content)
			free(content);
	}
	if (text_max_length >= 0)
		entry = gtk_entry_new_with_max_length(text_max_length);
	else
		entry = gtk_entry_new();

	if (text) {
		gtk_entry_set_text(GTK_ENTRY(entry), text);
		g_free(text);
	}

	gtk_entry_set_editable(GTK_ENTRY(entry), editable);
	gtk_entry_set_visibility(GTK_ENTRY(entry), text_visible);
	return entry;
}

static GtkWidget *
text_new(GladeXML *xml, GNode *node)
{
	GtkWidget *wid;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *text = NULL;
	gboolean editable = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		switch (info->name[0]) {
		case 'e':
			if (!strcmp(info->name, "editable"))
				editable = content[0] == 'T';
			break;
		case 't':
			if (!strcmp(info->name, "text")) {
				if (text) g_free(text);
				text = g_strdup(content);
			}
			break;
		}
		if (content)
			free(content);
	}

	wid = gtk_text_new(NULL, NULL);
	if (text) {
		gtk_editable_insert_text(GTK_EDITABLE(wid), text, strlen(text), NULL);
		g_free(text);
	}
	gtk_text_set_editable(GTK_TEXT(wid), editable);
	return wid;
}

static GtkWidget *
button_new(GladeXML *xml, GNode *node)
{
	GtkWidget *button;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
  
	/*
	 * This should really be a container, but GLADE is wierd in this respect.
	 * If the label property is set for this widget, insert a label.
	 * Otherwise, allow a widget to be packed
	 */
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		}
		if (content)
			free(content);
	}
	if (string != NULL) {
		button = gtk_button_new_with_label(string);
	} else
		button = gtk_button_new();

	if (string)
		g_free (string);
	return button;
}

static GtkWidget *
togglebutton_new(GladeXML *xml, GNode *node)
{
	GtkWidget *button;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
	gboolean active = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		} else if (!strcmp(info->name, "active"))
			active = content[0] == 'T';

		if (content)
			free (content);
	}
	if (string != NULL) {
		button = gtk_toggle_button_new_with_label(string);
		g_free(string);
	} else
		button = gtk_toggle_button_new();
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), active);
	return button;
}

static GtkWidget *
checkbutton_new (GladeXML *xml, GNode *node)
{
	GtkWidget *button;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
	gboolean active = FALSE, draw_indicator = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		} else if (!strcmp(info->name, "active"))
			active = content[0] == 'T';
		else if (!strcmp(info->name, "draw_indicator"))
			draw_indicator = content[0] == 'T';

		if (content)
			free(content);
	}
	if (string != NULL) {
		button = gtk_check_button_new_with_label(string);
		g_free(string);
	} else
		button = gtk_check_button_new();

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), active);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), draw_indicator);
	return button;
}

static GtkWidget *
radiobutton_new(GladeXML *xml, GNode *node)
{
	GtkWidget *button;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *string = NULL;
	gboolean active = FALSE, draw_indicator = TRUE;
	GSList *group = NULL;
	char *group_name = NULL;
	
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (string) g_free(string);
			string = g_strdup(content);
		} else if (!strcmp(info->name, "active"))
			active = content[0] == 'T';
		else if (!strcmp(info->name, "draw_indicator"))
			draw_indicator = content[0] == 'T';
		else if (!strcmp(info->name, "group")){
			group = g_hash_table_lookup (xml->priv->radio_groups, content);
			if (!group){
				if (group_name)
					g_free (group_name);
				group_name = g_strdup (content);
			}
		}
		
		if (content)
			free(content);
	}
	if (string != NULL) {
		button = gtk_radio_button_new_with_label(group, string);
		g_free(string);
	} else
		button = gtk_radio_button_new (group);

	/*
	 * If this is the first radio item within this group
	 * add it.
	 */
	if (group == NULL && group_name){
		GtkRadioButton *radio = GTK_RADIO_BUTTON (button);
		
		g_hash_table_insert (xml->priv->radio_groups,
				     g_strdup (group_name),
				     gtk_radio_button_group (radio));
	} 

	if (group_name)
		g_free (group_name);
	
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), active);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), draw_indicator);
	return button;
}

static GtkWidget *
optionmenu_new(GladeXML *xml, GNode *node)
{
	GtkWidget *option = gtk_option_menu_new(), *menu, *menuitem;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int initial_choice = 0;

	menu = gtk_menu_new();
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "items") && content) {
			char *pos = content;
			char *items_end = &content[strlen(content)];
			while (pos < items_end) {
				gchar *item_end = strchr (pos, '\n');
				if (item_end == NULL)
					item_end = items_end;
				*item_end = '\0';
	
				menuitem = gtk_menu_item_new_with_label (pos);
				gtk_widget_show (menuitem);
				gtk_menu_append (GTK_MENU (menu), menuitem);
	
				pos = item_end + 1;
			}
		} else if (!strcmp(info->name, "initial_choice"))
			initial_choice = strtol(content, NULL, 0);

		if (content)
			free(content);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(option), initial_choice);
	return option;
}

static GtkWidget *
combo_new (GladeXML *xml, GNode *node)
{
	GtkWidget *combo = gtk_combo_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "case_sensitive"))
				gtk_combo_set_case_sensitive(GTK_COMBO(combo), content[0]=='T');
			break;
		case 'i':
			if (!strcmp(info->name, "items") && content) {
				char *pos = content;
				char *items_end = &content[strlen(content)];
				GList *item_list = NULL;
				while (pos < items_end) {
					gchar *item_end = strchr (pos, '\n');
					if (item_end == NULL)
						item_end = items_end;
					*item_end = '\0';
	  
					item_list = g_list_append(item_list, pos);
					pos = item_end + 1;
				}
				gtk_combo_set_popdown_strings(GTK_COMBO(combo), item_list);
			}
			break;
		case 'u':
			if (!strcmp(info->name, "use_arrows"))
				gtk_combo_set_use_arrows(GTK_COMBO(combo), content[0] == 'T');
			else if (!strcmp(info->name, "use_arrows_always"))
				gtk_combo_set_use_arrows_always(GTK_COMBO(combo), content[0] == 'T');
			break;
		}

		if (content)
			free (content);
	}

	return combo;
}

static GtkWidget *
list_new(GladeXML *xml, GNode *node)
{
	GtkWidget *list = gtk_list_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "selection_mode"))
			gtk_list_set_selection_mode(GTK_LIST(list),
						    glade_enum_from_string(GTK_TYPE_SELECTION_MODE,
									   content));
		if (content)
			free(content);
	}
	return list;
}

static GtkWidget *
clist_new(GladeXML *xml, GNode *node) {
	GtkWidget *clist;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int cols = 1;
	GtkPolicyType hpol = GTK_POLICY_ALWAYS, vpol = GTK_POLICY_ALWAYS;

	for (; info; info = info->next)
		if (!strcmp(info->name, "columns")) {
			char *content = xmlNodeGetContent(info);
			cols = strtol(content, NULL, 0);
			if (content) free(content);
			break;
		}

	clist = gtk_clist_new(cols);
	info = ((xmlNodePtr)node->data)->childs;
	cols = 0;
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "column_widths")) {
				char *pos = content;
				while (pos && *pos != '\0') {
					int width = strtol(pos, &pos, 0);
					if (*pos == ',') pos++;
					gtk_clist_set_column_width(GTK_CLIST(clist), cols++, width);
				}
			}
			break;
		case 'h':
			if (!strcmp(info->name, "hscrollbar_policy"))
				hpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE, content);
			break;
		case 's':
			if (!strcmp(info->name, "selection_mode"))
				gtk_clist_set_selection_mode(GTK_CLIST(clist),
							     glade_enum_from_string(GTK_TYPE_SELECTION_MODE,
										    content));
			else if (!strcmp(info->name, "shadow_type"))
				gtk_clist_set_shadow_type(GTK_CLIST(clist),
							  glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
										 content));
			else if (!strcmp(info->name, "show_titles")) {
				if (content[0] == 'T')
					gtk_clist_column_titles_show(GTK_CLIST(clist));
				else
					gtk_clist_column_titles_hide(GTK_CLIST(clist));
			}
			break;
		case 'v':
			if (!strcmp(info->name, "vscrollbar_policy"))
				vpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE,
							      content);
			break;
		}
		if (content)
			free(content);
	}
	/*gtk_clist_set_policy(GTK_CLIST(clist), hpol, vpol);*/
	return clist;
}

static GtkWidget *
ctree_new(GladeXML *xml, GNode *node)
{
	GtkWidget *ctree;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int cols = 1;
	GtkPolicyType hpol = GTK_POLICY_ALWAYS, vpol = GTK_POLICY_ALWAYS;

	for (; info; info = info->next)
		if (!strcmp(info->name, "columns")) {
			char *content = xmlNodeGetContent(info);
			cols = strtol(content, NULL, 0);
			if (content) free(content);
			break;
		}

	ctree = gtk_ctree_new(cols, 0);
	info = ((xmlNodePtr)node->data)->childs;
	cols = 0;
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "column_widths")) {
				char *pos = content;
				while (pos && *pos != '\0') {
					int width = strtol(pos, &pos, 0);
					if (*pos == ',') pos++;
					gtk_clist_set_column_width(GTK_CLIST(ctree), cols++, width);
				}
			}
			break;
		case 'h':
			if (!strcmp(info->name, "hscrollbar_policy"))
				hpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE,
							      content);
			break;
		case 's':
			if (!strcmp(info->name, "selection_mode"))
				gtk_clist_set_selection_mode(GTK_CLIST(ctree),
							     glade_enum_from_string(GTK_TYPE_SELECTION_MODE,
										    content));
			else if (!strcmp(info->name, "shadow_type"))
				gtk_clist_set_shadow_type(GTK_CLIST(ctree),
							  glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
										 content));
			else if (!strcmp(info->name, "show_titles")) {
				if (content[0] == 'T')
					gtk_clist_column_titles_show(GTK_CLIST(ctree));
				else
					gtk_clist_column_titles_hide(GTK_CLIST(ctree));
			}
			break;
		case 'v':
			if (!strcmp(info->name, "vscrollbar_policy"))
				vpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE,
							      content);
			break;
		}
		if (content)
			free(content);
	}
	/*gtk_clist_set_policy(GTK_CLIST(ctree), hpol, vpol);*/
	return ctree;
}

static GtkWidget *
tree_new(GladeXML *xml, GNode *node)
{
	GtkWidget *tree = gtk_tree_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "selection_mode"))
			gtk_tree_set_selection_mode(GTK_TREE(tree),
						    glade_enum_from_string(GTK_TYPE_SELECTION_MODE,
									   content));
		else if (!strcmp(info->name, "view_mode"))
			gtk_tree_set_view_mode(GTK_TREE(tree),
					       glade_enum_from_string(GTK_TYPE_TREE_VIEW_MODE,
								      content));
		else if (!strcmp(info->name, "view_line"))
			gtk_tree_set_view_lines(GTK_TREE(tree), content[0] == 'T');

		if (content)
			free(content);
	}
	return tree;
}

static GtkWidget *
spinbutton_new(GladeXML *xml, GNode *node)
{
	GtkWidget *spinbutton;
	GtkAdjustment *adj = glade_get_adjustment(node);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int climb_rate = 1, digits = 0;
	gboolean numeric = FALSE, snap = FALSE, wrap = FALSE;
	GtkSpinButtonUpdatePolicy update_policy = GTK_UPDATE_IF_VALID;
  
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "climb_rate"))
				climb_rate = strtol(content, NULL, 0);
			break;
		case 'd':
			if (!strcmp(info->name, "digits"))
				digits = strtol(content, NULL, 0);
			break;
		case 'n':
			if (!strcmp(info->name, "numeric"))
				numeric = content[0] == 'T';
			break;
		case 's':
			if (!strcmp(info->name, "snap"))
				snap = content[0] == 'T';
			break;
		case 'u':
			if (!strcmp(info->name, "update_policy"))
				update_policy = glade_enum_from_string(
					GTK_TYPE_SPIN_BUTTON_UPDATE_POLICY, content);
			break;
		case 'w':
			if (!strcmp(info->name, "wrap"))
				wrap = content[0] == 'T';
			break;
		}
		if (content)
			free(content);
	}

	spinbutton = gtk_spin_button_new(adj, climb_rate, digits);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), numeric);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spinbutton),update_policy);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spinbutton), snap);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spinbutton), wrap);
	return spinbutton;
}

static GtkWidget *
hscale_new(GladeXML *xml, GNode *node)
{
	GtkAdjustment *adj = glade_get_adjustment(node);
	GtkWidget *scale = gtk_hscale_new(adj);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'd':
			if (!strcmp(info->name, "digits"))
				gtk_scale_set_digits(GTK_SCALE(scale), strtol(content, NULL, 0));
			else if (!strcmp(info->name, "draw_value"))
				gtk_scale_set_draw_value(GTK_SCALE(scale), content[0]=='T');
			break;
		case 'p':
			if (!strcmp(info->name, "policy"))
				gtk_range_set_update_policy(GTK_RANGE(scale),
							    glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
										   content));
			break;
		case 'v':
			if (!strcmp(info->name, "value_pos"))
				gtk_scale_set_value_pos(GTK_SCALE(scale),
							glade_enum_from_string(GTK_TYPE_POSITION_TYPE,
									       content));
			break;
		}
		if (content)
			free (content);
	}
	return scale;
}

static GtkWidget *
vscale_new (GladeXML *xml, GNode *node)
{
	GtkAdjustment *adj = glade_get_adjustment(node);
	GtkWidget *scale = gtk_vscale_new(adj);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'd':
			if (!strcmp(info->name, "digits"))
				gtk_scale_set_digits(GTK_SCALE(scale), strtol(content, NULL, 0));
			else if (!strcmp(info->name, "draw_value"))
				gtk_scale_set_draw_value(GTK_SCALE(scale), content[0]=='T');
			break;
		case 'p':
			if (!strcmp(info->name, "policy"))
				gtk_range_set_update_policy(GTK_RANGE(scale),
							    glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
										   content));
			break;
		case 'v':
			if (!strcmp(info->name, "value_pos"))
				gtk_scale_set_value_pos(GTK_SCALE(scale),
							glade_enum_from_string(GTK_TYPE_POSITION_TYPE,
									       content));
			break;
		}
		if (content)
			free (content);
	}
	return scale;
}

static GtkWidget *
hruler_new(GladeXML *xml, GNode *node)
{
	GtkWidget *ruler = gtk_hruler_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gdouble lower = 0, upper = 10, pos = 0, max = 10;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'l':
			if (!strcmp(info->name, "lower"))
				lower = g_strtod(content, NULL);
			break;
		case 'm':
			if (!strcmp(info->name, "max"))
				max = g_strtod(content, NULL);
			else if (!strcmp(info->name, "metric"))
				gtk_ruler_set_metric(GTK_RULER(ruler),
						     glade_enum_from_string(GTK_TYPE_METRIC_TYPE,
									    content));
			break;
		case 'p':
			if (!strcmp(info->name, "pos"))
				pos = g_strtod(content, NULL);
			break;
		case 'u':
			if (!strcmp(info->name, "upper"))
				upper = g_strtod(content, NULL);
			break;
		}

		if (content)
			free(content);
	}
	gtk_ruler_set_range(GTK_RULER(ruler), lower, upper, pos, max);
	return ruler;
}

static GtkWidget *
vruler_new(GladeXML *xml, GNode *node)
{
	GtkWidget *ruler = gtk_vruler_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gdouble lower = 0, upper = 10, pos = 0, max = 10;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'l':
			if (!strcmp(info->name, "lower"))
				lower = g_strtod(content, NULL);
			break;
		case 'm':
			if (!strcmp(info->name, "max"))
				max = g_strtod(content, NULL);
			else if (!strcmp(info->name, "metric"))
				gtk_ruler_set_metric(GTK_RULER(ruler),
						     glade_enum_from_string(GTK_TYPE_METRIC_TYPE,
									    content));
			break;
		case 'p':
			if (!strcmp(info->name, "pos"))
				pos = g_strtod(content, NULL);
			break;
		case 'u':
			if (!strcmp(info->name, "upper"))
				upper = g_strtod(content, NULL);
			break;
		}
		if (content)
			free(content);
	}
	gtk_ruler_set_range(GTK_RULER(ruler), lower, upper, pos, max);
	return ruler;
}

static GtkWidget *
hscrollbar_new(GladeXML *xml, GNode *node)
{
	GtkAdjustment *adj = glade_get_adjustment(node);
	GtkWidget *scroll = gtk_hscrollbar_new(adj);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "policy"))
			gtk_range_set_update_policy(GTK_RANGE(scroll),
						    glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
									   content));
		if (content)
			free(content);
	}
	return scroll;
}

static GtkWidget *
vscrollbar_new(GladeXML *xml, GNode *node)
{
	GtkAdjustment *adj = glade_get_adjustment(node);
	GtkWidget *scroll = gtk_vscrollbar_new(adj);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "policy"))
			gtk_range_set_update_policy(GTK_RANGE(scroll),
						    glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
									   content));
		if (content)
			free(content);
	}
	return scroll;
}

static GtkWidget *
statusbar_new(GladeXML *xml, GNode *node)
{
	return gtk_statusbar_new();
}

static GtkWidget *
toolbar_new(GladeXML *xml, GNode *node)
{
	GtkWidget *tool;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkOrientation orient = GTK_ORIENTATION_HORIZONTAL;
	GtkToolbarStyle style = GTK_TOOLBAR_ICONS;
	int space_size = 5;
	gboolean tooltips = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'o':
			if (!strcmp(info->name, "orientation"))
				orient = glade_enum_from_string(GTK_TYPE_ORIENTATION,
								content);
			break;
		case 's':
			if (!strcmp(info->name, "space_size"))
				space_size = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "style"))
				style = glade_enum_from_string(GTK_TYPE_TOOLBAR_STYLE,
							       content);
			break;
		case 't':
			if (!strcmp(info->name, "tooltips"))
				tooltips = content[0] == 'T';
			break;
		}

		if (content)
			free(content);
	}
	tool = gtk_toolbar_new(orient, style);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(tool), space_size);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(tool), tooltips);
	return tool;
}

static GtkWidget *
progressbar_new(GladeXML *xml, GNode *node)
{
	return gtk_progress_bar_new();
}

static GtkWidget *
arrow_new(GladeXML *xml, GNode *node)
{
	GtkWidget *arrow;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkArrowType arrow_type = GTK_ARROW_RIGHT;
	GtkShadowType shadow_type = GTK_SHADOW_OUT;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "arrow_type"))
			arrow_type = glade_enum_from_string(GTK_TYPE_ARROW_TYPE,
							    content);
		else if (!strcmp(info->name, "shadow_type"))
			shadow_type = glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
							     content);
		if (content)
			free(content);
	}
	arrow = gtk_arrow_new(arrow_type, shadow_type);
	misc_set(GTK_MISC(arrow), node->data);
	return arrow;
}

static GtkWidget *
pixmap_new(GladeXML *xml, GNode *node)
{
	GtkWidget *pix;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GdkPixmap *pixmap; GdkBitmap *bitmap;
	char *filename = NULL;
  
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "filename")) {
			if (filename) g_free(filename);
			filename = g_strdup(content);
			break;
		}
		if (content)
			free(content);
	}
	pixmap = gdk_pixmap_colormap_create_from_xpm(
		NULL,
		gtk_widget_get_default_colormap(), &bitmap, NULL, filename);
	if (filename)
		g_free(filename);
	pix = gtk_pixmap_new(pixmap, bitmap);
	misc_set(GTK_MISC(pix), node->data);

	return pix;
}

static GtkWidget *
drawingarea_new (GladeXML *xml, GNode *node)
{
	return gtk_drawing_area_new();
}

static GtkWidget *
hseparator_new(GladeXML *xml, GNode *node)
{
	return gtk_hseparator_new();
}

static GtkWidget *
vseparator_new(GladeXML *xml, GNode *node)
{
	return gtk_vseparator_new();
}

static GtkWidget *
menubar_new(GladeXML *xml, GNode *node)
{
	return gtk_menu_bar_new();
}

static GtkWidget *
menu_new(GladeXML *xml, GNode *node)
{
	return gtk_menu_new();
}

static GtkWidget *
menuitem_new(GladeXML *xml, GNode *node)
{
	GtkWidget *menuitem;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	gboolean right = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (label) g_free(label);
			label = g_strdup(content);
		} else if (!strcmp(info->name, "right_justify"))
			right = content[0] == 'T';

		if (content)
			free(content);
	}
	if (label) {
		menuitem = gtk_menu_item_new_with_label(label);
		g_free(label);
	} else
		menuitem = gtk_menu_item_new();

	if (right)
		gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));
	return menuitem;
}

static GtkWidget *
checkmenuitem_new(GladeXML *xml, GNode *node)
{
	GtkWidget *menuitem;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	gboolean right = FALSE, active = FALSE, toggle = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (label) g_free(label);
			label = g_strdup(content);
		} else if (!strcmp(info->name, "right_justify"))
			right = content[0] == 'T';
		else if (!strcmp(info->name, "active"))
			active = content[0] == 'T';
		else if (!strcmp(info->name, "always_show_toggle"))
			toggle = content[0] == 'T';

		if (content)
			free(content);
	}
	menuitem = gtk_check_menu_item_new_with_label(label);
	g_free (label);
	if (right)
		gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));
	gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menuitem), active);
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menuitem), toggle);

	return menuitem;
}

static GtkWidget *
radiomenuitem_new(GladeXML *xml, GNode *node)
{
	GtkWidget *menuitem;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	gboolean right = FALSE, active = FALSE, toggle = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		if (!strcmp(info->name, "label")) {
			if (label) g_free(label);
			label = g_strdup(content);
		} else if (!strcmp(info->name, "right_justify"))
			right = content[0] == 'T';
		else if (!strcmp(info->name, "active"))
			active = content[0] == 'T';
		else if (!strcmp(info->name, "always_show_toggle"))
			toggle = content[0] == 'T';
		if (content)
			free(content);
	}

	/* XXXX -- must do something about radio item groups ... */
	menuitem = gtk_radio_menu_item_new_with_label(NULL, label);
	g_free(label);

	if (right)
		gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));
	gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menuitem), active);
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menuitem), toggle);

	return menuitem;
}

static GtkWidget *
hbox_new(GladeXML *xml, GNode *node)
{
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int spacing = 0;
	gboolean homog = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "homogeneous"))
			homog = content[0] == 'T';
		else if (!strcmp(info->name, "spacing"))
			spacing = strtol(content, NULL, 0);

		if (content)
			free (content);
	}
	return gtk_hbox_new (homog, spacing);
}

static GtkWidget *vbox_new (GladeXML *xml, GNode *node)
{
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int spacing = 0;
	gboolean homog = FALSE;

	for (; info; info = info->next) {
		char *content =  xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "homogeneous"))
			homog = content[0] == 'T';
		else if (!strcmp(info->name, "spacing"))
			spacing = strtol(content, NULL, 0);

		if (content)
			free (content);
	}
	return gtk_vbox_new(homog, spacing);
}

static GtkWidget *
table_new(GladeXML *xml, GNode *node)
{
	GtkWidget *table;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int rows = 1, cols = 1, rspace = 0, cspace = 0;
	gboolean homog = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "columns"))
				cols = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "col_spacing"))
				cspace = strtol(content, NULL, 0);
			break;
		case 'h':
			if (!strcmp(info->name, "homogeneous"))
				homog = content[0] == 'T';
			break;
		case 'r':
			if (!strcmp(info->name, "rows"))
				rows = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "row_spacing"))
				rspace = strtol(content, NULL, 0);
			break;
		}
		if (content)
			free(content);
	}

	table = gtk_table_new(rows, cols, homog);
	gtk_table_set_row_spacings(GTK_TABLE(table), rspace);
	gtk_table_set_col_spacings(GTK_TABLE(table), cspace);
	return table;
}

static GtkWidget *
fixed_new(GladeXML *xml, GNode *node)
{
	return gtk_fixed_new();
}

static GtkWidget *
hbuttonbox_new(GladeXML *xml, GNode *node)
{
	GtkWidget *bbox = gtk_hbutton_box_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int minw, minh, ipx, ipy;

	gtk_button_box_get_child_size_default(&minw, &minh);
	gtk_button_box_get_child_ipadding_default(&ipx, &ipy);

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "child_min_width"))
				minw = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_min_height"))
				minh = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_ipad_x"))
				ipx = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_ipad_y"))
				ipy = strtol(content, NULL, 0);
			break;
		case 'l':
			if (!strcmp(info->name, "layout_style"))
				gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox),
							  glade_enum_from_string(GTK_TYPE_BUTTON_BOX_STYLE,
										 content));
			break;
		case 's':
			if (!strcmp(info->name, "spacing"))
				gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox),
							   strtol(content, NULL, 0));
			break;
		}

		if (content)
			free(content);
	}
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), minw, minh);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(bbox), ipx, ipy);
	return bbox;
}

static GtkWidget *
vbuttonbox_new(GladeXML *xml, GNode *node)
{
	GtkWidget *bbox = gtk_vbutton_box_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	int minw, minh, ipx, ipy;

	gtk_button_box_get_child_size_default(&minw, &minh);
	gtk_button_box_get_child_ipadding_default(&ipx, &ipy);

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'c':
			if (!strcmp(info->name, "child_min_width"))
				minw = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_min_height"))
				minh = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_ipad_x"))
				ipx = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "child_ipad_y"))
				ipy = strtol(content, NULL, 0);
			break;
		case 'l':
			if (!strcmp(info->name, "layout_style"))
				gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox),
							  glade_enum_from_string(GTK_TYPE_BUTTON_BOX_STYLE,
										 content));
			break;
		case 's':
			if (!strcmp(info->name, "spacing"))
				gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox),
							   strtol(content, NULL, 0));
			break;
		}

		if (content)
			free(content);
	}
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), minw, minh);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(bbox), ipx, ipy);
	return bbox;
}

static GtkWidget *
frame_new(GladeXML *xml, GNode *node)
{
	GtkWidget *frame;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	gdouble label_xalign = 0;
	GtkShadowType shadow_type = GTK_SHADOW_ETCHED_IN;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		switch (info->name[0]) {
		case 'l':
			if (!strcmp(info->name, "label")) {
				if (label) g_free(label);
				label = g_strdup(content);
			} else if (!strcmp(info->name, "label_xalign"))
				label_xalign = g_strtod(content, NULL);
			break;
		case 's':
			if (!strcmp(info->name, "shadow_type"))
				shadow_type = glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
								     content);
			break;
		}
		if (content)
			free(content);
	}
	frame = gtk_frame_new(label);
	if (label)
		g_free(label);
	gtk_frame_set_label_align(GTK_FRAME(frame), label_xalign, 0.5);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);

	return frame;
}

static GtkWidget *
aspectframe_new(GladeXML *xml, GNode *node)
{
	GtkWidget *frame;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	gdouble label_xalign = 0, xalign = 0, yalign = 0, ratio = 1;
	GtkShadowType shadow_type = GTK_SHADOW_ETCHED_IN;
	gboolean obey_child = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'l':
			if (!strcmp(info->name, "label")) {
				if (label) g_free(label);
				label = g_strdup(content);
			} else if (!strcmp(info->name, "label_xalign"))
				label_xalign = g_strtod(content, NULL);
			break;
		case 'o':
			if (!strcmp(info->name, "obey_child"))
				obey_child = content[0] == 'T';
			break;
		case 'r':
			if (!strcmp(info->name, "ratio"))
				ratio = g_strtod(content, NULL);
			break;
		case 's':
			if (!strcmp(info->name, "shadow_type"))
				shadow_type = glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
								     content);
			break;
		case 'x':
			if (!strcmp(info->name, "xalign"))
				xalign = g_strtod(content, NULL);
			break;
		case 'y':
			if (!strcmp(info->name, "yalign"))
				yalign = g_strtod(content, NULL);
			break;
		}

		if (content)
			free(content);
	}
	frame = gtk_aspect_frame_new(label, xalign, yalign, ratio, obey_child);
	gtk_frame_set_label_align(GTK_FRAME(frame), label_xalign, 0.5);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
	return frame;
}

static GtkWidget *
hpaned_new(GladeXML *xml, GNode *node)
{
	GtkWidget *paned = gtk_hpaned_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "handle_size"))
			gtk_paned_set_handle_size(GTK_PANED(paned), g_strtod(content, NULL));
		else if (!strcmp(info->name, "gutter_size"))
			gtk_paned_set_gutter_size(GTK_PANED(paned), g_strtod(content, NULL));

		if (content)
			free (content);
	}
	return paned;
}

static GtkWidget *
vpaned_new(GladeXML *xml, GNode *node)
{
	GtkWidget *paned = gtk_vpaned_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "handle_size"))
			gtk_paned_set_handle_size(GTK_PANED(paned), g_strtod(content, NULL));
		else if (!strcmp(info->name, "gutter_size"))
			gtk_paned_set_gutter_size(GTK_PANED(paned), g_strtod(content, NULL));

		if (content)
			free(content);
	}
	return paned;
}

static GtkWidget *
handlebox_new(GladeXML *xml, GNode *node)
{
	return gtk_handle_box_new();
}

static GtkWidget *
notebook_new(GladeXML *xml, GNode *node)
{
	GtkWidget *notebook = gtk_notebook_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "popup_enable")) {
			if (content[0] == 'T')
				gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
			else
				gtk_notebook_popup_disable(GTK_NOTEBOOK(notebook));
		} else if (!strcmp(info->name, "scrollable"))
			gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook),
						    content[0] == 'T');
		else if (!strcmp(info->name, "show_border"))
			gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook),
						     content[0] == 'T');
		else if (!strcmp(info->name, "show_tabs"))
			gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),
						   content[0] == 'T');
		else if (!strcmp(info->name, "tab_pos"))
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook),
						 glade_enum_from_string(GTK_TYPE_POSITION_TYPE,
									content));
		if (content)
			free (content);
	}
	return notebook;
}

static GtkWidget *
alignment_new (GladeXML *xml, GNode *node)
{
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gdouble xalign = 0.5, yalign = 0.5, xscale = 0, yscale = 0;
  
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "xalign"))
			xalign = g_strtod(content, NULL);
		else if (!strcmp(info->name, "xscale"))
			xscale = g_strtod(content, NULL);
		else if (!strcmp(info->name, "yalign"))
			yalign = g_strtod(content, NULL);
		else if (!strcmp(info->name, "yscale"))
			yscale = g_strtod(content, NULL);

		if (content)
			free(content);
	}
	return gtk_alignment_new(xalign, yalign, xscale, yscale);
}

static GtkWidget *
eventbox_new(GladeXML *xml, GNode *node)
{
	return gtk_event_box_new();
}

static GtkWidget *
scrolledwindow_new (GladeXML *xml, GNode *node)
{
	GtkWidget *win = gtk_scrolled_window_new(NULL, NULL);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkPolicyType hpol = GTK_POLICY_ALWAYS, vpol = GTK_POLICY_ALWAYS;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "hscrollbar_policy"))
			hpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE,
						      content);
		else if (!strcmp(info->name, "hupdate_policy"))
			gtk_range_set_update_policy(
				GTK_RANGE(GTK_SCROLLED_WINDOW(win)->hscrollbar),
				glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
						       content));
		else if (!strcmp(info->name, "shadow_type"))
			gtk_viewport_set_shadow_type(GTK_VIEWPORT(win),
						     glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
									    content));
		else if (!strcmp(info->name, "vscrollbar_policy"))
			vpol = glade_enum_from_string(GTK_TYPE_POLICY_TYPE,
						      content);
		else if (!strcmp(info->name, "vupdate_policy"))
			gtk_range_set_update_policy(
				GTK_RANGE(GTK_SCROLLED_WINDOW(win)->vscrollbar),
				glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
						       content));

		if (content)
			free(content);
	}
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win), hpol, vpol);
	return win;
}

static GtkWidget *
viewport_new(GladeXML *xml, GNode *node)
{
	GtkWidget *port = gtk_viewport_new(NULL, NULL);
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);
		if (!strcmp(info->name, "shadow_type"))
			gtk_viewport_set_shadow_type(GTK_VIEWPORT(port),
						     glade_enum_from_string(GTK_TYPE_SHADOW_TYPE,
									    content));
		if (content)
			free(content);
	}
	return port;
}

static GtkWidget *
curve_new(GladeXML *xml, GNode *node)
{
	GtkWidget *curve = gtk_curve_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gdouble minx=0, miny=0, maxx=1, maxy=1;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "curve_type"))
			gtk_curve_set_curve_type(GTK_CURVE(curve),
						 glade_enum_from_string(GTK_TYPE_CURVE_TYPE,
									content));
		else if (!strcmp(info->name, "min_x"))
			minx = g_strtod(content, NULL);
		else if (!strcmp(info->name, "min_y"))
			miny = g_strtod(content, NULL);
		else if (!strcmp(info->name, "max_x"))
			maxx = g_strtod(content, NULL);
		else if (!strcmp(info->name, "max_y"))
			maxy = g_strtod(content, NULL);

		if (content)
			free (content);
	}
	gtk_curve_set_range(GTK_CURVE(curve), minx, maxx, miny, maxy);
	return curve;
}

static GtkWidget *
gammacurve_new(GladeXML *xml, GNode *node)
{
	GtkWidget *gamma = gtk_gamma_curve_new();
	GtkWidget *curve = GTK_GAMMA_CURVE(gamma)->curve;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gdouble minx=0, miny=0, maxx=1, maxy=1;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "curve_type"))
			gtk_curve_set_curve_type(GTK_CURVE(curve),
						 glade_enum_from_string(GTK_TYPE_CURVE_TYPE,
									content));
		else if (!strcmp(info->name, "min_x"))
			minx = g_strtod(content, NULL);
		else if (!strcmp(info->name, "min_y"))
			miny = g_strtod(content, NULL);
		else if (!strcmp(info->name, "max_x"))
			maxx = g_strtod(content, NULL);
		else if (!strcmp(info->name, "max_y"))
			maxy = g_strtod(content, NULL);
		if (content)
			free(content);
	}
	gtk_curve_set_range(GTK_CURVE(curve), minx, maxx, miny, maxy);
	return gamma;
}

static GtkWidget *
colorselection_new(GladeXML *xml, GNode *node)
{
	GtkWidget *csel = gtk_color_selection_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "policy"))
			gtk_color_selection_set_update_policy(GTK_COLOR_SELECTION(csel),
							      glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
										     content));
		if (content)
			free (content);
	}
	return csel;
}

static GtkWidget *
preview_new(GladeXML *xml, GNode *node)
{
	GtkWidget *preview;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkPreviewType type = GTK_PREVIEW_COLOR;
	gboolean expand = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "expand"))
			expand = content[0] == 'T';
		else if (!strcmp(info->name, "type"))
			type = glade_enum_from_string(GTK_TYPE_PREVIEW_TYPE,
						      content);
		if (content)
			free (content);
	}
	preview = gtk_preview_new(type);
	gtk_preview_set_expand(GTK_PREVIEW(preview), expand);
	return preview;
}

static GtkWidget *
window_new (GladeXML *xml, GNode *node)
{
	GtkWidget *win;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gint xpos = -1, ypos = -1;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;
	GtkWindowPosition pos = GTK_WIN_POS_NONE;
	GtkWindowType type = GTK_WINDOW_TOPLEVEL;
	char *title = NULL;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'a':
			if (!strcmp(info->name, "allow_grow"))
				allow_grow = content[0] == 'T';
			else if (!strcmp(info->name, "allow_shrink"))
				allow_shrink = content[0] == 'T';
			else if (!strcmp(info->name, "auto_shrink"))
				auto_shrink = content[0] == 'T';
			break;
		case 'p':
			if (!strcmp(info->name, "position"))
				pos = glade_enum_from_string(GTK_TYPE_WINDOW_POSITION,
							     content);
			break;
		case 't':
			if (!strcmp(info->name, "title")) {
				if (title) g_free(title);
				title = g_strdup(content);
			} else if (!strcmp(info->name, "type"))
				type = glade_enum_from_string(GTK_TYPE_WINDOW_TYPE,
							      content);
			break;
		case 'x':
			if (info->name[1] == '\0') xpos = strtol(content, NULL, 0);
			break;
		case 'y':
			if (info->name[1] == '\0') ypos = strtol(content, NULL, 0);
			break;
		}

		if (content)
			free(content);
	}
	win = gtk_window_new(type);
	gtk_window_set_title(GTK_WINDOW(win), title);
	gtk_window_set_position(GTK_WINDOW(win), pos);
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink,allow_grow,auto_shrink);

	if (xpos >= 0 || ypos >= 0)
		gtk_widget_set_uposition(win, xpos, ypos);

	return win;
}

static GtkWidget *
dialog_new(GladeXML *xml, GNode *node)
{
	GtkWidget *win = gtk_dialog_new();
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gint xpos = -1, ypos = -1;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'a':
			if (!strcmp(info->name, "allow_grow"))
				allow_grow = content[0] == 'T';
			else if (!strcmp(info->name, "allow_shrink"))
				allow_shrink = content[0] == 'T';
			else if (!strcmp(info->name, "auto_shrink"))
				auto_shrink = content[0] == 'T';
			break;
		case 'p':
			if (!strcmp(info->name, "position"))
				gtk_window_set_position(GTK_WINDOW(win),
							glade_enum_from_string(GTK_TYPE_WINDOW_POSITION,
									       content));
			break;
		case 't':
			if (!strcmp(info->name, "title"))
				gtk_window_set_title(GTK_WINDOW(win), content);
			break;
		case 'x':
			if (info->name[1] == '\0') xpos = strtol(content, NULL, 0);
			break;
		case 'y':
			if (info->name[1] == '\0') ypos = strtol(content, NULL, 0);
			break;
		}

		if (content)
			free(content);
	}
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink,allow_grow,auto_shrink);

	if (xpos >= 0 || ypos >= 0)
		gtk_widget_set_uposition (win, xpos, ypos);

	return win;
}

static GtkWidget *
colorselectiondialog_new (GladeXML *xml, GNode *node)
{
	/* XXXX - fix this */
	return gtk_color_selection_dialog_new("ColorSel");
}

static GtkWidget *
custom_new (GladeXML *xml, GNode *node) {
	typedef GtkWidget *(* create_func)(gchar *name,
					   gchar *string1, gchar *string2,
					   gint int1, gint int2);
	GtkWidget *wid = NULL;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *name=NULL, *func_name = NULL, *string1 = NULL, *string2 = NULL;
	gint int1 = 0, int2 = 0;
	create_func func;
	GModule *allsymbols;

	if (!g_module_supported()) {
		g_error("custom_new requires gmodule to work correctly");
		return NULL;
	}

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "name")) {
			if (name) g_free(name);
			name = g_strdup(content);
		} else if (!strcmp(info->name, "creation_function")) {
			if (func_name) g_free(func_name);
			func_name = g_strdup(content);
		} else if (!strcmp(info->name, "string1")) {
			if (string1) g_free(string1);
			string1 = g_strdup(content);
		} else if (!strcmp(info->name, "string2")) {
			if (string2) g_free(string2);
			string2 = g_strdup(content);
		} else if (!strcmp(info->name, "int1"))
			int1 = strtol(content, NULL, 0);
		else if (!strcmp(info->name, "int2"))
			int2 = strtol(content, NULL, 0);
		if (content)
			free(content);
	}
	allsymbols = g_module_open(NULL, 0);
	if (g_module_symbol(allsymbols, func_name, (gpointer *)&func))
		wid = func(name, string1, string2, int1, int2);
	else
		g_warning("could not func widget creation function");
	if (name) g_free(name);
	if (func_name) g_free(func_name);
	if (string1) g_free(string1);
	if (string2) g_free(string2);
	return wid;
}

static const GladeWidgetBuildData widget_data[] = {
	/* general widgets */
	{"GtkLabel",         label_new,         NULL},
	{"GtkAccelLabel",    accellabel_new,    NULL},
	{"GtkEntry",         entry_new,         NULL},
	{"GtkText",          text_new,          NULL},
	{"GtkButton",        button_new,        glade_standard_build_children},
	{"GtkToggleButton",  togglebutton_new,  glade_standard_build_children},
	{"GtkCheckButton",   checkbutton_new,   glade_standard_build_children},
	{"GtkRadioButton",   radiobutton_new,   glade_standard_build_children},
	{"GtkOptionMenu",    optionmenu_new,    NULL},
	{"GtkCombo",         combo_new,         NULL},
	{"GtkList",          list_new,          NULL}, /* XXXX list appends ? */
	{"GtkCList",         clist_new,         clist_build_children},
	{"GtkCTree",         ctree_new,         clist_build_children},
	{"GtkTree",          tree_new,          NULL},
	{"GtkSpinButton",    spinbutton_new,    NULL},
	{"GtkHScale",        hscale_new,        NULL},
	{"GtkVScale",        vscale_new,        NULL},
	{"GtkHRuler",        hruler_new,        NULL},
	{"GtkVRuler",        vruler_new,        NULL},
	{"GtkHScrollbar",    hscrollbar_new,    NULL},
	{"GtkVScrollbar",    vscrollbar_new,    NULL},
	{"GtkStatusbar",     statusbar_new,     NULL},
	{"GtkToolbar",       toolbar_new,       NULL},
	{"GtkProgressBar",   progressbar_new,   NULL},
	{"GtkArrow",         arrow_new,         NULL},
	/* {"GtkImage",         image_new,         NULL}, */
	{"GtkPixmap",        pixmap_new,        NULL},
	{"GtkDrawingArea",   drawingarea_new,   NULL},
	{"GtkHSeparator",    hseparator_new,    NULL},
	{"GtkVSeparator",    vseparator_new,    NULL},

	/* menu widgets */
	{"GtkMenuBar",       menubar_new,       menushell_build_children},
	{"GtkMenu",          menu_new,          menushell_build_children},
	{"GtkMenuItem",      menuitem_new,      menuitem_build_children},
	{"GtkCheckMenuItem", checkmenuitem_new, menuitem_build_children},
	{"GtkRadioMenuItem", radiomenuitem_new, menuitem_build_children},

	/* container widgets */
	{"GtkHBox",          hbox_new,          box_build_children},
	{"GtkVBox",          vbox_new,          box_build_children},
	{"GtkTable",         table_new,         table_build_children},
	{"GtkFixed",         fixed_new,         fixed_build_children},
	{"GtkHButtonBox",    hbuttonbox_new,    glade_standard_build_children},
	{"GtkVButtonBox",    vbuttonbox_new,    glade_standard_build_children},
	{"GtkFrame",         frame_new,         glade_standard_build_children},
	{"GtkAspectFrame",   aspectframe_new,   glade_standard_build_children},
	{"GtkHPaned",        hpaned_new,        paned_build_children},
	{"GtkVPaned",        vpaned_new,        paned_build_children},
	{"GtkHandleBox",     handlebox_new,     glade_standard_build_children},
	{"GtkNotebook",      notebook_new,      notebook_build_children},
	{"GtkAlignment",     alignment_new,     glade_standard_build_children},
	{"GtkEventBox",      eventbox_new,      glade_standard_build_children},
	{"GtkScrolledWindow",scrolledwindow_new,glade_standard_build_children},
	{"GtkViewport",      viewport_new,      glade_standard_build_children},

  /* other widgets */
	{"GtkCurve",         curve_new,         NULL},
	{"GtkGammaCurve",    gammacurve_new,    NULL},
	{"GtkColorSelection",colorselection_new,NULL},
	{"GtkPreview",       preview_new,       NULL},

  /* toplevel widgets */
	{"GtkWindow",        window_new,        glade_standard_build_children},
	{"GtkDialog",        dialog_new,        dialog_build_children},
	{"GtkColorSelectionDialog", colorselectiondialog_new, NULL},

  /* the custom widget */
	{"Custom",           custom_new,        NULL},
	{NULL, NULL, NULL}
};

void glade_init_gtk_widgets(void) {
	glade_register_widgets(widget_data);
}


