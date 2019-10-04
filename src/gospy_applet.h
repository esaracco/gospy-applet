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

#ifndef _GOSPY_H
#define _GOSPY_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <panel-applet.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>
#include <glade/glade.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <gconf/gconf-client.h>
#include <panel-applet-gconf.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dirent.h>
#include <gnet.h>
#include <regex.h>
#ifdef ENABLE_GNUTLS
#include <gnutls/gnutls.h>
#endif
#ifdef ENABLE_LIBESMTP
#include <libesmtp.h>
#endif

#define GLADE_FILE   GLADEDIR "/gospy-applet.glade.glade"
#define XML_ENCODING "UTF-8"
#define XML_MAIN_FILENAME "spies.xml"
#define XML_GROUPS_FILENAME "groups.xml"

#define GS_SYSTEM_NAME "Linux"

#if defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#ifdef __NetBSD__
#define GS_SYSTEM_NAME "NetBSD"
#elif __OpenBSD__
#define GS_SYSTEM_NAME "OpenBSD"
#elif __FreeBSD__
#define GS_SYSTEM_NAME "FreeBSD"
#endif
#endif

#define GS_VERSION VERSION

#define GS_USER_AGENT \
	"Mozilla/5.0 gospy-applet/" GS_VERSION " (" GS_SYSTEM_NAME ")"

#define GS_BUFFER_LEN 2048

#define GS_COLOR_PANEL_SPY_SELECTED \
        "#4286CE"

#define IS_PANEL_HORIZONTAL(o) \
  (o == PANEL_APPLET_ORIENT_UP || o == PANEL_APPLET_ORIENT_DOWN)

#define GS_UPDATE_UI \
	while (gtk_events_pending ()) gtk_main_iteration ()

#define GS_STRSTRIP( string ) \
	gs_strchomp (gs_strchug (string))

#define WGET(glade, name) \
	glade_xml_get_widget (glade, name)

#define GTB(widget) \
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))

#define GS_GCONF_PATH "/schemas/apps/gospy_applet/prefs"

#define GS_DATE_FORMAT "%Y-%m-%d"
#define GS_DATE_HOUR_FORMAT "%Y-%m-%d, %H:%M:%S"
#define GS_DATE_FORMAT_LABEL _("On <b>%Y-%m-%d</b>")
#define GS_DATE_HOUR_FORMAT_LABEL _("On <b>%Y-%m-%d</b> at <b>%H:%M:%S</b>")

#define GS_SET_STRING(d, s) \
        g_free (d); \
	d = g_strdup (s)

#define GS_SET_REFRESH_FLAG(flag) gs->refresh_flag |= (flag)

#define GS_UNSET_REFRESH_FLAG(flag) gs->refresh_flag &= ~(flag)

#define GS_IS_REFRESH_FLAG(flag) (gs->refresh_flag & flag)

#define GS_SPY_HAVE_SCAN_PORTS(p) \
        (p->scan_ports != NULL && *p->scan_ports != '\0')

#define GS_SPY_HAVE_SEARCH_CONTENT(p) \
        (p->search_content != NULL && *p->search_content != '\0')

#define GS_SPY_HAVE_IP_CHANGED(p) \
        (p->ip_changed == 1)

#define GS_SPY_HAVE_SERVER_CHANGED(p) \
        (p->server_changed == 1)

#define GS_SPY_HAVE_STATUS(p) \
        (p->status > 0)

#define GS_SPY_HAVE_STATUS_CHANGED(p) \
        (p->status_changed == 1)

#define GS_SPY_HAVE_LOAD_TIME(p) \
        (p->load_time > 0)

#define GS_SPY_HAVE_LAST_MODIFIED(p) \
        (p->last_modified > 0)

#define GS_SPY_HAVE_SIZE(p) \
        (p->size > 0)

#define GS_SPY_HAVE_SIZE_CHANGED(p) \
        (p->size_changed == 1)

#define GS_SPY_HAVE_CONTENT_CHANGED(p) \
        (p->content_changed == 1)

#define GS_ALERT_IS_CHANGE(a) \
  ( \
    a == GS_ALERT_IP_CHANGED || \
    a == GS_ALERT_SERVER_CHANGED || \
    a == GS_ALERT_LAST_MODIFIED_CHANGED || \
    a == GS_ALERT_SIZE_CHANGED || \
    a == GS_ALERT_CONTENT_CHANGED || \
    a == GS_ALERT_STATUS_CHANGED || \
    a == GS_ALERT_REDIRECTION \
  )

#define GS_ALERT_IS_ALERT(a) \
  ( \
    a == GS_ALERT_SIZE_FOUND || \
    a == GS_ALERT_STATUS_FOUND || \
    a == GS_ALERT_CONTENT_FOUND || \
    a == GS_ALERT_PORT_OPENED || \
    a == GS_ALERT_PORT_CLOSED || \
    a == GS_ALERT_LOAD_TIME_FOUND || \
    a == GS_ALERT_CONNECTION_ERROR || \
    a == GS_ALERT_CONNECTION_TIMEOUT \
  )

typedef struct _GS GS;
typedef struct _GSSpyProperties GSSpyProperties;
typedef struct _GSCurrent GSCurrent;
typedef struct _GSGui GSGui;
typedef struct _GSChangeProperties GSChangeProperties;
typedef struct _GSAlertProperties GSAlertProperties;
typedef struct _GSScanPortsType GSScanPortsType;
typedef struct _GSActionEmail GSActionEmail;

typedef enum _GSRefreshFlags GSRefreshFlags;
typedef enum _GSCompareChoice GSCompareChoice;
typedef enum _GSScanPortsChoice GSScanPortsChoice;
typedef enum _GSHTMLTagsChoice GSHTMLTagsChoice;
typedef enum _GSType GSType;
typedef enum _GSAlert GSAlert;
typedef enum _GSState GSState;
typedef enum _GSTask GSTask;

enum _GSRefreshFlags
{
  GS_REFRESH_TYPE_NONE = 0,
  GS_REFRESH_TYPE_MAIN_TREE_VIEW = 1 << 0,
  GS_REFRESH_TYPE_ALERTS_TREE_VIEW = 1 << 1,
  GS_REFRESH_TYPE_CHANGES_TREE_VIEW = 1 << 2,
  GS_REFRESH_TYPE_GROUPS_TREE_VIEW = 1 << 3,
  GS_REFRESH_TYPE_GROUPS_COMBO_VIEW = 1 << 4,
  GS_REFRESH_TYPE_SAVE_DATA = 1 << 5
};

#define GS_COMPARE_CHOICE_ITEMS "=,!=,<,>"
enum _GSCompareChoice
{
  GS_COMPARE_CHOICE_EQUAL,
  GS_COMPARE_CHOICE_DIFFERENT,
  GS_COMPARE_CHOICE_INFERIOR,
  GS_COMPARE_CHOICE_SUPERIOR
};

#define GS_SCAN_PORTS_CHOICE_ITEMS _("Opened,Closed")
enum _GSScanPortsChoice
{
  GS_SCAN_PORTS_CHOICE_OPENED,
  GS_SCAN_PORTS_CHOICE_CLOSED
};

#define GS_HTML_TAGS_CHOICE_ITEMS \
  _("The whole page,BODY,DIV,HEAD,HTML,TITLE,HREF,META")
enum _GSHTMLTagsChoice
{
  GS_HTML_TAGS_CHOICE_ALL,
  GS_HTML_TAGS_CHOICE_BODY,
  GS_HTML_TAGS_CHOICE_DIV,
  GS_HTML_TAGS_CHOICE_HEAD,
  GS_HTML_TAGS_CHOICE_HTML,
  GS_HTML_TAGS_CHOICE_TITLE,
  GS_HTML_TAGS_CHOICE_HREF,
  GS_HTML_TAGS_CHOICE_META
};

enum
{
  MAIN_COLUMN_ID,
  MAIN_COLUMN_GROUP,
  MAIN_COLUMN_TYPE,
  MAIN_COLUMN_STATE,
  MAIN_COLUMN_TASK,
  MAIN_COLUMN_TITLE,
  MAIN_COLUMN_DATE_LAST_EVENT,
  MAIN_COLUMN_NB
};

enum
{
  CHANGES_COLUMN_ID,
  CHANGES_COLUMN_DATE,
  CHANGES_COLUMN_TIME,
  CHANGES_COLUMN_TYPE,
  CHANGES_COLUMN_OLD_VALUE,
  CHANGES_COLUMN_NEW_VALUE,
  CHANGES_COLUMN_NB
};

enum
{
  ALERTS_COLUMN_ID,
  ALERTS_COLUMN_DATE,
  ALERTS_COLUMN_TIME,
  ALERTS_COLUMN_TYPE,
  ALERTS_COLUMN_VALUE,
  ALERTS_COLUMN_NB
};

enum
{
  GS_NOTEBOOK_PAGE_MONITORING_WEB_PAGE = 1,
  GS_NOTEBOOK_PAGE_MONITORING_SERVER = 2,
  GS_NOTEBOOK_PAGE_RESULTS = 2
};

enum _GSType
{
  GS_TYPE_NONE,
  GS_TYPE_SERVER,
  GS_TYPE_WEB_PAGE
};

enum _GSAlert
{
  GS_ALERT_NONE,
  GS_ALERT_IP_CHANGED,
  GS_ALERT_SERVER_CHANGED,
  GS_ALERT_LAST_MODIFIED_CHANGED,
  GS_ALERT_SIZE_CHANGED,
  GS_ALERT_CONTENT_CHANGED,
  GS_ALERT_STATUS_CHANGED,
  GS_ALERT_REDIRECTION,
  GS_ALERT_CONNECTION_ERROR,
  GS_ALERT_CONNECTION_TIMEOUT,
  GS_ALERT_CONTENT_FOUND,
  GS_ALERT_STATUS_FOUND,
  GS_ALERT_SIZE_FOUND,
  GS_ALERT_PORT_OPENED,
  GS_ALERT_PORT_CLOSED,
  GS_ALERT_LOAD_TIME_FOUND
};

enum _GSState
{
  GS_STATE_ACTIVATED,
  GS_STATE_DEACTIVATED
};

enum _GSTask
{
  GS_TASK_RUNNING,
  GS_TASK_SLEEPING
};

struct _GSActionEmail
{
  gboolean enabled;
  gchar *address;
  gchar *server;
  guint port;
  gchar *login;
  gchar *password;
};

struct _GSAlertProperties
{
  guint32 id;
  GSAlert alert_type;
  gchar *date_create;
  gchar *time_create;
  gchar *value;

};

struct _GSChangeProperties
{
  guint32 id;
  GSAlert alert_type;
  gchar *file_basename;
  gchar *file_diff_suffix;
  gchar *date_create;
  gchar *time_create;
  gchar *old_value;
  gchar *new_value;

};

struct _GSSpyProperties
{
  GtkWidget *image;
  GdkColor bg_color;
  GtkWidget *event_box;
  GtkTooltips *tooltip;
  time_t timestamp;
  time_t timestamp_update;
  time_t timestamp_last_exec;
  time_t timestamp_next_exec;
  time_t timestamp_last_event;
  gboolean changed;
  guint timeout_id;
  gchar *title;
  gchar *group;

  pthread_t thread_id;
  GSType type;
  GSState state;
  GSTask task;

  /* target */
  gchar *target;
  gchar *scheme;
  guint port;
  gchar *hostname;
  gchar *path;

  /* authentication */
  gboolean use_auth;
  gchar *auth_user;
  gchar *auth_password;
  gchar *basic_auth_line;

  /* Actions */
  GSActionEmail *action_email;

  /* ip alert */
  gboolean ip_changed;
  gchar *old_ip;
  gchar *new_ip;
  /* server alert */
  gboolean server_changed;
  gchar *old_server;
  gchar *new_server;
  /* last modified alert */
  gboolean last_modified;
  gchar *old_last_modified;
  gchar *new_last_modified;
  /* size alert */
  guint size;
  GSCompareChoice size_compare;
  GSHTMLTagsChoice search_content_in;
  gboolean size_changed;
  guint old_size_changed;
  guint new_size_changed;
  /* content alert */
  gboolean content_changed;
  gchar *old_content_checksum_changed;
  gchar *new_content_checksum_changed;
  /* status alert */
  guint status;
  GSCompareChoice status_compare;
  gboolean status_changed;
  guint old_status_changed;
  guint new_status_changed;
  /* load alert */
  GSCompareChoice load_compare;
  guint load_time;
  /* search content alert */
  gchar *search_content;
  guint search_content_icase;
  /* porst scanner alert */
  gint scan_ports_compare;
  gchar *scan_ports;
  gboolean alert_connection_error;

  GList *changes_list;
  GList *alerts_list;
  guint schedule;
};

struct _GSCurrent
{
  GSSpyProperties *spy;
  GSChangeProperties *change;
  GSAlertProperties *alert;
};

struct _GSGui
{
  GtkWidget *about_dialog;
  GladeXML *glade_settings_dialog;
  GtkWidget *settings_dialog;
  GladeXML *glade_edit_dialog;
  GtkWidget *edit_dialog;
  GladeXML *glade_actions_dialog;
  GtkWidget *actions_dialog;
  GtkWidget *status_compare_combo;
  GtkWidget *size_compare_combo;
  GtkWidget *search_content_combo;
  GtkWidget *load_compare_combo;
  GtkWidget *scan_ports_compare_combo;
};

struct _GS
{
  PanelApplet *applet;
  GdkColor bg_color;
  gboolean block_tooltips;
  gboolean display_tooltips;
  gboolean display_main_icon;
  gboolean display_spies_icons;
  gboolean deactivated_all;
  guint deactivated_items;
  GtkWidget *event_box_main;
  GtkWidget *applet_image;
  GtkWidget *applet_box;
  GtkWidget *applet_box_main;
  GtkWidget *applet_box_spies;
  guint timeout_refresh;
  GSCurrent *current;
  GSGui *gui;
  gchar *home_path;
  gchar *main_tree_filter;
  GSRefreshFlags refresh_flag;
  GtkTreeView *main_treeview;
  GtkListStore *main_liststore;
  GtkTreeView *groups_treeview;
  GtkListStore *groups_liststore;
  GtkTreeView *alerts_treeview;
  GtkListStore *alerts_liststore;
  GtkTreeView *changes_treeview;
  GtkListStore *changes_liststore;
  GList *spies_list;
  GList *groups_list;
};

/* Global applet structure */
GS *gs;

gchar *gs_get_combo_groups_selection (GtkWidget * combo);
void gs_build_combo_groups (GtkWidget * combo, const gchar * group);
void
gs_sendmail (const gchar * server, const guint port, const gchar * to,
             const gchar * subject, const gchar * data, GError ** error);
gchar *gs_quote_regex_string (const gchar * unquoted_string);
gchar *gs_strreplace (const gchar * str, const gchar * old, const gchar * new);
gchar **gs_html_tag_get_pattern (const GSHTMLTagsChoice tag);
void gs_spy_set_scheduling (GSSpyProperties * p);
gchar *gs_get_timestamp2string (time_t timestamp, const gchar * format);
gboolean gs_panel_applet_block_events (void);
void gs_add_spy_image (GSSpyProperties * p);
void gs_remove_spy_image (GSSpyProperties * p);
void gs_file_save_diff (const time_t timestamp, const GSAlert alert,
                        const guint suffix);
void gs_file_save (const time_t timestamp, const GSAlert alert,
                   const guint index, const gchar * content);
void gs_append_to_spy_log (const time_t timestamp, const gchar * line);
gboolean gs_target_ip_is_valid (gchar * ip);
gboolean gs_scan_ports_is_valid (gchar * port);
gchar *gs_strchug (gchar * string);
gchar *gs_strchomp (gchar * string);
void gs_split_uri (const gchar * uri, gchar ** scheme, guint * port,
                   gchar ** hostname, gchar ** path);
gchar *gs_convert_to_utf8 (const gchar * data);
void gs_show_message (const gchar * message, const GtkMessageType msg_type);
void gs_refresh_main_tree_content (void);
gint gs_get_selected_row_id (GtkTreeView * tv);
void gs_refresh_alerts_tree_content (GSSpyProperties * p);
void gs_refresh_changes_tree_content (GSSpyProperties * p);
gchar *gs_alert_get_label (const GSAlert alert);
gboolean gs_use_proxy (void);
void gs_get_gnome_proxy_conf (gchar ** host, guint * port);
gchar *gs_get_gnome_browser_conf (void);
void gs_launch_browser (const gchar * uri);
gchar *gs_build_full_uri (const GSSpyProperties * p);
void gs_launch_editor (const gchar * uri);
gchar *gs_build_basic_auth_line (const gchar * auth_user,
                                 const gchar * auth_password);
GtkWidget *gs_build_combo_box (const gchar * name, const guint choice);
gboolean gs_compare_match (const GSCompareChoice choice,
                           const guint ref, const guint new);
gboolean gs_window_present (GtkWidget * w);
#endif
