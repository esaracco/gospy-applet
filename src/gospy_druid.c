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

#include "gospy_druid.h"
#include "gospy_properties.h"

typedef struct _GSDruid GSDruid;
struct _GSDruid
{
  GladeXML *glade;
  GtkWidget *dialog;
  GtkWidget *druid;
  /* page "welcome" */
  GtkWidget *page_start;
  /* page "choose spy type" */
  GtkWidget *page_type;
  GtkWidget *option_web_page_type;
  GtkWidget *option_server_type;
  GtkWidget *option_title;
  GtkWidget *option_group;
  /* page "web page type" */
  GtkWidget *page_type_web_page;
  GtkWidget *option_location;
  GtkWidget *auth_user;
  GtkWidget *auth_password;
  GtkWidget *alert_size;
  GtkWidget *alert_size_value;
  GtkWidget *alert_last_modified;
  GtkWidget *alert_size_changed;
  GtkWidget *alert_content_changed;
  GtkWidget *alert_status;
  GtkWidget *alert_status_compare;
  GtkWidget *alert_size_compare_combo;
  GtkWidget *alert_search_content_combo;
  GtkWidget *alert_load_compare_combo;
  GtkWidget *alert_scan_ports_compare_combo;
  GtkWidget *alert_status_changed;
  GtkWidget *alert_status_value;
  GtkWidget *alert_load_time;
  GtkWidget *alert_load_time_value;
  GtkWidget *alert_search_content;
  GtkWidget *alert_search_content_value;
  GtkWidget *alert_search_content_icase;
  GtkWidget *alert_scan_ports;
  GtkWidget *alert_scan_ports_value;
  /* page "server type" */
  GtkWidget *page_type_server;
  GtkWidget *option_server_name;
  GtkWidget *alert_ip_changed;
  GtkWidget *alert_server_changed;
  /* page "scheduling" */
  GtkWidget *page_scheduler;
  GtkWidget *option_schedule_minutes;
  /* page "terminated" */
  GtkWidget *page_finish;
};

static GSDruid *gs_druid;

static void gs_druid_schedule_clicked_cb (GtkWidget * widget, gpointer data);
static gboolean
gs_druid_page_back_cb (GnomeDruidPage * page, GnomeDruid * druid,
                       gpointer data);
static gboolean gs_druid_page_next_cb (GnomeDruidPage * page,
                                       GnomeDruid * druid, gpointer data);
static void gs_druid_destroy_cb (GtkObject * object, gpointer user_data);
static void gs_druid_text_changed_cb (GtkEditable * editable,
                                      gpointer user_data);
static void gs_druid_page_finish_prepare_cb (GnomeDruidPage * page,
                                             GnomeDruid * druid,
                                             gpointer user_data);
static void gs_druid_page_finish_finish_cb (GnomeDruidPage * page,
                                            GnomeDruid * druid,
                                            gpointer user_data);
void
gs_druid_new (void)
{
  gs_druid = g_new0 (GSDruid, 1);
  gs_druid->glade = NULL;
  gs_druid->dialog = NULL;
  gs_druid->druid = NULL;
  gs_druid->page_start = NULL;
  gs_druid->page_type = NULL;
  gs_druid->option_web_page_type = NULL;
  gs_druid->option_server_type = NULL;
  gs_druid->option_title = NULL;
  gs_druid->option_group = NULL;
  gs_druid->page_type_web_page = NULL;
  gs_druid->option_location = NULL;
  gs_druid->auth_user = NULL;
  gs_druid->auth_password = NULL;
  gs_druid->alert_size = NULL;
  gs_druid->alert_size_value = NULL;
  gs_druid->alert_last_modified = NULL;
  gs_druid->alert_size_changed = NULL;
  gs_druid->alert_content_changed = NULL;
  gs_druid->alert_status = NULL;
  gs_druid->alert_status_changed = NULL;
  gs_druid->alert_status_value = NULL;
  gs_druid->alert_status_compare = NULL;
  gs_druid->alert_size_compare_combo = NULL;
  gs_druid->alert_search_content_combo = NULL;
  gs_druid->alert_load_compare_combo = NULL;
  gs_druid->alert_scan_ports_compare_combo = NULL;
  gs_druid->alert_load_time = NULL;
  gs_druid->alert_load_time_value = NULL;
  gs_druid->alert_search_content = NULL;
  gs_druid->alert_search_content_value = NULL;
  gs_druid->alert_search_content_icase = NULL;
  gs_druid->alert_scan_ports = NULL;
  gs_druid->alert_scan_ports_value = NULL;
  gs_druid->page_type_server = NULL;
  gs_druid->option_server_name = NULL;
  gs_druid->alert_ip_changed = NULL;
  gs_druid->alert_server_changed = NULL;
  gs_druid->page_scheduler = NULL;
  gs_druid->option_schedule_minutes = NULL;
  gs_druid->page_finish = NULL;
}

void
gs_druid_free ()
{
  g_free (gs_druid), gs_druid = NULL;
}


void
gs_druid_open (void)
{
  GladeXML *g = NULL;
  GtkWidget *w = NULL;


  if (gs_druid != NULL && gs_window_present (gs_druid->dialog))
    return;

  gs_druid_new ();

  if (gs_druid->glade)
    g_object_unref (gs_druid->glade);
  gs_druid->glade = g = glade_xml_new (GLADE_FILE, "druid_dialog", NULL);
  gs_druid->dialog = WGET (g, "druid_dialog");
  g_signal_connect (G_OBJECT (gs_druid->dialog),
                    "destroy", G_CALLBACK (gs_druid_destroy_cb), NULL);

  gs_druid->druid = WGET (g, "druid");
  g_signal_connect_swapped (G_OBJECT (gs_druid->druid),
                            "cancel", G_CALLBACK (gtk_widget_destroy),
                            gs_druid->dialog);

  /* page "start" */
  gs_druid->page_start = WGET (g, "page_start");

  /* page "choose spy type" */
  gs_druid->page_type = WGET (g, "page_type");
  gs_druid->option_web_page_type = WGET (g, "option_web_page_type");
  gs_druid->option_server_type = WGET (g, "option_server_type");
  gs_druid->option_title = WGET (g, "option_title");

  /* Groups */
  gs_druid->option_group = WGET (g, "option_group");
  gs_build_combo_groups (gs_druid->option_group, " ");

  g_signal_connect (G_OBJECT (gs_druid->page_type), "next",
                    G_CALLBACK (gs_druid_page_next_cb), NULL);

  /* display the "web page type" page */
  gs_druid->page_type_web_page = WGET (g, "page_type_web_page");
  gs_druid->option_location = WGET (g, "option_location");
  gs_druid->auth_user = WGET (g, "auth_user");
  gs_druid->auth_password = WGET (g, "auth_password");
  gs_druid->alert_size = WGET (g, "alert_size");
  gs_druid->alert_size_value = WGET (g, "alert_size_value");
  gs_druid->alert_last_modified = WGET (g, "alert_last_modified");
  gs_druid->alert_size_changed = WGET (g, "alert_size_changed");
  gs_druid->alert_content_changed = WGET (g, "alert_content_changed");
  gs_druid->alert_status_changed = WGET (g, "alert_status_changed");
  gs_druid->alert_status = WGET (g, "alert_status");
  gs_druid->alert_status_value = WGET (g, "alert_status_value");

  /* display combo for status choices */
  gs_druid->alert_status_compare = gs_build_combo_box ("compare", 0);
  w = WGET (g, "status_table");
  gtk_table_attach (GTK_TABLE (w), gs_druid->alert_status_compare,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for size choices */
  gs_druid->alert_size_compare_combo = gs_build_combo_box ("compare", 0);
  w = WGET (g, "size_table");
  gtk_table_attach (GTK_TABLE (w), gs_druid->alert_size_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for load choices */
  gs_druid->alert_load_compare_combo = gs_build_combo_box ("compare", 0);
  w = WGET (g, "load_table");
  gtk_table_attach (GTK_TABLE (w), gs_druid->alert_load_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  /* display combo for "Search content in" */
  gs_druid->alert_search_content_combo =
    gs_build_combo_box ("search_content_in", 0);
  w = WGET (g, "search_content_table");
  gtk_table_attach (GTK_TABLE (w), gs_druid->alert_search_content_combo,
                    0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);

  gs_druid->alert_load_time = WGET (g, "alert_load_time");
  gs_druid->alert_load_time_value = WGET (g, "alert_load_time_value");
  gs_druid->alert_search_content = WGET (g, "alert_search_content");
  gs_druid->alert_search_content_value = WGET (g, "alert_search_content_value");
  gs_druid->alert_search_content_icase = WGET (g, "alert_search_content_icase");

  g_signal_connect (G_OBJECT (gs_druid->page_type_web_page), "next",
                    G_CALLBACK (gs_druid_page_next_cb), NULL);
  g_signal_connect (G_OBJECT (gs_druid->page_type_web_page), "back",
                    G_CALLBACK (gs_druid_page_back_cb), NULL);

  /* display the "server type" page */
  gs_druid->page_type_server = WGET (g, "page_type_server");
  gs_druid->option_server_name = WGET (g, "option_server_name");
  gs_druid->alert_ip_changed = WGET (g, "alert_ip_changed");
  gs_druid->alert_server_changed = WGET (g, "alert_server_changed");
  gs_druid->alert_scan_ports = WGET (g, "alert_scan_ports");
  gs_druid->alert_scan_ports_value = WGET (g, "alert_scan_ports_value");

  /* display combo for scan ports choices */
  gs_druid->alert_scan_ports_compare_combo = gs_build_combo_box ("ports", 0);
  w = WGET (g, "scan_ports_table");
  gtk_table_attach (GTK_TABLE (w), gs_druid->alert_scan_ports_compare_combo,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (gs_druid->page_type_server), "next",
                    G_CALLBACK (gs_druid_page_next_cb), NULL);
  g_signal_connect (G_OBJECT (gs_druid->page_type_server), "back",
                    G_CALLBACK (gs_druid_page_back_cb), NULL);
  g_signal_connect (G_OBJECT (gs_druid->option_server_name),
                    "changed", G_CALLBACK (gs_druid_text_changed_cb), NULL);

  /* page "sheduler" */
  gs_druid->page_scheduler = WGET (g, "page_scheduler");
  gs_druid->option_schedule_minutes = WGET (g, "option_schedule_minutes");

  g_signal_connect (G_OBJECT (WGET (g, "option_schedule_on_request")),
                    "clicked", G_CALLBACK (gs_druid_schedule_clicked_cb), NULL);
  g_signal_connect (G_OBJECT (WGET (g, "option_auto")), "clicked",
                    G_CALLBACK (gs_druid_schedule_clicked_cb), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WGET (g, "option_auto")),
                                TRUE);

  g_signal_connect (G_OBJECT (gs_druid->page_scheduler), "back",
                    G_CALLBACK (gs_druid_page_back_cb), NULL);

  /* page "terminated" */
  gs_druid->page_finish = WGET (g, "page_finish");
  g_signal_connect (G_OBJECT (gs_druid->page_finish), "prepare",
                    G_CALLBACK (gs_druid_page_finish_prepare_cb), NULL);
  g_signal_connect (G_OBJECT (gs_druid->page_finish),
                    "finish", G_CALLBACK (gs_druid_page_finish_finish_cb),
                    NULL);

  gtk_widget_show_all (gs_druid->dialog);
}

static void
gs_druid_page_finish_finish_cb (GnomeDruidPage * page,
                                GnomeDruid * druid, gpointer user_data)
{
  gboolean use_auth = FALSE;
  guint size = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON (gs_druid->alert_size_value));
  guint status = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON (gs_druid->alert_status_value));
  guint load_time = gtk_spin_button_get_value_as_int
    (GTK_SPIN_BUTTON (gs_druid->alert_load_time_value));
  const gchar *search_content =
    gtk_entry_get_text (GTK_ENTRY (gs_druid->alert_search_content_value));
  const gchar *scan_ports =
    gtk_entry_get_text (GTK_ENTRY (gs_druid->alert_scan_ports_value));
  guint search_content_icase = GTB (gs_druid->alert_search_content_icase);

  if (!GTB (gs_druid->alert_size))
    size = 0;
  if (!GTB (gs_druid->alert_status))
    status = 0;
  if (!GTB (gs_druid->alert_load_time))
    load_time = 0;
  if (!GTB (gs_druid->alert_search_content))
    search_content = "";
  if (!GTB (gs_druid->alert_scan_ports))
    scan_ports = "";

  use_auth = (*(gtk_entry_get_text (GTK_ENTRY (gs_druid->auth_user))));

  gs_properties_spies_append (GTB (gs_druid->option_server_type),
                              gtk_entry_get_text (GTK_ENTRY
                                                  (gs_druid->option_title)),
                              gs_get_combo_groups_selection
                              (gs_druid->option_group),
                              gtk_entry_get_text (GTK_ENTRY
                                                  (gs_druid->option_server_name)),
                              GTB (gs_druid->alert_ip_changed),
                              GTB (gs_druid->alert_server_changed),
                              GTB (gs_druid->option_web_page_type),
                              gtk_entry_get_text (GTK_ENTRY
                                                  (gs_druid->option_location)),
                              GTB (gs_druid->alert_last_modified),
                              GTB (gs_druid->alert_size_changed),
                              GTB (gs_druid->alert_content_changed),
                              GTB (gs_druid->alert_status_changed),
                              gtk_combo_box_get_active (GTK_COMBO_BOX
                                                        (gs_druid->alert_status_compare)),
                              gtk_combo_box_get_active (GTK_COMBO_BOX
                                                        (gs_druid->alert_size_compare_combo)),
                              gtk_combo_box_get_active (GTK_COMBO_BOX
                                                        (gs_druid->alert_search_content_combo)),
                              gtk_combo_box_get_active (GTK_COMBO_BOX
                                                        (gs_druid->alert_load_compare_combo)),
                              gtk_combo_box_get_active (GTK_COMBO_BOX
                                                        (gs_druid->alert_scan_ports_compare_combo)),
                              size, status, load_time, search_content,
                              search_content_icase, scan_ports,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
                                                                (gs_druid->option_schedule_minutes)),
                              use_auth,
                              gtk_entry_get_text (GTK_ENTRY
                                                  (gs_druid->auth_user)),
                              gtk_entry_get_text (GTK_ENTRY
                                                  (gs_druid->auth_password)));

  gtk_widget_destroy (gs_druid->dialog);

  gs_druid_free ();
}

static void
gs_druid_destroy_cb (GtkObject * object, gpointer user_data)
{
  if (gs_druid != NULL)
    gs_druid_free ();
}

static void
gs_druid_text_changed_cb (GtkEditable * editable, gpointer user_data)
{
  gboolean sensitive = FALSE;

  sensitive = gs_target_ip_is_valid ((gchar *)
                                     gtk_entry_get_text (GTK_ENTRY
                                                         (gs_druid->
                                                          option_server_name)));
  gtk_widget_set_sensitive (gs_druid->alert_ip_changed, !sensitive);
}

static void
gs_druid_schedule_clicked_cb (GtkWidget * widget, gpointer data)
{
  const gchar *name = NULL;

  name = gtk_widget_get_name (widget);
  if (strcmp (name, "option_auto") == 0)
  {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON
                               (gs_druid->option_schedule_minutes), 0);
    gtk_widget_set_sensitive (gs_druid->option_schedule_minutes, TRUE);
  }
  else if (strcmp (name, "option_schedule_on_request") == 0)
    gtk_widget_set_sensitive (gs_druid->option_schedule_minutes, FALSE);
}

static gboolean
gs_druid_page_back_cb (GnomeDruidPage * page, GnomeDruid * druid, gpointer data)
{
  if (page == GNOME_DRUID_PAGE (gs_druid->page_type_web_page)
      || page == GNOME_DRUID_PAGE (gs_druid->page_type_server))
    gnome_druid_set_page (druid, GNOME_DRUID_PAGE (gs_druid->page_type));
  else if (page == GNOME_DRUID_PAGE (gs_druid->page_scheduler))
  {
    if (GTB (gs_druid->option_web_page_type))
      gnome_druid_set_page (druid,
                            GNOME_DRUID_PAGE (gs_druid->page_type_web_page));
    else
      gnome_druid_set_page (druid,
                            GNOME_DRUID_PAGE (gs_druid->page_type_server));
  }

  return TRUE;
}

static gboolean
gs_druid_page_next_cb (GnomeDruidPage * page, GnomeDruid * druid, gpointer data)
{
  gboolean ret = FALSE;
  gchar *tmp = NULL;
  gchar *tmp1 = NULL;
  GString *message = NULL;

  if (page == GNOME_DRUID_PAGE (gs_druid->page_type))
  {
    tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (gs_druid->option_title)));
    g_strstrip (tmp);
    if ((ret = (*tmp == '\0')))
      message =
        g_string_new (_
                      ("Please, enter a identification string for this new Spy\n\n"));
    else
    {
      if (GTB (gs_druid->option_web_page_type))
      {
        gtk_widget_hide (gs_druid->page_type_server);
        gtk_widget_show (gs_druid->page_type_web_page);
      }
      else
      {
        gtk_widget_hide (gs_druid->page_type_web_page);
        gtk_widget_show (gs_druid->page_type_server);
      }
    }
    g_free (tmp), tmp = NULL;
  }
  else if (page == GNOME_DRUID_PAGE (gs_druid->page_type_web_page))
  {
    message = g_string_new (_("Please, enter the following elements:\n\n"));
    tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (gs_druid->option_location)));
    g_strstrip (tmp);
    /* check the "location" entry */
    if (*tmp == '\0')
    {
      ret = TRUE;
      g_string_append (message, _("- The location to monitor\n"));
    }
    g_free (tmp), tmp = NULL;

    /* check the authentication */
    tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (gs_druid->auth_user)));
    g_strstrip (tmp);
    tmp1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (gs_druid->auth_password)));
    g_strstrip (tmp1);
    if ((*tmp != '\0' || *tmp1 != '\0') && (*tmp == '\0' || *tmp1 == '\0'))
    {
      ret = TRUE;
      g_string_append (message, _("- All authentication fields\n"));
    }
    g_free (tmp), tmp = NULL;
    g_free (tmp1), tmp1 = NULL;

    /* check if a leat one option was selected */
    if (!GTB (gs_druid->alert_size) &&
        !GTB (gs_druid->alert_last_modified) &&
        !GTB (gs_druid->alert_size_changed) &&
        !GTB (gs_druid->alert_content_changed) &&
        !GTB (gs_druid->alert_status) &&
        !GTB (gs_druid->alert_status_changed) &&
        !GTB (gs_druid->alert_load_time) &&
        !GTB (gs_druid->alert_search_content))
    {
      ret = TRUE;
      g_string_append (message, _("- A option\n"));
    }
  }
  else if (page == GNOME_DRUID_PAGE (gs_druid->page_type_server))
  {
    message = g_string_new (_("Please, enter the following elements:\n\n"));
    tmp =
      g_strdup (gtk_entry_get_text (GTK_ENTRY (gs_druid->option_server_name)));
    g_strstrip (tmp);
    /* check the "server" entry */
    if (*tmp == '\0')
    {
      ret = TRUE;
      g_string_append (message, _("- The server to monitor\n"));
    }
    g_free (tmp), tmp = NULL;

    /* check if a leat one option was selected */
    if (!GTB (gs_druid->alert_ip_changed) &&
        !GTB (gs_druid->alert_scan_ports) &&
        !GTB (gs_druid->alert_server_changed))
    {
      ret = TRUE;
      g_string_append (message, _("- A option\n"));
    }

    if (!ret && GTB (gs_druid->alert_scan_ports))
    {
      tmp =
        g_strdup (gtk_entry_get_text
                  (GTK_ENTRY (gs_druid->alert_scan_ports_value)));
      g_strstrip (tmp);
      /* check the "ports" entry */
      if (*tmp == '\0')
      {
        ret = TRUE;
        g_string_append (message, _("- A port to check\n"));
      }
      else if (!gs_scan_ports_is_valid (tmp))
      {
        ret = TRUE;
        g_string_append (message, _("- Scan ports format is wrong\n"));
      }
      g_free (tmp), tmp = NULL;
    }
  }

  if (ret)
    gs_show_message (message->str, GTK_MESSAGE_ERROR);

  if (message != NULL)
    g_string_free (message, TRUE), message = NULL;

  return ret;
}

static void
gs_druid_page_finish_prepare_cb (GnomeDruidPage * page, GnomeDruid * druid,
                                 gpointer user_data)
{
  gchar *summary = NULL;
  gchar *schedule = NULL;

  schedule =
    (!gtk_spin_button_get_value_as_int
     (GTK_SPIN_BUTTON (gs_druid->option_schedule_minutes))) ?
    g_strdup (_("Manual")) : g_strdup_printf (_("Every %u minute(s)"),
                                              gtk_spin_button_get_value_as_int
                                              (GTK_SPIN_BUTTON
                                               (gs_druid->
                                                option_schedule_minutes)));

  summary = (GTB (gs_druid->option_server_type)) ?
    g_strdup_printf (_("TYPE : %s\n" "TARGET : %s\n" "SCHEDULING : %s\n"),
                     _
                     ("Server monitoring"),
                     gtk_entry_get_text
                     (GTK_ENTRY
                      (gs_druid->option_server_name)),
                     schedule) :
    g_strdup_printf (_("TYPE : %s\n" "TARGET : %s\n" "SCHEDULING : %s\n"),
                     _
                     ("Web page monitoring"),
                     gtk_entry_get_text
                     (GTK_ENTRY (gs_druid->option_location)), schedule);

  gnome_druid_page_edge_set_text
    (GNOME_DRUID_PAGE_EDGE (gs_druid->page_finish), summary);

  g_free (schedule), schedule = NULL;
  g_free (summary), summary = NULL;
}
