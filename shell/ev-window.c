/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8;
 * c-indent-level: 8 -*- */
/* this file is part of xreader, a generic document viewer
 *
 *  Copyright (C) 2009 Juanjo Marín <juanj.marin@juntadeandalucia.es>
 *  Copyright (C) 2008 Carlos Garcia Campos
 *  Copyright (C) 2004 Martin Kretzschmar
 *  Copyright (C) 2004 Red Hat, Inc.
 *  Copyright (C) 2000, 2001, 2002, 2003, 2004 Marco Pesenti Gritti
 *  Copyright © 2003, 2004, 2005, 2009 Christian Persch
 *
 *  Author:
 *    Martin Kretzschmar <martink@gnome.org>
 *
 * Xreader Pro is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xreader Pro is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef G_GNUC_USED
#define G_GNUC_USED __attribute__((__used__))
#endif

#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxapp/xapp-favorites.h>

#include "ev-tabs.h"
#include "xreader-view.h"

#include "eggfindbar.h"

#include "ephy-zoom-action.h"
#include "ephy-zoom.h"

#include "ev-annotation-properties-dialog.h"
#include "ev-annotations-toolbar.h"
#include "ev-application.h"
#include "ev-bookmark-action.h"
#include "ev-bookmarks.h"
#include "ev-document-annotations.h"
#include "ev-document-factory.h"
#include "ev-document-find.h"
#include "ev-document-fonts.h"
#include "ev-document-images.h"
#include "ev-document-links.h"
#include "ev-document-misc.h"
#include "ev-document-thumbnails.h"
#include "ev-document-type-builtins.h"
#include "ev-file-exporter.h"
#include "ev-file-helpers.h"
#include "ev-file-monitor.h"
#include "ev-history.h"
#include "ev-image.h"
#include "ev-job-scheduler.h"
#include "ev-jobs.h"
#include "ev-keyring.h"
#include "ev-message-area.h"
#include "ev-metadata.h"
#include "ev-open-recent-action.h"
#include "ev-page-action.h"
#include "ev-password-view.h"
#include "ev-preferences-dialog.h"
#include "ev-print-operation.h"
#include "ev-progress-message-area.h"
#include "ev-properties-dialog.h"
#include "ev-sidebar-annotations.h"
#include "ev-sidebar-attachments.h"
#include "ev-sidebar-bookmarks.h"
#include "ev-sidebar-layers.h"
#include "ev-sidebar-links.h"
#include "ev-sidebar-page.h"
#include "ev-sidebar-thumbnails.h"
#include "ev-sidebar.h"
#include "ev-stock-icons.h"
#include "ev-toolbar.h"
#include "ev-utils.h"
#include "ev-view-presentation.h"
#include "ev-view-type-builtins.h"
#include "ev-view.h"
#include "ev-web-view.h"
#include "ev-window-title.h"
#include "ev-window.h"
#include <ev-history-action.h>

#ifdef ENABLE_DBUS
#include "ev-gdbus-generated.h"
#endif /* ENABLE_DBUS */

typedef enum { PAGE_MODE_DOCUMENT, PAGE_MODE_PASSWORD } EvWindowPageMode;

typedef enum {
  EV_CHROME_MENUBAR = 1 << 0,
  EV_CHROME_TOOLBAR = 1 << 1,
  EV_CHROME_FINDBAR = 1 << 2,
  EV_CHROME_RAISE_TOOLBAR = 1 << 3,
  EV_CHROME_FULLSCREEN_TOOLBAR = 1 << 4,
  EV_CHROME_SIDEBAR = 1 << 5,
  EV_CHROME_NORMAL = EV_CHROME_MENUBAR | EV_CHROME_TOOLBAR | EV_CHROME_SIDEBAR
} EvChrome;

typedef enum { EV_SAVE_DOCUMENT, EV_SAVE_ATTACHMENT, EV_SAVE_IMAGE } EvSaveType;

typedef enum {
  EV_MENUBAR_HIDE,
  EV_MENUBAR_SHOW,
  EV_MENUBAR_TOGGLE
} EvMenubarAction;

struct _EvWindowPrivate {
#if ENABLE_EPUB
#endif
  /* UI */
  EvChrome chrome;

  GtkWidget *main_box;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkWidget *toolbar_revealer;
  GtkWidget *hpaned;
  GtkWidget *view_box;
  GtkWidget *sidebar;
  GtkWidget *find_bar;
  GtkWidget *scrolled_window;
  GtkWidget *view;
  GtkWidget *presentation_view;
  GtkWidget *message_area;
  GtkWidget *password_view;
  GtkWidget *sidebar_thumbs;
  GtkWidget *sidebar_links;
  GtkWidget *sidebar_attachments;
  GtkWidget *sidebar_layers;
  GtkWidget *sidebar_annots;
  GtkWidget *sidebar_bookmarks;
#if ENABLE_EPUB
  GtkWidget *webview;
#endif
  GtkWidget *notebook; // Added for multiple tabs support
  /* Settings */
  GSettings *settings;
  GSettings *default_settings;

  /* Menubar */
  guint menubar_accel_keyval;
  GdkModifierType menubar_accel_modifier;
  gboolean menubar_skip_release;
  gboolean menubar_show_queued;

  /* Progress Messages */
  guint progress_idle;
  GCancellable *progress_cancellable;

  /* Dialogs */
  GtkWidget *properties;
  GtkWidget *print_dialog;

  /* UI Builders */
  /* GtkActionGroup *action_group; */
  /* GtkUIManager *ui_manager; */
  GMenuModel *menubar_model;
  GMenu *favorites_menu;
  GMenu *recent_menu;
  GMenu *bookmarks_menu;
  XAppFavorites *favorites;
  GtkRecentManager *recent_manager;

  /* Popup view */
  GtkWidget *view_popup;
  EvLink *link;
  EvImage *image;
  EvAnnotation *annot;

  /* Popup attachment */
  GtkWidget *attachment_popup;
  GList *attach_list;

  /* Document */
  EvDocumentModel *model;
  char *uri;
  glong uri_mtime;
  char *local_uri;
  gboolean in_reload;
  EvFileMonitor *monitor;
  guint setup_document_idle;

  EvDocument *document;
  EvHistory *history;
  EvWindowPageMode page_mode;
  EvWindowTitle *title;
  EvMetadata *metadata;
  EvBookmarks *bookmarks;

  /* Load params */
  EvLinkDest *dest;
  gchar *search_string;
  EvWindowRunMode window_mode;

  EvJob *load_job;
  EvJob *reload_job;
  EvJob *thumbnail_job;
  EvJob *save_job;
  EvJob *find_job;

  /* Printing */
  GQueue *print_queue;
  GtkPrintSettings *print_settings;
  GtkPageSetup *print_page_setup;
  gboolean close_after_print;
  gboolean close_after_save;

#ifdef ENABLE_DBUS
  /* DBus */
  EvXreaderWindow *skeleton;
  gchar *dbus_object_path;
#endif
};

#define EV_WINDOW_IS_PRESENTATION(w) (w->priv->presentation_view != NULL)

#define PAGE_SELECTOR_ACTION "PageSelector"
#define HISTORY_ACTION "History"

#ifdef ENABLE_DBUS
#define EV_WINDOW_DBUS_OBJECT_PATH "/org/x/reader/Window/%d"
#define EV_WINDOW_DBUS_INTERFACE "org.x.reader.Window"
#endif

#define SIDEBAR_DEFAULT_SIZE 132
#define LINKS_SIDEBAR_ID "links"
#define THUMBNAILS_SIDEBAR_ID "thumbnails"
#define ATTACHMENTS_SIDEBAR_ID "attachments"
#define LAYERS_SIDEBAR_ID "layers"
#define ANNOTS_SIDEBAR_ID "annotations"
#define BOOKMARKS_SIDEBAR_ID "bookmarks"


#define EV_PRINT_SETTINGS_FILE "print-settings"
#define EV_PRINT_SETTINGS_GROUP "Print Settings"
#define EV_PAGE_SETUP_GROUP "Page Setup"

#define MIN_SCALE 0.05409

#define MAX_RECENT_ITEM_LEN (40)

static const gchar * G_GNUC_USED document_print_settings[] = {
    GTK_PRINT_SETTINGS_N_COPIES,    GTK_PRINT_SETTINGS_COLLATE,
    GTK_PRINT_SETTINGS_REVERSE,     GTK_PRINT_SETTINGS_NUMBER_UP,
    GTK_PRINT_SETTINGS_SCALE,       GTK_PRINT_SETTINGS_PRINT_PAGES,
    GTK_PRINT_SETTINGS_PAGE_RANGES, GTK_PRINT_SETTINGS_PAGE_SET,
    GTK_PRINT_SETTINGS_OUTPUT_URI};

static void G_GNUC_USED  ev_window_update_actions(EvWindow *ev_window);
static void G_GNUC_USED  ev_window_setup_bookmarks(EvWindow *window);
void ev_window_close_tab(GtkNotebook *notebook, gint page_num);
void ev_window_close_tabs_to_right(GtkNotebook *notebook, gint page_num);
void ev_window_close_tabs_to_left(GtkNotebook *notebook, gint page_num);
void ev_window_close_all_tabs(GtkNotebook *notebook);
static void G_GNUC_USED  update_chrome_actions(EvWindow *window);
static void G_GNUC_USED  ev_window_setup_action_sensitivity(EvWindow *ev_window);
gboolean G_GNUC_USED  view_actions_focus_in_cb(GtkWidget *widget,
                                         GdkEventFocus *event,
                                         EvWindow *ev_window);
gboolean G_GNUC_USED  view_actions_focus_out_cb(GtkWidget *widget,
                                          GdkEventFocus *event,
                                          EvWindow *ev_window);
static gboolean G_GNUC_USED  view_menu_popup_cb(EvView *view, GList *items,
                                   EvWindow *window);
static void G_GNUC_USED  view_selection_changed_cb(EvView *view, EvWindow *window);
void G_GNUC_USED  view_annot_added(EvView *view, EvAnnotation *annot,
                             EvWindow *window);
void G_GNUC_USED  view_annot_removed(EvView *view, EvAnnotation *annot,
                               EvWindow *window);
static void G_GNUC_USED  view_layers_changed_cb(EvView *view, EvWindow *window);
static void G_GNUC_USED  find_bar_search_changed_cb(EggFindBar *find_bar, GParamSpec *param, EvWindow *ev_window);
static void G_GNUC_USED  ev_window_update_find_status_message(EvWindow *ev_window);
static void G_GNUC_USED  find_bar_close_cb(EggFindBar *find_bar, EvWindow *ev_window);
static void G_GNUC_USED  ev_window_cmd_edit_find(GtkAction *action, EvWindow *ev_window);
static void G_GNUC_USED  ev_window_cmd_edit_find_next(GtkAction *action, EvWindow *ev_window);
static void G_GNUC_USED  ev_window_cmd_edit_find_previous(GtkAction *action, EvWindow *ev_window);
#ifdef ENABLE_DBUS
void G_GNUC_USED  ev_window_sync_source(EvWindow *window, EvSourceLink *link);
#endif
static void G_GNUC_USED  ev_window_toggle_menubar(EvWindow *window, EvMenubarAction action);
static void G_GNUC_USED  ev_window_set_page_mode(EvWindow *window,
                                    EvWindowPageMode page_mode);
static void G_GNUC_USED  ev_window_page_changed_cb(EvWindow *ev_window, gint old_page,
                                      gint new_page, EvDocumentModel *model);
static void G_GNUC_USED  setup_chrome_from_metadata(EvWindow *window);
static void G_GNUC_USED  setup_document_from_metadata(EvWindow *window);
static void G_GNUC_USED  setup_view_from_metadata(EvWindow *window);
static void G_GNUC_USED  ev_window_load_job_cb(EvJob *job, gpointer data);
static void G_GNUC_USED  ev_window_reload_document(EvWindow *window, EvLinkDest *dest);
static void G_GNUC_USED  ev_window_reload_job_cb(EvJob *job, EvWindow *window);
static void G_GNUC_USED  ev_window_set_icon_from_thumbnail(EvJobThumbnail *job,
                                              EvWindow *ev_window);
static void G_GNUC_USED  ev_window_save_job_cb(EvJob *save, EvWindow *window);
static void G_GNUC_USED  ev_window_sizing_mode_changed_cb(EvDocumentModel *model,
                                             GParamSpec *pspec,
                                             EvWindow *ev_window);
static void G_GNUC_USED  ev_window_zoom_changed_cb(EvDocumentModel *model, GParamSpec *pspec,
                                      EvWindow *ev_window);
void G_GNUC_USED  view_external_link_cb(EvWindow *window, EvLinkAction *action);
static void G_GNUC_USED  view_handle_link_cb(EvView *view, EvLink *link, EvWindow *window);
static void G_GNUC_USED  ev_window_document_changed_cb(EvDocumentModel *model,
                                          GParamSpec *pspec, EvWindow *window);
static void G_GNUC_USED  ev_window_rotation_changed_cb(EvDocumentModel *model,
                                          GParamSpec *pspec, EvWindow *window);
static void G_GNUC_USED  ev_window_continuous_changed_cb(EvDocumentModel *model,
                                            GParamSpec *pspec,
                                            EvWindow *window);
static void G_GNUC_USED  ev_window_dual_mode_changed_cb(EvDocumentModel *model,
                                           GParamSpec *pspec, EvWindow *window);
static void G_GNUC_USED  ev_window_dual_mode_odd_pages_left_changed_cb(
    EvDocumentModel *model, GParamSpec *pspec, EvWindow *window);
static void G_GNUC_USED  ev_window_direction_changed_cb(EvDocumentModel *model,
                                           GParamSpec *pspec, EvWindow *window);
static void G_GNUC_USED  ev_window_inverted_colors_changed_cb(EvDocumentModel *model,
                                                 GParamSpec *pspec,
                                                 EvWindow *window);
void G_GNUC_USED  activate_link_cb(GObject *object, EvLink *link, EvWindow *window);
void G_GNUC_USED  history_changed_cb(EvHistory *history, EvWindow *window);
static void G_GNUC_USED  ev_tab_data_free(EvTabData *tab_data);
static void G_GNUC_USED  ev_window_set_document(EvWindow *ev_window, EvDocument *document);
static void G_GNUC_USED  ev_window_set_history(EvWindow *ev_window, EvHistory *history);

static void G_GNUC_USED  ev_window_setup_view(EvWindow *ev_window, EvView *view) {
  g_signal_connect_object(view, "focus_in_event",
                          G_CALLBACK(view_actions_focus_in_cb), ev_window, 0);
  g_signal_connect_object(view, "focus_out_event",
                          G_CALLBACK(view_actions_focus_out_cb), ev_window, 0);
  g_signal_connect_swapped(view, "external-link",
                           G_CALLBACK(view_external_link_cb), ev_window);
  g_signal_connect_object(view, "handle-link", G_CALLBACK(view_handle_link_cb),
                          ev_window, 0);
  g_signal_connect_object(view, "popup", G_CALLBACK(view_menu_popup_cb),
                          ev_window, 0);
  g_signal_connect_object(view, "selection-changed",
                          G_CALLBACK(view_selection_changed_cb), ev_window, 0);
  g_signal_connect_object(view, "annot-added", G_CALLBACK(view_annot_added),
                          ev_window, 0);
  g_signal_connect_object(view, "annot-removed", G_CALLBACK(view_annot_removed),
                          ev_window, 0);
  g_signal_connect_object(view, "layers-changed",
                          G_CALLBACK(view_layers_changed_cb), ev_window, 0);
#ifdef ENABLE_DBUS
  g_signal_connect_swapped(view, "sync-source",
                           G_CALLBACK(ev_window_sync_source), ev_window);
#endif
}

static void G_GNUC_USED  ev_tab_data_free(EvTabData *tab_data) {
  if (!tab_data)
    return;
  if (tab_data->model)
    g_object_unref(tab_data->model);
  if (tab_data->history)
    g_object_unref(tab_data->history);
  if (tab_data->document)
    g_object_unref(tab_data->document);
  g_free(tab_data->uri);
  g_free(tab_data);
}

static void G_GNUC_USED  ev_window_set_history(EvWindow *ev_window, EvHistory *history) {
  if (ev_window->priv->history == history)
    return;

  if (ev_window->priv->history) {
    g_signal_handlers_disconnect_by_data(ev_window->priv->history, ev_window);
    g_object_unref(ev_window->priv->history);
  }

  if (history) {
    ev_window->priv->history = g_object_ref(history);
    g_signal_connect(ev_window->priv->history, "activate-link",
                     G_CALLBACK(activate_link_cb), ev_window);
    g_signal_connect(ev_window->priv->history, "changed",
                     G_CALLBACK(history_changed_cb), ev_window);
  } else {
    ev_window->priv->history = NULL;
  }
}

static void G_GNUC_USED  ev_window_set_model(EvWindow *ev_window, EvDocumentModel *model) {
  if (ev_window->priv->model == model)
    return;

  if (ev_window->priv->model) {
    g_signal_handlers_disconnect_by_data(ev_window->priv->model, ev_window);
    g_object_unref(ev_window->priv->model);
  }

  if (model) {
    ev_window->priv->model = g_object_ref(model);

    /* Connect to model signals */
    g_signal_connect_swapped(ev_window->priv->model, "page-changed",
                             G_CALLBACK(ev_window_page_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::document",
                     G_CALLBACK(ev_window_document_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::scale",
                     G_CALLBACK(ev_window_zoom_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::sizing-mode",
                     G_CALLBACK(ev_window_sizing_mode_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::rotation",
                     G_CALLBACK(ev_window_rotation_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::continuous",
                     G_CALLBACK(ev_window_continuous_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::dual-page",
                     G_CALLBACK(ev_window_dual_mode_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::dual-odd-left",
                     G_CALLBACK(ev_window_dual_mode_odd_pages_left_changed_cb),
                     ev_window);
    g_signal_connect(ev_window->priv->model, "notify::rtl",
                     G_CALLBACK(ev_window_direction_changed_cb), ev_window);
    g_signal_connect(ev_window->priv->model, "notify::inverted-colors",
                     G_CALLBACK(ev_window_inverted_colors_changed_cb),
                     ev_window);

    /* Use official way to set document and setup components */
    ev_window_set_document(ev_window, ev_document_model_get_document(model));

    if (ev_window->priv->sidebar)
      ev_sidebar_set_model(EV_SIDEBAR(ev_window->priv->sidebar), model);

    if (ev_window->priv->view)
      ev_view_set_model(EV_VIEW(ev_window->priv->view), model);

    /* Update sidebar pages unconditionally; they handle NULL documents */
    if (ev_window->priv->sidebar_thumbs)
      ev_sidebar_page_set_model(
          EV_SIDEBAR_PAGE(ev_window->priv->sidebar_thumbs), model);
    if (ev_window->priv->sidebar_links)
      ev_sidebar_page_set_model(EV_SIDEBAR_PAGE(ev_window->priv->sidebar_links),
                                model);
    if (ev_window->priv->sidebar_attachments)
      ev_sidebar_page_set_model(
          EV_SIDEBAR_PAGE(ev_window->priv->sidebar_attachments), model);
    if (ev_window->priv->sidebar_layers)
      ev_sidebar_page_set_model(
          EV_SIDEBAR_PAGE(ev_window->priv->sidebar_layers), model);
    if (ev_window->priv->sidebar_annots)
      ev_sidebar_page_set_model(
          EV_SIDEBAR_PAGE(ev_window->priv->sidebar_annots), model);
    if (ev_window->priv->sidebar_bookmarks)
      ev_sidebar_page_set_model(
          EV_SIDEBAR_PAGE(ev_window->priv->sidebar_bookmarks), model);
  } else {
    ev_window->priv->model = NULL;
    ev_window_set_document(ev_window, NULL);
  }
}

static void G_GNUC_USED  ev_window_setup_bookmarks(EvWindow *window);

void ev_window_switch_page_cb(GtkNotebook *notebook, GtkWidget *page,
                                  guint page_num, EvWindow *ev_window) {
  EvTabData *tab_data =
      (EvTabData *)g_object_get_data(G_OBJECT(page), "ev-tab-data");
  if (tab_data) {
    if (ev_window->priv->view != tab_data->view) {
      if (ev_window->priv->view)
        g_object_unref(ev_window->priv->view);
      ev_window->priv->view = g_object_ref(tab_data->view);
    }

    if (tab_data->model) {
      ev_window_set_model(ev_window, tab_data->model);
    }
    if (tab_data->history) {
      ev_window_set_history(ev_window, tab_data->history);
    }

    if (tab_data->uri) {
      g_free(ev_window->priv->uri);
      ev_window->priv->uri = g_strdup(tab_data->uri);
      ev_window_title_set_uri(ev_window->priv->title, ev_window->priv->uri);
    } else {
      g_free(ev_window->priv->uri);
      ev_window->priv->uri = NULL;
      ev_window_title_set_uri(ev_window->priv->title, NULL);
    }

    update_chrome_actions(ev_window);
    ev_window_update_actions(ev_window);
    ev_window_setup_action_sensitivity(ev_window);
    ev_window_setup_bookmarks(ev_window);
  }
}

static void G_GNUC_USED  ev_window_add_recent(EvWindow *window, const char *filename);
static void G_GNUC_USED  ev_window_run_fullscreen(EvWindow *window);
static void G_GNUC_USED  ev_window_stop_fullscreen(EvWindow *window,
                                      gboolean unfullscreen_window);
static void G_GNUC_USED  ev_window_run_presentation(EvWindow *window);
static void G_GNUC_USED  ev_window_stop_presentation(EvWindow *window,
                                        gboolean unfullscreen_window);
void G_GNUC_USED  view_external_link_cb(EvWindow *window, EvLinkAction *action);
static void G_GNUC_USED  ev_window_load_file_remote(EvWindow *ev_window, GFile *source_file);
static void G_GNUC_USED  ev_window_update_max_min_scale(EvWindow *window);
#ifdef ENABLE_DBUS
void G_GNUC_USED  ev_window_emit_closed(EvWindow *window);
void G_GNUC_USED ev_window_emit_doc_loaded(EvWindow *window);
#endif
static void G_GNUC_USED  ev_window_setup_bookmarks(EvWindow *window);

static void G_GNUC_USED  zoom_control_changed_cb(EphyZoomAction *action, float zoom,
                                    EvWindow *ev_window);

static gint G_GNUC_USED  compare_recent_items(GtkRecentInfo *a, GtkRecentInfo *b);
static gboolean G_GNUC_USED  ev_window_close(EvWindow *window);
G_DEFINE_TYPE_WITH_PRIVATE(EvWindow, ev_window, GTK_TYPE_APPLICATION_WINDOW)

static void G_GNUC_USED on_tab_close_current_activate(GtkMenuItem *item,
                                                      gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
  ev_window_close_current_tab(notebook);
}

static void G_GNUC_USED on_tab_close_left_activate(GtkMenuItem *item,
                                                   gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
  gint current = gtk_notebook_get_current_page(notebook);
  if (current >= 0)
    ev_window_close_tabs_to_left(notebook, current);
}

static void G_GNUC_USED on_tab_close_right_activate(GtkMenuItem *item,
                                                    gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
  gint current = gtk_notebook_get_current_page(notebook);
  if (current >= 0)
    ev_window_close_tabs_to_right(notebook, current);
}

static void G_GNUC_USED on_tab_close_all_activate(GtkMenuItem *item,
                                                  gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
  ev_window_close_all_tabs(notebook);
}

static GtkWidget *G_GNUC_USED ev_window_create_tabs_menu(GtkNotebook *notebook) {
  GtkWidget *menu = gtk_menu_new();
  GtkWidget *item;

  item = gtk_menu_item_new_with_label(_("Close Current Tab"));
  g_signal_connect(item, "activate", G_CALLBACK(on_tab_close_current_activate),
                   notebook);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  item = gtk_menu_item_new_with_label(_("Close Tabs to the Left"));
  g_signal_connect(item, "activate", G_CALLBACK(on_tab_close_left_activate),
                   notebook);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  item = gtk_menu_item_new_with_label(_("Close Tabs to the Right"));
  g_signal_connect(item, "activate", G_CALLBACK(on_tab_close_right_activate),
                   notebook);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  item = gtk_menu_item_new_with_label(_("Close Other Tabs"));
  g_signal_connect(item, "activate", G_CALLBACK(on_tab_close_all_activate),
                   notebook);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  return menu;
}

static void G_GNUC_USED on_tab_menu_cb(GtkWidget *page, gint page_num,
                                       EvWindow *window) {
  GtkWidget *menu = ev_window_create_tabs_menu(GTK_NOTEBOOK(window->priv->notebook));
  gtk_widget_show_all(menu);
  gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

static gboolean G_GNUC_USED  on_tab_button_press_cb(GtkWidget *widget, GdkEventButton *event,
                                       EvWindow *window) {
  if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
    gint page_num;
    GtkWidget *page;

    /* Find which tab was clicked */
    // Note: This is a simplification, GtkNotebook doesn't easily expose tab
    // coordinates. We use current page if no specific tab is found easily.
    page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget));
    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), page_num);
    on_tab_menu_cb(page, page_num, window);
    return TRUE;
  }
  return FALSE;
}

void ev_window_setup_tab_close(GtkNotebook *notebook,
                               gpointer window_user_data) {
  g_signal_connect(notebook, "button-press-event",
                   G_CALLBACK(on_tab_button_press_cb), window_user_data);
  gtk_widget_add_events(GTK_WIDGET(notebook), GDK_BUTTON_PRESS_MASK);
}

static EvTabData * G_GNUC_USED ev_window_find_tab_by_uri(GtkNotebook *notebook,
                                            const gchar *uri) {
  int n = gtk_notebook_get_n_pages(notebook);
  for (int i = 0; i < n; i++) {
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, i);
    EvTabData *tab_data =
        (EvTabData *)g_object_get_data(G_OBJECT(child), "ev-tab-data");
    if (tab_data && g_strcmp0(tab_data->uri, uri) == 0)
      return tab_data;
  }
  return NULL;
}

static gchar *ev_window_get_tab_title_from_uri(const gchar *uri) {
  gchar *filename;
  gchar *raw;
  gchar *unescaped;

  if (!uri)
    return g_strdup(_("Document"));

  filename = g_filename_from_uri(uri, NULL, NULL);
  if (filename) {
    gchar *basename = g_path_get_basename(filename);
    g_free(filename);
    return basename;
  }

  raw = g_path_get_basename(uri);
  unescaped = g_uri_unescape_string(raw, NULL);
  if (unescaped && *unescaped) {
    g_free(raw);
    return unescaped;
  }

  g_free(unescaped);
  return raw;
}

static void G_GNUC_USED on_tab_close_button_clicked(GtkButton *button,
                                                    gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
  GtkWidget *child = g_object_get_data(G_OBJECT(button), "tab-child");
  gint page_num;

  if (!GTK_IS_NOTEBOOK(notebook) || !GTK_IS_WIDGET(child))
    return;

  page_num = gtk_notebook_page_num(notebook, child);
  if (page_num >= 0)
    ev_window_close_tab(notebook, page_num);
}

static void G_GNUC_USED on_tab_close_current_button_clicked(GtkButton *button,
                                                            gpointer user_data) {
  GtkNotebook *notebook = GTK_NOTEBOOK(user_data);

  if (!GTK_IS_NOTEBOOK(notebook))
    return;

  ev_window_close_current_tab(notebook);
}

static GtkWidget *G_GNUC_USED ev_window_get_tab_title_label(GtkNotebook *notebook,
                                                             GtkWidget *child) {
  GtkWidget *tab_label;

  tab_label = gtk_notebook_get_tab_label(notebook, child);
  if (GTK_IS_LABEL(tab_label))
    return tab_label;

  if (GTK_IS_WIDGET(tab_label))
    return g_object_get_data(G_OBJECT(tab_label), "tab-title-label");

  return NULL;
}

static void G_GNUC_USED ev_window_set_tab_title(GtkNotebook *notebook,
                                                GtkWidget *child,
                                                const gchar *title) {
  GtkWidget *label = ev_window_get_tab_title_label(notebook, child);

  if (GTK_IS_LABEL(label))
    gtk_label_set_text(GTK_LABEL(label), title ? title : _("Document"));
}

static GtkWidget *G_GNUC_USED ev_window_create_tab_label(GtkNotebook *notebook,
                                                         GtkWidget *child,
                                                         const gchar *title) {
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *close_button;
  GtkWidget *image;

  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  label = gtk_label_new(title ? title : _("Document"));
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars(GTK_LABEL(label), 28);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

  close_button = gtk_button_new();
  image = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(close_button), image);
  gtk_style_context_add_class(gtk_widget_get_style_context(close_button), "flat");
  gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(close_button), FALSE);
  gtk_widget_set_tooltip_text(close_button, _("Close Tab"));
  g_object_set_data(G_OBJECT(close_button), "tab-child", child);
  g_signal_connect(close_button, "clicked",
                   G_CALLBACK(on_tab_close_button_clicked), notebook);
  gtk_box_pack_end(GTK_BOX(box), close_button, FALSE, FALSE, 0);

  g_object_set_data(G_OBJECT(box), "tab-title-label", label);
  gtk_widget_show_all(box);
  return box;
}

static GtkWidget *G_GNUC_USED ev_window_create_tab_actions(GtkNotebook *notebook) {
  GtkWidget *box;
  GtkWidget *close_button;
  GtkWidget *menu_button;
  GtkWidget *menu;
  GtkWidget *image;

  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

  close_button = gtk_button_new();
  image = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(close_button), image);
  gtk_style_context_add_class(gtk_widget_get_style_context(close_button), "flat");
  gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(close_button), FALSE);
  gtk_widget_set_tooltip_text(close_button, _("Close Current Tab"));
  g_signal_connect(close_button, "clicked",
                   G_CALLBACK(on_tab_close_current_button_clicked), notebook);
  gtk_box_pack_start(GTK_BOX(box), close_button, FALSE, FALSE, 0);

  menu_button = gtk_menu_button_new();
  image = gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(menu_button), image);
  gtk_style_context_add_class(gtk_widget_get_style_context(menu_button), "flat");
  gtk_button_set_relief(GTK_BUTTON(menu_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(menu_button), FALSE);
  gtk_widget_set_tooltip_text(menu_button, _("Tab Actions"));
  menu = ev_window_create_tabs_menu(notebook);
  gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), menu);
  gtk_box_pack_start(GTK_BOX(box), menu_button, FALSE, FALSE, 0);

  gtk_widget_show_all(box);
  return box;
}

int ev_window_add_tab(GtkNotebook *notebook, const gchar *uri) {
  if (!notebook || !GTK_IS_NOTEBOOK(notebook))
    return -1;
  EvTabData *tab_data = g_new0(EvTabData, 1);
  tab_data->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  tab_data->view = ev_view_new();
  tab_data->uri = g_strdup(uri);

  GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(notebook));
  if (EV_IS_WINDOW(toplevel)) {
    EvWindow *ev_window = EV_WINDOW(toplevel);
    tab_data->model = ev_document_model_new();
    tab_data->history = ev_history_new(tab_data->model);
    ev_view_set_model(EV_VIEW(tab_data->view), tab_data->model);
    ev_window_setup_view(ev_window, EV_VIEW(tab_data->view));
  }

  gtk_container_add(GTK_CONTAINER(tab_data->scrolled_window), tab_data->view);
  gtk_widget_show(tab_data->view);
  gtk_widget_show(tab_data->scrolled_window);

  // Tab title
  gchar *tab_title = ev_window_get_tab_title_from_uri(uri);
  GtkWidget *tab_label = ev_window_create_tab_label(GTK_NOTEBOOK(notebook),
                                                     tab_data->scrolled_window,
                                                     tab_title);
  int page_num = gtk_notebook_append_page(notebook, tab_data->scrolled_window,
                                          tab_label);
  g_free(tab_title);

  // Stores the pointer to tab_data in the tab widget
  g_object_set_data_full(G_OBJECT(tab_data->scrolled_window), "ev-tab-data",
                         tab_data, (GDestroyNotify)ev_tab_data_free);

  if (gtk_notebook_get_n_pages(notebook) > 1) {
    gtk_notebook_set_show_tabs(notebook, TRUE);
  }

  gtk_notebook_set_current_page(notebook, page_num);
  return page_num;
}

EvTabData *ev_window_get_current_tab(GtkNotebook *notebook) {
  if (!notebook || !GTK_IS_NOTEBOOK(notebook))
    return NULL;
  int page = gtk_notebook_get_current_page(notebook);
  if (page < 0)
    return NULL;
  GtkWidget *child = gtk_notebook_get_nth_page(notebook, page);
  if (!child)
    return NULL;
  return (EvTabData *)g_object_get_data(G_OBJECT(child), "ev-tab-data");
}

void ev_window_close_tab(GtkNotebook *notebook, gint page_num) {
  gint n_pages = gtk_notebook_get_n_pages(notebook);

  if (n_pages <= 1)
    return;

  if (page_num >= 0 && page_num < n_pages) {
    gtk_notebook_remove_page(notebook, page_num);
    if (gtk_notebook_get_n_pages(notebook) <= 1)
      gtk_notebook_set_show_tabs(notebook, FALSE);
  }
}

void ev_window_close_current_tab(GtkNotebook *notebook) {
  gint current_page = gtk_notebook_get_current_page(notebook);
  ev_window_close_tab(notebook, current_page);
}

void ev_window_close_tabs_to_right(GtkNotebook *notebook, gint page_num) {
  int n = gtk_notebook_get_n_pages(notebook);
  if (n <= 1)
    return;
  for (int i = n - 1; i > page_num; i--) {
    ev_window_close_tab(notebook, i);
  }
}

void ev_window_close_tabs_to_left(GtkNotebook *notebook, gint page_num) {
  if (gtk_notebook_get_n_pages(notebook) <= 1)
    return;
  for (int i = 0; i < page_num; i++) {
    /* Always remove page 0 because indexes shift */
    ev_window_close_tab(notebook, 0);
  }
}

void ev_window_close_all_tabs(GtkNotebook *notebook) {
  gint current = gtk_notebook_get_current_page(notebook);
  ev_window_close_tabs_to_right(notebook, current);
  ev_window_close_tabs_to_left(notebook, current);
}

static gchar * G_GNUC_USED sizing_mode_to_string(EvSizingMode mode) {
  switch (mode) {
  case EV_SIZING_BEST_FIT:
    return "best-fit";
  case EV_SIZING_FIT_WIDTH:
    return "fit-width";
  case EV_SIZING_FREE:
    return "free";
  default:
    g_assert_not_reached();
  }
}

static gint G_GNUC_USED  sizing_mode_string_to_int(const gchar *string) {
  if (g_strcmp0(string, "best-fit") == 0) {
    return 0;
  } else if (g_strcmp0(string, "fit-width") == 0) {
    return 1;
  } else if (g_strcmp0(string, "free") == 0) {
    return 2;
  }

  return 0;
}

static gdouble get_screen_dpi(EvWindow *window) {
  return ev_document_misc_get_screen_dpi_at_window(GTK_WINDOW(window));
}

static void G_GNUC_USED  ev_window_set_action_sensitive(EvWindow *ev_window,
                                           const char *name,
                                           gboolean sensitive) {
  GAction *action = g_action_map_lookup_action (G_ACTION_MAP (ev_window), name);
  if (action && G_IS_SIMPLE_ACTION (action)) {
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
  }
}

static void G_GNUC_USED  ev_window_set_action_visible(EvWindow *window,
                                          const char *name, gboolean visible) {
  /* GAction visibility is usually handled via GMenu/GAction enabled state or by removing from menu.
     For Wave 1, we treat it as sensitivity or just a no-op if it's not applicable. */
  ev_window_set_action_sensitive (window, name, visible);
}

static void G_GNUC_USED  ev_window_setup_action_sensitivity(EvWindow *ev_window) {
  EvDocument *document = ev_window->priv->document;
  const EvDocumentInfo *info = NULL;
  gboolean has_document = FALSE;
  gboolean ok_to_print = TRUE;
  gboolean ok_to_copy = TRUE;
  gboolean has_properties = TRUE;
  gboolean override_restrictions = TRUE;
  gboolean can_get_text = FALSE;
  gboolean has_pages = FALSE;
  gboolean can_find = FALSE;

  if (document) {
    has_document = TRUE;
    has_pages = ev_document_get_n_pages(document) > 0;
    info = ev_document_get_info(document);
  }

  if (!info || info->fields_mask == 0) {
    has_properties = FALSE;
  }

  if (has_document && EV_IS_SELECTION(document)) {
    can_get_text = TRUE;
  } else if (has_document && document->iswebdocument) {
    can_get_text = TRUE;
  }
  if (has_pages && EV_IS_DOCUMENT_FIND(document)) {
    can_find = TRUE;
  }

  if (has_document && ev_window->priv->settings) {
    override_restrictions = g_settings_get_boolean(ev_window->priv->settings,
                                                   GS_OVERRIDE_RESTRICTIONS);
  }

  if (!override_restrictions && info &&
      info->fields_mask & EV_DOCUMENT_INFO_PERMISSIONS) {
    ok_to_print = (info->permissions & EV_DOCUMENT_PERMISSIONS_OK_TO_PRINT);
    ok_to_copy = (info->permissions & EV_DOCUMENT_PERMISSIONS_OK_TO_COPY);
  }

  if (has_document && !ev_print_operation_exists_for_document(document))
    ok_to_print = FALSE;

  /* File menu */
  ev_window_set_action_sensitive(ev_window, "FileOpenCopy", has_document);
  ev_window_set_action_sensitive(ev_window, "FileSaveAs",
                                 has_document && ok_to_copy);
  ev_window_set_action_sensitive(ev_window, "FilePrint",
                                 has_pages && ok_to_print);
  ev_window_set_action_sensitive(ev_window, "FileProperties",
                                 has_document && has_properties);

  /* Edit menu */
  ev_window_set_action_sensitive(ev_window, "EditSelectAll",
                                 has_pages && can_get_text);
  ev_window_set_action_sensitive(ev_window, "EditFind", can_find);
  ev_window_set_action_sensitive(ev_window, "Slash", can_find);
  ev_window_set_action_sensitive(ev_window, "EditRotateLeft",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "EditRotateRight",
                                 has_pages && !(document->iswebdocument));

  /* View menu */
  /*If it has pages it is a document, so our check for a webdocument won't lead
   * to a crash. We need to switch these view modes off since more than one
   *webview is hard to manage, and would lead to unexpected behaviour in case
   * the number of webviews gets too large.
   */
  ev_window_set_action_sensitive(ev_window, "ViewContinuous",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewDual",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewDualOddLeft", has_pages);
  ev_window_set_action_sensitive(ev_window, "ViewRtl", has_pages);
  ev_window_set_action_sensitive(ev_window, "ViewBestFit",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewPageWidth",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewReload", has_pages);
  ev_window_set_action_sensitive(ev_window, "ViewAutoscroll",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewInvertedColors", has_pages);
  ev_window_set_action_sensitive(ev_window, "ViewExpandWindow",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewZoomIn",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewZoomOut",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewZoomReset",
                                 has_pages && !(document->iswebdocument));
  ev_window_set_action_sensitive(ev_window, "ViewPresentation",
                                 has_pages && !(document->iswebdocument));

  /* Bookmarks menu */
  ev_window_set_action_sensitive(ev_window, "BookmarksAdd",
                                 has_pages && ev_window->priv->bookmarks);

  /* Toolbar-specific actions: */
  ev_window_set_action_sensitive(ev_window, PAGE_SELECTOR_ACTION, has_pages);
  ev_toolbar_set_preset_sensitivity(EV_TOOLBAR(ev_window->priv->toolbar),
                                    has_pages && !(document->iswebdocument));

  ev_window_update_actions(ev_window);
}

static void G_GNUC_USED  ev_window_update_actions(EvWindow *ev_window) {
#if ENABLE_EPUB
  EvWebView *webview = NULL;
#endif
  EvView *view = NULL;

  int n_pages = 0, page = -1;
  gboolean has_pages = FALSE;
  gboolean presentation_mode;
  gboolean can_find_in_page = FALSE;

  if (ev_window->priv->document) {
    page = ev_document_model_get_page(ev_window->priv->model);
    n_pages = ev_document_get_n_pages(ev_window->priv->document);
    has_pages = n_pages > 0;
  }
#if ENABLE_EPUB
  if (ev_window->priv->document &&
      ev_window->priv->document->iswebdocument == TRUE) {
    webview = EV_WEB_VIEW(ev_window->priv->webview);
  } else
#endif
  {
    view = EV_VIEW(ev_window->priv->view);
  }
  can_find_in_page =
      (ev_window->priv->find_job &&
       ev_job_find_has_results(EV_JOB_FIND(ev_window->priv->find_job)));
  if (view) {
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_window_set_action_sensitive(
        ev_window, "EditCopy",
        has_pages && ev_view_get_has_selection(
                         tab_data ? EV_VIEW(tab_data->view) : view));
  }
#if ENABLE_EPUB
  else if (webview) {
    /*
     * The webkit2 function for this is an asynchronous call,
     * so our only option is to set this to always on, and we'll take care of
     * whether we can copy or not when this command is actually given.
     */
    ev_window_set_action_sensitive(ev_window, "EditCopy", has_pages);
  }
#endif
  ev_window_set_action_sensitive(ev_window, "EditFindNext",
                                 has_pages && can_find_in_page);
  ev_window_set_action_sensitive(ev_window, "EditFindPrevious",
                                 has_pages && can_find_in_page);
  ev_window_set_action_sensitive(ev_window, "F3",
                                 has_pages && can_find_in_page);

  presentation_mode = EV_WINDOW_IS_PRESENTATION(ev_window);

  if (ev_window->priv->document &&
      ev_window->priv->document->iswebdocument == FALSE) {
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_window_set_action_sensitive(
        ev_window, "ViewZoomIn",
        has_pages &&
            ev_view_can_zoom_in(tab_data ? EV_VIEW(tab_data->view) : view) &&
            !presentation_mode);
    ev_window_set_action_sensitive(
        ev_window, "ViewZoomOut",
        has_pages &&
            ev_view_can_zoom_out(tab_data ? EV_VIEW(tab_data->view) : view) &&
            !presentation_mode);
    ev_window_set_action_sensitive(ev_window, "ViewZoomReset",
                                   has_pages && !presentation_mode);
  }
  /* Go menu */
  if (has_pages) {
    ev_window_set_action_sensitive(ev_window, "GoPreviousPage", page > 0);
    ev_window_set_action_sensitive(ev_window, "GoNextPage", page < n_pages - 1);
    ev_window_set_action_sensitive(ev_window, "GoFirstPage", page > 0);
    ev_window_set_action_sensitive(ev_window, "GoLastPage", page < n_pages - 1);
    ev_window_set_action_sensitive(
        ev_window, "GoPreviousHistory",
        ev_history_can_go_back(ev_window->priv->history));
    ev_window_set_action_sensitive(
        ev_window, "GoNextHistory",
        ev_history_can_go_forward(ev_window->priv->history));
  } else {
    ev_window_set_action_sensitive(ev_window, "GoFirstPage", FALSE);
    ev_window_set_action_sensitive(ev_window, "GoPreviousPage", FALSE);
    ev_window_set_action_sensitive(ev_window, "GoNextPage", FALSE);
    ev_window_set_action_sensitive(ev_window, "GoLastPage", FALSE);
    ev_window_set_action_sensitive(ev_window, "GoPreviousHistory", FALSE);
    ev_window_set_action_sensitive(ev_window, "GoNextHistory", FALSE);
  }
}

static void G_GNUC_USED  set_widget_visibility(GtkWidget *widget, gboolean visible) {
  g_assert(GTK_IS_WIDGET(widget));

  if (visible)
    gtk_widget_show(widget);
  else
    gtk_widget_hide(widget);
}

static void G_GNUC_USED  update_chrome_visibility(EvWindow *window) {
  EvWindowPrivate *priv = window->priv;
  gboolean menubar, toolbar, findbar, fullscreen_toolbar, sidebar;
  gboolean fullscreen_mode, presentation, fullscreen;

  presentation = EV_WINDOW_IS_PRESENTATION(window);
  fullscreen = ev_document_model_get_fullscreen(priv->model);
  fullscreen_mode = fullscreen || presentation;

  menubar = (priv->chrome & EV_CHROME_MENUBAR) != 0 && !fullscreen_mode;
  toolbar = ((priv->chrome & EV_CHROME_TOOLBAR) != 0 ||
             (priv->chrome & EV_CHROME_RAISE_TOOLBAR) != 0) &&
            !presentation && !fullscreen;
  fullscreen_toolbar = ((priv->chrome & EV_CHROME_FULLSCREEN_TOOLBAR) != 0 ||
                        (priv->chrome & EV_CHROME_RAISE_TOOLBAR) != 0) &&
                       fullscreen;
  findbar = (priv->chrome & EV_CHROME_FINDBAR) != 0;
  sidebar = (priv->chrome & EV_CHROME_SIDEBAR) != 0 && priv->document &&
            !presentation;

  set_widget_visibility(priv->menubar, menubar);
  set_widget_visibility(priv->find_bar, findbar);
  set_widget_visibility(priv->sidebar, sidebar);
  gtk_revealer_set_reveal_child(GTK_REVEALER(priv->toolbar_revealer), toolbar);

  ev_toolbar_set_style(EV_TOOLBAR(priv->toolbar), fullscreen_toolbar);
}

static void G_GNUC_USED  update_chrome_flag(EvWindow *window, EvChrome flag,
                               gboolean active) {
  EvWindowPrivate *priv = window->priv;

  if (active) {
    priv->chrome |= flag;
  } else {
    priv->chrome &= ~flag;
  }
}


static void G_GNUC_USED  update_chrome_actions(EvWindow *window) {
  EvWindowPrivate *priv = window->priv;
  GAction *action;
  gboolean show_menubar = g_settings_get_boolean(priv->settings, "show-menubar");
  gboolean show_toolbar = g_settings_get_boolean(priv->settings, "show-toolbar");

  action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewMenubar");
  if (action) g_action_change_state(action, g_variant_new_boolean(show_menubar));

  action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewToolbar");
  if (action) g_action_change_state(action, g_variant_new_boolean(show_toolbar));
}

/**
 * ev_window_is_empty:
 * @ev_window: The instance of the #EvWindow.
 *
 * It does look if there is any document loaded or if there is any job to load
 * a document.
 *
 * Returns: %TRUE if there isn't any document loaded or any any documente to be
 *          loaded, %FALSE in other case.
 */
gboolean ev_window_is_empty(const EvWindow *ev_window) {
  g_return_val_if_fail(EV_IS_WINDOW(ev_window), FALSE);

  return (ev_window->priv->document == NULL) &&
         (ev_window->priv->load_job == NULL);
}

static void G_GNUC_USED  ev_window_set_message_area(EvWindow *window, GtkWidget *area) {
  if (window->priv->message_area == area)
    return;

  if (window->priv->message_area)
    gtk_widget_destroy(window->priv->message_area);
  window->priv->message_area = area;

  if (!area)
    return;

  gtk_box_pack_start(GTK_BOX(window->priv->view_box),
                     window->priv->message_area, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(window->priv->view_box),
                        window->priv->message_area, 0);
  g_object_add_weak_pointer(G_OBJECT(window->priv->message_area),
                            (gpointer) & (window->priv->message_area));
}

static void G_GNUC_USED  ev_window_message_area_response_cb(EvMessageArea *area,
                                               gint response_id,
                                               EvWindow *window) {
  ev_window_set_message_area(window, NULL);
}

static void G_GNUC_USED  ev_window_error_message(EvWindow *window, GError *error,
                                    const gchar *format, ...) {
  GtkWidget *area;
  va_list args;
  gchar *msg = NULL;

  if (window->priv->message_area)
    return;

  va_start(args, format);
  msg = g_strdup_vprintf(format, args);
  va_end(args);

  area = ev_message_area_new(GTK_MESSAGE_ERROR, msg, GTK_STOCK_CLOSE,
                             GTK_RESPONSE_CLOSE, NULL);
  g_free(msg);

  if (error)
    ev_message_area_set_secondary_text(EV_MESSAGE_AREA(area), error->message);
  g_signal_connect(area, "response",
                   G_CALLBACK(ev_window_message_area_response_cb), window);
  gtk_widget_show(area);
  ev_window_set_message_area(window, area);
}

static void G_GNUC_USED  ev_window_warning_message(EvWindow *window, const gchar *format,
                                      ...) {
  GtkWidget *area;
  va_list args;
  gchar *msg = NULL;

  if (window->priv->message_area)
    return;

  va_start(args, format);
  msg = g_strdup_vprintf(format, args);
  va_end(args);

  area = ev_message_area_new(GTK_MESSAGE_WARNING, msg, GTK_STOCK_CLOSE,
                             GTK_RESPONSE_CLOSE, NULL);
  g_free(msg);
  g_signal_connect(area, "response",
                   G_CALLBACK(ev_window_message_area_response_cb), window);
  gtk_widget_show(area);
  ev_window_set_message_area(window, area);
}

typedef struct _LinkTitleData {
  EvLink *link;
  const gchar *link_title;
} LinkTitleData;

static gboolean G_GNUC_USED  find_link_cb(GtkTreeModel *tree_model, GtkTreePath *path,
                             GtkTreeIter *iter, LinkTitleData *data) {
  EvLink *link;
  gboolean retval = FALSE;

  gtk_tree_model_get(tree_model, iter, EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
                     -1);

  if (!link) {
    return retval;
  }

  if (ev_link_action_equal(ev_link_get_action(data->link),
                           ev_link_get_action(link))) {
    data->link_title = ev_link_get_title(link);
    retval = TRUE;
  }

  g_object_unref(link);

  return retval;
}

static const gchar * G_GNUC_USED ev_window_find_title_for_link(EvWindow *window,
                                                  EvLink *link) {
  if (EV_IS_DOCUMENT_LINKS(window->priv->document) &&
      ev_document_links_has_document_links(
          EV_DOCUMENT_LINKS(window->priv->document))) {
    LinkTitleData data;
    GtkTreeModel *model;

    data.link = link;
    data.link_title = NULL;

    g_object_get(G_OBJECT(window->priv->sidebar_links), "model", &model, NULL);
    if (model) {
      gtk_tree_model_foreach(model, (GtkTreeModelForeachFunc)find_link_cb,
                             &data);

      g_object_unref(model);
    }

    return data.link_title;
  }

  return NULL;
}

static void G_GNUC_USED  view_handle_link_cb(EvView *view, EvLink *link, EvWindow *window) {
  EvLink *new_link = NULL;

  if (!ev_link_get_title(link)) {
    const gchar *link_title;

    link_title = ev_window_find_title_for_link(window, link);
    if (link_title) {
      new_link = ev_link_new(link_title, ev_link_get_action(link));
    } else {
      EvLinkAction *action;
      EvLinkDest *dest;
      gchar *page_label;
      gchar *title;

      action = ev_link_get_action(link);
      dest = ev_link_action_get_dest(action);
      page_label = ev_document_links_get_dest_page_label(
          EV_DOCUMENT_LINKS(window->priv->document), dest);
      if (!page_label) {
        return;
      }

      title = g_strdup_printf(_("Page %s"), page_label);
      g_free(page_label);

      new_link = ev_link_new(title, action);
      g_free(title);
    }
  }
  ev_history_add_link(window->priv->history, new_link ? new_link : link);
  if (new_link) {
    g_object_unref(new_link);
  }
}

static void G_GNUC_USED  view_selection_changed_cb(EvView *view, EvWindow *window) {
  EvTabData *tab_data =
      ev_window_get_current_tab(GTK_NOTEBOOK(window->priv->notebook));
  ev_window_set_action_sensitive(
      window, "EditCopy",
      ev_view_get_has_selection(tab_data ? EV_VIEW(tab_data->view) : view));
}

static void G_GNUC_USED  view_layers_changed_cb(EvView *view, EvWindow *window) {
  ev_sidebar_layers_update_layers_state(
      EV_SIDEBAR_LAYERS(window->priv->sidebar_layers));
}

static void G_GNUC_USED  ev_window_page_changed_cb(EvWindow *ev_window, gint old_page,
                                      gint new_page, EvDocumentModel *model) {
  ev_window_update_actions(ev_window);

  ev_window_update_find_status_message(ev_window);

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_int(ev_window->priv->metadata, "page", new_page);
}

static const gchar * G_GNUC_USED ev_window_sidebar_get_current_page_id(EvWindow *ev_window) {
  GtkWidget *current_page;
  const gchar *id;

  g_object_get(ev_window->priv->sidebar, "current_page", &current_page, NULL);

  if (current_page == ev_window->priv->sidebar_links) {
    id = LINKS_SIDEBAR_ID;
  } else if (current_page == ev_window->priv->sidebar_thumbs) {
    id = THUMBNAILS_SIDEBAR_ID;
  } else if (current_page == ev_window->priv->sidebar_attachments) {
    id = ATTACHMENTS_SIDEBAR_ID;
  } else if (current_page == ev_window->priv->sidebar_layers) {
    id = LAYERS_SIDEBAR_ID;
  } else if (current_page == ev_window->priv->sidebar_annots) {
    id = ANNOTS_SIDEBAR_ID;
  } else if (current_page == ev_window->priv->sidebar_bookmarks) {
    id = BOOKMARKS_SIDEBAR_ID;
  } else {
    g_assert_not_reached();
  }

  g_object_unref(current_page);

  return id;
}

static void G_GNUC_USED  ev_window_sidebar_set_current_page(EvWindow *window,
                                               const gchar *page_id) {
  EvDocument *document = window->priv->document;

  if (document == NULL)
    return;

  EvSidebar *sidebar = EV_SIDEBAR(window->priv->sidebar);
  GtkWidget *links = window->priv->sidebar_links;
  GtkWidget *thumbs = window->priv->sidebar_thumbs;
  GtkWidget *attachments = window->priv->sidebar_attachments;
  GtkWidget *annots = window->priv->sidebar_annots;
  GtkWidget *layers = window->priv->sidebar_layers;
  GtkWidget *bookmarks = window->priv->sidebar_bookmarks;

  if (strcmp(page_id, LINKS_SIDEBAR_ID) == 0 &&
      ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(links), document)) {
    ev_sidebar_set_page(sidebar, links);
  } else if (strcmp(page_id, THUMBNAILS_SIDEBAR_ID) == 0 &&
             ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(thumbs),
                                              document)) {
    ev_sidebar_set_page(sidebar, thumbs);
  } else if (strcmp(page_id, ATTACHMENTS_SIDEBAR_ID) == 0 &&
             ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(attachments),
                                              document)) {
    ev_sidebar_set_page(sidebar, attachments);
  } else if (strcmp(page_id, LAYERS_SIDEBAR_ID) == 0 &&
             ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(layers),
                                              document)) {
    ev_sidebar_set_page(sidebar, layers);
  } else if (strcmp(page_id, ANNOTS_SIDEBAR_ID) == 0 &&
             ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(annots),
                                              document)) {
    ev_sidebar_set_page(sidebar, annots);
  } else if (strcmp(page_id, BOOKMARKS_SIDEBAR_ID) == 0 &&
             ev_sidebar_page_support_document(EV_SIDEBAR_PAGE(bookmarks),
                                              document)) {
    ev_sidebar_set_page(sidebar, bookmarks);
  }
}

static void G_GNUC_USED  update_document_mode(EvWindow *window, EvDocumentMode mode) {
  if (mode == EV_DOCUMENT_MODE_PRESENTATION) {
    if (window->priv->document) {
      if (window->priv->document->iswebdocument) {
        ev_window_warning_message(window,
                                  _("Cannot enter presentation mode with ePub "
                                    "documents, use fullscreen mode instead."));
        return;
      }
    }
    ev_window_run_presentation(window);
  } else if (mode == EV_DOCUMENT_MODE_FULL_SCREEN) {
    ev_window_run_fullscreen(window);
  }
}

static void G_GNUC_USED  ev_window_init_metadata_with_default_values(EvWindow *window) {
  GSettings *settings = window->priv->default_settings;
  EvMetadata *metadata = window->priv->metadata;

  /* Chrome */
  if (!ev_metadata_has_key(metadata, "sidebar_visibility")) {
    ev_metadata_set_boolean(metadata, "sidebar_visibility",
                            g_settings_get_boolean(settings, "show-sidebar"));
  }

  /* Sidebar */
  if (!ev_metadata_has_key(metadata, "sidebar_size")) {
    ev_metadata_set_int(metadata, "sidebar_size",
                        g_settings_get_int(settings, "sidebar-size"));
  }
  if (!ev_metadata_has_key(metadata, "thumbnails_size")) {
    ev_metadata_set_int(metadata, "thumbnails_size",
                        g_settings_get_int(settings, "thumbnails-size"));
  }
  if (!ev_metadata_has_key(metadata, "sidebar_page")) {
    gchar *sidebar_page_id = g_settings_get_string(settings, "sidebar-page");

    ev_metadata_set_string(metadata, "sidebar_page", sidebar_page_id);
    g_free(sidebar_page_id);
  }

  /* Document model */
  if (!ev_metadata_has_key(metadata, "continuous")) {
    ev_metadata_set_boolean(metadata, "continuous",
                            g_settings_get_boolean(settings, "continuous"));
  }
  if (!ev_metadata_has_key(metadata, "dual-page")) {
    ev_metadata_set_boolean(metadata, "dual-page",
                            g_settings_get_boolean(settings, "dual-page"));
  }
  if (!ev_metadata_has_key(metadata, "dual-page-odd-left")) {
    ev_metadata_set_boolean(
        metadata, "dual-page-odd-left",
        g_settings_get_boolean(settings, "dual-page-odd-left"));
  }
  if (!ev_metadata_has_key(metadata, "rtl")) {
    ev_metadata_set_boolean(
        metadata, "rtl",
        gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL ? TRUE : FALSE);
  }
  if (!ev_metadata_has_key(metadata, "inverted-colors")) {
    ev_metadata_set_boolean(
        metadata, "inverted-colors",
        g_settings_get_boolean(settings, "inverted-colors"));
  }
  if (!ev_metadata_has_key(metadata, "sizing_mode")) {
    EvSizingMode mode = g_settings_get_enum(settings, "sizing-mode");

    ev_metadata_set_string(metadata, "sizing_mode",
                           sizing_mode_to_string(mode));
  }

  if (!ev_metadata_has_key(metadata, "zoom")) {
    ev_metadata_set_double(metadata, "zoom",
                           g_settings_get_double(settings, "zoom"));
  }

  if (!ev_metadata_has_key(metadata, "fullscreen")) {
    ev_metadata_set_boolean(metadata, "fullscreen",
                            g_settings_get_boolean(settings, "fullscreen"));
  }
  if (!ev_metadata_has_key(metadata, "window_maximized")) {
    ev_metadata_set_boolean(
        metadata, "window_maximized",
        g_settings_get_boolean(settings, "window-maximized"));
  }
}

static void G_GNUC_USED  setup_chrome_from_metadata(EvWindow *window) {
  gboolean show_sidebar;

  if (!window->priv->metadata)
    return;

  if (ev_metadata_get_boolean(window->priv->metadata, "sidebar_visibility",
                              &show_sidebar))
    update_chrome_flag(window, EV_CHROME_SIDEBAR, show_sidebar);

  update_chrome_visibility(window);
}

static void G_GNUC_USED  setup_sidebar_from_metadata(EvWindow *window) {
  gchar *page_id;
  gint sidebar_size;
  gint thumbnails_size;

  if (!window->priv->metadata)
    return;

  if (ev_metadata_get_int(window->priv->metadata, "sidebar_size",
                          &sidebar_size))
    gtk_paned_set_position(GTK_PANED(window->priv->hpaned), sidebar_size);

  if (ev_metadata_get_int(window->priv->metadata, "thumbnails_size",
                          &thumbnails_size))
    ev_sidebar_thumbnails_set_size(
        EV_SIDEBAR_THUMBNAILS(window->priv->sidebar_thumbs), thumbnails_size);

  if (ev_metadata_get_string(window->priv->metadata, "sidebar_page", &page_id))
    ev_window_sidebar_set_current_page(window, page_id);
}

static void G_GNUC_USED  setup_model_from_metadata(EvWindow *window) {
  gint page;
  gchar *sizing_mode;
  gdouble zoom;
  gint rotation;
  gboolean inverted_colors = FALSE;
  gboolean continuous = FALSE;
  gboolean dual_page = FALSE;
  gboolean dual_page_odd_left = FALSE;
  gboolean rtl = FALSE;
  gboolean fullscreen = FALSE;

  if (!window->priv->metadata)
    return;

  /* Current page */
  if (!window->priv->dest &&
      ev_metadata_get_int(window->priv->metadata, "page", &page)) {
    ev_document_model_set_page(window->priv->model, page);
  }

  /* Sizing mode */
  if (ev_metadata_get_string(window->priv->metadata, "sizing_mode",
                             &sizing_mode)) {
    ev_document_model_set_sizing_mode(window->priv->model,
                                      sizing_mode_string_to_int(sizing_mode));
  }

  /* Zoom */
  if (ev_document_model_get_sizing_mode(window->priv->model) ==
      EV_SIZING_FREE) {
    if (ev_metadata_get_double(window->priv->metadata, "zoom", &zoom)) {
      zoom *= get_screen_dpi(window) / 72.0;
      ev_document_model_set_scale(window->priv->model, zoom);
    }
  }

  /* Rotation */
  if (ev_metadata_get_int(window->priv->metadata, "rotation", &rotation)) {
    switch (rotation) {
    case 90:
      rotation = 90;
      break;
    case 180:
      rotation = 180;
      break;
    case 270:
      rotation = 270;
      break;
    default:
      rotation = 0;
      break;
    }
    ev_document_model_set_rotation(window->priv->model, rotation);
  }

  /* Inverted Colors */
  if (ev_metadata_get_boolean(window->priv->metadata, "inverted-colors",
                              &inverted_colors)) {
    ev_document_model_set_inverted_colors(window->priv->model, inverted_colors);
  }

  /* Continuous */
  if (ev_metadata_get_boolean(window->priv->metadata, "continuous",
                              &continuous)) {
    ev_document_model_set_continuous(window->priv->model, continuous);
  }

  /* Dual page */
  if (ev_metadata_get_boolean(window->priv->metadata, "dual-page",
                              &dual_page)) {
    ev_document_model_set_dual_page(window->priv->model, dual_page);
  }

  /* Dual page odd pages left */
  if (ev_metadata_get_boolean(window->priv->metadata, "dual-page-odd-left",
                              &dual_page_odd_left)) {
    ev_document_model_set_dual_page_odd_pages_left(window->priv->model,
                                                   dual_page_odd_left);
  }

  /* Right to left document */
  if (ev_metadata_get_boolean(window->priv->metadata, "rtl", &rtl)) {
    ev_document_model_set_rtl(window->priv->model, rtl);
  }

  /* Fullscreen */
  if (ev_metadata_get_boolean(window->priv->metadata, "fullscreen",
                              &fullscreen)) {
    if (fullscreen)
      ev_window_run_fullscreen(window);
  }
}

static void G_GNUC_USED  setup_document_from_metadata(EvWindow *window) {
  gint width;
  gint height;
  gdouble width_ratio;
  gdouble height_ratio;

  if (!window->priv->metadata)
    return;

  setup_sidebar_from_metadata(window);

  if (ev_metadata_get_int(window->priv->metadata, "window_width", &width) &&
      ev_metadata_get_int(window->priv->metadata, "window_height", &height))
    return; /* size was already set in setup_size_from_metadata */

  g_settings_get(window->priv->default_settings, "window-ratio", "(dd)",
                 &width_ratio, &height_ratio);
  if (width_ratio > 0. && height_ratio > 0.) {
    gdouble document_width;
    gdouble document_height;
    GdkScreen *screen;
    gint request_width;
    gint request_height;

    ev_document_get_max_page_size(window->priv->document, &document_width,
                                  &document_height);

    request_width = (gint)(width_ratio * document_width + 0.5);
    request_height = (gint)(height_ratio * document_height + 0.5);

    screen = gtk_window_get_screen(GTK_WINDOW(window));
    if (screen) {
      request_width = MIN(request_width, gdk_screen_get_width(screen));
      request_height = MIN(request_height, gdk_screen_get_height(screen));
    }

    if (request_width > 0 && request_height > 0) {
      gtk_window_resize(GTK_WINDOW(window), request_width, request_height);
    }
  }
}

static void G_GNUC_USED  setup_size_from_metadata(EvWindow *window) {
  gint width;
  gint height;
  gboolean maximized;
  gint x;
  gint y;

  if (!window->priv->metadata)
    return;

  if (ev_metadata_get_boolean(window->priv->metadata, "window_maximized",
                              &maximized)) {
    if (maximized) {
      gtk_window_maximize(GTK_WINDOW(window));
      return;
    } else {
      gtk_window_unmaximize(GTK_WINDOW(window));
    }
  }

  if (ev_metadata_get_int(window->priv->metadata, "window_x", &x) &&
      ev_metadata_get_int(window->priv->metadata, "window_y", &y)) {
    gtk_window_move(GTK_WINDOW(window), x, y);
  }

  if (ev_metadata_get_int(window->priv->metadata, "window_width", &width) &&
      ev_metadata_get_int(window->priv->metadata, "window_height", &height)) {
    gtk_window_resize(GTK_WINDOW(window), width, height);
  }
}

static void G_GNUC_USED  setup_view_from_metadata(EvWindow *window) {
  gboolean presentation;

  if (!window->priv->metadata)
    return;

  /* Presentation */
  if (ev_metadata_get_boolean(window->priv->metadata, "presentation",
                              &presentation)) {
    if (presentation) {
      if (window->priv->document->iswebdocument == TRUE) {
        return;
      } else {
        ev_window_run_presentation(window);
      }
    }
  }
}

static void G_GNUC_USED  page_cache_size_changed(GSettings *settings, gchar *key,
                                    EvWindow *ev_window) {
  guint page_cache_mb;

  page_cache_mb = g_settings_get_uint(settings, GS_PAGE_CACHE_SIZE);
  ev_view_set_page_cache_size(EV_VIEW(ev_window->priv->view),
                              page_cache_mb * 1024 * 1024);
}

static void G_GNUC_USED  ev_window_clear_thumbnail_job(EvWindow *ev_window) {
  if (ev_window->priv->thumbnail_job != NULL) {
    if (!ev_job_is_finished(ev_window->priv->thumbnail_job))
      ev_job_cancel(ev_window->priv->thumbnail_job);

    g_signal_handlers_disconnect_by_func(ev_window->priv->thumbnail_job,
                                         ev_window_set_icon_from_thumbnail,
                                         ev_window);
    g_object_unref(ev_window->priv->thumbnail_job);
    ev_window->priv->thumbnail_job = NULL;
  }
}

static void G_GNUC_USED  ev_window_set_icon_from_thumbnail(EvJobThumbnail *job,
                                              EvWindow *ev_window) {
  if (job->thumbnail) {
    if (ev_document_model_get_inverted_colors(ev_window->priv->model))
      ev_document_misc_invert_pixbuf(job->thumbnail);
    gtk_window_set_icon(GTK_WINDOW(ev_window), job->thumbnail);
  }

  ev_window_clear_thumbnail_job(ev_window);
}

static void G_GNUC_USED  ev_window_refresh_window_thumbnail(EvWindow *ev_window) {
  gdouble page_width;
  gdouble scale;
  gint rotation;
  EvDocument *document = ev_window->priv->document;

  if (!document || ev_document_get_n_pages(document) <= 0 ||
      ev_document_get_n_pages(document) <= 0 ||
      !ev_document_check_dimensions(document) || document->iswebdocument) {
    return;
  }

  ev_window_clear_thumbnail_job(ev_window);

  ev_document_get_page_size(document, 0, &page_width, NULL);
  scale = 128. / page_width;
  rotation = ev_document_model_get_rotation(ev_window->priv->model);

  ev_window->priv->thumbnail_job =
      ev_job_thumbnail_new(document, 0, rotation, scale);

  if (document->iswebdocument) {
    ev_job_set_run_mode(EV_JOB(ev_window->priv->thumbnail_job),
                        EV_JOB_RUN_MAIN_LOOP);
  }
  g_signal_connect(ev_window->priv->thumbnail_job, "finished",
                   G_CALLBACK(ev_window_set_icon_from_thumbnail), ev_window);
  ev_job_scheduler_push_job(ev_window->priv->thumbnail_job,
                            EV_JOB_PRIORITY_NONE);
}

static void G_GNUC_USED  override_restrictions_changed(GSettings *settings, gchar *key,
                                          EvWindow *ev_window) {
  ev_window_setup_action_sensitivity(ev_window);
}

static GSettings * G_GNUC_USED ev_window_ensure_settings(EvWindow *ev_window) {
  EvWindowPrivate *priv = ev_window->priv;

  if (priv->settings != NULL)
    return priv->settings;

  priv->settings = g_settings_new(GS_SCHEMA_NAME);
  g_signal_connect(priv->settings, "changed::" GS_OVERRIDE_RESTRICTIONS,
                   G_CALLBACK(override_restrictions_changed), ev_window);
  g_signal_connect(priv->settings, "changed::" GS_PAGE_CACHE_SIZE,
                   G_CALLBACK(page_cache_size_changed), ev_window);

  return priv->settings;
}

static gboolean G_GNUC_USED  ev_window_setup_document(EvWindow *ev_window) {
  const EvDocumentInfo *info;
  EvDocument *document = ev_window->priv->document;

  ev_window->priv->setup_document_idle = 0;
  
  ev_window_refresh_window_thumbnail(ev_window);

  ev_window_set_page_mode(ev_window, PAGE_MODE_DOCUMENT);

  ev_window_title_set_document(ev_window->priv->title, document);
  ev_window_title_set_uri(ev_window->priv->title, ev_window->priv->uri);

  ev_window_ensure_settings(ev_window);
  ev_window_setup_action_sensitivity(ev_window);

  if (ev_window->priv->properties) {
    ev_properties_dialog_set_document(
        EV_PROPERTIES_DIALOG(ev_window->priv->properties), ev_window->priv->uri,
        ev_window->priv->document);
  }

  info = ev_document_get_info(document);
  update_document_mode(ev_window, info->mode);

  if (EV_IS_DOCUMENT_FIND(document)) {
    if (ev_window->priv->search_string &&
        !EV_WINDOW_IS_PRESENTATION(ev_window)) {
      ev_window_cmd_edit_find(NULL, ev_window);
      egg_find_bar_set_search_string(EGG_FIND_BAR(ev_window->priv->find_bar),
                                     ev_window->priv->search_string);
    }

    g_clear_pointer(&ev_window->priv->search_string, g_free);
  }

  /*FIXME*/
  if (EV_WINDOW_IS_PRESENTATION(ev_window) && document->iswebdocument == FALSE)
    gtk_widget_grab_focus(ev_window->priv->presentation_view);
  else if (!gtk_widget_get_visible(ev_window->priv->find_bar)) {
    if (document->iswebdocument == FALSE)
      gtk_widget_grab_focus(ev_window->priv->view);
#if ENABLE_EPUB
    else
      gtk_widget_grab_focus(ev_window->priv->webview);
#endif
  }
  return FALSE;
}

static void G_GNUC_USED  ev_window_set_document(EvWindow *ev_window, EvDocument *document) {
  if (ev_window->priv->document == document)
    return;

  if (ev_window->priv->document)
    g_object_unref(ev_window->priv->document);
  ev_window->priv->document = document ? g_object_ref(document) : NULL;

  ev_window_update_max_min_scale(ev_window);

  ev_window_set_message_area(ev_window, NULL);

  if (!document)
    return;

  if (ev_document_get_n_pages(document) <= 0) {
    ev_window_warning_message(ev_window, "%s",
                              _("The document contains no pages"));
  } else if (!ev_document_check_dimensions(document) &&
             document->iswebdocument == FALSE) {
    ev_window_warning_message(ev_window, "%s",
                              _("The document contains only empty pages"));
  }

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window)) {
    ev_metadata_set_int(ev_window->priv->metadata, "num-pages",
                        ev_document_get_n_pages(document));
  }

  gtk_widget_show(ev_window->priv->toolbar);

#if ENABLE_EPUB
  if (document->iswebdocument == TRUE && ev_window->priv->view != NULL) {
    /*We have encountered a web document, replace the xreader view with a web
     * view, if the web view is not already loaded.*/
    gtk_container_remove(GTK_CONTAINER(ev_window->priv->scrolled_window),
                         ev_window->priv->view);
    ev_view_disconnect_handlers(EV_VIEW(ev_window->priv->view));
    g_object_unref(ev_window->priv->view);
    ev_window->priv->view = NULL;
    gtk_container_add(GTK_CONTAINER(ev_window->priv->scrolled_window),
                      ev_window->priv->webview);
    gtk_widget_show(ev_window->priv->webview);
  } else if (ev_window->priv->webview != NULL &&
             document->iswebdocument == FALSE) {
    /*Since the document is not a webdocument might as well get rid of the
     * webview now*/
    ev_web_view_disconnect_handlers(EV_WEB_VIEW(ev_window->priv->webview));
    g_object_ref_sink(ev_window->priv->webview);
    g_object_unref(ev_window->priv->webview);
    ev_window->priv->webview = NULL;
  }
#endif
  if (EV_WINDOW_IS_PRESENTATION(ev_window) &&
      document->iswebdocument == FALSE) {
    gint current_page;

    current_page = ev_view_presentation_get_current_page(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view));
    gtk_widget_destroy(ev_window->priv->presentation_view);
    ev_window->priv->presentation_view = NULL;

    /* Update the model with the current presentation page */
    ev_document_model_set_page(ev_window->priv->model, current_page);
    ev_window_run_presentation(ev_window);
  } else if (EV_WINDOW_IS_PRESENTATION(ev_window) &&
             document->iswebdocument == TRUE) {
    ev_window_warning_message(
        ev_window, "%s",
        _("Presentation mode is not supported for ePub documents."));
  }

  if (ev_window->priv->setup_document_idle > 0)
    g_source_remove(ev_window->priv->setup_document_idle);

  ev_window->priv->setup_document_idle =
      g_idle_add((GSourceFunc)ev_window_setup_document, ev_window);
}

static void G_GNUC_USED  ev_window_document_changed(EvWindow *ev_window,
                                       gpointer user_data) {
  if (ev_window->priv->settings &&
      g_settings_get_boolean(ev_window->priv->settings, GS_AUTO_RELOAD))
    ev_window_reload_document(ev_window, NULL);
}

void ev_window_password_view_unlock(EvWindow *ev_window) {
  const gchar *password;

  g_assert(ev_window->priv->load_job);

  password = ev_password_view_get_password(
      EV_PASSWORD_VIEW(ev_window->priv->password_view));
  ev_job_load_set_password(EV_JOB_LOAD(ev_window->priv->load_job), password);
  ev_job_scheduler_push_job(ev_window->priv->load_job, EV_JOB_PRIORITY_NONE);
}

static void G_GNUC_USED  ev_window_clear_load_job(EvWindow *ev_window) {
  if (ev_window->priv->load_job) {
    /* Em modo multi-abas, não desconectamos o sinal para permitir que
       carregamentos em segundo plano terminem e atualizem suas abas.
       Apenas limpamos a referência da janela ao job "principal". */
    if (ev_window->priv->notebook &&
        gtk_notebook_get_n_pages(GTK_NOTEBOOK(ev_window->priv->notebook)) > 1) {
      g_object_unref(ev_window->priv->load_job);
      ev_window->priv->load_job = NULL;
      return;
    }

    g_signal_handlers_disconnect_by_func(ev_window->priv->load_job,
                                         ev_window_load_job_cb, ev_window);
    ev_job_cancel(ev_window->priv->load_job);
    g_object_unref(ev_window->priv->load_job);
    ev_window->priv->load_job = NULL;
  }
}

static void G_GNUC_USED  ev_window_clear_reload_job(EvWindow *ev_window) {
  if (ev_window->priv->reload_job != NULL) {
    if (!ev_job_is_finished(ev_window->priv->reload_job))
      ev_job_cancel(ev_window->priv->reload_job);

    g_signal_handlers_disconnect_by_func(ev_window->priv->reload_job,
                                         ev_window_reload_job_cb, ev_window);
    g_object_unref(ev_window->priv->reload_job);
    ev_window->priv->reload_job = NULL;
  }
}

static void G_GNUC_USED  ev_window_clear_local_uri(EvWindow *ev_window) {
  if (ev_window->priv->local_uri) {
    ev_tmp_uri_unlink(ev_window->priv->local_uri);
    g_free(ev_window->priv->local_uri);
    ev_window->priv->local_uri = NULL;
  }
}

static void G_GNUC_USED  ev_window_handle_link(EvWindow *ev_window, EvLinkDest *dest) {
  if (ev_window->priv->document->iswebdocument == TRUE) {
    return;
  }
  if (dest) {
    EvLink *link;
    EvLinkAction *link_action;

    link_action = ev_link_action_new_dest(dest);
    link = ev_link_new(NULL, link_action);
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_view_handle_link(
        EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), link);
    g_object_unref(link);
  }
}

/* This callback will executed when load job will be finished.
 *
 * Since the flow of the error dialog is very confusing, we assume that both
 * document and uri will go away after this function is called, and thus we need
 * to ref/dup them.  Additionally, it needs to clear
 * ev_window->priv->password_{uri,document}, and thus people who call this
 * function should _not_ necessarily expect those to exist after being
 * called. */
static void G_GNUC_USED  ev_window_load_job_cb(EvJob *job, gpointer data) {
  EvWindow *ev_window = EV_WINDOW(data);
  EvDocument *document = EV_JOB(job)->document;
  EvJobLoad *job_load = EV_JOB_LOAD(job);

  const gchar *uri = job_load->uri;
  EvTabData *tab_data =
      ev_window_find_tab_by_uri(GTK_NOTEBOOK(ev_window->priv->notebook), uri);

  if (tab_data) {
    ev_view_set_loading(EV_VIEW(tab_data->view), FALSE);
    if (!ev_job_is_failed(job)) {
      if (tab_data->document)
        g_object_unref(tab_data->document);
      tab_data->document = g_object_ref(document);
      ev_document_model_set_document(tab_data->model, document);
    }
  }

  EvTabData *current_tab =
      ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));

  // Se for o documento da aba atual (ou se não houver abas), atualiza o modelo
  // da janela
  if (current_tab == tab_data || !tab_data) {
    ev_view_set_loading(
        EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), FALSE);
    /* Success! */
    if (!ev_job_is_failed(job)) {
      ev_document_model_set_document(ev_window->priv->model, document);

#ifdef ENABLE_DBUS
      ev_window_emit_doc_loaded(ev_window);
#endif
      setup_chrome_from_metadata(ev_window);
      update_chrome_actions(ev_window);
      setup_document_from_metadata(ev_window);
      setup_view_from_metadata(ev_window);

      ev_window_add_recent(ev_window, uri);

      ev_window_title_set_type(ev_window->priv->title,
                               EV_WINDOW_TITLE_DOCUMENT);
      if (job_load->password) {
        GPasswordSave flags;

        flags = ev_password_view_get_password_save_flags(
            EV_PASSWORD_VIEW(ev_window->priv->password_view));
        ev_keyring_save_password(uri, job_load->password, flags);
      }

      ev_window_handle_link(ev_window, ev_window->priv->dest);
      g_clear_object(&ev_window->priv->dest);

      switch (ev_window->priv->window_mode) {
      case EV_WINDOW_MODE_FULLSCREEN:
        ev_window_run_fullscreen(ev_window);
        break;
      case EV_WINDOW_MODE_PRESENTATION:
        ev_window_run_presentation(ev_window);
        break;
      default:
        break;
      }

      /* Create a monitor for the document */
      if (ev_window->priv->monitor)
        g_object_unref(ev_window->priv->monitor);
      ev_window->priv->monitor = ev_file_monitor_new(uri);
      g_signal_connect_swapped(ev_window->priv->monitor, "changed",
                               G_CALLBACK(ev_window_document_changed),
                               ev_window);

      ev_window_clear_load_job(ev_window);
      return;
    }

    if (g_error_matches(job->error, EV_DOCUMENT_ERROR,
                        EV_DOCUMENT_ERROR_ENCRYPTED)) {
      gchar *password;

      setup_view_from_metadata(ev_window);

      /* First look whether password is in keyring */
      password = ev_keyring_lookup_password(ev_window->priv->uri);
      if (password) {
        if (job_load->password && strcmp(password, job_load->password) == 0) {
          /* Password in kering is wrong */
          ev_job_load_set_password(job_load, NULL);
          /* FIXME: delete password from keyring? */
        } else {
          ev_job_load_set_password(job_load, password);
          ev_job_scheduler_push_job(job, EV_JOB_PRIORITY_NONE);
          g_free(password);
          return;
        }

        g_free(password);
      }

      /* We need to ask the user for a password */
      ev_window_title_set_uri(ev_window->priv->title, ev_window->priv->uri);
      ev_window_title_set_type(ev_window->priv->title,
                               EV_WINDOW_TITLE_PASSWORD);

      ev_password_view_set_uri(EV_PASSWORD_VIEW(ev_window->priv->password_view),
                               job_load->uri);

      ev_window_set_page_mode(ev_window, PAGE_MODE_PASSWORD);

      ev_job_load_set_password(job_load, NULL);
      ev_password_view_ask_password(
          EV_PASSWORD_VIEW(ev_window->priv->password_view));
    } else {
      ev_window_error_message(ev_window, job->error, "%s",
                              _("Unable to open document"));
      ev_window_clear_load_job(ev_window);
    }
  }
}

static void G_GNUC_USED  ev_window_reload_job_cb(EvJob *job, EvWindow *ev_window) {
  GtkWidget *widget;

  if (ev_job_is_failed(job)) {
    ev_window_clear_reload_job(ev_window);
    ev_window->priv->in_reload = FALSE;
    if (ev_window->priv->dest) {
      g_object_unref(ev_window->priv->dest);
      ev_window->priv->dest = NULL;
    }

    return;
  }

  ev_document_model_set_document(ev_window->priv->model, job->document);
  if (ev_window->priv->dest) {
    ev_window_handle_link(ev_window, ev_window->priv->dest);
    g_clear_object(&ev_window->priv->dest);
  }

  /* Restart the search after reloading */
  widget = gtk_window_get_focus(GTK_WINDOW(ev_window));
  if (widget && gtk_widget_get_ancestor(widget, EGG_TYPE_FIND_BAR)) {
    find_bar_search_changed_cb(EGG_FIND_BAR(ev_window->priv->find_bar), NULL,
                               ev_window);
  }

  ev_window_clear_reload_job(ev_window);
  ev_window->priv->in_reload = FALSE;
}

/**
 * ev_window_get_uri:
 * @ev_window: The instance of the #EvWindow.
 *
 * It returns the uri of the document showed in the #EvWindow.
 *
 * Returns: the uri of the document showed in the #EvWindow.
 */
const char *ev_window_get_uri(EvWindow *ev_window) {
  return ev_window->priv->uri;
}

/**
 * ev_window_close_dialogs:
 * @ev_window: The window where dialogs will be closed.
 *
 * It looks for password, print and properties dialogs and closes them and
 * frees them from memory. If there is any print job it does free it too.
 */
static void G_GNUC_USED  ev_window_close_dialogs(EvWindow *ev_window) {
  if (ev_window->priv->print_dialog)
    gtk_widget_destroy(ev_window->priv->print_dialog);
  ev_window->priv->print_dialog = NULL;

  if (ev_window->priv->properties)
    gtk_widget_destroy(ev_window->priv->properties);
  ev_window->priv->properties = NULL;
}

static void G_GNUC_USED  ev_window_clear_progress_idle(EvWindow *ev_window) {
  if (ev_window->priv->progress_idle > 0)
    g_source_remove(ev_window->priv->progress_idle);
  ev_window->priv->progress_idle = 0;
}

static void G_GNUC_USED  reset_progress_idle(EvWindow *ev_window) {
  ev_window->priv->progress_idle = 0;
}

static void G_GNUC_USED  ev_window_show_progress_message(EvWindow *ev_window, guint interval,
                                            GSourceFunc function) {
  if (ev_window->priv->progress_idle > 0)
    g_source_remove(ev_window->priv->progress_idle);
  ev_window->priv->progress_idle = g_timeout_add_seconds_full(
      G_PRIORITY_DEFAULT, interval, function, ev_window,
      (GDestroyNotify)reset_progress_idle);
}

static void G_GNUC_USED  ev_window_reset_progress_cancellable(EvWindow *ev_window) {
  if (ev_window->priv->progress_cancellable)
    g_cancellable_reset(ev_window->priv->progress_cancellable);
  else
    ev_window->priv->progress_cancellable = g_cancellable_new();
}

static void G_GNUC_USED  ev_window_progress_response_cb(EvProgressMessageArea *area,
                                           gint response, EvWindow *ev_window) {
  if (response == GTK_RESPONSE_CANCEL)
    g_cancellable_cancel(ev_window->priv->progress_cancellable);
  ev_window_set_message_area(ev_window, NULL);
}

static gboolean G_GNUC_USED  show_loading_progress(EvWindow *ev_window) {
  GtkWidget *area;
  gchar *text;
  gchar *display_name;

  if (ev_window->priv->message_area)
    return FALSE;

  text = g_uri_unescape_string(ev_window->priv->uri, NULL);
  display_name = g_markup_escape_text(text, -1);
  g_free(text);
  text = g_strdup_printf(_("Loading document from “%s”"), display_name);

  area = ev_progress_message_area_new(GTK_STOCK_OPEN, text, GTK_STOCK_CLOSE,
                                      GTK_RESPONSE_CLOSE, GTK_STOCK_CANCEL,
                                      GTK_RESPONSE_CANCEL, NULL);
  g_signal_connect(area, "response", G_CALLBACK(ev_window_progress_response_cb),
                   ev_window);
  gtk_widget_show(area);
  ev_window_set_message_area(ev_window, area);

  g_free(text);
  g_free(display_name);

  return FALSE;
}

static void G_GNUC_USED  ev_window_load_remote_failed(EvWindow *ev_window, GError *error) {
  if (!ev_window->priv->view)
    return;

  EvTabData *tab_data =
      ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
  ev_view_set_loading(
      EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), FALSE);
  ev_window->priv->in_reload = FALSE;
  ev_window_error_message(ev_window, error, "%s", _("Unable to open document"));
  g_free(ev_window->priv->local_uri);
  ev_window->priv->local_uri = NULL;
  ev_window->priv->uri_mtime = 0;
}

static void G_GNUC_USED  set_uri_mtime(GFile *source, GAsyncResult *async_result,
                          EvWindow *ev_window) {
  GFileInfo *info;
  GError *error = NULL;

  info = g_file_query_info_finish(source, async_result, &error);

  if (error) {
    ev_window->priv->uri_mtime = 0;
    g_error_free(error);
  } else {
    GTimeVal mtime;

    g_file_info_get_modification_time(info, &mtime);
    ev_window->priv->uri_mtime = mtime.tv_sec;
    g_object_unref(info);
  }

  g_object_unref(source);
}

static void G_GNUC_USED  mount_volume_ready_cb(GFile *source, GAsyncResult *async_result,
                                  EvWindow *ev_window) {
  GError *error = NULL;

  g_file_mount_enclosing_volume_finish(source, async_result, &error);

  if (error) {
    ev_window_load_remote_failed(ev_window, error);
    g_object_unref(source);
    g_error_free(error);
  } else {
    /* Volume successfully mounted,
       try opening the file again */
    ev_window_load_file_remote(ev_window, source);
  }
}

static void G_GNUC_USED  window_open_file_copy_ready_cb(GFile *source,
                                           GAsyncResult *async_result,
                                           EvWindow *ev_window) {
  GError *error = NULL;

  ev_window_clear_progress_idle(ev_window);
  ev_window_set_message_area(ev_window, NULL);

  g_file_copy_finish(source, async_result, &error);
  if (!error) {
    ev_job_scheduler_push_job(ev_window->priv->load_job, EV_JOB_PRIORITY_NONE);
    g_file_query_info_async(source, G_FILE_ATTRIBUTE_TIME_MODIFIED, 0,
                            G_PRIORITY_DEFAULT, NULL,
                            (GAsyncReadyCallback)set_uri_mtime, ev_window);
    return;
  }

  if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED)) {
    GMountOperation *operation;

    operation = gtk_mount_operation_new(GTK_WINDOW(ev_window));
    g_file_mount_enclosing_volume(source, G_MOUNT_MOUNT_NONE, operation, NULL,
                                  (GAsyncReadyCallback)mount_volume_ready_cb,
                                  ev_window);
    g_object_unref(operation);
  } else if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    ev_window_clear_load_job(ev_window);
    ev_window_clear_local_uri(ev_window);
    g_free(ev_window->priv->uri);
    ev_window->priv->uri = NULL;
    g_object_unref(source);

    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_view_set_loading(
        EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), FALSE);
  } else {
    ev_window_load_remote_failed(ev_window, error);
    g_object_unref(source);
  }

  g_error_free(error);
}

static void G_GNUC_USED  window_open_file_copy_progress_cb(goffset n_bytes,
                                              goffset total_bytes,
                                              EvWindow *ev_window) {
  gchar *status;
  gdouble fraction;

  if (!ev_window->priv->message_area)
    return;

  if (total_bytes <= 0)
    return;

  fraction = n_bytes / (gdouble)total_bytes;
  status =
      g_strdup_printf(_("Downloading document (%d%%)"), (gint)(fraction * 100));

  ev_progress_message_area_set_status(
      EV_PROGRESS_MESSAGE_AREA(ev_window->priv->message_area), status);
  ev_progress_message_area_set_fraction(
      EV_PROGRESS_MESSAGE_AREA(ev_window->priv->message_area), fraction);

  g_free(status);
}

static void G_GNUC_USED  ev_window_load_file_remote(EvWindow *ev_window,
                                       GFile *source_file) {
  GFile *target_file;

  if (!ev_window->priv->local_uri) {
    char *base_name, *template;
    GFile *tmp_file;
    GError *err = NULL;

    /* We'd like to keep extension of source uri since
     * it helps to resolve some mime types, say cbz.
     */
    base_name = g_file_get_basename(source_file);
    template = g_strdup_printf("document.XXXXXX-%s", base_name);
    g_free(base_name);

    tmp_file = ev_mkstemp_file(template, &err);
    g_free(template);
    if (tmp_file == NULL) {
      ev_window_error_message(ev_window, err, "%s",
                              _("Failed to load remote file."));
      g_error_free(err);
      return;
    }

    ev_window->priv->local_uri = g_file_get_uri(tmp_file);
    g_object_unref(tmp_file);

    ev_job_load_set_uri(EV_JOB_LOAD(ev_window->priv->load_job),
                        ev_window->priv->local_uri);
  }

  ev_window_reset_progress_cancellable(ev_window);

  target_file = g_file_new_for_uri(ev_window->priv->local_uri);
  g_file_copy_async(
      source_file, target_file, G_FILE_COPY_OVERWRITE, G_PRIORITY_DEFAULT,
      ev_window->priv->progress_cancellable,
      (GFileProgressCallback)window_open_file_copy_progress_cb, ev_window,
      (GAsyncReadyCallback)window_open_file_copy_ready_cb, ev_window);
  g_object_unref(target_file);

  ev_window_show_progress_message(ev_window, 1,
                                  (GSourceFunc)show_loading_progress);
}

void ev_window_open_uri(EvWindow *ev_window, const char *uri, EvLinkDest *dest,
                        EvWindowRunMode mode, const gchar *search_string) {
  GFile *source_file;

  ev_window->priv->in_reload = FALSE;

  if (ev_window->priv->uri &&
      g_ascii_strcasecmp(ev_window->priv->uri, uri) == 0) {
    ev_window_reload_document(ev_window, dest);
    return;
  }

  if (ev_window->priv->uri &&
      g_ascii_strcasecmp(ev_window->priv->uri, uri) != 0) {
    /* If a different document is already open, add it as a new tab */
    ev_window_add_tab(GTK_NOTEBOOK(ev_window->priv->notebook), uri);
  }

  ev_window_close_dialogs(ev_window);

  /* Só limpamos o job se estivermos abrindo o MESMO URI (reload)
     ou se não houver suporte a abas. */
  if (!ev_window->priv->notebook ||
      (ev_window->priv->uri &&
       g_ascii_strcasecmp(ev_window->priv->uri, uri) == 0)) {
    ev_window_clear_load_job(ev_window);
  } else if (ev_window->priv->load_job) {
    // Para novos URIs em abas, apenas "desprendemos" o job atual sem
    // cancelá-lo
    g_object_unref(ev_window->priv->load_job);
    ev_window->priv->load_job = NULL;
  }

  ev_window_clear_local_uri(ev_window);

  if (ev_window->priv->monitor) {
    g_object_unref(ev_window->priv->monitor);
    ev_window->priv->monitor = NULL;
  }

  ev_window->priv->window_mode = mode;

  if (ev_window->priv->uri)
    g_free(ev_window->priv->uri);
  ev_window->priv->uri = g_strdup(uri);

  if (ev_window->priv->notebook) {
    EvTabData *current_tab =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    if (current_tab && current_tab->scrolled_window) {
      gchar *tab_title;

      g_free(current_tab->uri);
      current_tab->uri = g_strdup(uri);

      tab_title = ev_window_get_tab_title_from_uri(uri);
      ev_window_set_tab_title(GTK_NOTEBOOK(ev_window->priv->notebook),
                              current_tab->scrolled_window, tab_title);
      g_free(tab_title);
    }
  }

  if (ev_window->priv->metadata)
    g_object_unref(ev_window->priv->metadata);
  if (ev_window->priv->bookmarks)
    g_object_unref(ev_window->priv->bookmarks);

  source_file = g_file_new_for_uri(uri);
  if (ev_is_metadata_supported_for_file(source_file)) {
    ev_window->priv->metadata = ev_metadata_new(source_file);
    ev_window_init_metadata_with_default_values(ev_window);
  } else {
    ev_window->priv->metadata = NULL;
  }

  if (ev_window->priv->metadata) {
    ev_window->priv->bookmarks = ev_bookmarks_new(ev_window->priv->metadata);
    ev_sidebar_bookmarks_set_bookmarks(
        EV_SIDEBAR_BOOKMARKS(ev_window->priv->sidebar_bookmarks),
        ev_window->priv->bookmarks);
    g_signal_connect_swapped(ev_window->priv->bookmarks, "changed",
                             G_CALLBACK(ev_window_setup_bookmarks), ev_window);
  } else {
    ev_window->priv->bookmarks = NULL;
  }

  if (ev_window->priv->search_string)
    g_free(ev_window->priv->search_string);
  ev_window->priv->search_string =
      search_string ? g_strdup(search_string) : NULL;

  if (ev_window->priv->dest)
    g_object_unref(ev_window->priv->dest);
  ev_window->priv->dest = dest ? g_object_ref(dest) : NULL;

  setup_size_from_metadata(ev_window);
  setup_model_from_metadata(ev_window);
  ev_window_setup_bookmarks(ev_window);

  ev_window->priv->load_job = ev_job_load_new(uri);
  g_signal_connect(ev_window->priv->load_job, "finished",
                   G_CALLBACK(ev_window_load_job_cb), ev_window);

  if (!g_file_is_native(source_file) && !ev_window->priv->local_uri) {
    ev_window_load_file_remote(ev_window, source_file);
  } else {
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_view_set_loading(
        EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), TRUE);
    g_object_unref(source_file);
    ev_job_scheduler_push_job(ev_window->priv->load_job, EV_JOB_PRIORITY_NONE);
  }
}

void ev_window_open_document(EvWindow *ev_window, EvDocument *document,
                             EvLinkDest *dest, EvWindowRunMode mode,
                             const gchar *search_string) {
  ev_window_close_dialogs(ev_window);
  ev_window_clear_load_job(ev_window);
  ev_window_clear_local_uri(ev_window);

  if (ev_window->priv->monitor) {
    g_object_unref(ev_window->priv->monitor);
    ev_window->priv->monitor = NULL;
  }

  if (ev_window->priv->uri)
    g_free(ev_window->priv->uri);
  ev_window->priv->uri = g_strdup(ev_document_get_uri(document));

  setup_size_from_metadata(ev_window);
  setup_model_from_metadata(ev_window);

  ev_document_model_set_document(ev_window->priv->model, document);

  setup_document_from_metadata(ev_window);
  setup_view_from_metadata(ev_window);

  if (dest && document->iswebdocument == FALSE) {
    EvLink *link;
    EvLinkAction *link_action;

    link_action = ev_link_action_new_dest(dest);
    link = ev_link_new(NULL, link_action);
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(ev_window->priv->notebook));
    ev_view_handle_link(
        EV_VIEW(tab_data ? tab_data->view : ev_window->priv->view), link);
    g_object_unref(link_action);
    g_object_unref(link);
  }

  switch (mode) {
  case EV_WINDOW_MODE_FULLSCREEN:
    ev_window_run_fullscreen(ev_window);
    break;
  case EV_WINDOW_MODE_PRESENTATION:
    ev_window_run_presentation(ev_window);
    break;
  default:
    break;
  }

  if (search_string && EV_IS_DOCUMENT_FIND(document) &&
      mode != EV_WINDOW_MODE_PRESENTATION) {
    ev_window_cmd_edit_find(NULL, ev_window);
    egg_find_bar_set_search_string(EGG_FIND_BAR(ev_window->priv->find_bar),
                                   search_string);
  }

  /* Create a monitor for the document */
  ev_window->priv->monitor = ev_file_monitor_new(ev_window->priv->uri);
  g_signal_connect_swapped(ev_window->priv->monitor, "changed",
                           G_CALLBACK(ev_window_document_changed), ev_window);
}

static void G_GNUC_USED  ev_window_reload_local(EvWindow *ev_window) {
  const gchar *uri;

  uri = ev_window->priv->local_uri ? ev_window->priv->local_uri
                                   : ev_window->priv->uri;
  ev_window->priv->reload_job = ev_job_load_new(uri);
  g_signal_connect(ev_window->priv->reload_job, "finished",
                   G_CALLBACK(ev_window_reload_job_cb), ev_window);
  ev_job_scheduler_push_job(ev_window->priv->reload_job, EV_JOB_PRIORITY_NONE);
}

static void G_GNUC_USED  reload_remote_copy_ready_cb(GFile *remote,
                                        GAsyncResult *async_result,
                                        EvWindow *ev_window) {
  GError *error = NULL;

  ev_window_clear_progress_idle(ev_window);

  g_file_copy_finish(remote, async_result, &error);
  if (error) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      ev_window_error_message(ev_window, error, "%s",
                              _("Failed to reload document."));
    g_error_free(error);
  } else {
    ev_window_reload_local(ev_window);
    // Corrigir: obter uri e info corretamente
    const gchar *uri = ev_window->priv->uri;

    ev_window_add_tab(GTK_NOTEBOOK(ev_window->priv->notebook), uri);

    // O restante da lógica de carregamento do documento deve ser adaptada
    // para O restante da lógica de carregamento do documento deve ser
    // adaptada para usar tab_data->view, tab_data->scrolled_window, etc.
    // ... (adaptação futura: carregar documento na view da aba)
    // info não está declarado aqui, então não faz sentido dar unref
  }
}

// Callback para g_file_query_info_async
static void G_GNUC_USED  query_remote_uri_mtime_cb(GFile *remote, GAsyncResult *async_result,
                                      EvWindow *ev_window) {
  GFileInfo *info = NULL;
  GError *error = NULL;
  info = g_file_query_info_finish(remote, async_result, &error);
  if (error) {
    ev_window_error_message(ev_window, error, "%s",
                            _("Failed to reload document."));
    g_error_free(error);
  } else {
    GTimeVal mtime;
    g_file_info_get_modification_time(info, &mtime);
    ev_window->priv->uri_mtime = mtime.tv_sec;
    g_object_unref(info);
    // Após obter mtime, iniciar cópia remota
    GFile *dest = g_file_new_for_uri(ev_window->priv->local_uri);
    g_file_copy_async(
        remote, dest, G_FILE_COPY_OVERWRITE, G_PRIORITY_DEFAULT, NULL, NULL,
        NULL, (GAsyncReadyCallback)reload_remote_copy_ready_cb, ev_window);
    g_object_unref(dest);
  }
  g_object_unref(remote);
}

static void G_GNUC_USED  ev_window_reload_remote(EvWindow *ev_window) {
  GFile *remote;

  remote = g_file_new_for_uri(ev_window->priv->uri);
  /* Reload the remote uri only if it has changed */
  g_file_query_info_async(
      remote, G_FILE_ATTRIBUTE_TIME_MODIFIED, 0, G_PRIORITY_DEFAULT, NULL,
      (GAsyncReadyCallback)query_remote_uri_mtime_cb, ev_window);
}

static void G_GNUC_USED  ev_window_reload_document(EvWindow *ev_window, EvLinkDest *dest) {
  ev_window_clear_reload_job(ev_window);
  ev_window->priv->in_reload = TRUE;

  if (ev_window->priv->dest)
    g_object_unref(ev_window->priv->dest);
  ev_window->priv->dest = dest ? g_object_ref(dest) : NULL;

  if (ev_window->priv->local_uri) {
    ev_window_reload_remote(ev_window);
  } else {
    ev_window_reload_local(ev_window);
  }
}

static const gchar * G_GNUC_USED get_settings_key_for_directory(GUserDirectory directory) {
  switch (directory) {
  case G_USER_DIRECTORY_PICTURES:
    return GS_LAST_PICTURES_DIRECTORY;
  case G_USER_DIRECTORY_DOCUMENTS:
  default:
    return GS_LAST_DOCUMENT_DIRECTORY;
  }
}

static void G_GNUC_USED  ev_window_file_chooser_restore_folder(EvWindow *window,
                                                  GtkFileChooser *file_chooser,
                                                  const gchar *uri,
                                                  GUserDirectory directory) {
  const gchar *dir;
  gchar *folder_uri;

  g_settings_get(ev_window_ensure_settings(window),
                 get_settings_key_for_directory(directory), "ms", &folder_uri);
  if (folder_uri == NULL && uri != NULL) {
    GFile *file, *parent;

    file = g_file_new_for_uri(uri);
    parent = g_file_get_parent(file);
    g_object_unref(file);
    if (parent) {
      folder_uri = g_file_get_uri(parent);
      g_object_unref(parent);
    }
  }

  if (folder_uri) {
    gtk_file_chooser_set_current_folder_uri(file_chooser, folder_uri);
  } else {
    dir = g_get_user_special_dir(directory);
    gtk_file_chooser_set_current_folder(file_chooser,
                                        dir ? dir : g_get_home_dir());
  }

  g_free(folder_uri);
}

static void G_GNUC_USED  ev_window_file_chooser_save_folder(EvWindow *window,
                                               GtkFileChooser *file_chooser,
                                               GUserDirectory directory) {
  gchar *uri, *folder;

  folder = gtk_file_chooser_get_current_folder(file_chooser);
  if (g_strcmp0(folder, g_get_user_special_dir(directory)) == 0) {
    /* Store 'nothing' if the folder is the default one */
    uri = NULL;
  } else {
    uri = gtk_file_chooser_get_current_folder_uri(file_chooser);
  }
  g_free(folder);

  g_settings_set(ev_window_ensure_settings(window),
                 get_settings_key_for_directory(directory), "ms", uri);
  g_free(uri);
}

static void G_GNUC_USED  file_open_dialog_response_cb(GtkWidget *chooser, gint response_id,
                                         EvWindow *ev_window) {
  if (response_id == GTK_RESPONSE_OK) {
    GSList *uris;

    ev_window_file_chooser_save_folder(ev_window, GTK_FILE_CHOOSER(chooser),
                                       G_USER_DIRECTORY_DOCUMENTS);

    uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(chooser));

    ev_application_open_uri_list(EV_APP, uris,
                                 gtk_window_get_screen(GTK_WINDOW(ev_window)),
                                 gtk_get_current_event_time());

    g_slist_foreach(uris, (GFunc)g_free, NULL);
    g_slist_free(uris);
  }

  gtk_widget_destroy(chooser);
}

static void G_GNUC_USED  ev_window_cmd_file_open(GtkAction *action, EvWindow *window) {
  GtkWidget *chooser;

  chooser = gtk_file_chooser_dialog_new(_("Open Document"), GTK_WINDOW(window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  ev_document_factory_add_filters(chooser, NULL);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), FALSE);

  ev_window_file_chooser_restore_folder(window, GTK_FILE_CHOOSER(chooser), NULL,
                                        G_USER_DIRECTORY_DOCUMENTS);

  g_signal_connect(chooser, "response",
                   G_CALLBACK(file_open_dialog_response_cb), window);

  gtk_widget_show(chooser);
}

static void G_GNUC_USED  ev_window_open_copy_at_dest(EvWindow *window, EvLinkDest *dest) {
  EvWindow *new_window = EV_WINDOW(ev_window_new());

  if (window->priv->metadata)
    new_window->priv->metadata = g_object_ref(window->priv->metadata);
  ev_window_open_document(new_window, window->priv->document, dest, 0, NULL);
  gtk_window_present(GTK_WINDOW(new_window));
}

static void G_GNUC_USED  ev_window_cmd_file_open_copy(GtkAction *action, EvWindow *window) {
  ev_window_open_copy_at_dest(window, NULL);
}


static void G_GNUC_USED  ev_window_add_recent(EvWindow *window, const char *filename) {
  gtk_recent_manager_add_item(window->priv->recent_manager, filename);
}

static gint G_GNUC_USED  compare_recent_items(GtkRecentInfo *a, GtkRecentInfo *b) {
  gboolean has_ev_a, has_ev_b;
  const gchar *xreader = g_get_application_name();

  has_ev_a = gtk_recent_info_has_application(a, xreader);
  has_ev_b = gtk_recent_info_has_application(b, xreader);

  if (has_ev_a && has_ev_b) {
    time_t time_a, time_b;

    time_a = gtk_recent_info_get_modified(a);
    time_b = gtk_recent_info_get_modified(b);

    return (time_b - time_a);
  } else if (has_ev_a) {
    return -1;
  } else if (has_ev_b) {
    return 1;
  }

  return 0;
}

/*
 * Doubles underscore to avoid spurious menu accels.
 */
static gchar * G_GNUC_USED ev_window_get_menu_file_label(gint index, const gchar *filename) {
  GString *str;
  gint length;
  const gchar *p;
  const gchar *end;
  gboolean is_rtl;

  is_rtl = (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL);

  g_return_val_if_fail(filename != NULL, NULL);

  length = strlen(filename);
  str = g_string_sized_new(length + 10);
  g_string_printf(str, "%s_%d.  ", is_rtl ? "\xE2\x80\x8F" : "", index);

  p = filename;
  end = filename + length;

  while (p != end) {
    const gchar *next;
    next = g_utf8_next_char(p);

    switch (*p) {
    case '_':
      g_string_append(str, "__");
      break;
    default:
      g_string_append_len(str, p, next - p);
      break;
    }

    p = next;
  }

  return g_string_free(str, FALSE);
}


static void G_GNUC_USED  ev_window_setup_recent(EvWindow *ev_window) {
  GList *items, *l;
  guint n_items = 0;
  const gchar *xreader = g_get_application_name();

  g_menu_remove_all(ev_window->priv->recent_menu);

  items = gtk_recent_manager_get_items(ev_window->priv->recent_manager);
  items = g_list_sort(items, (GCompareFunc)compare_recent_items);

  for (l = items; l && l->data; l = g_list_next(l)) {
    GtkRecentInfo *info;
    GMenuItem *item;
    gchar *label;

    info = (GtkRecentInfo *)l->data;

    if (!gtk_recent_info_has_application(info, xreader) ||
        (gtk_recent_info_is_local(info) && !gtk_recent_info_exists(info)))
      continue;

    label = ev_window_get_menu_file_label(
        n_items + 1, gtk_recent_info_get_display_name(info));
    item = g_menu_item_new(label, NULL);
    g_menu_item_set_action_and_target_value(
        item, "win.OpenRecent",
        g_variant_new_string(gtk_recent_info_get_uri(info)));
    g_menu_append_item(ev_window->priv->recent_menu, item);
    g_object_unref(item);
    g_free(label);

    if (++n_items == 10)
      break;
  }

  g_list_foreach(items, (GFunc)gtk_recent_info_unref, NULL);
  g_list_free(items);
}

static void G_GNUC_USED  ev_window_setup_favorites(EvWindow *ev_window) {
  GList *infos, *l;
  guint n_items = 0;

  g_menu_remove_all(ev_window->priv->favorites_menu);

  infos = xapp_favorites_get_favorites(ev_window->priv->favorites,
                                       (const gchar **)supported_mimetypes);

  for (l = infos; l && l->data; l = l->next) {
    XAppFavoriteInfo *info;
    GMenuItem *item;
    gchar *label;

    info = (XAppFavoriteInfo *)l->data;

    /* Check existence logic skipped for brevity, relies on GFile */
    /* ... */

    label = ev_window_get_menu_file_label(n_items + 1, info->display_name);
    item = g_menu_item_new(label, NULL);
    g_menu_item_set_action_and_target_value(item, "win.OpenFavorite",
                                            g_variant_new_string(info->uri));
    g_menu_append_item(ev_window->priv->favorites_menu, item);
    g_object_unref(item);
    g_free(label);

    n_items++;
  }

  g_list_free_full(infos, (GDestroyNotify)xapp_favorite_info_free);
}

static void G_GNUC_USED  ev_window_action_open_favorite(GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer user_data) {
  EvWindow *window = EV_WINDOW(user_data);
  const gchar *uri = g_variant_get_string(parameter, NULL);
  ev_window_open_uri(window, uri, NULL, EV_WINDOW_MODE_NORMAL, NULL);
}

static void G_GNUC_USED  ev_window_action_open_recent(GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer user_data) {
  EvWindow *window = EV_WINDOW(user_data);
  const gchar *uri = g_variant_get_string(parameter, NULL);
  ev_window_open_uri(window, uri, NULL, EV_WINDOW_MODE_NORMAL, NULL);
}

static void G_GNUC_USED  ev_window_clear_save_job(EvWindow *ev_window) {
  if (ev_window->priv->save_job != NULL) {
    if (!ev_job_is_finished(ev_window->priv->save_job))
      ev_job_cancel(ev_window->priv->save_job);

    g_signal_handlers_disconnect_by_func(ev_window->priv->save_job,
                                         ev_window_save_job_cb, ev_window);
    g_object_unref(ev_window->priv->save_job);
    ev_window->priv->save_job = NULL;
  }
}

static void G_GNUC_USED  ev_window_save_job_cb(EvJob *job, EvWindow *window) {
  if (ev_job_is_failed(job)) {
    ev_window_error_message(window, job->error,
                            _("The file could not be saved as “%s”."),
                            EV_JOB_SAVE(job)->uri);
  } else {
    ev_window_add_recent(window, EV_JOB_SAVE(job)->uri);

    GFile *dst = g_file_new_for_uri(EV_JOB_SAVE(job)->uri);
    if (dst && ev_is_metadata_supported_for_file(dst))
      ev_metadata_copy_to(window->priv->metadata, dst);

    g_object_unref(dst);
  }

  ev_window_clear_save_job(window);

  if (window->priv->close_after_save) {
    gtk_widget_destroy(GTK_WIDGET(window));
  }
}

static void G_GNUC_USED  ev_window_save_as(EvWindow *ev_window, gchar *uri) {
  /* FIXME: remote copy should be done here rather than in the save job,
   * so that we can track progress and cancel the operation
   */
  ev_window_clear_save_job(ev_window);
  ev_window->priv->save_job =
      ev_job_save_new(ev_window->priv->document, uri, ev_window->priv->uri);
  g_signal_connect(ev_window->priv->save_job, "finished",
                   G_CALLBACK(ev_window_save_job_cb), ev_window);
  /* The priority doesn't matter for this job */
  ev_job_scheduler_push_job(ev_window->priv->save_job, EV_JOB_PRIORITY_NONE);
}

static void G_GNUC_USED  ev_window_save(EvWindow *ev_window) {
  ev_window_save_as(ev_window, ev_window->priv->uri);
}

static gboolean G_GNUC_USED  ev_window_cmd_save_as(GtkAction *action, EvWindow *ev_window) {
  GtkWidget *fc;
  gchar *base_name;
  GFile *file;

  fc = gtk_file_chooser_dialog_new(_("Save a Copy"), GTK_WINDOW(ev_window),
                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                   GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

  ev_document_factory_add_filters(fc, ev_window->priv->document);
  gtk_dialog_set_default_response(GTK_DIALOG(fc), GTK_RESPONSE_OK);

  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(fc), FALSE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fc), TRUE);
  file = g_file_new_for_uri(ev_window->priv->uri);
  base_name = g_file_get_basename(file);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fc), base_name);

  g_object_unref(file);
  g_free(base_name);

  ev_window_file_chooser_restore_folder(ev_window, GTK_FILE_CHOOSER(fc),
                                        ev_window->priv->uri,
                                        G_USER_DIRECTORY_DOCUMENTS);

  if (gtk_dialog_run(GTK_DIALOG(fc)) != GTK_RESPONSE_OK) {
    gtk_widget_destroy(fc);
    return FALSE;
  }

  ev_window_file_chooser_save_folder(ev_window, GTK_FILE_CHOOSER(fc),
                                     G_USER_DIRECTORY_DOCUMENTS);

  gchar *uri;
  uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(fc));
  ev_window_save_as(ev_window, uri);

  g_free(uri);
  gtk_widget_destroy(fc);
  return TRUE;
}

static GKeyFile *get_print_settings_file(void) {
  GKeyFile *print_settings_file;
  gchar *filename;
  GError *error = NULL;

  print_settings_file = g_key_file_new();

  filename = g_build_filename(ev_application_get_dot_dir(EV_APP, FALSE),
                              EV_PRINT_SETTINGS_FILE, NULL);
  if (!g_key_file_load_from_file(
          print_settings_file, filename,
          G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {

    /* Don't warn if the file simply doesn't exist */
    if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
      g_warning("%s", error->message);

    g_error_free(error);
  }

  g_free(filename);

  return print_settings_file;
}

static void G_GNUC_USED  save_print_setting_file(GKeyFile *key_file) {
  gchar *filename;
  gchar *data;
  gsize data_length;
  GError *error = NULL;

  filename = g_build_filename(ev_application_get_dot_dir(EV_APP, TRUE),
                              EV_PRINT_SETTINGS_FILE, NULL);
  data = g_key_file_to_data(key_file, &data_length, NULL);
  g_file_set_contents(filename, data, data_length, &error);
  if (error) {
    g_warning("Failed to save print settings: %s", error->message);
    g_error_free(error);
  }
  g_free(data);
  g_free(filename);
}

static void G_GNUC_USED  ev_window_save_print_settings(EvWindow *window,
                                          GtkPrintSettings *print_settings) {
  GKeyFile *key_file;
  gint i;

  key_file = get_print_settings_file();
  gtk_print_settings_to_key_file(print_settings, key_file,
                                 EV_PRINT_SETTINGS_GROUP);

  /* Always Remove n_copies from global settings */
  g_key_file_remove_key(key_file, EV_PRINT_SETTINGS_GROUP,
                        GTK_PRINT_SETTINGS_N_COPIES, NULL);

  /* Save print settings that are specific to the document */
  for (i = 0; i < G_N_ELEMENTS(document_print_settings); i++) {
    /* Remove it from global settings */
    g_key_file_remove_key(key_file, EV_PRINT_SETTINGS_GROUP,
                          document_print_settings[i], NULL);

    if (window->priv->metadata) {
      const gchar *value;

      value =
          gtk_print_settings_get(print_settings, document_print_settings[i]);
      ev_metadata_set_string(window->priv->metadata, document_print_settings[i],
                             value);
    }
  }

  save_print_setting_file(key_file);
  g_key_file_free(key_file);
}

static void G_GNUC_USED  ev_window_save_print_page_setup(EvWindow *window,
                                            GtkPageSetup *page_setup) {
  GKeyFile *key_file;

  key_file = get_print_settings_file();
  gtk_page_setup_to_key_file(page_setup, key_file, EV_PAGE_SETUP_GROUP);

  /* Do not save document settings in global file */
  g_key_file_remove_key(key_file, EV_PAGE_SETUP_GROUP, "page-setup-orientation",
                        NULL);
  g_key_file_remove_key(key_file, EV_PAGE_SETUP_GROUP, "page-setup-margin-top",
                        NULL);
  g_key_file_remove_key(key_file, EV_PAGE_SETUP_GROUP,
                        "page-setup-margin-bottom", NULL);
  g_key_file_remove_key(key_file, EV_PAGE_SETUP_GROUP, "page-setup-margin-left",
                        NULL);
  g_key_file_remove_key(key_file, EV_PAGE_SETUP_GROUP,
                        "page-setup-margin-right", NULL);

  save_print_setting_file(key_file);
  g_key_file_free(key_file);

  if (!window->priv->metadata)
    return;

  /* Save page setup options that are specific to the document */
  ev_metadata_set_int(window->priv->metadata, "page-setup-orientation",
                      gtk_page_setup_get_orientation(page_setup));
  ev_metadata_set_double(
      window->priv->metadata, "page-setup-margin-top",
      gtk_page_setup_get_top_margin(page_setup, GTK_UNIT_MM));
  ev_metadata_set_double(
      window->priv->metadata, "page-setup-margin-bottom",
      gtk_page_setup_get_bottom_margin(page_setup, GTK_UNIT_MM));
  ev_metadata_set_double(
      window->priv->metadata, "page-setup-margin-left",
      gtk_page_setup_get_left_margin(page_setup, GTK_UNIT_MM));
  ev_metadata_set_double(
      window->priv->metadata, "page-setup-margin-right",
      gtk_page_setup_get_right_margin(page_setup, GTK_UNIT_MM));
}

static void
ev_window_load_print_settings_from_metadata(EvWindow *window,
                                            GtkPrintSettings *print_settings) {
  gint i;

  if (!window->priv->metadata)
    return;

  /* Load print setting that are specific to the document */
  for (i = 0; i < G_N_ELEMENTS(document_print_settings); i++) {
    gchar *value = NULL;

    ev_metadata_get_string(window->priv->metadata, document_print_settings[i],
                           &value);
    gtk_print_settings_set(print_settings, document_print_settings[i], value);
  }
}

static void
ev_window_load_print_page_setup_from_metadata(EvWindow *window,
                                              GtkPageSetup *page_setup) {
  gint int_value;
  gdouble double_value;
  GtkPaperSize *paper_size = gtk_page_setup_get_paper_size(page_setup);

  /* Load page setup options that are specific to the document */
  if (window->priv->metadata &&
      ev_metadata_get_int(window->priv->metadata, "page-setup-orientation",
                          &int_value)) {
    gtk_page_setup_set_orientation(page_setup, int_value);
  } else {
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
  }

  if (window->priv->metadata &&
      ev_metadata_get_double(window->priv->metadata, "page-setup-margin-top",
                             &double_value)) {
    gtk_page_setup_set_top_margin(page_setup, double_value, GTK_UNIT_MM);
  } else {
    gtk_page_setup_set_top_margin(
        page_setup,
        gtk_paper_size_get_default_top_margin(paper_size, GTK_UNIT_MM),
        GTK_UNIT_MM);
  }

  if (window->priv->metadata &&
      ev_metadata_get_double(window->priv->metadata, "page-setup-margin-bottom",
                             &double_value)) {
    gtk_page_setup_set_bottom_margin(page_setup, double_value, GTK_UNIT_MM);
  } else {
    gtk_page_setup_set_bottom_margin(
        page_setup,
        gtk_paper_size_get_default_bottom_margin(paper_size, GTK_UNIT_MM),
        GTK_UNIT_MM);
  }

  if (window->priv->metadata &&
      ev_metadata_get_double(window->priv->metadata, "page-setup-margin-left",
                             &double_value)) {
    gtk_page_setup_set_left_margin(page_setup, double_value, GTK_UNIT_MM);
  } else {
    gtk_page_setup_set_left_margin(
        page_setup,
        gtk_paper_size_get_default_left_margin(paper_size, GTK_UNIT_MM),
        GTK_UNIT_MM);
  }

  if (window->priv->metadata &&
      ev_metadata_get_double(window->priv->metadata, "page-setup-margin-right",
                             &double_value)) {
    gtk_page_setup_set_right_margin(page_setup, double_value, GTK_UNIT_MM);
  } else {
    gtk_page_setup_set_right_margin(
        page_setup,
        gtk_paper_size_get_default_right_margin(paper_size, GTK_UNIT_MM),
        GTK_UNIT_MM);
  }
}

static GtkPrintSettings *get_print_settings(GKeyFile *key_file) {
  GtkPrintSettings *print_settings;

  print_settings = g_key_file_has_group(key_file, EV_PRINT_SETTINGS_GROUP)
                       ? gtk_print_settings_new_from_key_file(
                             key_file, EV_PRINT_SETTINGS_GROUP, NULL)
                       : gtk_print_settings_new();

  return print_settings ? print_settings : gtk_print_settings_new();
}

static GtkPageSetup *get_print_page_setup(GKeyFile *key_file) {
  GtkPageSetup *page_setup;

  page_setup = g_key_file_has_group(key_file, EV_PAGE_SETUP_GROUP)
                   ? gtk_page_setup_new_from_key_file(key_file,
                                                      EV_PAGE_SETUP_GROUP, NULL)
                   : gtk_page_setup_new();

  return page_setup ? page_setup : gtk_page_setup_new();
}

static void G_GNUC_USED  ev_window_print_cancel(EvWindow *ev_window) {
  EvPrintOperation *op;

  if (!ev_window->priv->print_queue)
    return;

  while ((op = g_queue_peek_tail(ev_window->priv->print_queue))) {
    ev_print_operation_cancel(op);
  }
}

static void G_GNUC_USED  ev_window_print_update_pending_jobs_message(EvWindow *ev_window,
                                                        gint n_jobs) {
  gchar *text = NULL;

  if (!EV_IS_PROGRESS_MESSAGE_AREA(ev_window->priv->message_area) ||
      !ev_window->priv->print_queue)
    return;

  if (n_jobs == 0) {
    ev_window_set_message_area(ev_window, NULL);
    return;
  }

  if (n_jobs > 1) {
    text = g_strdup_printf(ngettext("%d pending job in queue",
                                    "%d pending jobs in queue", n_jobs - 1),
                           n_jobs - 1);
  }

  ev_message_area_set_secondary_text(
      EV_MESSAGE_AREA(ev_window->priv->message_area), text);
  g_free(text);
}

static gboolean G_GNUC_USED  destroy_window(GtkWidget *window) {
  gtk_widget_destroy(window);

  return FALSE;
}

static void G_GNUC_USED  ev_window_print_operation_done(EvPrintOperation *op,
                                           GtkPrintOperationResult result,
                                           EvWindow *ev_window) {
  gint n_jobs;

  switch (result) {
  case GTK_PRINT_OPERATION_RESULT_APPLY: {
    GtkPrintSettings *print_settings;

    print_settings = ev_print_operation_get_print_settings(op);
    ev_window_save_print_settings(ev_window, print_settings);

    if (ev_print_operation_get_embed_page_setup(op)) {
      GtkPageSetup *page_setup;

      page_setup = ev_print_operation_get_default_page_setup(op);
      ev_window_save_print_page_setup(ev_window, page_setup);
    }
  }

  break;
  case GTK_PRINT_OPERATION_RESULT_ERROR: {
    GtkWidget *dialog;
    GError *error = NULL;

    ev_print_operation_get_error(op, &error);

    /* The message area is already used by
     * the printing progress, so it's better to
     * use a popup dialog in this case
     */
    dialog = gtk_message_dialog_new(GTK_WINDOW(ev_window),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
                                    _("Failed to print document"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             error->message);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show(dialog);

    g_error_free(error);
  } break;
  case GTK_PRINT_OPERATION_RESULT_CANCEL:
  default:
    break;
  }

  g_queue_remove(ev_window->priv->print_queue, op);
  g_object_unref(op);
  n_jobs = g_queue_get_length(ev_window->priv->print_queue);
  ev_window_print_update_pending_jobs_message(ev_window, n_jobs);

  if (n_jobs == 0 && ev_window->priv->close_after_print)
    g_idle_add((GSourceFunc)destroy_window, ev_window);
}

static void G_GNUC_USED  ev_window_print_progress_response_cb(EvProgressMessageArea *area,
                                                 gint response,
                                                 EvWindow *ev_window) {
  if (response == GTK_RESPONSE_CANCEL) {
    EvPrintOperation *op;

    op = g_queue_peek_tail(ev_window->priv->print_queue);
    ev_print_operation_cancel(op);
  } else {
    gtk_widget_hide(GTK_WIDGET(area));
  }
}

static void G_GNUC_USED  ev_window_print_operation_status_changed(EvPrintOperation *op,
                                                     EvWindow *ev_window) {
  const gchar *status;
  gdouble fraction;

  status = ev_print_operation_get_status(op);
  fraction = ev_print_operation_get_progress(op);

  if (!ev_window->priv->message_area) {
    GtkWidget *area;
    const gchar *job_name;
    gchar *text;

    job_name = ev_print_operation_get_job_name(op);
    text = g_strdup_printf(_("Printing job “%s”"), job_name);

    area = ev_progress_message_area_new(GTK_STOCK_PRINT, text, GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE, GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL, NULL);
    ev_window_print_update_pending_jobs_message(ev_window, 1);
    g_signal_connect(area, "response",
                     G_CALLBACK(ev_window_print_progress_response_cb),
                     ev_window);
    gtk_widget_show(area);
    ev_window_set_message_area(ev_window, area);
    g_free(text);
  }

  ev_progress_message_area_set_status(
      EV_PROGRESS_MESSAGE_AREA(ev_window->priv->message_area), status);
  ev_progress_message_area_set_fraction(
      EV_PROGRESS_MESSAGE_AREA(ev_window->priv->message_area), fraction);
}

static void G_GNUC_USED  ev_window_print_operation_begin_print(EvPrintOperation *op,
                                                  EvWindow *ev_window) {
  if (!ev_window->priv->print_queue)
    ev_window->priv->print_queue = g_queue_new();

  g_queue_push_head(ev_window->priv->print_queue, op);
  ev_window_print_update_pending_jobs_message(
      ev_window, g_queue_get_length(ev_window->priv->print_queue));
}

void ev_window_print_range(EvWindow *ev_window, gint first_page,
                           gint last_page) {
  EvPrintOperation *op;
  GKeyFile *print_settings_file;
  GtkPrintSettings *print_settings;
  GtkPageSetup *print_page_setup;
  gint current_page;
  gint document_last_page;

  g_return_if_fail(EV_IS_WINDOW(ev_window));
  g_return_if_fail(ev_window->priv->document != NULL);

  if (!ev_window->priv->print_queue)
    ev_window->priv->print_queue = g_queue_new();

  op = ev_print_operation_new(ev_window->priv->document);
  if (!op) {
    g_warning("%s", "Printing is not supported for document\n");
    return;
  }

  g_signal_connect(op, "begin_print",
                   G_CALLBACK(ev_window_print_operation_begin_print),
                   (gpointer)ev_window);
  g_signal_connect(op, "status_changed",
                   G_CALLBACK(ev_window_print_operation_status_changed),
                   (gpointer)ev_window);
  g_signal_connect(op, "done", G_CALLBACK(ev_window_print_operation_done),
                   (gpointer)ev_window);

  current_page = ev_document_model_get_page(ev_window->priv->model);
  document_last_page = ev_document_get_n_pages(ev_window->priv->document);

  print_settings_file = get_print_settings_file();

  print_settings = get_print_settings(print_settings_file);
  ev_window_load_print_settings_from_metadata(ev_window, print_settings);

  print_page_setup = get_print_page_setup(print_settings_file);
  ev_window_load_print_page_setup_from_metadata(ev_window, print_page_setup);

  if (first_page != 1 || last_page != document_last_page) {
    GtkPageRange range;

    /* Ranges in GtkPrint are 0 - N */
    range.start = first_page - 1;
    range.end = last_page - 1;

    gtk_print_settings_set_print_pages(print_settings, GTK_PRINT_PAGES_RANGES);
    gtk_print_settings_set_page_ranges(print_settings, &range, 1);
  }

  ev_print_operation_set_job_name(op,
                                  gtk_window_get_title(GTK_WINDOW(ev_window)));
  ev_print_operation_set_current_page(op, current_page);
  ev_print_operation_set_print_settings(op, print_settings);
  ev_print_operation_set_default_page_setup(op, print_page_setup);
  ev_print_operation_set_embed_page_setup(op, TRUE);

  g_object_unref(print_settings);
  g_object_unref(print_page_setup);
  g_key_file_free(print_settings_file);

  ev_print_operation_run(op, GTK_WINDOW(ev_window));
}

static void G_GNUC_USED  ev_window_print(EvWindow *window) {
  ev_window_print_range(window, 1,
                        ev_document_get_n_pages(window->priv->document));
}

static void G_GNUC_USED  ev_window_cmd_file_print(GtkAction *action, EvWindow *ev_window) {
  ev_window_print(ev_window);
}

static void G_GNUC_USED  ev_window_cmd_file_properties(GtkAction *action,
                                          EvWindow *ev_window) {
  if (ev_window->priv->properties == NULL) {
    ev_window->priv->properties = ev_properties_dialog_new();
    ev_properties_dialog_set_document(
        EV_PROPERTIES_DIALOG(ev_window->priv->properties), ev_window->priv->uri,
        ev_window->priv->document);
    g_object_add_weak_pointer(G_OBJECT(ev_window->priv->properties),
                              (gpointer) & (ev_window->priv->properties));
    gtk_window_set_transient_for(GTK_WINDOW(ev_window->priv->properties),
                                 GTK_WINDOW(ev_window));
  }

  ev_document_fc_mutex_lock();
  gtk_widget_show(ev_window->priv->properties);
  ev_document_fc_mutex_unlock();
}

static void G_GNUC_USED  ev_window_cmd_edit_preferences(GtkAction *action,
                                           EvWindow *ev_window) {
  ev_preferences_dialog_show(ev_window);
}

static gboolean G_GNUC_USED  ev_window_check_document_modified(EvWindow *ev_window) {
  EvDocument *document = ev_window->priv->document;
  GtkWidget *dialog;
  GtkWidget *button;
  gchar *text, *markup;
  const gchar *secondary_text;

  if (!document)
    return FALSE;

  if (EV_IS_DOCUMENT_FORMS(document) &&
      ev_document_forms_document_is_modified(EV_DOCUMENT_FORMS(document))) {
    secondary_text =
        _("Document contains form fields that have been filled out. "
          "If you don't save a copy, changes will be permanently lost.");
  } else if (EV_IS_DOCUMENT_ANNOTATIONS(document) &&
             ev_document_annotations_document_is_modified(
                 EV_DOCUMENT_ANNOTATIONS(document))) {
    secondary_text =
        _("Document contains new or modified annotations. "
          "If you don't save a copy, changes will be permanently lost.");
  } else {
    return FALSE;
  }

  text =
      g_markup_printf_escaped(_("Save a copy of document “%s” before closing?"),
                              gtk_window_get_title(GTK_WINDOW(ev_window)));

  dialog = gtk_message_dialog_new(GTK_WINDOW(ev_window), GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, NULL);

  markup = g_strdup_printf("<b>%s</b>", text);
  g_free(text);

  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), markup);
  g_free(markup);

  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           secondary_text);

  button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Save a _Copy"),
                                 GTK_RESPONSE_YES);
  gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Save"), GTK_RESPONSE_ACCEPT);
  gtk_dialog_add_button(GTK_DIALOG(dialog), _("Close _without Saving"),
                        GTK_RESPONSE_NO);
  gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
                        GTK_RESPONSE_CANCEL);

  gtk_style_context_add_class(gtk_widget_get_style_context(button),
                              GTK_STYLE_CLASS_SUGGESTED_ACTION);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);

  int result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(GTK_WIDGET(dialog));

  if (result == GTK_RESPONSE_YES) {
    ev_window->priv->close_after_save = TRUE;
    return !ev_window_cmd_save_as(NULL, ev_window);
  } else if (result == GTK_RESPONSE_NO) {
    ev_window->priv->close_after_save = FALSE;
    return FALSE;
  } else if (result == GTK_RESPONSE_ACCEPT) {
    ev_window->priv->close_after_save = TRUE;
    ev_window_save(ev_window);
    return FALSE;
  }

  return TRUE;
}

static void G_GNUC_USED  print_jobs_confirmation_dialog_response(GtkDialog *dialog,
                                                    gint response,
                                                    EvWindow *ev_window) {
  gtk_widget_destroy(GTK_WIDGET(dialog));

  switch (response) {
  case GTK_RESPONSE_YES:
    if (!ev_window->priv->print_queue ||
        g_queue_is_empty(ev_window->priv->print_queue))
      gtk_widget_destroy(GTK_WIDGET(ev_window));
    else
      ev_window->priv->close_after_print = TRUE;
    break;
  case GTK_RESPONSE_NO:
    ev_window->priv->close_after_print = TRUE;
    if (ev_window->priv->print_queue &&
        !g_queue_is_empty(ev_window->priv->print_queue)) {
      gtk_widget_set_sensitive(GTK_WIDGET(ev_window), FALSE);
      ev_window_print_cancel(ev_window);
    } else {
      gtk_widget_destroy(GTK_WIDGET(ev_window));
    }
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    ev_window->priv->close_after_print = FALSE;
  }
}

static gboolean G_GNUC_USED  ev_window_check_print_queue(EvWindow *ev_window) {
  GtkWidget *dialog;
  GtkWidget *button;
  gchar *text, *markup;
  gint n_print_jobs;

  n_print_jobs = ev_window->priv->print_queue
                     ? g_queue_get_length(ev_window->priv->print_queue)
                     : 0;

  if (n_print_jobs == 0)
    return FALSE;

  dialog = gtk_message_dialog_new(GTK_WINDOW(ev_window), GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, NULL);
  if (n_print_jobs == 1) {
    EvPrintOperation *op;
    const gchar *job_name;

    op = g_queue_peek_tail(ev_window->priv->print_queue);
    job_name = ev_print_operation_get_job_name(op);

    text = g_strdup_printf(
        _("Wait until print job “%s” finishes before closing?"), job_name);
  } else {
    /* TRANS: the singular form is not really used as n_print_jobs > 1
          but some languages distinguish between different plurals forms,
          so the ngettext is needed. */
    text = g_strdup_printf(ngettext("There is %d print job active. "
                                    "Wait until print finishes before closing?",
                                    "There are %d print jobs active. "
                                    "Wait until print finishes before closing?",
                                    n_print_jobs),
                           n_print_jobs);
  }

  markup = g_strdup_printf("<b>%s</b>", text);
  g_free(text);

  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), markup);
  g_free(markup);

  gtk_message_dialog_format_secondary_text(
      GTK_MESSAGE_DIALOG(dialog), "%s",
      _("If you close the window, pending print "
        "jobs will not be printed."));

  gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel _print and Close"),
                        GTK_RESPONSE_NO);
  gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
                        GTK_RESPONSE_CANCEL);
  button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Close _after Printing"),
                                 GTK_RESPONSE_YES);

  gtk_style_context_add_class(gtk_widget_get_style_context(button),
                              GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);

  g_signal_connect(dialog, "response",
                   G_CALLBACK(print_jobs_confirmation_dialog_response),
                   ev_window);
  gtk_widget_show(dialog);

  return TRUE;
}

static gboolean G_GNUC_USED  ev_window_close(EvWindow *ev_window) {
  if (EV_WINDOW_IS_PRESENTATION(ev_window)) {
    gint current_page;

    /* Save current page */
    current_page = ev_view_presentation_get_current_page(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view));
    ev_document_model_set_page(ev_window->priv->model, current_page);
  }

  if (ev_window_check_document_modified(ev_window))
    return FALSE;

  if (ev_window_check_print_queue(ev_window))
    return FALSE;

  return !ev_window->priv->close_after_save;
}

static void G_GNUC_USED  ev_window_cmd_file_close_window(GtkAction *action,
                                            EvWindow *ev_window) {
  if (ev_window_close(ev_window))
    gtk_widget_destroy(GTK_WIDGET(ev_window));
}

static void G_GNUC_USED  ev_window_cmd_file_close_all_windows(GtkAction *action,
                                                 EvWindow *ev_window) {
  GList *l, *windows;

  windows = g_list_copy(gtk_application_get_windows((GtkApplication *)EV_APP));
  for (l = windows; l != NULL; l = l->next) {
    ev_window_cmd_file_close_window(action, (EvWindow *)l->data);
  }
}

static void G_GNUC_USED  ev_window_cmd_focus_page_selector(GtkAction *act,
                                              EvWindow *window) {
  update_chrome_flag(window, EV_CHROME_RAISE_TOOLBAR, TRUE);
  ev_window_set_action_sensitive(window, "ViewToolbar", FALSE);
  update_chrome_visibility(window);

/* action = gtk_action_group_get_action(window->priv->action_group,
                                       PAGE_SELECTOR_ACTION);
  ev_page_action_grab_focus(EV_PAGE_ACTION(action)); */
}

static void G_GNUC_USED  ev_window_cmd_scroll_forward(GtkAction *action, EvWindow *window) {
  /* If the webview is occupying the window */
  if (window->priv->document && window->priv->document->iswebdocument == TRUE)
    return;

  ev_view_scroll(EV_VIEW(window->priv->view), GTK_SCROLL_PAGE_FORWARD, FALSE);
}

static void G_GNUC_USED  ev_window_cmd_scroll_backward(GtkAction *action, EvWindow *window) {
  /* If the webview is occupying the window */
  if (window->priv->document && window->priv->document->iswebdocument == TRUE)
    return;

  ev_view_scroll(EV_VIEW(window->priv->view), GTK_SCROLL_PAGE_BACKWARD, FALSE);
}


static void G_GNUC_USED  ev_window_cmd_edit_select_all(GtkAction *action,
                                          EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  /*
   * If the find bar is open, select all applies to the find field contents.
   * If the zoom level selector is active, select all applies to its entry
   * field.Otherwise it applies to the viewing window's contents.
   */
  if (ev_window->priv->chrome & EV_CHROME_FINDBAR) {
    egg_find_bar_grab_focus(ev_window->priv->find_bar);
  } else if (ev_window->priv->chrome & EV_CHROME_TOOLBAR &&
             ev_toolbar_zoom_action_get_focused(
                 EV_TOOLBAR(ev_window->priv->toolbar))) {
    ev_toolbar_zoom_action_select_all(EV_TOOLBAR(ev_window->priv->toolbar));
  } else if (ev_window->priv->document->iswebdocument == FALSE) {
    ev_view_select_all(EV_VIEW(ev_window->priv->view));
  }
#if ENABLE_EPUB
  else {
    ev_web_view_select_all(EV_WEB_VIEW(ev_window->priv->webview));
  }
#endif
}

static void G_GNUC_USED  ev_window_cmd_edit_find(GtkAction *action, EvWindow *ev_window) {
  if (ev_window->priv->document == NULL ||
      !EV_IS_DOCUMENT_FIND(ev_window->priv->document)) {
    g_error("Find action should be insensitive since document doesn't support "
            "find");
    return;
  }

  if (EV_WINDOW_IS_PRESENTATION(ev_window))
    return;

  update_chrome_flag(ev_window, EV_CHROME_FINDBAR, TRUE);
  update_chrome_visibility(ev_window);
  gtk_widget_grab_focus(ev_window->priv->find_bar);
}

static void G_GNUC_USED  ev_window_cmd_edit_find_next(GtkAction *action,
                                         EvWindow *ev_window) {
  if (EV_WINDOW_IS_PRESENTATION(ev_window))
    return;

  update_chrome_flag(ev_window, EV_CHROME_FINDBAR, TRUE);
  update_chrome_visibility(ev_window);
  gtk_widget_grab_focus(ev_window->priv->find_bar);
  if (ev_window->priv->document->iswebdocument == FALSE) {
    ev_view_find_next(EV_VIEW(ev_window->priv->view));
  }
#if ENABLE_EPUB
  else {
    ev_web_view_find_next(EV_WEB_VIEW(ev_window->priv->webview));
  }
#endif
}

static void G_GNUC_USED  ev_window_cmd_edit_find_previous(GtkAction *action,
                                             EvWindow *ev_window) {
  if (EV_WINDOW_IS_PRESENTATION(ev_window))
    return;

  update_chrome_flag(ev_window, EV_CHROME_FINDBAR, TRUE);
  update_chrome_visibility(ev_window);
  gtk_widget_grab_focus(ev_window->priv->find_bar);
  if (ev_window->priv->document->iswebdocument == FALSE) {
    ev_view_find_previous(EV_VIEW(ev_window->priv->view));
  }
#if ENABLE_EPUB
  else {
    ev_web_view_find_previous(EV_WEB_VIEW(ev_window->priv->webview));
  }
#endif
}

static void G_GNUC_USED  ev_window_cmd_edit_copy(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument) {
    ev_web_view_copy(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_copy(EV_VIEW(ev_window->priv->view));
  }
}

void ev_window_sidebar_position_change_cb(GObject *object,
                                            GParamSpec *pspec,
                                            EvWindow *ev_window) {
  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_int(ev_window->priv->metadata, "sidebar_size",
                        gtk_paned_get_position(GTK_PANED(object)));
}

static void G_GNUC_USED  ev_window_update_fullscreen_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewFullscreen");
  EvDocument *document = window->priv->document;
  gboolean has_pages = FALSE;
  
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_fullscreen(window->priv->model)));
  }

  /* Disable presentation start button, if no document has been loaded */
  if (document) {
    has_pages = ev_document_get_n_pages(document) > 0;
  }
  ev_window_set_action_sensitive(window, "StartPresentation",
                                 has_pages && !(document->iswebdocument));
}

static void G_GNUC_USED  ev_window_run_fullscreen(EvWindow *window) {
  gboolean fullscreen_window = TRUE;

  if (ev_document_model_get_fullscreen(window->priv->model))
    return;

  if (EV_WINDOW_IS_PRESENTATION(window)) {
    ev_window_stop_presentation(window, FALSE);
    fullscreen_window = FALSE;
  }

  ev_document_model_set_fullscreen(window->priv->model, TRUE);
  ev_window_update_fullscreen_action(window);

  /* Don't show the fullscreen toolbar
   */
  update_chrome_flag(window, EV_CHROME_FULLSCREEN_TOOLBAR, FALSE);
  update_chrome_visibility(window);

  if (fullscreen_window)
    gtk_window_fullscreen(GTK_WINDOW(window));
  if (window->priv->view) {
    gtk_widget_grab_focus(window->priv->view);
  }
#if ENABLE_EPUB
  else {
    gtk_widget_grab_focus(window->priv->webview);
  }
#endif
  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_boolean(window->priv->metadata, "fullscreen", TRUE);
}

static void G_GNUC_USED  ev_window_stop_fullscreen(EvWindow *window,
                                      gboolean unfullscreen_window) {
  if (!ev_document_model_get_fullscreen(window->priv->model))
    return;

  ev_document_model_set_fullscreen(window->priv->model, FALSE);
  ev_window_update_fullscreen_action(window);
  update_chrome_flag(window, EV_CHROME_FULLSCREEN_TOOLBAR, FALSE);
  update_chrome_visibility(window);
  if (unfullscreen_window)
    gtk_window_unfullscreen(GTK_WINDOW(window));

  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_boolean(window->priv->metadata, "fullscreen", FALSE);
}


static void G_GNUC_USED  ev_window_update_presentation_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewPresentation");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(EV_WINDOW_IS_PRESENTATION(window)));
  }
}

static void G_GNUC_USED  ev_window_view_presentation_finished(EvWindow *window) {
  ev_window_stop_presentation(window, TRUE);
}

static gboolean G_GNUC_USED  ev_window_view_presentation_focus_in(EvWindow *window) {
  ev_application_screensaver_disable(EV_APP);

  return FALSE;
}

static gboolean G_GNUC_USED  ev_window_view_presentation_focus_out(EvWindow *window) {
  ev_application_screensaver_enable(EV_APP);

  return FALSE;
}

static void G_GNUC_USED  ev_window_run_presentation(EvWindow *window) {
  gboolean fullscreen_window = TRUE;
  guint current_page;
  guint rotation;
  gboolean inverted_colors;
  gboolean rtl;

  if (EV_WINDOW_IS_PRESENTATION(window))
    return;

  if (window->priv->document->iswebdocument) {
    ev_window_warning_message(
        window, _("Presentation mode is not supported for ePub documents"));
    return;
  }
  if (ev_document_model_get_fullscreen(window->priv->model)) {
    ev_window_stop_fullscreen(window, FALSE);
    fullscreen_window = FALSE;
  }

  current_page = ev_document_model_get_page(window->priv->model);
  rotation = ev_document_model_get_rotation(window->priv->model);
  inverted_colors = ev_document_model_get_inverted_colors(window->priv->model);
  rtl = ev_document_model_get_rtl(window->priv->model);
  window->priv->presentation_view = ev_view_presentation_new(
      window->priv->document, current_page, rotation, inverted_colors);
  ev_view_presentation_set_rtl(
      EV_VIEW_PRESENTATION(window->priv->presentation_view), rtl);
  g_signal_connect_swapped(window->priv->presentation_view, "finished",
                           G_CALLBACK(ev_window_view_presentation_finished),
                           window);

  g_signal_connect_swapped(window->priv->presentation_view, "external-link",
                           G_CALLBACK(view_external_link_cb), window);
  g_signal_connect_swapped(window->priv->presentation_view, "focus-in-event",
                           G_CALLBACK(ev_window_view_presentation_focus_in),
                           window);
  g_signal_connect_swapped(window->priv->presentation_view, "focus-out-event",
                           G_CALLBACK(ev_window_view_presentation_focus_out),
                           window);

  gtk_box_pack_start(GTK_BOX(window->priv->main_box),
                     window->priv->presentation_view, TRUE, TRUE, 0);

  gtk_widget_hide(window->priv->hpaned);
  ev_window_update_presentation_action(window);
  update_chrome_visibility(window);

  gtk_widget_grab_focus(window->priv->presentation_view);
  if (fullscreen_window)
    gtk_window_fullscreen(GTK_WINDOW(window));

  gtk_widget_show(window->priv->presentation_view);

  ev_application_screensaver_disable(EV_APP);

  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_boolean(window->priv->metadata, "presentation", TRUE);
}

static void G_GNUC_USED  ev_window_stop_presentation(EvWindow *window,
                                        gboolean unfullscreen_window) {
  guint current_page;
  guint rotation;

  if (!EV_WINDOW_IS_PRESENTATION(window))
    return;

  current_page = ev_view_presentation_get_current_page(
      EV_VIEW_PRESENTATION(window->priv->presentation_view));
  ev_document_model_set_page(window->priv->model, current_page);
  rotation = ev_view_presentation_get_rotation(
      EV_VIEW_PRESENTATION(window->priv->presentation_view));
  ev_document_model_set_rotation(window->priv->model, rotation);

  gtk_container_remove(GTK_CONTAINER(window->priv->main_box),
                       window->priv->presentation_view);
  window->priv->presentation_view = NULL;

  gtk_widget_show(window->priv->hpaned);
  ev_window_update_presentation_action(window);
  update_chrome_visibility(window);
  if (unfullscreen_window)
    gtk_window_unfullscreen(GTK_WINDOW(window));

  if (window->priv->view) {
    gtk_widget_grab_focus(window->priv->view);
  }
#if ENABLE_EPUB
  else {
    gtk_widget_grab_focus(window->priv->webview);
  }
#endif
  ev_application_screensaver_enable(EV_APP);

  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_boolean(window->priv->metadata, "presentation", FALSE);
}

static void G_GNUC_USED  ev_window_setup_gtk_settings(EvWindow *window) {
  GtkSettings *settings;
  GdkScreen *screen;
  gchar *menubar_accel_accel;

  screen = gtk_window_get_screen(GTK_WINDOW(window));
  settings = gtk_settings_get_for_screen(screen);

  g_object_get(settings, "gtk-menu-bar-accel", &menubar_accel_accel, NULL);
  if (menubar_accel_accel != NULL && menubar_accel_accel[0] != '\0') {
    gtk_accelerator_parse(menubar_accel_accel,
                          &window->priv->menubar_accel_keyval,
                          &window->priv->menubar_accel_modifier);
    if (window->priv->menubar_accel_keyval == 0) {
      g_warning("Failed to parse menu bar accelerator '%s'\n",
                menubar_accel_accel);
    }
  } else {
    window->priv->menubar_accel_keyval = 0;
    window->priv->menubar_accel_modifier = 0;
  }

  g_free(menubar_accel_accel);
}

static void G_GNUC_USED  ev_window_update_max_min_scale(EvWindow *window) {
  gdouble dpi;
  gdouble min_width, min_height;
  gdouble width, height;
  gdouble max_scale;
  guint page_cache_mb;
  gint rotation = ev_document_model_get_rotation(window->priv->model);

  if (!window->priv->document)
    return;

  page_cache_mb =
      g_settings_get_uint(window->priv->settings, GS_PAGE_CACHE_SIZE);
  dpi = get_screen_dpi(window) / 72.0;

  ev_document_get_min_page_size(window->priv->document, &min_width,
                                &min_height);
  width = (rotation == 0 || rotation == 180) ? min_width : min_height;
  height = (rotation == 0 || rotation == 180) ? min_height : min_width;
  max_scale =
      sqrt((page_cache_mb * 1024 * 1024) / (width * dpi * 4 * height * dpi));

  ev_document_model_set_min_scale(window->priv->model, MIN_SCALE * dpi);
  ev_document_model_set_max_scale(window->priv->model, max_scale * dpi);
}

static void G_GNUC_USED  ev_window_screen_changed(GtkWidget *widget, GdkScreen *old_screen) {
  EvWindow *window = EV_WINDOW(widget);
  GdkScreen *screen;

  screen = gtk_widget_get_screen(widget);
  if (screen == old_screen)
    return;

  ev_window_setup_gtk_settings(window);
  ev_window_update_max_min_scale(window);

  if (GTK_WIDGET_CLASS(ev_window_parent_class)->screen_changed) {
    GTK_WIDGET_CLASS(ev_window_parent_class)
        ->screen_changed(widget, old_screen);
  }
}

static gboolean G_GNUC_USED  ev_window_state_event(GtkWidget *widget,
                                      GdkEventWindowState *event) {
  EvWindow *window = EV_WINDOW(widget);

  if (GTK_WIDGET_CLASS(ev_window_parent_class)->window_state_event) {
    GTK_WIDGET_CLASS(ev_window_parent_class)->window_state_event(widget, event);
  }

  if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0)
    return FALSE;

  if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
    if (ev_document_model_get_fullscreen(window->priv->model) ||
        EV_WINDOW_IS_PRESENTATION(window))
      return FALSE;

    ev_window_run_fullscreen(window);
  } else {
    if (ev_document_model_get_fullscreen(window->priv->model))
      ev_window_stop_fullscreen(window, FALSE);
    else if (EV_WINDOW_IS_PRESENTATION(window))
      ev_window_stop_presentation(window, FALSE);
  }

  return FALSE;
}

void ev_window_set_page_mode(EvWindow *window,
                            EvWindowPageMode page_mode) {
  GtkWidget *child = NULL;
  GtkWidget *real_child;

  if (window->priv->page_mode == page_mode)
    return;

  window->priv->page_mode = page_mode;

  switch (page_mode) {
  case PAGE_MODE_DOCUMENT:
    if (window->priv->document &&
        window->priv->document->iswebdocument == FALSE) {
      child = window->priv->view;
    }
#if ENABLE_EPUB
    else {
      child = window->priv->webview;
    }
#endif
    break;
  case PAGE_MODE_PASSWORD:
    child = window->priv->password_view;
    break;
  default:
    g_assert_not_reached();
  }

  real_child = gtk_bin_get_child(GTK_BIN(window->priv->scrolled_window));
  if (child != real_child) {
    gtk_container_remove(GTK_CONTAINER(window->priv->scrolled_window),
                         real_child);
    gtk_container_add(GTK_CONTAINER(window->priv->scrolled_window), child);
  }
  ev_window_update_actions(window);
}

static void G_GNUC_USED  ev_window_cmd_edit_rotate_left(GtkAction *action,
                                           EvWindow *ev_window) {
  gint rotation;

  if (EV_WINDOW_IS_PRESENTATION(ev_window)) {
    rotation = ev_view_presentation_get_rotation(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view));
    ev_view_presentation_set_rotation(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view),
        rotation - 90);
  } else {
    rotation = ev_document_model_get_rotation(ev_window->priv->model);

    ev_document_model_set_rotation(ev_window->priv->model, rotation - 90);
  }
}

static void G_GNUC_USED  ev_window_cmd_edit_rotate_right(GtkAction *action,
                                            EvWindow *ev_window) {
  gint rotation;

  if (EV_WINDOW_IS_PRESENTATION(ev_window)) {
    rotation = ev_view_presentation_get_rotation(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view));
    ev_view_presentation_set_rotation(
        EV_VIEW_PRESENTATION(ev_window->priv->presentation_view),
        rotation + 90);
  } else {
    rotation = ev_document_model_get_rotation(ev_window->priv->model);

    ev_document_model_set_rotation(ev_window->priv->model, rotation + 90);
  }
}

static void G_GNUC_USED  ev_window_cmd_edit_save_settings(GtkAction *action,
                                             EvWindow *ev_window) {
  EvWindowPrivate *priv = ev_window->priv;
  EvDocumentModel *model = priv->model;
  GSettings *settings = priv->default_settings;

  g_settings_set_boolean(settings, "continuous",
                         ev_document_model_get_continuous(model));
  g_settings_set_boolean(settings, "dual-page",
                         ev_document_model_get_dual_page(model));
  g_settings_set_boolean(settings, "dual-page-odd-left",
                         ev_document_model_get_dual_page_odd_pages_left(model));
  g_settings_set_boolean(settings, "fullscreen",
                         ev_document_model_get_fullscreen(model));
  g_settings_set_boolean(settings, "inverted-colors",
                         ev_document_model_get_inverted_colors(model));
  EvSizingMode sizing_mode = ev_document_model_get_sizing_mode(model);
  g_settings_set_enum(settings, "sizing-mode", sizing_mode);
  if (sizing_mode == EV_SIZING_FREE) {
    gdouble zoom = ev_document_model_get_scale(model);

    zoom *= 72.0 / get_screen_dpi(ev_window);
    g_settings_set_double(settings, "zoom", zoom);
  }
  g_settings_set_boolean(settings, "show-sidebar",
                         gtk_widget_get_visible(priv->sidebar));
  g_settings_set_int(settings, "sidebar-size",
                     gtk_paned_get_position(GTK_PANED(priv->hpaned)));
  g_settings_set_string(settings, "sidebar-page",
                        ev_window_sidebar_get_current_page_id(ev_window));
  g_settings_apply(settings);
}

static void G_GNUC_USED  ev_window_cmd_view_zoom_in(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_document_model_set_sizing_mode(ev_window->priv->model, EV_SIZING_FREE);
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument) {
    ev_web_view_zoom_in(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_zoom_in(EV_VIEW(ev_window->priv->view));
  }
}

static void G_GNUC_USED  ev_window_cmd_view_zoom_out(GtkAction *action,
                                        EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_document_model_set_sizing_mode(ev_window->priv->model, EV_SIZING_FREE);
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument) {
    ev_web_view_zoom_out(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_zoom_out(EV_VIEW(ev_window->priv->view));
  }
}

static void G_GNUC_USED  ev_window_cmd_view_zoom_reset(GtkAction *action,
                                          EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_document_model_set_sizing_mode(ev_window->priv->model, EV_SIZING_FREE);
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument) {
    ev_web_view_zoom_reset(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_document_model_set_scale(ev_window->priv->model,
                                get_screen_dpi(ev_window) / 72.0);
  }
}

static void G_GNUC_USED  ev_window_cmd_view_zoom(GSimpleAction *action, GVariant *parameter,
                                    gpointer user_data) {
  EvWindow *ev_window = user_data;
  EvWindowPrivate *priv = ev_window_get_instance_private(ev_window);
  gdouble zoom = g_variant_get_double(parameter);

  ev_document_model_set_sizing_mode(priv->model, EV_SIZING_FREE);
  ev_document_model_set_scale(priv->model,
                              zoom * get_screen_dpi(ev_window) / 72.0);
}

static void G_GNUC_USED  ev_window_cmd_go_previous_history(GtkAction *action,
                                              EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));
  ev_history_go_back(ev_window->priv->history);
}

static void G_GNUC_USED  ev_window_cmd_go_next_history(GtkAction *action,
                                          EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));
  ev_history_go_forward(ev_window->priv->history);
}

static void G_GNUC_USED  ev_window_cmd_go_previous_page(GtkAction *action,
                                           EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_previous_page(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_previous_page(EV_VIEW(ev_window->priv->view));
  }
}

static void G_GNUC_USED  ev_window_cmd_go_next_page(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_next_page(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_next_page(EV_VIEW(ev_window->priv->view));
  }
}

static void G_GNUC_USED  ev_window_cmd_go_first_page(GtkAction *action,
                                        EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_document_model_set_page(ev_window->priv->model, 0);
}

static void G_GNUC_USED  ev_window_cmd_go_last_page(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_document_model_set_page(
      ev_window->priv->model,
      ev_document_get_n_pages(ev_window->priv->document) - 1);
}

static void G_GNUC_USED  ev_window_cmd_go_forward(GtkAction *action, EvWindow *ev_window) {
  int n_pages, current_page;

  g_return_if_fail(EV_IS_WINDOW(ev_window));

  n_pages = ev_document_get_n_pages(ev_window->priv->document);
  current_page = ev_document_model_get_page(ev_window->priv->model);

  if (current_page + 10 < n_pages) {
    ev_document_model_set_page(ev_window->priv->model, current_page + 10);
  }
}

static void G_GNUC_USED  ev_window_cmd_go_backward(GtkAction *action, EvWindow *ev_window) {
  int current_page;

  g_return_if_fail(EV_IS_WINDOW(ev_window));

  current_page = ev_document_model_get_page(ev_window->priv->model);

  if (current_page - 10 >= 0) {
    ev_document_model_set_page(ev_window->priv->model, current_page - 10);
  }
}

static void G_GNUC_USED  ev_window_cmd_reader_view(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_toolbar_activate_reader_view(EV_TOOLBAR(ev_window->priv->toolbar));
}

static void G_GNUC_USED  ev_window_cmd_page_view(GtkAction *action, EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  ev_toolbar_activate_page_view(EV_TOOLBAR(ev_window->priv->toolbar));
}

static void G_GNUC_USED  ev_window_cmd_bookmark_activate(GtkAction *action,
                                            EvWindow *window) {
  guint page = ev_bookmark_action_get_page(EV_BOOKMARK_ACTION(action));

  ev_document_model_set_page(window->priv->model, page);
}

static gint G_GNUC_USED  compare_bookmarks(EvBookmark *a, EvBookmark *b) {
  return strcmp(a->title, b->title);
}

void ev_window_setup_bookmarks(EvWindow *window) {
  GList *items, *l;

  if (!window->priv->bookmarks || !window->priv->bookmarks_menu)
    return;

  g_menu_remove_all (window->priv->bookmarks_menu);

  items = ev_bookmarks_get_bookmarks(window->priv->bookmarks);
  items = g_list_sort(items, (GCompareFunc)compare_bookmarks);

  for (l = items; l && l->data; l = g_list_next(l)) {
    EvBookmark *bm = (EvBookmark *)l->data;
    GMenuItem *item;

    item = g_menu_item_new (bm->title, NULL);
    g_menu_item_set_action_and_target (item, "win.BookmarkJump", "i", bm->page);
    g_menu_append_item (window->priv->bookmarks_menu, item);
    g_object_unref (item);
  }

  g_list_free(items);
}

static void G_GNUC_USED  ev_window_cmd_bookmarks_add(GtkAction *action, EvWindow *window) {
  EvBookmark bm;
  gchar *page_label;

  bm.page = ev_document_model_get_page(window->priv->model);
  page_label = ev_document_get_page_label(window->priv->document, bm.page);
  bm.title = g_strdup_printf(_("Page %s"), page_label);
  g_free(page_label);

  /* EvBookmarks takes ownership of bookmark */
  ev_bookmarks_add(window->priv->bookmarks, &bm);
}

static void G_GNUC_USED  ev_window_cmd_view_reload(GtkAction *action, EvWindow *ev_window) {
  ev_window_reload_document(ev_window, NULL);
}

static void G_GNUC_USED  ev_window_cmd_view_expand_window(GtkAction *action,
                                             EvWindow *ev_window) {
  g_return_if_fail(EV_IS_WINDOW(ev_window));

  zoom_control_changed_cb(NULL, EPHY_ZOOM_EXPAND_WINDOW_TO_FIT, ev_window);
}

static void G_GNUC_USED  ev_window_cmd_view_autoscroll(GtkAction *action,
                                          EvWindow *ev_window) {
  EvDocument *document = ev_window->priv->document;
  if (document->iswebdocument == TRUE) {
    return;
  } else {
    ev_view_autoscroll_start(EV_VIEW(ev_window->priv->view));
  }
}

#define EV_HELP "help:xreader"

static void G_GNUC_USED  ev_window_cmd_help_contents(GtkAction *action,
                                        EvWindow *ev_window) {
  ev_window_show_help(ev_window, NULL);
}

void ev_window_show_help(EvWindow *ev_window, const gchar *uri) {
  GError *error = NULL;
  gchar *help_page = EV_HELP;

  if (uri) {
    help_page = g_strdup_printf("%s:%s", EV_HELP, uri);
  }
  gtk_show_uri(gtk_window_get_screen(GTK_WINDOW(ev_window)), help_page,
               gtk_get_current_event_time(), &error);
  if (error) {
    ev_window_error_message(ev_window, error, "%s",
                            _("There was an error displaying help"));
    g_error_free(error);
  }
}

static void G_GNUC_USED  ev_window_cmd_leave_fullscreen(GtkAction *action,
                                           EvWindow *window) {
  ev_window_stop_fullscreen(window, TRUE);
}

static void G_GNUC_USED  ev_window_cmd_start_presentation(GtkAction *action,
                                             EvWindow *window) {
  ev_window_run_presentation(window);
}

static void G_GNUC_USED  ev_window_cmd_escape(GtkAction *action, EvWindow *window) {
  GtkWidget *widget;

  if (window->priv->document && !window->priv->document->iswebdocument &&
      window->priv->view)
    ev_view_autoscroll_stop(EV_VIEW(window->priv->view));

  if (gtk_widget_is_visible(window->priv->find_bar)) {
    update_chrome_flag(window, EV_CHROME_FINDBAR, FALSE);
    update_chrome_visibility(window);
    return;
  }

  widget = gtk_window_get_focus(GTK_WINDOW(window));
  if (widget && gtk_widget_get_ancestor(widget, EGG_TYPE_FIND_BAR)) {
    update_chrome_flag(window, EV_CHROME_FINDBAR, FALSE);
    update_chrome_visibility(window);

    if (window->priv->view)
      gtk_widget_grab_focus(window->priv->view);
#if ENABLE_EPUB
    else
      gtk_widget_grab_focus(window->priv->webview);
#endif
  } else {
    gboolean fullscreen;

    fullscreen = ev_document_model_get_fullscreen(window->priv->model);

    if (fullscreen) {
      ev_window_stop_fullscreen(window, TRUE);
    } else if (EV_WINDOW_IS_PRESENTATION(window)) {
      ev_window_stop_presentation(window, TRUE);
      gtk_widget_grab_focus(window->priv->view);
    } else {
      if (window->priv->view)
        gtk_widget_grab_focus(window->priv->view);
#if ENABLE_EPUB
      else
        gtk_widget_grab_focus(window->priv->webview);
#endif
    }

    if (fullscreen && EV_WINDOW_IS_PRESENTATION(window))
      g_warning("Both fullscreen and presentation set somehow");
  }
}

static void G_GNUC_USED  save_sizing_mode(EvWindow *window) {
  EvSizingMode mode;

  if (!window->priv->metadata || ev_window_is_empty(window))
    return;

  mode = ev_document_model_get_sizing_mode(window->priv->model);
  ev_metadata_set_string(window->priv->metadata, "sizing_mode",
                         sizing_mode_to_string(mode));
}

void ev_window_document_changed_cb(EvDocumentModel *model,
                                          GParamSpec *pspec,
                                          EvWindow *ev_window) {
  ev_window_set_document(ev_window, ev_document_model_get_document(model));
}

void ev_window_sizing_mode_changed_cb(EvDocumentModel *model,
                                           GParamSpec *pspec,
                                           EvWindow *ev_window) {
  EvSizingMode sizing_mode = ev_document_model_get_sizing_mode(model);

  g_object_set(ev_window->priv->scrolled_window, "hscrollbar-policy",
               sizing_mode == EV_SIZING_FREE ? GTK_POLICY_AUTOMATIC
                                             : GTK_POLICY_NEVER,
               "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);

  /* update_sizing_buttons(ev_window); */
  save_sizing_mode(ev_window);
}

void ev_window_zoom_changed_cb(EvDocumentModel *model, GParamSpec *pspec,
                                      EvWindow *ev_window) {
  ev_window_update_actions(ev_window);

  if (!ev_window->priv->metadata)
    return;

  if (ev_document_model_get_sizing_mode(model) == EV_SIZING_FREE &&
      !ev_window_is_empty(ev_window)) {
    gdouble zoom;

    zoom = ev_document_model_get_scale(model);
    zoom *= 72.0 / get_screen_dpi(ev_window);
    ev_metadata_set_double(ev_window->priv->metadata, "zoom", zoom);
  }
}

static void G_GNUC_USED  ev_window_update_continuous_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewContinuous");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_continuous(window->priv->model)));
  }
}

void ev_window_continuous_changed_cb(EvDocumentModel *model,
                                      GParamSpec *pspec,
                                      EvWindow *ev_window) {
  ev_window_update_continuous_action(ev_window);

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_boolean(ev_window->priv->metadata, "continuous",
                            ev_document_model_get_continuous(model));
}

static void G_GNUC_USED  ev_window_rotation_changed_cb(EvDocumentModel *model,
                                          GParamSpec *pspec, EvWindow *window) {
  gint rotation = ev_document_model_get_rotation(model);

  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_int(window->priv->metadata, "rotation", rotation);

  ev_window_update_max_min_scale(window);
  ev_window_refresh_window_thumbnail(window);
}

static void G_GNUC_USED  ev_window_update_inverted_colors_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewInvertedColors");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_inverted_colors(window->priv->model)));
  }
}

void ev_window_inverted_colors_changed_cb(EvDocumentModel *model,
                                                 GParamSpec *pspec,
                                                 EvWindow *window) {
  gboolean inverted_colors = ev_document_model_get_inverted_colors(model);

  ev_window_update_inverted_colors_action(window);

  if (window->priv->metadata && !ev_window_is_empty(window))
    ev_metadata_set_boolean(window->priv->metadata, "inverted-colors",
                            inverted_colors);

  ev_window_refresh_window_thumbnail(window);
}

static void G_GNUC_USED  ev_window_update_dual_page_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewDual");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_dual_page(window->priv->model)));
  }
}

void ev_window_dual_mode_changed_cb(EvDocumentModel *model, GParamSpec *pspec,
                                     EvWindow *ev_window) {
  ev_window_update_dual_page_action(ev_window);

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_boolean(ev_window->priv->metadata, "dual-page",
                            ev_document_model_get_dual_page(model));
}

static void G_GNUC_USED  ev_window_update_dual_page_odd_pages_left_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewDualOddLeft");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_dual_page_odd_pages_left(window->priv->model)));
  }
}

static void G_GNUC_USED  ev_window_update_rtl_action(EvWindow *window) {
  GAction *action = g_action_map_lookup_action(G_ACTION_MAP(window), "ViewRtl");
  if (action) {
    g_simple_action_set_state(G_SIMPLE_ACTION(action),
                              g_variant_new_boolean(ev_document_model_get_rtl(window->priv->model)));
  }
}

void ev_window_dual_mode_odd_pages_left_changed_cb(EvDocumentModel *model,
                                                  GParamSpec *pspec,
                                                  EvWindow *ev_window) {
  ev_window_update_dual_page_odd_pages_left_action(ev_window);

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_boolean(
        ev_window->priv->metadata, "dual-page-odd-left",
        ev_document_model_get_dual_page_odd_pages_left(model));
}

void ev_window_direction_changed_cb(EvDocumentModel *model, GParamSpec *pspec,
                                     EvWindow *ev_window) {
  ev_window_update_rtl_action(ev_window);

  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window))
    ev_metadata_set_boolean(ev_window->priv->metadata, "rtl",
                            ev_document_model_get_rtl(model));
}

static void G_GNUC_USED  ev_window_cmd_help_about(GtkAction *action, EvWindow *ev_window) {
  const char *license[] = {
      N_("Xreader is free software; you can redistribute it and/or modify "
         "it under the terms of the GNU General Public License as published "
         "by "
         "the Free Software Foundation; either version 2 of the License, or "
         "(at your option) any later version.\n"),
      N_("Xreader is distributed in the hope that it will be useful, "
         "but WITHOUT ANY WARRANTY; without even the implied warranty of "
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
         "GNU General Public License for more details.\n"),
      N_("You should have received a copy of the GNU General Public License "
         "along with Xreader; if not, write to the Free Software Foundation, "
         "Inc., "
         "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA\n")};

  char *license_trans;
  license_trans = g_strconcat(_(license[0]), "\n", _(license[1]), "\n",
                              _(license[2]), "\n", NULL);

  gtk_show_about_dialog(GTK_WINDOW(ev_window), "program-name", _("Xreader Pro"),
                        "version", VERSION, "license", license_trans, "website",
                        "http://www.github.com/linuxmint/xreader/",
                        "logo-icon-name", "xreader", "wrap-license", TRUE,
                        NULL);

  g_free(license_trans);

  /* Fix for accessibility visibility */
  GList *toplevels = gtk_window_list_toplevels();
  for (GList *l = toplevels; l != NULL; l = l->next) {
    GtkWidget *w = GTK_WIDGET(l->data);
    if (GTK_IS_ABOUT_DIALOG(w)) {
      gtk_widget_set_name(w, "About Xreader");
      AtkObject *atk_obj = gtk_widget_get_accessible(w);
      if (atk_obj) {
        atk_object_set_name(atk_obj, "About Xreader");
      }
    }
  }
  g_list_free(toplevels);
}

static void G_GNUC_USED  ev_window_toggle_menubar(EvWindow *window, EvMenubarAction action) {
  GtkWidget *menubar;

  menubar = window->priv->menubar;

  gtk_widget_show(menubar);
  return;

  if (EV_WINDOW_IS_PRESENTATION(window))
    return;

  if (action == EV_MENUBAR_TOGGLE)
    action =
        gtk_widget_get_visible(menubar) ? EV_MENUBAR_HIDE : EV_MENUBAR_SHOW;

  if (action == EV_MENUBAR_HIDE) {
    gtk_widget_hide(menubar);
  } else {
    gtk_widget_show(menubar);

    /* When the menu is normally hidden, have an activation of it trigger a
     * key grab. For keyboard users, this is a natural progression, that they
     * will type a mnemonic next to open a menu.  Any loss of focus or click
     * elsewhere will re-hide the menu and cancel focus.
     */
    gtk_widget_grab_focus(menubar);
    gtk_window_set_mnemonics_visible(GTK_WINDOW(window), TRUE);
  }

  return;
}

void ev_window_sidebar_current_page_changed_cb(EvSidebar *ev_sidebar,
                                                      GParamSpec *pspec,
                                                      EvWindow *ev_window) {
  if (ev_window->priv->metadata && !ev_window_is_empty(ev_window)) {
    ev_metadata_set_string(ev_window->priv->metadata, "sidebar_page",
                           ev_window_sidebar_get_current_page_id(ev_window));
  }
}

void ev_window_sidebar_visibility_changed_cb(EvSidebar *ev_sidebar,
                                            GParamSpec *pspec,
                                            EvWindow *ev_window) {
  if (!EV_WINDOW_IS_PRESENTATION(ev_window)) {
    gboolean visible = gtk_widget_get_visible(GTK_WIDGET(ev_sidebar));
    GAction *action;

    action = g_action_map_lookup_action(G_ACTION_MAP(ev_window), "ViewSidebar");
    if (action) {
      g_action_change_state(action, g_variant_new_boolean(visible));
    }

    if (ev_window->priv->metadata)
      ev_metadata_set_boolean(ev_window->priv->metadata, "sidebar_visibility",
                              visible);
    if (!visible)
      gtk_widget_grab_focus(ev_window->priv->view);
  }
}

static void G_GNUC_USED  view_menu_link_popup(EvWindow *ev_window, EvLink *link) {
  gboolean show_external = FALSE;
  // gboolean   show_internal = FALSE;

  if (ev_window->priv->document->iswebdocument == TRUE)
    return;

  if (ev_window->priv->link)
    g_object_unref(ev_window->priv->link);

  if (link)
    ev_window->priv->link = g_object_ref(link);
  else
    ev_window->priv->link = NULL;

  if (ev_window->priv->link) {
    EvLinkAction *ev_action;

    ev_action = ev_link_get_action(link);
    if (ev_action) {
      switch (ev_link_action_get_action_type(ev_action)) {
      case EV_LINK_ACTION_TYPE_GOTO_DEST:
      case EV_LINK_ACTION_TYPE_GOTO_REMOTE:
        //  2021-03-21 WHY WAS THIS BEING SET BUT NEVER USED?
        // show_internal = TRUE;
        break;
      case EV_LINK_ACTION_TYPE_EXTERNAL_URI:
      case EV_LINK_ACTION_TYPE_LAUNCH:
        show_external = TRUE;
        break;
      default:
        break;
      }
    }
  }

  ev_window_set_action_visible(ev_window,
                               "OpenLink", show_external);
  ev_window_set_action_visible(ev_window,
                               "CopyLinkAddress", show_external);
  ev_window_set_action_visible(ev_window,
                               "GoLink", show_external);
  ev_window_set_action_visible(ev_window,
                               "OpenLinkNewWindow", show_external);
}

static void G_GNUC_USED  view_menu_image_popup(EvWindow *ev_window, EvImage *image) {
  gboolean show_image = FALSE;

  if (ev_window->priv->document->iswebdocument == TRUE)
    return;
  if (ev_window->priv->image)
    g_object_unref(ev_window->priv->image);

  if (image)
    ev_window->priv->image = g_object_ref(image);
  else
    ev_window->priv->image = NULL;

  show_image = (ev_window->priv->image != NULL);

  ev_window_set_action_visible(ev_window,
                               "SaveImageAs", show_image);
  ev_window_set_action_visible(ev_window,
                               "CopyImage", show_image);
}

static void G_GNUC_USED  view_menu_annot_popup(EvWindow *ev_window, EvAnnotation *annot) {
  gboolean show_annot = FALSE;
  if (ev_window->priv->document->iswebdocument == TRUE)
    return;
  if (ev_window->priv->annot)
    g_object_unref(ev_window->priv->annot);
  ev_window->priv->annot = (annot) ? g_object_ref(annot) : NULL;

  ev_window_set_action_visible(
      ev_window, "AnnotProperties",
      (annot != NULL && EV_IS_ANNOTATION_MARKUP(annot)));

  ev_window_set_action_visible(
      ev_window, "RemoveAnnotation",
      (annot != NULL && EV_IS_ANNOTATION_MARKUP(annot)));

  if (annot && EV_IS_ANNOTATION_ATTACHMENT(annot)) {
    EvAttachment *attachment;

    attachment = ev_annotation_attachment_get_attachment(
        EV_ANNOTATION_ATTACHMENT(annot));
    if (attachment) {
      show_annot = TRUE;
      if (ev_window->priv->attach_list) {
        g_list_foreach(ev_window->priv->attach_list, (GFunc)g_object_unref,
                       NULL);
        g_list_free(ev_window->priv->attach_list);
        ev_window->priv->attach_list = NULL;
      }
      ev_window->priv->attach_list = g_list_prepend(
          ev_window->priv->attach_list, g_object_ref(attachment));
    }
  }

  ev_window_set_action_visible(ev_window,
                               "OpenAttachment", show_annot);
  ev_window_set_action_visible(ev_window,
                               "SaveAttachmentAs", show_annot);
}

static gboolean G_GNUC_USED  view_menu_popup_cb(EvView *view, GList *items,
                                   EvWindow *ev_window) {
  GList *l;
  gboolean has_link = FALSE;
  gboolean has_image = FALSE;
  gboolean has_annot = FALSE;

  for (l = items; l; l = g_list_next(l)) {
    if (EV_IS_LINK(l->data)) {
      view_menu_link_popup(ev_window, EV_LINK(l->data));
      has_link = TRUE;
    } else if (EV_IS_IMAGE(l->data)) {
      view_menu_image_popup(ev_window, EV_IMAGE(l->data));
      has_image = TRUE;
    } else if (EV_IS_ANNOTATION(l->data)) {
      view_menu_annot_popup(ev_window, EV_ANNOTATION(l->data));
      has_annot = TRUE;
    }
  }

  if (!has_link)
    view_menu_link_popup(ev_window, NULL);
  if (!has_image)
    view_menu_image_popup(ev_window, NULL);
  if (!has_annot)
    view_menu_annot_popup(ev_window, NULL);

  gtk_menu_popup(GTK_MENU(ev_window->priv->view_popup), NULL, NULL, NULL, NULL,
                 3, gtk_get_current_event_time());
  return TRUE;
}

gboolean attachment_bar_menu_popup_cb(EvSidebarAttachments *attachbar,
                                      GList *attach_list,
                                      EvWindow *ev_window) {
  GtkWidget *popup;

  g_assert(attach_list != NULL);

  if (ev_window->priv->attach_list) {
    g_list_foreach(ev_window->priv->attach_list, (GFunc)g_object_unref, NULL);
    g_list_free(ev_window->priv->attach_list);
  }

  ev_window->priv->attach_list = attach_list;

  popup = ev_window->priv->attachment_popup;

  gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL, 3,
                 gtk_get_current_event_time());

  return TRUE;
}

static void G_GNUC_USED  ev_window_update_find_status_message(EvWindow *ev_window) {
  gchar *message;

  if (!ev_window->priv->find_job)
    return;

  if (ev_job_is_finished(ev_window->priv->find_job)) {
    EvJobFind *job_find = EV_JOB_FIND(ev_window->priv->find_job);

    if (ev_job_find_has_results(job_find)) {
      gint n_results;

      n_results = ev_job_find_get_n_results(
          job_find, ev_document_model_get_page(ev_window->priv->model));
      /* TRANS: Sometimes this could be better translated as
         "%d hit(s) on this page".  Therefore this string
         contains plural cases. */
      message = g_strdup_printf(ngettext("%d found on this page (%d total)",
                                         "%d found on this page (%d total)",
                                         n_results),
                                n_results, job_find->total_count);
    } else {
      message = g_strdup(_("Not found"));
    }
  } else {
    gdouble percent;

    percent = ev_job_find_get_progress(EV_JOB_FIND(ev_window->priv->find_job));
    message = g_strdup_printf(_("%3d%% remaining to search"),
                              (gint)((1.0 - percent) * 100));
  }

  egg_find_bar_set_status_text(EGG_FIND_BAR(ev_window->priv->find_bar),
                               message);
  g_free(message);
}

static void G_GNUC_USED  ev_window_find_job_finished_cb(EvJobFind *job,
                                           EvWindow *ev_window) {
  ev_window_update_find_status_message(ev_window);
}

static void G_GNUC_USED  ev_window_find_job_updated_cb(EvJobFind *job, gint page,
                                          EvWindow *ev_window) {
  ev_window_update_actions(ev_window);
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_find_changed(EV_WEB_VIEW(ev_window->priv->webview),
                             job->results, job->text, job->case_sensitive);
  } else
#endif
  {
    ev_view_find_changed(EV_VIEW(ev_window->priv->view),
                         ev_job_find_get_results(job), page);
  }
  ev_window_update_find_status_message(ev_window);
}

static void G_GNUC_USED  ev_window_clear_find_job(EvWindow *ev_window) {
  if (ev_window->priv->find_job != NULL) {
    if (!ev_job_is_finished(ev_window->priv->find_job))
      ev_job_cancel(ev_window->priv->find_job);

    g_signal_handlers_disconnect_by_func(
        ev_window->priv->find_job, ev_window_find_job_finished_cb, ev_window);
    g_signal_handlers_disconnect_by_func(
        ev_window->priv->find_job, ev_window_find_job_updated_cb, ev_window);
    g_object_unref(ev_window->priv->find_job);
    ev_window->priv->find_job = NULL;
  }
}

void find_bar_previous_cb(EggFindBar *find_bar, EvWindow *ev_window) {
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_find_previous(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_find_previous(EV_VIEW(ev_window->priv->view));
  }
}

void find_bar_next_cb(EggFindBar *find_bar, EvWindow *ev_window) {
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_find_next(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_find_next(EV_VIEW(ev_window->priv->view));
  }
}

static void G_GNUC_USED  find_bar_close_cb(EggFindBar *find_bar, EvWindow *ev_window) {
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument == TRUE) {
    ev_web_view_find_cancel(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_find_cancel(EV_VIEW(ev_window->priv->view));
  }
  ev_window_clear_find_job(ev_window);
  update_chrome_flag(ev_window, EV_CHROME_FINDBAR, FALSE);
  update_chrome_visibility(ev_window);
}

static void G_GNUC_USED  ev_window_search_start(EvWindow *ev_window) {
  EggFindBar *find_bar = EGG_FIND_BAR(ev_window->priv->find_bar);
  const char *search_string;

  if (!ev_window->priv->document ||
      !EV_IS_DOCUMENT_FIND(ev_window->priv->document))
    return;

  search_string = egg_find_bar_get_search_string(find_bar);
  gboolean has_string = (search_string && search_string[0]);

  if (has_string && ev_window->priv->find_job != NULL &&
      ev_job_is_finished(ev_window->priv->find_job)) {
    EvJobFind *job_find = EV_JOB_FIND(ev_window->priv->find_job);
    const gchar *searched_string = ev_job_find_get_text(job_find);

    if (!ev_job_find_has_results(job_find) &&
        strstr(search_string, searched_string)) {
      egg_find_bar_set_status_text(find_bar, _("Not found"));
      return;
    }
  }

  ev_window_clear_find_job(ev_window);

  if (has_string) {
    ev_window->priv->find_job = ev_job_find_new(
        ev_window->priv->document,
        ev_document_model_get_page(ev_window->priv->model),
        ev_document_get_n_pages(ev_window->priv->document), search_string,
        egg_find_bar_get_case_sensitive(find_bar));

    g_signal_connect(ev_window->priv->find_job, "finished",
                     G_CALLBACK(ev_window_find_job_finished_cb), ev_window);
    g_signal_connect(ev_window->priv->find_job, "updated",
                     G_CALLBACK(ev_window_find_job_updated_cb), ev_window);
    ev_job_scheduler_push_job(ev_window->priv->find_job, EV_JOB_PRIORITY_NONE);
  } else {
    ev_window_update_actions(ev_window);
    egg_find_bar_set_status_text(find_bar, NULL);
    if (ev_window->priv->document->iswebdocument == FALSE) {
      gtk_widget_queue_draw(GTK_WIDGET(ev_window->priv->view));
    }
  }
}

static void G_GNUC_USED  find_bar_search_changed_cb(EggFindBar *find_bar, GParamSpec *param,
                                       EvWindow *ev_window) {
  /* Either the string or case sensitivity could have changed. */
#if ENABLE_EPUB
  if (ev_window->priv->document->iswebdocument) {
    ev_web_view_find_search_changed(EV_WEB_VIEW(ev_window->priv->webview));
  } else
#endif
  {
    ev_view_find_search_changed(EV_VIEW(ev_window->priv->view));
  }

  ev_window_search_start(ev_window);
}

void find_bar_visibility_changed_cb(EggFindBar *find_bar,
                                           GParamSpec *param,
                                           EvWindow *ev_window) {
  gboolean visible;
  visible = gtk_widget_get_visible(GTK_WIDGET(find_bar));

  if (ev_window->priv->document &&
      EV_IS_DOCUMENT_FIND(ev_window->priv->document)) {

    if (!ev_window->priv->document->iswebdocument) {
      ev_view_find_set_highlight_search(EV_VIEW(ev_window->priv->view),
                                        visible);
    }
#if ENABLE_EPUB
    else {
      ev_web_view_find_search_changed(EV_WEB_VIEW(ev_window->priv->webview));
      ev_web_view_set_handler(EV_WEB_VIEW(ev_window->priv->webview), visible);
    }
#endif
    ev_window_update_actions(ev_window);

    if (visible)
      ev_window_search_start(ev_window);
    else {
      egg_find_bar_set_status_text(EGG_FIND_BAR(ev_window->priv->find_bar),
                                   NULL);
      egg_find_bar_set_search_string(EGG_FIND_BAR(ev_window->priv->find_bar),
                                     NULL);
    }
  }
}

void find_bar_scroll(EggFindBar *find_bar, GtkScrollType scroll,
                            EvWindow *ev_window) {
  if (ev_window->priv->document->iswebdocument == TRUE)
    return;
  ev_view_scroll(EV_VIEW(ev_window->priv->view), scroll, FALSE);
}

static void G_GNUC_USED  zoom_control_changed_cb(EphyZoomAction *action, float zoom,
                                    EvWindow *ev_window) {
  EvSizingMode mode;
  GtkWindow *window;
  gdouble doc_width, doc_height, scale;
  gint old_width, old_height;
  gint new_width, new_height;

  if (zoom == EPHY_ZOOM_EXPAND_WINDOW_TO_FIT) {
    window = GTK_WINDOW(ev_window);

    ev_document_get_max_page_size(ev_window->priv->document, &doc_width,
                                  &doc_height);
    scale = ev_document_model_get_scale(ev_window->priv->model);

    new_width = (gint)(doc_width * scale);
    new_height = (gint)(doc_height * scale);

    /*
     * If the sidebar, menu bar, or tool bars are open,
     * we must account for their sizes in calculating
     * the new expanded window size.
     */

    if (ev_window->priv->chrome & EV_CHROME_SIDEBAR) {
      GtkAllocation alloc;
      gtk_widget_get_allocation(ev_window->priv->sidebar_thumbs, &alloc);
      new_width += alloc.width;
    }
    if (ev_window->priv->chrome & EV_CHROME_TOOLBAR) {
      GtkAllocation alloc;
      gtk_widget_get_allocation(GTK_WIDGET(ev_window->priv->toolbar), &alloc);
      new_height += alloc.height;
    }
    if (ev_window->priv->chrome & EV_CHROME_MENUBAR) {
      GtkAllocation alloc;
      gtk_widget_get_allocation(GTK_WIDGET(ev_window->priv->menubar), &alloc);
      new_height += alloc.height;
    }

    /*
     * Add a little slack
     */
    new_width += 50;
    new_height += 50;

    /*
     * Only resize if the old window isn't already
     * big enough.
     */
    gtk_window_get_size(window, &old_width, &old_height);
    if (!(old_width >= new_width && old_height >= new_height))
      gtk_window_resize(window, new_width, new_height);

    return;
  }

  if (zoom == EPHY_ZOOM_BEST_FIT) {
    mode = EV_SIZING_BEST_FIT;
  } else if (zoom == EPHY_ZOOM_FIT_WIDTH) {
    mode = EV_SIZING_FIT_WIDTH;
  } else {
    mode = EV_SIZING_FREE;
  }

  ev_document_model_set_sizing_mode(ev_window->priv->model, mode);

  if (mode == EV_SIZING_FREE) {
    ev_document_model_set_scale(ev_window->priv->model, zoom);
  }
}

static void G_GNUC_USED 
ev_window_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x,
                             gint y, GtkSelectionData *selection_data,
                             guint info, guint time)

{
  EvWindow *window = EV_WINDOW(widget);
  gchar **uris;
  gint i = 0;
  GSList *uri_list = NULL;
  GtkWidget *source;

  source = gtk_drag_get_source_widget(context);
  if (source && widget == gtk_widget_get_toplevel(source)) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  uris = gtk_selection_data_get_uris(selection_data);
  if (!uris) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  for (i = 0; uris[i]; i++) {
    uri_list = g_slist_prepend(uri_list, (gpointer)uris[i]);
  }

  ev_application_open_uri_list(EV_APP, uri_list,
                               gtk_window_get_screen(GTK_WINDOW(window)), 0);
  gtk_drag_finish(context, TRUE, FALSE, time);

  g_strfreev(uris);
  g_slist_free(uri_list);
}

static void G_GNUC_USED  ev_window_dispose(GObject *object) {
  EvWindow *window = EV_WINDOW(object);
  EvWindowPrivate *priv = window->priv;

#ifdef ENABLE_DBUS
  if (priv->skeleton != NULL) {
    ev_window_emit_closed(window);

    g_dbus_interface_skeleton_unexport(
        G_DBUS_INTERFACE_SKELETON(priv->skeleton));
    g_object_unref(priv->skeleton);
    priv->skeleton = NULL;
    g_free(priv->dbus_object_path);
    priv->dbus_object_path = NULL;
  }
#endif /* ENABLE_DBUS */

  if (priv->bookmarks) {
    g_object_unref(priv->bookmarks);
    priv->bookmarks = NULL;
  }

  if (priv->metadata) {
    g_object_unref(priv->metadata);
    priv->metadata = NULL;
  }

  if (priv->setup_document_idle > 0) {
    g_source_remove(priv->setup_document_idle);
    priv->setup_document_idle = 0;
  }

  if (priv->monitor) {
    g_object_unref(priv->monitor);
    priv->monitor = NULL;
  }

  if (priv->title) {
    ev_window_title_free(priv->title);
    priv->title = NULL;
  }





  if (priv->recent_manager) {
    g_signal_handlers_disconnect_by_func(priv->recent_manager,
                                         ev_window_setup_recent, window);
    priv->recent_manager = NULL;
  }

  if (priv->favorites) {
    g_signal_handlers_disconnect_by_func(priv->favorites,
                                         ev_window_setup_favorites, window);
    priv->favorites = NULL;
  }

  if (priv->settings) {
    g_object_unref(priv->settings);
    priv->settings = NULL;
  }

  if (priv->default_settings) {
    g_settings_apply(priv->default_settings);
    g_object_unref(priv->default_settings);
    priv->default_settings = NULL;
  }


  if (priv->model) {
    g_signal_handlers_disconnect_by_func(priv->model, ev_window_page_changed_cb,
                                         window);
    g_object_unref(priv->model);
    priv->model = NULL;
  }

  if (priv->document) {
    g_object_unref(priv->document);
    priv->document = NULL;
  }

  if (priv->view) {
    g_object_unref(priv->view);
    priv->view = NULL;
  }

  if (priv->password_view) {
    g_object_unref(priv->password_view);
    priv->password_view = NULL;
  }

  if (priv->load_job) {
    ev_window_clear_load_job(window);
  }

  if (priv->reload_job) {
    ev_window_clear_reload_job(window);
  }

  if (priv->save_job) {
    ev_window_clear_save_job(window);
  }

  if (priv->thumbnail_job) {
    ev_window_clear_thumbnail_job(window);
  }

  if (priv->find_job) {
    ev_window_clear_find_job(window);
  }

  if (priv->local_uri) {
    ev_window_clear_local_uri(window);
    priv->local_uri = NULL;
  }

  ev_window_clear_progress_idle(window);
  if (priv->progress_cancellable) {
    g_object_unref(priv->progress_cancellable);
    priv->progress_cancellable = NULL;
  }

  ev_window_close_dialogs(window);

  if (priv->link) {
    g_object_unref(priv->link);
    priv->link = NULL;
  }

  if (priv->image) {
    g_object_unref(priv->image);
    priv->image = NULL;
  }

  if (priv->annot) {
    g_object_unref(priv->annot);
    priv->annot = NULL;
  }

  if (priv->attach_list) {
    g_list_foreach(priv->attach_list, (GFunc)g_object_unref, NULL);
    g_list_free(priv->attach_list);
    priv->attach_list = NULL;
  }

  if (priv->find_bar) {
    g_signal_handlers_disconnect_by_func(window->priv->find_bar,
                                         G_CALLBACK(find_bar_close_cb), window);
    priv->find_bar = NULL;
  }

  if (priv->uri) {
    g_free(priv->uri);
    priv->uri = NULL;
  }

  if (priv->search_string) {
    g_free(priv->search_string);
    priv->search_string = NULL;
  }

  if (priv->dest) {
    g_object_unref(priv->dest);
    priv->dest = NULL;
  }

  if (priv->history) {
    g_object_unref(priv->history);
    priv->history = NULL;
  }

  if (priv->print_queue) {
    g_queue_free(priv->print_queue);
    priv->print_queue = NULL;
  }

  G_OBJECT_CLASS(ev_window_parent_class)->dispose(object);
}

static gboolean G_GNUC_USED  is_alt_key_event(GdkEventKey *event) {
  GdkModifierType nominal_state;
  gboolean state_ok;

  nominal_state = event->state & gtk_accelerator_get_default_mod_mask();

  /* A key press of alt will show just the alt keyval (GDK_KEY_Alt_L/R).  A
   * key release of a single modifier is always modified by itself.  So a
   * valid press state is 0 and a valid release state is GDK_MOD1_MASK (alt
   * modifier).
   */
  state_ok = (event->type == GDK_KEY_PRESS && nominal_state == 0) ||
             (event->type == GDK_KEY_RELEASE && nominal_state == GDK_MOD1_MASK);

  if (state_ok &&
      (event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R)) {
    return TRUE;
  }

  return FALSE;
}

/*
 * GtkWindow catches keybindings for the menu items _before_ passing them to
 * the focused widget. This is unfortunate and means that pressing Ctrl+a,
 * Ctrl+left or Ctrl+right in the search bar ends up selecting text in the
 * EvView or rotating it. Here we override GtkWindow's handler to do the same
 * things that it does, but in the opposite order and then we chain up to the
 * grand parent handler, skipping gtk_window_key_press_event.
 */
static gboolean G_GNUC_USED  ev_window_key_press_event(GtkWidget *widget,
                                          GdkEventKey *event) {
  static gpointer grand_parent_class = NULL;
  EvWindow *window = EV_WINDOW(widget);

  if (grand_parent_class == NULL)
    grand_parent_class = g_type_class_peek_parent(ev_window_parent_class);

  /* An alt key press by itself will always hide the menu if it's visible.  We
   * set a flag to skip the subsequent release, otherwise we'll show the menu
   * again.
   *
   * When alt is pressed and the menu is NOT visible, we flag that on release
   * we'll show the menu.  If any other keys are pressed between alt being
   * pressed and released, we clear that flag, because it was more than likely
   * part of some other shortcut, and otherwise, depending on the order the
   * keys are released, if the alt key is last to be released, we don't want
   * to show the menu, as that was not the original intent.
   */
  if (is_alt_key_event(event)) {
    if (gtk_widget_get_visible(window->priv->menubar)) {
      ev_window_toggle_menubar(window, EV_MENUBAR_HIDE);
      window->priv->menubar_skip_release = TRUE;
    } else {
      window->priv->menubar_show_queued = TRUE;
    }
  } else {
    window->priv->menubar_show_queued = FALSE;
  }

  if (window->priv->view != NULL) {
    /* Handle focus widget key events */
    if (gtk_window_propagate_key_event(GTK_WINDOW(window), event))
      return TRUE;

    // /* Handle mnemonics and accelerators */
    if (gtk_window_activate_key(GTK_WINDOW(window), event))
      return TRUE;
  }

  /* Chain up, invokes binding set on window */
  return GTK_WIDGET_CLASS(grand_parent_class)->key_press_event(widget, event);
}

static gboolean G_GNUC_USED  ev_window_key_release_event(GtkWidget *widget,
                                            GdkEventKey *event) {
  EvWindow *window = EV_WINDOW(widget);
  /* Conditions to show the menu via the alt key is that it must have been
   * pressed and released without any other key events in between, and we must
   * not have hidden the menu on the alt key press event.  Show we check both
   * flags here, for opposing states.
   */

  if (is_alt_key_event(event)) {
    if (!window->priv->menubar_skip_release &&
        window->priv->menubar_show_queued) {
      ev_window_toggle_menubar(window, EV_MENUBAR_SHOW);
    }
  }

  window->priv->menubar_skip_release = FALSE;
  window->priv->menubar_show_queued = FALSE;

  return GTK_WIDGET_CLASS(ev_window_parent_class)
      ->key_release_event(widget, event);
}

static gboolean G_GNUC_USED  ev_window_delete_event(GtkWidget *widget, GdkEventAny *event) {
  return !ev_window_close(EV_WINDOW(widget));
}

static void ev_window_class_init(EvWindowClass *ev_window_class) {
  GObjectClass *g_object_class = G_OBJECT_CLASS(ev_window_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(ev_window_class);

  g_object_class->dispose = ev_window_dispose;

  widget_class->delete_event = ev_window_delete_event;
  widget_class->key_press_event = ev_window_key_press_event;
  widget_class->key_release_event = ev_window_key_release_event;
  widget_class->screen_changed = ev_window_screen_changed;
  widget_class->window_state_event = ev_window_state_event;
  widget_class->drag_data_received = ev_window_drag_data_received;
}



#define G_ACTION_WRAPPER(name, cmd) \
static void G_GNUC_USED  name (GSimpleAction *action, GVariant *parameter, gpointer user_data) { \
    cmd (NULL, EV_WINDOW (user_data)); \
}

G_ACTION_WRAPPER(ev_window_action_file_open, ev_window_cmd_file_open)
G_ACTION_WRAPPER(ev_window_action_file_open_copy, ev_window_cmd_file_open_copy)
G_ACTION_WRAPPER(ev_window_action_file_save_as, ev_window_cmd_save_as)
G_ACTION_WRAPPER(ev_window_action_file_print, ev_window_cmd_file_print)
G_ACTION_WRAPPER(ev_window_action_file_properties, ev_window_cmd_file_properties)
G_ACTION_WRAPPER(ev_window_action_file_close_all_windows, ev_window_cmd_file_close_all_windows)
G_ACTION_WRAPPER(ev_window_action_file_close_window, ev_window_cmd_file_close_window)
G_ACTION_WRAPPER(ev_window_action_edit_copy, ev_window_cmd_edit_copy)
G_ACTION_WRAPPER(ev_window_action_edit_select_all, ev_window_cmd_edit_select_all)
G_ACTION_WRAPPER(ev_window_action_edit_find, ev_window_cmd_edit_find)
G_ACTION_WRAPPER(ev_window_action_edit_find_next, ev_window_cmd_edit_find_next)
G_ACTION_WRAPPER(ev_window_action_edit_find_previous, ev_window_cmd_edit_find_previous)
G_ACTION_WRAPPER(ev_window_action_edit_rotate_left, ev_window_cmd_edit_rotate_left)
G_ACTION_WRAPPER(ev_window_action_edit_rotate_right, ev_window_cmd_edit_rotate_right)
G_ACTION_WRAPPER(ev_window_action_edit_save_settings, ev_window_cmd_edit_save_settings)
G_ACTION_WRAPPER(ev_window_action_edit_preferences, ev_window_cmd_edit_preferences)
G_ACTION_WRAPPER(ev_window_action_view_zoom_reset, ev_window_cmd_view_zoom_reset)
G_ACTION_WRAPPER(ev_window_action_view_zoom_in, ev_window_cmd_view_zoom_in)
G_ACTION_WRAPPER(ev_window_action_view_zoom_out, ev_window_cmd_view_zoom_out)
G_ACTION_WRAPPER(ev_window_action_view_reload, ev_window_cmd_view_reload)
G_ACTION_WRAPPER(ev_window_action_view_expand_window, ev_window_cmd_view_expand_window)
G_ACTION_WRAPPER(ev_window_action_view_autoscroll, ev_window_cmd_view_autoscroll)
G_ACTION_WRAPPER(ev_window_action_go_previous_page, ev_window_cmd_go_previous_page)
G_ACTION_WRAPPER(ev_window_action_go_next_page, ev_window_cmd_go_next_page)
G_ACTION_WRAPPER(ev_window_action_go_first_page, ev_window_cmd_go_first_page)
G_ACTION_WRAPPER(ev_window_action_go_last_page, ev_window_cmd_go_last_page)
G_ACTION_WRAPPER(ev_window_action_go_previous_history, ev_window_cmd_go_previous_history)
G_ACTION_WRAPPER(ev_window_action_go_next_history, ev_window_cmd_go_next_history)
G_ACTION_WRAPPER(ev_window_action_bookmarks_add, ev_window_cmd_bookmarks_add)
G_ACTION_WRAPPER(ev_window_action_help_contents, ev_window_cmd_help_contents)
G_ACTION_WRAPPER(ev_window_action_help_about, ev_window_cmd_help_about)
G_ACTION_WRAPPER(ev_window_action_leave_fullscreen, ev_window_cmd_leave_fullscreen)
G_ACTION_WRAPPER(ev_window_action_start_presentation, ev_window_cmd_start_presentation)


G_ACTION_WRAPPER(ev_window_action_escape, ev_window_cmd_escape)
G_ACTION_WRAPPER(ev_window_action_scroll_forward, ev_window_cmd_scroll_forward)
G_ACTION_WRAPPER(ev_window_action_scroll_backward, ev_window_cmd_scroll_backward)
G_ACTION_WRAPPER(ev_window_action_focus_page_selector, ev_window_cmd_focus_page_selector)
G_ACTION_WRAPPER(ev_window_action_go_backward, ev_window_cmd_go_backward)
G_ACTION_WRAPPER(ev_window_action_go_forward, ev_window_cmd_go_forward)
G_ACTION_WRAPPER(ev_window_action_reader_view, ev_window_cmd_reader_view)
G_ACTION_WRAPPER(ev_window_action_page_view, ev_window_cmd_page_view)
static void G_GNUC_USED  ev_window_action_view_menubar (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    g_settings_set_boolean (window->priv->settings, "show-menubar", active);
}

static void G_GNUC_USED  ev_window_action_view_toolbar (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    g_settings_set_boolean (window->priv->settings, "show-toolbar", active);
}

static void G_GNUC_USED  ev_window_action_view_sidebar (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    if (EV_WINDOW_IS_PRESENTATION (window)) return;
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    update_chrome_flag (window, EV_CHROME_SIDEBAR, active);
    update_chrome_visibility (window);
}

static void G_GNUC_USED  ev_window_action_view_continuous (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    if (window->priv->model)
        ev_document_model_set_continuous (window->priv->model, active);
}

static void G_GNUC_USED  ev_window_action_view_dual (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    if (window->priv->model)
        ev_document_model_set_dual_page (window->priv->model, active);
}

static void G_GNUC_USED  ev_window_action_view_dual_odd_left (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    if (window->priv->model)
        ev_document_model_set_dual_page_odd_pages_left (window->priv->model, active);
}

static void G_GNUC_USED  ev_window_action_view_rtl (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    if (window->priv->model)
        ev_document_model_set_rtl (window->priv->model, active);
}

static void G_GNUC_USED  ev_window_action_view_inverted_colors (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gboolean active = g_variant_get_boolean (state);
    g_simple_action_set_state (action, state);
    if (window->priv->model)
        ev_document_model_set_inverted_colors (window->priv->model, active);
}

static void G_GNUC_USED  ev_window_action_view_fullscreen (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    ev_window_run_fullscreen (window);
}

static void G_GNUC_USED  ev_window_action_view_presentation (GSimpleAction *action, GVariant *state, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    ev_window_run_presentation (window);
}

static void G_GNUC_USED  ev_window_action_view_best_fit (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    if (window->priv->model)
        ev_document_model_set_sizing_mode (window->priv->model, EV_SIZING_BEST_FIT);
}

static void G_GNUC_USED  ev_window_action_view_page_width (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    if (window->priv->model)
        ev_document_model_set_sizing_mode (window->priv->model, EV_SIZING_FIT_WIDTH);
}

static void G_GNUC_USED  ev_window_action_bookmark_jump (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    EvWindow *window = EV_WINDOW (user_data);
    gint page = g_variant_get_int32 (parameter);

    if (window->priv->model)
        ev_document_model_set_page (window->priv->model, page);
}

static void G_GNUC_USED ev_window_action_tab_close_current(GSimpleAction *action,
                                                           GVariant *parameter,
                                                           gpointer user_data) {
    EvWindow *window = EV_WINDOW(user_data);

    if (!window->priv->notebook)
        return;

    ev_window_close_current_tab(GTK_NOTEBOOK(window->priv->notebook));
}

static void G_GNUC_USED ev_window_action_tab_close_left(GSimpleAction *action,
                                                        GVariant *parameter,
                                                        gpointer user_data) {
    EvWindow *window = EV_WINDOW(user_data);
    gint current;

    if (!window->priv->notebook)
        return;

    current = gtk_notebook_get_current_page(GTK_NOTEBOOK(window->priv->notebook));
    if (current < 0)
        return;

    ev_window_close_tabs_to_left(GTK_NOTEBOOK(window->priv->notebook), current);
}

static void G_GNUC_USED ev_window_action_tab_close_right(GSimpleAction *action,
                                                         GVariant *parameter,
                                                         gpointer user_data) {
    EvWindow *window = EV_WINDOW(user_data);
    gint current;

    if (!window->priv->notebook)
        return;

    current = gtk_notebook_get_current_page(GTK_NOTEBOOK(window->priv->notebook));
    if (current < 0)
        return;

    ev_window_close_tabs_to_right(GTK_NOTEBOOK(window->priv->notebook), current);
}

static void G_GNUC_USED ev_window_action_tab_close_others(GSimpleAction *action,
                                                          GVariant *parameter,
                                                          gpointer user_data) {
    EvWindow *window = EV_WINDOW(user_data);

    if (!window->priv->notebook)
        return;

    ev_window_close_all_tabs(GTK_NOTEBOOK(window->priv->notebook));
}

static void G_GNUC_USED ev_window_setup_default_shortcuts(void) {
    GtkApplication *application;
    guint i;
    static const gchar *accel_none[] = { NULL };
    static const gchar *accel_file_open[] = { "<Primary>o", NULL };
    static const gchar *accel_file_open_copy[] = { "<Primary>n", NULL };
    static const gchar *accel_file_save_as[] = { "<Primary>s", NULL };
    static const gchar *accel_file_print[] = { "<Primary>p", NULL };
    static const gchar *accel_file_properties[] = { "<Alt>Return", NULL };
    static const gchar *accel_file_close_all[] = { "<Primary>q", NULL };
    static const gchar *accel_file_close_window[] = { "<Primary>w", NULL };
    static const gchar *accel_edit_copy[] = { "<Primary>c", "<Primary>Insert", NULL };
    static const gchar *accel_edit_select_all[] = { "<Primary>a", NULL };
    static const gchar *accel_edit_find[] = { "<Primary>f", NULL };
    static const gchar *accel_edit_find_next[] = { "<Primary>g", NULL };
    static const gchar *accel_edit_find_previous[] = { "<Primary><Shift>g", NULL };
    static const gchar *accel_edit_rotate_left[] = { "<Primary>Left", NULL };
    static const gchar *accel_edit_rotate_right[] = { "<Primary>Right", NULL };
    static const gchar *accel_edit_save_settings[] = { "<Primary>t", NULL };
    static const gchar *accel_edit_preferences[] = { "<Primary><Shift>p", NULL };
    static const gchar *accel_view_zoom_reset[] = { "<Primary>0", NULL };
    static const gchar *accel_view_zoom_in[] = {
        "<Primary>plus",
        "<Primary>equal",
        "<Primary>KP_Add",
        NULL
    };
    static const gchar *accel_view_zoom_out[] = {
        "<Primary>minus",
        "<Primary>KP_Subtract",
        NULL
    };
    static const gchar *accel_view_reload[] = { "<Primary>r", NULL };
    static const gchar *accel_view_expand_window[] = { "<Primary>e", NULL };
    static const gchar *accel_go_previous_page[] = { "<Primary>Page_Up", NULL };
    static const gchar *accel_go_next_page[] = { "<Primary>Page_Down", NULL };
    static const gchar *accel_go_first_page[] = { "<Primary>Home", NULL };
    static const gchar *accel_go_last_page[] = { "<Primary>End", NULL };
    static const gchar *accel_go_previous_history[] = { "<Primary><Shift>Page_Up", NULL };
    static const gchar *accel_go_next_history[] = { "<Primary><Shift>Page_Down", NULL };
    static const gchar *accel_bookmarks_add[] = { "<Primary>d", NULL };
    static const gchar *accel_help_contents[] = { "F1", NULL };
    static const gchar *accel_view_fullscreen[] = { "F11", NULL };
    static const gchar *accel_view_presentation[] = { "F5", NULL };
    static const gchar *accel_view_toolbar[] = { "<Primary><Shift>t", NULL };
    static const gchar *accel_view_sidebar[] = { "F9", NULL };
    static const gchar *accel_view_continuous[] = { "<Primary><Shift>c", NULL };
    static const gchar *accel_view_dual[] = { "d", NULL };
    static const gchar *accel_view_best_fit[] = { "f", NULL };
    static const gchar *accel_view_page_width[] = { "w", NULL };
    static const gchar *accel_view_inverted_colors[] = { "<Primary>i", NULL };
    static const gchar *accel_escape[] = { "Escape", NULL };
    static const gchar *accel_slash[] = { "slash", NULL };
    static const gchar *accel_f3[] = { "F3", NULL };
    static const gchar *accel_page_down[] = { "Page_Down", NULL };
    static const gchar *accel_page_up[] = { "Page_Up", NULL };
    static const gchar *accel_space[] = { "space", NULL };
    static const gchar *accel_shift_space[] = { "<Shift>space", NULL };
    static const gchar *accel_backspace[] = { "BackSpace", NULL };
    static const gchar *accel_shift_backspace[] = { "<Shift>BackSpace", NULL };
    static const gchar *accel_return[] = { "Return", NULL };
    static const gchar *accel_shift_return[] = { "<Shift>Return", NULL };
    static const gchar *accel_p[] = { "p", NULL };
    static const gchar *accel_n[] = { "n", NULL };
    static const gchar *accel_plus[] = { "plus", NULL };
    static const gchar *accel_equal[] = { "equal", NULL };
    static const gchar *accel_minus[] = { "minus", NULL };
    static const gchar *accel_kp_plus[] = { "KP_Add", NULL };
    static const gchar *accel_kp_minus[] = { "KP_Subtract", NULL };
    static const gchar *accel_focus_page_selector[] = { "<Primary>l", NULL };
    static const gchar *accel_go_backward_fast[] = { "<Shift>Page_Up", NULL };
    static const gchar *accel_go_forward_fast[] = { "<Shift>Page_Down", NULL };
    static const gchar *accel_reader_view[] = { "<Primary>1", NULL };
    static const gchar *accel_page_view[] = { "<Primary>2", NULL };
    static const gchar *accel_tab_close_current[] = { "<Primary>F4", NULL };
    static const gchar *accel_tab_close_left[] = { "<Primary><Alt>Left", NULL };
    static const gchar *accel_tab_close_right[] = { "<Primary><Alt>Right", NULL };
    static const gchar *accel_tab_close_others[] = { "<Primary><Shift>w", NULL };

    static const struct {
        const gchar *action_name;
        const gchar *const *accels;
    } accel_map[] = {
        { "win.FileOpen", accel_file_open },
        { "win.FileOpenCopy", accel_file_open_copy },
        { "win.FileSaveAs", accel_file_save_as },
        { "win.FilePrint", accel_file_print },
        { "win.FileProperties", accel_file_properties },
        { "win.FileCloseAllWindows", accel_file_close_all },
        { "win.FileCloseWindow", accel_file_close_window },
        { "win.EditCopy", accel_edit_copy },
        { "win.EditSelectAll", accel_edit_select_all },
        { "win.EditFind", accel_edit_find },
        { "win.EditFindNext", accel_edit_find_next },
        { "win.EditFindPrevious", accel_edit_find_previous },
        { "win.EditRotateLeft", accel_edit_rotate_left },
        { "win.EditRotateRight", accel_edit_rotate_right },
        { "win.EditSaveSettings", accel_edit_save_settings },
        { "win.EditPreferences", accel_edit_preferences },
        { "win.ViewZoomReset", accel_view_zoom_reset },
        { "win.ViewZoomIn", accel_view_zoom_in },
        { "win.ViewZoomOut", accel_view_zoom_out },
        { "win.ViewReload", accel_view_reload },
        { "win.ViewExpandWindow", accel_view_expand_window },
        { "win.GoPreviousPage", accel_go_previous_page },
        { "win.GoNextPage", accel_go_next_page },
        { "win.GoFirstPage", accel_go_first_page },
        { "win.GoLastPage", accel_go_last_page },
        { "win.GoPreviousHistory", accel_go_previous_history },
        { "win.GoNextHistory", accel_go_next_history },
        { "win.BookmarksAdd", accel_bookmarks_add },
        { "win.HelpContents", accel_help_contents },
        { "win.ViewFullscreen", accel_view_fullscreen },
        { "win.ViewPresentation", accel_view_presentation },
        { "win.ViewToolbar", accel_view_toolbar },
        { "win.ViewSidebar", accel_view_sidebar },
        { "win.ViewContinuous", accel_view_continuous },
        { "win.ViewDual", accel_view_dual },
        { "win.ViewBestFit", accel_view_best_fit },
        { "win.ViewPageWidth", accel_view_page_width },
        { "win.ViewInvertedColors", accel_view_inverted_colors },
        { "win.Escape", accel_escape },
        { "win.Slash", accel_slash },
        { "win.F3", accel_f3 },
        { "win.PageDown", accel_page_down },
        { "win.PageUp", accel_page_up },
        { "win.Space", accel_space },
        { "win.ShiftSpace", accel_shift_space },
        { "win.BackSpace", accel_backspace },
        { "win.ShiftBackSpace", accel_shift_backspace },
        { "win.Return", accel_return },
        { "win.ShiftReturn", accel_shift_return },
        { "win.p", accel_p },
        { "win.n", accel_n },
        { "win.Plus", accel_plus },
        { "win.Equal", accel_equal },
        { "win.Minus", accel_minus },
        { "win.KpPlus", accel_kp_plus },
        { "win.KpMinus", accel_kp_minus },
        { "win.FocusPageSelector", accel_focus_page_selector },
        { "win.GoBackwardFast", accel_go_backward_fast },
        { "win.GoForwardFast", accel_go_forward_fast },
        { "win.ReaderView", accel_reader_view },
        { "win.PageView", accel_page_view },
        { "win.TabCloseCurrent", accel_tab_close_current },
        { "win.TabCloseLeft", accel_tab_close_left },
        { "win.TabCloseRight", accel_tab_close_right },
        { "win.TabCloseOthers", accel_tab_close_others },
        { NULL, accel_none }
    };

    application = GTK_APPLICATION(g_application_get_default());
    if (!application)
        return;

    for (i = 0; accel_map[i].action_name != NULL; i++) {
        gtk_application_set_accels_for_action(application,
                                              accel_map[i].action_name,
                                              accel_map[i].accels);
    }
}

static const GActionEntry win_actions[] = {
    { "FileOpen", ev_window_action_file_open },
    { "FileOpenCopy", ev_window_action_file_open_copy },
    { "FileSaveAs", ev_window_action_file_save_as },
    { "FilePrint", ev_window_action_file_print },
    { "FileProperties", ev_window_action_file_properties },
    { "FileCloseAllWindows", ev_window_action_file_close_all_windows },
    { "FileCloseWindow", ev_window_action_file_close_window },
    { "EditCopy", ev_window_action_edit_copy },
    { "EditSelectAll", ev_window_action_edit_select_all },
    { "EditFind", ev_window_action_edit_find },
    { "EditFindNext", ev_window_action_edit_find_next },
    { "EditFindPrevious", ev_window_action_edit_find_previous },
    { "EditRotateLeft", ev_window_action_edit_rotate_left },
    { "EditRotateRight", ev_window_action_edit_rotate_right },
    { "EditSaveSettings", ev_window_action_edit_save_settings },
    { "EditPreferences", ev_window_action_edit_preferences },
    { "ViewZoomReset", ev_window_action_view_zoom_reset },
    { "ViewZoomIn", ev_window_action_view_zoom_in },
    { "ViewZoomOut", ev_window_action_view_zoom_out },
    { "ViewReload", ev_window_action_view_reload },
    { "ViewExpandWindow", ev_window_action_view_expand_window },
    { "ViewAutoscroll", ev_window_action_view_autoscroll },
    { "GoPreviousPage", ev_window_action_go_previous_page },
    { "GoNextPage", ev_window_action_go_next_page },
    { "GoFirstPage", ev_window_action_go_first_page },
    { "GoLastPage", ev_window_action_go_last_page },
    { "GoPreviousHistory", ev_window_action_go_previous_history },
    { "GoNextHistory", ev_window_action_go_next_history },
    { "BookmarksAdd", ev_window_action_bookmarks_add },
    { "BookmarkJump", ev_window_action_bookmark_jump, "i" },
    { "HelpContents", ev_window_action_help_contents },
    { "HelpAbout", ev_window_action_help_about },
    { "LeaveFullscreen", ev_window_action_leave_fullscreen },
    { "StartPresentation", ev_window_action_start_presentation },
    { "TabCloseCurrent", ev_window_action_tab_close_current },
    { "TabCloseLeft", ev_window_action_tab_close_left },
    { "TabCloseRight", ev_window_action_tab_close_right },
    { "TabCloseOthers", ev_window_action_tab_close_others },
    { "ViewFullscreen", ev_window_action_view_fullscreen },
    { "ViewPresentation", ev_window_action_view_presentation },
    { "ViewBestFit", ev_window_action_view_best_fit },
    { "ViewPageWidth", ev_window_action_view_page_width },
    { "OpenRecent", ev_window_action_open_recent, "s" },
    { "OpenFavorite", ev_window_action_open_favorite, "s" },
    { "ViewToolbar", NULL, NULL, "true", ev_window_action_view_toolbar },
    { "ViewSidebar", NULL, NULL, "true", ev_window_action_view_sidebar },
    { "ViewContinuous", NULL, NULL, "true", ev_window_action_view_continuous },
    { "ViewDual", NULL, NULL, "false", ev_window_action_view_dual },
    { "ViewDualOddLeft", NULL, NULL, "false", ev_window_action_view_dual_odd_left },
    { "ViewRtl", NULL, NULL, "false", ev_window_action_view_rtl },
    { "ViewInvertedColors", NULL, NULL, "false", ev_window_action_view_inverted_colors },
    /* Accelerator Actions */
    { "Escape", ev_window_action_escape },
    { "Slash", ev_window_action_edit_find },
    { "F3", ev_window_action_edit_find_next },
    { "PageDown", ev_window_action_go_next_page },
    { "PageUp", ev_window_action_go_previous_page },
    { "Space", ev_window_action_scroll_forward },
    { "ShiftSpace", ev_window_action_scroll_backward },
    { "BackSpace", ev_window_action_scroll_backward },
    { "ShiftBackSpace", ev_window_action_scroll_forward },
    { "Return", ev_window_action_scroll_forward },
    { "ShiftReturn", ev_window_action_scroll_backward },
    { "p", ev_window_action_go_previous_page },
    { "n", ev_window_action_go_next_page },
    { "Plus", ev_window_action_view_zoom_in },
    { "Equal", ev_window_action_view_zoom_in },
    { "Minus", ev_window_action_view_zoom_out },
    { "KpPlus", ev_window_action_view_zoom_in },
    { "KpMinus", ev_window_action_view_zoom_out },
    { "FocusPageSelector", ev_window_action_focus_page_selector },
    { "GoBackwardFast", ev_window_action_go_backward },
    { "GoForwardFast", ev_window_action_go_forward },
    { "ReaderView", ev_window_action_reader_view },
    { "PageView", ev_window_action_page_view },
    /* Toggle actions */
    { "ViewMenubar", NULL, NULL, "true", ev_window_action_view_menubar },
    { "ViewToolbar", NULL, NULL, "true", ev_window_action_view_toolbar },
    { "ViewSidebar", NULL, NULL, "true", ev_window_action_view_sidebar },
    { "ViewContinuous", NULL, NULL, "true", ev_window_action_view_continuous },
    { "ViewDual", NULL, NULL, "false", ev_window_action_view_dual },
    { "ViewDualOddLeft", NULL, NULL, "false", ev_window_action_view_dual_odd_left },
    { "ViewRtl", NULL, NULL, "false", ev_window_action_view_rtl },
    { "ViewFullscreen", NULL, NULL, "false", ev_window_action_view_fullscreen },
    { "ViewPresentation", NULL, NULL, "false", ev_window_action_view_presentation },
    /* { "ViewBestFit", NULL, NULL, "false", ev_window_action_toggle }, */
    /* { "ViewPageWidth", NULL, NULL, "false", ev_window_action_toggle }, */
    { "ViewInvertedColors", NULL, NULL, "false", ev_window_action_view_inverted_colors },
    { "zoom", ev_window_cmd_view_zoom, "d" },
};



void sidebar_links_link_activated_cb(EvSidebarLinks *sidebar_links,
                                            EvLink *link, EvWindow *window) {
  if (window->priv->document->iswebdocument == FALSE) {
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(window->priv->notebook));
    ev_view_handle_link(EV_VIEW(tab_data ? tab_data->view : window->priv->view),
                        link);
  }
#if ENABLE_EPUB
  else {
    ev_web_view_handle_link(EV_WEB_VIEW(window->priv->webview), link);
  }
#endif
}

void G_GNUC_USED  history_changed_cb(EvHistory *history, EvWindow *window) {
  ev_window_update_actions(window);
}

void G_GNUC_USED  activate_link_cb(GObject *object, EvLink *link, EvWindow *window) {
  if (window->priv->view) {
    EvTabData *tab_data =
        ev_window_get_current_tab(GTK_NOTEBOOK(window->priv->notebook));
    ev_view_handle_link(EV_VIEW(tab_data ? tab_data->view : window->priv->view),
                        link);
    gtk_widget_grab_focus(window->priv->view);
  }
#if ENABLE_EPUB
  else {
    ev_web_view_handle_link(EV_WEB_VIEW(window->priv->webview), link);
    gtk_widget_grab_focus(window->priv->webview);
  }
#endif
}

void sidebar_layers_visibility_changed(EvSidebarLayers *layers,
                                              EvWindow *window) {
  if (window->priv->document->iswebdocument == FALSE) {
    ev_view_reload(EV_VIEW(window->priv->view));
  }
#if ENABLE_EPUB
  else {
    ev_web_view_reload(EV_WEB_VIEW(window->priv->webview));
  }
#endif
}

void
sidebar_thumbnails_size_changed(EvSidebarThumbnails *sidebar_thumbnails,
                                gint new_size, EvWindow *window) {
  if (window->priv->metadata)
    ev_metadata_set_int(window->priv->metadata, "thumbnails_size", new_size);
}

void
sidebar_annots_annot_activated_cb(EvSidebarAnnotations *sidebar_annots,
                                  EvMapping *annot_mapping, EvWindow *window) {
  if (window->priv->document->iswebdocument == TRUE)
    return;
  ev_view_focus_annotation(EV_VIEW(window->priv->view), annot_mapping);
}

void ev_window_begin_add_annot(EvSidebarAnnotations *sidebar_annots,
                              EvAnnotationType annot_type, EvWindow *window) {
  if (window->priv->document->iswebdocument == TRUE)
    return;
  ev_view_begin_add_annotation(EV_VIEW(window->priv->view), annot_type);
}

void G_GNUC_USED  view_annot_added(EvView *view, EvAnnotation *annot,
                             EvWindow *window) {
  EvSidebarAnnotations *sidebar =
      EV_SIDEBAR_ANNOTATIONS(window->priv->sidebar_annots);
  EvAnnotationsToolbar *toolbar = ev_sidebar_annotations_get_toolbar(sidebar);

  ev_sidebar_annotations_annot_added(sidebar, annot);
  ev_annotations_toolbar_add_annot_finished(toolbar);
}

void G_GNUC_USED  view_annot_removed(EvView *view, EvAnnotation *annot,
                               EvWindow *window) {
  ev_sidebar_annotations_annot_removed(
      EV_SIDEBAR_ANNOTATIONS(window->priv->sidebar_annots), annot);
}

void ev_window_cancel_add_annot(EvSidebarAnnotations *sidebar_annots,
                               EvWindow *window) {
  if (window->priv->document->iswebdocument == TRUE)
    return;
  ev_view_cancel_add_annotation(EV_VIEW(window->priv->view));
}

void
sidebar_bookmarks_add_bookmark(EvSidebarBookmarks *sidebar_bookmarks,
                               EvWindow *window) {
  ev_window_cmd_bookmarks_add(NULL, window);
}

void sidebar_widget_model_set(EvSidebarLinks *ev_sidebar_links,
                                     GParamSpec *pspec, EvWindow *ev_window) {
  /* FIXME: Port to GAction / EvPageAction replacement */
  /*
  GtkTreeModel *model;
  GtkAction *action;

  g_object_get(G_OBJECT(ev_sidebar_links), "model", &model, NULL);

  action = gtk_action_group_get_action(ev_window->priv->action_group,
                                       PAGE_SELECTOR_ACTION);
  ev_page_action_set_links_model(EV_PAGE_ACTION(action), model);
  g_object_unref(model);
  */
}

gboolean G_GNUC_USED  view_actions_focus_in_cb(GtkWidget *widget,
                                         GdkEventFocus *event,
                                         EvWindow *window) {
  update_chrome_flag(window, EV_CHROME_RAISE_TOOLBAR, FALSE);
  ev_window_set_action_sensitive(window, "ViewToolbar", TRUE);

  ev_window_set_action_sensitive(window, "ViewToolbar", TRUE);

  /* ev_window_set_view_accels_sensitivity(window, TRUE); */

  update_chrome_visibility(window);

  return FALSE;
}

gboolean G_GNUC_USED  view_actions_focus_out_cb(GtkWidget *widget,
                                          GdkEventFocus *event,
                                          EvWindow *window) {
  /* ev_window_set_view_accels_sensitivity(window, FALSE); */

  return FALSE;
}

static void G_GNUC_USED  sidebar_page_main_widget_update_cb(GObject *ev_sidebar_page,
                                               GParamSpec *pspec,
                                               EvWindow *ev_window) {
  GtkWidget *widget;

  g_object_get(ev_sidebar_page, "main_widget", &widget, NULL);

  if (widget != NULL) {
    g_signal_connect_object(widget, "focus_in_event",
                            G_CALLBACK(view_actions_focus_in_cb), ev_window, 0);
    g_signal_connect_object(widget, "focus_out_event",
                            G_CALLBACK(view_actions_focus_out_cb), ev_window,
                            0);
    g_object_unref(widget);
  }
}

static gboolean G_GNUC_USED  window_state_event_cb(EvWindow *window,
                                      GdkEventWindowState *event,
                                      gpointer dummy) {
  if (!(event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)) {
    gboolean maximized;

    maximized = event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED;
    if (window->priv->metadata && !ev_window_is_empty(window))
      ev_metadata_set_boolean(window->priv->metadata, "window_maximized",
                              maximized);
  }

  return FALSE;
}

#define EPSILON 0.0000001
static void G_GNUC_USED  maybe_update_zoom(EvWindow *ev_window) {
  gdouble md_zoom, doc_zoom, mon_dpi, new_dpi;
  EvSizingMode mode;

  if (!ev_window->priv->metadata)
    return;
  if (!ev_window->priv->model)
    return;

  mode = ev_document_model_get_sizing_mode(ev_window->priv->model);
  if (mode != EV_SIZING_FREE)
    return;

  ev_metadata_get_double(ev_window->priv->metadata, "zoom", &md_zoom);
  doc_zoom = ev_document_model_get_scale(ev_window->priv->model);

  mon_dpi = (doc_zoom / md_zoom) * 72.0;
  new_dpi = get_screen_dpi(ev_window);

  if (ABS(mon_dpi - new_dpi) > EPSILON) {
    ev_document_model_set_scale(ev_window->priv->model,
                                (md_zoom * new_dpi) / 72.0);
  }
}

static gboolean G_GNUC_USED  window_configure_event_cb(EvWindow *window,
                                          GdkEventConfigure *event,
                                          gpointer dummy) {
  GdkWindowState state;
  gdouble document_width, document_height;

  if (!window->priv->metadata)
    return FALSE;

  state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window)));

  if (!(state & GDK_WINDOW_STATE_FULLSCREEN)) {
    if (window->priv->document) {
      ev_document_get_max_page_size(window->priv->document, &document_width,
                                    &document_height);
      g_settings_set(window->priv->default_settings, "window-ratio", "(dd)",
                     (double)event->width / document_width,
                     (double)event->height / document_height);

      ev_metadata_set_int(window->priv->metadata, "window_x", event->x);
      ev_metadata_set_int(window->priv->metadata, "window_y", event->y);
      ev_metadata_set_int(window->priv->metadata, "window_width", event->width);
      ev_metadata_set_int(window->priv->metadata, "window_height",
                          event->height);
    }
  }

  maybe_update_zoom(window);

  return FALSE;
}

static void G_GNUC_USED  launch_action(EvWindow *window, EvLinkAction *action) {
  const char *filename = ev_link_action_get_filename(action);
  GAppInfo *app_info;
  GFile *file;
  GList file_list = {NULL};
  GdkAppLaunchContext *context;
  GdkScreen *screen;
  GError *error = NULL;

  if (filename == NULL)
    return;

  if (g_path_is_absolute(filename)) {
    file = g_file_new_for_path(filename);
  } else {
    GFile *base_file;
    gchar *dir;

    dir = g_path_get_dirname(window->priv->uri);
    base_file = g_file_new_for_uri(dir);
    g_free(dir);

    file = g_file_resolve_relative_path(base_file, filename);
    g_object_unref(base_file);
  }

  app_info = g_file_query_default_handler(file, NULL, &error);
  if (!app_info) {
    ev_window_error_message(window, error, "%s",
                            _("Unable to launch external application."));
    g_object_unref(file);
    g_error_free(error);

    return;
  }

  screen = gtk_window_get_screen(GTK_WINDOW(window));
  context = gdk_display_get_app_launch_context(gdk_screen_get_display(screen));
  gdk_app_launch_context_set_screen(context, screen);
  gdk_app_launch_context_set_timestamp(context, gtk_get_current_event_time());

  file_list.data = file;
  if (!g_app_info_launch(app_info, &file_list, G_APP_LAUNCH_CONTEXT(context),
                         &error)) {
    ev_window_error_message(window, error, "%s",
                            _("Unable to launch external application."));
    g_error_free(error);
  }

  g_object_unref(app_info);
  g_object_unref(file);
  /* FIXMEchpe: unref launch context? */

  /* According to the PDF spec filename can be an executable. I'm not sure
     allowing to launch executables is a good idea though. -- marco */
}

static void G_GNUC_USED  launch_external_uri(EvWindow *window, EvLinkAction *action) {
  const gchar *uri = ev_link_action_get_uri(action);
  GError *error = NULL;
  gboolean ret;
  GdkAppLaunchContext *context;
  GdkScreen *screen;

  screen = gtk_window_get_screen(GTK_WINDOW(window));
  context = gdk_display_get_app_launch_context(gdk_screen_get_display(screen));
  gdk_app_launch_context_set_screen(context, screen);
  gdk_app_launch_context_set_timestamp(context, gtk_get_current_event_time());

  if (!g_strstr_len(uri, strlen(uri), "://") &&
      !g_str_has_prefix(uri, "mailto:")) {
    gchar *new_uri;

    /* Not a valid uri, assume http if it starts with www */
    if (g_str_has_prefix(uri, "www.")) {
      new_uri = g_strdup_printf("http://%s", uri);
    } else {
      GFile *file, *parent;

      file = g_file_new_for_uri(window->priv->uri);
      parent = g_file_get_parent(file);
      g_object_unref(file);
      if (parent) {
        gchar *parent_uri = g_file_get_uri(parent);

        new_uri = g_build_filename(parent_uri, uri, NULL);
        g_free(parent_uri);
        g_object_unref(parent);
      } else {
        new_uri = g_strdup_printf("file:///%s", uri);
      }
    }
    ret = g_app_info_launch_default_for_uri(
        new_uri, G_APP_LAUNCH_CONTEXT(context), &error);
    g_free(new_uri);
  } else {
    ret = g_app_info_launch_default_for_uri(uri, G_APP_LAUNCH_CONTEXT(context),
                                            &error);
  }

  if (ret == FALSE) {
    ev_window_error_message(window, error, "%s",
                            _("Unable to open external link"));
    g_error_free(error);
  }

  /* FIXMEchpe: unref launch context? */
}

static void G_GNUC_USED  open_remote_link(EvWindow *window, EvLinkAction *action) {
  gchar *uri;
  gchar *dir;

  dir = g_path_get_dirname(window->priv->uri);

  uri = g_build_filename(dir, ev_link_action_get_filename(action), NULL);
  g_free(dir);

  ev_application_open_uri_at_dest(
      EV_APP, uri, gtk_window_get_screen(GTK_WINDOW(window)),
      ev_link_action_get_dest(action), 0, NULL, gtk_get_current_event_time());

  g_free(uri);
}

static void G_GNUC_USED  do_action_named(EvWindow *window, EvLinkAction *action) {
  const gchar *name = ev_link_action_get_name(action);

  if (g_ascii_strcasecmp(name, "FirstPage") == 0) {
    ev_window_cmd_go_first_page(NULL, window);
  } else if (g_ascii_strcasecmp(name, "PrevPage") == 0) {
    ev_window_cmd_go_previous_page(NULL, window);
  } else if (g_ascii_strcasecmp(name, "NextPage") == 0) {
    ev_window_cmd_go_next_page(NULL, window);
  } else if (g_ascii_strcasecmp(name, "LastPage") == 0) {
    ev_window_cmd_go_last_page(NULL, window);
  } else if (g_ascii_strcasecmp(name, "GoToPage") == 0) {
    ev_window_cmd_focus_page_selector(NULL, window);
  } else if (g_ascii_strcasecmp(name, "Find") == 0) {
    ev_window_cmd_edit_find(NULL, window);
  } else if (g_ascii_strcasecmp(name, "Close") == 0) {
    ev_window_cmd_file_close_window(NULL, window);
  } else if (g_ascii_strcasecmp(name, "Print") == 0) {
    ev_window_cmd_file_print(NULL, window);
  } else {
    g_warning("Unimplemented named action: %s, please post a "
              "bug report on Xreader bug tracker "
              "(https://github.com/linuxmint/xreader/issues) with a testcase.",
              name);
  }
}

void G_GNUC_USED  view_external_link_cb(EvWindow *window, EvLinkAction *action) {
  switch (ev_link_action_get_action_type(action)) {
  case EV_LINK_ACTION_TYPE_GOTO_DEST: {
    EvLinkDest *dest;

    dest = ev_link_action_get_dest(action);
    if (!dest)
      return;

    ev_window_open_copy_at_dest(window, dest);
  } break;
  case EV_LINK_ACTION_TYPE_EXTERNAL_URI:
    launch_external_uri(window, action);
    break;
  case EV_LINK_ACTION_TYPE_LAUNCH:
    launch_action(window, action);
    break;
  case EV_LINK_ACTION_TYPE_GOTO_REMOTE:
    open_remote_link(window, action);
    break;
  case EV_LINK_ACTION_TYPE_NAMED:
    do_action_named(window, action);
    break;
  default:
    g_assert_not_reached();
  }
}


#ifdef ENABLE_DBUS
void G_GNUC_USED  ev_window_sync_source(EvWindow *window, EvSourceLink *link) {
  guint32 timestamp;
  gchar *uri_input;
  GFile *input_gfile;

  if (window->priv->skeleton == NULL)
    return;

  timestamp = gtk_get_current_event_time();
  if (g_path_is_absolute(link->filename)) {
    input_gfile = g_file_new_for_path(link->filename);
  } else {
    GFile *gfile, *parent_gfile;

    gfile = g_file_new_for_uri(window->priv->uri);
    parent_gfile = g_file_get_parent(gfile);

    /* parent_gfile should never be NULL */
    if (parent_gfile == NULL) {
      g_printerr("Document URI is '/'\n");
      return;
    }

    input_gfile = g_file_get_child(parent_gfile, link->filename);
    g_object_unref(parent_gfile);
    g_object_unref(gfile);
  }

  uri_input = g_file_get_uri(input_gfile);
  g_object_unref(input_gfile);

  ev_xreader_window_emit_sync_source(
      window->priv->skeleton, uri_input,
      g_variant_new("(ii)", link->line, link->col), timestamp);
  g_free(uri_input);
}

void G_GNUC_USED  ev_window_emit_closed(EvWindow *window) {
  if (window->priv->skeleton == NULL)
    return;

  ev_xreader_window_emit_closed(window->priv->skeleton);

  /* If this is the last window call g_dbus_connection_flush_sync()
   * to make sure the signal is emitted.
   */
  if (ev_application_get_n_windows(EV_APP) == 1)
    g_dbus_connection_flush_sync(
        g_application_get_dbus_connection(g_application_get_default()), NULL,
        NULL);
}

void G_GNUC_USED ev_window_emit_doc_loaded(EvWindow *window) {
  if (window->priv->skeleton == NULL)
    return;

  ev_xreader_window_emit_document_loaded(window->priv->skeleton,
                                         window->priv->uri);
}

static gboolean G_GNUC_USED  handle_sync_view_cb(EvXreaderWindow *object,
                                    GDBusMethodInvocation *invocation,
                                    const gchar *source_file,
                                    GVariant *source_point, guint timestamp,
                                    EvWindow *window) {
  if (window->priv->document &&
      ev_document_has_synctex(window->priv->document)) {
    EvSourceLink link;

    link.filename = (char *)source_file;
    g_variant_get(source_point, "(ii)", &link.line, &link.col);
    ev_view_highlight_forward_search(EV_VIEW(window->priv->view), &link);
    gtk_window_present_with_time(GTK_WINDOW(window), timestamp);
  }

  ev_xreader_window_complete_sync_view(object, invocation);

  return TRUE;
}
#endif /* ENABLE_DBUS */

static gboolean G_GNUC_USED  _gtk_css_provider_load_from_resource(GtkCssProvider *provider,
                                                     const char *resource_path,
                                                     GError **error) {
  GBytes *data;
  gboolean retval;

  data = g_resources_lookup_data(resource_path, 0, error);
  if (!data)
    return FALSE;

  retval = gtk_css_provider_load_from_data(
      provider, g_bytes_get_data(data, NULL), g_bytes_get_size(data), error);
  g_bytes_unref(data);

  return retval;
}

static void ev_window_init(EvWindow *ev_window) {
  GtkBuilder *builder;
  GMenuModel *menu_model;
  GError *error = NULL;
  GtkWidget *sidebar_widget;
  GtkCssProvider *css_provider;
  guint page_cache_mb;
  GtkStyleContext *context;
#ifdef ENABLE_DBUS
  GDBusConnection *connection;
  static gint G_GNUC_USED  window_id = 0;
#endif

  g_signal_connect(ev_window, "configure_event",
                   G_CALLBACK(window_configure_event_cb), NULL);
  g_signal_connect(ev_window, "window_state_event",
                   G_CALLBACK(window_state_event_cb), NULL);

  ev_window->priv = ev_window_get_instance_private(ev_window);

#ifdef ENABLE_DBUS
  connection = g_application_get_dbus_connection(g_application_get_default());
  if (connection) {
    EvXreaderWindow *skeleton;

    ev_window->priv->dbus_object_path =
        g_strdup_printf(EV_WINDOW_DBUS_OBJECT_PATH, window_id++);

    skeleton = ev_xreader_window_skeleton_new();
    if (g_dbus_interface_skeleton_export(
            G_DBUS_INTERFACE_SKELETON(skeleton), connection,
            ev_window->priv->dbus_object_path, &error)) {
      ev_window->priv->skeleton = skeleton;
      g_signal_connect(skeleton, "handle-sync-view",
                       G_CALLBACK(handle_sync_view_cb), ev_window);
    } else {
      g_printerr("Failed to register bus object %s: %s\n",
                 ev_window->priv->dbus_object_path, error->message);
      g_error_free(error);
      g_free(ev_window->priv->dbus_object_path);
      ev_window->priv->dbus_object_path = NULL;
      error = NULL;

      g_object_unref(skeleton);
      ev_window->priv->skeleton = NULL;
    }
  }
#endif /* ENABLE_DBUS */

  /* disable automatic menubar handling, since we show our regular
   * menubar together with the app menu.
   */
  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(ev_window),
                                          FALSE);

  ev_window->priv->menubar_skip_release = FALSE;
  ev_window->priv->menubar_show_queued = FALSE;

  ev_window->priv->model = ev_document_model_new();

  ev_window->priv->page_mode = PAGE_MODE_DOCUMENT;
  ev_window->priv->chrome = EV_CHROME_NORMAL;
  ev_window->priv->title = ev_window_title_new(ev_window);

  context = gtk_widget_get_style_context(GTK_WIDGET(ev_window));
  gtk_style_context_add_class(context, "xreader-window");

  ev_window->priv->history = ev_history_new(ev_window->priv->model);
  g_signal_connect(ev_window->priv->history, "activate-link",
                   G_CALLBACK(activate_link_cb), ev_window);
  g_signal_connect(ev_window->priv->history, "changed",
                   G_CALLBACK(history_changed_cb), ev_window);

  ev_window->priv->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(ev_window), ev_window->priv->main_box);
  gtk_widget_show(ev_window->priv->main_box);

  g_action_map_add_action_entries(G_ACTION_MAP(ev_window), win_actions,
                                  G_N_ELEMENTS(win_actions), ev_window);
  ev_window_setup_default_shortcuts();

  css_provider = gtk_css_provider_new();
  _gtk_css_provider_load_from_resource(
      css_provider, "/org/x/reader/shell/ui/xreader.css", &error);
  g_assert_no_error(error);
  gtk_style_context_add_provider_for_screen(
      gtk_widget_get_screen(GTK_WIDGET(ev_window)),
      GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  ev_window->priv->recent_manager = gtk_recent_manager_get_default();
  g_signal_connect_swapped(ev_window->priv->recent_manager, "changed",
                           G_CALLBACK(ev_window_setup_recent), ev_window);
  ev_window->priv->favorites = xapp_favorites_get_default();
  g_signal_connect_swapped(ev_window->priv->favorites, "changed",
                           G_CALLBACK(ev_window_setup_favorites), ev_window);

  /* Load menus from resource */
  builder = gtk_builder_new();
  gtk_builder_add_from_resource(builder, "/org/x/reader/shell/ui/xreader-menus.ui", &error);
  if (error) {
    g_warning("Failed to load menus: %s", error->message);
    g_error_free(error);
  } else {
    menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "MainMenu"));
    ev_window->priv->menubar = gtk_menu_bar_new_from_model(menu_model);

    menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "DocumentPopup"));
    ev_window->priv->view_popup = gtk_menu_new_from_model(menu_model);

    menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "AttachmentPopup"));
    ev_window->priv->attachment_popup = gtk_menu_new_from_model(menu_model);

    ev_window->priv->favorites_menu =
        G_MENU(gtk_builder_get_object(builder, "FavoritesMenu"));
    ev_window->priv->recent_menu =
        G_MENU(gtk_builder_get_object(builder, "RecentsMenu"));
    ev_window->priv->bookmarks_menu =
        G_MENU(gtk_builder_get_object(builder, "BookmarksPlaceholder"));

    g_object_unref(builder);
  }

  gtk_box_pack_start(GTK_BOX(ev_window->priv->main_box),
                     ev_window->priv->menubar, FALSE, FALSE, 0);

  ev_window->priv->toolbar_revealer = gtk_revealer_new();
  gtk_box_pack_start(GTK_BOX(ev_window->priv->main_box),
                     ev_window->priv->toolbar_revealer, FALSE, TRUE, 0);
  gtk_revealer_set_transition_duration(
      GTK_REVEALER(ev_window->priv->toolbar_revealer), 175);
  gtk_widget_show(ev_window->priv->toolbar_revealer);

  ev_window->priv->toolbar = ev_toolbar_new(ev_window);
  gtk_container_add(GTK_CONTAINER(ev_window->priv->toolbar_revealer),
                    ev_window->priv->toolbar);
  gtk_widget_show(ev_window->priv->toolbar);

  ev_window->priv->hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

  g_signal_connect(ev_window->priv->hpaned, "notify::position",
                   G_CALLBACK(ev_window_sidebar_position_change_cb), ev_window);

  gtk_paned_set_position(GTK_PANED(ev_window->priv->hpaned),
                         SIDEBAR_DEFAULT_SIZE);
  gtk_box_pack_start(GTK_BOX(ev_window->priv->main_box),
                     ev_window->priv->hpaned, TRUE, TRUE, 0);
  gtk_widget_show(ev_window->priv->hpaned);

  ev_window->priv->sidebar = ev_sidebar_new();
  ev_sidebar_set_model(EV_SIDEBAR(ev_window->priv->sidebar),
                       ev_window->priv->model);
  gtk_paned_pack1(GTK_PANED(ev_window->priv->hpaned), ev_window->priv->sidebar,
                  FALSE, FALSE);
  gtk_widget_show(ev_window->priv->sidebar);

  /* Stub sidebar, for now */

  sidebar_widget = ev_sidebar_thumbnails_new();
  ev_window->priv->sidebar_thumbs = sidebar_widget;
  g_signal_connect(sidebar_widget, "notify::main-widget",
                   G_CALLBACK(sidebar_page_main_widget_update_cb), ev_window);
  g_signal_connect(sidebar_widget, "size_changed",
                   G_CALLBACK(sidebar_thumbnails_size_changed), ev_window);
  sidebar_page_main_widget_update_cb(G_OBJECT(sidebar_widget), NULL, ev_window);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  sidebar_widget = ev_sidebar_links_new();
  ev_window->priv->sidebar_links = sidebar_widget;
  g_signal_connect(sidebar_widget, "notify::model",
                   G_CALLBACK(sidebar_widget_model_set), ev_window);
  g_signal_connect(sidebar_widget, "link_activated",
                   G_CALLBACK(sidebar_links_link_activated_cb), ev_window);
  sidebar_page_main_widget_update_cb(G_OBJECT(sidebar_widget), NULL, ev_window);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  sidebar_widget = ev_sidebar_attachments_new();
  ev_window->priv->sidebar_attachments = sidebar_widget;
  g_signal_connect_object(sidebar_widget, "popup",
                          G_CALLBACK(attachment_bar_menu_popup_cb), ev_window,
                          0);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  sidebar_widget = ev_sidebar_layers_new();
  ev_window->priv->sidebar_layers = sidebar_widget;
  g_signal_connect(sidebar_widget, "layers_visibility_changed",
                   G_CALLBACK(sidebar_layers_visibility_changed), ev_window);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  sidebar_widget = ev_sidebar_annotations_new();
  EvAnnotationsToolbar *annot_toolbar = ev_sidebar_annotations_get_toolbar(
      EV_SIDEBAR_ANNOTATIONS(sidebar_widget));
  ev_window->priv->sidebar_annots = sidebar_widget;
  g_signal_connect(sidebar_widget, "annot_activated",
                   G_CALLBACK(sidebar_annots_annot_activated_cb), ev_window);
  g_signal_connect(annot_toolbar, "begin-add-annot",
                   G_CALLBACK(ev_window_begin_add_annot), ev_window);
  g_signal_connect(annot_toolbar, "cancel-add-annot",
                   G_CALLBACK(ev_window_cancel_add_annot), ev_window);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  sidebar_widget = ev_sidebar_bookmarks_new();
  ev_window->priv->sidebar_bookmarks = sidebar_widget;
  g_signal_connect(sidebar_widget, "add-bookmark",
                   G_CALLBACK(sidebar_bookmarks_add_bookmark), ev_window);
  gtk_widget_show(sidebar_widget);
  ev_sidebar_add_page(EV_SIDEBAR(ev_window->priv->sidebar), sidebar_widget);

  ev_window->priv->view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  ev_window->priv->notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(ev_window->priv->notebook), TRUE);
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(ev_window->priv->notebook), FALSE);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(ev_window->priv->notebook), FALSE);
  gtk_notebook_set_action_widget(
      GTK_NOTEBOOK(ev_window->priv->notebook),
      ev_window_create_tab_actions(GTK_NOTEBOOK(ev_window->priv->notebook)),
      GTK_PACK_END);
  gtk_box_pack_start(GTK_BOX(ev_window->priv->view_box),
                     ev_window->priv->notebook, TRUE, TRUE, 0);
  gtk_widget_show(ev_window->priv->notebook);

  ev_window_setup_tab_close(GTK_NOTEBOOK(ev_window->priv->notebook), ev_window);

  ev_window->priv->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_notebook_append_page(
      GTK_NOTEBOOK(ev_window->priv->notebook), ev_window->priv->scrolled_window,
      ev_window_create_tab_label(GTK_NOTEBOOK(ev_window->priv->notebook),
                                 ev_window->priv->scrolled_window,
                                 _("Default")));
  gtk_widget_show(ev_window->priv->scrolled_window);

  ev_window->priv->view = ev_view_new();

  EvTabData *initial_tab = g_new0(EvTabData, 1);
  initial_tab->scrolled_window = ev_window->priv->scrolled_window;
  initial_tab->view = ev_window->priv->view;
  initial_tab->model =
      ev_window->priv->model ? g_object_ref(ev_window->priv->model) : NULL;
  initial_tab->history =
      ev_window->priv->history ? g_object_ref(ev_window->priv->history) : NULL;
  g_object_set_data_full(G_OBJECT(ev_window->priv->scrolled_window),
                         "ev-tab-data", initial_tab,
                         (GDestroyNotify)ev_tab_data_free);

  g_signal_connect(ev_window->priv->notebook, "switch-page",
                   G_CALLBACK(ev_window_switch_page_cb), ev_window);

  gtk_paned_add2(GTK_PANED(ev_window->priv->hpaned), ev_window->priv->view_box);
  gtk_widget_show(ev_window->priv->view_box);

#if ENABLE_EPUB /*The webview, we won't add it now but it will replace the     \
                   xreader-view if a web(epub) document is encountered.*/
  ev_window->priv->webview = ev_web_view_new();
  ev_web_view_set_model(EV_WEB_VIEW(ev_window->priv->webview),
                        ev_window->priv->model);
#endif
  page_cache_mb = g_settings_get_uint(ev_window_ensure_settings(ev_window),
                                      GS_PAGE_CACHE_SIZE);
  ev_view_set_page_cache_size(EV_VIEW(ev_window->priv->view),
                              page_cache_mb * 1024 * 1024);
  ev_view_set_model(EV_VIEW(ev_window->priv->view), ev_window->priv->model);

  ev_window->priv->password_view = ev_password_view_new(GTK_WINDOW(ev_window));
  g_signal_connect_swapped(ev_window->priv->password_view, "unlock",
                           G_CALLBACK(ev_window_password_view_unlock),
                           ev_window);
  ev_window_setup_view(ev_window, EV_VIEW(ev_window->priv->view));
  gtk_widget_show(ev_window->priv->view);
  gtk_widget_show(ev_window->priv->password_view);

  /* Find Bar */
  ev_window->priv->find_bar = egg_find_bar_new();
  gtk_box_pack_end(GTK_BOX(ev_window->priv->main_box),
                   ev_window->priv->find_bar, FALSE, TRUE, 0);

  /* We own a ref on these widgets, as we can swap them in and out */
  g_object_ref(ev_window->priv->view);
  g_object_ref(ev_window->priv->password_view);

  gtk_container_add(GTK_CONTAINER(ev_window->priv->scrolled_window),
                    ev_window->priv->view);

  /* Connect to model signals */
  g_signal_connect_swapped(ev_window->priv->model, "page-changed",
                           G_CALLBACK(ev_window_page_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::document",
                   G_CALLBACK(ev_window_document_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::scale",
                   G_CALLBACK(ev_window_zoom_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::sizing-mode",
                   G_CALLBACK(ev_window_sizing_mode_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::rotation",
                   G_CALLBACK(ev_window_rotation_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::continuous",
                   G_CALLBACK(ev_window_continuous_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::dual-page",
                   G_CALLBACK(ev_window_dual_mode_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::dual-odd-left",
                   G_CALLBACK(ev_window_dual_mode_odd_pages_left_changed_cb),
                   ev_window);
  g_signal_connect(ev_window->priv->model, "notify::rtl",
                   G_CALLBACK(ev_window_direction_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->model, "notify::inverted-colors",
                   G_CALLBACK(ev_window_inverted_colors_changed_cb), ev_window);

  /* Connect sidebar signals */
  g_signal_connect(ev_window->priv->sidebar, "notify::visible",
                   G_CALLBACK(ev_window_sidebar_visibility_changed_cb),
                   ev_window);
  g_signal_connect(ev_window->priv->sidebar, "notify::current-page",
                   G_CALLBACK(ev_window_sidebar_current_page_changed_cb),
                   ev_window);

  /* Connect to find bar signals */
  g_signal_connect(ev_window->priv->find_bar, "previous",
                   G_CALLBACK(find_bar_previous_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "next",
                   G_CALLBACK(find_bar_next_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "close",
                   G_CALLBACK(find_bar_close_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "notify::search-string",
                   G_CALLBACK(find_bar_search_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "notify::case-sensitive",
                   G_CALLBACK(find_bar_search_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "notify::visible",
                   G_CALLBACK(find_bar_visibility_changed_cb), ev_window);
  g_signal_connect(ev_window->priv->find_bar, "scroll",
                   G_CALLBACK(find_bar_scroll), ev_window);

  /* Popups */
  ev_window->priv->link = NULL;
  ev_window->priv->attach_list = NULL;

  /* Give focus to the document view */
  gtk_widget_grab_focus(ev_window->priv->view);

  ev_window->priv->default_settings = g_settings_new(GS_SCHEMA_NAME_DEFAULT);
  g_settings_delay(ev_window->priv->default_settings);
  update_chrome_actions(ev_window);

  /* Set it user interface params */
  ev_window_setup_recent(ev_window);
  ev_window_setup_favorites(ev_window);

  ev_window_setup_gtk_settings(ev_window);

  gtk_window_set_default_size(GTK_WINDOW(ev_window), 600, 600);

  ev_window_sizing_mode_changed_cb(ev_window->priv->model, NULL, ev_window);
  ev_window_setup_action_sensitivity(ev_window);

  /* Drag and Drop */
  gtk_drag_dest_set(GTK_WIDGET(ev_window), GTK_DEST_DEFAULT_ALL, NULL, 0,
                    GDK_ACTION_COPY);
  gtk_drag_dest_add_uri_targets(GTK_WIDGET(ev_window));
}

/**
 * ev_window_new:
 *
 * Creates a #GtkWidget that represents the window.
 *
 * Returns: the #GtkWidget that represents the window.
 */
GtkWidget * G_GNUC_USED ev_window_new(void) {
  GtkWidget *ev_window;

  ev_window = GTK_WIDGET(g_object_new(EV_TYPE_WINDOW, "type",
                                      GTK_WINDOW_TOPLEVEL, "application",
                                      g_application_get_default(), NULL));

  return ev_window;
}

const gchar * G_GNUC_USED ev_window_get_dbus_object_path(EvWindow *ev_window) {
#ifdef ENABLE_DBUS
  return ev_window->priv->dbus_object_path;
#else
  return NULL;
#endif
}

EvDocumentModel * G_GNUC_USED ev_window_get_document_model(EvWindow *ev_window) {
  g_return_val_if_fail(EV_WINDOW(ev_window), NULL);

  return ev_window->priv->model;
}
