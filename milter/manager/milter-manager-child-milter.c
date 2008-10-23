/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "milter-manager-child-milter.h"

#define MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_MANAGER_TYPE_CHILD_MILTER,      \
                                 MilterManagerChildMilterPrivate))

typedef struct _MilterManagerChildMilterPrivate	MilterManagerChildMilterPrivate;
struct _MilterManagerChildMilterPrivate
{
    gchar *name;
    gchar *user_name;
    gchar *working_directory;
    gchar *command;
    gchar *command_options;
    GSpawnChildSetupFunc child_setup;
    GPid pid;
    guint child_watch_id;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_USER_NAME,
    PROP_COMMAND,
    PROP_COMMAND_OPTIONS,
    PROP_WORKING_DIRECTORY,
};

MILTER_DEFINE_ERROR_EMITABLE_TYPE(MilterManagerChildMilter,
                                  milter_manager_child_milter,
                                  MILTER_TYPE_SERVER_CONTEXT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_manager_child_milter_class_init (MilterManagerChildMilterClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the child milter",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    spec = g_param_spec_string("user-name",
                               "User name",
                               "The name of process owner user",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_USER_NAME, spec);

    spec = g_param_spec_string("command",
                               "Command",
                               "The command string of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND, spec);

    spec = g_param_spec_string("command-options",
                               "Command options",
                               "The command options string of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND_OPTIONS, spec);

    spec = g_param_spec_string("working-directory",
                               "Working directory",
                               "The working directory name of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WORKING_DIRECTORY, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerChildMilterPrivate));
}

static void
milter_manager_child_milter_init (MilterManagerChildMilter *milter)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(milter);
    priv->name = NULL;
    priv->user_name = NULL;
    priv->working_directory = NULL;
    priv->command = NULL;
    priv->command_options = NULL;
    priv->child_setup = NULL;
    priv->pid = -1;
    priv->child_watch_id = 0;
}

static void
dispose (GObject *object)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->user_name) {
        g_free(priv->user_name);
        priv->user_name = NULL;
    }

    if (priv->working_directory) {
        g_free(priv->working_directory);
        priv->working_directory = NULL;
    }

    if (priv->command) {
        g_free(priv->command);
        priv->command = NULL;
    }

    if (priv->command_options) {
        g_free(priv->command_options);
        priv->command_options = NULL;
    }

    if (priv->pid > 0) {
        g_spawn_close_pid(priv->pid);
        priv->pid = -1;
    }

    if (priv->child_watch_id > 0) {
        g_source_remove(priv->child_watch_id);
        priv->child_watch_id = 0;
    }

    G_OBJECT_CLASS(milter_manager_child_milter_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        if (priv->name)
            g_free(priv->name);
        priv->name = g_value_dup_string(value);
        break;
      case PROP_USER_NAME:
        if (priv->user_name)
            g_free(priv->user_name);
        priv->user_name = g_value_dup_string(value);
        break;
      case PROP_COMMAND:
        if (priv->command)
            g_free(priv->command);
        priv->command = g_value_dup_string(value);
        break;
      case PROP_COMMAND_OPTIONS:
        if (priv->command_options)
            g_free(priv->command_options);
        priv->command_options = g_value_dup_string(value);
        break;
      case PROP_WORKING_DIRECTORY:
        if (priv->working_directory)
            g_free(priv->working_directory);
        priv->working_directory = g_value_dup_string(value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;
      case PROP_USER_NAME:
        g_value_set_string(value, priv->user_name);
        break;
      case PROP_COMMAND:
        g_value_set_string(value, priv->command);
        break;
      case PROP_COMMAND_OPTIONS:
        g_value_set_string(value, priv->command_options);
        break;
      case PROP_WORKING_DIRECTORY:
        g_value_set_string(value, priv->working_directory);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_child_milter_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-child-milter-error-quark");
}

MilterManagerChildMilter *
milter_manager_child_milter_new (const gchar *name)
{
    return g_object_new(MILTER_MANAGER_TYPE_CHILD_MILTER,
                        "name", name,
                        NULL);
}

static void
child_watch_func (GPid pid, gint status, gpointer user_data)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(user_data);

    if (WCOREDUMP(status)) {
        GError *error = NULL;
        g_set_error(&error,
                    MILTER_MANAGER_CHILD_MILTER_ERROR,
                    MILTER_MANAGER_CHILD_MILTER_ERROR_MILTER_CORE_DUMP,
                    "%s produced a core dump", priv->name);
        milter_error("%s", error->message);
        milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(user_data),
                                         error);
        g_error_free(error);
    } else if (WIFEXITED(status)) {
        GError *error = NULL;
        g_set_error(&error,
                    MILTER_MANAGER_CHILD_MILTER_ERROR,
                    MILTER_MANAGER_CHILD_MILTER_ERROR_MILTER_EXIT,
                    "%s exits with status: %d", priv->name, status);
        milter_error("%s", error->message);
        milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(user_data),
                                         error);
        g_error_free(error);
    }
}

static gboolean
set_user (const gchar *user_name, GError **error)
{
    struct passwd *password;

    password = getpwnam(user_name);
    if (!password) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_INVALID_USER_NAME,
                                              NULL,
                                              "No passwd entry for %s. :%s",
                                              user_name,
                                              strerror(errno));
        return FALSE;
    }

    if (setregid(-1, password->pw_gid) == -1) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_NO_PRIVILEGE_MODE,
                                              NULL,
                                              "MilterManager does not run on privilege mode.");
        return FALSE;
    }

    if (setreuid(-1, password->pw_uid) == -1) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_NO_PRIVILEGE_MODE,
                                              NULL,
                                              "MilterManager does not run on privilege mode.");
        return FALSE;
    }

    return TRUE;
}

gboolean
milter_manager_child_milter_start (MilterManagerChildMilter *milter,
                                   GError **error)
{
    gint argc;
    gchar **argv;
    gchar *command_line;
    gboolean success;
    MilterManagerChildMilterPrivate *priv;
    GError *internal_error = NULL;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(milter);

    if (!priv->command) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_BAD_COMMAND_STRING,
                                              NULL,
                                              "No command set yet.");
        return FALSE;
    }

    if (priv->command_options)
        command_line = g_strdup_printf("%s %s", priv->command, priv->command_options);
    else
        command_line = g_strdup(priv->command);

    success = g_shell_parse_argv(command_line,
                                 &argc, &argv,
                                 &internal_error);
    g_free(command_line);

    if (!success) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_BAD_COMMAND_STRING,
                                              internal_error,
                                              "Command string has invalid character(s).");
        return FALSE;
    }

    if (priv->user_name && !set_user(priv->user_name, error))
        return FALSE;

    success = g_spawn_async(priv->working_directory,
                            argv,
                            NULL,
                            G_SPAWN_DO_NOT_REAP_CHILD,
                            priv->child_setup,
                            milter,
                            &priv->pid,
                            &internal_error);
    g_strfreev(argv);

    if (!success) {
        milter_utils_set_error_with_sub_error(error,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR,
                                              MILTER_MANAGER_CHILD_MILTER_ERROR_START_FAILURE,
                                              internal_error,
                                              "Couldn't start new milter process.");
        return FALSE;
    }

    priv->child_watch_id =
        g_child_watch_add(priv->pid, (GChildWatchFunc)child_watch_func, milter);

    return success;
}

#if 0
MilterStatus
milter_manager_child_milter_option_negotiate (MilterManagerChildMilter *milter,
                                              MilterOption             *option)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_connect (MilterManagerChildMilter *milter,
                                     const gchar              *host_name,
                                     struct sockaddr          *address,
                                     socklen_t                 address_length)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_helo (MilterManagerChildMilter *milter,
                                  const gchar              *fqdn)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_envelope_from (MilterManagerChildMilter *milter,
                                           const gchar              *from)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_envelope_receipt (MilterManagerChildMilter *milter,
                                              const gchar              *receipt)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_data (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_unknown (MilterManagerChildMilter *milter,
                                     const gchar              *command)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_header (MilterManagerChildMilter *milter,
                                    const gchar              *name,
                                    const gchar              *value)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_end_of_header (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_body (MilterManagerChildMilter *milter,
                                  const guchar             *chunk,
                                  gsize                     size)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_end_of_message (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_quit (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_abort (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
