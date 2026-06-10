/*
 * panel-session.c:
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2012-2021 MATE Developers
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
 *
 * Authors:
 *	Vincent Untz <vuntz@gnome.org>
 */

#include <gio/gio.h>
#include <unistd.h>
#include "panel-cleanup.h"
#include "panel-session-manager.h"

#define GSM_SERVICE_DBUS        "org.gnome.SessionManager"
#define GSM_PATH_DBUS           "/org/gnome/SessionManager"
#define GSM_INTERFACE_DBUS      "org.gnome.SessionManager"

#define MSM_SERVICE_DBUS        "org.mate.SessionManager"
#define MSM_PATH_DBUS           "/org/mate/SessionManager"
#define MSM_INTERFACE_DBUS      "org.mate.SessionManager"

struct _PanelSessionManager {
	GObject     parent;
	GDBusProxy *proxy;
	GDBusProxy *fallback_proxy;
};

G_DEFINE_TYPE (PanelSessionManager, panel_session_manager, G_TYPE_OBJECT)

static void
panel_session_manager_finalize (GObject *object)
{
	PanelSessionManager *manager = PANEL_SESSION_MANAGER (object);

	g_clear_object (&manager->proxy);
	g_clear_object (&manager->fallback_proxy);

	G_OBJECT_CLASS (panel_session_manager_parent_class)->finalize (object);
}

static void
panel_session_manager_class_init (PanelSessionManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = panel_session_manager_finalize;
}

static void
panel_session_manager_init (PanelSessionManager *manager)
{
	GError *error = NULL;

	manager->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
							G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
							NULL,
							GSM_SERVICE_DBUS,
							GSM_PATH_DBUS,
							GSM_INTERFACE_DBUS,
							NULL,
							&error);
	if (manager->proxy == NULL) {
		g_warning ("Unable to contact session manager daemon: %s\n", error->message);
		g_clear_error (&error);
	}

	manager->fallback_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
								G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
								NULL,
								MSM_SERVICE_DBUS,
								MSM_PATH_DBUS,
								MSM_INTERFACE_DBUS,
								NULL,
								&error);
	if (manager->fallback_proxy == NULL) {
		g_debug ("Unable to contact fallback session manager daemon: %s",
		         error->message);
		g_clear_error (&error);
	}
}

static gboolean
panel_session_manager_call (GDBusProxy  *proxy,
			    const char  *method,
			    GVariant    *parameters,
			    GVariant   **ret,
			    GError     **error)
{
	if (proxy == NULL)
		return FALSE;

	*ret = g_dbus_proxy_call_sync (proxy, method, parameters,
	                               G_DBUS_CALL_FLAGS_NONE,
	                               -1,
	                               NULL,
	                               error);

	return *ret != NULL;
}

static GVariant *
panel_session_manager_call_with_fallback (PanelSessionManager  *manager,
					  const char           *method,
					  GVariant             *parameters,
					  GVariant             *fallback_parameters,
					  GError              **error)
{
	GError *primary_error = NULL;
	GVariant *ret = NULL;

	if (panel_session_manager_call (manager->proxy, method, parameters, &ret, &primary_error))
		return ret;

	if (panel_session_manager_call (manager->fallback_proxy, method,
	                                fallback_parameters, &ret, error)) {
		g_clear_error (&primary_error);
		return ret;
	}

	if (*error == NULL && primary_error != NULL)
		g_propagate_error (error, primary_error);
	else
		g_clear_error (&primary_error);

	return NULL;
}

static gboolean
panel_session_manager_terminate_login1_session (GError **error)
{
	GDBusConnection *connection;
	GVariant *ret;
	const char *session_path = NULL;
	const char *session_id;
	GError *local_error = NULL;

	session_id = g_getenv ("XDG_SESSION_ID");
	connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, error);
	if (connection == NULL)
		return FALSE;

	if (session_id != NULL && *session_id != '\0') {
		ret = g_dbus_connection_call_sync (connection,
		                                   "org.freedesktop.login1",
		                                   "/org/freedesktop/login1",
		                                   "org.freedesktop.login1.Manager",
		                                   "TerminateSession",
		                                   g_variant_new ("(s)", session_id),
		                                   NULL,
		                                   G_DBUS_CALL_FLAGS_NONE,
		                                   -1,
		                                   NULL,
		                                   &local_error);
		if (ret != NULL) {
			g_variant_unref (ret);
			g_object_unref (connection);
			return TRUE;
		}

		g_clear_error (&local_error);
	}

	ret = g_dbus_connection_call_sync (connection,
	                                   "org.freedesktop.login1",
	                                   "/org/freedesktop/login1",
	                                   "org.freedesktop.login1.Manager",
	                                   "GetSessionByPID",
	                                   g_variant_new ("(u)", (guint32) getpid ()),
	                                   G_VARIANT_TYPE ("(o)"),
	                                   G_DBUS_CALL_FLAGS_NONE,
	                                   -1,
	                                   NULL,
	                                   error);
	if (ret == NULL) {
		g_object_unref (connection);
		return FALSE;
	}

	g_variant_get (ret, "(&o)", &session_path);
	if (session_path == NULL || *session_path == '\0') {
		g_variant_unref (ret);
		g_object_unref (connection);
		return FALSE;
	}

	GVariant *terminate_ret = g_dbus_connection_call_sync (connection,
	                                                       "org.freedesktop.login1",
	                                                       session_path,
	                                                       "org.freedesktop.login1.Session",
	                                                       "Terminate",
	                                                       NULL,
	                                                       NULL,
	                                                       G_DBUS_CALL_FLAGS_NONE,
	                                                       -1,
	                                                       NULL,
	                                                       error);

	g_variant_unref (ret);
	g_object_unref (connection);

	if (terminate_ret == NULL)
		return FALSE;

	g_variant_unref (terminate_ret);
	return TRUE;
}

void
panel_session_manager_request_logout (PanelSessionManager           *manager,
				      PanelSessionManagerLogoutType  mode)
{
	GError *error = NULL;
	GError *login1_error = NULL;
	GVariant *ret;

	g_return_if_fail (PANEL_IS_SESSION_MANAGER (manager));

	if (manager->proxy == NULL && manager->fallback_proxy == NULL) {
		if (!panel_session_manager_terminate_login1_session (&login1_error) &&
		    login1_error) {
			g_warning ("Could not terminate login1 session: %s",
			           login1_error->message);
			g_error_free (login1_error);
		}
		return;
	}

	ret = panel_session_manager_call_with_fallback (manager, "Logout",
							g_variant_new ("(u)", mode),
							g_variant_new ("(u)", mode),
							&error);
	if (ret == NULL) {
		if (panel_session_manager_terminate_login1_session (&login1_error)) {
			g_clear_error (&error);
			return;
		}

		if (error)
			g_warning ("Could not ask session manager to log out: %s",
			           error->message);
		if (login1_error) {
			g_warning ("Could not terminate login1 session: %s",
			           login1_error->message);
			g_error_free (login1_error);
		}
		g_clear_error (&error);
	} else {
		g_variant_unref (ret);
	}
}

void
panel_session_manager_request_shutdown (PanelSessionManager *manager)
{
	GError *error = NULL;
	GVariant *ret;

	g_return_if_fail (PANEL_IS_SESSION_MANAGER (manager));

	if (manager->proxy == NULL && manager->fallback_proxy == NULL)
		return;

	ret = panel_session_manager_call_with_fallback (manager, "Shutdown",
							g_variant_new ("()"),
							g_variant_new ("()"),
							&error);
	if (ret == NULL) {
		g_warning ("Could not ask session manager to shut down: %s",
			   error->message);
		g_error_free (error);
	} else {
		g_variant_unref (ret);
	}
}

gboolean
panel_session_manager_is_shutdown_available (PanelSessionManager *manager)
{
	GError *error = NULL;
	gboolean is_shutdown_available;
	GVariant *ret;

	g_return_val_if_fail (PANEL_IS_SESSION_MANAGER (manager), FALSE);

	if (manager->proxy == NULL && manager->fallback_proxy == NULL)
		return FALSE;

	ret = panel_session_manager_call_with_fallback (manager, "CanShutdown",
							g_variant_new ("()"),
							g_variant_new ("()"),
							&error);
	if (ret == NULL) {
		g_warning ("Could not ask session manager if shut down is available: %s",
			   error->message);
		g_error_free (error);
		return FALSE;
	} else {
		g_variant_get (ret, "(b)", &is_shutdown_available);
		g_variant_unref (ret);
	}

	return is_shutdown_available;
}

PanelSessionManager *
panel_session_manager_get (void)
{
	static PanelSessionManager *manager = NULL;

	if (manager == NULL) {
		manager = g_object_new (PANEL_TYPE_SESSION_MANAGER, NULL);
		panel_cleanup_register (panel_cleanup_unref_and_nullify,
					&manager);
	}

	return manager;
}
