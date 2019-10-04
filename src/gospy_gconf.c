/*
 * Copyright (C) 2004-2007 Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gospy_applet.h"

G_LOCK_DEFINE_STATIC (gs_gconf_lock);

void
gs_set_display_bg_color (const gchar * val)
{
  G_LOCK (gs_gconf_lock);
  panel_applet_gconf_set_string (gs->applet, "bg_color", val, NULL);
  G_UNLOCK (gs_gconf_lock);
}

void
gs_set_display_main_icon (const gboolean val)
{
  G_LOCK (gs_gconf_lock);
  panel_applet_gconf_set_bool (gs->applet, "display_main_icon", val, NULL);
  G_UNLOCK (gs_gconf_lock);
}

void
gs_set_display_spies_icons (const gboolean val)
{
  G_LOCK (gs_gconf_lock);
  panel_applet_gconf_set_bool (gs->applet, "display_spies_icons", val, NULL);
  G_UNLOCK (gs_gconf_lock);
}

void
gs_set_display_tooltips (const gboolean val)
{
  G_LOCK (gs_gconf_lock);
  panel_applet_gconf_set_bool (gs->applet, "display_tooltips", val, NULL);
  G_UNLOCK (gs_gconf_lock);
}

gchar *
gs_get_display_bg_color (void)
{
  return panel_applet_gconf_get_string (gs->applet, "bg_color", NULL);
}

gboolean
gs_get_display_main_icon (void)
{
  return panel_applet_gconf_get_bool (gs->applet, "display_main_icon", NULL);
}

gboolean
gs_get_display_spies_icons (void)
{
  return panel_applet_gconf_get_bool (gs->applet, "display_spies_icons", NULL);
}

gboolean
gs_get_display_tooltips (void)
{
  return panel_applet_gconf_get_bool (gs->applet, "display_tooltips", NULL);
}

gchar *
gs_get_gnome_browser_conf (void)
{
  GConfClient *client = NULL;
  gchar *ret = NULL;
  gchar *tmp = NULL;

  gdk_threads_enter ();
  G_LOCK (gs_gconf_lock);

  client = gconf_client_get_default ();

  if (!client)
  {
    g_warning ("%s\n", _("Could not get default gconf client"));
    goto handler_end;
  }

  ret = gconf_client_get_string (client,
                                 "/desktop/gnome/url-handlers/http/command",
                                 NULL);
  g_object_unref (client), client = NULL;

  if (ret != NULL && *ret != '\0')
    if ((tmp = strchr (ret, '%')) != NULL)
      *tmp = '\0';

handler_end:

  if (ret == NULL || *ret == '\0')
  {
    g_free (ret), ret = NULL;
    ret = g_strdup ("mozilla");
  }

  G_UNLOCK (gs_gconf_lock);
  gdk_threads_leave ();

  return ret;
}

void
gs_get_gnome_proxy_conf (gchar ** host, guint * port)
{
  GConfClient *client = NULL;

  gdk_threads_enter ();
  G_LOCK (gs_gconf_lock);

  client = gconf_client_get_default ();

  if (!client)
  {
    g_warning ("%s\n", _("Could not get default gconf client"));
    goto handler_end;
  }

  if (gconf_client_get_bool (client, "/system/http_proxy/use_http_proxy", NULL))
  {
    *host = gconf_client_get_string (client, "/system/http_proxy/host", NULL);
    *port = gconf_client_get_int (client, "/system/http_proxy/port", NULL);

    if (*port == 0)
      *port = 8080;
  }
  g_object_unref (client), client = NULL;

  if (*host == NULL)
    *host = g_strdup ("");

handler_end:

  G_UNLOCK (gs_gconf_lock);
  gdk_threads_leave ();
}
