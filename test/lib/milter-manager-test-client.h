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

#ifndef __MILTER_MANAGER_TEST_CLIENT_H__
#define __MILTER_MANAGER_TEST_CLIENT_H__

#include <gcutter.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_TEST_CLIENT            (milter_manager_test_client_get_type())
#define MILTER_MANAGER_TEST_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClient))
#define MILTER_MANAGER_TEST_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClientClass))
#define MILTER_MANAGER_IS_TEST_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_TEST_CLIENT))
#define MILTER_MANAGER_IS_TEST_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_TEST_CLIENT))
#define MILTER_MANAGER_TEST_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClientClass))

typedef struct _MilterManagerTestClient         MilterManagerTestClient;
typedef struct _MilterManagerTestClientClass    MilterManagerTestClientClass;

typedef guint (*MilterManagerTestClientGetNReceived) (MilterManagerTestClient *client);
typedef const gchar *(*MilterManagerTestClientGetString) (MilterManagerTestClient *client);

struct _MilterManagerTestClient
{
    GObject object;
};

struct _MilterManagerTestClientClass
{
    GObjectClass parent_class;
};

GType                    milter_manager_test_client_get_type (void) G_GNUC_CONST;

MilterManagerTestClient *milter_manager_test_client_new
                                              (guint port);

void                     milter_manager_test_client_set_arguments
                                              (MilterManagerTestClient *client,
                                               GArray *arguments);
gboolean                 milter_manager_test_client_run
                                              (MilterManagerTestClient *client,
                                               GError **error);

guint                    milter_manager_test_client_get_n_negotiate_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_connect_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_helo_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_helo_fqdn
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_envelope_from_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_envelope_from
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_envelope_recipient_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_envelope_recipient
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_data_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_header_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_header_name
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_header_value
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_end_of_header_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_body_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_body_chunk
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_end_of_message_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_end_of_message_chunk
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_quit_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_abort_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_unknown_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_unknown_command
                                              (MilterManagerTestClient *client);

void                     milter_manager_test_client_clear_data
                                              (MilterManagerTestClient *client);


void                     milter_manager_test_clients_wait_reply
                                              (GList *clients,
                                               MilterManagerTestClientGetNReceived getter);
guint                    milter_manager_test_clients_collect_n_received
                                              (GList *clients,
                                               MilterManagerTestClientGetNReceived getter);
const GList             *milter_manager_test_clients_collect_strings
                                              (GList *clients,
                                               MilterManagerTestClientGetString getter);
const GList             *milter_manager_test_clients_collect_pairs
                                              (GList *clients,
                                               MilterManagerTestClientGetString name_getter,
                                               MilterManagerTestClientGetString value_getter);


G_END_DECLS

#endif /* __MILTER_MANAGER_TEST_CLIENT_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
