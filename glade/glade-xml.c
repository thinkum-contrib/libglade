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

#include <stdlib.h>
#include <string.h>
#include <glade/glade-xml.h>
#include <glade/glade-build.h>
#include <glade/glade-private.h>
#include <gmodule.h>
#include <gtk/gtkobject.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtklabel.h>

static const char *glade_xml_tag = "GladeXML::";
static const char *glade_xml_name_tag = "GladeXML::name";
static const char *glade_xml_longname_tag = "GladeXML::longname";

static void glade_xml_init(GladeXML *xml);
static void glade_xml_class_init(GladeXMLClass *klass);

static GtkObjectClass *parent_class;
static void glade_xml_destroy(GtkObject *object);

static void glade_xml_build_interface(GladeXML *xml, GladeTreeData *tree,
				      const char *root);

/**
 * glade_xml_get_type:
 *
 * Creates the typecode for the GladeXML object type.
 *
 * Returns: the typecode for the GladeXML object type.
 */
GtkType
glade_xml_get_type(void)
{
	static GtkType xml_type = 0;

	if (!xml_type) {
		GtkTypeInfo xml_info = {
			"GladeXML",
			sizeof(GladeXML),
			sizeof(GladeXMLClass),
			(GtkClassInitFunc) glade_xml_class_init,
			(GtkObjectInitFunc) glade_xml_init,
			NULL,
			NULL
		};
		xml_type = gtk_type_unique(gtk_data_get_type(), &xml_info);
	}
	return xml_type;
}

static void
glade_xml_class_init (GladeXMLClass *klass)
{
	parent_class = gtk_type_class(gtk_data_get_type());

	GTK_OBJECT_CLASS(klass)->destroy = glade_xml_destroy;
}

static void
glade_xml_init (GladeXML *self)
{
	GladeXMLPrivate *priv;
	
	self->priv = priv = g_new (GladeXMLPrivate, 1);

	self->filename = NULL;
	priv->tooltips = NULL;
	priv->name_hash = g_hash_table_new(g_str_hash, g_str_equal);
	priv->longname_hash = g_hash_table_new(g_str_hash, g_str_equal);
	priv->signals = g_hash_table_new(g_str_hash, g_str_equal);
	priv->radio_groups = g_hash_table_new (g_str_hash, g_str_equal);
}

/**
 * glade_xml_new:
 * @fname: the XML file name.
 * @root: the widget node in @fname to start building from (or %NULL)
 *
 * Creates a new GladeXML object (and the corresponding widgets) from the
 * XML file @fname.  Optionally it will only build the interface from the
 * widget node @root (if it is not %NULL).  This feature is useful if you
 * only want to build say a toolbar or menu from the XML file, but not the
 * window it is embedded in.  Note also that the XML parse tree is cached
 * to speed up creating another GladeXML object for the same file
 *
 * Returns: the newly created GladeXML object, or NULL on failure.
 */
GladeXML *
glade_xml_new(const char *fname, const char *root)
{
	GladeXML *self = gtk_type_new(glade_xml_get_type());

	if (!glade_xml_construct(self, fname, root)) {
		gtk_object_destroy(GTK_OBJECT(self));
		return NULL;
	}
	return self;
}

/**
 * glade_xml_construct:
 * @self: the GladeXML object
 * @fname: the XML filename
 * @root: the root widget node (or %NULL for none)
 *
 * This routine can be used by bindings (such as gtk--) to help construct
 * a GladeXML object, if it is needed.
 *
 * Returns: TRUE if the construction succeeded.
 */
gboolean
glade_xml_construct (GladeXML *self, const char *fname, const char *root)
{
	GladeTreeData *tree = glade_tree_get(fname);

	if (!tree)
		return FALSE;

	if (self->filename)
		g_free(self->filename);
	self->filename = g_strdup(fname);
	glade_xml_build_interface(self, tree, root);

	if (self->priv->tooltips)
		gtk_tooltips_enable(self->priv->tooltips);

	return TRUE;
}

/**
 * glade_xml_signal_connect:
 * @self: the GladeXML object
 * @handlername: the signal handler name
 * @func: the signal handler function
 *
 * In the glade interface descriptions, signal handlers are specified for
 * widgets by name.  This function allows you to connect a C function to
 * all signals in the GladeXML file with the given signal handler name.
 */
void
glade_xml_signal_connect (GladeXML *self, const char *handlername,
			  GtkSignalFunc func)
{
	GList *signals = g_hash_table_lookup(self->priv->signals, signalname);
	
	for (; signals != NULL; signals = signals->next) {
		GladeSignalData *data = signals->data;

		if (data->connect_object) {
			GtkObject *other = g_hash_table_lookup(self->priv->name_hash,
							       data->connect_object);
			if (data->signal_after)
				gtk_signal_connect_object_after(data->signal_object, data->signal_name,
								func, other);
			else
				gtk_signal_connect_object(data->signal_object, data->signal_name,
							  func, other);
		} else {
			/* the signal_data argument is just a string, but may be helpful for
			 * someone */
			if (data->signal_after)
				gtk_signal_connect_after(data->signal_object, data->signal_name,
							 func, data->signal_data);
			else
				gtk_signal_connect(data->signal_object, data->signal_name,
						   func, data->signal_data);
		}
	}
}

static void
autoconnect_foreach(char *signal_handler, GList *signals,
		    GModule *allsymbols)
{
	GtkSignalFunc func;
	if (!g_module_symbol(allsymbols, signal_handler, (gpointer *)&func))
		g_warning("could not find signal handler '%s'.", signal_handler);
	else
		for (; signals != NULL; signals = signals->next) {
			GladeSignalData *data = signals->data;
			if (data->connect_object) {
				GladeXML *self = glade_get_widget_tree(
					GTK_WIDGET(data->signal_object));
				GtkObject *other = g_hash_table_lookup(self->priv->name_hash,
								       data->connect_object);
				if (data->signal_after)
					gtk_signal_connect_object_after(data->signal_object,
									data->signal_name, func, other);
				else
					gtk_signal_connect_object(data->signal_object, data->signal_name,
								  func, other);
			} else {
				/* the signal_data argument is just a string, but may be helpful for
				 * someone */
				if (data->signal_after)
					gtk_signal_connect_after(data->signal_object, data->signal_name,
								 func, data->signal_data);
				else
					gtk_signal_connect(data->signal_object, data->signal_name,
							   func, data->signal_data);
			}
		}
}

/**
 * glade_xml_signal_autoconnect:
 * @self: the GladeXML object.
 *
 * This function is a variation of glade_xml_signal_connect.  It uses
 * gmodule's introspective features (by openning the module %NULL) to
 * look at the application's symbol table.  From here it tries to match
 * the signal handler names given in the interface description with
 * symbols in the application and connects the signals.
 * 
 * Note that this function will not work correctly if gmodule is not
 * supported on the platform.
 */
void
glade_xml_signal_autoconnect (GladeXML *self)
{
	GModule *allsymbols;
	if (!g_module_supported())
		g_error("glade_xml_signal_autoconnect requires working gmodule");

	/* get a handle on the main executable -- use this to find symbols */
	allsymbols = g_module_open(NULL, 0);
	g_hash_table_foreach(self->priv->signals, (GHFunc)autoconnect_foreach, allsymbols);
}

/**
 * glade_xml_get_widget:
 * @self: the GladeXML object.
 * @name: the name of the widget.
 *
 * This function is used to get a pointer to the GtkWidget corresponding to
 * @name in the interface description.  You would use this if you have to do
 * anything to the widget after loading.
 *
 * Returns: the widget matching @name, or %NULL if none exists.
 */
GtkWidget *
glade_xml_get_widget (GladeXML *self, const char *name)
{
	return g_hash_table_lookup(self->priv->name_hash, name);
}

/**
 * glade_xml_get_widget_by_long_name:
 * @self: the GladeXML object.
 * @longname: the long name of the widget (eg toplevel.parent.widgetname).
 *
 * This function is used to get a pointer to the GtkWidget corresponding to
 * @longname in the interface description.  You would use this if you have
 * to do anything to the widget after loading.  This function differs from
 * glade_xml_get_widget, in that you have to give the long form of the
 * widget name, with all its parent widget names, separated by periods.
 *
 * Returns: the widget matching @longname, or %NULL if none exists.
 */
GtkWidget *
glade_xml_get_widget_by_long_name(GladeXML *self,
				  const char *longname)
{
	return g_hash_table_lookup(self->priv->longname_hash, longname);
}

/**
 * glade_get_widget_name:
 * @widget: the widget
 *
 * Used to get the name of a widget that was generated by a GladeXML object.
 *
 * Returns: the name of the widget.
 */
const char *
glade_get_widget_name(GtkWidget *widget)
{
	return (const char *)gtk_object_get_data(GTK_OBJECT(widget),
						 glade_xml_name_tag);
}

/**
 * glade_get_widget_long_name:
 * @widget: the widget
 *
 * Description:
 * Used to get the long name of a widget that was generated by a GladeXML
 * object.
 *
 * Returns: the long name of the widget.
 */
const char *
glade_get_widget_long_name (GtkWidget *widget)
{
	return (const char *)gtk_object_get_data(GTK_OBJECT(widget),
						 glade_xml_longname_tag);
}

/**
 * glade_get_widget_tree:
 * @widget: the widget
 *
 * Description:
 * This function is used to get the GladeXML object that built this widget.
 *
 * Returns: the GladeXML object that built this widget.
 */
GladeXML *
glade_get_widget_tree(GtkWidget *widget)
{
	return gtk_object_get_data(GTK_OBJECT(widget), glade_xml_tag);
}

/* this is a private function */
static void
glade_xml_add_signal(GladeXML *xml, GtkWidget *w, xmlNodePtr sig)
{
	GladeSignalData *data = g_new0(GladeSignalData, 1);
	char *signal_handler = NULL;
	GList *list;
	xmlNodePtr tmp;
	data->signal_object = GTK_OBJECT(w);
	for (tmp = sig->childs; tmp != NULL; tmp = tmp->next) {
		char *content = xmlNodeGetContent(tmp);
		switch (tmp->name[0]) {
		case 'n':
			if (!strcmp(tmp->name, "name"))
				data->signal_name = g_strdup(content);
			break;
		case 'h':
			if (!strcmp(tmp->name, "handler"))
				signal_handler = g_strdup(content);
			break;
		case 'o':
			if (!strcmp(tmp->name, "object"))
				data->connect_object = g_strdup(content);
			break;
		case 'a':
			if (!strcmp(tmp->name, "after"))
				data->signal_after = (content[0] == 'T');
			break;
		case 'd':
			if (!strcmp(tmp->name, "data"))
				data->signal_data = g_strdup(content);
		}
		if (content) free(content); /* don't use g_free -- mem wasn't g_malloc'd */
	}
	g_assert(signal_handler != NULL);
	g_assert(data->signal_name != NULL);
	debug(g_message("New signal: %s->%s", data->signal_name, signal_handler));
	list = g_hash_table_lookup(xml->priv->signals, signal_handler);
	list = g_list_prepend(list, data);
	g_hash_table_insert(xml->priv->signals, signal_handler, list);
	/* if list is longer than one element, then a signal for this handler has
	 * been inserted, so the string pointed to by signal_handler is not needed
	 */
	if (list->next != NULL)
		g_free(signal_handler);
}

static void
glade_xml_add_accel(GtkWidget *w, xmlNodePtr accel)
{
	guint key = 0;
	GdkModifierType modifiers = 0;
	char *signal = NULL;
	xmlNodePtr tmp;

	for (tmp = accel->childs; tmp != NULL; tmp = tmp->next) {
		char *content = xmlNodeGetContent(tmp);
		switch (tmp->name[0]) {
		case 'k':
			if (!strcmp(tmp->name, "key"))
				key = glade_key_get(content);
			break;
		case 'm':
			if (!strcmp(tmp->name, "modifiers")) {
				char *pos = content;
				while (pos[0]) {
					if (pos[0]=='G' && pos[1]=='D' && pos[2]=='K' && pos[3]=='_') {
						pos += 4;
						if (!strncmp(pos, "SHIFT_MASK", 10)) {
							modifiers |= GDK_SHIFT_MASK;
							pos += 10;
						} else if (!strncmp(pos, "LOCK_MASK", 9)) {
							modifiers |= GDK_LOCK_MASK;
							pos += 9;
						} else if (!strncmp(pos, "CONTROL_MASK", 12)) {
							modifiers |= GDK_CONTROL_MASK;
							pos += 12;
						} else if (!strncmp(pos, "MOD1_MASK", 9)) {
							modifiers |= GDK_MOD1_MASK;
							pos += 9;
						} else if (!strncmp(pos, "MOD2_MASK", 9)) {
							modifiers |= GDK_MOD2_MASK;
							pos += 9;
						} else if (!strncmp(pos, "MOD3_MASK", 9)) {
							modifiers |= GDK_MOD3_MASK;
							pos += 9;
						} else if (!strncmp(pos, "MOD4_MASK", 9)) {
							modifiers |= GDK_MOD4_MASK;
							pos += 9;
						} else if (!strncmp(pos, "MOD5_MASK", 9)) {
							modifiers |= GDK_MOD5_MASK;
							pos += 9;
						} else if (!strncmp(pos, "BUTTON1_MASK", 12)) {
							modifiers |= GDK_BUTTON1_MASK;
							pos += 12;
						} else if (!strncmp(pos, "BUTTON2_MASK", 12)) {
							modifiers |= GDK_BUTTON2_MASK;
							pos += 12;
						} else if (!strncmp(pos, "BUTTON3_MASK", 12)) {
							modifiers |= GDK_BUTTON3_MASK;
							pos += 12;
						} else if (!strncmp(pos, "BUTTON4_MASK", 12)) {
							modifiers |= GDK_BUTTON4_MASK;
							pos += 12;
						} else if (!strncmp(pos, "BUTTON5_MASK", 12)) {
							modifiers |= GDK_BUTTON5_MASK;
							pos += 12;
						} else if (!strncmp(pos, "RELEASE_MASK", 12)) {
							modifiers |= GDK_RELEASE_MASK;
							pos += 12;
						}
					} else
						pos++;
				}
			}
			break;
		case 's':
			if (!strcmp(tmp->name, "signal")) {
				if (signal) g_free(signal);
				signal = g_strdup(content);
			}
			break;
		}
		if (content) free(content);
	}
	g_return_if_fail(signal != NULL);
	debug(g_message("New Accel: key=%d,mod=%d -> %s:%s", key, modifiers,
			gtk_widget_get_name(w), signal));
	gtk_widget_add_accelerator(w, signal, gtk_accel_group_get_default(),
				   key, modifiers, GTK_ACCEL_VISIBLE);
	g_free(signal);
}

static void
glade_xml_destroy_signals(char *key, GList *signal_datas)
{
	GList *tmp;

	for (tmp = signal_datas; tmp; tmp = tmp->next) {
		GladeSignalData *data = tmp->data;
		if (data) {
			if (data->signal_name) g_free(data->signal_name);
			if (data->signal_data) g_free(data->signal_data);
			if (data->connect_object) g_free(data->connect_object);
			g_free(data);
		}
	}
	g_list_free(signal_datas);
	g_free(key);
}

static void
free_name (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
}

static void
glade_xml_destroy(GtkObject *object)
{
	GladeXML *self = GLADE_XML(object);
	GladeXMLPrivate *priv = self->priv;
	
	if (self->filename)
		g_free(self->filename);

	g_hash_table_destroy(priv->name_hash);
	g_hash_table_destroy(priv->longname_hash);

	g_hash_table_foreach(priv->signals, (GHFunc)glade_xml_destroy_signals, NULL);
	g_hash_table_destroy(priv->signals);

	g_hash_table_foreach (priv->radio_groups, free_name, NULL);
	g_hash_table_destroy (priv->radio_groups);
	
	g_free (self->priv);
	if (parent_class->destroy)
		(* parent_class->destroy)(object);
}

/**
 * glade_enum_from_string
 * @type: the GtkType for this flag or enum type.
 * @string: the string representation of the enum value.
 *
 * This helper routine is designed to be used by widget build routines to
 * convert the string representations of enumeration values found in the
 * XML descriptions to the integer values that can be used to configure
 * the widget.
 *
 * Returns: the integer value for this enumeration, or 0 if it couldn't be
 * found.
 */
gint
glade_enum_from_string (GtkType type, const char *string)
{
	GtkEnumValue *val = gtk_type_enum_find_value(type, string);

	if (val)
		return val->value;
	else
		return 0;
}

static void
glade_xml_build_interface(GladeXML *self, GladeTreeData *tree,
			  const char *root)
{
	GNode *root_node;

	if (root) {
		root_node = g_hash_table_lookup(tree->hash, root);
		g_return_if_fail(root_node != NULL);
		glade_xml_build_widget(self, root_node, NULL);
	} else {
		/* build all toplevel nodes */
		for (root_node = tree->tree->children; root_node != NULL;
		     root_node = g_node_next_sibling(root_node)) {
			glade_xml_build_widget(self, root_node, NULL);
		}
	}
}

/* below are functions from glade-build.h */

static GHashTable *widget_table = NULL;

/**
 * glade_register_widgets:
 * @widgets: a NULL terminated array of GladeWidgetBuildData structures.
 *
 * This function is used to register new sets of widget building functions.
 * each member of @widgets contains the widget name, a function to build
 * a widget of that type, and optionally a function to build the children
 * of this widget.  The child building routine would call
 * glade_xml_build_widget on each child node to create the child before
 * packing it.
 *
 * This function is mainly useful for addon widget modules for libglade
 * (it would get called from the glade_init_module function).
 */
void
glade_register_widgets(const GladeWidgetBuildData *widgets)
{
	int i = 0;

	if (!widget_table)
		widget_table = g_hash_table_new(g_str_hash, g_str_equal);
	while (widgets[i].name != NULL) {
		g_hash_table_insert(widget_table, widgets[i].name, (gpointer)&widgets[i]);
		i++;
	}
}

/**
 * GladeNewFunc
 * @xml: The GladeXML object.
 * @node: the GNode holding the xmlNode for this widget.
 *
 * This function signature should be used by functions that build particular
 * widget types.  The function should create the new widget and set any non
 * standard widget parameters (ie. don't set visibility, size, etc), as
 * this is handled by glade_xml_build_widget, which calls these functions.
 *
 * Returns: the new widget.
 */
/**
 * GladeBuildChildrenFunc
 * @xml: the GladeXML object.
 * @w: this widget.
 * @node: the GNode holding the xmlNode for this widget.
 * @longname: the long name for this widget.
 *
 * This function signature should be used by functions that are responsible
 * for adding children to a container widget.  To create each child widget,
 * glade_xml_build_widget should be called.  The GNode for the child widget
 * will be a child of this widget's GNode.
 */

/**
 * glade_xml_build_widget:
 * @self: the GladeXML object.
 * @node: the GNode holding the xmlNode of the child
 * @parent_long: the long name of the parent object.
 *
 * This function is not intended for people who just use libglade.  Instead
 * it is for people extending it (it is designed to be called in the child
 * build routine defined for the parent widget).  It first checks the type
 * of the widget from the class tag, then calls the corresponding widget
 * creation routine.  This routine sets up all the settings specific to that
 * type of widget.  Then general widget settings are performed on the widget.
 * Then it sets up accelerators for the widget, and extracts any signal
 * information for the widget.  Then it checks to see if there are any
 * child widget nodes for this widget, and if so calls the widget's
 * build routine, which will create the children with this function and add
 * them to the widget in the appropriate way.  Finally it returns the widget.
 * 
 * Returns: the newly created widget.
 */
GtkWidget *
glade_xml_build_widget(GladeXML *self, GNode *node,
		       const char *parent_long)
{
	xmlNodePtr xml = node->data, tmp;
	char *widget_class, *w_name = NULL, *w_longname, *w_style = NULL;
	gboolean visible = TRUE;
	GladeWidgetBuildData *data;
	GtkWidget *ret;

	if (!widget_table)
		widget_table = g_hash_table_new(g_str_hash, g_str_equal);
	tmp = glade_tree_find_node(xml, "class");
	widget_class = xmlNodeGetContent(tmp);
	debug(g_message("Widget class: %s (for parent %s)", widget_class,
			parent_long?parent_long:"(null)"));
	if (!strcmp(widget_class, "Placeholder")) {
		g_warning("placeholders exist in interface description");
		ret = gtk_label_new("[placeholder]");
		gtk_widget_show(ret);
		free(widget_class);
		return ret;
	}
	data = g_hash_table_lookup(widget_table, widget_class);
	if (data == NULL) {
		char buf[50];
		g_warning("unknown widget class '%s'", widget_class);
		g_snprintf(buf, 49, "[a %s]", widget_class);
		ret = gtk_label_new(buf);
		gtk_widget_show(ret);
		free(widget_class);
		return ret;
	}
	free(widget_class);
	g_assert(data->new);
	ret = data->new(self, node);

  /* set some common parameters that apply to all (or most) widgets */
	for (tmp = xml->childs; tmp != NULL; tmp = tmp->next) {
		const char *name = tmp->name;
		char *value = xmlNodeGetContent(tmp);
		if (name == NULL) continue;
		switch (name[0]) {
		case 'a':
			if (!strcmp(name, "accelerator"))
				glade_xml_add_accel(ret, tmp);
			break;
		case 'A': /* The old accelerator tag used 'Accelerator'
			   * rather than 'accelerator'. */
			if (!strcmp(name, "Accelerator"))
				glade_xml_add_accel(ret, tmp);
			break;
		case 'b':
			if (!strcmp(name, "border_width")) {
				long width = strtol(value, NULL, 0);
				gtk_container_set_border_width(GTK_CONTAINER(ret), width);
			}
			break;
		case 'c':
			if (!strcmp(name, "can_default")){
				if (*value == 'T')
					GTK_WIDGET_SET_FLAGS(ret, GTK_CAN_DEFAULT);
			} else if (!strcmp(name, "can_focus")){
				if (*value == 'T')
					GTK_WIDGET_SET_FLAGS(ret, GTK_CAN_FOCUS);
			}
			break;
		case 'e':
			if (!strcmp(name, "events")) {
				long events = strtol(value, NULL, 0);
				gtk_widget_set_events(ret, events);
			} else if (!strcmp(name, "extension_events")) {
				GdkExtensionMode ex =
					glade_enum_from_string(GTK_TYPE_GDK_EXTENSION_MODE, value);
				gtk_widget_set_extension_events(ret, ex);
			}
			break;
		case 'h':
			if (!strcmp(name, "height")) {
				long height = strtol(value, NULL, 0);
				gtk_widget_set_usize(ret, -1, height);
			}
			break;
		case 'n':
			if (!strcmp(name, "name")) {
				if (w_name) g_free(w_name);
				w_name = g_strdup(value);
			}
			break;
		case 's':
			if (!strcmp(name, "sensitive"))
				gtk_widget_set_sensitive(ret, *value == 'T');
			else if (!strcmp(name, "style_name")) {
				if (w_style) g_free(w_style);
				w_style = g_strdup(value);
			} else if (!strcmp(name, "signal"))
				glade_xml_add_signal(self, ret, tmp);
			break;
		case 'S': /* The old signal tag used 'Signal' rather than
			   * 'signal'. */
			if (!strcmp(name, "Signal"))
				glade_xml_add_signal(self, ret, tmp);
			break;
		case 't':
			if (!strcmp(name, "tooltip")) {
				if (self->priv->tooltips == NULL)
					self->priv->tooltips = gtk_tooltips_new();
				gtk_tooltips_set_tip(self->priv->tooltips, ret, value, NULL);
			}
			break;
		case 'v':
			if (!strcmp(name, "visible"))
				visible = (*value == 'T');
			break;
		case 'w':
			if (!strcmp(name, "width")) {
				long width = strtol(value, NULL, 0);
				gtk_widget_set_usize(ret, width, -1);
			}
			break;
		}
		if (value) free(value);
	}
	g_assert(w_name != NULL);
	gtk_widget_set_name(ret, w_name);
	if (parent_long)
		w_longname = g_strconcat(parent_long, ".", w_name, NULL);
	else
		w_longname = g_strdup(w_name);
	/* store this information as data of the widget.  w_longname is owned by
	 * the widget now */
	gtk_object_set_data(GTK_OBJECT(ret), glade_xml_tag, self);
	gtk_object_set_data_full(GTK_OBJECT(ret), glade_xml_name_tag,
				 w_name, (GtkDestroyNotify)g_free);
	gtk_object_set_data_full(GTK_OBJECT(ret), glade_xml_longname_tag,
				 w_longname, (GtkDestroyNotify)g_free);
	/* store widgets in hash table, for easy lookup */
	g_hash_table_insert(self->priv->name_hash, w_name, ret);
	g_hash_table_insert(self->priv->longname_hash, w_longname, ret);

	if (w_style) {
		glade_style_attach(ret, w_style);
		g_free(w_style);
	}

	if (data->build_children && node->children)
		data->build_children(self, ret, node, w_longname);
	if (visible)
		gtk_widget_show(ret);
	return ret;
}

/**
 * glade_default_build_children
 * @xml: the GladeXML object.
 * @w: the container widget.
 * @node: the node for this widget.
 * @longname: the long name for this widget.
 *
 * This is the standard child building function.  It simply calls
 * gtk_container_add on each child to add them to the parent.  It is
 * implemented here, as it should be useful to many GTK+ based widget
 * sets.
 */
void
glade_standard_build_children(GladeXML *xml, GtkWidget *w, GNode *node,
			     const char *longname)
{
	GNode *childnode;
	for (childnode = node->children; childnode;
	     childnode = childnode->next) {
		GtkWidget *child = glade_xml_build_widget(xml, childnode,
							  longname);
		gtk_container_add(GTK_CONTAINER(w), child);
	}
}

/**
 * glade_get_adjustment
 * @gnode: the XML node for the widget.
 *
 * This utility routine is used to create an adjustment object for a widget.
 */
GtkAdjustment *
glade_get_adjustment(GNode *gnode)
{
	xmlNodePtr node = gnode->data;
	gdouble hvalue=1, hlower=0, hupper=100, hstep=1, hpage=100, hpage_size=10;

	for (node = node->childs; node; node = node->next) {
		char *content = xmlNodeGetContent (node);

		if (node->name[0] == 'h')
			switch (node->name[1]) {
			case 'l':
				if (!strcmp(node->name, "hlower"))
					hlower = g_strtod(content, NULL);
				break;
			case 'p':
				if (!strcmp(node->name, "hpage"))
					hpage = g_strtod(content, NULL);
				else if (!strcmp(node->name, "hpage_size"))
					hpage_size = g_strtod(content, NULL);
				break;
			case 's':
				if (!strcmp(node->name, "hstep"))
					hstep = g_strtod(content, NULL);
				break;
			case 'u':
				if (!strcmp(node->name, "hupper"))
					hupper = g_strtod(content, NULL);
				break;
			case 'v':
				if (!strcmp(node->name, "hvalue"))
					hvalue = g_strtod(content, NULL);
				break;
			}
		if (content)
			free (content);
	}
	return GTK_ADJUSTMENT (gtk_adjustment_new(hvalue, hlower, hupper, hstep,
						  hpage, hpage_size));
}

