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

/*
 * a few functions to manage the cache of XML trees.  We bother with this, so
 * that multiple instantiations of a window will not need multiple parsings
 * of the associated XML file
 */
#include <stdlib.h>
#include <string.h>
#include <glade/glade-private.h>

static gpointer
new_func(gpointer key)
{
        gchar *filename = key;

	return glade_widget_tree_parse_file(filename);
}

static void
destroy_func(gpointer val)
{
	GladeWidgetTree *tree = (GladeWidgetTree *)val;
  
	if (tree)
		glade_widget_tree_free(tree);
}

GladeWidgetTree *
glade_tree_get(const char *filename)
{
	static GCache *cache = NULL;

	if (!cache) {
		/* cache uses file name as key */
		cache = g_cache_new((GCacheNewFunc)new_func,    /* create new tree */
				    (GCacheDestroyFunc)destroy_func, /* delete a tree */
				    (GCacheDupFunc)g_strdup,    /* strdup to dup key */
				    (GCacheDestroyFunc)g_free,  /* can just free strings */
				    (GHashFunc)g_str_hash,      /* key hash */
				    (GHashFunc)g_direct_hash,   /* value hash */
				    (GCompareFunc)g_str_equal); /* key compare */
	}
	return (GladeWidgetTree *)g_cache_insert(cache, (gpointer) filename);
}

