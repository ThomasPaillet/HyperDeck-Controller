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

#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <dirent.h>

#include "HyperDeck.h"


typedef struct {
	char *fresque_name;
	int fresque_name_len;
	const char *suffix;

	AVFrame *background_frame;
	AVFrame *fresque_frame;

	hyperdeck_t* first_hyperdeck;
	int nb_flux;
	float sin_minus_7;
	float stride;
	float step;
	char *creation_time;
} transition_rev_task_t;

int transition_type = 0;
int transition_direction = 0;
gboolean transition_return_inv = FALSE;
GdkRGBA transition_stripe_color = { 1.0, 1.0, 1.0, 1.0 };
uint8_t stripe_color_Y_8 = 235, stripe_color_U_8 = 128, stripe_color_V_8 = 128;
uint16_t stripe_color_Y_16 = 60160, stripe_color_U_16 = 32768, stripe_color_V_16 = 32768;
int transition_stripe_width = 100;
int transition_nb_shutters = 6;
int transition_nb_frames = 25;
transition_t transitions[NB_OF_TRANSITIONS];

char *background_dir = "Fresques";
GSList *background_slist = NULL;
int background_slist_length = 0;


gboolean check_background_resolution (const char *file_name)
{
	char name[CLIP_NAME_LENGTH + 9];
	AVFormatContext *av_format_context = NULL;
	AVCodecParameters *av_stream_codec_parameters;
	int stream_index;

	sprintf (name, "%s" G_DIR_SEPARATOR_S "%s", background_dir, file_name);

	if (avformat_open_input (&av_format_context, name, NULL, NULL) < 0) return FALSE;

	if (avformat_find_stream_info (av_format_context, NULL) < 0) {
		avformat_close_input (&av_format_context);
		return FALSE;
	}

	stream_index = av_find_best_stream (av_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (stream_index < 0) {
		avformat_close_input (&av_format_context);
		return FALSE;
	}

	av_stream_codec_parameters = av_format_context->streams[stream_index]->codecpar;

	if (((av_stream_codec_parameters->width / (16 * NB_OF_HYPERDECKS)) == (av_stream_codec_parameters->height / 9)) && (av_stream_codec_parameters->height <= 4320)) {
		avformat_close_input (&av_format_context);
		return TRUE;
	} else {
		avformat_close_input (&av_format_context);
		return FALSE;
	}
}

void fill_background_slist (void)
{
	DIR *dir;
	struct dirent *entree;
	char *name;
	size_t name_len;

	dir = opendir (background_dir);
	if (dir == NULL) return;

	while ((entree = readdir (dir)) != NULL) {
		name_len = strlen (entree->d_name);
		if (name_len >= CLIP_NAME_LENGTH) continue;

		if (check_background_resolution (entree->d_name)) {
			name = g_malloc (name_len + 1);
			memcpy (name, entree->d_name, name_len);
			name[name_len] = '\0';
			background_slist = g_slist_prepend (background_slist, name);
			background_slist_length++;
		}
	}

	closedir (dir);
}

gpointer reverse_transition (transition_rev_task_t *transition_rev)
{
	int i, j;
	float *last_x;
	hyperdeck_t* hyperdeck;
	drop_list_t *drop_list;
	AVFrame *frame_tmp, *frame_out;
	struct SwsContext *sws_context;
DEBUG_S("reverse_transition")
	last_x = g_malloc (nb_lines * sizeof (float));

	frame_tmp = av_frame_alloc ();
	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) frame_tmp->format = AV_PIX_FMT_YUV444P;
	else frame_tmp->format = AV_PIX_FMT_YUV444P16LE;
	frame_tmp->width = hyperdeck_width;
	frame_tmp->height = nb_lines;
	frame_tmp->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_tmp, 0);

	frame_out = av_frame_alloc ();
	frame_out->format = hyperdeck_pix_fmt;
	frame_out->chroma_location = HYPERDECK_CHROMA_LOCATION;
	frame_out->key_frame = 1;
	frame_out->pict_type = AV_PICTURE_TYPE_I;
	if (progressif) {
		frame_out->interlaced_frame = 0;
		frame_out->top_field_first = 0;
	} else {
		frame_out->interlaced_frame = 1;
		frame_out->top_field_first = 1;
	}
	frame_out->color_primaries = hyperdeck_color_primaries;
	frame_out->color_trc = hyperdeck_color_trc;
	frame_out->colorspace = hyperdeck_colorspace;
	frame_out->color_range = AVCOL_RANGE_MPEG;	//the normal 219*2^(n-8) "MPEG" YUV ranges
	frame_out->width = hyperdeck_width;
	frame_out->height = nb_lines;
	frame_out->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_out, 0);

	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P)
		sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P, SWS_BILINEAR, NULL, NULL, NULL);
	else sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P16LE, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P10LE, SWS_BILINEAR, NULL, NULL, NULL);

	for (i = 0, hyperdeck = transition_rev->first_hyperdeck; i < transition_rev->nb_flux; i++, hyperdeck++) {
		drop_list = g_malloc (sizeof (drop_list_t));
		drop_list->full_name = NULL;
		drop_list->file_name_in = NULL;
		memcpy (drop_list->file_name_out, transition_rev->fresque_name, transition_rev->fresque_name_len);
		drop_list->file_name_out_len = transition_rev->fresque_name_len - 5;
		drop_list->file_name_out[drop_list->file_name_out_len++] = '_';
		for (j = 0; transition_rev->suffix[j] != '\0'; j++) 
			drop_list->file_name_out[drop_list->file_name_out_len++] = transition_rev->suffix[j];
		drop_list->file_name_out[drop_list->file_name_out_len++] = '_';
		drop_list->file_name_out[drop_list->file_name_out_len++] = 'R';
		drop_list->file_name_out[drop_list->file_name_out_len++] = (char)hyperdeck->number + 49;
		complete_file_name_out (drop_list);

		if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) render_transition_8 (transition_rev->first_hyperdeck, hyperdeck, drop_list, transition_rev->fresque_frame, transition_rev->background_frame, transition_rev->nb_flux, last_x, transition_rev->sin_minus_7, transition_rev->stride, transition_rev->step, sws_context, frame_tmp, frame_out, transition_rev->creation_time, TRUE);
		else render_transition_16 (transition_rev->first_hyperdeck, hyperdeck, drop_list, transition_rev->fresque_frame, transition_rev->background_frame, transition_rev->nb_flux, last_x, transition_rev->sin_minus_7, transition_rev->stride, transition_rev->step, sws_context, frame_tmp, frame_out, transition_rev->creation_time, TRUE);
	}

	sws_freeContext (sws_context);
	av_frame_unref (frame_out);
	av_frame_free (&frame_out);
	av_frame_unref (frame_tmp);
	av_frame_free (&frame_tmp);
	g_free (last_x);
DEBUG_S("reverse_transition END")
	return NULL;
}

gpointer run_transition_task (transition_task_t *transition_task)
{
	AVFormatContext *av_format_context_background = NULL;
	AVCodecContext *av_codec_context_background;
	AVCodec *av_codec_background = NULL;
	int stream_index;
	AVStream *av_stream_background;
	AVPacket packet;
	AVFrame *first_background_frame, *second_background_frame, *background_frame;
	AVFrame *fresque_frame;
	transition_rev_task_t transition_rev;
	struct SwsContext *sws_context;
	const int *inv_table;
	int srcRange;
	drop_list_t *drop_list;
	AVFrame *frame_tmp, *frame_out;
	int nb_flux, second_frame_width, i, j;
	float *last_x;
	float sin_minus_7, stride, step;
	hyperdeck_t* hyperdeck;
	char name[CLIP_NAME_LENGTH + 9];
	GThread *thread_rev;

	fresque_frame = transition_task->fresque_frame;
	nb_flux = transition_task->nb_flux;

DEBUG_S("run_transition_task")
	sprintf (name, "%s" G_DIR_SEPARATOR_S "%s", background_dir, transition_task->background_name);
DEBUG_S(name)
	if (avformat_open_input (&av_format_context_background, name, NULL, NULL) < 0) {
		g_free (transition_task);
		return NULL;
	}

	if (avformat_find_stream_info (av_format_context_background, NULL) < 0) {
		avformat_close_input (&av_format_context_background);
		g_free (transition_task);
		return NULL;
	}

	stream_index = av_find_best_stream (av_format_context_background, AVMEDIA_TYPE_VIDEO, -1, -1, &av_codec_background, 0);
	if (stream_index < 0) {
		avformat_close_input (&av_format_context_background);
		g_free (transition_task);
		return NULL;
	}

	av_stream_background = av_format_context_background->streams[stream_index];

	if (((av_stream_background->codecpar->width / (16 * NB_OF_HYPERDECKS)) != (av_stream_background->codecpar->height / 9)) || (av_stream_background->codecpar->height > 4320)) {
		avformat_close_input (&av_format_context_background);
		g_free (transition_task);
		return NULL;
	}

	av_codec_context_background = avcodec_alloc_context3 (av_codec_background);
	avcodec_parameters_to_context (av_codec_context_background, av_stream_background->codecpar);

g_mutex_lock (&avcodec_open2_mutex);
	avcodec_open2 (av_codec_context_background, av_codec_background, NULL);
g_mutex_unlock (&avcodec_open2_mutex);

	packet.data = NULL;
	packet.size = 0;
	av_init_packet (&packet);

	do {
		av_packet_unref (&packet);
		if (av_read_frame (av_format_context_background, &packet) < 0) break;
	} while (packet.stream_index != stream_index);

	if (avcodec_send_packet (av_codec_context_background, &packet) < 0) {
		av_packet_unref (&packet);
		avcodec_free_context (&av_codec_context_background);
		avformat_close_input (&av_format_context_background);
		g_free (transition_task);
		return NULL;
	}

	first_background_frame = av_frame_alloc ();

	if (avcodec_receive_frame (av_codec_context_background, first_background_frame) < 0) {
		av_frame_free (&first_background_frame);
		av_packet_unref (&packet);
		avcodec_free_context (&av_codec_context_background);
		avformat_close_input (&av_format_context_background);
		g_free (transition_task);
		return NULL;
	}

	if (nb_lines == 480) second_frame_width = 720 * NB_OF_HYPERDECKS;
	else if (nb_lines == 576) second_frame_width = 720 * NB_OF_HYPERDECKS;
	else if (nb_lines == 720) second_frame_width = 1280 * NB_OF_HYPERDECKS;
	else if (nb_lines == 1080) second_frame_width = 1920 * NB_OF_HYPERDECKS;
	else if (nb_lines == 2160) second_frame_width = 3840 * NB_OF_HYPERDECKS;
	else if (nb_lines == 4320) second_frame_width = 7680 * NB_OF_HYPERDECKS;

	if ((first_background_frame->height != nb_lines) || (first_background_frame->width != second_frame_width) || \
								((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) && (first_background_frame->format != AV_PIX_FMT_YUV444P)) || \
								((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) && (first_background_frame->format != AV_PIX_FMT_YUV444P16LE))) {

		second_background_frame = av_frame_alloc ();
		if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) second_background_frame->format = AV_PIX_FMT_YUV444P;
		else second_background_frame->format = AV_PIX_FMT_YUV444P16LE;
		second_background_frame->pict_type = AV_PICTURE_TYPE_I;
		second_background_frame->height = nb_lines;
		second_background_frame->width = second_frame_width;
		second_background_frame->colorspace = hyperdeck_colorspace;

		av_frame_get_buffer (second_background_frame, 0);

		sws_context = sws_getContext (first_background_frame->width, first_background_frame->height, first_background_frame->format, second_frame_width, nb_lines, second_background_frame->format, SWS_BILINEAR, NULL, NULL, NULL);

		if (av_codec_context_background->colorspace == AVCOL_SPC_BT709) inv_table = sws_getCoefficients (SWS_CS_ITU709);
		else if (av_codec_context_background->colorspace == AVCOL_SPC_BT470BG) inv_table = sws_getCoefficients (SWS_CS_ITU601);
		else if (av_codec_context_background->colorspace == AVCOL_SPC_SMPTE170M) inv_table = sws_getCoefficients (SWS_CS_SMPTE170M);
		else if (av_codec_context_background->colorspace == AVCOL_SPC_SMPTE240M) inv_table = sws_getCoefficients (SWS_CS_SMPTE240M);
		else if (av_codec_context_background->colorspace == AVCOL_SPC_FCC) inv_table = sws_getCoefficients (SWS_CS_FCC);
		else if ((av_codec_context_background->colorspace == AVCOL_SPC_BT2020_NCL) || (av_codec_context_background->colorspace == AVCOL_SPC_BT2020_CL)) inv_table = sws_getCoefficients (SWS_CS_BT2020);
		else inv_table = sws_getCoefficients (SWS_CS_DEFAULT);

		if (av_codec_context_background->color_range == AVCOL_RANGE_MPEG) srcRange = 0;
		else srcRange = 1;

		sws_setColorspaceDetails (sws_context, inv_table, srcRange, hyperdeck_yuv2rgb_coefficients, 0, 0, 65536, 65536);
		sws_scale (sws_context, (const uint8_t * const*)first_background_frame->data, first_background_frame->linesize, 0, first_background_frame->height, second_background_frame->data, second_background_frame->linesize);
		sws_freeContext (sws_context);

		av_frame_unref (first_background_frame);
		av_frame_free (&first_background_frame);

		background_frame = second_background_frame;
	} else background_frame = first_background_frame;

/*	if (nb_lines == 480) second_frame_width = 720 * transition_task->nb_flux;
	else if (nb_lines == 576) second_frame_width = 720 * transition_task->nb_flux;
	else if (nb_lines == 720) second_frame_width = 1280 * transition_task->nb_flux;
	else if (nb_lines == 1080) second_frame_width = 1920 * transition_task->nb_flux;
	else if (nb_lines == 2160) second_frame_width = 3840 * transition_task->nb_flux;
	else if (nb_lines == 4320) second_frame_width = 7680 * transition_task->nb_flux;*/

	if (nb_flux > NB_OF_HYPERDECKS - transition_task->first_hyperdeck->number) nb_flux = NB_OF_HYPERDECKS - transition_task->first_hyperdeck->number;

/*	if ((first_fresque_frame->height != nb_lines) || (first_fresque_frame->width != second_frame_width) || \
								((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) && (first_fresque_frame->format != AV_PIX_FMT_YUV444P)) || \
								((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) && (first_fresque_frame->format != AV_PIX_FMT_YUV444P16LE))) {

		second_fresque_frame = av_frame_alloc ();
		if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) second_fresque_frame->format = AV_PIX_FMT_YUV444P;
		else second_fresque_frame->format = AV_PIX_FMT_YUV444P16LE;
		second_fresque_frame->pict_type = AV_PICTURE_TYPE_I;
		second_fresque_frame->height = nb_lines;
		second_fresque_frame->width = second_frame_width;

		if (nb_lines == 480) second_fresque_frame->colorspace = AVCOL_SPC_SMPTE170M;
		else if (nb_lines == 576) second_fresque_frame->colorspace = AVCOL_SPC_SMPTE170M;
		else if (nb_lines == 720) second_fresque_frame->colorspace = AVCOL_SPC_BT709;
		else if (nb_lines == 1080) second_fresque_frame->colorspace = AVCOL_SPC_BT709;
		else if (nb_lines == 2160) second_fresque_frame->colorspace = AVCOL_SPC_BT709;
		else if (nb_lines == 4320) second_fresque_frame->colorspace = AVCOL_SPC_BT709;

		av_frame_get_buffer (second_fresque_frame, 0);

		sws_context = sws_getContext (first_fresque_frame->width, first_fresque_frame->height, first_fresque_frame->format, second_frame_width, nb_lines, second_fresque_frame->format, SWS_BILINEAR, NULL, NULL, NULL);

		if (transition_task->first_fresque_colorspace == AVCOL_SPC_BT709) inv_table = sws_getCoefficients (SWS_CS_ITU709);
		else if (transition_task->first_fresque_colorspace == AVCOL_SPC_BT470BG) inv_table = sws_getCoefficients (SWS_CS_ITU601);
		else if (transition_task->first_fresque_colorspace == AVCOL_SPC_SMPTE170M) inv_table = sws_getCoefficients (SWS_CS_SMPTE170M);
		else if (transition_task->first_fresque_colorspace == AVCOL_SPC_SMPTE240M) inv_table = sws_getCoefficients (SWS_CS_SMPTE240M);
		else if (transition_task->first_fresque_colorspace == AVCOL_SPC_FCC) inv_table = sws_getCoefficients (SWS_CS_FCC);
		else if ((transition_task->first_fresque_colorspace == AVCOL_SPC_BT2020_NCL) || (transition_task->first_fresque_colorspace == AVCOL_SPC_BT2020_CL)) inv_table = sws_getCoefficients (SWS_CS_BT2020);
		else inv_table = sws_getCoefficients (SWS_CS_DEFAULT);

		if (transition_task->first_fresque_color_range == AVCOL_RANGE_MPEG) srcRange = 0;
		else srcRange = 1;

		sws_setColorspaceDetails (sws_context, inv_table, srcRange, hyperdeck_yuv2rgb_coefficients, 0, 0, 65536, 65536);
		sws_scale (sws_context, (const uint8_t * const*)first_fresque_frame->data, first_fresque_frame->linesize, 0, first_fresque_frame->height, second_fresque_frame->data, second_fresque_frame->linesize);
		sws_freeContext (sws_context);

		fresque_frame = second_fresque_frame;
	} else fresque_frame = first_fresque_frame;*/

	if (nb_lines == 480) {
		if (transition_type == 0) {
			sin_minus_7 = -0.100626063;
			stride = (720 * nb_flux + (0.100626063 * 480)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.100626063;
			stride = (720 * nb_flux + (0.100626063 * 480)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 720 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	} else if (nb_lines == 576) {
		if (transition_type == 0) {
			sin_minus_7 = -0.085689382;
			stride = (720 * nb_flux + (0.085689382 * 576)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.085689382;
			stride = (720 * nb_flux + (0.085689382 * 576)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 720 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	} else if (nb_lines == 720) {
		if (transition_type == 0) {
			sin_minus_7 = -0.121869343;
			stride = (1280 * nb_flux + (0.121869343 * 720)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.121869343;
			stride = (1280 * nb_flux + (0.121869343 * 720)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 1280 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	} else if (nb_lines == 1080) {
		if (transition_type == 0) {
			sin_minus_7 = -0.121869343;
			stride = (1920 * nb_flux + (0.121869343 * 1080)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.121869343;
			stride = (1920 * nb_flux + (0.121869343 * 1080)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 1920 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	} else if (nb_lines == 2160) {
		if (transition_type == 0) {
			sin_minus_7 = -0.121869343;
			stride = (3840 * nb_flux + (0.121869343 * 2160)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.121869343;
			stride = (3840 * nb_flux + (0.121869343 * 2160)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 3840 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	} else if (nb_lines == 4320) {
		if (transition_type == 0) {
			sin_minus_7 = -0.121869343;
			stride = (7680 * nb_flux + (0.121869343 * 4320)) / (transition_nb_frames - 1);
		} else if (transition_type == 1) {
			sin_minus_7 = -0.121869343;
			stride = (7680 * nb_flux + (0.121869343 * 4320)) / (transition_nb_frames - 1);
		} else if (transition_type == 2) {
			stride = 7680 / transition_nb_shutters;
			step = stride / (float)(transition_nb_frames - 1);
		}
	}

	transition_rev.background_frame = background_frame;
	transition_rev.fresque_frame = fresque_frame;

	transition_rev.fresque_name = transition_task->file_name;
	transition_rev.fresque_name_len = transition_task->file_name_len;
	transition_rev.suffix = transition_task->suffix;

	transition_rev.first_hyperdeck = transition_task->first_hyperdeck;
	transition_rev.nb_flux = nb_flux;
	transition_rev.sin_minus_7 = sin_minus_7;
	transition_rev.stride = stride;
	transition_rev.step = step;
	transition_rev.creation_time = transition_task->creation_time;

	thread_rev = g_thread_new (NULL, (GThreadFunc)reverse_transition, &transition_rev);

	last_x = g_malloc (nb_lines * sizeof (float));

	frame_tmp = av_frame_alloc ();
	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) frame_tmp->format = AV_PIX_FMT_YUV444P;
	else frame_tmp->format = AV_PIX_FMT_YUV444P16LE;
	frame_tmp->width = hyperdeck_width;
	frame_tmp->height = nb_lines;
	frame_tmp->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_tmp, 0);

	frame_out = av_frame_alloc ();
	frame_out->format = hyperdeck_pix_fmt;
	frame_out->chroma_location = HYPERDECK_CHROMA_LOCATION;
	frame_out->key_frame = 1;
	frame_out->pict_type = AV_PICTURE_TYPE_I;
	if (progressif) {
		frame_out->interlaced_frame = 0;
		frame_out->top_field_first = 0;
	} else {
		frame_out->interlaced_frame = 1;
		frame_out->top_field_first = 1;
	}
	frame_out->color_primaries = hyperdeck_color_primaries;
	frame_out->color_trc = hyperdeck_color_trc;
	frame_out->colorspace = hyperdeck_colorspace;
	frame_out->color_range = AVCOL_RANGE_MPEG;	//the normal 219*2^(n-8) "MPEG" YUV ranges
	frame_out->width = hyperdeck_width;
	frame_out->height = nb_lines;
	frame_out->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_out, 0);

	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P)
		sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P, SWS_BILINEAR, NULL, NULL, NULL);
	else sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P16LE, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P10LE, SWS_BILINEAR, NULL, NULL, NULL);

	for (i = 0, hyperdeck = transition_task->first_hyperdeck; i < nb_flux; i++, hyperdeck++) {
		drop_list = g_malloc (sizeof (drop_list_t));
		drop_list->full_name = NULL;
		drop_list->file_name_in = NULL;
		memcpy (drop_list->file_name_out, transition_task->file_name, transition_task->file_name_len);
		drop_list->file_name_out_len = transition_task->file_name_len - 5;
		drop_list->file_name_out[drop_list->file_name_out_len++] = '_';
		for (j = 0; transition_task->suffix[j] != '\0'; j++) 
			drop_list->file_name_out[drop_list->file_name_out_len++] = transition_task->suffix[j];
		drop_list->file_name_out[drop_list->file_name_out_len++] = (char)hyperdeck->number + 49;
		complete_file_name_out (drop_list);

		if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) render_transition_8 (transition_task->first_hyperdeck, hyperdeck, drop_list, background_frame, fresque_frame, nb_flux, last_x, sin_minus_7, stride, step, sws_context, frame_tmp, frame_out, transition_task->creation_time, FALSE);
		else render_transition_16 (transition_task->first_hyperdeck, hyperdeck, drop_list, background_frame, fresque_frame, nb_flux, last_x, sin_minus_7, stride, step, sws_context, frame_tmp, frame_out, transition_task->creation_time, FALSE);
	}

	sws_freeContext (sws_context);
	av_frame_unref (frame_out);
	av_frame_free (&frame_out);
	av_frame_unref (frame_tmp);
	av_frame_free (&frame_tmp);
	g_free (last_x);

	g_thread_join (thread_rev);

	av_frame_unref (background_frame);
	av_frame_free (&background_frame);

	avcodec_free_context (&av_codec_context_background);
	avformat_close_input (&av_format_context_background);

	g_free (transition_task);
DEBUG_S("run_transition_task END")
	return NULL;
}

void init_transitions (void)
{
	int i, j;

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		transitions[i].switched_on = FALSE;
		transitions[i].suffix[0] = i + 65;
		transitions[i].suffix[1] = '\0';
		transitions[i].file_name = g_malloc (CLIP_NAME_LENGTH);
		transitions[i].file_name[0] = '\0';

		for (j = 0; j < NB_OF_HYPERDECKS; j++) transitions[i].thread[j] = NULL;
	}
}

