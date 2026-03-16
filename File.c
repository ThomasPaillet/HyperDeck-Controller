/*
 * copyright (c) 2018-2021 2026 Thomas Paillet <thomas.paillet@net-c.fr

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

#include "File.h"

#include "Fresque.h"
#include "HyperDeck_Codec.h"
#include "HyperDeck_Protocol.h"
#include "Logging.h"
#include "Misc.h"
#include "Preset.h"
#include "Transcoding.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libavutil/mem.h>


#define FILE_SLICE 2097152


GtkWidget *clip_menu;

char file_to_delete[CLIP_NAME_LENGTH] = { '\0' };
hyperdeck_t *in_this_hyperdeck = NULL;

GtkWidget *fresque_menu;

char fresque_to_delete[CLIP_NAME_LENGTH] = { '\0' };
int fresque_to_delete_clips_id[NB_OF_HYPERDECKS];


gboolean refresh_hyperdeck_list_of_clips (hyperdeck_t *hyperdeck)
{
	send (hyperdeck->socket, msg_preview_enable_false, 23, 0);

	if (hyperdeck->slot_selected == 1) send (hyperdeck->socket, msg_disk_list_slot_id_1, 22, 0);
	else send (hyperdeck->socket, msg_disk_list_slot_id_2, 22, 0);

	send (hyperdeck->socket, msg_clips_get, 10, 0);

	send (hyperdeck->socket, msg_transport_info, 15, 0);

	return G_SOURCE_REMOVE;
}

void delete_file (void)
{
	int i;
	gboolean play[NB_OF_HYPERDECKS];
	SOCKET ftp_socket;
	struct sockaddr_in adresse;
	char msg[CLIP_NAME_LENGTH + 32];
	int msg_len;

	char ftp_buffer[4096];
	int recv_len, h1, h2;

	LOG_STRING ("delete_file ()")

	if (in_this_hyperdeck == NULL) return;

	ftp_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset (&adresse, 0, sizeof (struct sockaddr_in));
	adresse.sin_family = AF_INET;
	adresse.sin_port = htons (21);
	adresse.sin_addr.s_addr = inet_addr (in_this_hyperdeck->ip_address);

	g_mutex_lock (&in_this_hyperdeck->ftp_mutex);

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((hyperdecks[i].connected) && (hyperdecks[i].play)) {
			send (hyperdecks[i].socket, msg_stop, 5, 0);

			play[i] = TRUE;
		} else play[i] = FALSE;
	}

	if (connect (ftp_socket, (struct sockaddr *)&adresse, sizeof (struct sockaddr_in)) == 0) {
		SEND (in_this_hyperdeck, msg_clips_clear)

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (in_this_hyperdeck,ftp_buffer)

		msg_len = sprintf (msg, "CWD %d\r\n", in_this_hyperdeck->slot_selected);
		send (ftp_socket, msg, msg_len, 0);

		LOG_HYPERDECK_STRING (in_this_hyperdeck,msg)

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (in_this_hyperdeck,ftp_buffer)

		h1 = sscanf (ftp_buffer, "%d ", &h2);

		if ((h1 != 1) || (h2 != 250)) {
			SLEEP (0.3);

			send (ftp_socket, msg, msg_len, 0);

			LOG_HYPERDECK_STRING (in_this_hyperdeck,msg)
		}

		SLEEP (0.1);

		msg_len = sprintf (msg, "DELE %s.mov\r\n", file_to_delete);
		send (ftp_socket, msg, msg_len, 0);

		LOG_HYPERDECK_STRING (in_this_hyperdeck,msg)

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (in_this_hyperdeck,ftp_buffer)

		h1 = sscanf (ftp_buffer, "%d ", &h2);

		if ((h1 != 1) || (h2 != 250)) {
			SLEEP (0.3);

			send (ftp_socket, msg, msg_len, 0);

			LOG_HYPERDECK_STRING (in_this_hyperdeck,msg)
		}

		send (ftp_socket, "QUIT\r\n", 6, 0);

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (in_this_hyperdeck,ftp_buffer)

		g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, in_this_hyperdeck);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (play[i]) send (hyperdecks[i].socket, msg_play_single_loop[hyperdecks[i].loop], msg_play_single_loop_len[hyperdecks[i].loop], 0);
	}

	g_mutex_unlock (&in_this_hyperdeck->ftp_mutex);

	closesocket (ftp_socket);

	LOG_HYPERDECK_STRING (in_this_hyperdeck,"delete_file () return")

	file_to_delete[0] = '\0';
	in_this_hyperdeck = NULL;
}

void delete_files (void)
{
	int i;
	gboolean play[NB_OF_HYPERDECKS];
	int fresque_to_delete_name_len;
	GtkListBoxRow *selected_list_box_row;
	const gchar *name;

	SOCKET ftp_socket;
	struct sockaddr_in adresse;
	char msg[CLIP_NAME_LENGTH + 32];
	int msg_len;

	char ftp_buffer[4096];
	int recv_len, h1, h2;

	LOG_STRING ("delete_fresque ()")

	if (fresque_to_delete[0] == '\0') return;

	fresque_to_delete_name_len = strlen (fresque_to_delete);
	fresque_to_delete[fresque_to_delete_name_len + 1] = '\0';

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((hyperdecks[i].connected) && (hyperdecks[i].play)) {
			send (hyperdecks[i].socket, msg_stop, 5, 0);

			play[i] = TRUE;
		} else play[i] = FALSE;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((!hyperdecks[i].connected) || (hyperdecks[i].slot_selected == 0) || (hyperdecks[i].drop_thread != NULL)) continue;
		if (fresque_to_delete_clips_id[i] == 0) continue;

		fresque_to_delete[fresque_to_delete_name_len] = (char)hyperdecks[i].number + 49;

		LOG_HYPERDECK_STRING (hyperdecks + i,fresque_to_delete)

		selected_list_box_row = gtk_list_box_get_selected_row (GTK_LIST_BOX (hyperdecks[i].list_box));

		if (selected_list_box_row != NULL ) {
			name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (selected_list_box_row))))));
			if (strcmp (fresque_to_delete, name) == 0) continue;
		}

		ftp_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

		memset (&adresse, 0, sizeof (struct sockaddr_in));
		adresse.sin_family = AF_INET;
		adresse.sin_port = htons (21);
		adresse.sin_addr.s_addr = inet_addr (hyperdecks[i].ip_address);

		g_mutex_lock (&hyperdecks[i].ftp_mutex);

		if (connect (ftp_socket, (struct sockaddr *)&adresse, sizeof (struct sockaddr_in)) == 0) {
			SEND (&hyperdecks[i], msg_clips_clear)

			recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdecks + i,ftp_buffer)

			msg_len = sprintf (msg, "CWD %d\r\n", hyperdecks[i].slot_selected);
			send (ftp_socket, msg, msg_len, 0);

			LOG_HYPERDECK_STRING (hyperdecks + i,msg)

			recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdecks + i,ftp_buffer)

			h1 = sscanf (ftp_buffer, "%d ", &h2);

			if ((h1 != 1) || (h2 != 250)) {
				SLEEP (0.3);

				send (ftp_socket, msg, msg_len, 0);

				LOG_HYPERDECK_STRING (hyperdecks + i,msg)
			}

			SLEEP (0.1);

			msg_len = sprintf (msg, "DELE %s.mov\r\n", fresque_to_delete);
			send (ftp_socket, msg, msg_len, 0);

			LOG_HYPERDECK_STRING (hyperdecks + i,msg)

			recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdecks + i,ftp_buffer)

			h1 = sscanf (ftp_buffer, "%d ", &h2);

			if ((h1 != 1) || (h2 != 250)) {
				SLEEP (0.3);

				send (ftp_socket, msg, msg_len, 0);

				LOG_HYPERDECK_STRING (hyperdecks + i,msg)
			}

			send (ftp_socket, "QUIT\r\n", 6, 0);

			recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdecks + i,ftp_buffer)

			g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, &hyperdecks[i]);
		}

		g_mutex_unlock (&hyperdecks[i].ftp_mutex);

		closesocket (ftp_socket);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (play[i]) send (hyperdecks[i].socket, msg_play_single_loop[hyperdecks[i].loop], msg_play_single_loop_len[hyperdecks[i].loop], 0);
	}

	fresque_to_delete[0] = '\0';

	LOG_STRING ("delete_fresque () return")
}

gpointer purge_hyperdeck (hyperdeck_t *hyperdeck)
{
	SOCKET ftp_socket;
	struct sockaddr_in adresse;
	clip_list_t *clip_list_tmp;
	char msg[CLIP_NAME_LENGTH + 32];
	int msg_len, i;

	char ftp_buffer[4096];
	int recv_len, h1, h2;

	LOG_HYPERDECK_STRING (hyperdeck,"purge_hyperdeck ()")

	ftp_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset (&adresse, 0, sizeof (struct sockaddr_in));
	adresse.sin_family = AF_INET;
	adresse.sin_port = htons (21);
	adresse.sin_addr.s_addr = inet_addr (hyperdeck->ip_address);

	g_mutex_lock (&hyperdeck->ftp_mutex);

	if (connect (ftp_socket, (struct sockaddr *)&adresse, sizeof (struct sockaddr_in)) == 0) {
		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

		if (hyperdeck->slot_selected == 1) send (ftp_socket, "CWD 1\r\n", 7, 0);
		else send (ftp_socket, "CWD 2\r\n", 7, 0);

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

		h1 = sscanf (ftp_buffer, "%d ", &h2);
		if ((h1 != 1) || (h2 != 250)) {
			SLEEP (0.3);

			if (hyperdeck->slot_selected == 1) send (ftp_socket, "CWD 1\r\n", 7, 0);
			else send (ftp_socket, "CWD 2\r\n", 7, 0);
		}

		for (clip_list_tmp = hyperdeck->list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
			for (i = 0; i < NB_OF_PRESETS; i++) {
				if (presets[i].clips[hyperdeck->number].slot != hyperdeck->slot_selected) continue;
				if (strcmp (presets[i].clips[hyperdeck->number].name, clip_list_tmp->name) == 0) break;
			}
			if (i < NB_OF_PRESETS) continue;

			SLEEP (0.1);

			msg_len = sprintf (msg, "DELE %s.%s\r\n", clip_list_tmp->name, file_ext);
			send (ftp_socket, msg, msg_len, 0);

			LOG_HYPERDECK_STRING (hyperdeck,msg)

			recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

			h1 = sscanf (ftp_buffer, "%d ", &h2);

			if ((h1 != 1) || (h2 != 250)) {
				SLEEP (0.3);

				send (ftp_socket, msg, msg_len, 0);

				LOG_HYPERDECK_STRING (hyperdeck,msg)
			}
		}

		send (ftp_socket, "QUIT\r\n", 6, 0);

		recv_len = recv (ftp_socket, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

		g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, hyperdeck);
	}

	g_mutex_unlock (&hyperdeck->ftp_mutex);

	closesocket (ftp_socket);

	LOG_HYPERDECK_STRING (hyperdeck,"purge_hyperdeck () return")

	return NULL;
}

gboolean delete_clip (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck)
{
	GtkListBoxRow *list_box_row, *selected_list_box_row;
	const gchar *name;

	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == GDK_BUTTON_SECONDARY) {
			list_box_row = (GtkListBoxRow *) gtk_widget_get_parent (event_box);
			selected_list_box_row = gtk_list_box_get_selected_row (GTK_LIST_BOX (hyperdeck->list_box));

			if ((list_box_row != selected_list_box_row) && (hyperdeck->drop_thread == NULL)) {
				name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (event_box))));
				strcpy (file_to_delete, name);
				in_this_hyperdeck = hyperdeck;

				gtk_menu_popup_at_pointer (GTK_MENU (clip_menu), NULL);
			}

			return GDK_EVENT_STOP;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean delete_fresque (GtkWidget *event_box, GdkEventButton *event)
{
	const gchar *name;
	fresque_t *fresque_tmp;

	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == GDK_BUTTON_SECONDARY) {
			if ((current_fresque == NULL) || (current_fresque->list_box_row != gtk_widget_get_parent (event_box))) {
				name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (event_box))));
				strcpy (fresque_to_delete, name);

				for (fresque_tmp = fresques; fresque_tmp != NULL; fresque_tmp = fresque_tmp->next) {
					if (strcmp (fresque_tmp->name, fresque_to_delete) == 0) {
						memcpy (fresque_to_delete_clips_id, fresque_tmp->clips_id, NB_OF_HYPERDECKS * sizeof (int));

						break;
					}
				}

				gtk_menu_popup_at_pointer (GTK_MENU (fresque_menu), NULL);
			}

			return GDK_EVENT_STOP;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gpointer drop_file_to_hyperdeck (hyperdeck_t* hyperdeck)
{
	drop_list_t *drop_list;
	FILE *drop_file, *logging_local_file;
	struct stat file_stat;
	guint64 file_size;
	int file_slice_nb, file_slice_nb_full, fread_len, i;
	char *file_buffer;

	SOCKET socket_pi, socket_dtp;
	struct sockaddr_in adresse_pi, adresse_dtp;
	int h1, h2, h3, h4;
	unsigned short p1, p2;
	char msg[CLIP_NAME_LENGTH * 2 + 32];
	int msg_len;

	char ftp_buffer[4096];
	int recv_len;

	LOG_HYPERDECK_STRING (hyperdeck,"drop_to_hyperdeck ()")

	if ((!hyperdeck->connected) || (hyperdeck->slot_selected == 0)) {
		g_mutex_lock (&hyperdeck->drop_mutex);

		while (hyperdeck->drop_list_file != NULL) {
			drop_list = hyperdeck->drop_list_file;
			hyperdeck->drop_list_file = drop_list->next;

			if (drop_list->ffmpeg_buffer != NULL) av_free (drop_list->ffmpeg_buffer);

			g_free (drop_list->full_name);
			g_free (drop_list);
		}

		g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc)g_source_consume_thread, hyperdeck->drop_thread, NULL);
		hyperdeck->drop_thread = NULL;

		g_mutex_unlock (&hyperdeck->drop_mutex);

		LOG_HYPERDECK_STRING (hyperdeck,"drop_to_hyperdeck () return (unable to continue)")

		return NULL;
	}

	socket_pi = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset (&adresse_pi, 0, sizeof (struct sockaddr_in));
	adresse_pi.sin_family = AF_INET;
	adresse_pi.sin_port = htons (21);
	adresse_pi.sin_addr.s_addr = inet_addr (hyperdeck->ip_address);

	g_mutex_lock (&hyperdeck->ftp_mutex);

	if (connect (socket_pi, (struct sockaddr *)&adresse_pi, sizeof (struct sockaddr_in)) == 0) {
		recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

		sprintf (msg, "CWD %d\r\n", hyperdeck->slot_selected);
		send (socket_pi, msg, 7, 0);

		LOG_HYPERDECK_STRING (hyperdeck,msg)

		recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

		h1 = sscanf (ftp_buffer, "%d ", &h2);

		if ((h1 != 1) || (h2 != 250)) {
			SLEEP (0.3);

			send (socket_pi, msg, 7, 0);

			LOG_HYPERDECK_STRING (hyperdeck,msg)
		}

		g_mutex_lock (&hyperdeck->drop_mutex);

		while (hyperdeck->drop_list_file != NULL) {
			drop_list = hyperdeck->drop_list_file;
			hyperdeck->drop_list_file = drop_list->next;

			g_mutex_unlock (&hyperdeck->drop_mutex);

			g_idle_add ((GSourceFunc)g_source_init_hyperdeck_progress_bar, hyperdeck);

			LOG_HYPERDECK_2_STRINGS (hyperdeck,"full_name = ",drop_list->full_name)
			LOG_HYPERDECK_2_STRINGS (hyperdeck,"file_name_out = ",drop_list->file_name_out)

			send (socket_pi, "PASV\r\n", 6, 0);

			LOG_HYPERDECK_STRING (hyperdeck,"PASV\r\n")

			recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
			ftp_buffer[recv_len] = '\0';

			LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

			h1 = sscanf (ftp_buffer, "%d ", &h2);
			if ((h1 != 1) || (h2 != 227)) {
				SLEEP (0.3);

				send (socket_pi, "PASV\r\n", 6, 0);

				LOG_HYPERDECK_STRING (hyperdeck,"PASV\r\n")
			}

			if (sscanf (ftp_buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%hu,%hu)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
				socket_dtp = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

				memset (&adresse_dtp, 0, sizeof (struct sockaddr_in));
				adresse_dtp.sin_family = AF_INET;
				adresse_dtp.sin_port = htons (p1 * 256 + p2);
				adresse_dtp.sin_addr.s_addr = inet_addr (hyperdeck->ip_address);

				msg_len = sprintf (msg, "STOR %s\r\n", drop_list->file_name_out);
				send (socket_pi, msg, msg_len, 0);

				LOG_HYPERDECK_STRING (hyperdeck,msg)

				recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
				ftp_buffer[recv_len] = '\0';

				LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

				h1 = sscanf (ftp_buffer, "%d ", &h2);
				if ((h1 != 1) || (h2 != 150)) {
					SLEEP (0.3);

					send (socket_pi, msg, msg_len, 0);

					LOG_HYPERDECK_STRING (hyperdeck,msg)

					recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
					ftp_buffer[recv_len] = '\0';

					LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

					h1 = sscanf (ftp_buffer, "%d ", &h2);
				}

				if ((h1 == 1) && (h2 == 150)) {
					if (connect (socket_dtp, (struct sockaddr *) &adresse_dtp, sizeof (struct sockaddr_in)) == 0) {
						if (logging) logging_local_file = fopen (drop_list->file_name_out, "wb");

						if (drop_list->ffmpeg_buffer != NULL) {
							file_slice_nb = drop_list->ffmpeg_buffer_size / FILE_SLICE;

							if (drop_list->ffmpeg_buffer_size % FILE_SLICE) file_slice_nb_full = file_slice_nb + 1;
							if (file_slice_nb == 0) {
								send (socket_dtp, drop_list->ffmpeg_buffer, drop_list->ffmpeg_buffer_size, 0);

								if (logging) fwrite (drop_list->ffmpeg_buffer, drop_list->ffmpeg_buffer_size, 1, logging_local_file);
							} else {
								i = 0;
								do {
									LOG_HYPERDECK_INT (hyperdeck,i)

									send (socket_dtp, drop_list->ffmpeg_buffer + i * FILE_SLICE, FILE_SLICE, 0);

									if (logging) fwrite (drop_list->ffmpeg_buffer + i * FILE_SLICE, FILE_SLICE, 1, logging_local_file);

									drop_list->ffmpeg_buffer_size -= FILE_SLICE;
									i++;
									hyperdeck->progress_bar_fraction = (double)i / (double)file_slice_nb_full;
								} while (i < file_slice_nb);

								if (drop_list->ffmpeg_buffer_size > 0) {
									send (socket_dtp, drop_list->ffmpeg_buffer + i * FILE_SLICE, drop_list->ffmpeg_buffer_size, 0);

									if (logging) fwrite (drop_list->ffmpeg_buffer + i * FILE_SLICE, drop_list->ffmpeg_buffer_size, 1, logging_local_file);
								}
							}

							av_freep (&drop_list->ffmpeg_buffer);
							avformat_free_context (drop_list->av_format_context_out);
						} else {
							drop_file = fopen (drop_list->full_name, "rb");

							if (drop_file != NULL) {
								stat (drop_list->full_name, &file_stat);
								file_size = file_stat.st_size;

								LOG_HYPERDECK_STRING_LONG (hyperdeck,"file_size = ",file_size)

								file_slice_nb = file_size / (guint64)FILE_SLICE;
								if (file_size % (guint64)FILE_SLICE) file_slice_nb++;

								LOG_HYPERDECK_STRING_LONG (hyperdeck,"modulo = ",file_size%(guint64)FILE_SLICE)

								file_buffer = g_malloc (FILE_SLICE);

								for (i = 1; i <= file_slice_nb; i++) {
									LOG_HYPERDECK_INT (hyperdeck,i)

									fread_len = fread (file_buffer, 1, FILE_SLICE, drop_file);
									send (socket_dtp, file_buffer, fread_len, 0);

									if (logging) fwrite (file_buffer, fread_len, 1, logging_local_file);

									hyperdeck->progress_bar_fraction = (double)i / (double)file_slice_nb;
								}

								g_free (file_buffer);
								fclose (drop_file);
							}
						}

						hyperdeck->progress_bar_fraction = 1.0;

						if (logging) fclose (logging_local_file);
					}
				}

				closesocket (socket_dtp);

				recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
				ftp_buffer[recv_len] = '\0';

				LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)

				if (hyperdeck->drop_list_file == NULL) {
					g_mutex_lock (&hyperdeck->last_file_dropped_mutex);

					if (hyperdeck->last_file_dropped != NULL) g_free (hyperdeck->last_file_dropped);

					hyperdeck->last_file_dropped = g_malloc (drop_list->file_name_out_len - 3);
					memcpy (hyperdeck->last_file_dropped, drop_list->file_name_out, drop_list->file_name_out_len - 4);
					hyperdeck->last_file_dropped[drop_list->file_name_out_len - 4] = '\0';

					LOG_HYPERDECK_2_STRINGS (hyperdeck,"last_file_dropped = ",hyperdeck->last_file_dropped)

					g_timeout_add (REFRESH_WAITING_TIME, (GSourceFunc)refresh_hyperdeck_list_of_clips, hyperdeck);

					g_mutex_unlock (&hyperdeck->last_file_dropped_mutex);
				}
			}

			g_free (drop_list->full_name);
			g_free (drop_list);

			g_mutex_lock (&hyperdeck->drop_mutex);
		}

		g_mutex_unlock (&hyperdeck->drop_mutex);

		send (socket_pi, "QUIT\r\n", 6, 0);

		LOG_HYPERDECK_STRING (hyperdeck,"QUIT\r\n")

		recv_len = recv (socket_pi, ftp_buffer, sizeof (ftp_buffer), 0);
		ftp_buffer[recv_len] = '\0';

		LOG_HYPERDECK_STRING (hyperdeck,ftp_buffer)
	}

	g_mutex_unlock (&hyperdeck->ftp_mutex);

	closesocket (socket_pi);

	g_mutex_lock (&hyperdeck->drop_mutex);

	g_idle_add ((GSourceFunc)g_source_hide_hyperdeck_progress_bar, hyperdeck);

	g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc)g_source_consume_thread, hyperdeck->drop_thread, NULL);
	hyperdeck->drop_thread = NULL;

	g_mutex_unlock (&hyperdeck->drop_mutex);

	LOG_HYPERDECK_STRING (hyperdeck,"drop_to_hyperdeck () return")

	return NULL;
}

void complete_file_name_out (drop_list_t *drop_list)
{
	drop_list->file_name_out[drop_list->file_name_out_len++] = '.';
	drop_list->file_name_out[drop_list->file_name_out_len++] = 'm';

	if (file_ext == file_ext_mov) {
		drop_list->file_name_out[drop_list->file_name_out_len++] = 'o';
		drop_list->file_name_out[drop_list->file_name_out_len++] = 'v';
	} else {
		drop_list->file_name_out[drop_list->file_name_out_len++] = 'x';
		drop_list->file_name_out[drop_list->file_name_out_len++] = 'f';
	}

	drop_list->file_name_out[drop_list->file_name_out_len] = '\0';
}

void add_drop_list_to_hyperdeck_transfert_queue (drop_list_t *drop_list, hyperdeck_t *hyperdeck)
{
	g_mutex_lock (&hyperdeck->drop_mutex);

	drop_list->next = hyperdeck->drop_list_file;
	hyperdeck->drop_list_file = drop_list;

	if (hyperdeck->drop_thread == NULL) hyperdeck->drop_thread = g_thread_new (NULL, (GThreadFunc)drop_file_to_hyperdeck, hyperdeck);

	g_mutex_unlock (&hyperdeck->drop_mutex);
}

void add_drop_list_to_hyperdeck (drop_list_t *drop_list, hyperdeck_t *hyperdeck)
{
	LOG_HYPERDECK_2_STRINGS (hyperdeck,"file_name_out = ",drop_list->file_name_out)

	switch (check_need_for_transcoding (hyperdeck, drop_list)) {
		case 1:
			complete_file_name_out (drop_list);

			add_drop_list_to_hyperdeck_transfert_queue (drop_list, hyperdeck);

			break;
		case 2:
			g_mutex_lock (&hyperdeck->remuxing_mutex);

			drop_list->next = hyperdeck->remuxing_list_file;
			hyperdeck->remuxing_list_file = drop_list;

			if (hyperdeck->remuxing_thread == NULL) hyperdeck->remuxing_thread = g_thread_new (NULL, (GThreadFunc)hyperdeck_remux, hyperdeck);

			g_mutex_unlock (&hyperdeck->remuxing_mutex);

			break;
		case 3:
			g_mutex_lock (&hyperdeck->transcoding_mutex);

			drop_list->next = hyperdeck->transcoding_list_file;
			hyperdeck->transcoding_list_file = drop_list;

			if (hyperdeck->transcoding_thread == NULL) hyperdeck->transcoding_thread = g_thread_new (NULL, (GThreadFunc)hyperdeck_transcode, hyperdeck);

			g_mutex_unlock (&hyperdeck->transcoding_mutex);

			break;
		default:
			g_free (drop_list->full_name);
			g_free (drop_list);
	}
}

void hyperdeck_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, hyperdeck_t *hyperdeck)
{
	int i, j, k, l, full_name_len;
	const guchar *const_data_data;
	guchar *data_data;
	gint data_length;
	drop_list_t *drop_list;
	char last = '\0';

	const_data_data = gtk_selection_data_get_data_with_length (data, &data_length);
	data_data = g_malloc (data_length);
	memcpy (data_data, const_data_data, data_length);
	gtk_drag_finish (context, TRUE, FALSE, time);

	LOG_HYPERDECK_STRING (hyperdeck,"hyperdeck_drag_data_received ()")
	LOG_HYPERDECK_STRING (hyperdeck,(char *)data_data)

	for (i = 0, j = 0; i < data_length; i++) {
		if (data_data[i] == '\r') {
			data_data[i] = '\0';

			drop_list = g_malloc (sizeof (drop_list_t));
			drop_list->av_format_context_out = NULL;
			drop_list->ffmpeg_buffer = NULL;
			drop_list->ffmpeg_buffer_size = 0;

			drop_list->full_name = g_filename_from_uri ((const gchar*)(data_data + j), NULL, NULL);
			full_name_len = strlen (drop_list->full_name);

			LOG_HYPERDECK_2_STRINGS (hyperdeck,"full_name = ",drop_list->full_name)

			if (full_name_len < 5) {
				g_free (drop_list->full_name);
				g_free (drop_list);
				i++;

				continue;
			}

			for (k = full_name_len - 1; k >= 0; k--) {
				if (drop_list->full_name[k] == G_DIR_SEPARATOR) break;
			}
			k++;
			drop_list->file_name_in = drop_list->full_name + k;

			LOG_HYPERDECK_2_STRINGS (hyperdeck,"file_name_in = ",drop_list->file_name_in)

			for (l = 0; k < full_name_len; k++) {
				if ((('0' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= '9')) || \
				(('A' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= 'Z')) || \
				(('a' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= 'z')) || \
				(drop_list->full_name[k] == '-') || \
				(drop_list->full_name[k] == ' ')) {
					drop_list->file_name_out[l] = drop_list->full_name[k];
					last = drop_list->file_name_out[l];
					l++;
				} else if (drop_list->full_name[k] == '.') break;
				else {
					if (last != '_') {
						drop_list->file_name_out[l] = '_';
						last = '_';
						l++;
					}
				}
			}
			drop_list->file_name_out_len = l;
			drop_list->file_name_out[l] = '\0';

			add_drop_list_to_hyperdeck (drop_list, hyperdeck);

			i++;
			j = i + 1;
		}
	}

	g_free (data_data);

	LOG_HYPERDECK_STRING (hyperdeck,"hyperdeck_drag_data_received () return")
}

void fresques_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time)
{
	int i, j, k, l, full_name_len;
	const guchar *const_data_data;
	guchar *data_data;
	gint data_length;
	drop_list_t *drop_list;
	char last = '\0';

	const_data_data = gtk_selection_data_get_data_with_length (data, &data_length);
	data_data = g_malloc (data_length);
	memcpy (data_data, const_data_data, data_length);
	gtk_drag_finish (context, TRUE, FALSE, time);

	LOG_STRING ("fresques_drag_data_received ()")
	LOG_STRING ((char *)data_data)

	for (i = 0, j = 0; i < data_length; i++) {
		if (data_data[i] == '\r') {
			data_data[i] = '\0';

			drop_list = g_malloc (sizeof (drop_list_t));
			drop_list->av_format_context_out = NULL;
			drop_list->ffmpeg_buffer = NULL;
			drop_list->ffmpeg_buffer_size = 0;

			drop_list->full_name = g_filename_from_uri ((const gchar*)(data_data + j), NULL, NULL);
			full_name_len = strlen (drop_list->full_name);

			LOG_2_STRINGS ("full_name = ",drop_list->full_name)

			if (full_name_len < 5) {
				g_free (drop_list->full_name);
				g_free (drop_list);
				i++;

				continue;
			}

			for (k = full_name_len - 1; k >= 0; k--) {
				if (drop_list->full_name[k] == G_DIR_SEPARATOR) break;
			}
			k++;
			drop_list->file_name_in = drop_list->full_name + k;

			LOG_2_STRINGS ("file_name_in = ",drop_list->file_name_in)

			for (l = 0; k < full_name_len; k++) {
				if ((('0' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= '9')) || \
				(('A' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= 'Z')) || \
				(('a' <= drop_list->full_name[k]) && (drop_list->full_name[k] <= 'z')) || \
				(drop_list->full_name[k] == '-') || \
				(drop_list->full_name[k] == ' ')) {
					drop_list->file_name_out[l] = drop_list->full_name[k];
					last = drop_list->file_name_out[l];
					l++;
				} else if (drop_list->full_name[k] == '.') break;
				else {
					if (last != '_') {
						drop_list->file_name_out[l] = '_';
						last = '_';
						l++;
					}
				}
			}
			drop_list->file_name_out_len = l;
			drop_list->file_name_out[l] = '\0';

			LOG_2_STRINGS ("file_name_out = ",drop_list->file_name_out)

			for (k = full_name_len - 1; k >= 1; k--) {
				if (drop_list->full_name[k] == '.') { k--; break; }
			}

			for (l = 0; l < NB_OF_HYPERDECKS; l++) {
				if (drop_list->full_name[k] == '1' + (char)hyperdecks[l].number) {
					add_drop_list_to_hyperdeck (drop_list, hyperdecks + l);

					break;
				}
			}

			if (l == NB_OF_HYPERDECKS) add_drop_list_to_hyperdeck (drop_list, hyperdecks);

			i++;
			j = i + 1;
		}
	}

	g_free (data_data);

	LOG_STRING ("fresques_drag_data_received () return\n")
}

void init_file (void)
{
	GtkWidget *menu_item;

	clip_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_label ("Supprimer");
	g_signal_connect (menu_item, "activate", G_CALLBACK (delete_file), NULL);
	gtk_container_add (GTK_CONTAINER (clip_menu), menu_item);
	gtk_widget_show_all (clip_menu);

#if NB_OF_HYPERDECKS != 1
	fresque_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_label ("Supprimer");
	g_signal_connect (menu_item, "activate", G_CALLBACK (delete_files), NULL);
	gtk_container_add (GTK_CONTAINER (fresque_menu), menu_item);
	gtk_widget_show_all (fresque_menu);
#endif
}

