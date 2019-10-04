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

#include "gospy_properties.h"
#include "gospy_process.h"

G_LOCK_DEFINE_STATIC (gs_properties_lock);

static xmlDocPtr gs_properties_xml_get_doc (gchar * docname);
static xmlXPathObjectPtr gs_properties_xml_get_nodeset (xmlDocPtr doc,
                                                        xmlNodePtr node,
                                                        xmlChar * xpath);
static void gs_properties_xml_get_alerts (xmlDocPtr doc, xmlNodePtr node,
                                          GSSpyProperties * p);
static void gs_properties_xml_get_actions (xmlDocPtr doc, xmlNodePtr node,
                                           GSSpyProperties * p);
static GSSpyProperties *gs_properties_node_new (void);
static GSChangeProperties *gs_properties_changes_node_new (void);
static GSAlertProperties *gs_properties_alerts_node_new (void);
static void
gs_properties_xml_get_spy_prop (xmlDocPtr doc, xmlNodePtr node,
                                GSSpyProperties * p);
static void
gs_properties_xml_write_attribute (xmlTextWriterPtr writer,
                                   const gchar * name,
                                   const gchar * value_char,
                                   const guint value_int);
static GList *gs_properties_groups_load_real (GList * list, xmlDocPtr doc,
                                              xmlNodePtr cur);


void
gs_properties_spies_append (const guint option_server_type,
                            const gchar * option_title,
                            gchar * option_group,
                            const gchar * option_server_name,
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
                            const GSHTMLTagsChoice alert_search_content_in,
                            const GSCompareChoice alert_load_compare,
                            const gint alert_scan_ports_compare,
                            const guint alert_size_value,
                            const guint alert_status_value,
                            const guint alert_load_time_value,
                            const gchar * alert_search_content_value,
                            const guint alert_search_content_icase,
                            const gchar * alert_scan_ports_value,
                            const guint option_schedule,
                            const gboolean use_auth,
                            const gchar * auth_user,
                            const gchar * auth_password)
{
  GSSpyProperties *p = NULL;

  p = gs_properties_node_new ();

  p->action_email = gs_properties_action_email_new ();

  p->schedule = option_schedule;
  p->timestamp = time (NULL);
  p->timestamp_update = 0;
  p->timestamp_last_event = 0;
  p->timestamp_last_exec = 0;
  p->timestamp_next_exec = 0;
  p->title = g_strdup (option_title);
  // if != NULL, option_group has already been allocated
  p->group = (option_group == NULL) ? g_strdup (" ") : option_group;
  p->alert_connection_error = FALSE;

  /* server type */
  if (option_server_type == 1)
  {
    p->type = GS_TYPE_SERVER;
    p->target = g_strdup (option_server_name);
    p->ip_changed = (gboolean) alert_ip_changed;
    p->server_changed = (gboolean) alert_server_changed;
    p->scan_ports_compare = alert_scan_ports_compare;
    p->scan_ports = g_strdup (alert_scan_ports_value);
  }
  /* web page type */
  else
  {
    p->type = GS_TYPE_WEB_PAGE;
    p->target = g_strdup (option_location);
    p->last_modified = (gboolean) alert_last_modified;
    p->size_changed = (gboolean) alert_size_changed;
    p->content_changed = (gboolean) alert_content_changed;
    p->status_changed = (gboolean) alert_status_changed;
    p->size = alert_size_value;
    p->size_compare = alert_size_compare;
    p->search_content_in = alert_search_content_in;
    p->load_compare = alert_load_compare;
    p->status_compare = alert_status_compare;
    p->status = alert_status_value;
    p->load_time = alert_load_time_value;
    p->search_content = g_strdup (alert_search_content_value);
    p->search_content_icase = alert_search_content_icase;
    p->use_auth = use_auth;
    if (use_auth)
    {
      p->auth_user = g_strdup (auth_user);
      p->auth_password = g_strdup (auth_password);
      p->basic_auth_line = gs_build_basic_auth_line (auth_user, auth_password);
    }
  }

  gs_split_uri (p->target, &p->scheme, &p->port, &p->hostname, &p->path);

  G_LOCK (gs_properties_lock);
  gs->spies_list = g_list_prepend (gs->spies_list, p);
  G_UNLOCK (gs_properties_lock);

  gs_add_spy_image (p);

  gs_spy_set_scheduling (p);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

GSChangeProperties *
gs_properties_change_lookup_by_id (GList * list, const guint id)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL)
  {
    GSChangeProperties *p = (GSChangeProperties *) item->data;

    if (p->id == id)
      return p;

    item = g_list_next (item);
  }

  return NULL;
}

GSSpyProperties *
gs_properties_spy_lookup_by_id (const guint32 id)
{
  GList *item = NULL;

  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;

    if (p->timestamp == id)
      return p;

    item = g_list_next (item);
  }

  return NULL;
}

void
gs_properties_action_email_free (GSActionEmail * p)
{
  if (p == NULL)
    return;

  g_free (p->address), p->address = NULL;
  g_free (p->server), p->server = NULL;
  g_free (p->login), p->login = NULL;
  g_free (p->password), p->password = NULL;

  g_free (p), p = NULL;
}

gpointer
gs_properties_node_free (GSSpyProperties * p, const gboolean delete_files)
{
  if (p == NULL)
    return NULL;

  if (delete_files)
    gs_properties_delete_files (p->timestamp);

  if (p->timeout_id)
    g_source_remove (p->timeout_id), p->timeout_id = 0;
  if (p->event_box != NULL)
    gs_remove_spy_image (p);
  if (p->changes_list != NULL)
    gs_properties_changes_free (p->changes_list), p->changes_list = NULL;
  if (p->alerts_list != NULL)
    gs_properties_alerts_free (p->alerts_list), p->alerts_list = NULL;
  g_free (p->target), p->target = NULL;
  g_free (p->scheme), p->scheme = NULL;
  g_free (p->hostname), p->hostname = NULL;
  g_free (p->auth_user), p->auth_user = NULL;
  g_free (p->auth_password), p->auth_password = NULL;
  g_free (p->basic_auth_line), p->basic_auth_line = NULL;
  g_free (p->path), p->path = NULL;
  g_free (p->title), p->title = NULL;
  g_free (p->group), p->group = NULL;
  g_free (p->search_content), p->search_content = NULL;
  g_free (p->scan_ports), p->scan_ports = NULL;
  g_free (p->old_ip), p->old_ip = NULL;
  g_free (p->new_ip), p->new_ip = NULL;
  g_free (p->old_server), p->old_server = NULL;
  g_free (p->new_server), p->new_server = NULL;
  g_free (p->old_last_modified), p->old_last_modified = NULL;
  g_free (p->new_last_modified), p->new_last_modified = NULL;
  g_free (p->old_content_checksum_changed),
    p->old_content_checksum_changed = NULL;
  g_free (p->new_content_checksum_changed),
    p->new_content_checksum_changed = NULL;
  if (p->action_email != NULL)
    gs_properties_action_email_free (p->action_email), p->action_email = NULL;

  G_LOCK (gs_properties_lock);
  gs->spies_list = g_list_remove (gs->spies_list, p);
  G_UNLOCK (gs_properties_lock);

  g_free (p), p = NULL;

  return NULL;
}

void
gs_properties_delete_files (const time_t timestamp)
{
  DIR *dir = NULL;
  struct dirent *file = NULL;
  gchar *path = NULL;
  gchar *to_delete = NULL;

  to_delete = g_strdup_printf ("%u", (guint) timestamp);
  path = g_strdup_printf ("%s/logs/", gs->home_path);

  dir = opendir (path);
  while ((file = readdir (dir)) != NULL)
    if (strstr (file->d_name, to_delete))
    {
      gchar *file_path = g_strconcat (path, file->d_name, NULL);
      unlink (file_path);
      g_free (file_path), file_path = NULL;
    }
  closedir (dir);

  g_free (path), path = NULL;
  g_free (to_delete), to_delete = NULL;
}

void
gs_properties_alerts_free (GList * list)
{
  GList *item = NULL;

  if (list == NULL)
    return;

  item = g_list_first (list);
  while (item != NULL)
  {
    GSAlertProperties *a = (GSAlertProperties *) item->data;

    item = g_list_next (item);

    g_free (a->date_create), a->date_create = NULL;
    g_free (a->time_create), a->time_create = NULL;
    g_free (a->value), a->value = NULL;

    list = g_list_remove (list, a);

    g_free (a), a = NULL;
  }

  g_list_free (list), list = NULL;
}

void
gs_properties_changes_free (GList * list)
{
  GList *item = NULL;

  if (list == NULL)
    return;

  item = g_list_first (list);
  while (item != NULL)
  {
    GSChangeProperties *p = (GSChangeProperties *) item->data;

    item = g_list_next (item);

    g_free (p->file_basename), p->file_basename = NULL;
    g_free (p->file_diff_suffix), p->file_diff_suffix = NULL;
    g_free (p->date_create), p->date_create = NULL;
    g_free (p->time_create), p->time_create = NULL;
    g_free (p->old_value), p->old_value = NULL;
    g_free (p->new_value), p->new_value = NULL;

    list = g_list_remove (list, p);

    g_free (p), p = NULL;
  }

  g_list_free (list), list = NULL;
}


void
gs_properties_groups_free (void)
{
  if (gs->groups_list != NULL)
  {
    g_list_foreach (gs->groups_list, (GFunc) g_free, NULL);
    g_list_free (gs->groups_list), gs->groups_list = NULL;
  }
}


void
gs_properties_free (const gboolean delete_files)
{
  GList *item = NULL;

  /* Free spies */
  if (gs->spies_list != NULL)
  {
    item = g_list_first (gs->spies_list);
    while (item != NULL)
    {
      GSSpyProperties *p = (GSSpyProperties *) item->data;

      item = g_list_next (item);

      /* cancel thread if this spy is running */
      if (p->task == GS_TASK_RUNNING)
      {
        pthread_cancel (p->thread_id);
        g_usleep (1000);
        G_LOCK (gs_properties_lock);
        p->task = GS_TASK_SLEEPING;
        G_UNLOCK (gs_properties_lock);
      }

      gs_properties_node_free (p, delete_files);
    }

    G_LOCK (gs_properties_lock);
    g_list_free (gs->spies_list), gs->spies_list = NULL;
    G_UNLOCK (gs_properties_lock);
  }
}

static void
gs_properties_xml_write_attribute (xmlTextWriterPtr writer,
                                   const gchar * name,
                                   const gchar * value_char,
                                   const guint value_int)
{
  gchar *tmp = NULL;


  tmp = (value_char == NULL) ?
    g_strdup_printf ("%u", value_int) : gs_convert_to_utf8 (value_char);

  xmlTextWriterWriteAttribute (writer, BAD_CAST name, BAD_CAST tmp);
  g_free (tmp), tmp = NULL;
}

void
gs_properties_groups_save (void)
{
  GList *item = NULL;
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;
  gchar *file = NULL;


  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, XML_ENCODING, NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "groups");

  item = g_list_first (gs->groups_list);
  while (item != NULL)
  {
    xmlTextWriterWriteElement (writer, BAD_CAST "group",
                               BAD_CAST (xmlChar *) item->data);

    item = g_list_next (item);
  }

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer);
  file = g_strconcat (gs->home_path, "/", XML_GROUPS_FILENAME, NULL);
  xmlSaveFormatFileEnc (file, doc, XML_ENCODING, 1);
  g_free (file), file = NULL;
  xmlFreeDoc (doc);
}

void
gs_properties_spies_save (void)
{
  GList *item = NULL;
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;
  gchar *file = NULL;

  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, XML_ENCODING, NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "spys");

  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;

    /* * * - SPY */
    xmlTextWriterStartElement (writer, BAD_CAST "spy");
    /* timestamp */
    gs_properties_xml_write_attribute (writer, "timestamp", NULL, p->timestamp);
    /* timestamp_update */
    gs_properties_xml_write_attribute (writer, "timestamp_update", NULL,
                                       p->timestamp_update);
    /* timestamp_last_event */
    gs_properties_xml_write_attribute (writer, "timestamp_last_event", NULL,
                                       p->timestamp_last_event);
    /* timestamp_last_exec */
    gs_properties_xml_write_attribute (writer, "timestamp_last_exec", NULL,
                                       p->timestamp_last_exec);
    /* type */
    gs_properties_xml_write_attribute (writer, "type", NULL, p->type);
    /* state */
    gs_properties_xml_write_attribute (writer, "state", NULL, p->state);
    /* title */
    gs_properties_xml_write_attribute (writer, "title",
                                       (p->title) ? p->title : "", 0);
    /* Group */
    gs_properties_xml_write_attribute (writer, "group",
                                       (p->group) ? p->group : "", 0);
    /* target */
    gs_properties_xml_write_attribute (writer, "target",
                                       (p->target) ? p->target : "", 0);
    /* schedule */
    gs_properties_xml_write_attribute (writer, "schedule", NULL, p->schedule);

    /* ** - URI */
    xmlTextWriterStartElement (writer, BAD_CAST "uri");

    /* authentication */
    gs_properties_xml_write_attribute (writer, "use_auth", NULL, p->use_auth);
    if (p->use_auth)
    {
      gs_properties_xml_write_attribute (writer, "auth_user", p->auth_user, 0);
      gs_properties_xml_write_attribute (writer, "auth_password",
                                         p->auth_password, 0);
    }
    else
    {
      gs_properties_xml_write_attribute (writer, "auth_user", "", 0);
      gs_properties_xml_write_attribute (writer, "auth_password", "", 0);
    }

    /* scheme */
    gs_properties_xml_write_attribute (writer, "scheme",
                                       (p->scheme) ? p->scheme : "", 0);
    /* hostname */
    gs_properties_xml_write_attribute (writer, "hostname",
                                       (p->hostname) ? p->hostname : "", 0);
    /* port */
    gs_properties_xml_write_attribute (writer, "port", NULL, p->port);
    /* path */
    gs_properties_xml_write_attribute (writer, "path",
                                       (p->path) ? p->path : "", 0);
    /* ** */
    xmlTextWriterEndElement (writer);

    /* ** - ACTIONS */
    xmlTextWriterStartElement (writer, BAD_CAST "actions");

    /* E-Mail */
    if (p->action_email != NULL)
    {
      xmlTextWriterStartElement (writer, BAD_CAST "email");
      gs_properties_xml_write_attribute (writer, "enabled", NULL,
                                         (guint) p->action_email->enabled);
      gs_properties_xml_write_attribute (writer, "address",
                                         (p->action_email->address) ? p->
                                         action_email->address : "", 0);
      gs_properties_xml_write_attribute (writer, "server",
                                         (p->action_email->server) ? p->
                                         action_email->server : "", 0);
      gs_properties_xml_write_attribute (writer, "port", NULL,
                                         p->action_email->port);
      gs_properties_xml_write_attribute (writer, "login",
                                         (p->action_email->login) ? p->
                                         action_email->login : "", 0);
      gs_properties_xml_write_attribute (writer, "password",
                                         (p->action_email->password) ? p->
                                         action_email->password : "", 0);
      xmlTextWriterEndElement (writer);
    }

    xmlTextWriterEndElement (writer);

    /* ** - ALERTS */
    xmlTextWriterStartElement (writer, BAD_CAST "alerts");
    if (p->type == GS_TYPE_SERVER)
    {
      /* ip changed */
      xmlTextWriterStartElement (writer, BAD_CAST "ip_changed");
      gs_properties_xml_write_attribute (writer, "check", NULL, p->ip_changed);
      gs_properties_xml_write_attribute (writer, "old",
                                         (p->old_ip) ? p->old_ip : "", 0);
      gs_properties_xml_write_attribute (writer, "new",
                                         (p->new_ip) ? p->new_ip : "", 0);
      xmlTextWriterEndElement (writer);

      /* server changed */
      xmlTextWriterStartElement (writer, BAD_CAST "server_changed");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL, p->server_changed);
      gs_properties_xml_write_attribute (writer, "old",
                                         (p->old_server) ? p->old_server : "",
                                         0);
      gs_properties_xml_write_attribute (writer, "new",
                                         (p->new_server) ? p->new_server : "",
                                         0);
      xmlTextWriterEndElement (writer);

      /* scan ports */
      xmlTextWriterStartElement (writer, BAD_CAST "scan_ports");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL,
                                         (GS_SPY_HAVE_SCAN_PORTS (p)) ? 1 : 0);
      gs_properties_xml_write_attribute (writer, "value",
                                         (GS_SPY_HAVE_SCAN_PORTS (p)) ?
                                         p->scan_ports : "", 0);
      gs_properties_xml_write_attribute (writer, "compare", NULL,
                                         p->scan_ports_compare);
      xmlTextWriterEndElement (writer);
    }
    else
    {
      /* last modified */
      xmlTextWriterStartElement (writer, BAD_CAST "last_modified");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL, p->last_modified);
      gs_properties_xml_write_attribute (writer, "old",
                                         (p->old_last_modified) ?
                                         p->old_last_modified : "", 0);
      gs_properties_xml_write_attribute (writer, "new",
                                         (p->new_last_modified) ?
                                         p->new_last_modified : "", 0);
      xmlTextWriterEndElement (writer);

      /* size changed */
      xmlTextWriterStartElement (writer, BAD_CAST "size_changed");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL, p->size_changed);
      gs_properties_xml_write_attribute (writer, "old",
                                         NULL, p->old_size_changed);
      gs_properties_xml_write_attribute (writer, "new",
                                         NULL, p->new_size_changed);
      xmlTextWriterEndElement (writer);

      /* content changed */
      xmlTextWriterStartElement (writer, BAD_CAST "content_changed");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL, p->content_changed);
      gs_properties_xml_write_attribute (writer, "old",
                                         (p->old_content_checksum_changed) ?
                                         p->old_content_checksum_changed : "",
                                         0);
      gs_properties_xml_write_attribute (writer, "new",
                                         (p->new_content_checksum_changed) ?
                                         p->new_content_checksum_changed : "",
                                         0);
      xmlTextWriterEndElement (writer);

      /* size */
      xmlTextWriterStartElement (writer, BAD_CAST "size");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL,
                                         ((GS_SPY_HAVE_SIZE (p)) ? 1 : 0));
      gs_properties_xml_write_attribute (writer, "value", NULL, p->size);
      gs_properties_xml_write_attribute (writer, "compare",
                                         NULL, p->size_compare);
      xmlTextWriterEndElement (writer);

      /* status changed */
      xmlTextWriterStartElement (writer, BAD_CAST "status_changed");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL, p->status_changed);
      gs_properties_xml_write_attribute (writer, "old",
                                         NULL, p->old_status_changed);
      gs_properties_xml_write_attribute (writer, "new",
                                         NULL, p->new_status_changed);
      xmlTextWriterEndElement (writer);

      /* status */
      xmlTextWriterStartElement (writer, BAD_CAST "status");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL,
                                         ((GS_SPY_HAVE_STATUS (p)) ? 1 : 0));
      gs_properties_xml_write_attribute (writer, "value", NULL, p->status);
      gs_properties_xml_write_attribute (writer, "compare", NULL,
                                         p->status_compare);
      xmlTextWriterEndElement (writer);

      /* load time */
      xmlTextWriterStartElement (writer, BAD_CAST "load_time");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL,
                                         (GS_SPY_HAVE_LOAD_TIME (p)) ? 1 : 0);
      gs_properties_xml_write_attribute (writer, "value", NULL, p->load_time);
      gs_properties_xml_write_attribute (writer, "compare", NULL,
                                         p->load_compare);
      xmlTextWriterEndElement (writer);

      /* search content */
      xmlTextWriterStartElement (writer, BAD_CAST "search_content");
      gs_properties_xml_write_attribute (writer, "check",
                                         NULL,
                                         (GS_SPY_HAVE_SEARCH_CONTENT (p)) ? 1 :
                                         0);
      gs_properties_xml_write_attribute (writer, "value",
                                         (GS_SPY_HAVE_SEARCH_CONTENT (p)) ?
                                         p->search_content : "", 0);
      gs_properties_xml_write_attribute (writer, "in", NULL,
                                         p->search_content_in);
      gs_properties_xml_write_attribute (writer, "icase", NULL,
                                         p->search_content_icase);
      xmlTextWriterEndElement (writer);
    }
    /* ** */
    xmlTextWriterEndElement (writer);

    /* * */
    xmlTextWriterEndElement (writer);

    item = g_list_next (item);
  }

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer);
  file = g_strconcat (gs->home_path, "/", XML_MAIN_FILENAME, NULL);
  xmlSaveFormatFileEnc (file, doc, XML_ENCODING, 1);
  g_free (file), file = NULL;
  xmlFreeDoc (doc);
}

GSActionEmail *
gs_properties_action_email_new (void)
{
  GSActionEmail *p = NULL;

  p = g_new0 (GSActionEmail, 1);
  p->enabled = FALSE;
  p->address = NULL;
  p->server = NULL;
  p->port = 25;
  p->login = NULL;
  p->password = NULL;

  return p;
}

static GSAlertProperties *
gs_properties_alerts_node_new (void)
{
  GSAlertProperties *p = NULL;

  p = g_new0 (GSAlertProperties, 1);
  p->alert_type = GS_ALERT_NONE;
  p->date_create = NULL;
  p->time_create = NULL;
  p->value = NULL;

  return p;
}

static GSChangeProperties *
gs_properties_changes_node_new (void)
{
  GSChangeProperties *p = NULL;

  p = g_new0 (GSChangeProperties, 1);
  p->alert_type = GS_ALERT_NONE;
  p->file_basename = NULL;
  p->file_diff_suffix = NULL;
  p->date_create = NULL;
  p->time_create = NULL;
  p->old_value = NULL;
  p->new_value = NULL;

  return p;
}

static GSSpyProperties *
gs_properties_node_new (void)
{
  GSSpyProperties *p = NULL;

  p = g_new0 (GSSpyProperties, 1);
  p->image = NULL;
  p->event_box = NULL;
  p->tooltip = NULL;
  p->timestamp = 0;
  p->timestamp_update = 0;
  p->timestamp_last_event = 0;
  p->timestamp_last_exec = 0;
  p->timestamp_next_exec = 0;
  p->type = GS_TYPE_NONE;
  p->state = GS_STATE_ACTIVATED;
  p->task = GS_TASK_SLEEPING;
  p->changed = FALSE;
  p->target = NULL;
  p->use_auth = FALSE;
  p->auth_user = NULL;
  p->auth_password = NULL;
  p->basic_auth_line = NULL;
  p->action_email = NULL;
  p->scheme = NULL;
  p->port = 0;
  p->hostname = NULL;
  p->path = NULL;
  p->title = NULL;
  p->group = NULL;
  p->ip_changed = 0;
  p->server_changed = 0;
  p->last_modified = 0;
  p->size_changed = 0;
  p->content_changed = 0;
  p->size = 0;
  p->status = 0;
  p->status_compare = GS_COMPARE_CHOICE_EQUAL;
  p->size_compare = GS_COMPARE_CHOICE_EQUAL;
  p->search_content_in = GS_HTML_TAGS_CHOICE_ALL;
  p->load_compare = GS_COMPARE_CHOICE_EQUAL;
  p->scan_ports_compare = GS_SCAN_PORTS_CHOICE_OPENED;
  p->status_changed = 0;
  p->load_time = 0;
  p->search_content = NULL;
  p->scan_ports = NULL;
  p->schedule = 0;
  p->old_ip = NULL;
  p->new_ip = NULL;
  p->old_server = NULL;
  p->new_server = NULL;
  p->old_last_modified = NULL;
  p->new_last_modified = NULL;
  p->old_size_changed = 0;
  p->new_size_changed = 0;
  p->old_content_checksum_changed = NULL;
  p->new_content_checksum_changed = NULL;
  p->old_status_changed = 0;
  p->new_status_changed = 0;

  return p;
}

GList *
gs_properties_alerts_load (const time_t timestamp, GList * list)
{
  enum
  {
    GS_DATE,
    GS_TIME,
    GS_ALERT_TYPE,
    GS_VALUE
  };
  FILE *fd = NULL;
  gchar *filename = NULL;
  gchar buffer[GS_BUFFER_LEN + 1];
  guint32 i = 0;

  filename =
    g_strdup_printf ("%s/logs/%u.log", gs->home_path, (guint) timestamp);
  fd = fopen (filename, "r");
  g_free (filename), filename = NULL;

  if (fd == NULL)
    return list;

  while (fgets (buffer, GS_BUFFER_LEN, fd) != NULL)
  {
    GSAlertProperties *p = NULL;
    gchar **fields = NULL;

    fields = g_strsplit (buffer, "\t", 0);

    if (GS_ALERT_IS_ALERT (g_ascii_strtod (fields[GS_ALERT_TYPE], NULL)))
    {
      p = gs_properties_alerts_node_new ();
      p->id = i++;
      p->alert_type = (guint) g_ascii_strtod (fields[GS_ALERT_TYPE], NULL);
      p->date_create = g_strdup (fields[GS_DATE]);
      p->time_create = g_strdup (fields[GS_TIME]);
      p->value = g_strdup (fields[GS_VALUE]);

      list = g_list_append (list, p);
    }

    g_strfreev (fields), fields = NULL;
  }

  fclose (fd);

  return list;
}

GList *
gs_properties_changes_load (const time_t timestamp, GList * list)
{
  enum
  {
    GS_DATE,
    GS_TIME,
    GS_ALERT_TYPE,
    GS_OLD_VALUE,
    GS_NEW_VALUE,
    GS_DIFF_SUFFIX
  };
  FILE *fd = NULL;
  gchar *filename = NULL;
  gchar buffer[GS_BUFFER_LEN + 1];
  guint32 i = 0;

  filename =
    g_strdup_printf ("%s/logs/%u.log", gs->home_path, (guint) timestamp);
  fd = fopen (filename, "r");
  g_free (filename), filename = NULL;

  if (fd == NULL)
    return list;

  while (fgets (buffer, GS_BUFFER_LEN, fd) != NULL)
  {
    GSChangeProperties *p = NULL;
    gchar **fields = NULL;

    fields = g_strsplit (buffer, "\t", 0);

    if (GS_ALERT_IS_CHANGE (g_ascii_strtod (fields[GS_ALERT_TYPE], NULL)))
    {
      fields[GS_NEW_VALUE] = g_strchomp (fields[GS_NEW_VALUE]);
      if (fields[GS_DIFF_SUFFIX])
        fields[GS_DIFF_SUFFIX] = g_strchomp (fields[GS_DIFF_SUFFIX]);

      p = gs_properties_changes_node_new ();
      p->id = i++;
      p->alert_type = (guint) g_ascii_strtod (fields[GS_ALERT_TYPE], NULL);
      p->file_basename = g_strdup_printf ("%u", (guint) timestamp);
      p->file_diff_suffix = (fields[GS_DIFF_SUFFIX]) ?
        g_strdup (fields[GS_DIFF_SUFFIX]) : g_strdup ("");
      p->date_create = g_strdup (fields[GS_DATE]);
      p->time_create = g_strdup (fields[GS_TIME]);
      p->old_value = g_strdup (fields[GS_OLD_VALUE]);
      p->new_value = g_strdup (fields[GS_NEW_VALUE]);

      list = g_list_append (list, p);
    }

    g_strfreev (fields), fields = NULL;
  }

  fclose (fd);

  return list;
}


static GList *
gs_properties_groups_load_real (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *value = NULL;


  cur = cur->xmlChildrenNode;
  while (cur != NULL)
  {
    if (!xmlStrcmp (cur->name, BAD_CAST "group"))
    {
      value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      gs->groups_list = g_list_append (gs->groups_list, value);
    }

    cur = cur->next;
  }

  return gs->groups_list;
}


GList *
gs_properties_groups_load (void)
{
  xmlDocPtr doc = NULL;
  xmlNodePtr n = NULL;
  gchar *file = NULL;


  gs->groups_list = NULL;

  file = g_strconcat (gs->home_path, "/", XML_GROUPS_FILENAME, NULL);
  if (!g_file_test (file, G_FILE_TEST_EXISTS))
  {
    g_free (file), file = NULL;
    return NULL;
  }

  doc = gs_properties_xml_get_doc (file);
  g_free (file), file = NULL;

  n = doc->xmlChildrenNode;
  while (n != NULL)
  {
    if (!xmlStrcmp (n->name, BAD_CAST "groups"))
      gs->groups_list =
        gs_properties_groups_load_real (gs->groups_list, doc, n);

    n = n->next;
  }

  xmlFreeDoc (doc);
  xmlCleanupParser ();

  return gs->groups_list;
}

GList *
gs_properties_spies_load (void)
{
  xmlDocPtr doc = NULL;
  xmlNodeSetPtr nodeset = NULL;
  xmlNodePtr n = NULL;
  xmlXPathObjectPtr result = NULL;
  gchar *file = NULL;
  gchar *tmp = NULL;
  guint i = 0;

  gs->spies_list = NULL;

  file = g_strconcat (gs->home_path, "/", XML_MAIN_FILENAME, NULL);
  if (!g_file_test (file, G_FILE_TEST_EXISTS))
  {
    g_free (file), file = NULL;
    return NULL;
  }

  doc = gs_properties_xml_get_doc (file);
  g_free (file), file = NULL;

  result = gs_properties_xml_get_nodeset (doc, NULL, (xmlChar *) "//spy");
  if (result != NULL)
  {
    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
      n = nodeset->nodeTab[i];

      if (strcmp ((char *) n->name, "spy") == 0)
      {
        GSSpyProperties *p = NULL;

        p = gs_properties_node_new ();

        /* Timestamp */
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "timestamp");
        p->timestamp = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        /* Timestamp update */
        if ((tmp = (gchar *) xmlGetProp (n, BAD_CAST "timestamp_update")))
        {
          p->timestamp_update = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
        else
          p->timestamp_update = 0;

        /* Timestamp last event */
        if ((tmp = (gchar *) xmlGetProp (n, BAD_CAST "timestamp_last_event")))
        {
          p->timestamp_last_event = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
        else
          p->timestamp_last_event = 0;

        /* Timestamp last exec */
        if ((tmp = (gchar *) xmlGetProp (n, BAD_CAST "timestamp_last_exec")))
        {
          p->timestamp_last_exec = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
        else
          p->timestamp_last_exec = 0;

        /* Type */
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "type");
        p->type = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        /* State */
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "state");
        p->state = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        /* Title */
        p->title = (gchar *) xmlGetProp (n, BAD_CAST "title");

        /* Group */
        p->group = (gchar *) xmlGetProp (n, BAD_CAST "group");

        /* Target */
        p->target = (gchar *) xmlGetProp (n, BAD_CAST "target");

        /* Schedule */
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "schedule");
        p->schedule = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        gs_properties_xml_get_spy_prop (doc, n, p);

        gs->spies_list = g_list_append (gs->spies_list, p);

        gs_add_spy_image (p);

        gs_spy_set_scheduling (p);
      }
    }

    xmlXPathFreeObject (result);
  }

  xmlFreeDoc (doc);
  xmlCleanupParser ();

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);

  return gs->spies_list;
}

static void
gs_properties_xml_get_actions (xmlDocPtr doc, xmlNodePtr node,
                               GSSpyProperties * p)
{
  xmlNodeSetPtr nodeset = NULL;
  xmlXPathObjectPtr result = NULL;
  xmlNodePtr n = NULL;
  guint i = 0;
  gchar *tmp = NULL;

  result = gs_properties_xml_get_nodeset (doc, node, (xmlChar *) "*");
  if (result != NULL)
  {
    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
      n = nodeset->nodeTab[i];

      if (strcmp ((char *) n->name, "email") == 0)
      {
        if (p->action_email == NULL)
          p->action_email = gs_properties_action_email_new ();

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "enabled");
        p->action_email->enabled = (gboolean) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->action_email->address = (gchar *) xmlGetProp (n, BAD_CAST "address");

        p->action_email->server = (gchar *) xmlGetProp (n, BAD_CAST "server");

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "port");
        p->action_email->port = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->action_email->login = (gchar *) xmlGetProp (n, BAD_CAST "login");

        p->action_email->password =
          (gchar *) xmlGetProp (n, BAD_CAST "password");
      }
    }
    xmlXPathFreeObject (result);
  }
}

static void
gs_properties_xml_get_alerts (xmlDocPtr doc, xmlNodePtr node,
                              GSSpyProperties * p)
{
  xmlNodeSetPtr nodeset = NULL;
  xmlXPathObjectPtr result = NULL;
  xmlNodePtr n = NULL;
  guint i = 0;
  gchar *tmp = NULL;
  gchar *tmp1 = NULL;

  result = gs_properties_xml_get_nodeset (doc, node, (xmlChar *) "*");
  if (result != NULL)
  {
    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
      n = nodeset->nodeTab[i];

      if (strcmp ((char *) n->name, "last_modified") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->last_modified = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->old_last_modified = (gchar *) xmlGetProp (n, BAD_CAST "old");

        p->new_last_modified = (gchar *) xmlGetProp (n, BAD_CAST "new");

      }
      else if (strcmp ((char *) n->name, "size_changed") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->size_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "old");
        p->old_size_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "new");
        p->new_size_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;
      }
      else if (strcmp ((char *) n->name, "content_changed") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->content_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->old_content_checksum_changed =
          (gchar *) xmlGetProp (n, BAD_CAST "old");

        p->new_content_checksum_changed =
          (gchar *) xmlGetProp (n, BAD_CAST "new");
      }
      else if (strcmp ((char *) n->name, "size") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        if (tmp[0] == '1')
        {
          tmp1 = (gchar *) xmlGetProp (n, BAD_CAST "value");
          p->size = (guint) g_ascii_strtod (tmp1, NULL);
          g_free (tmp1), tmp1 = NULL;
        }
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "compare");
        if (tmp != NULL)
        {
          p->size_compare = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
      }
      else if (strcmp ((char *) n->name, "status_changed") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->status_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "old");
        p->old_status_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "new");
        p->new_status_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;
      }
      else if (strcmp ((char *) n->name, "status") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        if (tmp[0] == '1')
        {
          tmp1 = (gchar *) xmlGetProp (n, BAD_CAST "value");
          p->status = (guint) g_ascii_strtod (tmp1, NULL);
          g_free (tmp1), tmp1 = NULL;
        }
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "compare");
        if (tmp != NULL)
        {
          p->status_compare = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
      }
      else if (strcmp ((char *) n->name, "load_time") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        if (tmp[0] == '1')
        {
          tmp1 = (gchar *) xmlGetProp (n, BAD_CAST "value");
          p->load_time = (guint) g_ascii_strtod (tmp1, NULL);
          g_free (tmp1), tmp1 = NULL;
        }
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "compare");
        if (tmp != NULL)
        {
          p->load_compare = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
      }
      else if (strcmp ((char *) n->name, "search_content") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        if (tmp[0] == '1')
          p->search_content = (gchar *) xmlGetProp (n, BAD_CAST "value");
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "in");
        if (tmp != NULL)
        {
          p->search_content_in = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "icase");
        if (tmp != NULL)
        {
          p->search_content_icase = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
      }
      else if (strcmp ((char *) n->name, "scan_ports") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        if (tmp[0] == '1')
          p->scan_ports = (gchar *) xmlGetProp (n, BAD_CAST "value");
        g_free (tmp), tmp = NULL;

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "compare");
        if (tmp != NULL)
        {
          p->scan_ports_compare = (guint) g_ascii_strtod (tmp, NULL);
          g_free (tmp), tmp = NULL;
        }
      }
      else if (strcmp ((char *) n->name, "server_changed") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->server_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->old_server = (gchar *) xmlGetProp (n, BAD_CAST "old");

        p->new_server = (gchar *) xmlGetProp (n, BAD_CAST "new");
      }
      else if (strcmp ((char *) n->name, "ip_changed") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "check");
        p->ip_changed = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->old_ip = (gchar *) xmlGetProp (n, BAD_CAST "old");

        p->new_ip = (gchar *) xmlGetProp (n, BAD_CAST "new");
      }
    }
    xmlXPathFreeObject (result);
  }
}

static void
gs_properties_xml_get_spy_prop (xmlDocPtr doc, xmlNodePtr node,
                                GSSpyProperties * p)
{
  xmlNodeSetPtr nodeset = NULL;
  xmlXPathObjectPtr result = NULL;
  xmlNodePtr n = NULL;
  guint i = 0;
  gchar *tmp = NULL;

  result = gs_properties_xml_get_nodeset (doc, node, (xmlChar *) "*");
  if (result != NULL)
  {
    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
      n = nodeset->nodeTab[i];

      /* uri */
      if (strcmp ((char *) n->name, "uri") == 0)
      {
        tmp = (gchar *) xmlGetProp (n, BAD_CAST "use_auth");
        p->use_auth = (gboolean) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        if (p->use_auth)
        {
          p->auth_user = (gchar *) xmlGetProp (n, BAD_CAST "auth_user");

          p->auth_password = (gchar *) xmlGetProp (n, BAD_CAST "auth_password");

          p->basic_auth_line =
            gs_build_basic_auth_line (p->auth_user, p->auth_password);
        }

        p->scheme = (gchar *) xmlGetProp (n, BAD_CAST "scheme");

        p->hostname = (gchar *) xmlGetProp (n, BAD_CAST "hostname");

        tmp = (gchar *) xmlGetProp (n, BAD_CAST "port");
        p->port = (guint) g_ascii_strtod (tmp, NULL);
        g_free (tmp), tmp = NULL;

        p->path = (gchar *) xmlGetProp (n, BAD_CAST "path");
      }
      /* alerts */
      else if (strcmp ((char *) n->name, "alerts") == 0)
        gs_properties_xml_get_alerts (doc, n, p);
      /* Actions */
      else if (strcmp ((char *) n->name, "actions") == 0)
        gs_properties_xml_get_actions (doc, n, p);
    }
    xmlXPathFreeObject (result);
  }
}

static xmlDocPtr
gs_properties_xml_get_doc (gchar * docname)
{
  xmlDocPtr doc = NULL;
  doc = xmlParseFile (docname);

  if (doc == NULL)
  {
    g_warning ("Document not parsed successfully. \n");
    return NULL;
  }

  return doc;
}

xmlXPathObjectPtr
gs_properties_xml_get_nodeset (xmlDocPtr doc, xmlNodePtr node, xmlChar * xpath)
{
  xmlXPathContextPtr context = NULL;
  xmlXPathObjectPtr result = NULL;

  context = xmlXPathNewContext (doc);
  if (node != NULL)
    context->node = node;
  result = xmlXPathEvalExpression (xpath, context);
  xmlXPathFreeContext (context);

  return (xmlXPathNodeSetIsEmpty (result->nodesetval)) ? NULL : result;
}
