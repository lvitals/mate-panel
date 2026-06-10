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

#include <config.h>

#ifndef HAVE_WAYLAND
#error file should only be compiled when HAVE_WAYLAND is enabled
#endif

#include <gdk/gdkwayland.h>
#include <gio/gdesktopappinfo.h>

#include "wayland-backend.h"
#include "wayland-protocol/ext-workspace-v1-client.h"
#include "wayland-protocol/wlr-foreign-toplevel-management-unstable-v1-client.h"

/*shorter than wnck-tasklist due to common use of larger fonts*/
#define TASKLIST_TEXT_MAX_WIDTH 16

/*In the future this could be changable from the panel-prefs dialog*/
static const int max_button_width = 180;
static const int icon_size = 16;
int full_button_width;

typedef struct
{
	GtkWidget *menu;
	GtkWidget *maximize;
	GtkWidget *minimize;
	GtkWidget *on_top;
	GtkWidget *close;
} ContextMenu;

typedef enum
{
	TASKLIST_MODE_BUTTONS,
	TASKLIST_MODE_MENU
} TasklistMode;

typedef struct
{
	GtkWidget *list;
	GtkWidget *outer_box;
	ContextMenu *context_menu;
	struct zwlr_foreign_toplevel_manager_v1 *manager;
	TasklistMode mode;
} TasklistManager;

typedef struct
{
	GtkWidget *button;
	GtkWidget *icon;
	GtkWidget *label;
	struct zwlr_foreign_toplevel_handle_v1 *toplevel;
	gboolean active;
	gboolean maximized;
	gboolean minimized;
	gboolean fullscreen;
} ToplevelTask;

typedef struct
{
	GtkWidget *box;
	struct ext_workspace_manager_v1 *manager;
	struct ext_workspace_group_handle_v1 *group;
	GList *workspaces;
} WorkspaceManager;

typedef struct
{
	GtkWidget *button;
	GtkWidget *drawing_area;
	char *name;
	uint32_t state;
	uint32_t capabilities;
	struct ext_workspace_handle_v1 *workspace;
	WorkspaceManager *manager;
} WaylandWorkspace;

static int tasklist_invocations = 0;

static const char *tasklist_manager_key = "tasklist_manager";
static const char *toplevel_task_key = "toplevel_task";
static const char *workspace_manager_key = "workspace_manager";
static const char *wayland_workspace_key = "wayland_workspace";

static gboolean has_initialized = FALSE;
static struct wl_registry *wl_registry_global = NULL;
static struct wl_display *wl_display_global = NULL;
static uint32_t foreign_toplevel_manager_global_id = 0;
static uint32_t foreign_toplevel_manager_global_version = 0;
static uint32_t workspace_manager_global_id = 0;
static uint32_t workspace_manager_global_version = 0;

static ToplevelTask *toplevel_task_new (TasklistManager *tasklist, struct zwlr_foreign_toplevel_handle_v1 *handle);

guint buttons, tasklist_width;

gboolean window_hidden;

static void
wl_registry_handle_global (void *_data,
			   struct wl_registry *registry,
			   uint32_t id,
			   const char *interface,
			   uint32_t version)
{
	/* pull out needed globals */
	if (strcmp (interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
	{
		g_warn_if_fail (zwlr_foreign_toplevel_manager_v1_interface.version == 2);
		foreign_toplevel_manager_global_id = id;
		foreign_toplevel_manager_global_version =
			MIN((uint32_t)zwlr_foreign_toplevel_manager_v1_interface.version, version);
	}
	else if (strcmp (interface, ext_workspace_manager_v1_interface.name) == 0)
	{
		workspace_manager_global_id = id;
		workspace_manager_global_version =
			MIN((uint32_t)ext_workspace_manager_v1_interface.version, version);
	}
}

static void
wl_registry_handle_global_remove (void *_data,
				  struct wl_registry *_registry,
				  uint32_t id)
{
	if (id == foreign_toplevel_manager_global_id)
	{
		foreign_toplevel_manager_global_id = 0;
	}
	else if (id == workspace_manager_global_id)
	{
		workspace_manager_global_id = 0;
	}
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_handle_global,
    .global_remove = wl_registry_handle_global_remove,
};

static void
wayland_tasklist_init_if_needed (void)
{
	if (has_initialized)
		return;

	GdkDisplay *gdk_display = gdk_display_get_default ();
	g_return_if_fail (gdk_display);
	g_return_if_fail (GDK_IS_WAYLAND_DISPLAY (gdk_display));

	wl_display_global = gdk_wayland_display_get_wl_display (gdk_display);
	wl_registry_global = wl_display_get_registry (wl_display_global);
	wl_registry_add_listener (wl_registry_global, &wl_registry_listener, NULL);
	wl_display_roundtrip (wl_display_global);

	if (!foreign_toplevel_manager_global_id)
		g_warning ("%s not supported by Wayland compositor",
			   zwlr_foreign_toplevel_manager_v1_interface.name);

	if (!workspace_manager_global_id)
		g_warning ("%s not supported by Wayland compositor",
			   ext_workspace_manager_v1_interface.name);

	has_initialized = TRUE;
}

static void
foreign_toplevel_manager_handle_toplevel (void *data,
					  struct zwlr_foreign_toplevel_manager_v1 *manager,
					  struct zwlr_foreign_toplevel_handle_v1 *toplevel)
{
	TasklistManager *tasklist = data;
	ToplevelTask *task = toplevel_task_new (tasklist, toplevel);

	if (tasklist->mode == TASKLIST_MODE_MENU)
		gtk_menu_shell_append (GTK_MENU_SHELL (tasklist->list), task->button);
	else
		gtk_box_pack_start (GTK_BOX (tasklist->list), task->button, TRUE, TRUE, 0);
}

static void
foreign_toplevel_manager_handle_finished (void *data,
					  struct zwlr_foreign_toplevel_manager_v1 *manager)
{
	TasklistManager *tasklist = data;

	tasklist->manager = NULL;
	zwlr_foreign_toplevel_manager_v1_destroy (manager);

	if (tasklist->outer_box)
		g_object_set_data (G_OBJECT (tasklist->outer_box),
				   tasklist_manager_key,
				   NULL);

	g_free (tasklist);
}

static const struct zwlr_foreign_toplevel_manager_v1_listener foreign_toplevel_manager_listener = {
	.toplevel = foreign_toplevel_manager_handle_toplevel,
	.finished = foreign_toplevel_manager_handle_finished,
};

static void
tasklist_manager_disconnected_from_widget (TasklistManager *tasklist)
{
	if (tasklist->list)
	{
		if (GTK_IS_CONTAINER (tasklist->list))
		{
			GList *children = gtk_container_get_children (GTK_CONTAINER (tasklist->list));
			for (GList *iter = children; iter != NULL; iter = g_list_next (iter))
			{
				if (GTK_IS_WIDGET (iter->data))
					gtk_widget_destroy (GTK_WIDGET (iter->data));
			}
			g_list_free (children);
		}

		if (G_IS_OBJECT (tasklist->list))
			g_object_remove_weak_pointer (G_OBJECT (tasklist->list), (gpointer *)&tasklist->list);

		tasklist->list = NULL;
	}

	if (tasklist->outer_box)
	{
		if (G_IS_OBJECT (tasklist->outer_box))
			g_object_remove_weak_pointer (G_OBJECT (tasklist->outer_box), (gpointer *)&tasklist->outer_box);

		tasklist->outer_box = NULL;
	}

	if (tasklist->manager)
		zwlr_foreign_toplevel_manager_v1_stop (tasklist->manager);

	if (tasklist->context_menu)
	{
		if (tasklist->context_menu->menu)
		{
			if (GTK_IS_WIDGET (tasklist->context_menu->menu))
				gtk_widget_destroy (tasklist->context_menu->menu);

			if (tasklist->context_menu->menu && G_IS_OBJECT (tasklist->context_menu->menu))
				g_object_remove_weak_pointer (G_OBJECT (tasklist->context_menu->menu), (gpointer *)&tasklist->context_menu->menu);
		}

		g_free (tasklist->context_menu);
		tasklist->context_menu = NULL;
	}
}

static void
menu_on_maximize (GtkMenuItem *item, gpointer user_data)
{
	ToplevelTask *task = g_object_get_data (G_OBJECT (item), toplevel_task_key);
	if (task->toplevel) {
		if (task->maximized) {
			zwlr_foreign_toplevel_handle_v1_unset_maximized (task->toplevel);
		} else {
			zwlr_foreign_toplevel_handle_v1_set_maximized (task->toplevel);
		}
	}
}

static void
menu_on_minimize (GtkMenuItem *item, gpointer user_data)
{
	ToplevelTask *task = g_object_get_data (G_OBJECT (item), toplevel_task_key);
	if (task->toplevel) {
		if (task->minimized) {
			zwlr_foreign_toplevel_handle_v1_unset_minimized (task->toplevel);
		} else {
			zwlr_foreign_toplevel_handle_v1_set_minimized (task->toplevel);
		}
	}
}

static void
menu_on_close (GtkMenuItem *item, gpointer user_data)
{
	ToplevelTask *task = g_object_get_data (G_OBJECT (item), toplevel_task_key);
	if (task->toplevel) {
		zwlr_foreign_toplevel_handle_v1_close (task->toplevel);
	}
}

static ContextMenu *
context_menu_new ()
{
	ContextMenu *menu = g_new0 (ContextMenu, 1);
	menu->menu = gtk_menu_new ();
	menu->maximize = gtk_menu_item_new ();
	menu->minimize = gtk_menu_item_new ();
	menu->on_top = gtk_check_menu_item_new_with_label ("Always On Top");
	menu->close = gtk_menu_item_new_with_label ("Close");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), menu->maximize);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), menu->minimize);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), gtk_separator_menu_item_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), menu->on_top);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), gtk_separator_menu_item_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), menu->close);

	gtk_widget_show_all (menu->menu);

	g_object_add_weak_pointer (G_OBJECT (menu->menu), (gpointer *)&menu->menu);

	g_signal_connect (menu->maximize, "activate", G_CALLBACK (menu_on_maximize), NULL);
	g_signal_connect (menu->minimize, "activate", G_CALLBACK (menu_on_minimize), NULL);
	g_signal_connect (menu->close, "activate", G_CALLBACK (menu_on_close), NULL);
	gtk_widget_set_sensitive (menu->on_top, FALSE);
	return menu;
}

static TasklistManager *
tasklist_manager_new (TasklistMode mode)
{
	if (!foreign_toplevel_manager_global_id)
		return NULL;

	TasklistManager *tasklist = g_new0 (TasklistManager, 1);
	tasklist->mode = mode;

	if (mode == TASKLIST_MODE_MENU)
	{
		GtkWidget *image;

		tasklist->list = gtk_menu_new ();
		tasklist->outer_box = gtk_menu_button_new ();
		image = gtk_image_new_from_icon_name ("mate-panel-window-menu", GTK_ICON_SIZE_MENU);
		gtk_container_add (GTK_CONTAINER (tasklist->outer_box), image);
		gtk_menu_button_set_popup (GTK_MENU_BUTTON (tasklist->outer_box), tasklist->list);
		gtk_widget_show_all (tasklist->outer_box);
	}
	else
	{
		tasklist->list = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_set_homogeneous (GTK_BOX (tasklist->list), TRUE);
		tasklist->outer_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_size_request (tasklist->outer_box, icon_size * 3, -1);
		gtk_box_pack_start (GTK_BOX (tasklist->outer_box), tasklist->list, FALSE, FALSE, 0);
		gtk_widget_show (tasklist->list);
		tasklist->context_menu = context_menu_new ();
	}

	tasklist->manager = wl_registry_bind (wl_registry_global,
					     foreign_toplevel_manager_global_id,
					     &zwlr_foreign_toplevel_manager_v1_interface,
					     foreign_toplevel_manager_global_version);
	zwlr_foreign_toplevel_manager_v1_add_listener (tasklist->manager,
						       &foreign_toplevel_manager_listener,
						       tasklist);

	g_object_add_weak_pointer (G_OBJECT (tasklist->list), (gpointer *)&tasklist->list);
	g_object_add_weak_pointer (G_OBJECT (tasklist->outer_box), (gpointer *)&tasklist->outer_box);

	g_object_set_data_full (G_OBJECT (tasklist->outer_box),
				tasklist_manager_key,
				tasklist,
				(GDestroyNotify)tasklist_manager_disconnected_from_widget);
	wl_display_roundtrip (wl_display_global);
	return tasklist;
}

static void
foreign_toplevel_handle_title (void *data,
			       struct zwlr_foreign_toplevel_handle_v1 *toplevel,
			       const char *title)
{
	ToplevelTask *task = data;

	if (task->label)
	{
		gtk_label_set_label (GTK_LABEL (task->label), title);
	}
	else if (GTK_IS_MENU_ITEM (task->button))
	{
		gtk_menu_item_set_label (GTK_MENU_ITEM (task->button), title);
	}
}

static void
foreign_toplevel_handle_app_id (void *data,
				struct zwlr_foreign_toplevel_handle_v1 *toplevel,
				const char *app_id)
{
	ToplevelTask *task = data;

	if (!task->icon)
		return;

	gchar *app_id_lower = g_utf8_strdown (app_id, -1);
	gchar *desktop_app_id = g_strdup_printf ("%s.desktop", app_id_lower);
	GDesktopAppInfo *app_info = g_desktop_app_info_new (desktop_app_id);

	if (app_info) {
		GIcon *icon = g_app_info_get_icon (G_APP_INFO (app_info));
		if (icon) {
			gtk_image_set_from_gicon (GTK_IMAGE (task->icon), icon, GTK_ICON_SIZE_MENU);
			goto cleanup;
		}
	}
	gtk_image_set_from_icon_name (GTK_IMAGE (task->icon), app_id_lower, GTK_ICON_SIZE_MENU);

cleanup:
	if (app_info) {
		g_object_unref (G_OBJECT (app_info));
	}
	g_free (app_id_lower);
	g_free (desktop_app_id);
}

static void
foreign_toplevel_handle_output_enter (void *data,
				      struct zwlr_foreign_toplevel_handle_v1 *toplevel,
				      struct wl_output *output)
{
	/* ignore */
}

static void
foreign_toplevel_handle_output_leave (void *data,
				      struct zwlr_foreign_toplevel_handle_v1 *toplevel,
				      struct wl_output *output)
{
	/* ignore */
}

static void
foreign_toplevel_handle_state (void *data,
			       struct zwlr_foreign_toplevel_handle_v1 *toplevel,
			       struct wl_array *state)
{
	ToplevelTask *task = data;

	task->active = FALSE;
	task->maximized = FALSE;
	task->minimized = FALSE;
	task->fullscreen = FALSE;

	enum zwlr_foreign_toplevel_handle_v1_state *i;
	wl_array_for_each (i, state)
	{
		switch (*i)
		{
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:
			task->active = TRUE;
			break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:
			task->maximized = TRUE;
			break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:
			task->minimized = TRUE;
			break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN:
			task->fullscreen = TRUE;
			break;
		default:
			break;
		}
	}

	if (GTK_IS_BUTTON (task->button))
		gtk_button_set_relief (GTK_BUTTON (task->button), task->active ? GTK_RELIEF_NORMAL : GTK_RELIEF_NONE);
}

static void
foreign_toplevel_handle_done (void *data,
			      struct zwlr_foreign_toplevel_handle_v1 *toplevel)
{
	/* ignore */
}

static void
adjust_buttons (GtkContainer *outer_box, int button_space, int real_buttons, ToplevelTask *task)
{
	GtkWidget *widget, *button, *box;

	/*catch the case of an added button that can be missed
	 *Note that button space can come up zero on a first button
	 */
	if (real_buttons < 2)
	{
		if(task)
		{
			gtk_widget_set_size_request (task->button, full_button_width, -1);
		}
	}

	if ((task) && (button_space > 0) && (button_space < icon_size * 3))
	{
		gtk_widget_hide (task->icon);
	}
	else if (task)
	{
		gtk_widget_show (task->icon);
	}

	if ((task) && (button_space > 0) && (button_space < icon_size))
	{
		gtk_widget_hide (task->label);
	}
	else if (task)
	{
		gtk_widget_show (task->label);
	}

	GList* children = gtk_container_get_children (GTK_CONTAINER (outer_box));

	while (children != NULL)
	{
		button = GTK_WIDGET (children->data);
		box = gtk_bin_get_child (GTK_BIN (button));

		if ((real_buttons < 2) || (real_buttons * full_button_width < tasklist_width * 0.75))
		{
			gtk_widget_set_size_request (button, full_button_width, -1);
			gtk_widget_show_all (button);
			return;
		}
		else
		{
			gtk_widget_set_size_request (button, MIN(button_space, full_button_width), -1);
		}

		/* if the number of buttons forces width to less than 3x the icon size, hide the icons
		 * if the number of buttons forces width to less than the icon size, hide the labels too.
		 * This is roughy the same behavior as on x11
		 * To find the icon and label we must iterate through the children of the box we packed
		 * into the button, there are two of them
		 */

			GList* contents = gtk_container_get_children (GTK_CONTAINER (box));
			while (contents != NULL)
			{
				widget = GTK_WIDGET (contents->data);
				/*Show or hide the icon*/
				if (GTK_IS_IMAGE (widget))
				{
					if ((button_space < icon_size * 3) && (button_space > 1))
						gtk_widget_hide (widget);

					else
						gtk_widget_show (widget);

				}

				/*Show or hide the label*/
				if (GTK_IS_LABEL (widget))
				{
					if ((button_space < icon_size) && (button_space > 1))
					{
						gtk_widget_hide (widget);
						/*We can go a little wider for empty buttons*/
						gtk_widget_set_size_request (button, tasklist_width / real_buttons * 0.9, -1);
						if (task)
							gtk_widget_hide (task->label);

					}
					else
					{
						gtk_widget_show (widget);
					}
				}
				contents = contents->next;
			}
		children = children->next;
	}
	return;
}

static void
foreign_toplevel_handle_closed (void *data,
				struct zwlr_foreign_toplevel_handle_v1 *toplevel)
{
	ToplevelTask *task = data;

	if (task->button)
	{
		GtkOrientation orient;
		GtkWidget *outer_box, *parent_box;
		int real_buttons, button_space;

		outer_box = gtk_widget_get_parent (GTK_WIDGET (task->button));
		gtk_widget_destroy (task->button);

		if (!GTK_IS_BOX (outer_box))
			return;

		buttons = buttons -1;

		if (tasklist_invocations > 1)
			real_buttons = buttons / 2;

		else
			real_buttons = buttons;

		if (real_buttons == 0)
			return;

		/* We don't need to modify button size on a vertical panel*/
		orient = gtk_orientable_get_orientation (GTK_ORIENTABLE (outer_box));
		if (orient == GTK_ORIENTATION_VERTICAL)
			return;

		/*Get the box the tasklist outer box sits in
		 *and leave a little space so the buttons don't push other applets off the panel
		 */

		parent_box = gtk_widget_get_ancestor ((outer_box), GTK_TYPE_BOX);
		tasklist_width = MAX(gtk_widget_get_allocated_width (parent_box), tasklist_width) ;
		button_space = (tasklist_width / real_buttons) * 0.75;
		button_space = MIN(button_space, full_button_width);
		adjust_buttons (GTK_CONTAINER(outer_box), button_space, real_buttons, NULL);
	}
}

static const struct zwlr_foreign_toplevel_handle_v1_listener foreign_toplevel_handle_listener = {
	.title = foreign_toplevel_handle_title,
	.app_id = foreign_toplevel_handle_app_id,
	.output_enter = foreign_toplevel_handle_output_enter,
	.output_leave = foreign_toplevel_handle_output_leave,
	.state = foreign_toplevel_handle_state,
	.done = foreign_toplevel_handle_done,
	.closed = foreign_toplevel_handle_closed,
};

static void
toplevel_task_disconnected_from_widget (ToplevelTask *task)
{
	struct zwlr_foreign_toplevel_handle_v1 *toplevel = task->toplevel;

	task->button = NULL;
	task->icon = NULL;
	task->label = NULL;
	task->toplevel = NULL;

	if (toplevel)
		zwlr_foreign_toplevel_handle_v1_destroy (toplevel);

	g_free (task);
}

/*We have to use the "activate" signal here
 *as only signals valid for GtkButton work,
 *"clicked" is taken, and we need to separate
 *showing the desktop from mouse clicks on window buttons
 */
void
toggle_show_desktop(GtkWidget *button, gboolean desktop_showing)
{
	window_hidden = desktop_showing;
	g_signal_emit_by_name (button, "activate");
}

static void
toggle_window(GtkButton *button, ToplevelTask *task)
{
	if (task->toplevel)
	{
		if (window_hidden)
		{
			zwlr_foreign_toplevel_handle_v1_set_minimized (task->toplevel);
		}
		else
		{
			zwlr_foreign_toplevel_handle_v1_unset_minimized (task->toplevel);
		}
	}
}

static void
toplevel_task_handle_clicked (GtkButton *button, ToplevelTask *task)
{
	if (task->toplevel)
	{
		if (task->active)
		{
			zwlr_foreign_toplevel_handle_v1_set_minimized (task->toplevel);
		}
		else
		{
			GdkDisplay *gdk_display = gtk_widget_get_display (GTK_WIDGET (button));
			GdkSeat *gdk_seat = gdk_display_get_default_seat (gdk_display);
			struct wl_seat *wl_seat = gdk_wayland_seat_get_wl_seat (gdk_seat);
			zwlr_foreign_toplevel_handle_v1_activate (task->toplevel, wl_seat);
		}
	}
}

static void
toplevel_task_handle_activate (GtkMenuItem *item, ToplevelTask *task)
{
	if (task->toplevel)
	{
		GdkDisplay *gdk_display = gtk_widget_get_display (GTK_WIDGET (item));
		GdkSeat *gdk_seat = gdk_display_get_default_seat (gdk_display);
		struct wl_seat *wl_seat = gdk_wayland_seat_get_wl_seat (gdk_seat);
		zwlr_foreign_toplevel_handle_v1_activate (task->toplevel, wl_seat);
	}
}

static gboolean on_toplevel_button_press (GtkWidget *button, GdkEvent *event, TasklistManager *tasklist)
{
	/* Assume event is a button press */
	if (((GdkEventButton*)event)->button == GDK_BUTTON_SECONDARY)
	{
		ContextMenu *menu = tasklist->context_menu;
		ToplevelTask *task = g_object_get_data (G_OBJECT (button), toplevel_task_key);

		g_object_set_data (G_OBJECT (menu->maximize), toplevel_task_key, task);
		g_object_set_data (G_OBJECT (menu->minimize), toplevel_task_key, task);
		g_object_set_data (G_OBJECT (menu->close), toplevel_task_key, task);

		gtk_menu_item_set_label (GTK_MENU_ITEM (menu->minimize),
				task->minimized ? "Unminimize" : "Minimize");
		gtk_menu_item_set_label (GTK_MENU_ITEM (menu->maximize),
				task->maximized ? "Unmaximize" : "Maximize");

		gtk_menu_popup_at_widget (GTK_MENU (menu->menu), button,
				GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_SOUTH_WEST, event);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static ToplevelTask *
toplevel_task_new (TasklistManager *tasklist, struct zwlr_foreign_toplevel_handle_v1 *toplevel)
{
	ToplevelTask *task = g_new0 (ToplevelTask, 1);
	GtkOrientation orient;
	GtkWidget *whole_panel_box, *parent_box;
	int real_buttons, button_space, panel_width;

	if (tasklist->mode == TASKLIST_MODE_MENU)
	{
		task->button = gtk_menu_item_new_with_label ("");
		g_signal_connect (task->button, "activate", G_CALLBACK (toplevel_task_handle_activate), task);
		task->toplevel = toplevel;
		zwlr_foreign_toplevel_handle_v1_add_listener (toplevel,
							      &foreign_toplevel_handle_listener,
							      task);
		g_object_set_data_full (G_OBJECT (task->button),
					toplevel_task_key,
					task,
					(GDestroyNotify)toplevel_task_disconnected_from_widget);
		gtk_widget_show (task->button);
		return task;
	}

	buttons = buttons + 1;
	orient = gtk_orientable_get_orientation (GTK_ORIENTABLE (tasklist->outer_box));
	task->button = gtk_button_new ();
	g_signal_connect (task->button, "clicked", G_CALLBACK (toplevel_task_handle_clicked), task);
	g_signal_connect (task->button, "activate", G_CALLBACK (toggle_window), task);

	task->icon = gtk_image_new_from_icon_name ("unknown", icon_size);

	task->label = gtk_label_new ("");
	gtk_label_set_max_width_chars (GTK_LABEL (task->label), TASKLIST_TEXT_MAX_WIDTH);
	gtk_label_set_ellipsize (GTK_LABEL (task->label), PANGO_ELLIPSIZE_END);
	gtk_label_set_xalign (GTK_LABEL (task->label), 0.0);

	GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (box), task->icon, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX (box), task->label, TRUE, TRUE, 2);

	gtk_container_add (GTK_CONTAINER (task->button), box);
	gtk_widget_set_name (task->button , "tasklist-button");
	gtk_widget_show_all (task->button);

	task->toplevel = toplevel;
	zwlr_foreign_toplevel_handle_v1_add_listener (toplevel,
						      &foreign_toplevel_handle_listener,
						      task);
	g_object_set_data_full (G_OBJECT (task->button),
				toplevel_task_key,
				task,
				(GDestroyNotify)toplevel_task_disconnected_from_widget);

	g_signal_connect (task->button, "button-press-event",
			  G_CALLBACK (on_toplevel_button_press),
			  tasklist);

	/* Buttons on a vertical panel are not affected by how many are needed
	 * GTK handles compressing contents as needed as the window width tells
	 * GTK how much space to allocate the label and icon. Buttons will use
	 * the full width of a vertical panel without any special attention
	 * so break out here instead of breaking the vertical panel case
	 */

	if (orient == GTK_ORIENTATION_VERTICAL)
		return task;

	/* On horizontal panels, GTK does not by default limit the width of the tasklist
	 * as it does not run out of space in the window until the entire panel is used,
	 * leaving buttons at full width until then and overflowing all other applets
	 *
	 * Thus we must get the tasklist's allocated width when extra space remains,
	 * which will be most of the distance between the handle and the next applet
	 * From there, we can expand buttons and/or hide elements as needed
	 * For some reason this function always gets called twice, so use half the value of buttons
	 * but do not attempt to adjust the global value as it would get adjusted twice
	 * Since we are adding a button here the true value cannot be zero
	 */
	whole_panel_box = gtk_widget_get_toplevel(GTK_WIDGET (tasklist->outer_box));
	parent_box = gtk_widget_get_ancestor(GTK_WIDGET (tasklist->outer_box), GTK_TYPE_BOX);
	if (gtk_widget_get_allocated_width (parent_box) > 1)
	{
		tasklist_width = gtk_widget_get_allocated_width (parent_box);
	}
	else
	{
		tasklist_width = MAX(gtk_widget_get_allocated_width (parent_box), tasklist_width);
	}

	panel_width = gtk_widget_get_allocated_width (whole_panel_box);

	/*on startup we get an allocated with of zero, so start with 1/3 the panel width
	 *as a sane default
	 *This may overflow on very crowded panels where the tasklist is less than 1/3ed the
	 *panel witth but will self-correct on opening or closing a few windows
	 */

	if (tasklist_width <= 2)
		tasklist_width = panel_width / 3;

	if (tasklist_invocations > 1)
		real_buttons = MAX ((buttons / 2), 1);

	else
		real_buttons = MAX ((buttons), 1);

	/*always allow at least three buttons to fit without adjustment
	 *so short window lists don't overflow
	 */
	if (tasklist_width > 0)
	{
		full_button_width = MIN(max_button_width, tasklist_width / 3);
	}

	/*Leave a little space so the buttons don't push other applets off the panel*/
	button_space = (tasklist_width / real_buttons) * 0.75;
	button_space = MIN(button_space, full_button_width);

	/* iterate over all the buttons*/
	adjust_buttons (GTK_CONTAINER (tasklist->list), button_space, real_buttons, task);

	/*Reset the tasklist width after button adjustments*/
	if (gtk_widget_get_allocated_width (parent_box) > 1)
	{
		tasklist_width = gtk_widget_get_allocated_width (parent_box);
	}
	else
	{
		tasklist_width = MAX(gtk_widget_get_allocated_width (parent_box), tasklist_width);
	}
	return task;
}

GtkWidget*
wayland_tasklist_new ()
{
	wayland_tasklist_init_if_needed ();
	TasklistManager *tasklist = tasklist_manager_new (TASKLIST_MODE_BUTTONS);

	tasklist_invocations = tasklist_invocations + 1;
	if (!tasklist)
		return gtk_label_new ("Shell does not support WLR Foreign Toplevel Control");
	return tasklist->outer_box;
}

GtkWidget*
wayland_selector_new ()
{
	wayland_tasklist_init_if_needed ();
	TasklistManager *tasklist = tasklist_manager_new (TASKLIST_MODE_MENU);

	if (!tasklist)
		return gtk_label_new ("Shell does not support WLR Foreign Toplevel Control");
	return tasklist->outer_box;
}

static void
wayland_workspace_update_button (WaylandWorkspace *workspace)
{
	gboolean active;

	if (!workspace->button)
		return;

	active = (workspace->state & EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE) != 0;
	gtk_button_set_relief (GTK_BUTTON (workspace->button),
			       active ? GTK_RELIEF_NORMAL : GTK_RELIEF_NONE);
	gtk_widget_set_sensitive (workspace->button,
				  (workspace->capabilities & EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_ACTIVATE) != 0);
	if (workspace->drawing_area)
		gtk_widget_queue_draw (workspace->drawing_area);
}

static gboolean
wayland_workspace_draw (GtkWidget *widget, cairo_t *cr, WaylandWorkspace *workspace)
{
	GtkAllocation allocation;
	GtkStyleContext *context;
	GdkRGBA border;
	GdkRGBA fill;
	GdkRGBA window_fill;
	gboolean active;
	gboolean urgent;
	gint width;
	gint height;
	gint padding;
	gint win_w;
	gint win_h;

	gtk_widget_get_allocation (widget, &allocation);
	width = allocation.width;
	height = allocation.height;
	padding = 3;

	active = (workspace->state & EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE) != 0;
	urgent = (workspace->state & EXT_WORKSPACE_HANDLE_V1_STATE_URGENT) != 0;

	context = gtk_widget_get_style_context (widget);
	gtk_style_context_get_color (context,
				     gtk_widget_get_state_flags (widget),
				     &border);

	fill = border;
	fill.alpha = active ? 0.22 : 0.08;
	window_fill = border;
	window_fill.alpha = active ? 0.55 : 0.30;

	cairo_set_line_width (cr, active ? 2.0 : 1.0);
	gdk_cairo_set_source_rgba (cr, &fill);
	cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_rgba (cr, &border);
	cairo_stroke (cr);

	if (urgent)
	{
		GdkRGBA urgent_color = { 1.0, 0.25, 0.15, 0.45 };
		gdk_cairo_set_source_rgba (cr, &urgent_color);
		cairo_rectangle (cr, 2.5, 2.5, width - 5, height - 5);
		cairo_stroke (cr);
	}

	/* ext-workspace-v1 does not expose per-window geometry. Draw a compact
	 * content marker so the Wayland pager resembles the classic pager instead
	 * of showing workspace name buttons.
	 */
	win_w = MAX ((width - (padding * 3)) / 2, 4);
	win_h = MAX ((height - (padding * 3)) / 2, 3);

	gdk_cairo_set_source_rgba (cr, &window_fill);
	cairo_rectangle (cr, padding, padding, win_w, win_h);
	cairo_fill (cr);

	if (active)
	{
		cairo_rectangle (cr,
				 width - padding - win_w,
				 height - padding - win_h,
				 win_w,
				 win_h);
		cairo_fill (cr);
	}

	return FALSE;
}

static void
wayland_workspace_destroy (WaylandWorkspace *workspace)
{
	struct ext_workspace_handle_v1 *handle = workspace->workspace;

	workspace->button = NULL;
	workspace->drawing_area = NULL;
	workspace->workspace = NULL;

	if (workspace->manager)
		workspace->manager->workspaces =
			g_list_remove (workspace->manager->workspaces, workspace);

	if (handle)
		ext_workspace_handle_v1_destroy (handle);

	g_free (workspace->name);
	g_free (workspace);
}

static void
wayland_workspace_handle_id (void *data,
			     struct ext_workspace_handle_v1 *handle,
			     const char *id)
{
	/* not displayed */
}

static void
wayland_workspace_handle_name (void *data,
			       struct ext_workspace_handle_v1 *handle,
			       const char *name)
{
	WaylandWorkspace *workspace = data;

	g_free (workspace->name);
	workspace->name = g_strdup (name);
	gtk_widget_set_tooltip_text (workspace->button, workspace->name);
	if (workspace->drawing_area)
		gtk_widget_queue_draw (workspace->drawing_area);
}

static void
wayland_workspace_handle_coordinates (void *data,
				      struct ext_workspace_handle_v1 *handle,
				      struct wl_array *coordinates)
{
	/* Marco sends workspaces in configured order; no extra sorting needed. */
}

static void
wayland_workspace_handle_state (void *data,
				struct ext_workspace_handle_v1 *handle,
				uint32_t state)
{
	WaylandWorkspace *workspace = data;

	workspace->state = state;
	wayland_workspace_update_button (workspace);
}

static void
wayland_workspace_handle_capabilities (void *data,
				       struct ext_workspace_handle_v1 *handle,
				       uint32_t capabilities)
{
	WaylandWorkspace *workspace = data;

	workspace->capabilities = capabilities;
	wayland_workspace_update_button (workspace);
}

static void
wayland_workspace_handle_removed (void *data,
				  struct ext_workspace_handle_v1 *handle)
{
	WaylandWorkspace *workspace = data;

	if (workspace->button)
		gtk_widget_destroy (workspace->button);
}

static const struct ext_workspace_handle_v1_listener workspace_handle_listener = {
	.id = wayland_workspace_handle_id,
	.name = wayland_workspace_handle_name,
	.coordinates = wayland_workspace_handle_coordinates,
	.state = wayland_workspace_handle_state,
	.capabilities = wayland_workspace_handle_capabilities,
	.removed = wayland_workspace_handle_removed,
};

static void
wayland_workspace_button_clicked (GtkButton *button,
				  WaylandWorkspace *workspace)
{
	if (!workspace->workspace || !workspace->manager->manager)
		return;

	ext_workspace_handle_v1_activate (workspace->workspace);
	ext_workspace_manager_v1_commit (workspace->manager->manager);
}

static void
workspace_manager_handle_workspace (void *data,
				    struct ext_workspace_manager_v1 *manager,
				    struct ext_workspace_handle_v1 *handle)
{
	WorkspaceManager *workspace_manager = data;
	WaylandWorkspace *workspace;

	workspace = g_new0 (WaylandWorkspace, 1);
	workspace->workspace = handle;
	workspace->manager = workspace_manager;
	workspace_manager->workspaces = g_list_append (workspace_manager->workspaces, workspace);
	workspace->button = gtk_button_new ();
	workspace->drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (workspace->drawing_area, 48, 24);
	gtk_button_set_relief (GTK_BUTTON (workspace->button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (workspace->button), workspace->drawing_area);

	ext_workspace_handle_v1_add_listener (handle, &workspace_handle_listener, workspace);
	g_object_set_data_full (G_OBJECT (workspace->button),
				wayland_workspace_key,
				workspace,
				(GDestroyNotify)wayland_workspace_destroy);
	g_signal_connect (workspace->button, "clicked",
			  G_CALLBACK (wayland_workspace_button_clicked),
			  workspace);
	g_signal_connect (workspace->drawing_area, "draw",
			  G_CALLBACK (wayland_workspace_draw),
			  workspace);
	gtk_box_pack_start (GTK_BOX (workspace_manager->box), workspace->button, FALSE, FALSE, 0);
	gtk_widget_show_all (workspace->button);
}

static void
workspace_group_handle_capabilities (void *data,
				     struct ext_workspace_group_handle_v1 *group,
				     uint32_t capabilities)
{
	/* ignored */
}

static void
workspace_group_handle_output_enter (void *data,
				     struct ext_workspace_group_handle_v1 *group,
				     struct wl_output *output)
{
	/* ignored */
}

static void
workspace_group_handle_output_leave (void *data,
				     struct ext_workspace_group_handle_v1 *group,
				     struct wl_output *output)
{
	/* ignored */
}

static void
workspace_group_handle_workspace_enter (void *data,
					struct ext_workspace_group_handle_v1 *group,
					struct ext_workspace_handle_v1 *workspace)
{
	/* ignored */
}

static void
workspace_group_handle_workspace_leave (void *data,
					struct ext_workspace_group_handle_v1 *group,
					struct ext_workspace_handle_v1 *workspace)
{
	/* ignored */
}

static void
workspace_group_handle_removed (void *data,
				struct ext_workspace_group_handle_v1 *group)
{
	WorkspaceManager *workspace_manager = data;

	if (workspace_manager && workspace_manager->group == group)
		workspace_manager->group = NULL;

	ext_workspace_group_handle_v1_destroy (group);
}

static const struct ext_workspace_group_handle_v1_listener workspace_group_listener = {
	.capabilities = workspace_group_handle_capabilities,
	.output_enter = workspace_group_handle_output_enter,
	.output_leave = workspace_group_handle_output_leave,
	.workspace_enter = workspace_group_handle_workspace_enter,
	.workspace_leave = workspace_group_handle_workspace_leave,
	.removed = workspace_group_handle_removed,
};

static void
workspace_manager_handle_workspace_group (void *data,
					  struct ext_workspace_manager_v1 *manager,
					  struct ext_workspace_group_handle_v1 *group)
{
	WorkspaceManager *workspace_manager = data;

	if (!workspace_manager->group)
		workspace_manager->group = group;

	ext_workspace_group_handle_v1_add_listener (group, &workspace_group_listener, workspace_manager);
}

static void
workspace_manager_handle_done (void *data,
			       struct ext_workspace_manager_v1 *manager)
{
	/* ignored */
}

static void
workspace_manager_handle_finished (void *data,
				   struct ext_workspace_manager_v1 *manager)
{
	WorkspaceManager *workspace_manager = data;

	workspace_manager->manager = NULL;
	ext_workspace_manager_v1_destroy (manager);

	if (workspace_manager->box)
		g_object_set_data (G_OBJECT (workspace_manager->box),
				   workspace_manager_key,
				   NULL);

	g_free (workspace_manager);
}

static const struct ext_workspace_manager_v1_listener workspace_manager_listener = {
	.workspace_group = workspace_manager_handle_workspace_group,
	.workspace = workspace_manager_handle_workspace,
	.done = workspace_manager_handle_done,
	.finished = workspace_manager_handle_finished,
};

static void
workspace_manager_disconnected_from_widget (WorkspaceManager *workspace_manager)
{
	if (workspace_manager->box)
	{
		if (GTK_IS_CONTAINER (workspace_manager->box))
		{
			GList *children = gtk_container_get_children (GTK_CONTAINER (workspace_manager->box));
			for (GList *iter = children; iter != NULL; iter = g_list_next (iter))
			{
				if (GTK_IS_WIDGET (iter->data))
					gtk_widget_destroy (GTK_WIDGET (iter->data));
			}
			g_list_free (children);
		}

		if (G_IS_OBJECT (workspace_manager->box))
			g_object_remove_weak_pointer (G_OBJECT (workspace_manager->box), (gpointer *)&workspace_manager->box);

		workspace_manager->box = NULL;
	}

	g_clear_pointer (&workspace_manager->workspaces, g_list_free);

	if (workspace_manager->manager)
		ext_workspace_manager_v1_stop (workspace_manager->manager);
}

GtkWidget*
wayland_workspace_switcher_new ()
{
	WorkspaceManager *workspace_manager;

	wayland_tasklist_init_if_needed ();

	if (!workspace_manager_global_id)
		return gtk_label_new ("Shell does not support ext-workspace-v1");

	workspace_manager = g_new0 (WorkspaceManager, 1);
	workspace_manager->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	workspace_manager->manager = wl_registry_bind (wl_registry_global,
						      workspace_manager_global_id,
						      &ext_workspace_manager_v1_interface,
						      workspace_manager_global_version);
	ext_workspace_manager_v1_add_listener (workspace_manager->manager,
					       &workspace_manager_listener,
					       workspace_manager);

	g_object_add_weak_pointer (G_OBJECT (workspace_manager->box), (gpointer *)&workspace_manager->box);

	g_object_set_data_full (G_OBJECT (workspace_manager->box),
				workspace_manager_key,
				workspace_manager,
				(GDestroyNotify)workspace_manager_disconnected_from_widget);
	gtk_widget_show (workspace_manager->box);
	wl_display_roundtrip (wl_display_global);

	return workspace_manager->box;
}

static WorkspaceManager *
workspace_switcher_widget_get_manager (GtkWidget *switcher_widget)
{
	return g_object_get_data (G_OBJECT (switcher_widget), workspace_manager_key);
}

void
wayland_workspace_switcher_set_orientation (GtkWidget* switcher_widget, GtkOrientation orient)
{
	WorkspaceManager *workspace_manager = workspace_switcher_widget_get_manager (switcher_widget);

	if (!workspace_manager)
		return;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (workspace_manager->box), orient);
}

void
wayland_workspace_switcher_set_workspace_count (GtkWidget* switcher_widget, int count)
{
	WorkspaceManager *workspace_manager = workspace_switcher_widget_get_manager (switcher_widget);
	int current_count;

	if (!workspace_manager || !workspace_manager->manager || count < 1)
		return;

	current_count = g_list_length (workspace_manager->workspaces);

	while (current_count < count && workspace_manager->group)
	{
		char *name = g_strdup_printf ("Workspace %d", current_count + 1);
		ext_workspace_group_handle_v1_create_workspace (workspace_manager->group, name);
		g_free (name);
		current_count++;
	}

	while (current_count > count)
	{
		GList *last = g_list_last (workspace_manager->workspaces);
		WaylandWorkspace *workspace;

		if (!last)
			break;

		workspace = last->data;
		if (!workspace->workspace)
			break;

		ext_workspace_handle_v1_remove (workspace->workspace);
		current_count--;
	}

	ext_workspace_manager_v1_commit (workspace_manager->manager);
	wl_display_roundtrip (wl_display_global);
}

static TasklistManager *
tasklist_widget_get_tasklist (GtkWidget* tasklist_widget)
{
	return g_object_get_data (G_OBJECT (tasklist_widget), tasklist_manager_key);
}

void
wayland_tasklist_set_orientation (GtkWidget* tasklist_widget, GtkOrientation orient)
{
	TasklistManager *tasklist = tasklist_widget_get_tasklist (tasklist_widget);
	if (!tasklist)
		return;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (tasklist->list), orient);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (tasklist->outer_box), orient);
}
