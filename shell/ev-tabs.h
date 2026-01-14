#include <glib.h>


#ifndef EV_TABS_H
#define EV_TABS_H

#include <gtk/gtk.h>
#include "ev-window.h"

// Estrutura para armazenar dados de uma aba
typedef struct {
    GtkWidget *scrolled_window;
    GtkWidget *view;
    EvDocument *document;
    gchar *uri;
    // Adicione outros campos necessários para o estado da aba
} EvTabData;

// Apenas declarações, implementações no .c

void ev_window_setup_tab_close(GtkNotebook *notebook, gpointer window_user_data);
int ev_window_add_tab(GtkNotebook *notebook, const gchar *uri);
EvTabData* ev_window_get_current_tab(GtkNotebook *notebook);
void ev_window_close_current_tab(GtkNotebook *notebook);

#endif // EV_TABS_H


