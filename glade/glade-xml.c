/* -*- Mode: C; c-basic-offset: 8 -*- */
/* libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998, 1999  James Henstridge <james@daa.com.au>
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

#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

static const char *glade_xml_tag = "GladeXML::";
static const char *glade_xml_name_tag = "GladeXML::name";
static const char *glade_xml_longname_tag = "GladeXML::longname";

static void glade_xml_init(GladeXML *xml);
static void glade_xml_class_init(GladeXMLClass *klass);

static GtkObjectClass *parent_class;
static void glade_xml_destroy(GtkObject *object);

static void glade_xml_build_interface(GladeXML *xml, GladeWidgetTree *tree,
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
	self->textdomain = NULL;
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

	if (!glade_xml_construct(self, fname, root, NULL)) {
		gtk_object_destroy(GTK_OBJECT(self));
		return NULL;
	}
	return self;
}

/**
 * glade_xml_new_with_domain:
 * @fname: the XML file name.
 * @root: the widget node in @fname to start building from (or %NULL)
 * @domain: the translation domain to use for this interface (or %NULL)
 *
 * Creates a new GladeXML object (and the corresponding widgets) from the
 * XML file @fname.  Optionally it will only build the interface from the
 * widget node @root (if it is not %NULL).  This feature is useful if you
 * only want to build say a toolbar or menu from the XML file, but not the
 * window it is embedded in.  Note also that the XML parse tree is cached
 * to speed up creating another GladeXML object for the same file.  This
 * function differs from glade_xml_new in that you can specify a different
 * translation domain from the default to be used.
 *
 * Returns: the newly created GladeXML object, or NULL on failure.
 */
GladeXML *
glade_xml_new_with_domain(const char *fname, const char *root,
			  const char *domain)
{
	GladeXML *self = gtk_type_new(glade_xml_get_type());

	if (!glade_xml_construct(self, fname, root, domain)) {
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
 * @domain: the translation domain (or %NULL for the default)
 *
 * This routine can be used by bindings (such as gtk--) to help construct
 * a GladeXML object, if it is needed.
 *
 * Returns: TRUE if the construction succeeded.
 */
gboolean
glade_xml_construct (GladeXML *self, const char *fname, const char *root,
		     const char *domain)
{
	GladeWidgetTree *tree = glade_tree_get(fname);

	if (!tree)
		return FALSE;

	if (self->textdomain) g_free(self->textdomain);
	self->textdomain = g_strdup(domain);
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
	GList *signals = g_hash_table_lookup(self->priv->signals, handlername);
	
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
autoconnect_foreach(const char *signal_handler, GList *signals,
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


typedef struct {
  GladeXMLConnectFunc func;
  gpointer user_data;
} connect_struct;

static void
autoconnect_full_foreach(const char *signal_handler, GList *signals,
			 connect_struct *conn)
{
	GtkSignalFunc func;
	GladeXML *self = NULL;

	for (; signals != NULL; signals = signals->next) {
		GladeSignalData *data = signals->data;
		GtkObject *connect_object = NULL;
		
		if (data->connect_object) {
			if (!self)
				self = glade_get_widget_tree(
					GTK_WIDGET(data->signal_object));
			connect_object = g_hash_table_lookup(
					self->priv->name_hash,
					data->connect_object);
		}

		(* conn->func) (signal_handler, data->signal_object,
				data->signal_name, data->signal_data,
				connect_object, data->signal_after,
				conn->user_data);
	}
}

/**
 * GladeXMLConnectFunc:
 * @handler_name: the name of the handler function to connect.
 * @object: the object to connect the signal to.
 * @signal_name: the name of the signal.
 * @signal_data: the string value of the signal data given in the XML file.
 * @connect_object: non NULL if gtk_signal_connect_object should be used.
 * @after: TRUE if the connection should be made with gtk_signal_connect_after.
 * @user_data: the user data argument.
 *
 * This is the signature of a function used to connect signals.  It is used
 * by the glade_xml_signal_connect_full and glade_xml_signal_autoconnect_full
 * functions.  It is mainly intented for interpreted language bindings, but
 * could be useful where the programmer wants more control over the signal
 * connection process.
 */

/**
 * glade_xml_signal_connect_full:
 * @self: the GladeXML object.
 * @handler_name: the name of the signal handler that we want to connect.
 * @func: the function to use to connect the signals.
 * @user_data: arbitrary data to pass to the connect function.
 *
 * This function is similar to glade_xml_signal_connect, except that it
 * allows you to give an arbitrary function that will be used for actually
 * connecting the signals.  This is mainly useful for writers of interpreted
 * language bindings, or applications where you need more control over the
 * signal connection process.
 */
void
glade_xml_signal_connect_full(GladeXML *self, const gchar *handler_name,
			      GladeXMLConnectFunc func, gpointer user_data)
{
	connect_struct conn;
	GList *signals = g_hash_table_lookup(self->priv->signals,handler_name);

	g_return_if_fail (func != NULL);
	/* rather than rewriting the code from the autoconnect_full version,
	 * just reuse its helper function */
	conn.func = func;
	conn.user_data = user_data;
	autoconnect_full_foreach(handler_name, signals, &conn);
}

/**
 * glade_xml_signal_autoconnect_full:
 * @self: the GladeXML object.
 * @func: the function used to connect the signals.
 * @user_data: arbitrary data that will be passed to the connection function.
 *
 * This function is similar to glade_xml_signal_connect_full, except that it
 * will try to connect all signals in the interface, not just a single
 * named handler.  It can be thought of the interpeted language binding
 * version of glade_xml_signal_autoconnect, except that it does not
 * require gmodule to function correctly.
 */
void
glade_xml_signal_autoconnect_full (GladeXML *self, GladeXMLConnectFunc func,
				   gpointer user_data)
{
	connect_struct conn;

	g_return_if_fail (func != NULL);
	conn.func = func;
	conn.user_data = user_data;
	g_hash_table_foreach(self->priv->signals,
			     (GHFunc)autoconnect_full_foreach, &conn);
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
 * glade_xml_relative_file
 * @self: the GladeXML object.
 * @filename: the filename.
 *
 * This function resolves a relative pathname, using the directory of the
 * XML file as a base.  If the pathname is absolute, then the original
 * filename is returned.
 *
 * Returns: the filename.  The result must be g_free'd.
 */
gchar *
glade_xml_relative_file(GladeXML *self, const gchar *filename)
{
	gchar *dirname, *tmp;
	g_return_val_if_fail(filename != NULL, NULL);

	if (g_path_is_absolute(filename)) /* an absolute pathname */
		return g_strdup(filename);
	/* prepend XML file's dir onto filename */
	dirname = g_dirname(self->filename);	
	tmp = g_strconcat(dirname, G_DIR_SEPARATOR_S, filename, NULL);
	g_free(dirname);
	return tmp;
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

/**
 * glade_xml_gettext:
 * @xml: the GladeXML widget.
 * @msgid: the string to translate.
 *
 * This function is a wrapper for gettext, that uses the translation domain
 * requested by the user of the function, or the default if no domain has
 * been specified.  This should be used for translatable strings in all
 * widget building routines.
 *
 * Returns: the translated string
 */
char *
glade_xml_gettext(GladeXML *xml, const char *msgid)
{
	if (!msgid)
		return "";
#ifdef ENABLE_NLS
	if (xml->textdomain)
		return dgettext(xml->textdomain, msgid);
	else
		return gettext(msgid);
#else
	return msgid;
#endif
}

/* this is a private function */
static void
glade_xml_add_signals(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info)
{
	GList *tmp;

	for (tmp = info->signals; tmp; tmp = tmp->next) {
		GladeSignalInfo *sig = tmp->data;
		GladeSignalData *data = g_new0(GladeSignalData, 1);
		GList *list;

		data->signal_object = GTK_OBJECT(w);
		data->signal_name = sig->name;
		data->signal_data = sig->data;
		data->connect_object = sig->object;
		data->signal_after = sig->after;

		list = g_hash_table_lookup(xml->priv->signals, sig->handler);
		list = g_list_prepend(list, data);
		g_hash_table_insert(xml->priv->signals, sig->handler, list);
	}
}

static void
glade_xml_add_accels(GtkWidget *w, GladeWidgetInfo *info)
{
	GList *tmp;
	for (tmp = info->accelerators; tmp; tmp = tmp->next) {
		GladeAcceleratorInfo *accel = tmp->data;
		debug(g_message("New Accel: key=%d,mod=%d -> %s:%s",
				accel->key, accel->modifiers,
				gtk_widget_get_name(w), accel->signal));
		gtk_widget_add_accelerator(w, accel->signal,
					   gtk_accel_group_get_default(),
					   accel->key, accel->modifiers,
					   GTK_ACCEL_VISIBLE);
	}
}

static void
glade_style_attach (GtkWidget *widget, const char *style)
{
	const char *name = glade_get_widget_long_name (widget);
	char *full_name;
				
	full_name = g_strconcat ("widget \"", name, "\" style \"GLADE_",
				 style, "_style\"\n", NULL);
	gtk_rc_parse_string(full_name);
	g_free (full_name);
}

static void
glade_xml_destroy_signals(char *key, GList *signal_datas)
{
	GList *tmp;

	for (tmp = signal_datas; tmp; tmp = tmp->next) {
		GladeSignalData *data = tmp->data;
		g_free(data);
	}
	g_list_free(signal_datas);
}

static void
glade_xml_destroy(GtkObject *object)
{
	GladeXML *self = GLADE_XML(object);
	GladeXMLPrivate *priv = self->priv;
	
	if (self->filename)
		g_free(self->filename);
	if (self->textdomain)
		g_free(self->textdomain);

	/* strings are owned in the cached GladeWidgetTree structure */
	g_hash_table_destroy(priv->name_hash);
	/* strings belong to individual widgets -- don't free them */
	g_hash_table_destroy(priv->longname_hash);

	g_hash_table_foreach(priv->signals, (GHFunc)glade_xml_destroy_signals,
			     NULL);
	g_hash_table_destroy(priv->signals);

	/* the group name strings are owned by the GladeWidgetTree */
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
glade_xml_build_interface(GladeXML *self, GladeWidgetTree *tree,
			  const char *root)
{
	GList *tmp;
	GladeWidgetInfo *wid;

	if (root) {
		wid = g_hash_table_lookup(tree->names, root);
		g_return_if_fail(wid != NULL);
		glade_xml_build_widget(self, wid, NULL);
	} else {
		/* build all toplevel nodes */
		for (tmp = tree->widgets; tmp != NULL; tmp = tmp->next) {
			wid = tmp->data;
			glade_xml_build_widget(self, wid, NULL);
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

#ifndef ENABLE_NLS
/* a slight optimisation when gettext is off */
#define glade_xml_gettext(xml, msgid) (msgid)
#endif

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
glade_xml_build_widget(GladeXML *self, GladeWidgetInfo *info,
		       const char *parent_long)
{
	GladeWidgetBuildData *data;
	GtkWidget *ret;

	debug(g_message("Widget class: %s (for parent %s)", info->class,
			parent_long?parent_long:"(null)"));
	if (!strcmp(info->class, "Placeholder")) {
		g_warning("placeholders exist in interface description");
		ret = gtk_label_new("[placeholder]");
		gtk_widget_show(ret);
		return ret;
	}
	data = g_hash_table_lookup(widget_table, info->class);
	if (data == NULL) {
		char buf[50];
		g_warning("unknown widget class '%s'", info->class);
		g_snprintf(buf, 49, "[a %s]", info->class);
		ret = gtk_label_new(buf);
		gtk_widget_show(ret);
		return ret;
	}
	g_assert(data->new);
	ret = data->new(self, info);
	glade_xml_set_common_params(self, ret, info, parent_long);
	return ret;
}

/**
 * glade_xml_set_common_params
 * @self: the GladeXML widget.
 * @widget: the widget to set parameters on.
 * @node: the XML node for this widget.
 * @parent_long: the long name of the parent widget.
 * @widget_class: the class of this widget, or NULL to guess the class.
 *
 * This function sets the common parameters on a widget, and is responsible
 * for inserting it into the GladeXML object's internal structures.  It will
 * also add the children to this widget.  Usually this function is only called
 * by glade_xml_build_widget, but is exposed for difficult cases, such as
 * setting up toolbar buttons and the like.
 */
void
glade_xml_set_common_params(GladeXML *self, GtkWidget *widget,
			    GladeWidgetInfo *info, const char *parent_long)
{
	GList *tmp;
	GladeWidgetBuildData *data;
	char *w_longname;

	/* get the build data */
	if (!widget_table)
		widget_table = g_hash_table_new(g_str_hash, g_str_equal);
	data = g_hash_table_lookup(widget_table, info->class);
	glade_xml_add_signals(self, widget, info);
	glade_xml_add_accels(widget, info);

	gtk_widget_set_name(widget, info->name);
	if (info->tooltip) {
		if (self->priv->tooltips == NULL)
			self->priv->tooltips = gtk_tooltips_new();
		gtk_tooltips_set_tip(self->priv->tooltips,
				     widget,
				     glade_xml_gettext(self, info->tooltip),
				     NULL);
	}

	gtk_widget_set_usize(widget, info->width, info->height);
	if (info->border_width > 0)
		gtk_container_set_border_width(GTK_CONTAINER(widget),
					       info->border_width);
	gtk_widget_set_sensitive(widget, info->sensitive);
	if (info->can_default)
		GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_DEFAULT);
	if (info->can_focus)
		GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);


	for (tmp = info->attributes; tmp != NULL; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "events")) {
			long events = strtol(attr->value, NULL, 0);
			gtk_widget_set_events(widget, events);
		} else if (!strcmp(attr->name, "extension_events")) {
			GdkExtensionMode ex =
			    glade_enum_from_string(GTK_TYPE_GDK_EXTENSION_MODE,
						   attr->value);
			gtk_widget_set_extension_events(widget, ex);
		}
	}

	if (parent_long)
		w_longname = g_strconcat(parent_long, ".", info->name, NULL);
	else
		w_longname = g_strdup(info->name);
	/* store this information as data of the widget.  w_longname is owned by
	 * the widget now */
	gtk_object_set_data(GTK_OBJECT(widget), glade_xml_tag, self);
	gtk_object_set_data(GTK_OBJECT(widget), glade_xml_name_tag,info->name);
	gtk_object_set_data_full(GTK_OBJECT(widget), glade_xml_longname_tag,
				 w_longname, (GtkDestroyNotify)g_free);
	/* store widgets in hash table, for easy lookup */
	g_hash_table_insert(self->priv->name_hash, info->name, widget);
	g_hash_table_insert(self->priv->longname_hash, w_longname, widget);

	if (info->style)
		glade_style_attach(widget, info->style->name);

	if (data->build_children && info->children)
		data->build_children(self, widget, info, w_longname);
	if (info->visible)
		gtk_widget_show(widget);
}

/**
 * glade_standard_build_children
 * @self: the GladeXML object.
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
glade_standard_build_children(GladeXML *self, GtkWidget *w,
			      GladeWidgetInfo *info, const char *longname)
{
	GList *tmp;

	for (tmp = info->children; tmp != NULL; tmp = tmp->next) {
		GtkWidget *child = glade_xml_build_widget(self, tmp->data,
							  longname);
		gtk_container_add(GTK_CONTAINER(w), child);
	}
}

/**
 * glade_get_adjustment
 * @gnode: the XML node for the widget.
 *
 * This utility routine is used to create an adjustment object for a widget.
 *
 * Returns: the newly created GtkAdjustment.
 */
GtkAdjustment *
glade_get_adjustment(GladeWidgetInfo *info)
{
	GList *tmp;
	gdouble hvalue=1, hlower=0, hupper=100, hstep=1, hpage=100, hpage_size=10;

	for (tmp = info->attributes; tmp != NULL; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (attr->name[0] == 'h')
			switch (attr->name[1]) {
			case 'l':
				if (!strcmp(attr->name, "hlower"))
					hlower = g_strtod(attr->value, NULL);
				break;
			case 'p':
				if (!strcmp(attr->name, "hpage"))
					hpage = g_strtod(attr->value, NULL);
				else if (!strcmp(attr->name, "hpage_size"))
					hpage_size=g_strtod(attr->value, NULL);
				break;
			case 's':
				if (!strcmp(attr->name, "hstep"))
					hstep = g_strtod(attr->value, NULL);
				break;
			case 'u':
				if (!strcmp(attr->name, "hupper"))
					hupper = g_strtod(attr->value, NULL);
				break;
			case 'v':
				if (!strcmp(attr->name, "hvalue"))
					hvalue = g_strtod(attr->value, NULL);
				break;
			}
	}
	return GTK_ADJUSTMENT (gtk_adjustment_new(hvalue, hlower, hupper,
						  hstep, hpage, hpage_size));
}

