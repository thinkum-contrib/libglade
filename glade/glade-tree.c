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

/*
 * a few functions to manage the cache of XML trees.  We bother with this, so
 * that multiple instantiations of a window will not need multiple parsings
 * of the associated XML file
 */
#include <stdlib.h>
#include <string.h>
#include <glade/glade-private.h>
#include <parser.h>

static void
recurse_tree (xmlNodePtr xmlnode, GNode *parent, GHashTable *hash)
{
	xmlNodePtr tmp;
	GNode *self = g_node_new(xmlnode);

	g_node_append(parent, self);
	for (tmp = xmlnode->childs; tmp != NULL; tmp = tmp->next) {
		char *content = xmlNodeGetContent(tmp);
		if (tmp->name && !strcmp(tmp->name, "name"))
			g_hash_table_insert(hash, content, self);
		else if (tmp->name && !strcmp(tmp->name, "widget"))
			recurse_tree(tmp, self, hash);
		if (content) free(content);
	}
}

static gpointer
new_func(gpointer key)
{
	GladeTreeData *tree;
	xmlNodePtr tmp;

	tree = g_new(GladeTreeData, 1);
	tree->xml =  xmlParseFile((char *)key);
	debug(g_message("Base node: '%s'", tree->xml->root->name));

	if (!tree->xml || strcmp(tree->xml->root->name, "GTK-Interface")) {
		g_warning("%s doesn't appear to be a GLADE XML file", (char *)key);
		xmlFreeDoc(tree->xml);
		g_free(tree);
		return NULL;
	}
	glade_style_parse(tree->xml);
	tree->tree = g_node_new(NULL); /* root node */
	tree->hash = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_freeze(tree->hash);

	for (tmp = tree->xml->root->childs; tmp != NULL; tmp = tmp->next)
		if (tmp->name && !strcmp(tmp->name, "widget"))
			recurse_tree(tmp, tree->tree, tree->hash);
	g_hash_table_thaw(tree->hash);
	return tree;
}

static void
destroy_func(gpointer val)
{
	GladeTreeData *tree = (GladeTreeData *)val;
  
	if (val == NULL)
		return;
	xmlFreeDoc(tree->xml);
	g_node_destroy(tree->tree);
	g_hash_table_destroy(tree->hash);
	g_free(tree);
}

GladeTreeData *
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
	return (GladeTreeData *)g_cache_insert(cache, (gpointer) filename);
}

xmlNodePtr
glade_tree_find_node(xmlNodePtr parent, const char *childname)
{
	xmlNodePtr ptr;

	for (ptr = parent->childs; ptr != NULL; ptr = ptr->next) {
		if (!strcmp(ptr->name, childname))
			break;
	}
	return ptr;
}

