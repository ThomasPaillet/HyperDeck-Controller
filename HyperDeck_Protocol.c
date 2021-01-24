/*
 * copyright (c) 2018-2021 Thomas Paillet <thomas.paillet@net-c.fr

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
 * along with HyperDeck-Controller.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdio.h>

#include "HyperDeck.h"
#include "HyperDeck_Protocol.h"


typedef struct {
	int code;
	gchar *text;
	gboolean (*func)(hyperdeck_t *hyperdeck);
} response_t;


char* msg_play_single_loop[4] = {msg_play_single_clip_true_loop_true, msg_play_single_clip_true_loop_false, \
				msg_play_single_clip_false_loop_false, msg_play_single_clip_false_loop_true};
int msg_play_single_loop_len[4] = {35, 36, 37, 36};


void next_line (hyperdeck_t *hyperdeck)
{
	while (hyperdeck->buffer[hyperdeck->index] != '\n') hyperdeck->index++;
	hyperdeck->index++;
}

void clean_hyperdeck (hyperdeck_t *hyperdeck)
{
	clip_list_t *clip_list_tmp;
	fresque_t *fresque_tmp;

	for (fresque_tmp = fresques; fresque_tmp != NULL; fresque_tmp = fresque_tmp->next) {
		fresque_tmp->clips_id[hyperdeck->number] = 0;
	}

	hyperdeck->clip_count = 0;
	hyperdeck->default_preset_clip_id = 0;

	while (hyperdeck->list_of_clips != NULL) {
		clip_list_tmp = hyperdeck->list_of_clips;
		hyperdeck->list_of_clips = clip_list_tmp->next;
		gtk_container_remove (GTK_CONTAINER (hyperdeck->list_box), clip_list_tmp->list_box_row);
		g_free (clip_list_tmp);
	}
}

void set_hyperdeck_button_insensitive (hyperdeck_t *hyperdeck)
{
	gtk_widget_set_sensitive (hyperdeck->play_button, FALSE);
	gtk_widget_set_opacity (hyperdeck->play_button, 0.75);
	gtk_widget_set_sensitive (hyperdeck->stop_button, FALSE);
	gtk_widget_set_opacity (hyperdeck->stop_button, 0.75);
	gtk_widget_set_sensitive (hyperdeck->single_loop_button, FALSE);
	gtk_widget_set_opacity (hyperdeck->single_loop_button, 0.75);
	gtk_widget_set_sensitive (hyperdeck->del_button, FALSE);
	gtk_widget_set_opacity (hyperdeck->del_button, 0.75);
}

gboolean response_timeline_empty (hyperdeck_t *hyperdeck)								//107 timeline empty
{
	set_hyperdeck_button_insensitive (hyperdeck);
	clean_hyperdeck (hyperdeck);
	clean_fresques ();

	if (hyperdeck->timeline_empty_retry < 3) {
		g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, hyperdeck);
		hyperdeck->timeline_empty_retry++;
	} else hyperdeck->timeline_empty_retry = 0;

g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_remote_control_disabled (hyperdeck_t *hyperdeck)						//111 remote control disabled
{
	gtk_widget_set_opacity (hyperdeck->root_widget, 0.75);
	show_message_window (((response_t *)(hyperdeck->response))->text);

g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_connection_rejected (hyperdeck_t *hyperdeck)							//120 connection rejected
{
	show_message_window (((response_t *)(hyperdeck->response))->text);
	hyperdeck->connected = FALSE;

g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_slot_info (hyperdeck_t *hyperdeck)									//202 slot info:
{
	int slot_id;
	char status[16];
	char slot_video_format[16];
	char msg[64];
	int msg_len;

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "slot id: ", 7) == 0) {			//slot id:
			sscanf (&hyperdeck->buffer[hyperdeck->index], "slot id: %d\n", &slot_id);
DEBUG_HYPERDECK_S ("slot id:")
DEBUG_HYPERDECK_D (slot_id)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "status: ", 6) == 0) {			//status: {“empty”, “mounting”, “error”, “mounted”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "status: %s\n", status);
DEBUG_HYPERDECK_S ("status:")
DEBUG_HYPERDECK_S (status)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "volume name: ", 11) == 0) {		//volume name:
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "recording time: ", 14) == 0) {	//recording time:
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "video format: ", 12) == 0) {	//video format: {"PAL", "NTSC", "none"}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "video format: %s\n", slot_video_format);
DEBUG_HYPERDECK_S ("video format:")
DEBUG_HYPERDECK_S (slot_video_format)
			continue;
		}
	}

	if (strcmp (status, "mounted") == 0) {
		if (slot_id == 1) {
			hyperdeck->slot_1_is_mounted = TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1), pixbuf_S1F);
			if (hyperdeck->slot_selected == 1) {
				if (strcmp (slot_video_format, video_format) != 0) {
					msg_len = sprintf (msg, msg_slot_select_video_format_, video_format);
					send (hyperdeck->socket, msg, msg_len, 0);
					SLEEP (1)
				}
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
			}
			SEND (hyperdeck, msg_disk_list_slot_id_1)
		}
		if (slot_id == 2) {
			hyperdeck->slot_2_is_mounted = TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2), pixbuf_S2F);
			if (hyperdeck->slot_selected == 2) {
				if (strcmp (slot_video_format, video_format) != 0) {
					msg_len = sprintf (msg, msg_slot_select_video_format_, video_format);
					send (hyperdeck->socket, msg, msg_len, 0);
					SLEEP (1)
				}
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
			}
			SEND (hyperdeck, msg_disk_list_slot_id_2)
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_device_info (hyperdeck_t *hyperdeck)									//204 device info:
{
	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "model: ", 5) == 0) {			//model: {Model Name}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "protocol version: ", 7) == 0) {	//protocol version: HYPERDECK_PROTOCOL_VERSION
			sscanf (&hyperdeck->buffer[hyperdeck->index], "protocol version: %s\n", hyperdeck->protocol_version);
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "unique id: ", 9) == 0) {		//unique id: {unique alphanumeric identifier}
			continue;
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

void add_clip_to_hyperdeck (clip_list_t *clip, hyperdeck_t *hyperdeck)
{
	GtkWidget *list_box_row_event_box, *list_box_row_label;
	fresque_t *fresque_tmp;
	fresque_batch_t *fresque_batch_tmp;
	int name_len;
	char name[CLIP_NAME_LENGTH];

	clip->next = hyperdeck->list_of_clips;
	hyperdeck->list_of_clips = clip;
DEBUG_HYPERDECK_S ("clip_name")
DEBUG_HYPERDECK_S (hyperdeck->list_of_clips->name)

	clip->list_box_row = gtk_list_box_row_new ();
	gtk_container_add (GTK_CONTAINER (hyperdeck->list_box), clip->list_box_row);

	list_box_row_event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (clip->list_box_row), list_box_row_event_box);
	g_signal_connect (G_OBJECT (list_box_row_event_box), "button_press_event", G_CALLBACK (delete_clip), hyperdeck);

	list_box_row_label = gtk_label_new (clip->name);
	gtk_container_add (GTK_CONTAINER (list_box_row_event_box), list_box_row_label);

	gtk_widget_show_all (clip->list_box_row);

	name_len = strlen (clip->name);
	name_len--;

	if (clip->name[name_len] == (char)hyperdeck->number + 49) {
		memcpy (name, clip->name, name_len);
		name[name_len++] = '\0';

		for (fresque_tmp = fresques; fresque_tmp != NULL; fresque_tmp = fresque_tmp->next)
			if (memcmp (fresque_tmp->name, name, name_len) == 0) break;

		if (fresque_tmp != NULL) {
			fresque_tmp->clips_id[hyperdeck->number] = clip->id;
DEBUG_HYPERDECK_S ("fresque_name")
DEBUG_HYPERDECK_S (fresque_tmp->name)
		} else {
			fresque_tmp = g_malloc (sizeof (fresque_t));
			memset (fresque_tmp->clips_id, 0, sizeof (int) * NB_OF_HYPERDECKS);
			fresque_tmp->clips_id[hyperdeck->number] = clip->id;

			list_box_row_label = gtk_label_new (name);
			fresque_tmp->name = gtk_label_get_text (GTK_LABEL (list_box_row_label));
			list_box_row_event_box = gtk_event_box_new ();
			g_signal_connect (G_OBJECT (list_box_row_event_box), "button_press_event", G_CALLBACK (delete_fresque), NULL);
			fresque_tmp->list_box_row = gtk_list_box_row_new ();

			gtk_container_add (GTK_CONTAINER (list_box_row_event_box), list_box_row_label);
			gtk_container_add (GTK_CONTAINER (fresque_tmp->list_box_row), list_box_row_event_box);

g_mutex_lock (&fresque_batch_mutex);
DEBUG_HYPERDECK_S ("fresque_batch")
			for (fresque_batch_tmp = fresque_batches; fresque_batch_tmp != NULL; fresque_batch_tmp = fresque_batch_tmp->next) {
DEBUG_HYPERDECK_S (fresque_batch_tmp->name)
DEBUG_HYPERDECK_S (name)
DEBUG_HYPERDECK_D (name_len)
				if (memcmp (fresque_batch_tmp->name, name, strlen (fresque_batch_tmp->name)) == 0) {
					if (fresque_batch_tmp->initialized == FALSE) initialize_fresque_batch (fresque_batch_tmp);
DEBUG_HYPERDECK_S ("trouvé")
					gtk_container_add (GTK_CONTAINER (fresque_batch_tmp->list_box), fresque_tmp->list_box_row);
					fresque_batch_tmp->nb_fresques++;
					fresque_tmp->parent_fresque_batch = fresque_batch_tmp;
					break;
				}
			}
g_mutex_unlock (&fresque_batch_mutex);
			if (fresque_batch_tmp == NULL) {
				gtk_container_add (GTK_CONTAINER (fresques_list_box), fresque_tmp->list_box_row);
				fresques_list_box_num++;
				fresque_tmp->parent_fresque_batch = NULL;
			}

			gtk_widget_show_all (fresque_tmp->list_box_row);

			fresque_tmp->next = fresques;
			fresques = fresque_tmp;

DEBUG_HYPERDECK_S ("nouvelle fresque:")
DEBUG_HYPERDECK_S (fresques->name)
		}
	}
}

void response_clips_info_suite (hyperdeck_t *hyperdeck, int *clip_count)
{
	int i;
	clip_list_t *clip_list_tmp;

DEBUG_HYPERDECK_S ("\nresponse_clips_info_suite")
	hyperdeck->recv_len = recv (hyperdeck->socket, hyperdeck->buffer, sizeof (hyperdeck->buffer), 0);

	if (hyperdeck->recv_len <= 0) {
		hyperdeck->connected = FALSE;
		hyperdeck->clip_count = *clip_count;
DEBUG_HYPERDECK_S ("recv_len <= 0")
		return;
	}
hyperdeck->buffer[hyperdeck->recv_len] = '\0';
DEBUG_HYPERDECK_S ("[buffer]")
DEBUG_HYPERDECK_S (hyperdeck->buffer)

	hyperdeck->index = 0;
	while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
		clip_list_tmp = g_malloc (sizeof (clip_list_t));
		sscanf (&hyperdeck->buffer[hyperdeck->index], "%d", &clip_list_tmp->id);
DEBUG_HYPERDECK_S ("clip_id")
DEBUG_HYPERDECK_D (clip_list_tmp->id)
		while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
		hyperdeck->index++;
		i = hyperdeck->index;
		while (hyperdeck->buffer[i] != '\n') i++;
		hyperdeck->buffer[i - 29] = '\0';
		strcpy (clip_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
		hyperdeck->index = i + 1;

		add_clip_to_hyperdeck (clip_list_tmp, hyperdeck);

		*clip_count = *clip_count + 1;
	}
DEBUG_HYPERDECK_S ("response_clips_info_suite end")
}

gboolean response_clips_info (hyperdeck_t *hyperdeck)										//205 clips info:
{
	int i;
	int clip_count;
	clip_list_t *clip_list_tmp;

	clean_hyperdeck (hyperdeck);

	clip_count = 0;

	sscanf (&hyperdeck->buffer[hyperdeck->index], "clip count: %d\n", &hyperdeck->clip_count);	//clip count:
DEBUG_HYPERDECK_S ("clip count:")
DEBUG_HYPERDECK_D (hyperdeck->clip_count)
	next_line (hyperdeck);

	while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
		clip_list_tmp = g_malloc (sizeof (clip_list_t));
		sscanf (&hyperdeck->buffer[hyperdeck->index], "%d", &clip_list_tmp->id);
DEBUG_HYPERDECK_S ("clip_id")
DEBUG_HYPERDECK_D (clip_list_tmp->id)
		while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
		hyperdeck->index++;
		i = hyperdeck->index;
		while (hyperdeck->buffer[i] != '\n') i++;
		hyperdeck->buffer[i - 29] = '\0';
		strcpy (clip_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
		hyperdeck->index = i + 1;

		add_clip_to_hyperdeck (clip_list_tmp, hyperdeck);

		clip_count++;
	}

	while (clip_count < hyperdeck->clip_count) response_clips_info_suite (hyperdeck, &clip_count);

	clean_fresques ();

	gtk_widget_set_sensitive (hyperdeck->play_button, TRUE);
	gtk_widget_set_opacity (hyperdeck->play_button, 1.0);
	gtk_widget_set_sensitive (hyperdeck->stop_button, TRUE);
	gtk_widget_set_opacity (hyperdeck->stop_button, 1.0);
	gtk_widget_set_sensitive (hyperdeck->single_loop_button, TRUE);
	gtk_widget_set_opacity (hyperdeck->single_loop_button, 1.0);
	gtk_widget_set_sensitive (hyperdeck->del_button, TRUE);
	gtk_widget_set_opacity (hyperdeck->del_button, 1.0);

	hyperdeck->timeline_empty_retry = 0;

	for (i = 0, clip_list_tmp = hyperdeck->list_of_clips; i < NB_OF_PRESETS; i++, clip_list_tmp = hyperdeck->list_of_clips) {
		if ((!presets[i].switched_on) || (presets[i].clips[hyperdeck->number].slot != hyperdeck->slot_selected)) continue;
		if ((presets[i].clips[hyperdeck->number].name[0] == ' ') && (presets[i].clips[hyperdeck->number].name[1] == '\0')) continue;

		while ((clip_list_tmp != NULL) && (strcmp (presets[i].clips[hyperdeck->number].name, clip_list_tmp->name) != 0))
			clip_list_tmp = clip_list_tmp->next;

		if (clip_list_tmp == NULL) {
			g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, hyperdeck);
			break;
		}
	}

g_mutex_lock (&hyperdeck->last_file_dropped_mutex);
	if ((clip_list_tmp != NULL) && (hyperdeck->last_file_dropped != NULL)) {
		for (clip_list_tmp = hyperdeck->list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
			if (strcmp (hyperdeck->last_file_dropped, clip_list_tmp->name) == 0) break;
		}
		if (clip_list_tmp == NULL) g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, hyperdeck);

		g_free (hyperdeck->last_file_dropped);
		hyperdeck->last_file_dropped = NULL;
	}
g_mutex_unlock (&hyperdeck->last_file_dropped_mutex);

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

void response_disk_list_suite (hyperdeck_t *hyperdeck)
{
	int k, l;
	disk_list_t *disk_list_tmp;

	if (hyperdeck->disk_slot_id == 1) {
		while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
			while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
			hyperdeck->index++;
			for (k = hyperdeck->index; hyperdeck->buffer[k] != '\n'; k++) {}
			for (l = 0; l < 3; l++) {
				while (hyperdeck->buffer[k] != ' ') k--;
				k--;
			}
			k -= 3;
			hyperdeck->buffer[k] = '\0';
			disk_list_tmp = g_malloc (sizeof (clip_list_t));
			strcpy (disk_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
			disk_list_tmp->next = hyperdeck->slot_1_disk_list;
			hyperdeck->slot_1_disk_list = disk_list_tmp;
DEBUG_HYPERDECK_S ("disk_clip_name")
DEBUG_HYPERDECK_S (hyperdeck->slot_1_disk_list->name)
			hyperdeck->index = k;
			next_line (hyperdeck);
		}
	} else if (hyperdeck->disk_slot_id == 2) {
		while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
			while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
			hyperdeck->index++;
			for (k = hyperdeck->index; hyperdeck->buffer[k] != '\n'; k++) {}
			for (l = 0; l < 3; l++) {
				while (hyperdeck->buffer[k] != ' ') k--;
				k--;
			}
			k -= 3;
			hyperdeck->buffer[k] = '\0';
			disk_list_tmp = g_malloc (sizeof (clip_list_t));
			strcpy (disk_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
			disk_list_tmp->next = hyperdeck->slot_2_disk_list;
			hyperdeck->slot_2_disk_list = disk_list_tmp;
DEBUG_HYPERDECK_S ("disk_clip_name")
DEBUG_HYPERDECK_S (hyperdeck->slot_2_disk_list->name)
			hyperdeck->index = k;
			next_line (hyperdeck);
		}
	}

	next_line (hyperdeck);
}

gboolean response_disk_list (hyperdeck_t *hyperdeck)									//206 disk list:
{
	int k, l;
	int slot_id;
	disk_list_t *disk_list_tmp;

	sscanf (&hyperdeck->buffer[hyperdeck->index], "slot id: %d\n", &slot_id);				//slot id:
DEBUG_HYPERDECK_S ("slot id:")
DEBUG_HYPERDECK_D (slot_id)
	next_line (hyperdeck);

	if (slot_id == 1) {
		hyperdeck->disk_slot_id = 1;
		while (hyperdeck->slot_1_disk_list != NULL) {
			disk_list_tmp = hyperdeck->slot_1_disk_list;
			hyperdeck->slot_1_disk_list = disk_list_tmp->next;
			g_free (disk_list_tmp);
		}
		
		while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
			while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
			hyperdeck->index++;
			for (k = hyperdeck->index; hyperdeck->buffer[k] != '\n'; k++) {}
			for (l = 0; l < 3; l++) {
				while (hyperdeck->buffer[k] != ' ') k--;
				k--;
			}
			k -= 3;
			hyperdeck->buffer[k] = '\0';
			disk_list_tmp = g_malloc (sizeof (clip_list_t));
			strcpy (disk_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
			disk_list_tmp->next = hyperdeck->slot_1_disk_list;
			hyperdeck->slot_1_disk_list = disk_list_tmp;
DEBUG_HYPERDECK_S ("disk_clip_name")
DEBUG_HYPERDECK_S (hyperdeck->slot_1_disk_list->name)
			hyperdeck->index = k;
			next_line (hyperdeck);
		}
	} else if (slot_id == 2) {
		hyperdeck->disk_slot_id = 2;
		while (hyperdeck->slot_2_disk_list != NULL) {
			disk_list_tmp = hyperdeck->slot_2_disk_list;
			hyperdeck->slot_2_disk_list = disk_list_tmp->next;
			g_free (disk_list_tmp);
		}
		
		while ((hyperdeck->buffer[hyperdeck->index] != '\r') && (hyperdeck->index < hyperdeck->recv_len - 3)) {
			while (hyperdeck->buffer[hyperdeck->index] != ' ') hyperdeck->index++;
			hyperdeck->index++;
			for (k = hyperdeck->index; hyperdeck->buffer[k] != '\n'; k++) {}
			for (l = 0; l < 3; l++) {
				while (hyperdeck->buffer[k] != ' ') k--;
				k--;
			}
			k -= 3;
			hyperdeck->buffer[k] = '\0';
			disk_list_tmp = g_malloc (sizeof (clip_list_t));
			strcpy (disk_list_tmp->name, &hyperdeck->buffer[hyperdeck->index]);
			disk_list_tmp->next = hyperdeck->slot_2_disk_list;
			hyperdeck->slot_2_disk_list = disk_list_tmp;
DEBUG_HYPERDECK_S ("disk_clip_name")
DEBUG_HYPERDECK_S (hyperdeck->slot_2_disk_list->name)
			hyperdeck->index = k;
			next_line (hyperdeck);
		}
	} else hyperdeck->disk_slot_id = 0;

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_transport_info (hyperdeck_t *hyperdeck)								//208 transport info:
{
	char text[16];
	int clip_id, slot_id;
	clip_list_t *clip_list_tmp;
	gboolean play, preview, loop, single_clip, hyperdeck_loop;
	char msg[32];
	int msg_len;

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "status: ", 6) == 0) {			//status: {“stopped”, “play”, ...}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "status: %s\n", text);
DEBUG_HYPERDECK_S ("status: ")
DEBUG_HYPERDECK_S (text)
			if (strcmp (text, "play") == 0) {
				play = TRUE;
				preview = FALSE;
			} else {
				play = FALSE;
				if (strcmp (text, "preview") == 0) preview = TRUE;
				else preview = FALSE;
			}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "speed: ", 5) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "slot id: ", 7) == 0) {			//slot id: {slot id or none}
			if (sscanf (&hyperdeck->buffer[hyperdeck->index], "slot id: %d\n", &slot_id) != 1) slot_id = 0;
DEBUG_HYPERDECK_S ("slot id: ")
DEBUG_HYPERDECK_D (slot_id)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "display timecode: ", 16) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "timecode: ", 8) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "clip id: ", 7) == 0) {			//clip id: {clip id or none}
			if (sscanf (&hyperdeck->buffer[hyperdeck->index], "clip id: %d\n", &clip_id) == 1) {
DEBUG_HYPERDECK_S ("clip id: ")
DEBUG_HYPERDECK_D (clip_id)
				for (clip_list_tmp = hyperdeck->list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
					if (clip_id == clip_list_tmp->id) {
						gtk_list_box_select_row (GTK_LIST_BOX (hyperdeck->list_box), GTK_LIST_BOX_ROW (clip_list_tmp->list_box_row));
						break;
					}
				}
			} else clip_id = 0;
			continue;
		}

		if (strncmp (hyperdeck->buffer + hyperdeck->index, "video format: ", 12) == 0) {	//video format: {}
/*			if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "PAL", 3) == 0) {
				hyperdeck->video_format.nb_lines = 576;
				if (hyperdeck->buffer[hyperdeck->index + 17] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				hyperdeck->video_format.frequency = 25.0;
				hyperdeck->video_format.label = "SD";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "NTSC", 4) == 0) {
				hyperdeck->video_format.nb_lines = 480;
				if (hyperdeck->buffer[hyperdeck->index + 18] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				hyperdeck->video_format.frequency = 30.0;
				hyperdeck->video_format.label = "SD NTSC";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "1080", 4) == 0) {
				hyperdeck->video_format.nb_lines = 1080;
				if (hyperdeck->buffer[hyperdeck->index + 18] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "23", 2) == 0) hyperdeck->video_format.frequency = 23.976;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "24", 2) == 0) hyperdeck->video_format.frequency = 24.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "25", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "29", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "30", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "50", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "59", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "60", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				hyperdeck->video_format.label = "HD";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "4K", 2) == 0) {
				hyperdeck->video_format.nb_lines = 2160;
				if (hyperdeck->buffer[hyperdeck->index + 16] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "23", 2) == 0) hyperdeck->video_format.frequency = 23.976;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "24", 2) == 0) hyperdeck->video_format.frequency = 24.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "25", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "29", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "30", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "50", 2) == 0) hyperdeck->video_format.frequency = 50.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "59", 2) == 0) hyperdeck->video_format.frequency = 59.94;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "60", 2) == 0) hyperdeck->video_format.frequency = 60.0;
				hyperdeck->video_format.label = "UHD 4K";
			}
DEBUG_HYPERDECK_S ("video format: ")
DEBUG_HYPERDECK_D (hyperdeck->video_format.nb_lines)
DEBUG_HYPERDECK_D ((int)hyperdeck->video_format.progressif)
DEBUG_HYPERDECK_F (hyperdeck->video_format.frequency)*/
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "loop: ", 4) == 0) {			//loop: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "loop: %s\n", text);
			if (strcmp (text, "true") == 0) loop = TRUE;
			else loop = FALSE;
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "single clip: ", 11) == 0) {		//single clip: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "single clip: %s\n", text);
			if (strcmp (text, "true") == 0) single_clip = TRUE;
			else single_clip = FALSE;
			continue;
		}
	}

	if (hyperdeck->slot_selected != slot_id) {
		hyperdeck->slot_selected = slot_id;
		if (slot_id == 1) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1S);
		} else if (slot_id == 2) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2S);
		} else {
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);
			gtk_widget_set_sensitive (hyperdeck->play_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->play_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->stop_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->stop_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->single_loop_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->single_loop_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->del_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->del_button, 0.75);
		}
	}

	if (hyperdeck->play != play) {
		hyperdeck->play = play;
		if (play) gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_play_button), pixbuf_BPOn);
		else gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_play_button), pixbuf_BPOff);
	}

	if (single_clip) {
		if (loop) hyperdeck_loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
		else hyperdeck_loop = SINGLE_CLIP_TRUE_LOOP_FALSE;
	} else {
		if (loop) hyperdeck_loop = SINGLE_CLIP_FALSE_LOOP_TRUE;
		else hyperdeck_loop = SINGLE_CLIP_FALSE_LOOP_FALSE;
	}

	if (hyperdeck->loop != hyperdeck_loop) {
		hyperdeck->loop = hyperdeck_loop;
		gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck_loop]);
	}

	if ((play == FALSE) && (clip_id == 1)) {
		msg_len = sprintf (msg, "%s%d\n", msg_goto_clip_id_, clip_id);
DEBUG_HYPERDECK_S (msg)
		send (hyperdeck->socket, msg, msg_len, 0);
	}

	if (preset_to_load != NULL) load_preset ();

	if (preview) send (hyperdeck->socket, msg_preview_enable_false, 22, 0);

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_notify (hyperdeck_t *hyperdeck)										//209 notify:
{
	char text[8];

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "transport: ", 9) == 0) {		//transport: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "transport: %s\n", text);
DEBUG_HYPERDECK_S ("transport: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "slot: ", 4) == 0) {			//slot: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "slot: %s\n", text);
DEBUG_HYPERDECK_S ("slot: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "remote: ", 6) == 0) {			//remote: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "remote: %s\n", text);
DEBUG_HYPERDECK_S ("remote: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "configuration: ", 13) == 0) {	//configuration: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "configuration: %s\n", text);
DEBUG_HYPERDECK_S ("configuration: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "dropped frames: ", 14) == 0) {	//dropped frames: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "dropped frames: %s\n", text);
DEBUG_HYPERDECK_S ("dropped frames: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_configuration (hyperdeck_t *hyperdeck)								//211 configuration:
{
	char msg[64];
	int msg_len;

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "audio input: ", 11) == 0) {		//audio input: {“embedded”, “XLR”, “RCA”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "audio input: %s\n", msg);
DEBUG_HYPERDECK_S ("audio input: ")
DEBUG_HYPERDECK_S (msg)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "video input: ", 11) == 0) {		//video input: {“SDI”, “HDMI”, “component”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "video input: %s\n", msg);
DEBUG_HYPERDECK_S ("video input: ")
DEBUG_HYPERDECK_S (msg)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "file format: ", 11) == 0) {		//file format: {QuickTimeProResHQ, QuickTimeProRes, QuickTimeProResLT, QuickTimeProResProxy, QuickTimeDNxHD220, QuickTimeDNxHR_HQX, ... DNxHD220}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "file format: %s\n", msg);
DEBUG_HYPERDECK_S ("file format: ")
DEBUG_HYPERDECK_S (msg)
			if (strcmp (msg, file_format) != 0) {
				msg_len = sprintf (msg, msg_configuration_file_format_, file_format);
				send (hyperdeck->socket, msg, msg_len, 0);
				SLEEP(1)
			}
			continue;
		}
	}

	SEND (hyperdeck, msg_transport_info)
	SEND (hyperdeck, msg_slot_info_id_1)
	SEND (hyperdeck, msg_slot_info_id_2)
	SEND (hyperdeck, msg_notify_slot_true)
	SEND (hyperdeck, msg_notify_transport_true)
	SEND (hyperdeck, msg_notify_configuration_true)
	SEND (hyperdeck, msg_notify_remote_true)

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_deck_rebooting (hyperdeck_t *hyperdeck)								//213 deck rebooting
{
	show_message_window (((response_t *)(hyperdeck->response))->text);
	hyperdeck->reboot = TRUE;
	hyperdeck->connected = FALSE;

g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_uptime (hyperdeck_t *hyperdeck)										//215 uptime:
{
	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_asynchronous_connection_info (hyperdeck_t *hyperdeck)					//500 connection info:
{
	char msg[128];

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "model: ", 5) == 0) {			//model: Blackmagic HyperDeck Studio Mini
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "protocol version: ", 7) == 0) {	//protocol version: HYPERDECK_PROTOCOL_VERSION
			sscanf (&hyperdeck->buffer[hyperdeck->index], "protocol version: %s\n", hyperdeck->protocol_version);
			if (strcmp (hyperdeck->protocol_version, HYPERDECK_PROTOCOL_VERSION) != 0) {
				sprintf (msg, "Hyperdeck n°%d protocol version: %s\nSoftware protocol version: %s", hyperdeck->number + 1, hyperdeck->protocol_version, HYPERDECK_PROTOCOL_VERSION);
				show_message_window (msg);
			}
			continue;
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_asynchronous_slot_info (hyperdeck_t *hyperdeck)						//502 slot info:
{
	int slot_id;
	char status[16];
	disk_list_t *disk_list_tmp;

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "slot id: ", 7) == 0) {			//slot id:
			sscanf (&hyperdeck->buffer[hyperdeck->index], "slot id: %d\n", &slot_id);
DEBUG_HYPERDECK_S ("slot id:")
DEBUG_HYPERDECK_D (slot_id)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "status: ", 6) == 0) {			//status: {“empty”, “mounting”, “error”, “mounted”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "status: %s\n", status);
DEBUG_HYPERDECK_S ("status:")
DEBUG_HYPERDECK_S (status)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "volume name: ", 11) == 0) {		//volume name:
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "recording time: ", 14) == 0) {	//recording time:
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "video format: ", 12) == 0) {	//video format: {"PAL", "NTSC", "none"}
			continue;
		}
	}

	if (strcmp (status, "mounted") == 0) {
		if (slot_id == 1) {
			hyperdeck->slot_1_is_mounted = TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1), pixbuf_S1F);
			if (hyperdeck->slot_selected == 0) {
				hyperdeck->slot_selected = 1;
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1S);
			}
			SEND (hyperdeck, msg_disk_list_slot_id_1)
		}
		if (slot_id == 2) {
			hyperdeck->slot_2_is_mounted = TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2), pixbuf_S2F);
			if (hyperdeck->slot_selected == 0) {
				hyperdeck->slot_selected = 2;
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2S);
			}
			SEND (hyperdeck, msg_disk_list_slot_id_2)
		}
	} else if (strcmp (status, "mounting") != 0) {
		if (slot_id == 1) {
			if (hyperdeck->slot_selected == 1) {
				hyperdeck->slot_selected = 0;
				set_hyperdeck_button_insensitive (hyperdeck);
				clean_hyperdeck (hyperdeck);
				clean_fresques ();
			}

			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
			hyperdeck->slot_1_is_mounted = FALSE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1), pixbuf_S1E);

			while (hyperdeck->slot_1_disk_list != NULL) {
				disk_list_tmp = hyperdeck->slot_1_disk_list;
				hyperdeck->slot_1_disk_list = disk_list_tmp->next;
				g_free (disk_list_tmp);
			}
		}
		if (slot_id == 2) {
			if (hyperdeck->slot_selected == 2) {
				hyperdeck->slot_selected = 0;
				set_hyperdeck_button_insensitive (hyperdeck);
				clean_hyperdeck (hyperdeck);
				clean_fresques ();
			}

			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);
			hyperdeck->slot_2_is_mounted = FALSE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2), pixbuf_S2E);

			while (hyperdeck->slot_2_disk_list != NULL) {
				disk_list_tmp = hyperdeck->slot_2_disk_list;
				hyperdeck->slot_2_disk_list = disk_list_tmp->next;
				g_free (disk_list_tmp);
			}
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_asynchronous_transport_info (hyperdeck_t *hyperdeck)					//508 transport info:
{
	char text[16];
	int clip_id;
	int slot_id;
	clip_list_t *clip_list_tmp;
	gboolean play_has_changed, play, preview, single_clip_loop_has_changed, loop, single_clip, slot_has_changed;
	char msg[32];
	int msg_len;

	clip_id = 0;
	play_has_changed = FALSE;
	single_clip_loop_has_changed = FALSE;
	slot_has_changed = FALSE;

	switch (hyperdeck->loop) {
		case SINGLE_CLIP_TRUE_LOOP_TRUE: single_clip = TRUE;
						loop = TRUE;
						break;
		case SINGLE_CLIP_TRUE_LOOP_FALSE: single_clip = TRUE;
						loop = FALSE;
						break;
		case SINGLE_CLIP_FALSE_LOOP_FALSE: single_clip = FALSE;
						loop = FALSE;
						break;
		case SINGLE_CLIP_FALSE_LOOP_TRUE: single_clip = FALSE;
						loop = TRUE;
						break;
	}

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "status: ", 6) == 0) {			//status: {“stopped”, “play”, ...}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "status: %s\n", text);
DEBUG_HYPERDECK_S (text)
			if (strcmp (text, "play") == 0) {
				play = TRUE;
				preview = FALSE;
			} else {
				play = FALSE;
				if (strcmp (text, "preview") == 0) preview = TRUE;
				else preview = FALSE;
			}
			play_has_changed = TRUE;
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "speed: ", 5) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "active slot: ", 11) == 0) {		//active slot: {slot id or none}
			if (sscanf (&hyperdeck->buffer[hyperdeck->index], "active slot: %d\n", &slot_id) != 1) slot_id = 0;
DEBUG_HYPERDECK_S ("active slot:")
DEBUG_HYPERDECK_D (slot_id)
			slot_has_changed = TRUE;
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "display timecode: ", 16) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "timecode: ", 8) == 0) {
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "clip id: ", 7) == 0) {			//clip id: {clip id or none}
			if (sscanf (&hyperdeck->buffer[hyperdeck->index], "clip id: %d\n", &clip_id) == 1) {
DEBUG_HYPERDECK_S ("clip id: ")
DEBUG_HYPERDECK_D (clip_id)
				for (clip_list_tmp = hyperdeck->list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
					if (clip_id == clip_list_tmp->id) {
						gtk_list_box_select_row (GTK_LIST_BOX (hyperdeck->list_box), GTK_LIST_BOX_ROW (clip_list_tmp->list_box_row));
						break;
					}
				}
			}
			continue;
		}

		if (strncmp (hyperdeck->buffer + hyperdeck->index, "video format: ", 12) == 0) {	//video format: {}
/*			if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "PAL", 3) == 0) {
				hyperdeck->video_format.nb_lines = 576;
				if (hyperdeck->buffer[hyperdeck->index + 17] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				hyperdeck->video_format.frequency = 25.0;
				hyperdeck->video_format.label = "SD";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "NTSC", 4) == 0) {
				hyperdeck->video_format.nb_lines = 480;
				if (hyperdeck->buffer[hyperdeck->index + 18] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				hyperdeck->video_format.frequency = 30.0;
				hyperdeck->video_format.label = "SD NTSC";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "1080", 4) == 0) {
				hyperdeck->video_format.nb_lines = 1080;
				if (hyperdeck->buffer[hyperdeck->index + 18] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "23", 2) == 0) hyperdeck->video_format.frequency = 23.976;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "24", 2) == 0) hyperdeck->video_format.frequency = 24.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "25", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "29", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "30", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "50", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "59", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 19, "60", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				hyperdeck->video_format.label = "HD";
			} else if (strncmp (hyperdeck->buffer + hyperdeck->index + 14, "4K", 2) == 0) {
				hyperdeck->video_format.nb_lines = 2160;
				if (hyperdeck->buffer[hyperdeck->index + 16] == 'p') hyperdeck->video_format.progressif = TRUE;
				else hyperdeck->video_format.progressif = FALSE;
				if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "23", 2) == 0) hyperdeck->video_format.frequency = 23.976;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "24", 2) == 0) hyperdeck->video_format.frequency = 24.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "25", 2) == 0) hyperdeck->video_format.frequency = 25.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "29", 2) == 0) hyperdeck->video_format.frequency = 29.97;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "30", 2) == 0) hyperdeck->video_format.frequency = 30.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "50", 2) == 0) hyperdeck->video_format.frequency = 50.0;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "59", 2) == 0) hyperdeck->video_format.frequency = 59.94;
				else if (strncmp (hyperdeck->buffer + hyperdeck->index + 17, "60", 2) == 0) hyperdeck->video_format.frequency = 60.0;
				hyperdeck->video_format.label = "UHD 4K";
			}
DEBUG_HYPERDECK_S ("video format: ")
DEBUG_HYPERDECK_D (hyperdeck->video_format.nb_lines)
DEBUG_HYPERDECK_D ((int)hyperdeck->video_format.progressif)
DEBUG_HYPERDECK_F (hyperdeck->video_format.frequency)*/
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "loop: ", 4) == 0) {			//loop: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "loop: %s\n", text);
			if (strcmp (text, "true") == 0) loop = TRUE;
			else loop = FALSE;
			single_clip_loop_has_changed = TRUE;
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "single clip: ", 11) == 0) {		//single clip: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "single clip: %s\n", text);
			if (strcmp (text, "true") == 0) single_clip = TRUE;
			else single_clip = FALSE;
			single_clip_loop_has_changed = TRUE;
			continue;
		}
	}

	if (slot_has_changed) {
		if (slot_id == 1) {
			if (hyperdeck->slot_1_is_mounted) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);
				hyperdeck->slot_selected = 1;
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1S);
			}
		} else if (slot_id == 2) {
			if (hyperdeck->slot_2_is_mounted) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
				hyperdeck->slot_selected = 2;
				SEND (hyperdeck, msg_clips_get)
				SEND (hyperdeck, msg_transport_info)
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2S);
			}
		} else {
			hyperdeck->slot_selected = 0;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);
			gtk_widget_set_sensitive (hyperdeck->play_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->play_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->stop_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->stop_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->single_loop_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->single_loop_button, 0.75);
			gtk_widget_set_sensitive (hyperdeck->del_button, FALSE);
			gtk_widget_set_opacity (hyperdeck->del_button, 0.75);
		}
	}

	if (play_has_changed) {
		if (play) {
			hyperdeck->play = TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_play_button), pixbuf_BPOn);
		} else {
			hyperdeck->play = FALSE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_play_button), pixbuf_BPOff);

			if (preview) {
DEBUG_S("PREVIEW TRUE --> E to E")
//send (hyperdeck->socket, msg_preview_enable_false, 22, 0);
}
		}
	}

	if (!((play_has_changed) && (!play) && (current_fresque != NULL))) {
		if (single_clip_loop_has_changed) {
			if (single_clip) {
				if (loop) {
					hyperdeck->loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
					gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
				} else {
					hyperdeck->loop = SINGLE_CLIP_TRUE_LOOP_FALSE;
					gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_FALSE]);
				}
			} else {
				if (loop) {
					hyperdeck->loop = SINGLE_CLIP_FALSE_LOOP_TRUE;
					gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[SINGLE_CLIP_FALSE_LOOP_TRUE]);
				} else {
					hyperdeck->loop = SINGLE_CLIP_FALSE_LOOP_FALSE;
					gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[SINGLE_CLIP_FALSE_LOOP_FALSE]);
				}
			}
		}
	}

	if ((slot_has_changed == FALSE) && (play_has_changed == FALSE) && (single_clip_loop_has_changed == FALSE) && (clip_id != 0) && (hyperdeck->play == FALSE)) {
		msg_len = sprintf (msg, "%s%d\n", msg_goto_clip_id_, clip_id);
DEBUG_HYPERDECK_S (msg)
		send (hyperdeck->socket, msg, msg_len, 0);
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_asynchronous_remote_info (hyperdeck_t *hyperdeck)						//510 remote info:
{
	char text[8];

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "enabled: ", 7) == 0) {			//enabled: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "enabled: %s\n", text);
			if (strcmp (text, "true") == 0) {
				if (hyperdeck->clip_count == 0) {
					gtk_widget_set_sensitive (hyperdeck->play_button, FALSE);
					gtk_widget_set_opacity (hyperdeck->play_button, 0.75);
					gtk_widget_set_sensitive (hyperdeck->stop_button, FALSE);
					gtk_widget_set_opacity (hyperdeck->stop_button, 0.75);
					gtk_widget_set_sensitive (hyperdeck->single_loop_button, FALSE);
					gtk_widget_set_opacity (hyperdeck->single_loop_button, 0.75);
					gtk_widget_set_sensitive (hyperdeck->del_button, FALSE);
					gtk_widget_set_opacity (hyperdeck->del_button, 0.75);
				}
				gtk_widget_set_sensitive (hyperdeck->root_widget, TRUE);
			}
			if (strcmp (text, "false") == 0) {
				gtk_widget_set_sensitive (hyperdeck->play_button, TRUE);
				gtk_widget_set_opacity (hyperdeck->play_button, 1.0);
				gtk_widget_set_sensitive (hyperdeck->stop_button, TRUE);
				gtk_widget_set_opacity (hyperdeck->stop_button, 1.0);
				gtk_widget_set_sensitive (hyperdeck->single_loop_button, TRUE);
				gtk_widget_set_opacity (hyperdeck->single_loop_button, 1.0);
				gtk_widget_set_sensitive (hyperdeck->del_button, TRUE);
				gtk_widget_set_opacity (hyperdeck->del_button, 1.0);
				gtk_widget_set_sensitive (hyperdeck->root_widget, FALSE);
				show_message_window (((response_t *)(hyperdeck->response))->text);
			}
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "override: ", 8) == 0) {		//override: {“true”, “false”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "override: %s\n", text);
DEBUG_HYPERDECK_S ("override: ")
DEBUG_HYPERDECK_S (text)
//			if (strcmp (text, "true") == 0) {}
			continue;
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}

gboolean response_asynchronous_configuration (hyperdeck_t *hyperdeck)						//511 configuration:
{
	char msg[64];
	char hyperdeck_file_format[32];

	for (; hyperdeck->buffer[hyperdeck->index] != '\r'; next_line (hyperdeck)) {
		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "audio input: ", 11) == 0) {		//audio input: {“embedded”, “XLR”, “RCA”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "audio input: %s\n", msg);
DEBUG_HYPERDECK_S (msg)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "video input: ", 11) == 0) {		//video input: {“SDI”, “HDMI”, “component”}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "video input: %s\n", msg);
DEBUG_HYPERDECK_S (msg)
			continue;
		}

		if (strncmp (&hyperdeck->buffer[hyperdeck->index], "file format: ", 11) == 0) {		//file format: {QuickTimeProResHQ, QuickTimeProRes, QuickTimeProResLT, QuickTimeProResProxy, QuickTimeDNxHD220, QuickTimeDNxHR_HQX, ... DNxHD220}
			sscanf (&hyperdeck->buffer[hyperdeck->index], "file format: %s\n", hyperdeck_file_format);
DEBUG_HYPERDECK_S (msg)
			sprintf (msg, "Hyperdeck n°%d file format: %s", hyperdeck->number + 1, hyperdeck_file_format);
			show_message_window (msg);
			continue;
		}
	}

	next_line (hyperdeck);
g_mutex_unlock (&hyperdeck->connection_mutex);
	return G_SOURCE_REMOVE;
}


response_t responses_list[] = { {200, "OK", NULL}, \
				{202, "slot info:", response_slot_info}, \
				{204, "device info:", response_device_info}, \
				{205, "clips info:", response_clips_info}, \
				{206, "disk list:", response_disk_list}, \
				{208, "transport info:", response_transport_info}, \
				{209, "notify:", response_notify}, \
				{210, "remote info:", response_asynchronous_remote_info}, \
				{211, "configuration:", response_configuration}, \
				{213, "deck rebooting", response_deck_rebooting}, \
				{215, "uptime:", response_uptime}, \
				{500, "connection info:", response_asynchronous_connection_info}, \
				{502, "slot info:", response_asynchronous_slot_info}, \
				{508, "transport info:", response_asynchronous_transport_info}, \
				{510, "remote info:", response_asynchronous_remote_info}, \
				{511, "configuration:", response_asynchronous_configuration}, \
				{100, "syntax error", NULL}, \
				{101, "unsupported parameter", NULL}, \
				{102, "invalid value", NULL}, \
				{103, "unsupported", NULL}, \
				{104, "disk full", NULL}, \
				{105, "no disk", NULL}, \
				{106, "disk error", NULL}, \
				{107, "timeline empty", response_timeline_empty}, \
				{109, "out of range", NULL}, \
				{110, "no input", NULL}, \
				{111, "remote control disabled", response_remote_control_disabled}, \
				{120, "connection rejected", response_connection_rejected}, \
				{150, "invalid state", NULL}, \
				{0 , "unknown message", NULL} };


gboolean close_hyperdeck (hyperdeck_t *hyperdeck)
{
	disk_list_t *disk_list_tmp;

DEBUG_HYPERDECK_S ("close hyperdeck")

	hyperdeck->connected = FALSE;

	gtk_widget_set_opacity (hyperdeck->play_button, 1.0);
	gtk_widget_set_opacity (hyperdeck->stop_button, 1.0);
	gtk_widget_set_opacity (hyperdeck->single_loop_button, 1.0);
	gtk_widget_set_opacity (hyperdeck->del_button, 1.0);
	gtk_widget_set_sensitive (hyperdeck->root_widget, FALSE);

	hyperdeck->disk_slot_id = 0;
	while (hyperdeck->slot_1_disk_list != NULL) {
		disk_list_tmp = hyperdeck->slot_1_disk_list;
		hyperdeck->slot_1_disk_list = disk_list_tmp->next;
		g_free (disk_list_tmp);
	}
	while (hyperdeck->slot_2_disk_list != NULL) {
		disk_list_tmp = hyperdeck->slot_2_disk_list;
		hyperdeck->slot_2_disk_list = disk_list_tmp->next;
		g_free (disk_list_tmp);
	}

	clean_hyperdeck (hyperdeck);
	clean_fresques ();

	hyperdeck->play = FALSE;
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_play_button), pixbuf_BPOff);

	hyperdeck->slot_selected = 0;
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1_indicator), pixbuf_S1NS);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2_indicator), pixbuf_S2NS);

	hyperdeck->slot_1_is_mounted = FALSE;
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_1), pixbuf_S1E);
	hyperdeck->slot_2_is_mounted = FALSE;
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_slot_2), pixbuf_S2E);

	closesocket (hyperdeck->socket);

	g_thread_join (hyperdeck->connection_thread);
	hyperdeck->connection_thread = NULL;
DEBUG_HYPERDECK_S ("close hyperdeck fin")
	return G_SOURCE_REMOVE;
}

void receive_response_from_hyperdeck (hyperdeck_t *hyperdeck)
{
	int response_code;
	int i;

g_mutex_lock (&hyperdeck->connection_mutex);
	while ((hyperdeck->recv_len = recv (hyperdeck->socket, hyperdeck->buffer, sizeof (hyperdeck->buffer), 0)) > 0) {
DEBUG_HYPERDECK_S ("receive_response_from_hyperdeck")
hyperdeck->buffer[hyperdeck->recv_len] = '\0';
DEBUG_HYPERDECK_S ("[buffer]")
DEBUG_HYPERDECK_S (hyperdeck->buffer)

		for (hyperdeck->index = 0; hyperdeck->index < hyperdeck->recv_len; ) {
			if ((hyperdeck->buffer[hyperdeck->index + 1] == ':') || (hyperdeck->buffer[hyperdeck->index + 2] == ':') || (hyperdeck->buffer[hyperdeck->index + 3] == ':')) {
				response_disk_list_suite (hyperdeck);
DEBUG_HYPERDECK_S ("[fin d'un message contenant une liste partiel de fichier]\n")
				continue;
			}
			if (sscanf (&hyperdeck->buffer[hyperdeck->index], "%d", &response_code) != 1) {
DEBUG_HYPERDECK_S ("Arg, message unconnu")
				next_line (hyperdeck);
				continue;
			}
			next_line (hyperdeck);

DEBUG_HYPERDECK_D (response_code)
			for (i = 0; responses_list[i].code != 0; i++) {
				if (responses_list[i].code == response_code) {
DEBUG_HYPERDECK_S (responses_list[i].text)
					if (responses_list[i].func != NULL) {
						hyperdeck->response = &responses_list[i];
						g_idle_add ((GSourceFunc)responses_list[i].func, hyperdeck);
g_mutex_lock (&hyperdeck->connection_mutex);
					}
					break;
				}
			}
DEBUG_HYPERDECK_S ("[fin d'un message]\n")
		}
	}
g_mutex_unlock (&hyperdeck->connection_mutex);

//	g_idle_add ((GSourceFunc)close_hyperdeck, hyperdeck);

	if (hyperdeck->reboot == TRUE) {
		SLEEP (5)
		hyperdeck->reboot = FALSE;
		connect_to_hyperdeck (hyperdeck);
	}
}

gboolean g_source_hyperdeck_is_connected (hyperdeck_t *hyperdeck)
{
	gtk_widget_set_sensitive (hyperdeck->root_widget, TRUE);

	return G_SOURCE_REMOVE;
}

gpointer connect_to_hyperdeck (hyperdeck_t *hyperdeck)
{
DEBUG_HYPERDECK_S ("connect_to_hyperdeck")
	hyperdeck->socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset (&hyperdeck->adresse, 0, sizeof (struct sockaddr_in));
	hyperdeck->adresse.sin_family = AF_INET;
	hyperdeck->adresse.sin_port = htons (9993);
	hyperdeck->adresse.sin_addr.s_addr = inet_addr (hyperdeck->adresse_ip);

	if (connect (hyperdeck->socket, (struct sockaddr *) &hyperdeck->adresse, sizeof (struct sockaddr_in)) == 0) {
		g_idle_add ((GSourceFunc)g_source_hyperdeck_is_connected, hyperdeck);
		hyperdeck->connected = TRUE;
		SEND (hyperdeck, msg_remote_enable)
//		SEND (hyperdeck, msg_remote_override)
		SEND (hyperdeck, msg_configuration)

		receive_response_from_hyperdeck (hyperdeck);
	} else closesocket (hyperdeck->socket);

	g_idle_add ((GSourceFunc)close_hyperdeck, hyperdeck);

	return NULL;
}

void disconnect_from_hyperdeck (hyperdeck_t* hyperdeck)
{
	send (hyperdeck->socket, msg_quit, 5, 0);
//	close_hyperdeck (hyperdeck);	//?
}

