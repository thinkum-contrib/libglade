/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-init.c: initialisation functions for libglade
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

#include <string.h>
#include <glib.h>
#include <gmodule.h>

#include "glade-init.h"
#include "glade-build.h"

void _glade_init_gtk_widgets (void);

/**
 * glade_init:
 * 
 * It used to be necessary to call glade_init() before creating
 * GladeXML objects.  This is now no longer the case, as libglade will
 * be initialised on demand now.  Calling glade_init() manually will
 * not cause any problems though.
 */
void
glade_init(void)
{
    static gboolean initialised = FALSE;

    if (initialised) return;
    initialised = TRUE;
    _glade_init_gtk_widgets();
}

gchar *
glade_module_check_version(gint version)
{
  if (version != GLADE_MODULE_API_VERSION)
    return "Wrong plugin API version";
  else
    return NULL;
}

static GPtrArray *loaded_packages = NULL;

/**
 * glade_require:
 * @library: the required library
 *
 * Ensure that a required library is available.  If it is not already
 * available, libglade will attempt to dynamically load a module that
 * contains the handlers for that library.
 */

void
glade_require(const gchar *library)
{
    gboolean already_loaded = FALSE;
    gchar *filename;
    GModule *module;
    void (* init_func)(void);

    /* a call to glade_init here to make sure libglade is initialised */
    glade_init();

    if (loaded_packages) {
	gint i;

	for (i = 0; i < loaded_packages->len; i++)
	    if (!strcmp(library, g_ptr_array_index(loaded_packages, i))) {
		already_loaded = TRUE;
		break;
	    }
    }

    if (already_loaded)
	return;

    filename = g_module_build_path (GLADE_MODULE_DIR, library);
    module = g_module_open(filename, G_MODULE_BIND_LAZY);
    if (!module) {
	g_warning("Could not load support for `%s': %s", library,
		  g_module_error());
	g_free(filename);
	return;
    }
    g_free(filename);

    if (!g_module_symbol(module, "glade_module_register_widgets",
			 (gpointer)&init_func)) {
	g_warning("could not find `%s' init function: %s", library,
		  g_module_error());
	g_module_close(module);
	return;
    }

    init_func();
    g_module_make_resident(module);
}

/**
 * glade_provide:
 * @library: the provided library
 *
 * This function should be called by a module to assert that it
 * provides wrappers for a particular library.  This should be called
 * by the register_widgets() function of a libglade module so that it
 * isn't loaded twice, for instance.
 */

void
glade_provide(const gchar *library)
{
    gboolean already_loaded = FALSE;
    gint i;

    if (!loaded_packages)
	loaded_packages = g_ptr_array_new();

    for (i = 0; i < loaded_packages->len; i++)
	if (!strcmp(library, g_ptr_array_index(loaded_packages, i))) {
	    already_loaded = TRUE;
	    break;
	}

    if (!already_loaded)
	g_ptr_array_add(loaded_packages, g_strdup(library));
}
