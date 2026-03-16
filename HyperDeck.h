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

#ifndef __HYPERDECK_H
#define __HYPERDECK_H


#include <gtk/gtk.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>


#define NB_OF_HYPERDECKS 4

#define CLIP_NAME_LENGTH 128


#ifdef _WIN32
	#include <winsock2.h>

	#define SHUT_RD SD_RECEIVE

	typedef int socklen_t;

	void WSAInit (void);

	#define SLEEP(t) Sleep (t * 1000);

#elif defined (__linux)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>

	#define SOCKET int
	#define closesocket close
	#define WSAInit()
	#define WSACleanup()

	#define SLEEP(t) sleep (t);

#endif


typedef enum {
	SINGLE_CLIP_TRUE_LOOP_TRUE,
	SINGLE_CLIP_TRUE_LOOP_FALSE,
	SINGLE_CLIP_FALSE_LOOP_FALSE,
	SINGLE_CLIP_FALSE_LOOP_TRUE
} loop_t;

typedef struct disk_list_s {
	char name[CLIP_NAME_LENGTH];

	struct disk_list_s *next;
} disk_list_t;

typedef struct clip_list_s {
	char name[CLIP_NAME_LENGTH];
	int id;
	GtkWidget *list_box_row;

	struct clip_list_s *next;
} clip_list_t;

typedef struct drop_list_s {
	char *full_name;
	char *file_name_in;
	char file_name_out[CLIP_NAME_LENGTH * 2];
	int file_name_out_len;

	AVFormatContext *av_format_context_out;
	unsigned char *ffmpeg_buffer;
	int ffmpeg_buffer_size;

	AVFormatContext *av_format_context_in;
	const AVCodec *av_codec_in;
	int stream_index;
	int64_t nb_frames;
	char *format;
	gboolean codec_ok;
	gboolean pix_fmt_ok;
	gboolean field_order_ok;
	gboolean color_range_ok;
	gboolean color_primaries_ok;
	gboolean scale_ok;
	int nb_flux;
	char creation_time[80];

	struct drop_list_s *next;
} drop_list_t;

typedef struct hyperdeck_s {
	char name[2];
	int number;

	gboolean switched_on;
	char ip_address[16];
	char new_ip_address[16];
	gboolean ip_address_is_valid;
	gboolean connected;

	SOCKET socket;
	struct sockaddr_in adresse;

	GThread *connection_thread;
	GMutex connection_mutex;
	gpointer response;

	char protocol_version[16];

	gboolean slot_1_is_mounted;
	gboolean slot_2_is_mounted;
	int disk_slot_id;
	disk_list_t *slot_1_disk_list;
	disk_list_t *slot_2_disk_list;

	int slot_selected;
	int clip_count;
	clip_list_t *list_of_clips;
	GtkWidget *list_box;

	int default_preset_clip_id;

	gboolean play;
	loop_t loop;

	GtkWidget *root_widget;
	GtkWidget *image_slot_1_indicator;
	GtkWidget *image_slot_2_indicator;
	GtkWidget *image_slot_1, *image_slot_2;
	GtkWidget *image_play_button, *image_single_loop_button;

	GtkWidget *play_button, *stop_button, *single_loop_button, *del_button;

	GtkWidget *progress_bar;
	gdouble progress_bar_fraction;

	gboolean reboot;

	drop_list_t *drop_list_file;
	GMutex drop_mutex;
	GThread *drop_thread;

	drop_list_t *remuxing_list_file;
	GMutex remuxing_mutex;
	GThread *remuxing_thread;

	drop_list_t *transcoding_list_file;
	GMutex transcoding_mutex;
	GThread *transcoding_thread;

	GMutex ftp_mutex;

	char *last_file_dropped;
	GMutex last_file_dropped_mutex;

//receive_response_from_hyperdeck
	int recv_len;
	char buffer[4096];
	int index;

	int timeline_empty_retry;

//config_hyperdecks_window
	GtkWidget *on_off;
	GtkEntryBuffer *ip_entry_buffer[4];
} hyperdeck_t;


extern hyperdeck_t hyperdecks[NB_OF_HYPERDECKS];

extern GtkWidget *main_window;


gboolean select_slot_1 (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck);

gboolean select_slot_2 (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck);

gboolean children_window_key_press (GtkWidget *window, GdkEventKey *event);

gboolean select_clip_up (hyperdeck_t *hyperdeck);

gboolean select_clip_down (hyperdeck_t *hyperdeck);

gboolean select_fresque_up (void);

gboolean select_fresque_down (void);


#endif

