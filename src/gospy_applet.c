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

#include "gospy_applet.h"
#include "gospy_druid.h"
#include "gospy_properties.h"
#include "gospy_process.h"
#include "gospy_gconf.h"

G_LOCK_DEFINE_STATIC (gs_main_lock);

static void gs_build_liststore (const gchar * name);
static gboolean gs_factory (PanelApplet * applet, const char *iid,
                            gpointer data);
static gboolean gs_applet_fill (PanelApplet * applet);
static void gs_menu_properties_cb (BonoboUIComponent * uic, gpointer data,
                                   const gchar * verbname);
static void gs_menu_about_cb (BonoboUIComponent * uic, gpointer data,
                              const gchar * verbname);
static void gs_menu_help_cb (BonoboUIComponent * uic, gpointer data,
                             const gchar * verbname);
static void gs_menu_newsource_cb (BonoboUIComponent * uic, gpointer data,
                                  const gchar * verbname);
static void gs_menu_resetallstates_cb (BonoboUIComponent * uic, gpointer data,
                                       const gchar * verbname);
static void gs_resetallstates (void);
static gboolean
gs_panel_spy_button_clicked_event_cb (GtkWidget * widget,
                                      GdkEventButton * event, gpointer data);
static void gs_settings_destroy_cb (GtkObject * object, gpointer data);
static void gs_edit_destroy_cb (GtkObject * object, gpointer data);
static void gs_build_treeview (GtkTreeView * tv, const gchar * name);
static void gs_bt_activate_deactivate_all_clicked_cb (GtkButton * button,
                                                      gpointer data);
static GS *gs_new (PanelApplet * applet);
static void gs_free_treeview (const gchar * name);
static void gs_free (void);
static void gs_main_treeview_cursor_changed_cb (GtkTreeView * tv,
                                                gpointer data);
static GdkPixbuf *gs_get_type_pixbuf (const GSType type);
static void gs_settings_open (const gchar * type);
static gboolean
gs_main_treeview_button_release_event_cb (GtkWidget * widget,
                                          GdkEventButton * event,
                                          gpointer data);
static void gs_main_popup_open (GdkEventButton * event);
static void gs_panel_spy_popup_open (GdkEventButton * event,
                                     GSSpyProperties * p);
static GdkPixbuf *gs_get_task_pixbuf (const GSTask task);
static GdkPixbuf *gs_get_state_pixbuf (const GSState state,
                                       const gboolean changed);
static void gs_main_popup_activated_activate_cb (GtkMenuItem * menuitem,
                                                 gpointer data);
static void gs_main_popup_event_occured_activate_cb (GtkMenuItem * menuitem,
                                                     gpointer data);
static void gs_main_popup_view_online_activate_cb (GtkMenuItem * menuitem,
                                                   gpointer data);
static void gs_main_popup_view_results_activate_cb (GtkMenuItem * menuitem,
                                                    gpointer data);
static void gs_main_popup_edit_activate_cb (GtkMenuItem * menuitem,
                                            gpointer data);
static void gs_edit_web_page (const gchar * name);
static void gs_edit_server (const gchar * name);
static gboolean gs_edit_set_view_diff_button_state (void);
static void gs_edit_bt_validate_clicked_cb (GtkButton * button, gpointer data);
static void gs_edit_bt_view_online_clicked_cb (GtkButton * button,
                                               gpointer data);
static void gs_bt_treeview_clear_clicked_cb (GtkButton * button, gpointer data);
static void gs_schedule_clicked_cb (GtkWidget * widget, gpointer data);
static GSCurrent *gs_current_new (void);
static void gs_delete_selected_spy (void);
static gboolean gs_applet_clicked_cb (GtkWidget * widget,
                                      GdkEventButton * event, gpointer data);
static GSGui *gs_gui_new (void);
static gboolean gs_refresh_cb (gpointer data);
static void gs_edit_bt_view_diff_clicked_cb (GtkButton * button, gpointer data);
static void gs_settings_toggled_cb (GtkToggleButton * togglebutton,
                                    gpointer data);
static void gs_actions_email_toggled_cb (GtkToggleButton * togglebutton,
                                         gpointer data);
static void gs_color_selector_color_set (GtkWidget * colorpicker,
                                         gpointer data);
static gchar *gs_get_spy_tooltip (const GSSpyProperties * p);
static void gs_panel_spy_popup_destroy_cb (GtkObject * object, gpointer data);
static gboolean
gs_panel_applet_clicked_event_cb (GtkWidget * widget,
                                  GdkEventButton * event, gpointer data);
static void
gs_applet_change_orient_cb (GtkWidget * w, PanelAppletOrient o,
                            gpointer * data);
static void
gs_main_popup_run_now_activate_cb (GtkMenuItem * menuitem, gpointer data);
static void gs_gui_update (void);
static void gs_free_common (void);
static void gs_info_panel_update (GladeXML * g, const GSSpyProperties * p,
                                  const gchar * type);
static void gs_info_panel_empty (GladeXML * g);
static gint main_treeview_sort_func (GtkTreeModel * model,
                                     GtkTreeIter * a,
                                     GtkTreeIter * b, gpointer data);
static void gs_change_spy_image (const GSSpyProperties * p);
static void gs_change_applet_image (const gchar * icon_name);
static void
gs_color_panel_spy_selected (GSSpyProperties * p, const gboolean selected);
static gchar *gs_build_spy_treeview_path (const guint id);
static void gs_edit_bt_actions_clicked_cb (GtkButton * button, gpointer data);
static void
gs_actions_bt_validate_clicked_cb (GtkButton * button, gpointer data);
static void
gs_actions_bt_email_test_clicked_cb (GtkButton * button, gpointer data);
static void gs_actions_destroy_cb (GtkObject * object, gpointer data);

static const BonoboUIVerb gs_menu_verbs[] = {
  BONOBO_UI_UNSAFE_VERB ("gs_newsource", gs_menu_newsource_cb),
  BONOBO_UI_UNSAFE_VERB ("gs_resetallstates", gs_menu_resetallstates_cb),
  BONOBO_UI_UNSAFE_VERB ("gs_properties", gs_menu_properties_cb),
  BONOBO_UI_UNSAFE_VERB ("gs_about", gs_menu_about_cb),
  BONOBO_UI_UNSAFE_VERB ("gs_help", gs_menu_help_cb),
  BONOBO_UI_VERB_END
};

void
gs_split_uri (const gchar * uri, gchar ** scheme, guint * port,
              gchar ** hostname, gchar ** path)
{
  GnomeVFSURI *u = NULL;
  gchar *uri_real = NULL;

  gdk_threads_enter ();

  uri_real = (strstr (uri, "://")) ?
    g_strdup (uri) : g_strconcat ("http://", uri, NULL);
  u = gnome_vfs_uri_new (uri_real);
  g_free (uri_real), uri_real = NULL;

  *scheme = g_strdup (gnome_vfs_uri_get_scheme (u));
  *port = gnome_vfs_uri_get_host_port (u);
  *hostname = g_strdup (gnome_vfs_uri_get_host_name (u));
  *path = g_strdup (gnome_vfs_uri_get_path (u));
  gnome_vfs_uri_unref (u), u = NULL;

  if (*path == NULL || **path == '\0')
  {
    g_free (*path), *path = NULL;
    *path = g_strdup ("/");
  }
  if (*port == 0)
    *port = (!g_ascii_strcasecmp (*scheme, "https")) ? 443 : 80;

  gdk_threads_leave ();
}

void
gs_file_save (const time_t timestamp, const GSAlert alert,
              const guint index, const gchar * content)
{
  gchar *filename = NULL;
  GError *error = NULL;

  G_LOCK (gs_main_lock);

  filename =
    g_strdup_printf ("%s/logs/%u-%u:%u.file", gs->home_path,
                     (guint) timestamp, alert, index);

  if (!g_file_set_contents (filename, content, -1, &error))
  {
    g_warning ("%s \"%s\": %s\n",
               _("Failed writing file"), filename, error->message);
    g_error_free (error), error = NULL;
  }

  g_free (filename), filename = NULL;

  G_UNLOCK (gs_main_lock);
}

void
gs_file_save_diff (const time_t timestamp, const GSAlert alert,
                   const guint suffix)
{
  FILE *p = NULL;
  FILE *f = NULL;
  gchar *command = NULL;
  gchar *filename = NULL;
  gchar buffer[GS_BUFFER_LEN + 1];

  G_LOCK (gs_main_lock);

  command =
    g_strdup_printf
    ("diff --ignore-case --ignore-tab-expansion --ignore-space-change "
     "--ignore-blank-lines %s/logs/%u-%u:0.file %s/logs/%u-%u:1.file",
     gs->home_path, (guint) timestamp, alert, gs->home_path,
     (guint) timestamp, alert);
  filename =
    g_strdup_printf ("%s/logs/%u-%u:%u.diff", gs->home_path,
                     (guint) timestamp, alert, suffix);

  if ((p = popen (command, "r")))
  {
    if ((f = fopen (filename, "w")))
    {
      while (fgets (buffer, GS_BUFFER_LEN, p))
        fprintf (f, "%s", buffer);
      fclose (f);
    }
    else
      g_warning ("%s: %s\n", _("Could not open file"), filename);

    pclose (p);
  }
  else
    g_warning ("%s: %s\n", _("Could not open pipe"), command);

  g_free (filename), filename = NULL;
  g_free (command), command = NULL;

  G_UNLOCK (gs_main_lock);
}

void
gs_sendmail (const gchar * server, const guint port, const gchar * to,
             const gchar * subject, const gchar * data, GError ** error)
{
#ifdef ENABLE_LIBESMTP
  smtp_session_t session;
  smtp_message_t message;
  const smtp_status_t *status;
  gchar *server_port = NULL;
  gchar *mail_subject = NULL;
  gchar *mail_data = NULL;
  gchar *tmp = NULL;

  server_port = g_strdup_printf ("%s:%u", server, port);
  mail_subject = g_strdup_printf ("[gospy-applet] %s", subject);
  mail_data = g_strdup_printf ("MIME-Version: 1.0\r\n"
                               "Content-Type: text/plain; charset=utf-8\r\n"
                               "Content-Transfer-Encoding: 8bit\r\n"
                               "From: %s\r\n"
                               "To: %s"
                               "\r\n\r\n"
                               "%s"
                               "\r\n\r\n"
                               "%s",
                               to,
                               to,
                               data,
                               _
                               ("*This message has been automatically sent by gospy-applet*"));

  session = smtp_create_session ();
  message = smtp_add_message (session);
  smtp_set_server (session, server_port);

  smtp_set_reverse_path (message, to);
  smtp_set_header (message, "To", NULL, NULL);
  smtp_add_recipient (message, to);
  smtp_set_header (message, "Subject", mail_subject);
  smtp_set_header_option (message, "Subject", Hdr_OVERRIDE, 1);
  smtp_set_message_str (message, mail_data);

  //g_print ("* Server: %s\n* Subject: %s\n* Data:%s\n", server_port, mail_subject, mail_data);

  if (!smtp_start_session (session))
  {
    char buf[1024];

    tmp = g_strdup_printf ("%s:\n\n%s",
                           _("A error occured while contacting SMTP server"),
                           smtp_strerror (smtp_errno (), buf, sizeof buf));

    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, tmp);
  }
  else
  {
    status = smtp_message_transfer_status (message);
    tmp = g_strdup_printf ("%s:\n\n%d %s",
                           _("Test message has been sent"),
                           status->code,
                           (status->text != NULL) ? status->text : "");
    g_set_error (error, G_OPTION_ERROR, -1, tmp);
  }

  g_free (tmp), tmp = NULL;

  smtp_destroy_session (session);

  g_free (server_port), server_port = NULL;
  g_free (mail_subject), mail_subject = NULL;
  g_free (mail_data), mail_data = NULL;
#endif
}

void
gs_append_to_spy_log (const time_t timestamp, const gchar * line)
{
  gchar *filename = NULL;
  FILE *fd = NULL;
  struct tm *st;
  time_t t;
  gchar ftime[20 + 1];

  G_LOCK (gs_main_lock);

  t = time (NULL);
  st = localtime (&t);
  filename =
    g_strdup_printf ("%s/logs/%u.log", gs->home_path, (guint) timestamp);

  if ((fd = fopen (filename, "a")))
  {
    strftime (ftime, 20, "%Y-%m-%d\t%H:%M:%S", st);
    fprintf (fd, "%s\t%s\n", ftime, line);
    fclose (fd);
  }
  else
    g_warning ("%s: %s\n", _("Could not open file"), filename);

  g_free (filename), filename = NULL;

  G_UNLOCK (gs_main_lock);
}

gboolean
gs_scan_ports_is_valid (gchar * port)
{
  gchar *p = NULL;

  if (strchr (",-", *port) ||
      strchr (",-", port[strlen (port) - 1]) ||
      ((p = strchr (port, '-')) && (strchr (++p, '-') || strchr (port, ','))))
    return FALSE;

  p = port;
  do
    if (*p != ',' && *p != '-' && !g_ascii_isdigit (*p))
      return FALSE;
  while (*(++p));

  return TRUE;
}

gboolean
gs_target_ip_is_valid (gchar * ip)
{
  gchar *p = ip;

  do
    if (*p != '.' && !g_ascii_isdigit (*p))
      return FALSE;
  while (*(++p));
  return TRUE;
}

gchar *
gs_strchug (gchar * string)
{
  const gchar *start;

  for (start = (gchar *) string;
       (*start) && ((*start == '\n') ||
                    (*start == '\r') ||
                    (*start == '\t') || (g_ascii_isspace (*start))); start++);

  g_memmove (string, start, strlen ((gchar *) start) + 1);

  return string;
}

gchar *
gs_strchomp (gchar * string)
{
  gsize len = 0;

  len = strlen (string);
  while (len--)
    if ((string[len] == '\n') ||
        (string[len] == '\r') ||
        (string[len] == '\t') || (g_ascii_isspace (string[len])))
      string[len] = '\0';
    else
      break;

  return string;
}

gchar *
gs_convert_to_utf8 (const gchar * data)
{
  const gchar *end;
  gchar *ret = NULL;
  gsize len = 0;

  // FIXME g_locale_to_utf8()?

  if (data != NULL)
  {
    if (!g_utf8_validate (data, strlen (data), &end))
    {
      const gchar *charset;
      const gchar *encodings_to_try[2] = { 0, 0 };
      gboolean utf8 = FALSE;

      utf8 = g_get_charset (&charset);

      if (!utf8)
        encodings_to_try[0] = charset;

      if (g_ascii_strcasecmp (charset, "ISO-8859-1"))
        encodings_to_try[1] = "ISO-8859-1";

      if (encodings_to_try[0])
        ret = g_convert (data, strlen (data), "UTF-8",
                         encodings_to_try[0], NULL, &len, NULL);

      if (ret == NULL && encodings_to_try[1])
        ret = g_convert (data, strlen (data), "UTF-8",
                         encodings_to_try[1], NULL, &len, NULL);
    }
    else
      ret = g_strdup (data);
  }

  if (ret == NULL)
    ret = g_strdup ("");

  return ret;
}

void
gs_show_message (const gchar * message, const GtkMessageType msg_type)
{
  GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                              GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              msg_type,
                                              GTK_BUTTONS_OK,
                                              message);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog), dialog = NULL;
}

static void
gs_applet_change_orient_cb (GtkWidget * w, PanelAppletOrient o, gpointer * data)
{
  PanelApplet *applet = gs->applet;

  gs_free_common ();
  gs = gs_new (applet);

  gtk_widget_show_all (GTK_WIDGET (applet));

  gs_gui_update ();
}

static void
gs_actions_bt_email_test_clicked_cb (GtkButton * button, gpointer data)
{
  GladeXML *g = gs->gui->glade_actions_dialog;
  GSSpyProperties *p = gs->current->spy;
  GSActionEmail *action_email = NULL;
  gchar *tmp = NULL;
  GError *error = NULL;

  action_email = gs_properties_action_email_new ();

  action_email->enabled = (gboolean) GTB (WGET (g, "email_action"));
  action_email->address =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_address"))));
  action_email->server =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_server"))));
  action_email->port =
    gtk_spin_button_get_value (GTK_SPIN_BUTTON (WGET (g, "email_port")));
  action_email->login =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_login"))));
  action_email->password =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_password"))));

  tmp = g_strdup_printf ("%s: %s\r\n%s: %s\r\n\r\n%s\r\n\r\n",
                         _("Title"), p->title, _("Target"), p->target,
                         _
                         ("You asked gospy-applet to send a E-Mail test...\r\n\r\nSo, here I am :-)"));
  gs_sendmail (action_email->server, action_email->port, action_email->address,
               _("E-Mail test"), tmp, &error);
  g_free (tmp), tmp = NULL;

  if (error)
  {
    gs_show_message (error->message,
                     (error->code ==
                      G_OPTION_ERROR_BAD_VALUE) ? GTK_MESSAGE_ERROR :
                     GTK_MESSAGE_INFO);
    g_error_free (error), error = NULL;
  }
}

static void
gs_actions_bt_validate_clicked_cb (GtkButton * button, gpointer data)
{
  GladeXML *g = gs->gui->glade_actions_dialog;
  GSSpyProperties *p = gs->current->spy;

  gs_properties_action_email_free (p->action_email), p->action_email = NULL;
  p->action_email = gs_properties_action_email_new ();

  p->action_email->enabled = (gboolean) GTB (WGET (g, "email_action"));
  p->action_email->address =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_address"))));
  p->action_email->server =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_server"))));
  p->action_email->port =
    gtk_spin_button_get_value (GTK_SPIN_BUTTON (WGET (g, "email_port")));
  p->action_email->login =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_login"))));
  p->action_email->password =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (WGET (g, "email_password"))));

  gtk_widget_destroy (gs->gui->actions_dialog), gs->gui->actions_dialog = NULL;
}

static void
gs_edit_bt_actions_clicked_cb (GtkButton * button, gpointer data)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;
  GtkWidget *w1 = NULL;

  g = glade_xml_new (GLADE_FILE, "actions_dialog", NULL);
  w = WGET (g, "actions_dialog");

  if (gs->gui->glade_actions_dialog)
    g_object_unref (gs->gui->glade_actions_dialog);
  gs->gui->glade_actions_dialog = g;
  gs->gui->actions_dialog = w;

  g_signal_connect (G_OBJECT (w), "destroy",
                    G_CALLBACK (gs_actions_destroy_cb), NULL);

  w1 = WGET (g, "bt_action_cancel");
  g_signal_connect_swapped (G_OBJECT (w1), "clicked",
                            G_CALLBACK (gtk_widget_destroy), w);

  w1 = WGET (g, "bt_action_validate");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_actions_bt_validate_clicked_cb), NULL);

  w1 = WGET (g, "bt_email_test");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_actions_bt_email_test_clicked_cb), NULL);

  w1 = WGET (g, "email_action");
  g_signal_connect (G_OBJECT (w1), "toggled",
                    G_CALLBACK (gs_actions_email_toggled_cb), NULL);

  GSSpyProperties *p = gs->current->spy;

  if (p->action_email != NULL)
  {
    if (p->action_email->address != NULL)
      gtk_entry_set_text (GTK_ENTRY (WGET (g, "email_address")),
                          p->action_email->address);

    if (p->action_email->server != NULL)
      gtk_entry_set_text (GTK_ENTRY (WGET (g, "email_server")),
                          p->action_email->server);

    if (p->action_email->login != NULL)
      gtk_entry_set_text (GTK_ENTRY (WGET (g, "email_login")),
                          p->action_email->login);

    if (p->action_email->password != NULL)
      gtk_entry_set_text (GTK_ENTRY (WGET (g, "email_password")),
                          p->action_email->password);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (WGET (g, "email_port")),
                               p->action_email->port);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1),
                                  p->action_email->enabled);
  }
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), FALSE);

#ifndef ENABLE_LIBESMTP
  gtk_widget_set_sensitive (WGET (g, "email_table"), FALSE);
#endif

  gtk_widget_show_all (w);
}

static void
gs_bt_activate_deactivate_all_clicked_cb (GtkButton * button, gpointer data)
{
  GladeXML *g = gs->gui->glade_settings_dialog;
  GList *item = NULL;
  GtkWidget *w = NULL;
  gboolean deactivate = FALSE;

  w = WGET (g, "bt_deactivate_all");
  deactivate = GTB (w);

  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;

    p->state = (deactivate) ? GS_STATE_DEACTIVATED : GS_STATE_ACTIVATED;

    item = g_list_next (item);
  }

  gs->deactivated_all = deactivate;

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

gint
gs_get_selected_row_id (GtkTreeView * tv)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = NULL;
  gint32 ret = -1;

  gdk_threads_enter ();

  select = gtk_tree_view_get_selection (tv);

  if (select != NULL && gtk_tree_selection_get_selected (select, &model, &iter))
    gtk_tree_model_get (model, &iter, 0, &ret, -1);

  gdk_threads_leave ();

  return ret;
}

static void
gs_info_panel_empty (GladeXML * g)
{
  gtk_entry_set_text (GTK_ENTRY (WGET (g, "target")), "");
  gtk_entry_set_text (GTK_ENTRY (WGET (g, "title")), "");
  gtk_label_set_label (GTK_LABEL (WGET (g, "next_exec")), "");
}


gchar *
gs_get_combo_groups_selection (GtkWidget * combo)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;
  gchar *group = NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    gtk_tree_model_get (model, &iter, 0, &group, -1);

  return group;
}

void
gs_build_combo_groups (GtkWidget * combo, const gchar * group)
{
  GtkListStore *ls = NULL;
  GtkTreeIter iter;
  GtkTreeIter default_iter;
  GtkCellRenderer *trenderer = NULL;
  GList *item = NULL;
  gboolean set_default = FALSE;


  ls = gtk_list_store_new (1, G_TYPE_STRING);

  if (gs->groups_list != NULL)
  {
    item = g_list_first (gs->groups_list);
    item = g_list_sort (item, (GCompareFunc) strcmp);

    gtk_list_store_prepend (ls, &iter);
    gtk_list_store_set (ls, &iter, 0, " ", -1);

    while (item != NULL)
    {
      gchar *g = (gchar *) item->data;
      gtk_list_store_append (ls, &iter);
      gtk_list_store_set (ls, &iter, 0, g, -1);

      if (group != NULL && strcmp (g, group) == 0)
      {
        set_default = TRUE;
        default_iter = iter;
      }

      item = g_list_next (item);
    }
  }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (ls));

  g_object_unref (ls), ls = NULL;

  if (set_default)
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &default_iter);

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
  trenderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), trenderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), trenderer,
                                  "text", 0, NULL);
  gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo), trenderer, 0);
}


static void
gs_info_panel_update (GladeXML * g, const GSSpyProperties * p,
                      const gchar * type)
{
  GtkWidget *w = NULL;
  gchar *t_c = NULL;
  gchar *t_u = NULL;
  gchar *t_ex = NULL;
  gchar *t_nex = NULL;
  gchar *t_e = NULL;
  gchar *target = NULL;


  target = (gs->current->spy->type == GS_TYPE_WEB_PAGE) ?
    g_strdup_printf ("%s://%s:%u%s", p->scheme, p->hostname, p->port, p->path)
    : g_strdup (p->target);

  t_nex =
    gs_get_timestamp2string (p->timestamp_next_exec, GS_DATE_HOUR_FORMAT_LABEL);

  /* Main information panel under spies tree view */
  if (strcmp (type, "main") == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (WGET (g, "target")), target);

    gtk_entry_set_text (GTK_ENTRY (WGET (g, "title")), p->title);

    w = WGET (g, "next_exec");
    if (*t_nex)
      gtk_label_set_label (GTK_LABEL (w), t_nex);
    else
      gtk_label_set_label (GTK_LABEL (w), _("<i>Not yet</i>"));
  }
  /* Spy properties notebook */
  else if (strcmp (type, "edit") == 0)
  {
    t_c = gs_get_timestamp2string (p->timestamp, GS_DATE_HOUR_FORMAT_LABEL);
    t_u = gs_get_timestamp2string (p->timestamp_update,
                                   GS_DATE_HOUR_FORMAT_LABEL);
    t_ex = gs_get_timestamp2string (p->timestamp_last_exec,
                                    GS_DATE_HOUR_FORMAT_LABEL);
    t_e = gs_get_timestamp2string (p->timestamp_last_event,
                                   GS_DATE_HOUR_FORMAT_LABEL);

    gs_build_combo_groups (WGET (g, "edit_group"), p->group);

    gtk_entry_set_text (GTK_ENTRY (WGET (g, "edit_target")), target);

    gtk_entry_set_text (GTK_ENTRY (WGET (g, "edit_title")), p->title);

    gtk_label_set_label (GTK_LABEL (WGET (g, "edit_creation")), t_c);

    w = WGET (g, "edit_update");
    if (*t_u)
      gtk_label_set_label (GTK_LABEL (w), t_u);
    else
      gtk_label_set_label (GTK_LABEL (w), _("<i>Not yet</i>"));

    w = WGET (g, "edit_last_exec");
    if (*t_ex)
      gtk_label_set_label (GTK_LABEL (w), t_ex);
    else
      gtk_label_set_label (GTK_LABEL (w), _("<i>Not yet</i>"));

    w = WGET (g, "edit_next_exec");
    if (*t_nex)
      gtk_label_set_label (GTK_LABEL (w), t_nex);
    else
      gtk_label_set_label (GTK_LABEL (w), _("<i>Not yet</i>"));

    w = WGET (g, "edit_last_event");
    if (*t_e)
      gtk_label_set_label (GTK_LABEL (w), t_e);
    else
      gtk_label_set_label (GTK_LABEL (w), _("<i>Not yet</i>"));
  }

  g_free (target), target = NULL;
  g_free (t_c), t_c = NULL;
  g_free (t_u), t_u = NULL;
  g_free (t_ex), t_ex = NULL;
  g_free (t_nex), t_nex = NULL;
  g_free (t_e), t_e = NULL;
}


static void
gs_delete_selected_spy (void)
{
  gint id = 0;
  GSSpyProperties *p = NULL;
  GladeXML *g = gs->gui->glade_settings_dialog;


  if ((id = gs_get_selected_row_id (gs->main_treeview)) < 0)
    return;

  if (!(p = gs_properties_spy_lookup_by_id (id)))
    return;

  gs_color_panel_spy_selected (p, FALSE);

  gs_properties_node_free (p, TRUE), p = NULL;
  gs->current->spy = NULL;

  gs_info_panel_empty (g);

  if (gs->spies_list == NULL || g_list_length (gs->spies_list) == 0)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                  (WGET (g, "display_main_icon")), TRUE);
    gs_set_display_main_icon (TRUE);
  }

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

static void
gs_menu_newsource_cb (BonoboUIComponent * uic, gpointer data,
                      const gchar * verbname)
{
  gs_druid_open ();
}

static void
gs_menu_resetallstates_cb (BonoboUIComponent * uic, gpointer data,
                           const gchar * verbname)
{
  gs_resetallstates ();
}

static void
gs_build_liststore (const gchar * name)
{
  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  /* Main */
  if (strcmp (name, "main") == 0)
    gs->main_liststore =
      gtk_list_store_new (MAIN_COLUMN_NB, G_TYPE_INT, G_TYPE_STRING,
                          GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF,
                          G_TYPE_STRING, G_TYPE_STRING);
  /* Groups */
  else if (strcmp (name, "groups") == 0)
    gs->groups_liststore = gtk_list_store_new (1, G_TYPE_STRING);
  /* Spy Alerts */
  else if (strcmp (name, "alerts") == 0)
    gs->alerts_liststore =
      gtk_list_store_new (ALERTS_COLUMN_NB,
                          G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                          G_TYPE_STRING, G_TYPE_STRING);
  /* Spy Changes */
  else if (strcmp (name, "changes") == 0)
    gs->changes_liststore =
      gtk_list_store_new (CHANGES_COLUMN_NB, G_TYPE_INT, G_TYPE_STRING,
                          G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                          G_TYPE_STRING);

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

static gint
main_treeview_sort_func (GtkTreeModel * model,
                         GtkTreeIter * a, GtkTreeIter * b, gpointer data)
{
  gint sort_col = GPOINTER_TO_INT (data);
  gint32 id_a = -1;
  gint32 id_b = -1;
  gint ret = -1;
  GSSpyProperties *p_a = NULL;
  GSSpyProperties *p_b = NULL;

  gtk_tree_model_get (model, a, 0, &id_a, -1);
  gtk_tree_model_get (model, b, 0, &id_b, -1);

  p_a = gs_properties_spy_lookup_by_id (id_a);
  p_b = gs_properties_spy_lookup_by_id (id_b);

  switch (sort_col)
  {
  case MAIN_COLUMN_TYPE:
    if (p_a->type > p_b->type)
      ret = 1;
    break;

  case MAIN_COLUMN_STATE:
    if (p_a->changed || p_b->changed)
    {
      if (p_a->changed > p_b->changed)
        ret = 1;
    }
    else if (p_a->state > p_b->state)
      ret = 1;
    break;

  case MAIN_COLUMN_TASK:
    if (p_a->task > p_b->task)
      ret = 1;
  }

  return ret;
}


static void
gs_spy_update_groups (const gchar * old, const gchar * new)
{
  GList *item = NULL;


  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *s = (GSSpyProperties *) item->data;

    if (strcmp (s->group, old) == 0)
    {
      g_free (s->group), s->group = NULL;
      s->group = g_strdup (new);
    }

    item = g_list_next (item);
  }
}


static void
gs_spy_reset_groups (const gchar * group)
{
  GList *item = NULL;


  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *s = (GSSpyProperties *) item->data;

    if (strcmp (s->group, group) == 0)
    {
      g_free (s->group), s->group = NULL;
      s->group = g_strdup (" ");
    }

    item = g_list_next (item);
  }
}


static GList *
gs_groups_lookup (const gchar * group)
{
  GList *item = NULL;


  item = g_list_first (gs->groups_list);
  while (item != NULL)
  {
    if (strcmp ((gchar *) item->data, group) == 0)
      return item;

    item = g_list_next (item);
  }

  return NULL;
}


static void
gs_delete_group (const gchar * group)
{
  GList *item = NULL;


  item = g_list_first (gs->groups_list);
  while (item != NULL)
  {
    if (strcmp ((gchar *) item->data, group) == 0)
    {
      gchar *p = (gchar *) item->data;

      gs->groups_list = g_list_remove (gs->groups_list, item->data);
      g_free (p), p = NULL;

      return;
    }

    item = g_list_next (item);
  }
}


static void
gs_groups_changed_cb (GtkCellRendererText * renderer,
                      gchar * path_string, gchar * new_text,
                      GtkListStore * master)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  gchar *old_text;
  GladeXML *g = gs->gui->glade_settings_dialog;
  GtkTreePath *path = NULL;


  path = gtk_tree_path_new_from_string (path_string);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (WGET (g, "groups_treeview")));

  if (gtk_tree_model_get_iter (model, &iter, path))
  {
    gtk_tree_model_get (model, &iter, 0, &old_text, -1);

    /* Replace old group name */
    if (strcmp (old_text, new_text) != 0)
    {
      GList *item = NULL;

      item = gs_groups_lookup (old_text);
      g_free (item->data);
      item->data = strdup (new_text);

      gs_spy_update_groups (old_text, new_text);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new_text, -1);

      GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                           GS_REFRESH_TYPE_GROUPS_COMBO_VIEW |
                           GS_REFRESH_TYPE_SAVE_DATA);

    }

    g_free (old_text), old_text = NULL;
  }

  gtk_tree_path_free (path), path = NULL;
}


static void
gs_add_group (void)
{
  if (gs_groups_lookup (_("New group")) == NULL)
    gs->groups_list = g_list_prepend (gs->groups_list,
                                      g_strdup (_("New group")));

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_TREE_VIEW |
                       GS_REFRESH_TYPE_GROUPS_COMBO_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}


static void
gs_delete_selected_group (void)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;
  GladeXML *g = gs->gui->glade_settings_dialog;
  GtkTreeSelection *select = NULL;
  GtkTreeView *tv = GTK_TREE_VIEW (WGET (g, "groups_treeview"));
  gchar *text = NULL;


  if ((select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv))) &&
      gtk_tree_selection_get_selected (select, &model, &iter))
  {
    gtk_tree_model_get (model, &iter, 0, &text, -1);

    /* Delete group */
    gs_delete_group (text);
    /* Deassociate spies from this group */
    gs_spy_reset_groups (text);

    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

    if (gs->main_tree_filter && strcmp (gs->main_tree_filter, text) == 0)
      g_free (gs->main_tree_filter), gs->main_tree_filter = NULL;

    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                         GS_REFRESH_TYPE_GROUPS_COMBO_VIEW |
                         GS_REFRESH_TYPE_SAVE_DATA);

  }
}


static void
gs_build_treeview (GtkTreeView * tv, const gchar * name)
{
  GtkTreeView *t = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkCellRenderer *prenderer = NULL;
  GtkTreeViewColumn *column = NULL;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  renderer = gtk_cell_renderer_text_new ();
  prenderer = gtk_cell_renderer_pixbuf_new ();

  /* Main */
  if (strcmp (name, "main") == 0)
  {
    gs->main_treeview = t = tv;

    /* Id */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 "", renderer, "text",
                                                 MAIN_COLUMN_ID, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_ID);
    gtk_tree_view_column_set_visible (column, FALSE);

    /* Group */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 _("Group"),
                                                 renderer, "text",
                                                 MAIN_COLUMN_GROUP, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_GROUP);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, MAIN_COLUMN_GROUP);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Type */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 _("Type"),
                                                 prenderer, "pixbuf",
                                                 MAIN_COLUMN_TYPE, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_TYPE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (gs->main_liststore),
                                     MAIN_COLUMN_TYPE, main_treeview_sort_func,
                                     GINT_TO_POINTER (MAIN_COLUMN_TYPE), NULL);

    gtk_tree_view_column_set_sort_column_id (column, MAIN_COLUMN_TYPE);

    /* State */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 _("State"),
                                                 prenderer, "pixbuf",
                                                 MAIN_COLUMN_STATE, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_STATE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (gs->main_liststore),
                                     MAIN_COLUMN_STATE, main_treeview_sort_func,
                                     GINT_TO_POINTER (MAIN_COLUMN_STATE), NULL);

    gtk_tree_view_column_set_sort_column_id (column, MAIN_COLUMN_STATE);

    /* Process */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 _("Process"),
                                                 prenderer, "pixbuf",
                                                 MAIN_COLUMN_TASK, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_TASK);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (gs->main_liststore),
                                     MAIN_COLUMN_TASK, main_treeview_sort_func,
                                     GINT_TO_POINTER (MAIN_COLUMN_TASK), NULL);

    gtk_tree_view_column_set_sort_column_id (column, MAIN_COLUMN_TASK);

    /* Title */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 _("Title"),
                                                 renderer, "text",
                                                 MAIN_COLUMN_TITLE, NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_TITLE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, MAIN_COLUMN_TITLE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Last event */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Last event"),
                                                 renderer,
                                                 "text",
                                                 MAIN_COLUMN_DATE_LAST_EVENT,
                                                 NULL);
    column = gtk_tree_view_get_column (t, MAIN_COLUMN_DATE_LAST_EVENT);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column,
                                             MAIN_COLUMN_DATE_LAST_EVENT);
    gtk_tree_view_column_set_reorderable (column, TRUE);
  }
  /* Groups */
  else if (strcmp (name, "groups") == 0)
  {
    gs->groups_treeview = t = tv;

    g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
    g_signal_connect (G_OBJECT (renderer), "edited",
                      G_CALLBACK (gs_groups_changed_cb), NULL);

    gtk_tree_view_insert_column_with_attributes (t, -1, _("Name"), renderer,
                                                 "text", 0, NULL);
    column = gtk_tree_view_get_column (t, 0);
    gtk_tree_view_column_set_sort_column_id (column, 0);

  }
  /* Spy Alerts */
  else if (strcmp (name, "alerts") == 0)
  {
    gs->alerts_treeview = t = tv;

    /* Id */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 "", renderer, "text",
                                                 ALERTS_COLUMN_ID, NULL);
    column = gtk_tree_view_get_column (t, ALERTS_COLUMN_ID);
    gtk_tree_view_column_set_visible (column, FALSE);

    /* Date */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Date"), renderer,
                                                 "text", ALERTS_COLUMN_DATE,
                                                 NULL);
    column = gtk_tree_view_get_column (t, ALERTS_COLUMN_DATE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, ALERTS_COLUMN_DATE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Hour */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Hour"), renderer,
                                                 "text", ALERTS_COLUMN_TIME,
                                                 NULL);
    column = gtk_tree_view_get_column (t, ALERTS_COLUMN_TIME);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, ALERTS_COLUMN_TIME);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Type */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Type"), renderer,
                                                 "text", ALERTS_COLUMN_TYPE,
                                                 NULL);
    column = gtk_tree_view_get_column (t, ALERTS_COLUMN_TYPE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, ALERTS_COLUMN_TYPE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Value */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Value"),
                                                 renderer, "text",
                                                 ALERTS_COLUMN_VALUE, NULL);
    column = gtk_tree_view_get_column (t, ALERTS_COLUMN_VALUE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, ALERTS_COLUMN_VALUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);
  }
  /* Spy Changes */
  else if (strcmp (name, "changes") == 0)
  {
    gs->changes_treeview = t = tv;

    /* Id */
    gtk_tree_view_insert_column_with_attributes (t, -1,
                                                 "", renderer, "text",
                                                 CHANGES_COLUMN_ID, NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_ID);
    gtk_tree_view_column_set_visible (column, FALSE);

    /* Date */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Date"), renderer,
                                                 "text",
                                                 CHANGES_COLUMN_DATE, NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_DATE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, CHANGES_COLUMN_DATE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Hour */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Hour"), renderer,
                                                 "text",
                                                 CHANGES_COLUMN_TIME, NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_TIME);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, CHANGES_COLUMN_TIME);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Type */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Type"), renderer,
                                                 "text",
                                                 CHANGES_COLUMN_TYPE, NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_TYPE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, CHANGES_COLUMN_TYPE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* Old value */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("Old value"),
                                                 renderer, "text",
                                                 CHANGES_COLUMN_OLD_VALUE,
                                                 NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_OLD_VALUE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, CHANGES_COLUMN_OLD_VALUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);

    /* New value */
    gtk_tree_view_insert_column_with_attributes (t, -1, _("New value"),
                                                 renderer, "text",
                                                 CHANGES_COLUMN_NEW_VALUE,
                                                 NULL);
    column = gtk_tree_view_get_column (t, CHANGES_COLUMN_NEW_VALUE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, CHANGES_COLUMN_NEW_VALUE);
    gtk_tree_view_column_set_reorderable (column, TRUE);
  }

  gtk_tree_view_set_rules_hint (t, TRUE);

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

void
gs_refresh_alerts_tree_content (GSSpyProperties * p)
{
  GtkTreeIter iter;
  GList *item = NULL;
  GtkTreePath *treepath = NULL;

  if (gs->alerts_liststore == NULL)
    return;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  if (p->alerts_list != NULL)
  {
    gtk_tree_view_get_cursor (gs->alerts_treeview, &treepath, NULL);
    gs_properties_alerts_free (p->alerts_list), p->alerts_list = NULL;
  }

  p->alerts_list = gs_properties_alerts_load (p->timestamp, p->alerts_list);

  gtk_list_store_clear (gs->alerts_liststore);

  item = g_list_first (p->alerts_list);
  while (item != NULL)
  {
    GSAlertProperties *a = (GSAlertProperties *) item->data;

    gtk_list_store_prepend (gs->alerts_liststore, &iter);
    gtk_list_store_set (gs->alerts_liststore, &iter,
                        ALERTS_COLUMN_ID, a->id,
                        ALERTS_COLUMN_DATE, a->date_create,
                        ALERTS_COLUMN_TIME, a->time_create,
                        ALERTS_COLUMN_TYPE, gs_alert_get_label (a->alert_type),
                        ALERTS_COLUMN_VALUE, a->value, -1);

    item = g_list_next (item);
  }

  if (p->alerts_list != NULL)
  {
    if (treepath)
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (gs->alerts_treeview),
                                treepath, NULL, FALSE);
  }

  gtk_tree_path_free (treepath), treepath = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

void
gs_refresh_changes_tree_content (GSSpyProperties * p)
{
  GtkTreeIter iter;
  GList *item = NULL;
  GtkTreePath *treepath = NULL;

  if (gs->changes_liststore == NULL)
    return;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  if (p->changes_list != NULL)
  {
    gtk_tree_view_get_cursor (gs->changes_treeview, &treepath, NULL);
    gs_properties_changes_free (p->changes_list), p->changes_list = NULL;
  }

  p->changes_list = gs_properties_changes_load (p->timestamp, p->changes_list);

  gtk_list_store_clear (gs->changes_liststore);

  item = g_list_first (p->changes_list);
  while (item != NULL)
  {
    GSChangeProperties *c = (GSChangeProperties *) item->data;

    gtk_list_store_prepend (gs->changes_liststore, &iter);
    gtk_list_store_set (gs->changes_liststore, &iter,
                        CHANGES_COLUMN_ID, c->id,
                        CHANGES_COLUMN_DATE, c->date_create,
                        CHANGES_COLUMN_TIME, c->time_create,
                        CHANGES_COLUMN_TYPE,
                        gs_alert_get_label (c->alert_type),
                        CHANGES_COLUMN_OLD_VALUE,
                        c->old_value,
                        CHANGES_COLUMN_NEW_VALUE, c->new_value, -1);

    item = g_list_next (item);
  }

  if (p->changes_list != NULL)
  {
    if (treepath)
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (gs->changes_treeview),
                                treepath, NULL, FALSE);
  }

  gtk_tree_path_free (treepath), treepath = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}


void
gs_refresh_groups_combo_content (void)
{
  GladeXML *g = gs->gui->glade_settings_dialog;
  GtkWidget *wc = WGET (g, "filter_by_group");
  GtkWidget *ws = WGET (g, "filter_section");
  gchar *group = NULL;

  if (gs->groups_list != NULL)
  {
    if ((group = gs_get_combo_groups_selection (wc)) == NULL)
      group = g_strdup (" ");

    gs_build_combo_groups (wc, group);

    g_free (group), group = NULL;

    gtk_widget_show (ws);
  }
  else
    gtk_widget_hide (ws);
}


void
gs_refresh_groups_tree_content (void)
{
  GtkTreeIter iter;
  GList *item = NULL;
  GtkTreePath *treepath = NULL;


  if (gs->groups_liststore == NULL)
    return;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  if (gs->groups_list != NULL)
  {
    gtk_tree_view_get_cursor (gs->groups_treeview, &treepath, NULL);
    gs_properties_groups_free ();
  }

  gs->groups_list = gs_properties_groups_load ();

  gtk_list_store_clear (gs->groups_liststore);

  item = g_list_first (gs->groups_list);
  //FIXME item = g_list_sort (item, (GCompareFunc) strcmp);
  while (item != NULL)
  {
    gtk_list_store_append (gs->groups_liststore, &iter);
    gtk_list_store_set (gs->groups_liststore, &iter,
                        0, (gchar *) item->data, -1);

    item = g_list_next (item);
  }

  if (gs->groups_list != NULL)
  {
    if (treepath)
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (gs->groups_treeview),
                                treepath, NULL, FALSE);
  }

  gtk_tree_path_free (treepath), treepath = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

gchar *
gs_quote_regex_string (const gchar * unquoted_string)
{
  const gchar *p = NULL;
  GString *dest = NULL;
  gchar new[3] = { '\\', 0, '\0' };

  g_return_val_if_fail (unquoted_string != NULL, NULL);

  dest = g_string_new ("");

  p = unquoted_string;

  while (*p)
  {
    switch (*p)
    {
    case '_':
    case '?':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case '*':
    case '$':
    case '/':
    case '\\':
      new[1] = *p;
      g_string_append (dest, new);
      break;
    default:
      g_string_append_c (dest, *p);
    }
    ++p;
  }

  return g_string_free (dest, FALSE);
}

gchar *
gs_strreplace (const gchar * str, const gchar * old, const gchar * new)
{
  guint32 i = 0;
  guint32 str_len = 0;
  guint32 old_len = 0;
  GString *tmp = NULL;

  g_return_val_if_fail ((str != NULL && old != NULL && new != NULL), NULL);

  str_len = strlen (str);
  old_len = strlen (old);

  tmp = g_string_new ("");

  for (i = 0; i < str_len; i++)
    if (!memcmp (&str[i], old, old_len))
      tmp = g_string_append (tmp, new), i += old_len - 1;
    else
      tmp = g_string_append_c (tmp, str[i]);

  return g_string_free (tmp, FALSE);
}

gchar **
gs_html_tag_get_pattern (const GSHTMLTagsChoice tag)
{
  gchar *ret = NULL;

  switch (tag)
  {
  case GS_HTML_TAGS_CHOICE_BODY:
    ret = "<body\\b[^>]*>(.*?)-(.*?)</body>";
    break;
  case GS_HTML_TAGS_CHOICE_DIV:
    ret = "<div\\b[^>]*>(.*?)-(.*?)</div>";
    break;
  case GS_HTML_TAGS_CHOICE_HEAD:
    ret = "<head\\b[^>]*>(.*?)-(.*?)</head>";
    break;
  case GS_HTML_TAGS_CHOICE_HTML:
    ret = "<html\\b[^>]*>(.*?)-(.*?)</html>";
    break;
  case GS_HTML_TAGS_CHOICE_TITLE:
    ret = "<title\\b[^>]*>(.*?)-(.*?)</title>";
    break;

  case GS_HTML_TAGS_CHOICE_HREF:
    ret = "<a\\s+href\\s*=(.*?)-[^<]+</a>";
    break;
  case GS_HTML_TAGS_CHOICE_META:
    ret = "<meta\\s+(.*?)-(.*?)\\\\?>";
    break;
  case GS_HTML_TAGS_CHOICE_ALL:
  default:
    ret = "-";
  }

  return g_strsplit (ret, "-", 0);
}

gchar *
gs_alert_get_label (const GSAlert alert)
{
  gchar *ret = "";

  switch (alert)
  {
  case GS_ALERT_IP_CHANGED:
    ret = _(" IP change ");
    break;
  case GS_ALERT_SERVER_CHANGED:
    ret = _(" Web Server update ");
    break;
  case GS_ALERT_LAST_MODIFIED_CHANGED:
    ret = _(" Modification ");
    break;
  case GS_ALERT_REDIRECTION:
    ret = _(" Redirection ");
    break;
  case GS_ALERT_SIZE_CHANGED:
    ret = _(" Size change ");
    break;
  case GS_ALERT_CONTENT_CHANGED:
    ret = _(" Content change ");
    break;
  case GS_ALERT_STATUS_CHANGED:
    ret = _(" Status change ");
    break;
  case GS_ALERT_CONTENT_FOUND:
    ret = _(" Content found ");
    break;
  case GS_ALERT_PORT_OPENED:
    ret = _(" Opened port");
    break;
  case GS_ALERT_PORT_CLOSED:
    ret = _(" Closed port");
    break;
  case GS_ALERT_CONNECTION_ERROR:
    ret = _(" Connection error ");
    break;
  case GS_ALERT_CONNECTION_TIMEOUT:
    ret = _(" Connection timeout ");
    break;
  case GS_ALERT_STATUS_FOUND:
    ret = _(" Status found ");
    break;
  case GS_ALERT_SIZE_FOUND:
    ret = _(" Size found ");
    break;
  case GS_ALERT_LOAD_TIME_FOUND:
    ret = _(" Load ");
    break;

  default:
    ;
  }

  return ret;
}

gchar *
gs_get_timestamp2string (const time_t timestamp, const gchar * format)
{
  const time_t t = timestamp;
  gchar ftime[255 + 1] = { 0 };

  if (strlen (format) > 50)
  {
    g_warning ("%s\n", _("Date format is too long"));
    return g_strdup ("");
  }

  if (timestamp)
    strftime (ftime, 255, format, localtime (&t));

  return g_strdup (ftime);
}

static void
gs_resetallstates (void)
{
  GList *item = NULL;

  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;
    p->changed = FALSE;
    item = g_list_next (item);
  }

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

void
gs_refresh_main_tree_content (void)
{
  GtkTreeIter iter;
  GList *item = NULL;
  GdkPixbuf *pixbuf_state = NULL;
  GdkPixbuf *pixbuf_task = NULL;
  GdkPixbuf *pixbuf_type = NULL;
  GladeXML *g = gs->gui->glade_settings_dialog;
  GtkWidget *w = NULL;
  GtkTreePath *treepath = NULL;
  gboolean found = FALSE;


  if (gs->main_liststore == NULL || gs->main_treeview == NULL)
    return;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  gtk_tree_view_get_cursor (gs->main_treeview, &treepath, NULL);

  if (treepath == NULL)
    treepath = gtk_tree_path_new_from_string ("0");

  gtk_list_store_clear (gs->main_liststore);

  if (gs->spies_list != NULL)
  {
    gs->deactivated_items = 0;

    item = g_list_first (gs->spies_list);
    while (item != NULL)
    {
      GSSpyProperties *p = (GSSpyProperties *) item->data;

      if (p->state == GS_STATE_DEACTIVATED)
        gs->deactivated_items++;

      /* If we must filter results by group */
      if (gs->main_tree_filter == NULL ||
          strcmp (p->group, gs->main_tree_filter) == 0)
      {
        gchar *t_e = gs_get_timestamp2string (p->timestamp_last_event,
                                              GS_DATE_HOUR_FORMAT);


        found = TRUE;

        pixbuf_state = gs_get_state_pixbuf (p->state, p->changed);
        pixbuf_task = gs_get_task_pixbuf (p->task);
        pixbuf_type = gs_get_type_pixbuf (p->type);

        gtk_list_store_append (gs->main_liststore, &iter);

        gtk_list_store_set (gs->main_liststore, &iter,
                            MAIN_COLUMN_ID, p->timestamp,
                            MAIN_COLUMN_GROUP, p->group,
                            MAIN_COLUMN_TYPE, pixbuf_type,
                            MAIN_COLUMN_STATE, pixbuf_state,
                            MAIN_COLUMN_TASK, pixbuf_task,
                            MAIN_COLUMN_TITLE, p->title,
                            MAIN_COLUMN_DATE_LAST_EVENT, t_e, -1);

        g_object_unref (pixbuf_state), pixbuf_state = NULL;
        g_object_unref (pixbuf_task), pixbuf_task = NULL;
        g_object_unref (pixbuf_type), pixbuf_type = NULL;

        g_free (t_e), t_e = NULL;
      }

      item = g_list_next (item);
    }

    gs->deactivated_all =
      (gs->deactivated_items == g_list_length (gs->spies_list));

    w = WGET (g, "bt_deactivate_all");
    g_signal_handlers_block_by_func (G_OBJECT (w),
                                     G_CALLBACK
                                     (gs_bt_activate_deactivate_all_clicked_cb),
                                     NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), gs->deactivated_all);
    g_signal_handlers_unblock_by_func (G_OBJECT (w),
                                       G_CALLBACK
                                       (gs_bt_activate_deactivate_all_clicked_cb),
                                       NULL);

    if (found)
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (gs->main_treeview),
                                treepath, NULL, FALSE);

  }

  gtk_tree_path_free (treepath), treepath = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

static GdkPixbuf *
gs_get_type_pixbuf (const GSType type)
{
  GdkPixbuf *pix = NULL;

  gdk_threads_enter ();

  pix = (type == GS_TYPE_SERVER) ?
    gdk_pixbuf_new_from_file (GNOME_ICONDIR
                              "/gospy-applet/type_server.png", NULL) :
    gdk_pixbuf_new_from_file (GNOME_ICONDIR
                              "/gospy-applet/type_web_page.png", NULL);

  gdk_threads_leave ();

  return pix;
}

static GdkPixbuf *
gs_get_state_pixbuf (const GSState state, const gboolean changed)
{
  GdkPixbuf *pix = NULL;

  gdk_threads_enter ();

  if (changed)
    pix = gdk_pixbuf_new_from_file (GNOME_ICONDIR
                                    "/gospy-applet/state_changed.png", NULL);
  else
    pix = (state == GS_STATE_ACTIVATED) ?
      gdk_pixbuf_new_from_file (GNOME_ICONDIR
                                "/gospy-applet/state_activated.png", NULL) :
      gdk_pixbuf_new_from_file (GNOME_ICONDIR
                                "/gospy-applet/state_deactivated.png", NULL);

  gdk_threads_leave ();

  return pix;
}

static GdkPixbuf *
gs_get_task_pixbuf (const GSTask task)
{
  GdkPixbuf *pix = NULL;

  gdk_threads_enter ();

  pix = (task == GS_TASK_RUNNING) ?
    gdk_pixbuf_new_from_file (GNOME_ICONDIR
                              "/gospy-applet/task_running.png", NULL) :
    gdk_pixbuf_new_from_file (GNOME_ICONDIR
                              "/gospy-applet/task_sleeping.png", NULL);

  gdk_threads_leave ();

  return pix;
}

static gboolean
gs_edit_set_view_diff_button_state (void)
{
  GSChangeProperties *c = NULL;
  GSSpyProperties *p = gs->current->spy;
  GladeXML *g = gs->gui->glade_edit_dialog;
  GtkWidget *w = NULL;
  gint id = 0;

  id = gs_get_selected_row_id (gs->changes_treeview);
  c = gs_properties_change_lookup_by_id (p->changes_list, id);

  w = WGET (g, "bt_view_diff");
  gtk_widget_set_sensitive (GTK_WIDGET (w),
                            (c && (c->alert_type == GS_ALERT_SIZE_CHANGED ||
                                   c->alert_type == GS_ALERT_CONTENT_CHANGED
                                   || c->alert_type ==
                                   GS_ALERT_LAST_MODIFIED_CHANGED)));

  gs->current->change = c;

  return FALSE;
}

static gchar *
gs_build_spy_treeview_path (const guint id)
{
  GList *item = NULL;
  guint i = 0;
  gchar *ret = NULL;

  item = g_list_first (gs->spies_list);
  for (item = g_list_first (gs->spies_list); ret == NULL && item != NULL;
       item = g_list_next (item), i++)
    if (((GSSpyProperties *) item->data)->timestamp == id)
      ret = g_strdup_printf ("%u", i);

  g_assert (ret != NULL);

  return ret;
}

static void
gs_main_treeview_cursor_changed_cb (GtkTreeView * tv, gpointer data)
{
  GSSpyProperties *p = gs->current->spy;
  GladeXML *g = gs->gui->glade_settings_dialog;

  if (p != NULL)
  {
    gchar *color = gs_get_display_bg_color ();
    gs_color_panel_spy_selected (p, FALSE);
    g_free (color), color = NULL;
  }

  if ((p = gs->current->spy =
       gs_properties_spy_lookup_by_id (gs_get_selected_row_id (tv))) != NULL)
  {
    gs_color_panel_spy_selected (p, TRUE);
    gs_info_panel_update (g, p, "main");
  }
  else
    gs_info_panel_empty (g);
}

static gboolean
gs_main_treeview_button_release_event_cb (GtkWidget * widget,
                                          GdkEventButton * event, gpointer data)
{
  GSSpyProperties *p = gs->current->spy;

  if (p != NULL)
  {
    if (event->type == GDK_2BUTTON_PRESS)
      (gs->current->spy->type == GS_TYPE_WEB_PAGE) ?
        gs_edit_web_page ("edit") : gs_edit_server ("edit");
    else if (event->type == GDK_BUTTON_RELEASE && event->button == 3)
      gs_main_popup_open (event);
  }

  return FALSE;
}

static void
gs_main_popup_view_results_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  (gs->current->spy->type == GS_TYPE_WEB_PAGE) ?
    gs_edit_web_page ("results") : gs_edit_server ("results");
}

static void
gs_edit_bt_view_diff_clicked_cb (GtkButton * button, gpointer data)
{
  GSChangeProperties *c = gs->current->change;
  gchar *filename = g_strdup_printf ("%s/logs/%s-%u:%s.diff", gs->home_path,
                                     c->file_basename, c->alert_type,
                                     c->file_diff_suffix);

  gs_launch_editor (filename);

  g_free (filename), filename = NULL;
}

static void
gs_main_popup_activated_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;

  p->state = (p->state == GS_STATE_ACTIVATED) ?
    GS_STATE_DEACTIVATED : GS_STATE_ACTIVATED;

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

static void
gs_main_popup_event_occured_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;

  p->changed = (!p->changed);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                       GS_REFRESH_TYPE_SAVE_DATA);
}

gchar *
gs_build_full_uri (const GSSpyProperties * p)
{
  return g_strconcat (p->scheme, "://", p->hostname, p->path, NULL);
}

static void
gs_main_popup_view_online_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  GSSpyProperties *p = (GSSpyProperties *) data;
  gchar *uri = gs_build_full_uri (p);

  if (p->changed)
  {
    p->changed = FALSE;
    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                         GS_REFRESH_TYPE_SAVE_DATA);
  }

  gs_launch_browser (uri);

  g_free (uri), uri = NULL;
}

static void
gs_schedule_clicked_cb (GtkWidget * widget, gpointer data)
{
  const gchar *name = NULL;
  GladeXML *g = gs->gui->glade_edit_dialog;

  name = gtk_widget_get_name (widget);

  if (strcmp (name, "edit_option_auto") == 0)
    gtk_widget_set_sensitive (WGET (g, "edit_option_schedule_minutes"), TRUE);
  else if (strcmp (name, "edit_option_schedule_on_request") == 0)
  {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON
                               (WGET (g, "edit_option_schedule_minutes")), 0);

    gtk_widget_set_sensitive (WGET (g, "edit_option_schedule_minutes"), FALSE);
  }

}

static void
gs_bt_treeview_clear_clicked_cb (GtkButton * button, gpointer data)
{
  GSSpyProperties *p = gs->current->spy;

  /* remove spy's files */
  gs_properties_delete_files (p->timestamp);

  gs_refresh_changes_tree_content (p);

  gs_edit_set_view_diff_button_state ();

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW |
                       GS_REFRESH_TYPE_CHANGES_TREE_VIEW);
}

static void
gs_edit_bt_view_online_clicked_cb (GtkButton * button, gpointer data)
{
  GtkWidget *w = NULL;

  w = WGET (gs->gui->glade_edit_dialog, "edit_target");
  gs_launch_browser (gtk_entry_get_text (GTK_ENTRY (w)));
}

void
gs_spy_set_scheduling (GSSpyProperties * p)
{
  if (p->timeout_id)
    g_source_remove (p->timeout_id), p->timeout_id = 0;

  if (p->schedule > 0)
  {
    p->timestamp_next_exec = time (NULL) + (p->schedule * 60);
    p->timeout_id = g_timeout_add ((p->schedule * 60) * 1000, gs_process_cb, p);
  }
}

static void
gs_edit_bt_validate_clicked_cb (GtkButton * button, gpointer data)
{
  const gchar *page = (gchar *) data;
  GladeXML *g = gs->gui->glade_edit_dialog;
  GtkWidget *w = NULL;
  GString *message = NULL;
  gchar *tmp = NULL;
  gchar *tmp1 = NULL;
  gboolean ret = FALSE;
  GSSpyProperties *p = gs->current->spy;
  const gchar *target =
    gtk_entry_get_text (GTK_ENTRY (WGET (g, "edit_target")));

  /* web page */
  if (strcmp (page, "web_page") == 0)
  {
    gboolean size = GTB (WGET (g, "edit_alert_size"));
    guint size_value = gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON (WGET (g, "edit_alert_size_value")));
    gboolean status = GTB (WGET (g, "edit_alert_status"));
    guint status_value =
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
                                        (WGET (g, "edit_alert_status_value")));
    gboolean load_time = GTB (WGET (g, "edit_alert_load_time"));
    guint load_time_value =
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
                                        (WGET
                                         (g, "edit_alert_load_time_value")));
    gboolean last_modified = GTB (WGET (g, "edit_alert_last_modified"));
    gboolean size_changed = GTB (WGET (g, "edit_alert_size_changed"));
    gboolean content_changed = GTB (WGET (g, "edit_alert_content_changed"));
    gboolean status_changed = GTB (WGET (g, "edit_alert_status_changed"));
    gint status_compare =
      gtk_combo_box_get_active (GTK_COMBO_BOX (gs->gui->status_compare_combo));
    gint size_compare =
      gtk_combo_box_get_active (GTK_COMBO_BOX (gs->gui->size_compare_combo));
    gint search_content_in =
      gtk_combo_box_get_active (GTK_COMBO_BOX (gs->gui->search_content_combo));
    gint load_compare =
      gtk_combo_box_get_active (GTK_COMBO_BOX (gs->gui->load_compare_combo));
    gboolean search_content = GTB (WGET (g, "edit_alert_search_content"));
    gboolean search_content_icase =
      GTB (WGET (g, "edit_alert_search_content_icase"));
    const gchar *search_content_value =
      gtk_entry_get_text (GTK_ENTRY
                          (WGET (g, "edit_alert_search_content_value")));
    const gboolean use_auth = GTB (WGET (g, "edit_use_auth"));
    const gchar *auth_user =
      gtk_entry_get_text (GTK_ENTRY (WGET (g, "edit_auth_user")));
    const gchar *auth_password =
      gtk_entry_get_text (GTK_ENTRY (WGET (g, "edit_auth_password")));

    message = g_string_new (_("Please, enter the following elements:\n\n"));

    tmp = g_strdup (target);
    g_strstrip (tmp);
    /* check the "target" entry */
    if (*tmp == '\0')
    {
      ret = TRUE;
      g_string_append (message, _("- The location to monitor\n"));
    }
    /* check if a least one option was selected */
    if (!(size || last_modified || size_changed || content_changed ||
          status || status_changed || load_time || search_content))
    {
      ret = TRUE;
      g_string_append (message, _("- A option\n"));
    }
    g_free (tmp), tmp = NULL;

    /* check the authentication */
    if (use_auth)
    {
      w = WGET (g, "edit_auth_user");
      tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
      g_strstrip (tmp);
      w = WGET (g, "edit_auth_password");
      tmp1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
      g_strstrip (tmp1);
      if ((*tmp != '\0' || *tmp1 != '\0') && (*tmp == '\0' || *tmp1 == '\0'))
      {
        ret = TRUE;
        g_string_append (message, _("- All authentication fields\n"));
      }
      g_free (tmp), tmp = NULL;
      g_free (tmp1), tmp1 = NULL;
    }

    if (!ret)
    {
      if (!size)
        size_value = 0;
      if (!status)
        status_value = 0;
      if (!load_time)
        load_time_value = 0;
      if (!search_content)
        search_content_value = "";

      /* size changed */
      p->size_changed = size_changed;

      /* content changed */
      p->content_changed = content_changed;

      /* size */
      p->size = size_value;
      p->size_compare = size_compare;

      /* status */
      p->status = status_value;
      p->status_compare = status_compare;
      p->status_changed = status_changed;

      /* load time */
      p->load_time = load_time_value;
      p->load_compare = load_compare;

      /* search content */
      g_free (p->search_content), p->search_content = NULL;
      p->search_content = g_strdup (search_content_value);
      p->search_content_in = search_content_in;
      p->search_content_icase = search_content_icase;

      /* last modified */
      p->last_modified = last_modified;

      /* authentication */
      g_free (p->auth_user), p->auth_user = NULL;
      g_free (p->auth_password), p->auth_password = NULL;
      g_free (p->basic_auth_line), p->basic_auth_line = NULL;
      if ((p->use_auth = use_auth))
      {
        p->auth_user = g_strdup (auth_user);
        p->auth_password = g_strdup (auth_password);
        p->basic_auth_line = gs_build_basic_auth_line (auth_user,
                                                       auth_password);
      }
    }
  }
  /* server */
  else
  {
    gboolean alert_ip_changed = GTB (WGET (g, "edit_alert_ip_changed"));
    gboolean alert_server_changed = GTB (WGET (g, "edit_alert_server_changed"));
    gint scan_ports_compare =
      gtk_combo_box_get_active (GTK_COMBO_BOX
                                (gs->gui->scan_ports_compare_combo));
    gboolean scan_ports = GTB (WGET (g, "edit_alert_scan_ports"));
    gchar *scan_ports_value = (gchar *)
      gtk_entry_get_text (GTK_ENTRY (WGET (g, "edit_alert_scan_ports_value")));

    message = g_string_new (_("Please, enter the following elements:\n\n"));
    tmp = g_strdup (target);
    g_strstrip (tmp);
    /* check the "server" entry */
    if (*tmp == '\0')
    {
      ret = TRUE;
      g_string_append (message, _("- The server to monitor\n"));
    }
    /* check if a leat one option was selected */
    if (!(alert_ip_changed || alert_server_changed || scan_ports))
    {
      ret = TRUE;
      g_string_append (message, _("- A option\n"));
    }
    g_free (tmp), tmp = NULL;

    if ((!ret) && scan_ports && scan_ports_value)
    {
      g_strstrip (scan_ports_value);
      /* check the "ports" entry */
      if (*scan_ports_value == '\0')
      {
        ret = TRUE;
        g_string_append (message, _("- A port to check\n"));
      }
      else if (!gs_scan_ports_is_valid (scan_ports_value))
      {
        ret = TRUE;
        g_string_append (message, _("- Scan ports format is wrong\n"));
      }
    }

    if (!ret)
    {
      if (!scan_ports)
        scan_ports_value = "";

      /* ip changed */
      p->ip_changed = alert_ip_changed;
      /* server changed */
      p->server_changed = alert_server_changed;
      /* scan ports */
      g_free (p->scan_ports), p->scan_ports = NULL;
      p->scan_ports = g_strdup (scan_ports_value);
      p->scan_ports_compare = scan_ports_compare;
    }
  }

  if (!ret)
  {
    /* Timestamp update */
    p->timestamp_update = time (NULL);

    /* target */
    g_free (p->target), p->target = NULL;
    p->target = g_strdup (target);

    /* uri fields */
    g_free (p->scheme), p->scheme = NULL;
    g_free (p->hostname), p->hostname = NULL;
    g_free (p->path), p->path = NULL;
    gs_split_uri (p->target, &p->scheme, &p->port, &p->hostname, &p->path);

    /* state */
    w = WGET (g, "edit_option_activated");
    p->state = ((GTB (w))) ? GS_STATE_ACTIVATED : GS_STATE_DEACTIVATED;

    /* Group */
    g_free (p->group), p->group = NULL;
    w = WGET (g, "edit_group");
    if ((p->group = gs_get_combo_groups_selection (w)) == NULL)
      p->group = g_strdup (" ");

    /* title */
    g_free (p->title), p->title = NULL;
    w = WGET (g, "edit_title");
    p->title = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));

    /* schedule */
    if (p->timeout_id)
      g_source_remove (p->timeout_id), p->timeout_id = 0;
    w = WGET (g, "edit_option_schedule_minutes");
    if (GTB (WGET (g, "edit_option_auto")))
    {
      if ((p->schedule =
           gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w))))
        gs_spy_set_scheduling (p);
    }
    else
      p->schedule = 0;

    gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));

    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW |
                         GS_REFRESH_TYPE_SAVE_DATA);
  }
  else
    gs_show_message (message->str, GTK_MESSAGE_ERROR);

  g_string_free (message, TRUE), message = NULL;
}

static void
activate_deactivate_auth_cb (GtkToggleButton * togglebutton, gpointer data)
{
  GladeXML *g = gs->gui->glade_edit_dialog;


  if (GTB (WGET (g, "edit_use_auth")))
    gtk_widget_set_sensitive (WGET (g, "edit_auth_table"), TRUE);
  else
  {
    gtk_widget_set_sensitive (WGET (g, "edit_auth_table"), FALSE);
    gtk_entry_set_text (GTK_ENTRY (WGET (g, "edit_auth_user")), "");
    gtk_entry_set_text (GTK_ENTRY (WGET (g, "edit_auth_password")), "");
  }
}

static void
gs_edit_web_page (const gchar * name)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;
  GtkWidget *w1 = NULL;
  GtkNotebook *nb_main = NULL;
  GSSpyProperties *p = gs->current->spy;

  if (gs_window_present (gs->gui->edit_dialog))
    return;

  gs_color_panel_spy_selected (p, TRUE);

  if (p->changed)
  {
    p->changed = FALSE;
    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);
  }

  g = glade_xml_new (GLADE_FILE, "edit_dialog", NULL);
  w = WGET (g, "edit_dialog");
  nb_main = GTK_NOTEBOOK (WGET (g, "notebook1"));

  if (gs->gui->glade_edit_dialog)
    g_object_unref (gs->gui->glade_edit_dialog);
  gs->gui->glade_edit_dialog = g;
  gs->gui->edit_dialog = w;

  /* web page edition disposition */
  gtk_notebook_remove_page (nb_main, GS_NOTEBOOK_PAGE_MONITORING_SERVER);

  g_signal_connect (G_OBJECT (w), "destroy",
                    G_CALLBACK (gs_edit_destroy_cb), NULL);

  w1 = WGET (g, "bt_cancel");
  g_signal_connect_swapped (G_OBJECT (w1), "clicked",
                            G_CALLBACK (gtk_widget_destroy), w);
  w1 = WGET (g, "bt_web_page_actions");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_actions_clicked_cb), NULL);
  w1 = WGET (g, "bt_validate");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_validate_clicked_cb), "web_page");
  w1 = WGET (g, "bt_view_online");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_view_online_clicked_cb), w);
  w1 = WGET (g, "bt_view_diff");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_view_diff_clicked_cb), w);

  /* display combo for status choices */
  gs->gui->status_compare_combo = gs_build_combo_box ("compare",
                                                      p->status_compare);
  w1 = WGET (g, "edit_status_table");
  gtk_table_attach (GTK_TABLE (w1), gs->gui->status_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for size choices */
  gs->gui->size_compare_combo = gs_build_combo_box ("compare", p->size_compare);
  w1 = WGET (g, "edit_size_table");
  gtk_table_attach (GTK_TABLE (w1), gs->gui->size_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for search content */
  gs->gui->search_content_combo =
    gs_build_combo_box ("search_content_in", p->search_content_in);
  w1 = WGET (g, "edit_search_content_table");
  gtk_table_attach (GTK_TABLE (w1), gs->gui->search_content_combo,
                    0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for load choices */
  gs->gui->load_compare_combo = gs_build_combo_box ("compare", p->load_compare);
  w1 = WGET (g, "edit_load_table");
  gtk_table_attach (GTK_TABLE (w1), gs->gui->load_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* authentication */
  if (p->use_auth)
  {
    w1 = WGET (g, "edit_use_auth");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_auth_table");
    gtk_widget_set_sensitive (w1, (p->basic_auth_line != NULL));
    w1 = WGET (g, "edit_auth_user");
    gtk_entry_set_text (GTK_ENTRY (w1), p->auth_user);
    w1 = WGET (g, "edit_auth_password");
    gtk_entry_set_text (GTK_ENTRY (w1), p->auth_password);
  }
  w1 = WGET (g, "edit_use_auth");
  g_signal_connect (G_OBJECT (w1), "toggled",
                    G_CALLBACK (activate_deactivate_auth_cb), NULL);

  /* alerts tree view */
  gs_free_treeview ("alerts");
  gs_build_liststore ("alerts");
  w1 = WGET (g, "alerts_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w1), "alerts");
  gtk_tree_view_set_model (gs->alerts_treeview,
                           GTK_TREE_MODEL (gs->alerts_liststore));
  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW);

  /* changes tree view */
  gs_free_treeview ("changes");
  gs_build_liststore ("changes");
  w1 = WGET (g, "changes_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w1), "changes");
  gtk_tree_view_set_model (gs->changes_treeview,
                           GTK_TREE_MODEL (gs->changes_liststore));
  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_CHANGES_TREE_VIEW);

  w1 = GTK_WIDGET (gs->changes_treeview);
  g_signal_connect_swapped (G_OBJECT (w1), "cursor_changed",
                            G_CALLBACK (gs_edit_set_view_diff_button_state),
                            NULL);

  w1 = WGET (g, "bt_clear_results");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_bt_treeview_clear_clicked_cb), NULL);
  w1 = GTK_WIDGET (gs->changes_treeview);
  g_signal_connect_swapped (G_OBJECT (w1), "button_release_event",
                            G_CALLBACK (gs_edit_set_view_diff_button_state),
                            NULL);

  gs_info_panel_update (g, p, "edit");

  /* state */
  w1 = WGET (g, (p->state == GS_STATE_ACTIVATED) ?
             "edit_option_activated" : "edit_option_deactivated");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
  /* schedule */
  g_signal_connect (G_OBJECT (WGET (g, "edit_option_schedule_on_request")),
                    "clicked", G_CALLBACK (gs_schedule_clicked_cb), NULL);
  g_signal_connect (G_OBJECT (WGET (g, "edit_option_auto")), "clicked",
                    G_CALLBACK (gs_schedule_clicked_cb), NULL);
  if (p->schedule)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                  (WGET (g, "edit_option_auto")), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON
                               (WGET (g, "edit_option_schedule_minutes")),
                               (gdouble) p->schedule);
  }
  else
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                  (WGET (g, "edit_option_schedule_on_request")),
                                  TRUE);
    gtk_widget_set_sensitive (WGET (g, "edit_option_schedule_minutes"), FALSE);
  }
  /* target */
  w1 = WGET (g, "edit_target");
  gtk_entry_set_text (GTK_ENTRY (w1), p->target);
  /* last modified */
  w1 = WGET (g, "edit_alert_last_modified");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->last_modified);
  /* size changed */
  w1 = WGET (g, "edit_alert_size_changed");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->size_changed);
  /* content changed */
  w1 = WGET (g, "edit_alert_content_changed");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->content_changed);
  /* status changed */
  w1 = WGET (g, "edit_alert_status_changed");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->status_changed);

  /* size */
  if (p->size > 0)
  {
    w1 = WGET (g, "edit_alert_size");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_alert_size_value");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w1), (gdouble) p->size);
  }
  /* status */
  if (p->status > 0)
  {
    w1 = WGET (g, "edit_alert_status");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_alert_status_value");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w1), (gdouble) p->status);
  }
  /* load time */
  if (p->load_time > 0)
  {
    w1 = WGET (g, "edit_alert_load_time");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_alert_load_time_value");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w1), (gdouble) p->load_time);
  }
  /* search content */
  if (p->search_content != NULL && *p->search_content != '\0')
  {
    w1 = WGET (g, "edit_alert_search_content");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_alert_search_content_icase");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1),
                                  p->search_content_icase);
    w1 = WGET (g, "edit_alert_search_content_value");
    gtk_entry_set_text (GTK_ENTRY (w1), p->search_content);
  }

  /* if we are just displaying results */
  if (strcmp (name, "results") == 0)
    gtk_notebook_set_current_page (nb_main, GS_NOTEBOOK_PAGE_RESULTS);

  gtk_widget_show_all (w);
}

gchar *
gs_build_basic_auth_line (const gchar * auth_user, const gchar * auth_password)
{
  gchar *orig = NULL;
  gchar *base64 = NULL;
  gchar *ret = NULL;
  gint size = 0;

  gdk_threads_enter ();

  orig = g_strconcat (auth_user, ":", auth_password, NULL);
  base64 = gnet_base64_encode (orig, strlen (orig), &size, FALSE);
  g_free (orig), orig = NULL;

  ret = g_strconcat ("\r\nAuthorization: Basic ", base64, NULL);
  g_free (base64), base64 = NULL;

  gdk_threads_leave ();

  return ret;
}

static void
gs_edit_server (const gchar * name)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;
  GtkWidget *w1 = NULL;
  GtkNotebook *nb_main = NULL;
  GSSpyProperties *p = gs->current->spy;

  if (gs_window_present (gs->gui->edit_dialog))
    return;

  gs_color_panel_spy_selected (p, TRUE);

  if (p->changed)
  {
    p->changed = FALSE;
    GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);
  }

  g = glade_xml_new (GLADE_FILE, "edit_dialog", NULL);
  w = WGET (g, "edit_dialog");
  nb_main = GTK_NOTEBOOK (WGET (g, "notebook1"));

  if (gs->gui->glade_edit_dialog)
    g_object_unref (gs->gui->glade_edit_dialog);
  gs->gui->glade_edit_dialog = g;
  gs->gui->edit_dialog = w;

  /* web page edition disposition */
  gtk_notebook_remove_page (nb_main, GS_NOTEBOOK_PAGE_MONITORING_WEB_PAGE);

  w1 = WGET (g, "edit_auth_expand");
  gtk_widget_hide (w1);

  g_signal_connect (G_OBJECT (w), "destroy",
                    G_CALLBACK (gs_edit_destroy_cb), NULL);

  w1 = WGET (g, "bt_cancel");
  g_signal_connect_swapped (G_OBJECT (w1), "clicked",
                            G_CALLBACK (gtk_widget_destroy), w);
  w1 = WGET (g, "bt_server_actions");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_actions_clicked_cb), NULL);
  w1 = WGET (g, "bt_validate");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_edit_bt_validate_clicked_cb), "server_page");
  w1 = WGET (g, "bt_view_online");
  gtk_widget_set_sensitive (w1, FALSE);

  /* alerts tree view */
  gs_free_treeview ("alerts");
  gs_build_liststore ("alerts");
  w1 = WGET (g, "alerts_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w1), "alerts");
  gtk_tree_view_set_model (gs->alerts_treeview,
                           GTK_TREE_MODEL (gs->alerts_liststore));
  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW);

  /* changes tree view */
  gs_free_treeview ("changes");
  gs_build_liststore ("changes");
  gs_refresh_changes_tree_content (p);
  w1 = WGET (g, "changes_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w1), "changes");
  gtk_tree_view_set_model (gs->changes_treeview,
                           GTK_TREE_MODEL (gs->changes_liststore));
  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_CHANGES_TREE_VIEW);

  w1 = WGET (g, "bt_clear_results");
  g_signal_connect (G_OBJECT (w1), "clicked",
                    G_CALLBACK (gs_bt_treeview_clear_clicked_cb), NULL);

  /* display combo for scan ports choices */
  gs->gui->scan_ports_compare_combo =
    gs_build_combo_box ("ports", p->scan_ports_compare);
  w1 = WGET (g, "edit_scan_ports_table");
  gtk_table_attach (GTK_TABLE (w1), gs->gui->scan_ports_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  gs_info_panel_update (g, p, "edit");

  /* state */
  w1 = WGET (g, (p->state == GS_STATE_ACTIVATED) ?
             "edit_option_activated" : "edit_option_deactivated");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
  /* schedule */
  g_signal_connect (G_OBJECT (WGET (g, "edit_option_schedule_on_request")),
                    "clicked", G_CALLBACK (gs_schedule_clicked_cb), NULL);
  g_signal_connect (G_OBJECT (WGET (g, "edit_option_auto")), "clicked",
                    G_CALLBACK (gs_schedule_clicked_cb), NULL);
  if (p->schedule)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                  (WGET (g, "edit_option_auto")), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON
                               (WGET (g, "edit_option_schedule_minutes")),
                               (gdouble) p->schedule);
  }
  else
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                  (WGET (g, "edit_option_schedule_on_request")),
                                  TRUE);
    gtk_widget_set_sensitive (WGET (g, "edit_option_schedule_minutes"), FALSE);
  }
  /* target */
  w1 = WGET (g, "edit_target");
  gtk_entry_set_text (GTK_ENTRY (w1), p->target);
  /* ip changed */
  w1 = WGET (g, "edit_alert_ip_changed");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->ip_changed);
  /* server changed */
  w1 = WGET (g, "edit_alert_server_changed");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), p->server_changed);
  /* scan ports */
  if (GS_SPY_HAVE_SCAN_PORTS (p))
  {
    w1 = WGET (g, "edit_alert_scan_ports");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w1), TRUE);
    w1 = WGET (g, "edit_alert_scan_ports_value");
    gtk_entry_set_text (GTK_ENTRY (w1), p->scan_ports);
  }

  /* if we are just displaying results */
  if (strcmp (name, "results") == 0)
    gtk_notebook_set_current_page (nb_main, GS_NOTEBOOK_PAGE_RESULTS);

  gtk_widget_show (gs->gui->scan_ports_compare_combo);

  gtk_widget_show (w);
}

static void
gs_main_popup_run_now_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  gs_process_cb (gs->current->spy);
}

static void
gs_main_popup_edit_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  (gs->current->spy->type == GS_TYPE_WEB_PAGE) ?
    gs_edit_web_page ("edit") : gs_edit_server ("edit");
}

static void
on_main_popup_delete_activate_cb (GtkMenuItem * menuitem, gpointer data)
{
  gs_delete_selected_spy ();
}

static void
gs_main_popup_open (GdkEventButton * event)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;
  GtkWidget *w1 = NULL;
  GSSpyProperties *p = NULL;

  if ((p =
       gs_properties_spy_lookup_by_id (gs_get_selected_row_id
                                       (gs->main_treeview))) == NULL)
    return;

  g = glade_xml_new (GLADE_FILE, "main_popup", NULL);

  w = WGET (g, "main_popup");
  w1 = WGET (g, "event_occured");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (w1), p->changed);
  w1 = WGET (g, "activated");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (w1),
                                  (p->state == GS_STATE_ACTIVATED));

  gtk_menu_popup (GTK_MENU (w), NULL, NULL, NULL, NULL,
                  event->button, event->time);

  w1 = WGET (g, "view_results");
  gtk_widget_set_sensitive (w1, p->changed);

  if (p->type == GS_TYPE_SERVER)
  {
    w1 = WGET (g, "view_online");
    gtk_widget_set_sensitive (w1, FALSE);
  }
  else
    glade_xml_signal_connect_data (g, "on_main_popup_view_online_activate_cb",
                                   G_CALLBACK
                                   (gs_main_popup_view_online_activate_cb), p);

  glade_xml_signal_connect_data (g, "on_main_popup_event_occured_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_event_occured_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_activated_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_activated_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_view_results_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_view_results_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_edit_activate_cb",
                                 G_CALLBACK (gs_main_popup_edit_activate_cb),
                                 p);
  glade_xml_signal_connect_data (g, "on_main_popup_delete_activate_cb",
                                 G_CALLBACK
                                 (on_main_popup_delete_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_run_now_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_run_now_activate_cb), p);

  gtk_widget_show_all (w);

  g_object_unref (g);
}

gboolean
gs_use_proxy (void)
{
  gchar *host = NULL;
  guint port = 0;
  gboolean ret = FALSE;

  gs_get_gnome_proxy_conf (&host, &port);
  ret = (*host != '\0');
  g_free (host), host = NULL;

  return ret;
}

void
gs_launch_editor (const gchar * uri)
{
  gchar *command_line = NULL;
  GnomeVFSMimeApplication *app = NULL;

  app = gnome_vfs_mime_get_default_application ("text/plain");

  command_line = g_strdup_printf ("%s %s", (app) ? app->command : "vi", uri);
  g_spawn_command_line_async (command_line, NULL);
  g_free (command_line), command_line = NULL;

  gnome_vfs_mime_application_free (app), app = NULL;
}


void
gs_launch_browser (const gchar * uri)
{
  gchar *command_line = NULL;
  gchar *browser = NULL;


  browser = gs_get_gnome_browser_conf ();

  command_line = g_strconcat (browser, " ", uri, NULL);
  g_spawn_command_line_async (command_line, NULL);

  g_free (command_line), command_line = NULL;
  g_free (browser), browser = NULL;
}


static void
gs_filter_by_group_changed (GtkComboBox * combo, gpointer data)
{
  g_free (gs->main_tree_filter), gs->main_tree_filter = NULL;
  gs->main_tree_filter = gs_get_combo_groups_selection (GTK_WIDGET (combo));

  /* " " is to said: all groups */
  if (strcmp (gs->main_tree_filter, " ") == 0)
    g_free (gs->main_tree_filter), gs->main_tree_filter = NULL;

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);
}


static void
gs_settings_open (const gchar * type)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;


  if (gs_window_present (gs->gui->settings_dialog))
    return;

  gs->gui->glade_settings_dialog = g =
    glade_xml_new (GLADE_FILE, "settings_dialog", NULL);
  gs->gui->settings_dialog = WGET (g, "settings_dialog");

  /* main tree view */
  gs_build_liststore ("main");
  w = WGET (g, "sources_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w), "main");
  gtk_tree_view_set_model (gs->main_treeview,
                           GTK_TREE_MODEL (gs->main_liststore));
  g_signal_connect (G_OBJECT (gs->main_treeview), "cursor_changed",
                    G_CALLBACK (gs_main_treeview_cursor_changed_cb), NULL);
  g_signal_connect (G_OBJECT (gs->main_treeview), "button_press_event",
                    G_CALLBACK (gs_main_treeview_button_release_event_cb),
                    NULL);
  g_signal_connect (G_OBJECT (gs->main_treeview), "button_release_event",
                    G_CALLBACK (gs_main_treeview_button_release_event_cb),
                    NULL);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);

  /* Groups search filter */
  w = WGET (g, "filter_by_group");
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (gs_filter_by_group_changed), NULL);

  /* groups tree view */
  gs_build_liststore ("groups");
  w = WGET (g, "groups_treeview");
  gs_build_treeview (GTK_TREE_VIEW (w), "groups");
  gtk_tree_view_set_model (gs->groups_treeview,
                           GTK_TREE_MODEL (gs->groups_liststore));
  w = WGET (g, "bt_group_add");
  g_signal_connect_swapped (G_OBJECT (w), "clicked",
                            G_CALLBACK (gs_add_group), NULL);
  w = WGET (g, "bt_group_remove");
  g_signal_connect_swapped (G_OBJECT (w), "clicked",
                            G_CALLBACK (gs_delete_selected_group), NULL);

  GS_SET_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_TREE_VIEW |
                       GS_REFRESH_TYPE_GROUPS_COMBO_VIEW);

  /* background color */
  w = WGET (g, "color_button");
  gtk_color_button_set_color (GTK_COLOR_BUTTON (w), &gs->bg_color);
  g_signal_connect (G_OBJECT (w), "color_set",
                    G_CALLBACK (gs_color_selector_color_set), NULL);

  /* panel display */
  w = WGET (g, "display_main_icon");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                panel_applet_gconf_get_bool (gs->applet,
                                                             "display_main_icon",
                                                             NULL));
  w = WGET (g, "display_spies_icons");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                panel_applet_gconf_get_bool (gs->applet,
                                                             "display_spies_icons",
                                                             NULL));
  w = WGET (g, "display_tooltips");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                panel_applet_gconf_get_bool (gs->applet,
                                                             "display_tooltips",
                                                             NULL));

  if (!gs_get_display_main_icon ())
  {
    w = WGET (g, "display_spies_icons");
    gtk_widget_set_sensitive (w, FALSE);
  }
  else if (!gs_get_display_spies_icons ())
  {
    w = WGET (g, "display_main_icon");
    gtk_widget_set_sensitive (w, FALSE);
  }

  w = WGET (g, "display_main_icon");
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (gs_settings_toggled_cb), NULL);
  w = WGET (g, "display_spies_icons");
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (gs_settings_toggled_cb), NULL);
  w = WGET (g, "display_tooltips");
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (gs_settings_toggled_cb), NULL);
  w = gs->gui->settings_dialog;
  g_signal_connect (G_OBJECT (w), "destroy",
                    G_CALLBACK (gs_settings_destroy_cb), NULL);
  w = WGET (g, "bt_deactivate_all");
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (gs_bt_activate_deactivate_all_clicked_cb),
                    NULL);
  w = WGET (g, "bt_add");
  g_signal_connect_swapped (G_OBJECT (w), "clicked",
                            G_CALLBACK (gs_druid_open), NULL);
  w = WGET (g, "bt_remove");
  g_signal_connect_swapped (G_OBJECT (w), "clicked",
                            G_CALLBACK (gs_delete_selected_spy), NULL);
  w = WGET (g, "bt_deactivate_all");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), gs->deactivated_all);

  if (strcmp (type, "settings") == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (WGET (g, "notebook")), 1);
  else if (strcmp (type, "application") == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (WGET (g, "notebook")), 0);
  else
    g_assert_not_reached ();

  gtk_widget_show (gs->gui->settings_dialog);
}

static void
gs_color_panel_spy_selected (GSSpyProperties * p, const gboolean selected)
{
  GdkColor c;

  gdk_threads_enter ();

  if (selected)
    gdk_color_parse (GS_COLOR_PANEL_SPY_SELECTED, &c);
  else if (p != NULL)
    c = p->bg_color;

  if (p != NULL && p->event_box != NULL)
    gtk_widget_modify_bg (GTK_WIDGET (p->event_box), GTK_STATE_NORMAL, &c);

  gdk_threads_leave ();
}

static void
gs_color_selector_color_set (GtkWidget * colorpicker, gpointer data)
{
  gchar *tmp = NULL;

  gtk_color_button_get_color (GTK_COLOR_BUTTON (colorpicker), &gs->bg_color);

  gtk_widget_modify_bg (GTK_WIDGET (gs->applet), GTK_STATE_NORMAL,
                        &gs->bg_color);
  gtk_widget_modify_bg (GTK_WIDGET (gs->event_box_main), GTK_STATE_NORMAL,
                        &gs->bg_color);

  tmp = g_strdup_printf ("#%04x%04x%04x",
                         gs->bg_color.red,
                         gs->bg_color.green, gs->bg_color.blue);
  gs_set_display_bg_color (tmp);
  g_free (tmp), tmp = NULL;
}

static void
gs_actions_email_toggled_cb (GtkToggleButton * togglebutton, gpointer data)
{
  GladeXML *g = gs->gui->glade_actions_dialog;

#ifdef ENABLE_LIBESMTP
  gtk_widget_set_sensitive (WGET (g, "email_table"), GTB (togglebutton));
#else
  gtk_widget_set_sensitive (WGET (g, "email_table"), FALSE);
#endif
}

static void
gs_settings_toggled_cb (GtkToggleButton * togglebutton, gpointer data)
{
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
  const gboolean val = GTB (togglebutton);

  if (strcmp (name, "display_tooltips") == 0)
    gs_set_display_tooltips (val);
  else
  {
    GladeXML *g = gs->gui->glade_settings_dialog;
    GtkWidget *w_main = NULL;
    GtkWidget *w_spies = NULL;

    w_main = WGET (g, "display_main_icon");
    w_spies = WGET (g, "display_spies_icons");

    g_signal_handlers_block_by_func (w_main, gs_settings_toggled_cb, NULL);
    g_signal_handlers_block_by_func (w_spies, gs_settings_toggled_cb, NULL);

    gtk_widget_set_sensitive (w_main, TRUE);
    gtk_widget_set_sensitive (w_spies, TRUE);

    gs_set_display_main_icon (TRUE);
    gs_set_display_spies_icons (TRUE);

    if (strcmp (name, "display_main_icon") == 0)
    {
      gs_set_display_main_icon (val);

      if (gs->spies_list == NULL || g_list_length (gs->spies_list) == 0)
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w_main), TRUE);
        gs_set_display_main_icon (TRUE);
      }
      else if (!GTB (togglebutton))
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w_spies), TRUE);
        gs_set_display_spies_icons (TRUE);
        gtk_widget_set_sensitive (w_spies, FALSE);
      }
    }
    else if (strcmp (name, "display_spies_icons") == 0)
    {
      gs_set_display_spies_icons (val);

      if (!GTB (togglebutton))
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w_main), TRUE);
        gs_set_display_main_icon (TRUE);
        gtk_widget_set_sensitive (w_main, FALSE);
      }
    }

    g_signal_handlers_unblock_by_func (w_main, gs_settings_toggled_cb, NULL);
    g_signal_handlers_unblock_by_func (w_spies, gs_settings_toggled_cb, NULL);
  }

  gs_gui_update ();
}

static void
gs_gui_update (void)
{
  gchar *tmp = NULL;
  GList *item = NULL;

  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;

    if (!gs_get_display_tooltips ())
      gtk_tooltips_disable (p->tooltip);
    else
      gtk_tooltips_enable (p->tooltip);

    item = g_list_next (item);
  }

  if (gs_get_display_main_icon ())
    gtk_widget_show_all (gs->applet_box_main);
  else if (gs->spies_list != NULL && g_list_length (gs->spies_list) > 0)
    gtk_widget_hide (gs->applet_box_main);

  if (gs->spies_list != NULL && gs_get_display_spies_icons ())
    gtk_widget_show_all (gs->applet_box_spies);
  else
    gtk_widget_hide (gs->applet_box_spies);

  /* Color */
  tmp = panel_applet_gconf_get_string (gs->applet, "bg_color", NULL);
  gdk_color_parse (tmp, &gs->bg_color);
  g_free (tmp), tmp = NULL;
  gtk_widget_modify_bg (GTK_WIDGET (gs->applet), GTK_STATE_NORMAL,
                        &gs->bg_color);
  gtk_widget_modify_bg (GTK_WIDGET (gs->event_box_main), GTK_STATE_NORMAL,
                        &gs->bg_color);
}

static void
gs_menu_properties_cb (BonoboUIComponent * uic, gpointer data,
                       const gchar * verbname)
{
  gs_settings_open ("settings");
}

static void
gs_actions_destroy_cb (GtkObject * object, gpointer data)
{
  gs->gui->glade_actions_dialog = NULL;
  gs->gui->actions_dialog = NULL;
}

static void
gs_settings_destroy_cb (GtkObject * object, gpointer data)
{
  gs_free_treeview ("main");

  if (gs->gui->edit_dialog == NULL && gs->current->spy != NULL)
    gs_color_panel_spy_selected (gs->current->spy, FALSE);

  gs->gui->glade_settings_dialog = NULL;
  gs->gui->settings_dialog = NULL;
}

static void
gs_edit_destroy_cb (GtkObject * object, gpointer data)
{
  GSSpyProperties *p = gs->current->spy;

  if (gs->gui->settings_dialog == NULL)
    gs_color_panel_spy_selected (gs->current->spy, FALSE);

  gs_properties_changes_free (p->changes_list), p->changes_list = NULL;
  gs_properties_alerts_free (p->alerts_list), p->alerts_list = NULL;

  gs->gui->glade_edit_dialog = NULL;
  gs->gui->edit_dialog = NULL;

  gs_free_treeview ("alerts");
  gs_free_treeview ("changes");
}

gboolean
gs_window_present (GtkWidget * w)
{
  gboolean ret = FALSE;

  if (w != NULL)
  {
    gtk_window_set_screen (GTK_WINDOW (w),
                           gtk_widget_get_screen (GTK_WIDGET (gs->applet)));
    gtk_window_present (GTK_WINDOW (w));

    ret = TRUE;
  }

  return ret;
}

static void
gs_menu_about_cb (BonoboUIComponent * uic, gpointer data,
                  const gchar * verbname)
{
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *hbox = NULL;

  static const gchar *authors[] = {
    "Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>",
    NULL
  };
  const char *translators =
    _("Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>, French\n"
      "Angelo Theodorou <encelo@users.sourceforge.net>, Italian\n"
      "Francisco Javier <ffelix@sshinf.com>, Spanish\n");

  if (gs_window_present (gs->gui->about_dialog))
    return;

  pixbuf =
    gdk_pixbuf_new_from_file (GNOME_ICONDIR
                              "/gospy-applet/gospy_applet.png", NULL);
  gs->gui->about_dialog =
    gnome_about_new (PACKAGE, GS_VERSION,
                     "(C) 2004-2009 Emmanuel Saracco",
                     _("A applet monitoring servers and web pages"),
                     authors, NULL, translators, pixbuf);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gs->gui->about_dialog)->vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
                      gnome_href_new
                      ("http://gospy-applet.labs.libre-entreprise.org/",
                       _("gospy-applet project page")), TRUE, FALSE, 0);

  g_object_unref (pixbuf), pixbuf = NULL;

  g_signal_connect (G_OBJECT (gs->gui->about_dialog), "destroy",
                    G_CALLBACK (gtk_widget_destroyed), &gs->gui->about_dialog);

  gtk_widget_show_all (gs->gui->about_dialog);
}

static void
gs_menu_help_cb (BonoboUIComponent * uic, gpointer data, const gchar * verbname)
{
  GError *error = NULL;

  gnome_help_display (PACKAGE, NULL, &error);
}

void
gs_free_treeview (const gchar * name)
{
  G_LOCK (gs_main_lock);

  /* main tree */
  if ((strcmp (name, "main") == 0) && gs->main_liststore != NULL)
  {
    g_object_unref (G_OBJECT (gs->main_liststore));
    gs->main_liststore = NULL;
    gs->main_treeview = NULL;
  }
  /* Groups tree */
  else if ((strcmp (name, "groups") == 0) && gs->groups_liststore != NULL)
  {
    g_object_unref (G_OBJECT (gs->groups_liststore));
    gs->groups_liststore = NULL;
    gs->groups_treeview = NULL;
  }
  /* alerts tree */
  else if ((strcmp (name, "alerts") == 0) && gs->alerts_liststore != NULL)
  {
    g_object_unref (G_OBJECT (gs->alerts_liststore));
    gs->alerts_liststore = NULL;
    gs->alerts_treeview = NULL;
  }
  /* changes tree */
  else if ((strcmp (name, "changes") == 0) && gs->changes_liststore != NULL)
  {
    g_object_unref (G_OBJECT (gs->changes_liststore));
    gs->changes_liststore = NULL;
    gs->changes_treeview = NULL;
  }

  G_UNLOCK (gs_main_lock);
}

static GSCurrent *
gs_current_new (void)
{
  GSCurrent *current = NULL;

  current = g_new0 (GSCurrent, 1);
  current->spy = NULL;
  current->change = NULL;

  return current;
}

static GSGui *
gs_gui_new (void)
{
  GSGui *gui = NULL;

  gui = g_new0 (GSGui, 1);
  gui->about_dialog = NULL;
  gui->glade_settings_dialog = NULL;
  gui->settings_dialog = NULL;
  gui->glade_edit_dialog = NULL;
  gui->edit_dialog = NULL;
  gui->glade_actions_dialog = NULL;
  gui->actions_dialog = NULL;
  gui->status_compare_combo = NULL;
  gui->size_compare_combo = NULL;
  gui->search_content_combo = NULL;
  gui->load_compare_combo = NULL;
  gui->search_content_combo = NULL;
  gui->scan_ports_compare_combo = NULL;

  return gui;
}

static void
gs_free (void)
{
  /* Free gs common data */
  gs_free_common ();

  /* gnutls */
  gs_process_conn_deinit ();

  /* gnomevfs */
  gnome_vfs_shutdown ();
}

static void
gs_free_common (void)
{
  if (gs == NULL)
    return;

  if (gs->timeout_refresh)
    g_source_remove (gs->timeout_refresh), gs->timeout_refresh = 0;

  gs_properties_spies_save ();
  gs_properties_groups_save ();

  /* do not delete spies files */
  gs_properties_free (FALSE);

  gs_free_treeview ("main");
  gs_free_treeview ("alerts");
  gs_free_treeview ("changes");
  gs_free_treeview ("groups");

  if (gs->applet_image)
    gtk_widget_destroy (gs->applet_image), gs->applet_image = NULL;
  if (gs->applet_box_main)
    gtk_widget_destroy (gs->applet_box_main), gs->applet_box_main = NULL;
  if (gs->applet_box_spies)
    gtk_widget_destroy (gs->applet_box_spies), gs->applet_box_spies = NULL;
  if (gs->applet_box)
    gtk_widget_destroy (gs->applet_box), gs->applet_box = NULL;

  g_free (gs->home_path), gs->home_path = NULL;
  g_free (gs->main_tree_filter), gs->main_tree_filter = NULL;

  g_free (gs->current), gs->current = NULL;
  g_free (gs->gui), gs->gui = NULL;;
  g_free (gs), gs = NULL;
}

static GS *
gs_new (PanelApplet * applet)
{
  gchar *path = NULL;

  gs = g_new0 (GS, 1);
  gs->current = gs_current_new ();
  gs->gui = gs_gui_new ();
  gs->applet = applet;
  gs->refresh_flag = GS_REFRESH_TYPE_NONE;
  gs->block_tooltips = FALSE;
  gs->display_tooltips = TRUE;
  gs->display_main_icon = TRUE;
  gs->display_spies_icons = TRUE;
  gs->deactivated_all = FALSE;
  gs->deactivated_items = 0;
  gs->applet_image = NULL;
  gs->applet_box = NULL;
  gs->event_box_main = NULL;
  gs->applet_box_spies = NULL;
  gs->applet_box_main = NULL;
  gs->main_treeview = NULL;
  gs->main_liststore = NULL;
  gs->groups_treeview = NULL;
  gs->groups_liststore = NULL;
  gs->alerts_treeview = NULL;
  gs->alerts_liststore = NULL;
  gs->changes_treeview = NULL;
  gs->changes_liststore = NULL;
  gs->home_path = g_strconcat (g_getenv ("HOME"), "/.gospy-applet", NULL);
  gs->main_tree_filter = NULL;
  gs->timeout_refresh = g_idle_add (gs_refresh_cb, NULL);

  mkdir (gs->home_path, S_IRWXU);
  path = g_strconcat (gs->home_path, "/logs", NULL);
  mkdir (path, S_IRWXU);
  g_free (path), path = NULL;

  /* build applet container and add main image */

  /* horizontal orientation */
  if (IS_PANEL_HORIZONTAL (panel_applet_get_orient (gs->applet)))
  {
    gs->applet_box = gtk_hbox_new (FALSE, 4);
    gs->applet_box_main = gtk_hbox_new (FALSE, 0);
    gs->applet_box_spies = gtk_hbox_new (FALSE, 1);
  }
  /* vertical orientation */
  else
  {
    gs->applet_box = gtk_vbox_new (FALSE, 4);
    gs->applet_box_main = gtk_vbox_new (FALSE, 0);
    gs->applet_box_spies = gtk_vbox_new (FALSE, 1);
  }

  gs->event_box_main = gtk_event_box_new ();
  gtk_event_box_set_above_child (GTK_EVENT_BOX (gs->event_box_main), TRUE);
  g_signal_connect (G_OBJECT (gs->event_box_main),
                    "button-press-event", G_CALLBACK (gs_applet_clicked_cb),
                    NULL);
  g_signal_connect (G_OBJECT (gs->event_box_main),
                    "button-release-event",
                    G_CALLBACK (gs_applet_clicked_cb), NULL);

  gs->applet_image =
    gtk_image_new_from_file (GNOME_ICONDIR "/gospy-applet/gospy_applet.png");
  gtk_container_add (GTK_CONTAINER (gs->event_box_main), gs->applet_image);
  gtk_container_add (GTK_CONTAINER (gs->applet_box_main), gs->event_box_main);
  gtk_container_add (GTK_CONTAINER (gs->applet), gs->applet_box);
  gtk_container_add (GTK_CONTAINER (gs->applet_box), gs->applet_box_spies);
  gtk_container_add (GTK_CONTAINER (gs->applet_box), gs->applet_box_main);

  gs->spies_list = gs_properties_spies_load ();

  gs_set_display_tooltips (panel_applet_gconf_get_bool
                           (gs->applet, "display_tooltips", NULL));
  gs_set_display_main_icon (panel_applet_gconf_get_bool
                            (gs->applet, "display_main_icon", NULL));
  gs_set_display_spies_icons (panel_applet_gconf_get_bool
                              (gs->applet, "display_spies_icons", NULL));

  return gs;
}

GtkWidget *
gs_build_combo_box (const gchar * name, const guint choice)
{
  enum
  {
    UID_COLUMN,
    LABEL_COLUMN,
    N_COLUMNS
  };
  GtkWidget *w = NULL;
  GtkListStore *liststore;
  GtkTreeIter iter;
  GtkCellRenderer *trenderer = NULL;
  gchar **items = NULL;
  guint i = 0;

  liststore = gtk_list_store_new (N_COLUMNS, G_TYPE_INT, G_TYPE_STRING);

  w = gtk_combo_box_new_with_model (GTK_TREE_MODEL (liststore));

  trenderer = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), trenderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w),
                                  trenderer, "text", UID_COLUMN, NULL);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w),
                                  trenderer, "text", LABEL_COLUMN, NULL);


  if (strcmp (name, "compare") == 0)
    items = g_strsplit (GS_COMPARE_CHOICE_ITEMS, ",", -1);
  else if (strcmp (name, "ports") == 0)
    items = g_strsplit (GS_SCAN_PORTS_CHOICE_ITEMS, ",", -1);
  else if (strcmp (name, "search_content_in") == 0)
    items = g_strsplit (GS_HTML_TAGS_CHOICE_ITEMS, ",", -1);

  while (items[i])
  {
    gtk_list_store_append (liststore, &iter);
    gtk_list_store_set (liststore, &iter,
                        UID_COLUMN, i, LABEL_COLUMN, items[i], -1);
    i++;
  }

  g_strfreev (items), items = NULL;

  gtk_combo_box_set_active (GTK_COMBO_BOX (w), choice);

  //g_object_unref (liststore), liststore = NULL;

  return w;
}

gboolean
gs_compare_match (const GSCompareChoice choice,
                  const guint ref, const guint new)
{
  gboolean ret = FALSE;

  switch (choice)
  {
  case GS_COMPARE_CHOICE_EQUAL:
    ret = (ref == new);
    break;
  case GS_COMPARE_CHOICE_DIFFERENT:
    ret = (ref != new);
    break;
  case GS_COMPARE_CHOICE_INFERIOR:
    ret = (new < ref);
    break;
  case GS_COMPARE_CHOICE_SUPERIOR:
    ret = (new > ref);
  }

  return ret;
}

void
gs_remove_spy_image (GSSpyProperties * p)
{
  if (p->image)
    gtk_widget_destroy (p->image), p->image = NULL;
  if (p->event_box)
    gtk_widget_destroy (p->event_box), p->event_box = NULL;
  p->tooltip = NULL;
}

static gchar *
gs_get_spy_tooltip (const GSSpyProperties * p)
{
  gchar *schedule = NULL;
  gchar *ret = NULL;

  schedule = (!p->schedule) ?
    g_strdup (_("Manual")) : g_strdup_printf (_("Every %u minute(s)"),
                                              p->schedule);

  ret = g_strdup_printf (_("Title : %s\n"
                           "Target : %s\n"
                           "Scheduling : %s\n"
                           "\n"
                           "Event : [%s]\n"
                           "Process : [%s]\n"
                           "State : %s"),
                         p->title,
                         (p->type == GS_TYPE_SERVER) ?
                         _("Server") : _("Web page"),
                         schedule,
                         (p->changed) ? _("Yes") : _("No"),
                         (p->task == GS_TASK_RUNNING) ?
                         _("Running") : _("Sleeping"),
                         (p->state == GS_STATE_ACTIVATED) ?
                         _("Activated") : _("Deactivated"));

  g_free (schedule), schedule = NULL;

  return ret;
}

static void
gs_panel_spy_popup_open (GdkEventButton * event, GSSpyProperties * p)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;
  GtkWidget *w1 = NULL;
  gchar *tmp = NULL;

  gs->current->spy = p;

  g = glade_xml_new (GLADE_FILE, "panel_spy_popup", NULL);

  w = WGET (g, "panel_spy_popup");
  gtk_menu_popup (GTK_MENU (w), NULL, NULL, NULL, NULL,
                  event->button, event->time);
  g_signal_connect (G_OBJECT (w), "hide",
                    G_CALLBACK (gs_panel_spy_popup_destroy_cb), NULL);

  /* add item to the contextual menu */
  gtk_menu_shell_prepend (GTK_MENU_SHELL (w), gtk_separator_menu_item_new ());
  tmp = g_strconcat ("[", p->title, "]", NULL);
  w1 = gtk_image_menu_item_new_with_label (tmp);
  g_free (tmp), tmp = NULL;
  tmp = g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                     (p->type == GS_TYPE_SERVER) ?
                     "type_server.png" : "type_web_page.png", NULL);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (w1),
                                 gtk_image_new_from_file (tmp));
  g_free (tmp), tmp = NULL;
  gtk_widget_set_sensitive (w1, FALSE);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (w), w1);

  w1 = WGET (g, "panel_view_results");
  gtk_widget_set_sensitive (w1, p->changed);

  if (p->type == GS_TYPE_SERVER)
  {
    w1 = WGET (g, "panel_view_online");
    gtk_widget_set_sensitive (w1, FALSE);
  }
  else
    glade_xml_signal_connect_data (g, "on_main_popup_view_online_activate_cb",
                                   G_CALLBACK
                                   (gs_main_popup_view_online_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_view_results_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_view_results_activate_cb), p);
  glade_xml_signal_connect_data (g, "on_main_popup_edit_activate_cb",
                                 G_CALLBACK (gs_main_popup_edit_activate_cb),
                                 p);
  glade_xml_signal_connect_data (g, "on_main_popup_run_now_activate_cb",
                                 G_CALLBACK
                                 (gs_main_popup_run_now_activate_cb), p);

  gtk_widget_show_all (w);
}

static void
gs_panel_spy_popup_destroy_cb (GtkObject * object, gpointer data)
{
  if (gs->current->spy == NULL)
    return;

  gs_color_panel_spy_selected (gs->current->spy, FALSE);
  gs->block_tooltips = FALSE;
}

gboolean
gs_panel_applet_block_events (void)
{
  return (gs->gui->settings_dialog != NULL || gs->gui->edit_dialog != NULL);
}

static gboolean
gs_panel_applet_clicked_event_cb (GtkWidget * widget,
                                  GdkEventButton * event, gpointer data)
{
  return (gs_panel_applet_block_events ());
}

static gboolean
gs_panel_spy_button_clicked_event_cb (GtkWidget * widget,
                                      GdkEventButton * event, gpointer data)
{
  GSSpyProperties *p = NULL;
  gboolean ret = FALSE;

  if (data == NULL)
    return FALSE;

  p = (GSSpyProperties *) data;

  if (gs->gui->edit_dialog == NULL && gs->gui->settings_dialog != NULL &&
      gs->main_liststore != NULL && gs->main_treeview != NULL)
  {
    gchar *tpc = gs_build_spy_treeview_path (p->timestamp);
    GtkTreePath *tp = gtk_tree_path_new_from_string (tpc);

    gtk_tree_view_set_cursor (GTK_TREE_VIEW (gs->main_treeview), tp, NULL,
                              FALSE);

    gtk_tree_path_free (tp), tp = NULL;
    g_free (tpc), tpc = NULL;
  }
  else if (event->button == 1 && gs->gui->edit_dialog == NULL)
    gs_color_panel_spy_selected (p, TRUE);

  if (!gs_panel_applet_block_events ())
  {
    if (event->button == 1 && !gs->block_tooltips)
    {
      gs->block_tooltips = TRUE;
      gs_panel_spy_popup_open (event, p);

      ret = TRUE;
    }
  }
  else
    ret = TRUE;

  return ret;
}

void
gs_add_spy_image (GSSpyProperties * p)
{
  gchar *icon_path = NULL;
  gchar *str = NULL;
  GtkStyle *style = NULL;

  p->event_box = gtk_event_box_new ();
  gtk_event_box_set_above_child (GTK_EVENT_BOX (p->event_box), TRUE);
  g_signal_connect (G_OBJECT (p->event_box), "button_press_event",
                    G_CALLBACK (gs_panel_spy_button_clicked_event_cb), p);


  p->tooltip = gtk_tooltips_new ();
  str = gs_get_spy_tooltip (p);
  gtk_tooltips_set_tip (p->tooltip, p->event_box, str, NULL);
  g_free (str), str = NULL;
  (gs->block_tooltips || !gs_get_display_tooltips ())?
    gtk_tooltips_disable (p->tooltip) : gtk_tooltips_enable (p->tooltip);

  icon_path = g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                           (p->type == GS_TYPE_SERVER) ?
                           "type_server.png" : "type_web_page.png", NULL);
  p->image = gtk_image_new_from_file (icon_path);
  gtk_container_add (GTK_CONTAINER (p->event_box), GTK_WIDGET (p->image));
  gtk_container_add (GTK_CONTAINER (gs->applet_box_spies),
                     GTK_WIDGET (p->event_box));
  g_free (icon_path), icon_path = NULL;

  style = gtk_widget_get_style (p->event_box);
  p->bg_color = style->bg[GTK_STATE_NORMAL];

  if (gs_get_display_spies_icons ())
    gtk_widget_show_all (GTK_WIDGET (gs->applet_box_spies));
}

static gboolean
gs_applet_clicked_cb (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  gboolean ret = FALSE;

  if (!gs_panel_applet_block_events ())
  {
    if (event->type == GDK_2BUTTON_PRESS)
    {
      gs_settings_open ("application");
      ret = TRUE;
    }
  }
  else
    ret = TRUE;

  return ret;
}

static gboolean
gs_refresh_cb (gpointer data)
{
  G_LOCK_DEFINE_STATIC (gs_refresh_lock);
  gboolean changed = FALSE;
  gboolean running = FALSE;
  GList *item = NULL;

  GS_UPDATE_UI;

  g_usleep (15000);

  if (gs->refresh_flag == GS_REFRESH_TYPE_NONE)
    return TRUE;


  G_LOCK (gs_refresh_lock);

  /* Save spies data on disk */
  if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_SAVE_DATA))
  {
    gs_properties_spies_save ();
    gs_properties_groups_save ();
    GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_SAVE_DATA);
  }

  /* Refresh main tree */
  if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW))
  {
    gs_refresh_main_tree_content ();
    GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_MAIN_TREE_VIEW);
  }

  /* Refresh groups tree */
  if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_TREE_VIEW))
  {
    gs_refresh_groups_tree_content ();
    GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_TREE_VIEW);
  }

  /* Refresh groups filter combo */
  if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_COMBO_VIEW))
  {
    gs_refresh_groups_combo_content ();
    GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_GROUPS_COMBO_VIEW);
  }

  /* If edition dialog is opened */
  if (gs->current->spy != NULL)
  {
    /* Refresh alerts tree */
    if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW))
    {
      gs_refresh_alerts_tree_content (gs->current->spy);
      GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_ALERTS_TREE_VIEW);
    }
    /* Refresh changes tree */
    if (GS_IS_REFRESH_FLAG (GS_REFRESH_TYPE_CHANGES_TREE_VIEW))
    {
      gs_refresh_changes_tree_content (gs->current->spy);
      GS_UNSET_REFRESH_FLAG (GS_REFRESH_TYPE_CHANGES_TREE_VIEW);
    }
  }

  /* refresh main applet icon */
  item = g_list_first (gs->spies_list);
  while (item != NULL)
  {
    GSSpyProperties *p = (GSSpyProperties *) item->data;

    if (p->changed)
      changed = TRUE;
    if (p->task == GS_TASK_RUNNING)
      running = TRUE;

    gs_change_spy_image (p);

    item = g_list_next (item);
  }

  if (changed)
    gs_change_applet_image ((running) ?
                            "gospy_applet_changed_run.png" :
                            "gospy_applet_changed.png");
  else if (running)
    gs_change_applet_image ((changed) ?
                            "gospy_applet_changed_run.png" :
                            "gospy_applet_run.png");
  else
    gs_change_applet_image ("gospy_applet.png");

  GS_UPDATE_UI;

  G_UNLOCK (gs_refresh_lock);

  return TRUE;
}

static void
gs_change_spy_image (const GSSpyProperties * p)
{
  gchar *icon_path = NULL;
  gchar *str = NULL;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  str = gs_get_spy_tooltip (p);
  gtk_tooltips_set_tip (p->tooltip, p->event_box, str, NULL);
  g_free (str), str = NULL;

  (gs->block_tooltips || !gs_get_display_tooltips ())?
    gtk_tooltips_disable (p->tooltip) : gtk_tooltips_enable (p->tooltip);

  if (p->task == GS_TASK_RUNNING)
    icon_path = (p->changed) ?
      g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                   (p->type == GS_TYPE_SERVER) ?
                   "type_server_changed_run.png" :
                   "type_web_page_changed_run.png", NULL) :
      g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                   (p->type == GS_TYPE_SERVER) ?
                   "type_server_run.png" : "type_web_page_run.png", NULL);
  else
    icon_path = (p->changed) ?
      g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                   (p->type == GS_TYPE_SERVER) ?
                   "type_server_changed.png" :
                   "type_web_page_changed.png", NULL) :
      g_strconcat (GNOME_ICONDIR, "/gospy-applet/",
                   (p->type == GS_TYPE_SERVER) ?
                   (p->state == GS_STATE_DEACTIVATED) ?
                   "type_server_deactivated.png" :
                   "type_server.png" :
                   (p->state == GS_STATE_DEACTIVATED) ?
                   "type_web_page_deactivated.png" : "type_web_page.png", NULL);

  gtk_image_set_from_file (GTK_IMAGE (p->image), icon_path);
  g_free (icon_path), icon_path = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

static void
gs_change_applet_image (const gchar * icon_name)
{
  gchar *icon_path = NULL;

  gdk_threads_enter ();
  G_LOCK (gs_main_lock);

  icon_path = g_strconcat (GNOME_ICONDIR, "/gospy-applet/", icon_name, NULL);

  gtk_image_set_from_file (GTK_IMAGE (gs->applet_image), icon_path);

  g_free (icon_path), icon_path = NULL;

  G_UNLOCK (gs_main_lock);
  gdk_threads_leave ();
}

static gboolean
gs_applet_fill (PanelApplet * applet)
{
  /* LIBXML initialization */
  LIBXML_TEST_VERSION;
  xmlIndentTreeOutput = 1;
  xmlKeepBlanksDefault (0);

  /* GNET initialization */
  gnet_init ();

  /* GNOMEVFS initialization */
  gnome_vfs_init ();

  g_type_init ();

  /* GNUTLS initialization */
  gs_process_conn_init ();

  /* GCONF initialization */
  panel_applet_add_preferences (applet, GS_GCONF_PATH, NULL);

  gs = gs_new (applet);

  panel_applet_setup_menu_from_file (gs->applet,
                                     NULL,
                                     "GNOME_GospyApplet.xml",
                                     NULL, gs_menu_verbs, NULL);

  g_signal_connect (gs->applet,
                    "change_orient",
                    G_CALLBACK (gs_applet_change_orient_cb), NULL);
  g_signal_connect (G_OBJECT (gs->applet),
                    "button-press-event",
                    G_CALLBACK (gs_panel_applet_clicked_event_cb), NULL);
  g_signal_connect_swapped (G_OBJECT (gs->applet), "destroy",
                            G_CALLBACK (gs_free), NULL);

  gtk_widget_show_all (GTK_WIDGET (gs->applet));

  gs_gui_update ();

  return TRUE;
}

static gboolean
gs_factory (PanelApplet * applet, const char *iid, gpointer data)
{
  gboolean retval = FALSE;

  if (strcmp (iid, "OAFIID:GNOME_GospyApplet") == 0)
    retval = gs_applet_fill (applet);

  return retval;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:GNOME_GospyApplet_Factory",
                             PANEL_TYPE_APPLET,
                             PACKAGE, GS_VERSION, gs_factory, NULL)
