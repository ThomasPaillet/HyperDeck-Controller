/*
 * copyright (c) 2026 Thomas Paillet <thomas.paillet@net-c.fr>

 * This file is part of HyperDeck-Controller.

 * HyperDeck-Controller is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * HyperDeck-Controller is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with HyperDeck-Controller. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Osc.h"

#include "Fresque.h"
#include "HyperDeck_Protocol.h"
#include "Logging.h"
#include "Pixbufs.h"
#include "Preset.h"


struct sockaddr_in osc_address;
SOCKET osc_socket;

GThread *osc_thread = NULL;


gboolean hyperdeck_play (hyperdeck_t *hyperdeck)
{
	if (gtk_widget_get_sensitive (hyperdeck->play_button)) gtk_button_clicked (GTK_BUTTON (hyperdeck->play_button));

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_stop (hyperdeck_t *hyperdeck)
{
	if (gtk_widget_get_sensitive (hyperdeck->stop_button)) gtk_button_clicked (GTK_BUTTON (hyperdeck->stop_button));

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_loop (hyperdeck_t *hyperdeck)
{
	gtk_button_clicked (GTK_BUTTON (hyperdeck->single_loop_button));

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_single_clip_true_loop_true (hyperdeck_t *hyperdeck)
{
	hyperdeck->loop = SINGLE_CLIP_TRUE_LOOP_TRUE;

	if (hyperdeck->play) send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck->loop]);

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_single_clip_true_loop_false (hyperdeck_t *hyperdeck)
{
	hyperdeck->loop = SINGLE_CLIP_TRUE_LOOP_FALSE;

	if (hyperdeck->play) send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck->loop]);

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_single_clip_false_loop_false (hyperdeck_t *hyperdeck)
{
	hyperdeck->loop = SINGLE_CLIP_FALSE_LOOP_FALSE;

	if (hyperdeck->play) send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck->loop]);

	return G_SOURCE_REMOVE;
}

gboolean hyperdeck_single_clip_false_loop_true (hyperdeck_t *hyperdeck)
{
	hyperdeck->loop = SINGLE_CLIP_FALSE_LOOP_TRUE;

	if (hyperdeck->play) send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck->loop]);

	return G_SOURCE_REMOVE;
}

gboolean select_clip (clip_list_t *clip)
{
	LOG_OSC_2_STRINGS("select_clip ",clip->name)

	g_signal_emit_by_name (clip->list_box_row, "activate");

	return G_SOURCE_REMOVE;
}

void parse_osc_hyperdeck (hyperdeck_t *hyperdeck, char *buffer)
{
	clip_list_t *clip;
	int clip_id = -1;

	LOG_OSC_2_STRINGS("parse_osc_hyperdeck ",buffer)

	if (strcmp (buffer, "Play") == 0) g_idle_add ((GSourceFunc)hyperdeck_play, hyperdeck);
	else if (strcmp (buffer, "Stop") == 0) g_idle_add ((GSourceFunc)hyperdeck_stop, hyperdeck);
	else if (strcmp (buffer, "Loop") == 0) g_idle_add ((GSourceFunc)hyperdeck_loop, hyperdeck);
	else if (strcmp (buffer, "Single_clip_True_Loop_True") == 0) g_idle_add ((GSourceFunc)hyperdeck_single_clip_true_loop_true, hyperdeck);
	else if (strcmp (buffer, "Single_clip_True_Loop_False") == 0) g_idle_add ((GSourceFunc)hyperdeck_single_clip_true_loop_false, hyperdeck);
	else if (strcmp (buffer, "Single_clip_False_Loop_False") == 0) g_idle_add ((GSourceFunc)hyperdeck_single_clip_false_loop_false, hyperdeck);
	else if (strcmp (buffer, "Single_clip_False_Loop_True") == 0) g_idle_add ((GSourceFunc)hyperdeck_single_clip_false_loop_true, hyperdeck);
	else if (strcmp (buffer, "Select_Slot_1") == 0) select_slot_1 (NULL, NULL, hyperdeck);
	else if (strcmp (buffer, "Select_Slot_2") == 0) select_slot_2 (NULL, NULL, hyperdeck);
	else if (strcmp (buffer, "Up") == 0) g_idle_add ((GSourceFunc)select_clip_up, hyperdeck);
	else if (strcmp (buffer, "Down") == 0) g_idle_add ((GSourceFunc)select_clip_down, hyperdeck);
	else {
		for (clip = hyperdeck->list_of_clips; clip != NULL; clip = clip->next) {
			if (strcmp (buffer, clip->name) == 0) {
				g_idle_add ((GSourceFunc)select_clip, clip);

				break;
			}
		}

		if ((clip == NULL) && (scanf (buffer, "%d", &clip_id) == 1)) {
			for (clip = hyperdeck->list_of_clips; clip != NULL; clip = clip->next) {
				if (clip->id == clip_id) {
					g_idle_add ((GSourceFunc)select_clip, clip);

					break;
				}
			}
		}
	}
}

gboolean fresque_loop (void)
{
	if (current_fresque != NULL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fresques_loop_button), !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fresques_loop_button)));

	return G_SOURCE_REMOVE;
}

gboolean fresque_loop_on (void)
{
	if (current_fresque != NULL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fresques_loop_button), TRUE);

	return G_SOURCE_REMOVE;
}

gboolean fresque_loop_off (void)
{
	if (current_fresque != NULL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fresques_loop_button), FALSE);

	return G_SOURCE_REMOVE;
}

gboolean fresque_play (void)
{
	if (current_fresque != NULL) gtk_button_clicked (GTK_BUTTON (fresques_play_button));

	return G_SOURCE_REMOVE;
}

gboolean fresque_stop (void)
{
	if (current_fresque != NULL) gtk_button_clicked (GTK_BUTTON (fresques_stop_button));

	return G_SOURCE_REMOVE;
}

gboolean select_fresque (fresque_t *fresque)
{
	LOG_OSC_2_STRINGS("select_fresque ",fresque->name)

	gtk_list_box_select_row (GTK_LIST_BOX (fresques_list_box), GTK_LIST_BOX_ROW (fresque->list_box_row));

	return G_SOURCE_REMOVE;
}

void parse_osc_fresque (char *buffer)
{
	fresque_t *fresque;

	LOG_OSC_2_STRINGS("parse_osc_fresque ",buffer)

	if (strcmp (buffer, "Loop") == 0) g_idle_add ((GSourceFunc)fresque_loop, NULL);
	else if (strcmp (buffer, "Loop_On") == 0) g_idle_add ((GSourceFunc)fresque_loop_on, NULL);
	else if (strcmp (buffer, "Loop_Off") == 0) g_idle_add ((GSourceFunc)fresque_loop_off, NULL);
	else if (strcmp (buffer, "Play") == 0) g_idle_add ((GSourceFunc)fresque_play, NULL);
	else if (strcmp (buffer, "Stop") == 0) g_idle_add ((GSourceFunc)fresque_stop, NULL);
	else if (strcmp (buffer, "Up") == 0) g_idle_add ((GSourceFunc)select_fresque_up, NULL);
	else if (strcmp (buffer, "Down") == 0) g_idle_add ((GSourceFunc)select_fresque_down, NULL);
	else {
		for (fresque = fresques; fresque != NULL; fresque = fresque->next) {
			if (strcmp (buffer, fresque->name) == 0) {
				g_idle_add ((GSourceFunc)select_fresque, fresque);

				break;
			}
		}
	}
}

gboolean select_preset (GtkButton *button)
{
	LOG_OSC_2_STRINGS("select_preset ",gtk_button_get_label (button))

	gtk_button_clicked (button);

	return G_SOURCE_REMOVE;
}

void parse_osc_preset (char *buffer)
{
	int i;

	LOG_OSC_2_STRINGS("parse_osc_preset ",buffer)

	for (i = 0; i < NB_OF_PRESETS; i++) {
		if (strcmp (buffer, default_presets_name[i] + 7) == 0) {
			if (presets[i].switched_on) g_idle_add ((GSourceFunc)select_preset, presets[i].button);

			break;
		}

		if (strcmp (buffer, gtk_button_get_label (GTK_BUTTON (presets[i].button))) == 0) {
			if (presets[i].switched_on) g_idle_add ((GSourceFunc)select_preset, presets[i].button);

			break;
		}
	}
}

void parse_osc_message (char *buffer)
{
	int i;

	LOG_OSC_2_STRINGS("parse_osc_message ",buffer)

	i = 0;
	while ((buffer[i] != '/') && (buffer[i] != '\0')) i++;

#if NB_OF_HYPERDECKS == 1
	if ((i == 11) && (memcmp (buffer, "HyperDeck_1/", 12) == 0)) parse_osc_hyperdeck (hyperdecks, buffer + 12);
#else
	if ((i == 11) && (buffer[11] == '/')) {				//HyperDecks
		if (memcmp (buffer, "HyperDeck_", 10) == 0) {
			for (i = 0; i < NB_OF_HYPERDECKS; i++) {
				if (buffer[10] == ('1' + i)) {
					parse_osc_hyperdeck (hyperdecks + i, buffer + 12);

					break;
				}
			}
		}
	} else if (memcmp (buffer, "Fresque/", 8) == 0) {	//Fresques
		parse_osc_fresque (buffer + 8);
	} else if (memcmp (buffer, "Preset/", 7) == 0) {	//Presets
		parse_osc_preset (buffer + 7);
	}
#endif
}

void parse_osc_packet (char *buffer, int size)
{
	char osc_bundle[8] = "#bundle";
	int element_size, bundle_itr = 16;

	if ((size > 1) && (buffer[0] == '/')) parse_osc_message (buffer + 1);
	else if (size > 20) {
		if (*((guint64 *)buffer) == *((guint64 *)osc_bundle)) {
			do {
				((char *)&element_size)[3] = buffer[bundle_itr++];
				((char *)&element_size)[2] = buffer[bundle_itr++];
				((char *)&element_size)[1] = buffer[bundle_itr++];
				((char *)&element_size)[0] = buffer[bundle_itr++];

				parse_osc_packet (buffer + bundle_itr, element_size);

				bundle_itr += element_size;
			} while (bundle_itr < size - 4);
		}
	}
}

gpointer receive_osc_packet (void)
{
	struct sockaddr_in src_addr;
	socklen_t addrlen;
	char packet[2048];
	int size;

	LOG_OSC_STRING("receive_osc_packet ()")

	addrlen = sizeof (src_addr);

	while ((size = recvfrom (osc_socket, packet, sizeof (packet) - 1, 0, (struct sockaddr *)&src_addr, &addrlen)) > 1) {
		packet[size] = '\0';
		LOG_OSC_PACKET(src_addr.sin_addr,packet,size)

		parse_osc_packet (packet, size);

		addrlen = sizeof (src_addr);
	}

	LOG_OSC_STRING("receive_osc_packet () return")

	return NULL;
}

void init_osc (void)
{
	memset (&osc_address, 0, sizeof (struct sockaddr_in));

	osc_address.sin_family = AF_INET;
	osc_address.sin_port = htons (OSC_UDP_PORT);
	osc_address.sin_addr.s_addr = htonl (INADDR_ANY);
}

void start_osc (void)
{
	LOG_OSC_STRING("start_osc ()")

	osc_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind (osc_socket, (struct sockaddr *)&osc_address, sizeof (struct sockaddr_in));

	osc_thread = g_thread_new (NULL, (GThreadFunc)receive_osc_packet, NULL);
}

void stop_osc (void)
{
	LOG_OSC_STRING("stop_osc ()")

	shutdown (osc_socket, SHUT_RD);
	closesocket (osc_socket);

	if (osc_thread != NULL) {
		g_thread_join (osc_thread);
		osc_thread = NULL;
	}
}

