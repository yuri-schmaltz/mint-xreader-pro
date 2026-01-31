#ifndef EV_TABS_H
#define EV_TABS_H

#include <glib.h>

#include "ev-window.h"
#include <gtk/gtk.h>

// Structure to store tab data
typedef struct {
  GtkWidget *scrolled_window;
  GtkWidget *view;
  EvDocument *document;
  gchar *uri;
  // Add other necessary fields for the tab state
} EvTabData;

// Declarations only, implementations in the .c file

void ev_window_setup_tab_close(GtkNotebook *notebook,
                               gpointer window_user_data);
int ev_window_add_tab(GtkNotebook *notebook, const gchar *uri);
EvTabData *ev_window_get_current_tab(GtkNotebook *notebook);
void ev_window_close_current_tab(GtkNotebook *notebook);

#endif // EV_TABS_H
