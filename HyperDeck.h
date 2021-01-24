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

#ifndef __HYPERDECK_H
#define __HYPERDECK_H


#include <gtk/gtk.h>


#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>


#define NB_OF_HYPERDECKS 4


#define CLIP_NAME_LENGTH 128


#define DEBUG_HYPERDECK


#ifdef _WIN32
	#include <winsock2.h>
	void WSAInit (void);
//	extern u_long ioctlsocket_arg;
//	#define ENABLE_NON_BLOCKING_MODE(s) ioctlsocket ((s), FIONBIO, &ioctlsocket_arg);

	#define SLEEP(t) Sleep (t * 1000);

#ifdef DEBUG_HYPERDECK
	#include <stdio.h>
	extern GMutex debug_mutex;
	extern FILE *debug_file;
	extern FILE *debug_hyperdeck[NB_OF_HYPERDECKS];
	extern FILE *debug_drop_thread[NB_OF_HYPERDECKS];
	#define DEBUG_INIT char file_name[32]; g_mutex_init (&debug_mutex); debug_file = fopen ("debug.txt", "w"); \
		for (i = 0; i < NB_OF_HYPERDECKS; i++) { \
			sprintf (file_name, "hyperdeck_%d.txt", i + 1); debug_hyperdeck[i] = fopen (file_name, "w"); \
			sprintf (file_name, "drop_thread_%d.txt", i + 1); debug_drop_thread[i] = fopen (file_name, "w"); }
	#define DEBUG_S(m) g_mutex_lock (&debug_mutex); fprintf (debug_file, "%s\n", m); fflush (debug_file); g_mutex_unlock (&debug_mutex);
	#define DEBUG_D(m) g_mutex_lock (&debug_mutex); fprintf (debug_file, "%d\n", m); fflush (debug_file); g_mutex_unlock (&debug_mutex);
	#define DEBUG_F(m) g_mutex_lock (&debug_mutex); fprintf (debug_file, "%f\n", m); fflush (debug_file); g_mutex_unlock (&debug_mutex);
	#define DEBUG_P(m) g_mutex_lock (&debug_mutex); fprintf (debug_file, "%p\n", m); fflush (debug_file); g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_S(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_hyperdeck[hyperdeck->number], "%s\n", m); \
		fflush (debug_hyperdeck[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_D(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_hyperdeck[hyperdeck->number], "%d\n", m); \
		fflush (debug_hyperdeck[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_F(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_hyperdeck[hyperdeck->number], "%f\n", m); \
		fflush (debug_hyperdeck[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_S(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_drop_thread[hyperdeck->number], "%s\n", m); \
		fflush (debug_drop_thread[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_D(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_drop_thread[hyperdeck->number], "%d\n", m); \
		fflush (debug_drop_thread[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_L(m) g_mutex_lock (&debug_mutex); \
		fprintf (debug_drop_thread[hyperdeck->number], "%ld\n", m); \
		fflush (debug_drop_thread[hyperdeck->number]); \
		g_mutex_unlock (&debug_mutex);
#else
	#define DEBUG_INIT
	#define DEBUG_S(m)
	#define DEBUG_D(m)
	#define DEBUG_F(m)
	#define DEBUG_P(m)
	#define DEBUG_HYPERDECK_S(m)
	#define DEBUG_HYPERDECK_D(m)
	#define DEBUG_HYPERDECK_F(m)
	#define DEBUG_DROP_THREAD_S(m)
	#define DEBUG_DROP_THREAD_D(m)
	#define DEBUG_DROP_THREAD_L(m)
#endif

#elif defined (__linux)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#define SOCKET int
//	#define ENABLE_NON_BLOCKING_MODE(s) fcntl ((s), F_SETFL, fcntl ((s), F_GETFL) | O_NONBLOCK);
	#define closesocket close
	#define WSAInit()
	#define WSACleanup()

	#define SLEEP(t) sleep (t);

#ifdef DEBUG_HYPERDECK
	extern GMutex debug_mutex;
	#define DEBUG_INIT g_mutex_init (&debug_mutex);
	#define DEBUG_S(m) g_mutex_lock (&debug_mutex); printf ("%s\n", m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_D(m) g_mutex_lock (&debug_mutex); printf ("%d\n", m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_F(m) g_mutex_lock (&debug_mutex); printf ("%f\n", m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_P(m) g_mutex_lock (&debug_mutex); printf ("%p\n", m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_S(m) g_mutex_lock (&debug_mutex); printf ("hyperdeck %d: %s\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_D(m) g_mutex_lock (&debug_mutex); printf ("hyperdeck %d: %d\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_HYPERDECK_F(m) g_mutex_lock (&debug_mutex); printf ("hyperdeck %d: %f\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_S(m) g_mutex_lock (&debug_mutex); printf ("drop_thread %d: %s\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_D(m) g_mutex_lock (&debug_mutex); printf ("drop_thread %d: %d\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
	#define DEBUG_DROP_THREAD_L(m) g_mutex_lock (&debug_mutex); printf ("drop_thread %d: %ld\n", hyperdeck->number + 1, m); g_mutex_unlock (&debug_mutex);
#else
	#define DEBUG_INIT
	#define DEBUG_S(m)
	#define DEBUG_D(m)
	#define DEBUG_F(m)
	#define DEBUG_P(m)
	#define DEBUG_HYPERDECK_S(m)
	#define DEBUG_HYPERDECK_D(m)
	#define DEBUG_HYPERDECK_F(m)
	#define DEBUG_DROP_THREAD_S(m)
	#define DEBUG_DROP_THREAD_D(m)
	#define DEBUG_DROP_THREAD_L(m)
#endif

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

	unsigned char *ffmpeg_buffer;
	int ffmpeg_buffer_size;

	AVFormatContext *av_format_context_in;
	AVCodec *av_codec_in;
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
	char creation_time[32];

	struct drop_list_s *next;
} drop_list_t;

typedef struct {
	char name[2];
	int number;

	gboolean switched_on;
	char adresse_ip[16];
	char new_adresse_ip[16];
	gboolean adresse_ip_is_valid;
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
	GtkWidget *slot_1_event_box, *slot_2_event_box;
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


//HyperDeck.c
extern hyperdeck_t hyperdecks[NB_OF_HYPERDECKS];

extern GtkWidget *main_window;


//Config.c
void show_config_hyperdecks_window (void);

void show_config_presets_window (void);

void show_config_transitions_window (void);

gboolean read_config_file (void);

void write_config_file (void);


//Preset.c
#define NB_OF_PRESETS 8
#define PRESETS_NAME_LENGTH 12

typedef struct {
	hyperdeck_t *hyperdeck;
	int slot;
	char *name;

//config_presets_window
	GtkWidget *radio_button_slot_1, *radio_button_slot_2;
	GtkWidget *combo_box_text;
} preset_clip_t;

typedef struct {
	GtkWidget *button;
	gboolean switched_on;

	preset_clip_t clips[NB_OF_HYPERDECKS];

//config_presets_window
	GtkWidget *on_off;
	GtkEntryBuffer *entry_buffer;
} preset_t;

extern char *default_presets_name[NB_OF_PRESETS];

extern preset_t presets[NB_OF_PRESETS];

extern preset_t *preset_to_load;

void load_preset (void);

GtkWidget *create_presets_frame (void);

void init_presets (void);


//Transition.c
#define NB_OF_TRANSITIONS NB_OF_PRESETS
#define TRANSITION_SUFFIX_LENGTH 9

typedef struct {
	gboolean switched_on;
	char suffix[TRANSITION_SUFFIX_LENGTH];
	char *file_name;

//config_transitions_window
	GtkWidget *on_off;
	GtkEntryBuffer *entry_buffer;
	GtkWidget *combo_box_text;

	GThread *thread[NB_OF_HYPERDECKS];
} transition_t;

typedef struct {
	const char *background_name;
	char file_name[CLIP_NAME_LENGTH];
	int file_name_len;
	const char *suffix;
	AVFrame *fresque_frame;
	int nb_flux;
	hyperdeck_t* first_hyperdeck;
	char creation_time[32];
} transition_task_t;

extern int transition_type;
extern int transition_direction;
extern gboolean transition_return_inv;
extern GdkRGBA transition_stripe_color;
uint8_t stripe_color_Y_8, stripe_color_U_8, stripe_color_V_8;
uint16_t stripe_color_Y_16, stripe_color_U_16, stripe_color_V_16;
extern int transition_stripe_width;
extern int transition_nb_shutters;
extern int transition_nb_frames;
extern transition_t transitions[NB_OF_TRANSITIONS];

extern GSList *background_slist;
extern int background_slist_length;

void fill_background_slist (void);

gpointer run_transition_task (transition_task_t *transition_task);

void init_transitions (void);


//Render_Transition_8.c
void stripe_color_RGB_to_YUV_8 (void);

void render_transition_8 (hyperdeck_t* first_hyperdeck, hyperdeck_t* hyperdeck, drop_list_t *drop_list, AVFrame *background_frame, AVFrame *fresque_frame, int nb_flux, float *last_x, float sin_minus_7, float stride, float step, struct SwsContext *sws_context, AVFrame *frame_tmp, AVFrame *frame_out, char *creation_time, gboolean reverse);


//Render_Transition_16.c
void stripe_color_RGB_to_YUV_16 (void);

void render_transition_16 (hyperdeck_t* first_hyperdeck, hyperdeck_t* hyperdeck, drop_list_t *drop_list, AVFrame *background_frame, AVFrame *fresque_frame, int nb_flux, float *last_x, float sin_minus_7, float stride, float step, struct SwsContext *sws_context, AVFrame *frame_tmp, AVFrame *frame_out, char *creation_time, gboolean reverse);


//Transcoding.c
typedef struct {
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *progress_bar;
	int64_t nb_frames;
	int64_t frame_count;
	guint g_source_id;
} remuxing_frame_t;

typedef struct {
	GtkWidget *frame;
	GtkWidget *src_file_name_label;
	GtkWidget *dst_file_name_label[NB_OF_HYPERDECKS];
	GtkWidget *progress_bar;
	int64_t nb_frames;
	int frame_count;
	guint g_source_id;
} transcoding_frame_t;

extern remuxing_frame_t remuxing_frames[NB_OF_HYPERDECKS];
extern transcoding_frame_t transcoding_frames[NB_OF_HYPERDECKS];

int check_need_for_transcoding (hyperdeck_t* hyperdeck, drop_list_t *drop_list);

gpointer hyperdeck_remux (hyperdeck_t* hyperdeck);

gpointer hyperdeck_transcode (hyperdeck_t* hyperdeck);

void create_transcoding_frames (GtkBox *box);


//HyperDeck_Codec.c
extern char *video_format;
extern int nb_lines;		//480, 576, 720, 1080, 2160, 4320
extern gboolean progressif;
extern float frequency;		//23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0

extern int hyperdeck_width;
extern AVRational hyperdeck_sample_aspect_ratio;
extern enum AVColorPrimaries hyperdeck_color_primaries;
extern enum AVColorTransferCharacteristic hyperdeck_color_trc;
extern enum AVColorSpace hyperdeck_colorspace;
extern const int *hyperdeck_yuv2rgb_coefficients;
extern AVRational hyperdeck_time_base;
extern AVRational hyperdeck_framerate;
extern char *video_format_label;

extern char *file_ext_mov, *file_ext_mxf;

extern char *file_format;
extern char *file_ext;
extern enum AVCodecID hyperdeck_codec;
extern int codec_quality;
extern enum AVPixelFormat hyperdeck_pix_fmt;
extern int dnxhd_bitrate;

#define HYPERDECK_CHROMA_LOCATION AVCHROMA_LOC_UNSPECIFIED

extern AVCodec *av_codec_dnxhd, *av_codec_prores, *av_codec_out;

extern GMutex avcodec_open2_mutex;

void create_output_context (hyperdeck_t* hyperdeck, AVFormatContext **av_format_context, AVCodecContext **av_codec_context, AVStream **av_stream, char *creation_time);

AVFilterGraph* create_filter_graph (AVFilterContext **av_filter_context_in, AVFilterContext **av_filter_context_out, AVCodecContext *av_codec_context_in, const char *filter_descr);

GtkWidget* get_libavformat_version (void);
GtkWidget* get_libavcodec_version (void);
GtkWidget* get_libavfilter_version (void);

void init_hyperdeck_codec (void);


//Fresque_Batch.c
typedef struct fresque_batch_s {
	gboolean initialized;

	char name[CLIP_NAME_LENGTH];
	int name_len;
	GtkWidget *list_box_row;
	gint index;

	GtkWidget *list_box;
	int nb_fresques;

	AVFrame *fresque_frame;
	int nb_flux;
	int first_hyperdeck_number;

	struct fresque_batch_s *next;
} fresque_batch_t;

extern fresque_batch_t *fresque_batches;

extern GMutex fresque_batch_mutex;

extern GtkWidget *add_transition_frame;

void initialize_fresque_batch (fresque_batch_t *fresque_batch);

void create_add_transition_frame (GtkBox *box);

void init_fresque_batch (void);


//Fresque.c
typedef struct fresque {
	const gchar *name;
	GtkWidget *list_box_row;

	int clips_id[NB_OF_HYPERDECKS];

	fresque_batch_t *parent_fresque_batch;

	struct fresque *next;
} fresque_t;

extern fresque_t *fresques;

extern fresque_t *current_fresque;

extern GtkWidget *fresques_list_box;

extern int fresques_list_box_num;

extern GtkWidget *fresques_stop_button, *fresques_play_button, *fresques_loop_button;

void clean_fresques (void);

void deselect_fresque (void);

void load_fresque (GtkListBox *list_box, GtkListBoxRow *list_box_row);

GtkWidget *create_fresques_frame (void);


//File.c
#define REFRESH_WAITING_TIME 600

gboolean refresh_hyperdeck_list_of_clips (hyperdeck_t *hyperdeck);

gpointer purge_hyperdeck (hyperdeck_t *hyperdeck);

gboolean delete_clip (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck);

gboolean delete_fresque (GtkWidget *event_box, GdkEventButton *event);

gpointer drop_to_hyperdeck (hyperdeck_t* hyperdeck);

void hyperdeck_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, hyperdeck_t *hyperdeck);

void fresques_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time);

void complete_file_name_out (drop_list_t *drop_list);

void init_file (void);


//Misc.c
typedef struct {
	char text[512];
	GtkWidget *label;
} g_source_label_t;

gboolean g_source_hide_widget (GtkWidget *widget);

gboolean g_source_show_widget (GtkWidget *widget);

gboolean g_source_label_set_text (g_source_label_t *source_label);

gboolean g_source_init_progress_bar (GtkWidget *progress_bar);

gboolean g_source_update_remuxing_progress_bar (remuxing_frame_t *remuxing_frame);

gboolean g_source_init_remuxing_progress_bar (remuxing_frame_t *remuxing_frame);

gboolean g_source_end_remuxing_progress_bar (remuxing_frame_t *remuxing_frame);

gboolean g_source_hide_remuxing_progress_bar (gpointer index);

gboolean g_source_init_transcoding_progress_bar (transcoding_frame_t *transcoding_frame);

gboolean g_source_hide_transcoding_progress_bar (gpointer index);

gboolean g_source_init_hyperdeck_progress_bar (hyperdeck_t* hyperdeck);

gboolean g_source_hide_hyperdeck_progress_bar (hyperdeck_t* hyperdeck);

gboolean g_source_consume_thread (GThread *thread);

void show_message_window (const gchar* message);

void save_hyperdeck_state (void);

void restore_hyperdeck_state (void);


//Pixbufs.c
extern GdkPixbuf *pixbuf_1, *pixbuf_2, *pixbuf_3, *pixbuf_4, *pixbuf_5, *pixbuf_6, *pixbuf_7, *pixbuf_8, *pixbuf_9;
extern GdkPixbuf *pixbuf_10, *pixbuf_11, *pixbuf_12, *pixbuf_13, *pixbuf_14;
extern GdkPixbuf *pixbuf_S1NS, *pixbuf_S1S, *pixbuf_S1E, *pixbuf_S1F, *pixbuf_S2NS, *pixbuf_S2S, *pixbuf_S2E, *pixbuf_S2F;
extern GdkPixbuf *pixbuf_BPOff, *pixbuf_BPOn, *pixbuf_BS;
extern GdkPixbuf *pixbuf_loop[4];
extern GdkPixbuf *pixbuf_BDel;

extern GdkPixbuf *pixbuf_Up, *pixbuf_Down;

extern GdkPixbuf *pixbuf_Logo;
extern GdkPixbuf *pixbuf_Icon;

void load_pixbufs (void);


#endif

