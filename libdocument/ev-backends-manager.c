/* this file is part of xreader, a mate document viewer
 *
 *  Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
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

#include <config.h>

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "ev-backends-manager.h"
#include "ev-module.h"

static GList *ev_backends_list = NULL;

typedef struct _EvBackendInfo EvBackendInfo;
struct _EvBackendInfo {
  gchar *module_name;
  gchar *path; /* New field: directory where the backend resides */
  GTypeModule *module;
  gboolean resident;
  gboolean loaded; /* PATCH 2: Track if module is loaded */

  GType type_id;

  gchar *type_desc;
  gchar **mime_types;
};

#define EV_BACKENDS_GROUP "Xreader Backend"
#define EV_BACKENDS_EXTENSION ".xreader-backend"

static gchar *backendsdir = NULL;

static const gchar *backends_dir(void) {
  if (!backendsdir) {
    const gchar *env_dir = g_getenv("XREADER_BACKENDS_DIR");
    if (env_dir) {
      backendsdir = g_strdup(env_dir);
      g_printerr("[ev-backends-manager] backendsdir inicializado via ENV: %s\n",
                 backendsdir);
    } else {
      backendsdir = g_strdup(EV_BACKENDSDIR);
      g_printerr(
          "[ev-backends-manager] backendsdir inicializado via DEFAULT: %s\n",
          backendsdir);
    }
  }
  return backendsdir;
}

const gchar *ev_backends_manager_get_backends_dir(void) {
  return backends_dir();
}

static void ev_backend_info_free(EvBackendInfo *info) {
  g_free(info->module_name);
  g_free(info->path);
  g_free(info->type_desc);
  g_strfreev(info->mime_types);
  g_free(info);
}

static EvBackendInfo *ev_backends_manager_load_backend(const gchar *file,
                                                       const gchar *path) {
  EvBackendInfo *info;
  GKeyFile *backend_file = NULL;
  GError *error = NULL;

  backend_file = g_key_file_new();
  if (!g_key_file_load_from_file(backend_file, file, G_KEY_FILE_NONE, &error)) {
    g_warning("Error opening backend file %s: %s", file, error->message);
    g_error_free(error);
    g_key_file_free(backend_file);

    return NULL;
  }

  info = g_new0(EvBackendInfo, 1);
  info->path = g_strdup(path);
  info->loaded = FALSE; /* PATCH 2: Initially not loaded */
  info->module_name =
      g_key_file_get_string(backend_file, EV_BACKENDS_GROUP, "Module", NULL);
  if (!info->module_name) {
    g_warning("Bad xreader backend file %s: Could not find 'Module'", file);
    ev_backend_info_free(info);
    g_key_file_free(backend_file);

    return NULL;
  }

  info->resident =
      g_key_file_get_boolean(backend_file, EV_BACKENDS_GROUP, "Resident", NULL);

  info->type_desc = g_key_file_get_locale_string(
      backend_file, EV_BACKENDS_GROUP, "TypeDescription", NULL, NULL);
  if (!info->type_desc) {
    g_warning("Bad xreader backend file %s: Could not find 'TypeDescription'",
              file);
    ev_backend_info_free(info);
    g_key_file_free(backend_file);

    return NULL;
  }

  info->mime_types = g_key_file_get_string_list(backend_file, EV_BACKENDS_GROUP,
                                                "MimeType", NULL, NULL);
  if (!info->mime_types) {
    g_warning("Bad xreader backend file %s: Could not find 'MimeType'", file);
    ev_backend_info_free(info);
    g_key_file_free(backend_file);

    return NULL;
  }

  g_key_file_free(backend_file);

  return info;
}

static int ev_backends_manager_load_dir(const gchar *bdir, gboolean recursive) {
  GDir *dir;
  const gchar *dirent;
  GError *error = NULL;
  int count = 0;

  dir = g_dir_open(bdir, 0, &error);
  if (!dir) {
    g_warning("%s", error->message);
    g_error_free(error);
    return 0;
  }

  while ((dirent = g_dir_read_name(dir))) {
    gchar *file = g_build_filename(bdir, dirent, NULL);

    if (g_str_has_suffix(dirent, EV_BACKENDS_EXTENSION)) {
      EvBackendInfo *info = ev_backends_manager_load_backend(file, bdir);
      if (info) {
        ev_backends_list = g_list_prepend(ev_backends_list, info);
        count++;
      }
    } else if (recursive && g_file_test(file, G_FILE_TEST_IS_DIR)) {
      count += ev_backends_manager_load_dir(file, FALSE);
    }
    g_free(file);
  }
  g_dir_close(dir);
  return count;
}

static gboolean ev_backends_manager_load(void) {
  const gchar *bdir = backends_dir();
  gboolean is_dev = (g_getenv("XREADER_BACKENDS_DIR") != NULL);
  int count;

  g_printerr("[ev-backends-manager] Carregando backends de: %s (dev=%d)\n",
             bdir, is_dev);
  count = ev_backends_manager_load_dir(bdir, is_dev);
  g_printerr("[ev-backends-manager] Total de backends carregados: %d\n", count);

  return ev_backends_list != NULL;
}

/*
 * _ev_backends_manager_init:
 *
 * Initializes the xreader backends manager.
 *
 * Returns: %TRUE if there were any backends found; %FALSE otherwise
 */
gboolean _ev_backends_manager_init(void) {
  if (ev_backends_list)
    return TRUE;

  return ev_backends_manager_load();
}

/*
 * _ev_backends_manager_shutdown:
 *
 * Shuts the xreader backends manager down.
 */
void _ev_backends_manager_shutdown(void) {
  g_list_foreach(ev_backends_list, (GFunc)ev_backend_info_free, NULL);
  g_list_free(ev_backends_list);
  ev_backends_list = NULL;

  g_free(backendsdir);
}

static EvBackendInfo *
ev_backends_manager_get_backend_info(const gchar *mime_type) {
  EvBackendInfo *ret;
  GList *l;
  gchar *file_type = g_content_type_from_mime_type(mime_type);

  ret = NULL;
  // Check exact matches first - this allows more specific handlers to
  // take precedence.
  for (l = ev_backends_list; l; l = g_list_next(l)) {
    EvBackendInfo *info;
    gint i = 0;
    const char *mime;

    info = (EvBackendInfo *)l->data;

    while ((mime = info->mime_types[i++])) {
      gchar *t = g_content_type_from_mime_type(mime);

      if (g_content_type_equals(file_type, t)) {
        ret = info;
      }

      g_free(t);

      if (ret) {
        break;
      }
    }
  }

  if (ret == NULL) {
    // Then match for sub-types
    for (l = ev_backends_list; l; l = g_list_next(l)) {
      EvBackendInfo *info;
      gint i = 0;
      const char *mime;

      info = (EvBackendInfo *)l->data;

      while ((mime = info->mime_types[i++])) {
        gchar *t = g_content_type_from_mime_type(mime);
        if (g_content_type_is_a(file_type, t)) {
          ret = info;
        }

        g_free(t);

        if (ret) {
          break;
        }
      }
    }
  }

  g_free(file_type);
  return ret;
}

EvDocument *ev_backends_manager_get_document(const gchar *mime_type) {
  EvDocument *document;
  EvBackendInfo *info;

  info = ev_backends_manager_get_backend_info(mime_type);
  if (!info)
    return NULL;

  /* PATCH 2: Lazy-load backend module on first use */
  if (!info->loaded) {
    gchar *path;

    path = g_module_build_path(info->path, info->module_name);
    info->module = G_TYPE_MODULE(ev_module_new(path, info->resident));
    g_free(path);
    info->loaded = TRUE;
  }

  if (!info->module) {
    return NULL;
  }

  if (!g_type_module_use(info->module)) {
    g_warning("Cannot load backend '%s' since file '%s' cannot be read.",
              info->module_name, ev_module_get_path(EV_MODULE(info->module)));
    g_object_unref(G_OBJECT(info->module));
    info->module = NULL;

    return NULL;
  }

  document = EV_DOCUMENT(ev_module_new_object(EV_MODULE(info->module)));
  g_type_module_unuse(info->module);

  return document;
}

static EvBackendInfo *get_document_backend_info(EvDocument *document) {
  GList *l;

  for (l = ev_backends_list; l; l = g_list_next(l)) {
    EvBackendInfo *info;
    GType type_id;

    info = (EvBackendInfo *)l->data;

    if (!info->module)
      continue;

    type_id = ev_module_get_object_type(EV_MODULE(info->module));

    if (G_TYPE_CHECK_INSTANCE_TYPE(document, type_id)) {
      return info;
    }
  }

  return NULL;
}

const gchar *
ev_backends_manager_get_document_module_name(EvDocument *document) {
  EvBackendInfo *info;

  info = get_document_backend_info(document);
  return info ? info->module_name : NULL;
}

static EvTypeInfo *ev_type_info_new(const gchar *desc,
                                    const gchar **mime_types) {
  EvTypeInfo *info;

  info = g_new(EvTypeInfo, 1);

  info->desc = desc;
  info->mime_types = mime_types;

  return info;
}

EvTypeInfo *ev_backends_manager_get_document_type_info(EvDocument *document) {
  EvBackendInfo *info;

  info = get_document_backend_info(document);
  return info ? ev_type_info_new(info->type_desc,
                                 (const gchar **)info->mime_types)
              : NULL;
}

GList *ev_backends_manager_get_all_types_info(void) {
  GList *l;
  GList *retval = NULL;

  for (l = ev_backends_list; l; l = g_list_next(l)) {
    EvBackendInfo *info;
    EvTypeInfo *type_info;

    info = (EvBackendInfo *)l->data;

    type_info =
        ev_type_info_new(info->type_desc, (const gchar **)info->mime_types);
    retval = g_list_prepend(retval, type_info);
  }

  return retval;
}
