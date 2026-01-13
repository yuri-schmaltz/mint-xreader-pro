#include <glib.h>

// Callback para fechamento de aba
static void on_tab_close_requested(GtkNotebook *notebook, GtkWidget *child, guint page, gpointer user_data) {
    // Recupera dados da aba
    EvTabData *tab_data = (EvTabData*)g_object_get_data(G_OBJECT(child), "ev-tab-data");
    if (tab_data) {
        if (tab_data->uri) g_free(tab_data->uri);
        // Libera outros recursos se necessário
    }
    gtk_notebook_remove_page(notebook, page);
}

// Função para conectar o callback de fechamento
static void ev_window_setup_tab_close(EvWindow *window) {
    g_signal_connect(window->priv->notebook, "page-removed", G_CALLBACK(on_tab_close_requested), window);
}
// Funções utilitárias para suporte a múltiplas abas no Xreader
// Este arquivo será incluído em ev-window.c

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

// Cria uma nova aba no notebook e retorna o índice
int ev_window_add_tab(EvWindow *window, const gchar *uri) {
    EvTabData *tab_data = g_new0(EvTabData, 1);
    tab_data->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    tab_data->view = ev_view_new();
    gtk_container_add(GTK_CONTAINER(tab_data->scrolled_window), tab_data->view);
    gtk_widget_show(tab_data->view);
    gtk_widget_show(tab_data->scrolled_window);

    // Título da aba
    gchar *tab_title = g_path_get_basename(uri);
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), tab_data->scrolled_window, gtk_label_new(tab_title));
    g_free(tab_title);

    // Armazena o ponteiro para tab_data no widget da aba
    g_object_set_data_full(G_OBJECT(tab_data->scrolled_window), "ev-tab-data", tab_data, g_free);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), page_num);
    return page_num;
}

// Recupera os dados da aba ativa
EvTabData* ev_window_get_current_tab(EvWindow *window) {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window->priv->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window->priv->notebook), page);
    return (EvTabData*)g_object_get_data(G_OBJECT(child), "ev-tab-data");
}

// Remove a aba atual
void ev_window_close_current_tab(EvWindow *window) {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window->priv->notebook));
    gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), page);
}
