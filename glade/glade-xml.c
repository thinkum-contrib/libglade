/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-xml.c: implementation of core public interface functions
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
static void glade_xml_class_init(GladeXMLClass *class);

static GObjectClass *parent_class;
static void glade_xml_finalize(GObject *object);

static void glade_xml_build_interface(GladeXML *xml, GladeInterface *iface,
				      const char *root);

GladeExtendedFunc *glade_xml_build_extended_widget = NULL;

/**
 * glade_xml_get_type:
 *
 * Creates the typecode for the GladeXML object type.
 *
 * Returns: the typecode for the GladeXML object type.
 */
GType
glade_xml_get_type(void)
{
    static GType xml_type = 0;

    if (!xml_type) {
	static const GTypeInfo xml_info = {
	    sizeof(GladeXMLClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) glade_xml_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,

	    sizeof (GladeXML),
	    0, /* n_preallocs */
	    (GInstanceInitFunc) glade_xml_init,
	};

	xml_type = g_type_register_static(G_TYPE_OBJECT, "GladeXML",
					  &xml_info, 0);
    }
    return xml_type;
}

static void
glade_xml_class_init (GladeXMLClass *class)
{
    parent_class = g_type_class_ref (G_TYPE_OBJECT);

    G_OBJECT_CLASS(class)->finalize = glade_xml_finalize;
}

static void
glade_xml_init (GladeXML *self)
{
    GladeXMLPrivate *priv;
	
    self->priv = priv = g_new (GladeXMLPrivate, 1);

    self->filename = NULL;
    self->txtdomain = NULL;

    priv->tree = NULL;
    priv->tooltips = gtk_tooltips_new();
    gtk_tooltips_enable(priv->tooltips);
    priv->name_hash = g_hash_table_new(g_str_hash, g_str_equal);
    priv->longname_hash = g_hash_table_new(g_str_hash, g_str_equal);
    priv->signals = g_hash_table_new(g_str_hash, g_str_equal);
    priv->radio_groups = g_hash_table_new (g_str_hash, g_str_equal);
    priv->toplevel = NULL;
    priv->accel_groups = NULL;
    priv->focus_ulines = NULL;
    priv->default_widget = NULL;
    priv->focus_widget = NULL;
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
glade_xml_new(const char *fname, const char *root, const char *domain)
{
    GladeXML *self = g_object_new(GLADE_TYPE_XML, NULL);

    if (!glade_xml_construct(self, fname, root, NULL)) {
	gtk_object_unref(GTK_OBJECT(self));
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
    GladeXML *self = g_object_new(GLADE_TYPE_XML, NULL);

    if (!glade_xml_construct(self, fname, root, domain)) {
	gtk_object_unref(GTK_OBJECT(self));
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
    GladeInterface *iface;

    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(fname != NULL, FALSE);

    iface = glade_parser_parse_file(fname);

    if (!iface)
	return FALSE;

    self->priv->tree = iface;
    if (self->txtdomain) g_free(self->txtdomain);
    self->txtdomain = g_strdup(domain);
    if (self->filename)
	g_free(self->filename);
    self->filename = g_strdup(fname);
    glade_xml_build_interface(self, iface, root);

    return TRUE;
}

/**
 * glade_xml_new_from_buffer:
 * @buffer: the memory buffer containing the XML document.
 * @size: the size of the buffer.
 * @root: the widget node in @buffer to start building from (or %NULL)
 * @domain: the translation domain to use for this interface (or %NULL)
 *
 * Creates a new GladeXML object (and the corresponding widgets) from the
 * buffer @buffer.  Optionally it will only build the interface from the
 * widget node @root (if it is not %NULL).  This feature is useful if you
 * only want to build say a toolbar or menu from the XML document, but not the
 * window it is embedded in.
 *
 * Returns: the newly created GladeXML object, or NULL on failure.
 */
GladeXML *
glade_xml_new_from_buffer(const char *buffer, int size, const char *root,
			  const char *domain)
{
    GladeXML *self;
    GladeInterface *iface = glade_parser_parse_buffer(buffer, size);

    if (!iface)
	return NULL;
    self = g_object_new(GLADE_TYPE_XML, NULL);

    self->priv->tree = iface;
    self->txtdomain = g_strdup(domain);
    self->filename = NULL;
    glade_xml_build_interface(self, iface, root);

    return self;
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
    GList *signals;

    g_return_if_fail(self != NULL);
    g_return_if_fail(handlername != NULL);
    g_return_if_fail(func != NULL);

    signals = g_hash_table_lookup(self->priv->signals, handlername);
    for (; signals != NULL; signals = signals->next) {
	GladeSignalData *data = signals->data;

	if (data->connect_object) {
	    GtkObject *other = g_hash_table_lookup(self->priv->name_hash,
						   data->connect_object);
	    if (data->signal_after)
		gtk_signal_connect_object_after(data->signal_object,
						data->signal_name,
						func, other);
	    else
		gtk_signal_connect_object(data->signal_object,
					  data->signal_name, func, other);
	} else {
	    /* the signal_data argument is just a string, but may be
	     * helpful for someone */
	    if (data->signal_after)
		gtk_signal_connect_after(data->signal_object,
					 data->signal_name, func, NULL);
	    else
		gtk_signal_connect(data->signal_object, data->signal_name,
				   func, NULL);
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
						    data->signal_name,
						    func, other);
		else
		    gtk_signal_connect_object(data->signal_object,
					      data->signal_name, func, other);
	    } else {
		/* the signal_data argument is just a string, but may
		 * be helpful for someone */
		if (data->signal_after)
		    gtk_signal_connect_after(data->signal_object,
					     data->signal_name, func, NULL);
		else
		    gtk_signal_connect(data->signal_object, data->signal_name,
				       func, NULL);
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

    g_return_if_fail(self != NULL);
    if (!g_module_supported())
	g_error("glade_xml_signal_autoconnect requires working gmodule");

    /* get a handle on the main executable -- use this to find symbols */
    allsymbols = g_module_open(NULL, 0);
    g_hash_table_foreach(self->priv->signals, (GHFunc)autoconnect_foreach,
			 allsymbols);
}


typedef struct {
    GladeXMLConnectFunc func;
    gpointer user_data;
} connect_struct;

static void
autoconnect_full_foreach(const char *signal_handler, GList *signals,
			 connect_struct *conn)
{
    GladeXML *self = NULL;

    for (; signals != NULL; signals = signals->next) {
	GladeSignalData *data = signals->data;
	GtkObject *connect_object = NULL;
		
	if (data->connect_object) {
	    if (!self)
		self = glade_get_widget_tree(GTK_WIDGET(data->signal_object));
	    connect_object = g_hash_table_lookup(self->priv->name_hash,
						 data->connect_object);
	}

	(* conn->func) (signal_handler, data->signal_object,
			data->signal_name, NULL,
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
    GList *signals;

    g_return_if_fail(self != NULL);
    g_return_if_fail(handler_name != NULL);
    g_return_if_fail (func != NULL);

    /* rather than rewriting the code from the autoconnect_full
     * version, just reuse its helper function */
    conn.func = func;
    conn.user_data = user_data;
    signals = g_hash_table_lookup(self->priv->signals, handler_name);
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

    g_return_if_fail(self != NULL);
    g_return_if_fail (func != NULL);

    conn.func = func;
    conn.user_data = user_data;
    g_hash_table_foreach(self->priv->signals,
			 (GHFunc)autoconnect_full_foreach, &conn);
}

/**
 * glade_xml_signal_connect_data:
 * @self: the GladeXML object
 * @handlername: the signal handler name
 * @func: the signal handler function
 * @user_data: the signal handler data
 *
 * In the glade interface descriptions, signal handlers are specified for
 * widgets by name.  This function allows you to connect a C function to
 * all signals in the GladeXML file with the given signal handler name.
 *
 * It differs from glade_xml_signal_connect since it allows you to
 * specify the data parameter for the signal handler.  It is also a small
 * demonstration of how to use glade_xml_signal_connect_full.
 */
typedef struct {
    GtkSignalFunc func;
    gpointer user_data;
} connect_data_data;

static void
connect_data_connect_func(const gchar *handler_name, GtkObject *object,
			  const gchar *signal_name, const gchar *signal_data,
			  GtkObject *connect_object, gboolean after,
			  gpointer user_data)
{
    connect_data_data *data = (connect_data_data *)user_data;

    if (connect_object) {
	if (after)
	    gtk_signal_connect_object_after(object, signal_name,
					    data->func, connect_object);
	else
	    gtk_signal_connect_object(object, signal_name,
				      data->func, connect_object);
    } else {
	if (after)
	    gtk_signal_connect_after(object, signal_name,
				     data->func, data->user_data);
	else
	    gtk_signal_connect(object, signal_name,
			       data->func, data->user_data);
    }
}

void
glade_xml_signal_connect_data (GladeXML *self, const char *handlername,
			       GtkSignalFunc func, gpointer user_data)
{
    connect_data_data data;

    data.func = func;
    data.user_data = user_data;

    glade_xml_signal_connect_full(self, handlername,
				  connect_data_connect_func, &data);
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
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    return g_hash_table_lookup(self->priv->name_hash, name);
}


/**
 * glade_xml_get_widget_prefix:
 * @self: the GladeXML object.
 * @name: the name of the widget.
 *
 * This function is used to get a list of pointers to the GtkWidget(s)
 * with names that start with the string @name in the interface description.
 * You would use this if you have to do something  to all of these widgets
 * after loading.
 *
 * Returns: A list of the widget that match @name as the start of their
 * name, or %NULL if none exists.
 */
typedef struct {
    const gchar *name;
    GList *list;
} widget_prefix_data;

static void
widget_prefix_add_to_list (gchar *name, gpointer value,
			   widget_prefix_data *data)
{
    if (!strncmp (data->name, name, strlen (data->name)))
	data->list = g_list_prepend (data->list, value);
}

GList *
glade_xml_get_widget_prefix (GladeXML *self, const gchar *prefix)
{
    widget_prefix_data data;

    data.name = prefix;
    data.list = NULL;

    g_hash_table_foreach (self->priv->name_hash,
			  (GHFunc) widget_prefix_add_to_list, &data);

    return data.list;
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
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(longname != NULL, NULL);

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

    g_return_val_if_fail(self != NULL, NULL);
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
    g_return_val_if_fail(widget != NULL, NULL);

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
    g_return_val_if_fail(widget != NULL, NULL);

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
    g_return_val_if_fail(widget != NULL, NULL);

    return gtk_object_get_data(GTK_OBJECT(widget), glade_xml_tag);
}

/* ------------------------------------------- */


/**
 * glade_xml_set_toplevel:
 * @xml: the GladeXML object.
 * @window: the toplevel.
 *
 * This is used while the tree is being built to set the toplevel window that
 * is currently being built.  It is mainly used to enable GtkAccelGroup's to
 * be bound to the correct window, but could have other uses.
 */
void
glade_xml_set_toplevel(GladeXML *xml, GtkWindow *window)
{
    static char *tooltips_key = "libglade::GladeXML::tooltips";

    if (xml->priv->focus_widget)
	gtk_widget_grab_focus(xml->priv->focus_widget);
    if (xml->priv->default_widget)
	gtk_widget_grab_default(xml->priv->default_widget);
    xml->priv->focus_widget = NULL;
    xml->priv->default_widget = NULL;
    xml->priv->toplevel = window;
    /* new toplevel needs new accel group */
    if (xml->priv->accel_groups)
	glade_xml_pop_accel(xml);
    /* maybe put an assert that xml->priv->accel_groups == NULL here? */
    xml->priv->accel_groups = NULL;
    /* the window should hold a reference to the tooltips object */
    gtk_object_ref(GTK_OBJECT(xml->priv->tooltips));
    gtk_object_set_data_full(GTK_OBJECT(window), tooltips_key,
			     xml->priv->tooltips,
			     (GtkDestroyNotify)gtk_object_unref);
}

 /**
 * glade_xml_push_accel:
 * @xml: the GladeXML object.
 *
 * Make a new accel group amd push onto the stack.  This is intended for use
 * by GtkNotebook so that each notebook page can set up its own accelerators.
 * Returns: The current GtkAccelGroup after push.
 */
GtkAccelGroup *
glade_xml_push_accel(GladeXML *xml)
{
    GtkAccelGroup *accel = gtk_accel_group_new();
       
    xml->priv->accel_groups = g_slist_prepend(xml->priv->accel_groups, accel);
    return accel;
}

/* glade_xml_pop_accel:
 * @xml: the GladeXML object.
 *
 * Pops the accel group.  This will usually be called after a GtkNotebook page
 * has been built.
 * Returns: The current GtkAccelGroup after pop.
 */
GtkAccelGroup *
glade_xml_pop_accel(GladeXML *xml)
{
    GtkAccelGroup *accel;

    g_return_val_if_fail(xml->priv->accel_groups != NULL, NULL);

    /* the accel group at the top of the stack */
    accel = xml->priv->accel_groups->data;
    xml->priv->accel_groups = g_slist_remove(xml->priv->accel_groups, accel);
    /* unref the accel group that was at the top of the stack */
    gtk_accel_group_unref(accel);
    return xml->priv->accel_groups ? xml->priv->accel_groups->data : NULL;
}

/**
 * glade_xml_ensure_accel:
 * @xml: the GladeXML object.
 *
 * This function is used to get the current GtkAccelGroup.  If there isn't
 * one, a new one is created and bound to the current toplevel window (if
 * a toplevel has been set).
 * Returns: the current GtkAccelGroup.
 */
GtkAccelGroup *
glade_xml_ensure_accel(GladeXML *xml)
{
    if (!xml->priv->accel_groups) {
	glade_xml_push_accel(xml);
	if (xml->priv->toplevel)
	    gtk_window_add_accel_group(xml->priv->toplevel,
				       xml->priv->accel_groups->data);
    }
    return (GtkAccelGroup *)xml->priv->accel_groups->data;
}

#if 0
/**
 * glade_xml_handle_label_accel:
 * @xml: the GladeXML object.
 * @target: the target widget name (or %NULL).
 * @key: the key code for the accelerator.
 *
 * This function is called by the GtkLabel creation routine to register an
 * underline accelerator for the label.  If the target widget exists, the
 * accelerator is connected to the grab_focus signal.  If it does not exist
 * yet, the (target, key) pair is saved so that it can be attached when the
 * widget is created.
 * If @target is %NULL, then the accelerator should be attached to one of
 * the parents of this widget (typically a GtkButton).
 */
void
glade_xml_handle_label_accel(GladeXML *xml, const gchar *target, guint key)
{
	if (target) {
		GtkWidget *twidget = glade_xml_get_widget(xml, target);

		if (twidget)
			gtk_widget_add_accelerator(twidget, "grab_focus",
						   glade_xml_ensure_accel(xml),
						   key, GDK_MOD1_MASK, 0);
		else {
			GladeFocusULine *uline = g_new(GladeFocusULine, 1);

			uline->widget_name = target;
			uline->key = key;
			xml->priv->focus_ulines =
				g_list_prepend(xml->priv->focus_ulines, uline);
		}
	} else
		xml->priv->parent_accel = key;
}

/**
 * glade_xml_get_parent_accel:
 * @xml: the GladeXML object.
 *
 * This function gets the latest label underline accelerator directed at the
 * parent widget.  If there is no accelerator waiting, 0 is returned.  If
 * this function is called twice in a row, the second call will always return
 * 0.
 *
 * Returns: the key code for the accelerator, or zero.
 */
guint
glade_xml_get_parent_accel(GladeXML *xml)
{
	guint key = xml->priv->parent_accel;

	xml->priv->parent_accel = 0;
	return key;
}
#endif

/**
 * glade_xml_gettext:
 * @xml: the GladeXML object.
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
    if (!msgid || msgid[0] == '\0')
	return "";
#ifdef ENABLE_NLS
    if (xml->txtdomain)
	return dgettext(xml->txtdomain, msgid);
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
    gint i;

    for (i = 0; i < info->n_signals; i++) {
	GladeSignalInfo *sig = &info->signals[i];
	GladeSignalData *data = g_new0(GladeSignalData, 1);
	GList *list;

	data->signal_object = GTK_OBJECT(w);
	data->signal_name = sig->name;
	data->connect_object = sig->object;
	data->signal_after = sig->after;

	list = g_hash_table_lookup(xml->priv->signals, sig->handler);
	list = g_list_prepend(list, data);
	g_hash_table_insert(xml->priv->signals, sig->handler, list);
    }
}

static void
glade_xml_add_accels(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info)
{
    gint i;
    for (i = 0; i < info->n_accels; i++) {
	GladeAccelInfo *accel = &info->accels[i];

	debug(g_message("New Accel: key=%d,mod=%d -> %s:%s",
			accel->key, accel->modifiers,
			gtk_widget_get_name(w), accel->signal));
	gtk_widget_add_accelerator(w, accel->signal,
				   glade_xml_ensure_accel(xml),
				   accel->key, accel->modifiers,
				   GTK_ACCEL_VISIBLE);
    }
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
free_radio_groups(gpointer key, gpointer value, gpointer user_data)
{
    g_free(key);  /* the string name */
}

static void
glade_xml_finalize(GObject *object)
{
    GladeXML *self = GLADE_XML(object);
    GladeXMLPrivate *priv = self->priv;
	
    if (self->filename)
	g_free(self->filename);
    self->filename = NULL;
    if (self->txtdomain)
	g_free(self->txtdomain);
    self->txtdomain = NULL;

    if (priv) {
	if (priv->tree)
	    glade_interface_destroy(priv->tree);
	/* strings are owned in the cached GladeWidgetTree structure */
	g_hash_table_destroy(priv->name_hash);
	/* strings belong to individual widgets -- don't free them */
	g_hash_table_destroy(priv->longname_hash);

	g_hash_table_foreach(priv->signals,
			     (GHFunc)glade_xml_destroy_signals, NULL);
	g_hash_table_destroy(priv->signals);

	/* the group name strings are owned by the GladeWidgetTree */
	g_hash_table_foreach(self->priv->radio_groups,
			     free_radio_groups, NULL);
	g_hash_table_destroy(priv->radio_groups);

	if (priv->tooltips)
	    gtk_object_unref(GTK_OBJECT(priv->tooltips));

	/* there should only be at most one accel group on stack */
	if (priv->accel_groups)
	    glade_xml_pop_accel(self);

	g_free (self->priv);
    }
    self->priv = NULL;
    if (parent_class->finalize)
	(* parent_class->finalize)(object);
}

/**
 * glade_set_custom_handler
 * @handler: the custom widget handler
 * @user_data: user data passed to the custom handler
 *
 * Calling this function allows you to override the default behaviour
 * when a Custom widget is found in an interface.  This could be used by
 * a language binding to call some other function, or to limit what
 * functions can be called to create custom widgets.
 */
static GtkWidget *
default_custom_handler(GladeXML *xml, gchar *func_name, gchar *name,
		       gchar *string1, gchar *string2, gint int1, gint int2,
		       gpointer user_data)
{
    typedef GtkWidget *(* create_func)(gchar *name,
				       gchar *string1, gchar *string2,
				       gint int1, gint int2);
    GModule *allsymbols;
    create_func func;

    if (!g_module_supported()) {
	g_error("custom_new requires gmodule to work correctly");
	return NULL;
    }
    allsymbols = g_module_open(NULL, 0);
    if (g_module_symbol(allsymbols, func_name, (gpointer)&func))
	return (* func)(name, string1, string2, int1, int2);
    g_warning("could not find widget creation function");
    return NULL;
}

static GladeXMLCustomWidgetHandler custom_handler = default_custom_handler;
static gpointer custom_user_data = NULL;

void
glade_set_custom_handler(GladeXMLCustomWidgetHandler handler,
			 gpointer user_data)
{
    custom_handler = handler;
    custom_user_data = user_data;
}

/**
 * glade_set_custom_handler
 * @xml: the GladeXML object
 * @func_name: the name of the widget creation function
 * @name: the name of the widget
 * @string1: the first string argument
 * @string2: the second string argument
 * @int1: the first integer argument
 * @int2: the second integer argument
 *
 * Invokes the custom widget creation function.
 * Returns: the new widget or NULL on error.
 */
GtkWidget *
glade_create_custom(GladeXML *xml, gchar *func_name, gchar *name,
		    gchar *string1, gchar *string2, gint int1, gint int2)
{
    return (* custom_handler)(xml, func_name, name, string1, string2,
				  int1, int2, custom_user_data);
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
glade_enum_from_string (GType type, const char *string)
{
    GEnumClass *eclass;
    GEnumValue *ev;
    gchar *endptr;
    gint ret = 0;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    eclass = g_type_class_ref(type);
    ev = g_enum_get_value_by_name(eclass, string);
    if (!ev) ev = g_enum_get_value_by_nick(eclass, string);
    if (ev)  ret = ev->value;

    g_type_class_unref(eclass);

    return ret;
}

guint
glade_flags_from_string (GType type, const char *string)
{
    GFlagsClass *fclass;
    GFlagsValue *fv;
    gchar *endptr;
    guint ret = 0;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    ret = 0;
    fclass = g_type_class_ref(type);

    while ((endptr = strchr(string, ' '))) {
	char *str = g_strndup(string, endptr-string);

	fv = g_flags_get_value_by_name(fclass, str);
	if (!fv) fv = g_flags_get_value_by_nick(fclass, str);
	if (fv)  ret |= fv->value;

	g_free(str);
	string = endptr;
	while (string[0] == ' ' || string[0] == '|')
	    string++;
    }
    fv = g_flags_get_value_by_name(fclass, string);
    if (!fv) fv = g_flags_get_value_by_nick(fclass, string);
    if (fv)  ret |= fv->value;

    g_type_class_unref(fclass);

    return ret;
}

static void
glade_xml_build_interface(GladeXML *self, GladeInterface *iface,
			  const char *root)
{
    gint i;
    GladeWidgetInfo *wid;
    GtkWidget *w;

    if (root) {
	wid = g_hash_table_lookup(iface->names, root);
	g_return_if_fail(wid != NULL);
	w = glade_xml_build_widget(self, wid, NULL);
	if (GTK_IS_WINDOW(w)) {
	    if (self->priv->focus_widget)
		gtk_widget_grab_focus(self->priv->focus_widget);
	    if (self->priv->default_widget)
		gtk_widget_grab_default(self->priv->default_widget);
	}
    } else {
	/* build all toplevel nodes */
	for (i = 0; i < iface->n_toplevels; i++)
	    glade_xml_build_widget(self, iface->toplevels[i],NULL);
	if (self->priv->focus_widget)
	    gtk_widget_grab_focus(self->priv->focus_widget);
	if (self->priv->default_widget)
	    gtk_widget_grab_default(self->priv->default_widget);
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
	g_hash_table_insert(widget_table, widgets[i].name,
			    (gpointer)&widgets[i]);
	i++;
    }
}

gboolean
glade_xml_set_value_from_string (GParamSpec *pspec,
				 const gchar *string,
				 GValue *value)
{
    GType prop_type;
    gboolean ret = TRUE;

    prop_type = G_PARAM_SPEC_VALUE_TYPE(pspec);
    g_value_init(value, prop_type);
    switch (G_TYPE_FUNDAMENTAL(prop_type)) {
    case G_TYPE_CHAR:
	g_value_set_char(value, string[0]);
	break;
    case G_TYPE_UCHAR:
	g_value_set_uchar(value, (guchar)string[0]);
	break;
    case G_TYPE_BOOLEAN:
	g_value_set_boolean(value, string[0] == 'T' || string[0] == 'y');
	break;
    case G_TYPE_INT:
	g_value_set_int(value, strtol(string, NULL, 0));
	break;
    case G_TYPE_UINT:
	g_value_set_uint(value, strtoul(string, NULL, 0));
	break;
    case G_TYPE_LONG:
	g_value_set_long(value, strtol(string, NULL, 0));
	break;
    case G_TYPE_ULONG:
	g_value_set_ulong(value, strtoul(string, NULL, 0));
	break; 
    case G_TYPE_ENUM:
	g_value_set_enum(value, glade_enum_from_string(prop_type, string));
	break;
    case G_TYPE_FLAGS:
	g_value_set_flags(value, glade_flags_from_string(prop_type, string));
	break;
    case G_TYPE_FLOAT:
	g_value_set_float(value, g_strtod(string, NULL));
	break;
    case G_TYPE_DOUBLE:
	g_value_set_double(value, g_strtod(string, NULL));
	break;
    case G_TYPE_STRING:
	g_value_set_string(value, string);
	break;
    case G_TYPE_BOXED:
    default:
	g_warning("unsupported property type `%s'", g_type_name(prop_type));
	g_value_unset(value);
	ret = FALSE;
	break;
    }
    return ret;
}

GtkWidget *
glade_standard_build_widget(GladeXML *xml, GType widget_type,
			    GladeWidgetInfo *info)
{
    static GArray *props_array = NULL;
    GObjectClass *oclass;
    GtkWidget *widget;
    guint i;

    if (!props_array)
	props_array = g_array_new(FALSE, FALSE, sizeof(GParameter));

    /* we ref the class here as a slight optimisation */
    oclass = g_type_class_ref(widget_type);

    /* collect properties */
    for (i = 0; i < info->n_properties; i++) {
	GParameter param = { NULL };
	GParamSpec *pspec;

	pspec = g_object_class_find_property(oclass, info->properties[i].name);
	if (!pspec) {
	    g_warning("unknown property `%s' for class `%s'",
		      info->properties[i].name, g_type_name(widget_type));
	    continue;
	}

	if (glade_xml_set_value_from_string(pspec, info->properties[i].value,
					    &param.value)) {
	    param.name = info->properties[i].name;
	    g_array_append_val(props_array, param);
	}
    }
    widget = g_object_newv(widget_type, props_array->len,
			   (GParameter *)props_array->data);

    /* clean up props_array */
    for (i = 0; i < props_array->len; i++) {
	g_array_index(props_array, GParameter, i).name = NULL;
	g_value_unset(&g_array_index(props_array, GParameter, i).value);
    }
    g_array_set_size(props_array, 0);
    g_type_class_unref(oclass);

    return widget;
}

/**
 * glade_standard_build_children
 * @self: the GladeXML object.
 * @w: the container widget.
 * @info: the GladeWidgetInfo structure.
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
    gint i, j;

    g_object_ref(G_OBJECT(w));
    for (i = 0; i < info->n_children; i++) {
	GladeWidgetInfo *childinfo = info->children[i].child;
	GtkWidget *child;

	/* handle any internal children */
	if (info->children[i].internal_child) {
	    glade_xml_handle_internal_child(self, w, &info->children[i],
					    longname);
	    continue;
	}

	child = glade_xml_build_widget(self, childinfo, longname);

	g_object_ref(G_OBJECT(child));
	gtk_widget_freeze_child_notify(child);
	gtk_container_add(GTK_CONTAINER(w), child);
	for (j = 0; j < info->children[i].n_properties; j++) {
	    const gchar *name = info->children[i].properties[j].name;
	    GValue value = { 0 };
	    GParamSpec *pspec;

	    pspec = gtk_container_class_find_child_property(
		G_OBJECT_GET_CLASS(w), name);
	    if (!pspec) {
		g_warning("unknown child property `%s' for container `%s'",
			  name, G_OBJECT_TYPE_NAME(w));
		continue;
	    }

	    if (glade_xml_set_value_from_string(pspec,
			info->children[i].properties[j].value, &value)) {
		gtk_container_child_set_property(GTK_CONTAINER(w), child,
						 name, &value);
		g_value_unset(&value);
	    }
	}
	gtk_widget_thaw_child_notify(child);
	g_object_ref(G_OBJECT(child));
    }
    g_object_unref(G_OBJECT(w));
}

#ifndef ENABLE_NLS
/* a slight optimisation when gettext is off */
#define glade_xml_gettext(xml, msgid) (msgid)
#endif

/**
 * GladeNewFunc
 * @xml: The GladeXML object.
 * @widget_type: the GType code of the widget.
 * @info: the GladeWidgetInfo structure for this widget.
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
 * @info: the GladeWidgetInfo structure for this widget.
 * @longname: the long name for this widget.
 *
 * This function signature should be used by functions that are responsible
 * for adding children to a container widget.  To create each child widget,
 * glade_xml_build_widget should be called.
 */

/**
 * glade_xml_build_widget:
 * @self: the GladeXML object.
 * @info: the GladeWidgetInfo structure for the widget.
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
    GType widget_type;
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
	if (glade_xml_build_extended_widget) {
	    char *err = NULL;

	    ret = glade_xml_build_extended_widget(self, info, &err);
	    if (!ret) {
		g_warning("%s", err);
		ret = gtk_label_new(err);
		g_free(err);
		gtk_widget_show(ret);
	    }
	} else {
	    char buf[50];
	    g_warning("unknown widget class '%s'", info->class);
	    g_snprintf(buf, 49, "[a %s]", info->class);
	    ret = gtk_label_new(buf);
	    gtk_widget_show(ret);
	}
    } else {
	if (!data->typecode && data->get_type_func)
	    data->typecode = data->get_type_func();
	g_assert(data->new);
	g_assert(data->typecode);
	ret = data->new(self, data->typecode, info);
    }
    glade_xml_set_common_params(self, ret, info, parent_long);
    return ret;
}

void
glade_xml_handle_internal_child(GladeXML *self, GtkWidget *parent,
				GladeChildInfo *child_info,
				const gchar *parent_long)
{
    GladeWidgetBuildData *parent_build_data = NULL;
    GtkWidget *child;

    /* walk up the widget heirachy until we find a parent with a
     * find_internal_child handler */
    while (parent_build_data == NULL && parent != NULL) {
	parent_build_data = g_hash_table_lookup(widget_table,
						G_OBJECT_TYPE_NAME(parent));

	if (parent_build_data != NULL &&
	    parent_build_data->find_internal_child != NULL)
	    break;

	parent_build_data = NULL; /* set to NULL if no find_internal_child */
	parent = parent->parent;
    }

    if (!parent_build_data || !parent_build_data->find_internal_child) {
	g_warning("could not find a parent that handles internal"
		  " children for `%s'", child_info->internal_child);
	return;
    }

    child = parent_build_data->find_internal_child(self, parent,
						   child_info->internal_child);

    if (!child) {
	g_warning("could not find internal child `%s' in parent of type `%s'",
		  child_info->internal_child, G_OBJECT_TYPE_NAME(parent));
	return;
    }

    glade_xml_set_common_params(self, child, child_info->child, parent_long);
}


/**
 * glade_xml_set_common_params
 * @self: the GladeXML widget.
 * @widget: the widget to set parameters on.
 * @info: the GladeWidgetInfo structure for the widget.
 * @parent_long: the long name of the parent widget.
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
    glade_xml_add_accels(self, widget, info);

#if 0
    for (tmp = self->priv->focus_ulines; tmp; tmp = tmp->next) {
	GladeFocusULine *uline = tmp->data;

	if (!strcmp(uline->widget_name, info->name)) {
	    /* it is for us ... */
	    gtk_widget_add_accelerator(widget, "grab_focus",
				       glade_xml_ensure_accel(self),
				       uline->key, GDK_MOD1_MASK, 0);
	    tmp = tmp->next;
	    self->priv->focus_ulines =
		g_list_remove(self->priv->focus_ulines, uline);
	    g_free(uline);
	}
	if (!tmp)
	    break;
    }
#endif

    gtk_widget_set_name(widget, info->name);
#if 0
    if (info->tooltip) {
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
    else
	GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_FOCUS);
    if (info->has_default)
	self->priv->default_widget = widget;
    if (info->has_focus)
	self->priv->focus_widget = widget;

    for (tmp = info->attributes; tmp != NULL; tmp = tmp->next) {
	GladeAttribute *attr = tmp->data;

	if (!strcmp(attr->name, "events")) {
	    char *tmpptr, *endptr;
	    long events = strtol(attr->value, &endptr, 0);

	    /* format conversion error */
	    if (attr->value == endptr) {
		events = 0;
		tmpptr = attr->value;
		while ((endptr = strchr(tmpptr, ' '))) {
		    char *str = g_strndup(tmpptr, endptr-tmpptr);
		    events |= glade_enum_from_string(GDK_TYPE_EVENT_MASK, str);
		    g_free(str);
		    tmpptr = endptr;
		    while (tmpptr[0] == ' ' || tmpptr[0] == '|')
			tmpptr++;
		}
		events |= glade_enum_from_string(GDK_TYPE_EVENT_MASK, tmpptr);
	    }
	    gtk_widget_set_events(widget, events);
	} else if (!strcmp(attr->name, "extension_events")) {
	    GdkExtensionMode ex =
		glade_enum_from_string(GDK_TYPE_EXTENSION_MODE, attr->value);
	    gtk_widget_set_extension_events(widget, ex);
	}
    }
#endif

    if (parent_long)
	w_longname = g_strconcat(parent_long, ".", info->name, NULL);
    else
	w_longname = g_strdup(info->name);
    /* store this information as data of the widget.  w_longname is
     * owned by the widget now */
    gtk_object_set_data(GTK_OBJECT(widget), glade_xml_tag, self);
    gtk_object_set_data(GTK_OBJECT(widget), glade_xml_name_tag,info->name);
    gtk_object_set_data_full(GTK_OBJECT(widget), glade_xml_longname_tag,
			     w_longname, (GtkDestroyNotify)g_free);
    /* store widgets in hash table, for easy lookup */
    g_hash_table_insert(self->priv->name_hash, info->name, widget);
    g_hash_table_insert(self->priv->longname_hash, w_longname, widget);

    if (data && data->build_children && info->children)
	data->build_children(self, widget, info, w_longname);
#if 0
    if (info->visible)
	gtk_widget_show(widget);
#endif
}

/**
 * glade_get_adjustment
 * @info: the GladeWidgetInfo structure for this widget.
 *
 * This utility routine is used to create an adjustment object for a widget.
 *
 * Returns: the newly created GtkAdjustment.
 */
GtkAdjustment *
glade_get_adjustment(GladeWidgetInfo *info)
{
    gint i;
    gdouble hvalue=1, hlower=0, hupper=100, hstep=1, hpage=100, hpage_size=10;

    for (i = 0; i < info->n_properties; i++) {
	GladeProperty *prop = &info->properties[i];
	gchar *name = prop->name;

	if (name[0] == 'h')
	    name++;

	switch (name[0]) {
	case 'l':
	    if (!strcmp(name, "lower"))
		hlower = g_strtod(prop->value, NULL);
	    break;
	case 'p':
	    if (!strcmp(name, "page"))
		hpage = g_strtod(prop->value, NULL);
	    else if (!strcmp(name, "page_size"))
		hpage_size=g_strtod(prop->value, NULL);
	    break;
	case 's':
	    if (!strcmp(name, "step"))
		hstep = g_strtod(prop->value, NULL);
	    break;
	case 'u':
	    if (!strcmp(name, "upper"))
		hupper = g_strtod(prop->value, NULL);
	    break;
	case 'v':
	    if (!strcmp(name, "value"))
		hvalue = g_strtod(prop->value, NULL);
	    break;
	}
    }
    return GTK_ADJUSTMENT (gtk_adjustment_new(hvalue, hlower, hupper,
					      hstep, hpage, hpage_size));
}

#if 0
/**
 * glade_xml_set_window_props
 * @window: the GtkWindow to set the properties of.
 * @info: the GladeWidgetInfo structure holding the properties
 *
 * This is a convenience function to set some common attributes on
 * GtkWindow widgets and widgets derived from GtkWindow.  It does not
 * set the title or the window type, as these may need special handling
 * in some widgets.
 */
void
glade_xml_set_window_props(GtkWindow *window, GladeWidgetInfo *info)
{
    gint i;
    gboolean allow_grow = window->allow_grow;
    gboolean allow_shrink = window->allow_shrink;
    gboolean auto_shrink = window->auto_shrink;
    gchar *wmname = NULL, *wmclass = NULL;

    for (i = 0; i < info->n_properties; i++) {
	GladeProperty *prop = &info->properties[i];

	switch (prop->name[0]) {
	case 'a':
	    if (!strcmp(prop->name, "allow_grow"))
		allow_grow = prop->value[0] == 'T';
	    else if (!strcmp(prop->name, "allow_shrink"))
		allow_shrink = prop->value[0] == 'T';
	    else if (!strcmp(prop->name, "auto_shrink"))
		auto_shrink = prop->value[0] == 'T';
	    break;
	case 'd':
	    if (!strcmp(prop->name, "default_height"))
		gtk_window_set_default_size(window,
					    -2, strtol(prop->value, NULL, 0));
	    else if (!strcmp(prop->name, "default_width"))
		gtk_window_set_default_size(window,
					    strtol(prop->value, NULL, 0), -2);
	    break;
	case 'm':
	    if (!strcmp(prop->name, "modal"))
		gtk_window_set_modal(window, prop->value[0] == 'T');
	    break;
	case 'p':
	    if (!strcmp(prop->name, "position"))
		gtk_window_set_position(window,	glade_enum_from_string(
						GTK_TYPE_WINDOW_POSITION,
						prop->value));
	    break;
	case 'w':
	    if (!strcmp(prop->name, "wmclass_name"))
		wmname = prop->value;
	    else if (!strcmp(prop->name, "wmclass_class"))
		wmclass = prop->value;
	    break;
	case 'x':
	    if (prop->name[1] == '\0')
		gtk_widget_set_uposition(GTK_WIDGET(window),
					 strtol(prop->value, NULL, 0), -2);
	    break;
	case 'y':
	    if (prop->name[1] == '\0')
		gtk_widget_set_uposition(GTK_WIDGET(window),
					 -2, strtol(prop->value, NULL, 0));
	    break;
	}
    }
    gtk_window_set_policy(window, allow_shrink, allow_grow, auto_shrink);
    if (wmname != NULL || wmclass != NULL)
	gtk_window_set_wmclass(window,
			       wmname  ? wmname  : "",
			       wmclass ? wmclass : "");
}
#endif
