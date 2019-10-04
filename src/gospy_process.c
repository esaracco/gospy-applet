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

#include "gospy_process.h"
#include "gospy_applet.h"

G_LOCK_DEFINE_STATIC (gs_process_lock);
G_LOCK_DEFINE_STATIC (gs_process_inet_ntoa_lock);

#define GS_PROCESS_TIMEOUT 30

#define GS_PROCESS_TIMEOUT_SCAN_PORTS 10

#define GS_PORT_VALUE_MAX 65535
#define GS_PORT_VALUE_MIN 1

#define GS_PROCESS_PAGE_CONTENT_NEEDED(p) \
  ( \
    p->last_modified == 1 || \
    p->size > 0 || \
    p->size_changed == 1 || \
    p->content_changed == 1 || \
    p->load_time > 0 || \
    (p->search_content != NULL && *p->search_content != '\0') \
  )

#define GS_PROCESS_RUN_SPY(p) \
  ( \
    p != NULL && \
    p->state != GS_STATE_DEACTIVATED && \
    p->task != GS_TASK_RUNNING \
  )

#define GS_PROCESS_SOCKET_OPENED(s) \
        (s > 0)

#ifdef ENABLE_GNUTLS
static gnutls_certificate_client_credentials xcred_gnutls;
#endif

typedef struct _GSConn GSConn;
struct _GSConn
{
  GSSpyProperties *spy;
  struct sockaddr_in addr;
  gint socket;
  guint timeout_id;
  pthread_t thread_id;
  gchar *ip;
  gchar *hostname;
  guint port;
  gboolean is_timeout;
  gboolean is_connected;
#ifdef ENABLE_GNUTLS
  gnutls_session session_gnutls;
#endif
};

static GSConn *gs_process_conn_new (GSSpyProperties * p, const gboolean proxy);
static void gs_process_conn_free (GSConn * c);
static void *gs_process_server_thread (void *data);
static void *gs_process_web_page_thread (void *data);
static gboolean gs_process_timeout_cb (gpointer data);
static void *gs_process_connect_thread (void *data);
static GHashTable *gs_process_get_http_header (GSConn * conn);
static gchar *gs_process_get_http_content (GSConn * conn);
static void gs_process_conn_connect (GSConn * conn);
static void gs_process_conn_disconnect (GSConn * conn);
static gchar *gs_process_build_request (const gchar * name,
                                        const GSSpyProperties * p);
static void gs_process_socket_close (const int s);
static GList *gs_process_scan_ports (GSConn * conn);
static void
gs_process_check_http_status_found (GSSpyProperties * p, GHashTable * header,
                                    GString ** message);
static void
gs_process_check_http_redirection (GSSpyProperties * p, GSConn * c,
                                   GHashTable ** header, GString ** message);
static void
gs_process_check_http_status_changed (GSSpyProperties * p, GHashTable * header,
                                      GString ** message);
static void
gs_process_check_search_content (GSSpyProperties * p, const gchar * content,
                                 GString ** message);
static void
gs_process_check_page_load_time (GSSpyProperties * p, const guint diff,
                                 GString ** message);
static void
gs_process_check_http_status_last_modified (GSSpyProperties * p,
                                            GHashTable * header,
                                            const gchar * content,
                                            GString ** message);
static void
gs_process_check_page_size_found (GSSpyProperties * p, const gchar * content,
                                  GString ** message);
static void
gs_process_check_page_size_changed (GSSpyProperties * p, const gchar * content,
                                    GString ** message);
static void
gs_process_check_page_content_changed (GSSpyProperties * p,
                                       const gchar * content,
                                       GString ** message);
static void gs_process_check_ports (GSSpyProperties * p, GSConn * c,
                                    GString ** message);
static void gs_process_check_ip (GSSpyProperties * p, GSConn * c,
                                 GString ** message);
static void gs_process_check_web_server (GSSpyProperties * p, GSConn * c,
                                         GString ** message);

void
gs_process_conn_init (void)
{
#ifdef ENABLE_GNUTLS
  gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&xcred_gnutls);
  gnutls_certificate_set_x509_trust_file (xcred_gnutls, "ca.pem",
                                          GNUTLS_X509_FMT_PEM);
#endif
}

void
gs_process_conn_deinit (void)
{
#ifdef ENABLE_GNUTLS
  gnutls_certificate_free_credentials (xcred_gnutls);
  gnutls_global_deinit ();
#endif
}

static GSConn *
gs_process_conn_new (GSSpyProperties * p, const gboolean proxy)
{
  GSConn *c = NULL;

  c = g_new0 (GSConn, 1);
  c->spy = p;
  c->socket = 0;
  c->timeout_id = 0;
  c->ip = NULL;
  c->is_timeout = FALSE;
  c->is_connected = FALSE;

  if (proxy)
  {
    gs_get_gnome_proxy_conf (&c->hostname, &c->port);
    if (*c->hostname == '\0' ||
        strcasecmp (p->hostname, "localhost") == 0 ||
        strstr (p->hostname, "127.0.0."))
    {
      g_free (c->hostname), c->hostname = NULL;
      c->hostname = g_strdup (p->hostname);
      c->port = p->port;
    }
  }
  else
  {
    c->hostname = g_strdup (p->hostname);
    c->port = p->port;
  }

  return c;
}

static void
gs_process_conn_free (GSConn * c)
{
  gs_process_socket_close (c->socket), c->socket = 0;
  g_free (c->ip), c->ip = NULL;
  g_free (c->hostname), c->hostname = NULL;

  g_free (c), c = NULL;
}

static gboolean
gs_process_timeout_cb (gpointer data)
{
  GSConn *c = (GSConn *) data;

  if (c != NULL && c->thread_id > 0)
  {
    pthread_cancel (c->thread_id);
    gs_process_socket_close (c->socket), c->socket = 0;
    c->is_timeout = TRUE;
  }

  return FALSE;
}

gboolean
gs_process_cb (gpointer data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;

  if (!GS_PROCESS_RUN_SPY (p))
    return TRUE;

  G_LOCK (gs_process_lock);

  p->task = GS_TASK_RUNNING;

  p->timestamp_last_exec = p->timestamp_last_exec = time (NULL);
  p->timestamp_next_exec += p->schedule * 60;

  G_UNLOCK (gs_process_lock);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);

  (p->type == GS_TYPE_SERVER) ?
    pthread_create (&p->thread_id, NULL, gs_process_server_thread, p) :
    pthread_create (&p->thread_id, NULL, gs_process_web_page_thread, p);

  if (p->thread_id > 0)
    pthread_detach (p->thread_id);

  return TRUE;
}

static void
gs_process_check_ports (GSSpyProperties * p, GSConn * c, GString ** message)
{
  GList *ports = NULL;
  GList *item = NULL;
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_SCAN_PORTS (p) ||
      (ports = gs_process_scan_ports (c)) == NULL)
    return;

  item = g_list_first (ports);
  while (item != NULL)
  {
    guint port = (guint) g_ascii_strtod ((gchar *) item->data, NULL);
    GSAlert alert;

    if (c->spy->scan_ports_compare == GS_SCAN_PORTS_CHOICE_OPENED)
      alert = GS_ALERT_PORT_OPENED;
    else
      alert = GS_ALERT_PORT_CLOSED;

    /* Log */
    tmp = g_strdup_printf ("%u\t%u\t", alert, port);
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp = g_strdup_printf ("\t* %s: %u\r\n", gs_alert_get_label (alert), port);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    item = g_list_next (item);
  }

  G_LOCK (gs_process_lock);
  p->changed = TRUE;
  G_UNLOCK (gs_process_lock);

  g_list_foreach (ports, (GFunc) g_free, NULL);
  g_list_free (ports), ports = NULL;
}

static void
gs_process_check_ip (GSSpyProperties * p, GSConn * c, GString ** message)
{
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_IP_CHANGED (p))
    return;

  /* First initialization */
  if (p->old_ip == NULL || *p->old_ip == '\0')
  {
    GS_SET_STRING (p->old_ip, c->ip);
  }

  GS_SET_STRING (p->new_ip, c->ip);

  /* A change occured */
  if (strcmp (p->old_ip, p->new_ip) != 0)
  {
    /* Log */
    tmp = g_strdup_printf ("%u\t%s\t%s", GS_ALERT_IP_CHANGED,
                           p->old_ip, p->new_ip);

    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp = g_strdup_printf ("\t* %s:\r\n\t\t%s: %s\r\n\t\t%s: %s\r\n\r\n",
                           gs_alert_get_label (GS_ALERT_IP_CHANGED),
                           _("Old IP"), p->old_ip, _("New IP"), p->new_ip);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    GS_SET_STRING (p->old_ip, p->new_ip);

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

static void
gs_process_check_web_server (GSSpyProperties * p, GSConn * c,
                             GString ** message)
{
  gchar *tmp = NULL;
  GHashTable *header = NULL;
  gchar *server = NULL;

  if (!GS_PROCESS_SOCKET_OPENED (c->socket) || !GS_SPY_HAVE_SERVER_CHANGED (p))
    return;

  if ((header = gs_process_get_http_header (c)) != NULL &&
      (server = (gchar *) g_hash_table_lookup (header, "server")) != NULL)
  {

    /* First initialization */
    if (p->old_server == NULL || *p->old_server == '\0')
    {
      GS_SET_STRING (p->old_server, server);
    }

    GS_SET_STRING (p->new_server, server);

    /* A change occured */
    if (strcmp (p->old_server, p->new_server) != 0)
    {
      /* Log */
      tmp = g_strdup_printf ("%u\t%s\t%s", GS_ALERT_SERVER_CHANGED,
                             p->old_server, p->new_server);

      gs_append_to_spy_log (p->timestamp, tmp);
      g_free (tmp), tmp = NULL;

      /* Mail content */
      tmp = g_strdup_printf ("\t* %s:\r\n\t\t%s: %s\r\n\t\t%s: %s\r\n\r\n",
                             gs_alert_get_label (GS_ALERT_SERVER_CHANGED),
                             _("Old server"), p->old_ip,
                             _("New server"), p->new_ip);
      *message = g_string_append (*message, tmp);
      g_free (tmp), tmp = NULL;

      GS_SET_STRING (p->old_server, p->new_server);

      G_LOCK (gs_process_lock);
      p->changed = TRUE;
      G_UNLOCK (gs_process_lock);
    }

    g_hash_table_destroy (header), header = NULL;
  }
}

static void *
gs_process_server_thread (void *data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;
  GSConn *c = NULL;
  gchar *tmp = NULL;
  GString *mail_content = NULL;

  mail_content = g_string_new ("");

  c = gs_process_conn_new (p, TRUE);

  gs_process_conn_connect (c);

  if ((!c->is_timeout) && c->ip != NULL)
  {
    /* Check ports */
    gs_process_check_ports (p, c, &mail_content);

    /* Check for IP change */
    gs_process_check_ip (p, c, &mail_content);

    /* Check for server change */
    gs_process_check_web_server (p, c, &mail_content);
  }
  else
  {
    G_LOCK (gs_process_lock);
    p->alert_connection_error = TRUE;
    G_UNLOCK (gs_process_lock);

    /* Log */
    tmp = g_strdup_printf ("%u\t%s\t%s", GS_ALERT_CONNECTION_ERROR, " ", " ");
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp = g_strdup_printf ("\t* %s: %s:%u\r\n",
                           gs_alert_get_label (GS_ALERT_CONNECTION_ERROR),
                           c->hostname, c->port);
    mail_content = g_string_append (mail_content, tmp);
    g_free (tmp), tmp = NULL;

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }

  gs_process_conn_free (c), c = NULL;

  if (p->changed)
  {
    p->timestamp_last_event = time (NULL);
    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW |
                         GS_REFRESH_TYPE_CHANGES_TREE_VIEW);

    if (p->action_email->enabled && mail_content->len > 0)
    {
      g_print ("%s\n", mail_content->str);
      tmp = g_strdup_printf ("%s: %s\r\n%s: %s\r\n\r\n%s\r\n\r\n",
                             _("Title"), p->title, _("Target"), p->target,
                             _("The following alerts have been raised:"));
      mail_content = g_string_prepend (mail_content, tmp);
      gs_sendmail (p->action_email->server, p->action_email->port,
                   p->action_email->address, _("SERVER ALERT"),
                   mail_content->str, NULL);
      g_free (tmp), tmp = NULL;
    }
  }

  G_LOCK (gs_process_lock);
  p->task = GS_TASK_SLEEPING;
  G_UNLOCK (gs_process_lock);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);

  g_string_free (mail_content, TRUE), mail_content = NULL;

  return NULL;
}

static void
gs_process_conn_connect (GSConn * c)
{
  G_LOCK (gs_process_lock);

  c->is_timeout = FALSE;
  pthread_create (&c->thread_id, NULL, gs_process_connect_thread, c);
  c->timeout_id =
    g_timeout_add (GS_PROCESS_TIMEOUT * 1000, gs_process_timeout_cb, c);
  pthread_join (c->thread_id, NULL);
  if (c->timeout_id)
    g_source_remove (c->timeout_id), c->timeout_id = 0;

#ifdef ENABLE_GNUTLS
  if (c->port == 443)
  {
    gint ret = 0;
    const gint cert_type_priority[3] = { GNUTLS_CRT_X509,
      GNUTLS_CRT_OPENPGP, 0
    };

    gnutls_init (&c->session_gnutls, GNUTLS_CLIENT);
    gnutls_set_default_priority (c->session_gnutls);
    gnutls_certificate_type_set_priority (c->session_gnutls,
                                          cert_type_priority);
    gnutls_credentials_set (c->session_gnutls, GNUTLS_CRD_CERTIFICATE,
                            xcred_gnutls);
    gnutls_transport_set_ptr (c->session_gnutls, GINT_TO_POINTER (c->socket));

    do
      ret = gnutls_handshake (c->session_gnutls);
    while ((ret == GNUTLS_E_AGAIN) || (ret == GNUTLS_E_INTERRUPTED));

    if (ret < 0)
      gs_process_conn_disconnect (c);
  }
#endif

  G_UNLOCK (gs_process_lock);
}

static void
gs_process_socket_close (const gint s)
{
  if (GS_PROCESS_SOCKET_OPENED (s))
  {
    shutdown (s, SHUT_RDWR);
    close (s);
  }
}

static void
gs_process_conn_disconnect (GSConn * c)
{
#ifdef ENABLE_GNUTLS
  if (c->port == 443)
  {
    gnutls_bye (c->session_gnutls, GNUTLS_SHUT_RDWR);
    gnutls_deinit (c->session_gnutls);
  }
#endif

  gs_process_socket_close (c->socket), c->socket = 0;
}

static void
gs_process_check_http_redirection (GSSpyProperties * p, GSConn * c,
                                   GHashTable ** header, GString ** message)
{
  gchar *location = NULL;
  gchar *tmp = NULL;

  if ((location = (gchar *) g_hash_table_lookup (*header, "location")) == NULL)
    return;

  /* Log */
  tmp =
    g_strdup_printf ("%u\t%s\t%s", GS_ALERT_REDIRECTION, p->target, location);
  gs_append_to_spy_log (p->timestamp, tmp);
  g_free (tmp), tmp = NULL;

  /* Mail content */
  tmp = g_strdup_printf ("\t* %s:\r\n\t\t%s -> %s\r\n",
                         gs_alert_get_label (GS_ALERT_REDIRECTION),
                         p->target, location);
  *message = g_string_append (*message, tmp);
  g_free (tmp), tmp = NULL;

  G_LOCK (gs_process_lock);
  g_free (p->target), p->target = NULL;
  g_free (p->scheme), p->scheme = NULL;
  g_free (p->hostname), p->hostname = NULL;
  g_free (p->path), p->path = NULL;
  p->target = g_strdup (location);
  G_UNLOCK (gs_process_lock);

  gs_split_uri (p->target, &p->scheme, &p->port, &p->hostname, &p->path);

  g_hash_table_destroy (*header), *header = NULL;
  gs_process_conn_free (c), c = NULL;
  c = gs_process_conn_new (p, TRUE);
  gs_process_conn_connect (c);

  G_LOCK (gs_process_lock);
  p->changed = TRUE;
  G_UNLOCK (gs_process_lock);

  if ((!c->is_timeout) && GS_PROCESS_SOCKET_OPENED (c->socket))
    *header = gs_process_get_http_header (c);

  return;
}

static void
gs_process_check_http_status_found (GSSpyProperties * p, GHashTable * header,
                                    GString ** message)
{
  guint code = 0;
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_STATUS (p))
    return;

  code = (guint) g_ascii_strtod ((gchar *) g_hash_table_lookup (header, "code"),
                                 NULL);

  if (gs_compare_match (p->status_compare, p->status, code))
  {
    /* Log */
    tmp = g_strdup_printf ("%u\t%u\t%s", GS_ALERT_STATUS_FOUND, code, " ");
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp = g_strdup_printf ("\t* %s: %u\r\n",
                           gs_alert_get_label (GS_ALERT_STATUS_FOUND), code);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

static void
gs_process_check_http_status_changed (GSSpyProperties * p, GHashTable * header,
                                      GString ** message)
{
  guint code = 0;
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_STATUS_CHANGED (p))
    return;

  code = (guint) g_ascii_strtod ((gchar *) g_hash_table_lookup (header, "code"),
                                 NULL);

  if (!p->old_status_changed)
    p->old_status_changed = code;
  p->new_status_changed = code;

  if (p->old_status_changed != p->new_status_changed)
  {
    guint diff_suffix = ((guint) time (NULL)) + 2;

    /* Log */
    tmp = g_strdup_printf ("%u\t%u\t%u\t%u",
                           GS_ALERT_STATUS_CHANGED,
                           p->old_status_changed,
                           p->new_status_changed, diff_suffix);
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp = g_strdup_printf ("\t* %s:\r\n\t\t%s: %u\r\n\t\t%s: %u\r\n",
                           gs_alert_get_label (GS_ALERT_STATUS_CHANGED),
                           _("Old status"), p->old_status_changed,
                           _("New status"), p->new_status_changed);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    p->old_status_changed = p->new_status_changed;

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

static void
gs_process_check_search_content (GSSpyProperties * p, const gchar * content,
                                 GString ** message)
{
  gchar *tmp = NULL;
  gchar **tmp1 = NULL;
  gchar *ref_utf8 = NULL;
  gchar *content_utf8 = NULL;
  gchar *ref_quoted = NULL;
  gchar *pattern = NULL;
  regex_t compre;
  gint regex_flag = REG_EXTENDED;

  if (!GS_SPY_HAVE_SEARCH_CONTENT (p))
    return;

  ref_utf8 = gs_convert_to_utf8 (p->search_content);
  content_utf8 = gs_convert_to_utf8 (content);
  ref_quoted = gs_quote_regex_string (ref_utf8);

  /* Build regex search pattern */
  tmp1 = gs_html_tag_get_pattern (p->search_content_in);
  tmp = g_strconcat (tmp1[0], ref_quoted, tmp1[1], NULL);
  g_strfreev (tmp1), tmp1 = NULL;
  pattern = gs_strreplace (tmp, " ", "(\\s|\\n|\\r)+");
  g_free (tmp), tmp = NULL;

  if (p->search_content_icase)
    regex_flag |= REG_ICASE;

  //g_print ("--> %s\n", pattern);

  G_LOCK (gs_process_lock);
  if (regcomp (&compre, pattern, regex_flag) != 0)
    g_error ("Regular expression compilation failed");

  if (regexec (&compre, content_utf8, 0, NULL, 0) == 0)
  {
    /* Log */
    tmp = g_strdup_printf ("%u\t%s\t%s",
                           GS_ALERT_CONTENT_FOUND, p->search_content, " ");
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    /* Mail content */
    tmp =
      g_strdup_printf ("\t* %s:\r\n----------\r\n%s\r\n----------\r\n",
                       gs_alert_get_label (GS_ALERT_CONTENT_FOUND),
                       p->search_content);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    p->changed = TRUE;
  }
  regfree (&compre);
  G_UNLOCK (gs_process_lock);

  g_free (pattern), pattern = NULL;
  g_free (ref_utf8), ref_utf8 = NULL;
  g_free (content_utf8), content_utf8 = NULL;
  g_free (ref_quoted), ref_quoted = NULL;
}

static void
gs_process_check_page_load_time (GSSpyProperties * p, const guint diff,
                                 GString ** message)
{
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_LOAD_TIME (p) ||
      !gs_compare_match (p->load_compare, p->load_time, diff))
    return;

  /* Log */
  tmp = g_strdup_printf ("%u\t%u\t%s",
                         GS_ALERT_LOAD_TIME_FOUND, (guint) diff, " ");
  gs_append_to_spy_log (p->timestamp, tmp);
  g_free (tmp), tmp = NULL;

  /* Mail content */
  tmp =
    g_strdup_printf ("\t* %s: %u %s\r\n",
                     gs_alert_get_label (GS_ALERT_LOAD_TIME_FOUND),
                     (guint) diff, _("seconds"));
  *message = g_string_append (*message, tmp);
  g_free (tmp), tmp = NULL;

  G_LOCK (gs_process_lock);
  p->changed = TRUE;
  G_UNLOCK (gs_process_lock);
}

static void
gs_process_check_http_status_last_modified (GSSpyProperties * p,
                                            GHashTable * header,
                                            const gchar * content,
                                            GString ** message)
{
  gchar *last_modified = NULL;
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_LAST_MODIFIED (p))
    return;

  last_modified = (gchar *) g_hash_table_lookup (header, "last-modified");

  /* if there is no "last-modified" header field, try to retreive the
   * "date" header field */
  if (last_modified == NULL)
    last_modified = (gchar *) g_hash_table_lookup (header, "date");

  /* First initialization */
  if (p->old_last_modified == NULL || *p->old_last_modified == '\0')
  {
    GS_SET_STRING (p->old_last_modified, last_modified);
    gs_file_save (p->timestamp, GS_ALERT_LAST_MODIFIED_CHANGED, 0, content);
  }

  GS_SET_STRING (p->new_last_modified, last_modified);
  gs_file_save (p->timestamp, GS_ALERT_LAST_MODIFIED_CHANGED, 1, content);

  if (strcmp (p->old_last_modified, p->new_last_modified) != 0)
  {
    guint diff_suffix = ((guint) time (NULL));

    /* Log */
    tmp = g_strdup_printf ("%u\t%s\t%s\t%u",
                           GS_ALERT_LAST_MODIFIED_CHANGED,
                           p->old_last_modified,
                           p->new_last_modified, diff_suffix);
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    gs_file_save_diff (p->timestamp,
                       GS_ALERT_LAST_MODIFIED_CHANGED, diff_suffix);
    gs_file_save (p->timestamp, GS_ALERT_LAST_MODIFIED_CHANGED, 0, content);

    /* Mail content */
    tmp =
      g_strdup_printf ("\t* %s:\r\n\t\t%s: %s\r\n\t\t%s: %s\r\n",
                       gs_alert_get_label (GS_ALERT_LAST_MODIFIED_CHANGED),
                       _("Old date"), p->old_last_modified, _("New date"),
                       p->new_last_modified);
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    GS_SET_STRING (p->old_last_modified, p->new_last_modified);

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

static void
gs_process_check_page_size_found (GSSpyProperties * p, const gchar * content,
                                  GString ** message)
{
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_SIZE (p) ||
      !gs_compare_match (p->size_compare, p->size * 1000, strlen (content)))
    return;

  /* Log */
  tmp = g_strdup_printf ("%u\t%u\t%u",
                         GS_ALERT_SIZE_FOUND, (unsigned int) strlen (content),
                         p->size * 1000);
  gs_append_to_spy_log (p->timestamp, tmp);
  g_free (tmp), tmp = NULL;

  /* Mail content */
  tmp =
    g_strdup_printf ("\t* %s: %u %s\r\n",
                     gs_alert_get_label (GS_ALERT_SIZE_FOUND),
                     (unsigned int) strlen (content), _("bytes"));
  *message = g_string_append (*message, tmp);
  g_free (tmp), tmp = NULL;

  G_LOCK (gs_process_lock);
  p->changed = TRUE;
  G_UNLOCK (gs_process_lock);
}

static void
gs_process_check_page_size_changed (GSSpyProperties * p, const gchar * content,
                                    GString ** message)
{
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_SIZE_CHANGED (p))
    return;

  /* First initialization */
  if (!p->old_size_changed)
  {
    p->old_size_changed = strlen (content);
    gs_file_save (p->timestamp, GS_ALERT_SIZE_CHANGED, 0, content);
  }
  p->new_size_changed = strlen (content);
  gs_file_save (p->timestamp, GS_ALERT_SIZE_CHANGED, 1, content);

  if (p->old_size_changed != p->new_size_changed)
  {
    guint diff_suffix = ((guint) time (NULL)) + 2;

    /* Log */
    tmp =
      g_strdup_printf ("%u\t%u\t%u\t%u",
                       GS_ALERT_SIZE_CHANGED,
                       p->old_size_changed, p->new_size_changed, diff_suffix);
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    gs_file_save_diff (p->timestamp, GS_ALERT_SIZE_CHANGED, diff_suffix);
    gs_file_save (p->timestamp, GS_ALERT_SIZE_CHANGED, 0, content);

    /* Mail content */
    tmp =
      g_strdup_printf ("\t* %s:\r\n\t\t%s: %u %s\r\n\t\t%s: %u %s\r\n",
                       gs_alert_get_label (GS_ALERT_SIZE_CHANGED),
                       _("Old size"), p->old_size_changed, _("bytes"),
                       _("New size"), p->new_size_changed, _("bytes"));
    *message = g_string_append (*message, tmp);
    g_free (tmp), tmp = NULL;

    p->old_size_changed = p->new_size_changed;

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

static void
gs_process_check_page_content_changed (GSSpyProperties * p,
                                       const gchar * content,
                                       GString ** message)
{
  GMD5 *md5 = NULL;
  gchar *md5_str = NULL;
  gchar *tmp = NULL;

  if (!GS_SPY_HAVE_CONTENT_CHANGED (p))
    return;

  gdk_threads_enter ();
  G_LOCK (gs_process_lock);

  md5 = gnet_md5_new (content, strlen (content));
  md5_str = gnet_md5_get_string (md5);
  gnet_md5_delete (md5), md5 = NULL;

  G_UNLOCK (gs_process_lock);
  gdk_threads_leave ();

  /* First initialization */
  if (p->old_content_checksum_changed == NULL ||
      strlen (p->old_content_checksum_changed) == 0)
  {
    GS_SET_STRING (p->old_content_checksum_changed, md5_str);
    gs_file_save (p->timestamp, GS_ALERT_CONTENT_CHANGED, 0, content);
  }

  GS_SET_STRING (p->new_content_checksum_changed, md5_str);
  gs_file_save (p->timestamp, GS_ALERT_CONTENT_CHANGED, 1, content);

  g_free (md5_str), md5_str = NULL;

  if (strcmp (p->old_content_checksum_changed,
              p->new_content_checksum_changed) != 0)
  {
    guint diff_suffix = ((guint) time (NULL)) + 2;

    /* Log */
    tmp =
      g_strdup_printf ("%u\t%s\t%s\t%u",
                       GS_ALERT_CONTENT_CHANGED,
                       p->old_content_checksum_changed,
                       p->new_content_checksum_changed, diff_suffix);
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    gs_file_save_diff (p->timestamp, GS_ALERT_CONTENT_CHANGED, diff_suffix);
    gs_file_save (p->timestamp, GS_ALERT_CONTENT_CHANGED, 0, content);

// FIXME
#if 0
    /* Mail content */
    {
      gchar *filename = g_strdup_printf ("%s/logs/%s-%u:%s.diff",
                                         gs->home_path,
                                         p->file_basename, p->alert_type,
                                         p->file_diff_suffix);
      g_print ("-> %s\n", filename);
    }

    tmp = g_strdup_printf ("\t* %s:\r\n\t\t----- Diff -----\r\n%s\r\n\t\t\r\n",
                           gs_alert_get_label (GS_ALERT_CONTENT_CHANGED), diff);
    mail_content = g_string_append (mail_content, tmp);
    g_free (tmp), tmp = NULL;
#endif

    GS_SET_STRING (p->old_content_checksum_changed,
                   p->new_content_checksum_changed);

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }
}

/* Sorry for the "gotos" :-) */
static void *
gs_process_web_page_thread (void *data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;
  GSConn *c = NULL;
  gchar *tmp = NULL;
  GHashTable *header = NULL;
  GString *mail_content = NULL;

  mail_content = g_string_new ("");

  G_LOCK (gs_process_lock);
  p->alert_connection_error = TRUE;
  G_UNLOCK (gs_process_lock);

  c = gs_process_conn_new (p, TRUE);

  gs_process_conn_connect (c);
  if (c->is_timeout || !GS_PROCESS_SOCKET_OPENED (c->socket) ||
      (header = gs_process_get_http_header (c)) == NULL)
    goto handler_end;

  /* If there is a redirection, replace spy data and reconnect with
   * new location */
  gs_process_check_http_redirection (p, c, &header, &mail_content);
  if (c->is_timeout || !GS_PROCESS_SOCKET_OPENED (c->socket))
    goto handler_end;

  /* Check for HTTP status */
  gs_process_check_http_status_found (p, header, &mail_content);

  /* Check for status change */
  gs_process_check_http_status_changed (p, header, &mail_content);

  /* All the next checks needs page content */
  if (GS_PROCESS_PAGE_CONTENT_NEEDED (p))
  {
    gchar *content = NULL;
    time_t t0;
    time_t t1;
    guint diff = 0;

    /* begin - For page load time check */
    t0 = time (NULL);

    /* Get page content */
    gs_process_conn_connect (c);
    content = gs_process_get_http_content (c);
    if (content == NULL)
      goto handler_end;

    /* End - For page load time check */
    t1 = time (NULL);

    /* Check load time */
    diff = (guint) difftime (t1, t0);
    gs_process_check_page_load_time (p, diff, &mail_content);

    /* Search for content */
    gs_process_check_search_content (p, content, &mail_content);

    /* Check for "last modified" header field change */
    gs_process_check_http_status_last_modified (p, header, content,
                                                &mail_content);

    /* Content size found */
    gs_process_check_page_size_found (p, content, &mail_content);

    /* Check for page size change */
    gs_process_check_page_size_changed (p, content, &mail_content);

    /* Check for page content change */
    gs_process_check_page_content_changed (p, content, &mail_content);

    g_free (content), content = NULL;
  }

  G_LOCK (gs_process_lock);
  p->alert_connection_error = FALSE;
  G_UNLOCK (gs_process_lock);

handler_end:

  if (p->alert_connection_error)
  {
    tmp = g_strdup_printf ("%u\t%s\t%s",
                           (c->is_timeout) ?
                           GS_ALERT_CONNECTION_TIMEOUT :
                           GS_ALERT_CONNECTION_ERROR, " ", " ");
    gs_append_to_spy_log (p->timestamp, tmp);
    g_free (tmp), tmp = NULL;

    G_LOCK (gs_process_lock);
    p->changed = TRUE;
    G_UNLOCK (gs_process_lock);
  }

  if (header != NULL)
    g_hash_table_destroy (header), header = NULL;

  gs_process_conn_free (c), c = NULL;

  if (p->changed)
  {
    p->timestamp_last_event = time (NULL);
    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW |
                         GS_REFRESH_TYPE_CHANGES_TREE_VIEW);

    if (p->action_email->enabled && mail_content->len > 0)
    {
      //g_print ("%s\n", mail_content->str);
      tmp = g_strdup_printf ("%s: %s\r\n%s: %s\r\n\r\n%s\r\n\r\n",
                             _("Title"), p->title, _("Target"), p->target,
                             _("The following Web alerts have been raised:"));
      mail_content = g_string_prepend (mail_content, tmp);
      gs_sendmail (p->action_email->server, p->action_email->port,
                   p->action_email->address, _("WEB PAGE ALERT"),
                   mail_content->str, NULL);
      g_free (tmp), tmp = NULL;
    }
  }

  G_LOCK (gs_process_lock);
  p->task = GS_TASK_SLEEPING;
  G_UNLOCK (gs_process_lock);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);

  g_string_free (mail_content, TRUE), mail_content = NULL;

  return NULL;
}

static void *
gs_process_connect_thread (void *data)
{
  GSConn *c = (GSConn *) data;
  gint res = 0;

  g_return_val_if_fail (c != NULL, NULL);
  g_return_val_if_fail (c->spy != NULL, NULL);

  /* Hostname resolution */
  if (c->ip == NULL)
  {
    struct hostent *he = NULL;

    if ((he = gethostbyname (c->hostname)) != NULL)
    {
      memset (&c->addr, 0, sizeof (struct sockaddr_in));
      c->addr.sin_port = htons (c->port);
      c->addr.sin_family = AF_INET;
      memcpy (&c->addr.sin_addr, *he->h_addr_list, he->h_length);

      G_LOCK (gs_process_inet_ntoa_lock);
      c->ip = g_strdup (inet_ntoa (*((struct in_addr *) he->h_addr)));
      G_UNLOCK (gs_process_inet_ntoa_lock);
    }
  }

  /* Hostname resolution failed */
  if (c->ip == NULL)
  {
    g_warning ("gethostbyname (1): %s", c->hostname);
    c->is_timeout = TRUE;
    goto handler_end;
  }

  /* Connect socket only if needed */
  if (c->spy->server_changed || c->spy->type == GS_TYPE_WEB_PAGE)
  {
    /* Create socket */
    c->socket = socket (AF_INET, SOCK_STREAM, 0);
    if (!GS_PROCESS_SOCKET_OPENED (c->socket))
    {
      g_warning ("socket: %s", c->hostname);
      c->is_timeout = TRUE;
      goto handler_end;
    }

    /* Connect socket */
    res = connect (c->socket, (struct sockaddr *) &c->addr, sizeof (c->addr));
    if (res != 0)
    {
      g_warning ("connect: %s:%u", c->hostname, c->port);
      c->is_timeout = TRUE;
      goto handler_end;
    }
  }

handler_end:

  return NULL;
}

void *
gs_process_connect_scan_ports_thread (void *data)
{
  GSConn *c = (GSConn *) data;
  gint res = 0;

  g_return_val_if_fail (c != NULL, NULL);

  /* Hostname resolution */
  if (c->ip == NULL)
  {
    struct hostent *he = NULL;

    if ((he = gethostbyname (c->hostname)) != NULL)
    {
      memset (&c->addr, 0, sizeof (struct sockaddr_in));
      c->addr.sin_port = htons (c->port);
      c->addr.sin_family = AF_INET;
      memcpy (&c->addr.sin_addr, *he->h_addr_list, he->h_length);

      G_LOCK (gs_process_inet_ntoa_lock);
      c->ip = g_strdup (inet_ntoa (*((struct in_addr *) he->h_addr)));
      G_UNLOCK (gs_process_inet_ntoa_lock);
    }
  }

  /* Hostname resolution failed */
  if (c->ip == NULL)
  {
    g_warning ("gethostbyname (2): %s", c->hostname);
    goto handler_end;
  }

  /* Create socket */
  c->socket = socket (AF_INET, SOCK_STREAM, 0);
  if (!GS_PROCESS_SOCKET_OPENED (c->socket))
  {
    g_warning ("socket: %s", c->hostname);
    goto handler_end;
  }

  /* Connect socket */
  res = connect (c->socket, (struct sockaddr *) &c->addr, sizeof (c->addr));
  if (!res)
  {
    c->is_connected = TRUE;
    goto handler_end;
  }

handler_end:

  return NULL;
}

static gchar *
gs_process_build_request (const gchar * name, const GSSpyProperties * p)
{
  gchar *uri = NULL;
  gchar *request = NULL;

  uri = (gs_use_proxy ())?
    g_strdup_printf ("%s://%s%s", p->scheme, p->hostname, p->path) :
    g_strdup (p->path);

  request = (strcmp (name, "head") == 0) ?
    g_strdup_printf ("HEAD %s HTTP/1.0\r\n"
                     "User-Agent: %s\r\n"
                     "Host: %s"
                     "%s\r\n\r\n",
                     uri, GS_USER_AGENT, p->hostname,
                     (p->basic_auth_line && *p->basic_auth_line != '\0') ?
                     p->basic_auth_line : "") :
    g_strdup_printf ("GET %s HTTP/1.0\r\n"
                     "User-Agent: %s\r\n"
                     "Host: %s\r\n"
                     "Accept: */*"
                     "%s\r\n\r\n", uri, GS_USER_AGENT, p->hostname,
                     (p->basic_auth_line && *p->basic_auth_line != '\0') ?
                     p->basic_auth_line : "");

  g_free (uri), uri = NULL;

  return request;
}

static GHashTable *
gs_process_get_http_header (GSConn * c)
{
  GHashTable *header = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              g_free, g_free);
  gchar *request = NULL;
  gchar **lines = NULL;
  gchar buffer[GS_BUFFER_LEN + 1];
  guint nb_read = 0;
  gchar code[3 + 1] = { 0 };
  guint i = 0;
  GString *response = g_string_new ("");

  request = gs_process_build_request ("head", c->spy);

#ifdef ENABLE_GNUTLS
  if (c->port == 443)
  {
    gnutls_record_send (c->session_gnutls, request, strlen (request));
    while ((nb_read = gnutls_record_recv (c->session_gnutls,
                                          buffer, GS_BUFFER_LEN)) > 0)
      response = g_string_append_len (response, buffer, nb_read);
  }
  else
#endif
  {
    write (c->socket, request, strlen (request));
    while ((nb_read = read (c->socket, buffer, GS_BUFFER_LEN)) > 0)
      response = g_string_append_len (response, buffer, nb_read);
  }

  g_free (request), request = NULL;

  if (response->len < 9 + 3)
  {
    g_string_free (response, TRUE), response = NULL;
    g_hash_table_destroy (header), header = NULL;
    return NULL;
  }

  memcpy (&code, &response->str[9], 3);
  g_hash_table_insert (header, g_strdup ("code"), g_strdup (code));

  i = 0;
  lines = g_strsplit (response->str, "\r\n", 0);
  while (lines[i] != NULL && strchr (lines[i], '<') == NULL)
  {
    gchar **key_value = g_strsplit (lines[i], ":", 2);

    if (key_value[0] != NULL && key_value[1] != NULL)
    {
      GS_STRSTRIP (key_value[0]), GS_STRSTRIP (key_value[1]);

      g_hash_table_insert (header,
                           g_utf8_strdown (key_value[0], -1),
                           g_strdup (key_value[1]));
    }

    g_strfreev (key_value), key_value = NULL;
    i++;
  }
  g_strfreev (lines), lines = NULL;

  g_string_free (response, TRUE), response = NULL;

  return header;
}

static gchar *
gs_process_get_http_content (GSConn * c)
{
  const gchar *p = NULL;
  gchar *request = NULL;
  gchar *ret = NULL;
  gchar buffer[GS_BUFFER_LEN + 1];
  guint nb_read = 0;
  GString *response = g_string_new ("");

  request = gs_process_build_request ("get", c->spy);

#ifdef ENABLE_GNUTLS
  if (c->port == 443)
  {
    gnutls_record_send (c->session_gnutls, request, strlen (request));
    while ((nb_read = gnutls_record_recv (c->session_gnutls,
                                          buffer, GS_BUFFER_LEN)) > 0)
      response = g_string_append_len (response, buffer, nb_read);
  }
  else
#endif
  {
    write (c->socket, request, strlen (request));
    while ((nb_read = read (c->socket, buffer, GS_BUFFER_LEN)) > 0)
      response = g_string_append_len (response, buffer, nb_read);
  }

  g_free (request), request = NULL;

  /* pass HTTP header */
  if (response->len > 4)
  {
    p = response->str;
    while (memcmp (++p, "\r\n\r\n", 4) != 0);
    p += 4;

    ret = g_strdup (p);
  }

  g_string_free (response, TRUE), response = NULL;

  return ret;
}

static GList *
gs_process_scan_ports (GSConn * c)
{
  GList *list = NULL;
  gchar **ports = NULL;
  guint i = 0;
  gboolean simple_value = FALSE;

  /* A interval of ports */
  if (strchr (c->spy->scan_ports, '-'))
  {
    gchar **tmp = NULL;
    guint j = 0;
    guint port_max = 0;
    guint port_min = 0;

    tmp = g_strsplit (c->spy->scan_ports, "-", 2);
    port_min = (guint) g_ascii_strtod (tmp[0], NULL);
    port_min = MAX (port_min, GS_PORT_VALUE_MIN);
    port_max = (guint) g_ascii_strtod (tmp[1], NULL);
    port_max = MIN (port_max, GS_PORT_VALUE_MAX);
    g_strfreev (tmp), tmp = NULL;

    /* Port values are ok */
    if (port_max > 0 && (ports = g_new0 (gchar *, (port_max - port_min) + 2)))
    {
      for (j = 0, i = port_min; i <= port_max; i++, j++)
        ports[j] = g_strdup_printf ("%u", i);
    }
    /* Bad max port value. We remove it */
    else
    {
      simple_value = TRUE;

      g_free (c->spy->scan_ports), c->spy->scan_ports = NULL;
      c->spy->scan_ports = g_strdup_printf ("%u", port_min);

      GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_SAVE_DATA);
    }
  }
  else
    simple_value = TRUE;

  if (simple_value)
    ports = g_strsplit (c->spy->scan_ports, ",", 0);

  for (i = 0; ports[i] != NULL; i++)
  {
    GSConn *csp = NULL;
    guint port = (guint) g_ascii_strtod (ports[i], NULL);

    /* Do not check invalid ports */
    if (port < GS_PORT_VALUE_MIN || port > GS_PORT_VALUE_MAX)
      continue;

    csp = gs_process_conn_new (c->spy, FALSE);
    csp->port = port;

    G_LOCK (gs_process_lock);

    pthread_create (&csp->thread_id, NULL,
                    gs_process_connect_scan_ports_thread, csp);
    csp->timeout_id =
      g_timeout_add (GS_PROCESS_TIMEOUT_SCAN_PORTS * 1000,
                     gs_process_timeout_cb, csp);
    pthread_join (csp->thread_id, NULL);

    G_UNLOCK (gs_process_lock);

    if (csp->timeout_id)
      g_source_remove (csp->timeout_id), csp->timeout_id = 0;

    /* Opened */
    if (c->spy->scan_ports_compare == GS_SCAN_PORTS_CHOICE_OPENED)
    {
      if (csp->is_connected)
        list = g_list_append (list, g_strdup (ports[i]));
    }
    /* Closed */
    else if (c->spy->scan_ports_compare == GS_SCAN_PORTS_CHOICE_CLOSED)
    {
      if (!csp->is_connected)
        list = g_list_append (list, g_strdup (ports[i]));
    }

    gs_process_conn_free (csp), csp = NULL;
  }

  g_strfreev (ports), ports = NULL;

  return list;
}
