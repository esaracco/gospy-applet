/*
 * Copyright (C) 2004-2009 Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>
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

#ifndef _GOSPY_PROPERTIES_H
#define _GOSPY_PROPERTIES_H

#include "gospy_applet.h"

GSActionEmail *gs_properties_action_email_new (void);
void gs_properties_action_email_free (GSActionEmail * p);
void gs_properties_delete_files (const time_t timestamp);
GSChangeProperties *gs_properties_change_lookup_by_id (GList * list,
                                                       const guint id);
GSSpyProperties *gs_properties_spy_lookup_by_id (const guint32 id);
void gs_properties_free (const gboolean delete_files);
void gs_properties_changes_free (GList * list);
void gs_properties_alerts_free (GList * list);
void gs_properties_groups_free (void);
gpointer gs_properties_node_free (GSSpyProperties * prop,
                                  const gboolean delete_files);
void gs_properties_spies_save (void);
GList *gs_properties_spies_load (void);
void gs_properties_groups_save (void);
GList *gs_properties_groups_load (void);
GList *gs_properties_changes_load (const time_t timestamp, GList * list);
GList *gs_properties_alerts_load (const time_t timestamp, GList * list);
void gs_properties_spies_append (const guint option_server_type,
                                 const gchar * option_title,
                                 gchar * option_group,
                                 const gchar * option_target,
                                 const gboolean alert_ip_changed,
                                 const gboolean alert_server_changed,
                                 const guint option_web_page_type,
                                 const gchar * option_location,
                                 const gboolean alert_last_modified,
                                 const gboolean alert_size_changed,
                                 const gboolean alert_content_changed,
                                 const gboolean alert_status_changed,
                                 const GSCompareChoice alert_status_compare,
                                 const GSCompareChoice alert_size_compare,
                                 const GSHTMLTagsChoice alert_search_content,
                                 const GSCompareChoice alert_load_compare,
                                 const gint alert_scan_ports_compare,
                                 const guint alert_size_value,
                                 const guint alert_status_value,
                                 const guint alert_load_time_value,
                                 const gchar *
                                 alert_search_content_value,
                                 const guint
                                 alert_search_content_icase,
                                 const gchar * alert_scan_ports_value,
                                 const guint option_schedule,
                                 const gboolean use_auth,
                                 const gchar * auth_user,
                                 const gchar * auth_password);
#endif
