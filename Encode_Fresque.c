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

#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>

#include "HyperDeck.h"


void split_and_encode_fresque (hyperdeck_t* first_hyperdeck, drop_list_t *first_drop_list)
{
	fresque_batch_t *fresque_batch_tmp;

	drop_list_t *drop_list_array_ptr[NB_OF_HYPERDECKS];
	hyperdeck_t* hyperdeck_ptr;
	g_source_label_t *source_label;

	AVFormatContext *av_format_context_in = first_drop_list->av_format_context_in;
	AVCodec *av_codec_in = first_drop_list->av_codec_in;
	int stream_index = first_drop_list->stream_index;
	int second_frame_width, nb_flux;

	AVFormatContext *av_format_context_out[NB_OF_HYPERDECKS] = { NULL };
	AVCodecContext *av_codec_context_in, *av_codec_context_out[NB_OF_HYPERDECKS];
	AVStream *av_stream_in, *av_stream_out[NB_OF_HYPERDECKS];
	AVPacket packet_in, packet_out;
	AVFrame *first_frame_in, *second_frame_in, *frame_in, *fresque_frame;
	struct SwsContext *sws_context_in = NULL, *fresque_sws_context = NULL;
	const int *inv_table;
	int srcRange;
	const int *table;
	int dstRange;
	int brightness;
	int contrast;
	int saturation;
	AVFrame *frame_out[NB_OF_HYPERDECKS];
	int frame_duration[NB_OF_HYPERDECKS];
	int video_frame_count = 0, i, j;
	transition_task_t *transition_task;
DEBUG_S("split_and_encode_fresque")
	if (nb_lines == 480) second_frame_width = 720 * first_drop_list->nb_flux;
	else if (nb_lines == 576) second_frame_width = 720 * first_drop_list->nb_flux;
	else if (nb_lines == 720) second_frame_width = 1280 * first_drop_list->nb_flux;
	else if (nb_lines == 1080) second_frame_width = 1920 * first_drop_list->nb_flux;
	else if (nb_lines == 2160) second_frame_width = 3840 * first_drop_list->nb_flux;
	else if (nb_lines == 4320) second_frame_width = 7680 * first_drop_list->nb_flux;

	if (first_drop_list->nb_flux > NB_OF_HYPERDECKS - first_hyperdeck->number) nb_flux = NB_OF_HYPERDECKS - first_hyperdeck->number;
	else nb_flux = first_drop_list->nb_flux;

	fresque_batch_tmp = g_malloc (sizeof (fresque_batch_t));
	memcpy (fresque_batch_tmp->name, first_drop_list->file_name_out, first_drop_list->file_name_out_len);
	fresque_batch_tmp->name[first_drop_list->file_name_out_len] = '\0';
	fresque_batch_tmp->name_len = first_drop_list->file_name_out_len;
DEBUG_S(fresque_batch_tmp->name)

	for (i = 1, hyperdeck_ptr = first_hyperdeck + 1; i < nb_flux; i++, hyperdeck_ptr++) {
		drop_list_array_ptr[i] = g_malloc (sizeof (drop_list_t));
		drop_list_array_ptr[i]->full_name = NULL;
		drop_list_array_ptr[i]->file_name_in = NULL;
		memcpy (drop_list_array_ptr[i]->file_name_out, first_drop_list->file_name_out, first_drop_list->file_name_out_len);
		drop_list_array_ptr[i]->file_name_out_len = first_drop_list->file_name_out_len;
		drop_list_array_ptr[i]->file_name_out[drop_list_array_ptr[i]->file_name_out_len++] = (char)hyperdeck_ptr->number + 49;
		complete_file_name_out (drop_list_array_ptr[i]);

		source_label = g_malloc (sizeof (g_source_label_t));
		sprintf (source_label->text, "%s (%s)", drop_list_array_ptr[i]->file_name_out, video_format_label);
		source_label->label = transcoding_frames[first_hyperdeck->number].dst_file_name_label[i];
		g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

		g_idle_add ((GSourceFunc)g_source_show_widget, transcoding_frames[first_hyperdeck->number].dst_file_name_label[i]);
	}
	while (i < NB_OF_HYPERDECKS) g_idle_add ((GSourceFunc)g_source_hide_widget, transcoding_frames[first_hyperdeck->number].dst_file_name_label[i++]);

	drop_list_array_ptr[0] = first_drop_list;
	first_drop_list->file_name_out[first_drop_list->file_name_out_len++] = (char)first_hyperdeck->number + 49;
	complete_file_name_out (first_drop_list);

	source_label = g_malloc (sizeof (g_source_label_t));
	sprintf (source_label->text, "%s (%s)", first_drop_list->file_name_out, video_format_label);
	source_label->label = transcoding_frames[first_hyperdeck->number].dst_file_name_label[0];
	g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

	g_idle_add ((GSourceFunc)g_source_show_widget, transcoding_frames[first_hyperdeck->number].frame);

	av_stream_in = av_format_context_in->streams[stream_index];

	av_codec_context_in = avcodec_alloc_context3 (av_codec_in);
	avcodec_parameters_to_context (av_codec_context_in, av_stream_in->codecpar);

g_mutex_lock (&avcodec_open2_mutex);
	avcodec_open2 (av_codec_context_in, av_codec_in, NULL);
g_mutex_unlock (&avcodec_open2_mutex);

	if (av_codec_context_in->colorspace == AVCOL_SPC_BT709) inv_table = sws_getCoefficients (SWS_CS_ITU709);
	else if (av_codec_context_in->colorspace == AVCOL_SPC_BT470BG) inv_table = sws_getCoefficients (SWS_CS_ITU601);
	else if (av_codec_context_in->colorspace == AVCOL_SPC_SMPTE170M) inv_table = sws_getCoefficients (SWS_CS_SMPTE170M);
	else if (av_codec_context_in->colorspace == AVCOL_SPC_SMPTE240M) inv_table = sws_getCoefficients (SWS_CS_SMPTE240M);
	else if (av_codec_context_in->colorspace == AVCOL_SPC_FCC) inv_table = sws_getCoefficients (SWS_CS_FCC);
	else if ((av_codec_context_in->colorspace == AVCOL_SPC_BT2020_NCL) || (av_codec_context_in->colorspace == AVCOL_SPC_BT2020_CL)) inv_table = sws_getCoefficients (SWS_CS_BT2020);
	else inv_table = sws_getCoefficients (SWS_CS_DEFAULT);

	if (av_codec_context_in->color_range == AVCOL_RANGE_MPEG) srcRange = 0;
	else srcRange = 1;

	packet_in.data = NULL;
	packet_in.size = 0;
	av_init_packet (&packet_in);

	first_frame_in = av_frame_alloc ();

	if ((av_codec_context_in->height != nb_lines) || (av_codec_context_in->width != second_frame_width) || \
											(av_codec_context_in->pix_fmt != hyperdeck_pix_fmt)) {
		second_frame_in = av_frame_alloc ();
		second_frame_in->format = hyperdeck_pix_fmt;
		second_frame_in->pict_type = AV_PICTURE_TYPE_I;
		second_frame_in->height = nb_lines;
		second_frame_in->width = second_frame_width;
		second_frame_in->colorspace = hyperdeck_colorspace;

		av_frame_get_buffer (second_frame_in, 0);

		sws_context_in = sws_getContext (av_codec_context_in->width, av_codec_context_in->height, av_codec_context_in->pix_fmt, second_frame_width, nb_lines, hyperdeck_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);

printf ("sws_setColorspaceDetails = %d\n\n", sws_setColorspaceDetails (sws_context_in, inv_table, srcRange, hyperdeck_yuv2rgb_coefficients, 0, 0, 65536, 65536));
/*sws_getColorspaceDetails (sws_context_in, &inv_table, &srcRange, &table, &dstRange, &brightness, &contrast, &saturation);
printf ("inv_table: %d, %d, %d, %d\n", inv_table[0], inv_table[1], inv_table[2], inv_table[3]);
printf ("table: %d, %d, %d, %d\n", table[0], table[1], table[2], table[3]);
printf ("srcRange: %d, dstRange: %d, brightness: %d, contrast: %d, saturation: %d\n", srcRange, dstRange, brightness, contrast, saturation);*/

		frame_in = second_frame_in;
	} else frame_in = first_frame_in;

	for (i = 0, hyperdeck_ptr = first_hyperdeck; i < nb_flux; i++, hyperdeck_ptr++) {
		create_output_context (hyperdeck_ptr, &av_format_context_out[i], &av_codec_context_out[i], &av_stream_out[i], first_drop_list->creation_time);

		if (avformat_write_header (av_format_context_out[i], NULL) < 0) {
			avcodec_free_context (&av_codec_context_out[i]);
			avformat_free_context (av_format_context_out[i]);
			if (i == 0) g_free (drop_list_array_ptr[0]->full_name);
			g_free (drop_list_array_ptr[i]);
			drop_list_array_ptr[i] = NULL;
			continue;
		}

		frame_duration[i] = (av_stream_out[i]->time_base.den / av_codec_context_out[i]->framerate.num) * av_codec_context_out[i]->framerate.den;

		frame_out[i] = av_frame_alloc ();
		frame_out[i]->format = hyperdeck_pix_fmt;
		frame_out[i]->chroma_location = HYPERDECK_CHROMA_LOCATION;
		frame_out[i]->key_frame = 1;
		frame_out[i]->pict_type = AV_PICTURE_TYPE_I;
		if (progressif) {
			frame_out[i]->interlaced_frame = 0;
			frame_out[i]->top_field_first = 0;
		} else {
			frame_out[i]->interlaced_frame = 1;
			frame_out[i]->top_field_first = 1;
		}
		frame_out[i]->color_primaries = hyperdeck_color_primaries;
		frame_out[i]->color_trc = hyperdeck_color_trc;
		frame_out[i]->colorspace = hyperdeck_colorspace;
		frame_out[i]->color_range = AVCOL_RANGE_MPEG;	//the normal 219*2^(n-8) "MPEG" YUV ranges
		frame_out[i]->width = hyperdeck_width;
		frame_out[i]->height = nb_lines;
		frame_out[i]->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
		av_frame_get_buffer (frame_out[i], 0);
	}

	packet_out.data = NULL;
	packet_out.size = 0;
	av_init_packet (&packet_out);

	while (av_read_frame (av_format_context_in, &packet_in) == 0) {
		if (packet_in.stream_index != stream_index) {
			av_packet_unref (&packet_in);
			continue;
		}

		if (avcodec_send_packet (av_codec_context_in, &packet_in) < 0) {
			av_packet_unref (&packet_in);
			continue;
		}

		av_frame_unref (first_frame_in);
		if (avcodec_receive_frame (av_codec_context_in, first_frame_in) < 0) {
			av_packet_unref (&packet_in);
			continue;
		}

		if (sws_context_in != NULL) sws_scale (sws_context_in, (const uint8_t * const*)first_frame_in->data, first_frame_in->linesize, 0, first_frame_in->height, second_frame_in->data, second_frame_in->linesize);

		for (i = 0, hyperdeck_ptr = first_hyperdeck; i < nb_flux; i++, hyperdeck_ptr++) {
			if (drop_list_array_ptr[i] == NULL) continue;

			if (nb_lines == 480) {
				for (j = 0; j < 480; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 720) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 360) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 360) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1440) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 720) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 720) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1440) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 1440) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 1440) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 1440) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			} else if (nb_lines == 576) {
				for (j = 0; j < 576; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 720) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 360) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 360) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1440) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 720) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 720) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1440) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 1440) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 1440) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 1440) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			} else if (nb_lines == 720) {
				for (j = 0; j < 720; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1280) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 640) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 640) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 2560) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 1280) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 1280) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 2560) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 2560) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 2560) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 2560) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			} else if (nb_lines == 1080) {
				for (j = 0; j < 1080; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 1920) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 960) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 960) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 3840) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 1920) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 1920) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 3840) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 3840) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 3840) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 3840) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			} else if (nb_lines == 2160) {
				for (j = 0; j < 2160; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 3840) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 1920) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 1920) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 7680) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 3840) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 3840) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 7680) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 7680) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 7680) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 7680) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			} else if (nb_lines == 4320) {
				for (j = 0; j < 4320; j++) {
					if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 7680) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 3840) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 3840) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 15360) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 7680) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 7680) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
					} else if (hyperdeck_pix_fmt == AV_PIX_FMT_YUVA444P10LE) {
						memcpy (frame_out[i]->data[0] + (j * frame_out[i]->linesize[0]), frame_in->data[0] + (i * 15360) + (j * frame_in->linesize[0]), frame_out[i]->linesize[0]);
						memcpy (frame_out[i]->data[1] + (j * frame_out[i]->linesize[1]), frame_in->data[1] + (i * 15360) + (j * frame_in->linesize[1]), frame_out[i]->linesize[1]);
						memcpy (frame_out[i]->data[2] + (j * frame_out[i]->linesize[2]), frame_in->data[2] + (i * 15360) + (j * frame_in->linesize[2]), frame_out[i]->linesize[2]);
						memcpy (frame_out[i]->data[3] + (j * frame_out[i]->linesize[3]), frame_in->data[3] + (i * 15360) + (j * frame_in->linesize[3]), frame_out[i]->linesize[3]);
					}
				}
			}

			frame_out[i]->pts = video_frame_count * frame_duration[i];
			avcodec_send_frame (av_codec_context_out[i], frame_out[i]);

			avcodec_receive_packet (av_codec_context_out[i], &packet_out);
			packet_out.stream_index = av_stream_out[i]->index;
			packet_out.duration = frame_duration[i];
			av_interleaved_write_frame (av_format_context_out[i], &packet_out);
		}
		video_frame_count++;
		transcoding_frames[first_hyperdeck->number].frame_count = video_frame_count;

		av_packet_unref (&packet_in);
	}

	if (sws_context_in != NULL) {
		sws_freeContext (sws_context_in);
		av_frame_unref (second_frame_in);
		av_frame_free (&second_frame_in);
	}

	avcodec_free_context (&av_codec_context_in);
	avformat_close_input (&av_format_context_in);

	if (video_frame_count == 1) {
		fresque_batch_tmp->initialized = FALSE;

		if ((first_frame_in->height != nb_lines) || (first_frame_in->width != second_frame_width) || \
									((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) && (first_frame_in->format != AV_PIX_FMT_YUV444P)) || \
									((hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P10LE) && (first_frame_in->format != AV_PIX_FMT_YUV444P16LE))) {

			fresque_frame = av_frame_alloc ();
			if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) fresque_frame->format = AV_PIX_FMT_YUV444P;
			else fresque_frame->format = AV_PIX_FMT_YUV444P16LE;
			fresque_frame->pict_type = AV_PICTURE_TYPE_I;
			fresque_frame->height = nb_lines;
			fresque_frame->width = second_frame_width;
			fresque_frame->colorspace = hyperdeck_colorspace;

			av_frame_get_buffer (fresque_frame, 0);

			fresque_sws_context = sws_getContext (first_frame_in->width, first_frame_in->height, first_frame_in->format, second_frame_width, nb_lines, fresque_frame->format, SWS_BILINEAR, NULL, NULL, NULL);

			sws_setColorspaceDetails (fresque_sws_context, inv_table, srcRange, hyperdeck_yuv2rgb_coefficients, 0, 0, 65536, 65536);
			sws_scale (fresque_sws_context, (const uint8_t * const*)first_frame_in->data, first_frame_in->linesize, 0, first_frame_in->height, fresque_frame->data, fresque_frame->linesize);
			sws_freeContext (fresque_sws_context);
		} else fresque_frame = first_frame_in;

		fresque_batch_tmp->fresque_frame = fresque_frame;
		fresque_batch_tmp->nb_flux = nb_flux;
		fresque_batch_tmp->first_hyperdeck_number = first_hyperdeck->number;

g_mutex_lock (&fresque_batch_mutex);
		fresque_batch_tmp->next = fresque_batches;
		fresque_batches = fresque_batch_tmp;
g_mutex_unlock (&fresque_batch_mutex);

		transcoding_frames[first_hyperdeck->number].nb_frames = (int64_t)frequency;

		for (i = 0; i < NB_OF_TRANSITIONS; i++) {
			if (transitions[i].switched_on) {
				transcoding_frames[first_hyperdeck->number].nb_frames += transition_nb_frames * 2;
				if (transition_type == 1) transcoding_frames[first_hyperdeck->number].nb_frames += (transition_nb_frames / nb_flux) * 2;

				transition_task = g_malloc (sizeof (transition_task_t));
				transition_task->background_name = transitions[i].file_name;
				memcpy (transition_task->file_name, first_drop_list->file_name_out, first_drop_list->file_name_out_len);
				transition_task->file_name_len = first_drop_list->file_name_out_len;
				transition_task->suffix = transitions[i].suffix;
				transition_task->fresque_frame = fresque_frame;
				transition_task->nb_flux = first_drop_list->nb_flux;
				transition_task->first_hyperdeck = first_hyperdeck;
				memcpy (transition_task->creation_time, first_drop_list->creation_time, 32);

				transitions[i].thread[first_hyperdeck->number] = g_thread_new (NULL, (GThreadFunc)run_transition_task, transition_task);
			}
		}

		for (j = 1; j < (int)frequency; j++) {
			for (i = 0, hyperdeck_ptr = first_hyperdeck; i < nb_flux; i++, hyperdeck_ptr++) {
				if (drop_list_array_ptr[i] != NULL) {
					frame_out[i]->pts += frame_duration[i];
					avcodec_send_frame (av_codec_context_out[i], frame_out[i]);

					avcodec_receive_packet (av_codec_context_out[i], &packet_out);
					packet_out.stream_index = 0;
					packet_out.duration = frame_duration[i];
					av_interleaved_write_frame (av_format_context_out[i], &packet_out);
				}
			}
			transcoding_frames[first_hyperdeck->number].frame_count++;
		}
	} else g_free (fresque_batch_tmp);

	for (i = 0, hyperdeck_ptr = first_hyperdeck; i < nb_flux; i++, hyperdeck_ptr++) {
		if (drop_list_array_ptr[i] != NULL) {
			av_write_trailer (av_format_context_out[i]);
			drop_list_array_ptr[i]->ffmpeg_buffer_size = avio_close_dyn_buf (av_format_context_out[i]->pb, &drop_list_array_ptr[i]->ffmpeg_buffer);

g_mutex_lock (&hyperdeck_ptr->drop_mutex);
			drop_list_array_ptr[i]->next = hyperdeck_ptr->drop_list_file;
			hyperdeck_ptr->drop_list_file = drop_list_array_ptr[i];
			if (hyperdeck_ptr->drop_thread == NULL) hyperdeck_ptr->drop_thread = g_thread_new (NULL, (GThreadFunc)drop_to_hyperdeck, hyperdeck_ptr);
g_mutex_unlock (&hyperdeck_ptr->drop_mutex);

			av_frame_unref (frame_out[i]);
			av_frame_free (&frame_out[i]);
			avcodec_free_context (&av_codec_context_out[i]);
			avformat_free_context (av_format_context_out[i]);
		}
	}

	if (first_frame_in != fresque_batch_tmp->fresque_frame) {
		av_frame_unref (first_frame_in);
		av_frame_free (&first_frame_in);
	}

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		if (transitions[i].thread[first_hyperdeck->number] != NULL) {
			g_thread_join (transitions[i].thread[first_hyperdeck->number]);
			transitions[i].thread[first_hyperdeck->number] = NULL;
		}
	}
DEBUG_S("split_and_encode_fresque END")
}

