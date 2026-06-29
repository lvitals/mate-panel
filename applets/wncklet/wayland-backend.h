/* Wncklet applet Wayland backend */

/*
 * Copyright (C) 2019 William Wold
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef _WNCKLET_APPLET_WAYLAND_BACKEND_H_
#define _WNCKLET_APPLET_WAYLAND_BACKEND_H_

#ifdef PACKAGE_NAME /* only check HAVE_WAYLAND if config.h has been included */
#ifndef HAVE_WAYLAND
#error file should only be included when HAVE_WAYLAND is enabled
#endif
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef __cplusplus
extern "C" {
#endif

void toggle_show_desktop(GtkWidget *button, gboolean desktop_showing);
GtkWidget* wayland_tasklist_new (void);
GtkWidget* wayland_selector_new (void);
GtkWidget* wayland_workspace_switcher_new (int size);
void wayland_workspace_switcher_set_size (GtkWidget* switcher_widget, int size);
void wayland_tasklist_set_orientation (GtkWidget* tasklist_widget, GtkOrientation orient);
void wayland_workspace_switcher_set_orientation (GtkWidget* switcher_widget, GtkOrientation orient);
void wayland_workspace_switcher_set_workspace_count (GtkWidget* switcher_widget, int count);
void wayland_workspace_switcher_set_n_rows (GtkWidget* switcher_widget, int n_rows);
void wayland_workspace_switcher_set_show_all (GtkWidget* switcher_widget, gboolean show_all);
void wayland_workspace_switcher_set_display_names (GtkWidget* switcher_widget, gboolean display_names);
int wayland_workspace_switcher_get_workspace_count (GtkWidget* switcher_widget);

#ifdef __cplusplus
}
#endif

#endif /* _WNCKLET_APPLET_WAYLAND_BACKEND_H_ */
