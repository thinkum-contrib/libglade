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
#ifndef GLADE_H
#define GLADE_H

/* must be called before use of libglade */
void glade_init(void);
void glade_init_gnome_widgets(void);

/* Load the named dynamic module.  Basically it is loaded, and the
 * glade_init_module function is called.  This function should
 * do any library initialisation and call glade_register_widgets */
void glade_load_module(const char *module);

#include <glade/glade-xml.h>
/* don't include glade-build.h -- it is only for widget set definitions */

#endif
