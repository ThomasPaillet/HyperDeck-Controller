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
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <time.h>

#include "HyperDeck.h"


remuxing_frame_t remuxing_frames[NB_OF_HYPERDECKS];
transcoding_frame_t transcoding_frames[NB_OF_HYPERDECKS];


void split_and_encode_fresque (hyperdeck_t* first_hyperdeck, drop_list_t *first_drop_list);


void hyperdeck_remux_file (hyperdeck_t* hyperdeck, drop_list_t *drop_list)
{
	AVFormatContext *av_format_context_in = drop_list->av_format_context_in;
	int stream_index = drop_list->stream_index;
	AVFormatContext *av_format_context_out = NULL;
	AVStream *av_stream_in, *av_stream_out;
	AVDictionaryEntry* tag = NULL;
	char timecode[12];
	AVPacket packet;
	int frame_duration, video_frame_count = 0;
DEBUG_HYPERDECK_S("hyperdeck_remux_file")

	sprintf (timecode, "%02d:00:00:00", hyperdeck->number + 1);

	av_stream_in = av_format_context_in->streams[stream_index];

	avformat_alloc_output_context2 (&av_format_context_out, NULL, file_ext, NULL);

	av_dict_set (&av_format_context_out->metadata, "creation_time", drop_list->creation_time, 0);

	av_stream_out = avformat_new_stream (av_format_context_out, av_codec_out);
	avcodec_parameters_copy (av_stream_out->codecpar, av_stream_in->codecpar);

	av_dict_set (&av_stream_out->metadata, "language", "eng", 0);

	if ((tag = av_dict_get (av_stream_in->metadata, "encoder", NULL, AV_DICT_MATCH_CASE)) != NULL) av_dict_set (&av_stream_out->metadata, "encoder", tag->value, 0);	//printf ("%s=%s\n", tag->key, tag->value);
	else if (hyperdeck_codec == AV_CODEC_ID_PRORES) {
		if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','c','o')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 422 (Proxy)", 0);
		else if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','c','s')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 422 (LT)", 0);
		else if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','c','n')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 422", 0);
		else if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','c','h')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 422 (HQ)", 0);
		else if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','4','h')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 4444", 0);
//		else if (av_stream_out->codecpar->codec_tag == MKTAG ('a','p','4','x')) av_dict_set (&av_stream_out->metadata, "encoder", "Apple ProRes 4444 (XQ)", 0);
	}

	av_dict_set (&av_stream_out->metadata, "timecode", timecode, 0);

	av_stream_out->id = 1;
	av_stream_out->time_base = hyperdeck_time_base;
	av_stream_out->avg_frame_rate = hyperdeck_framerate;
	av_stream_out->r_frame_rate = hyperdeck_framerate;
	av_stream_out->start_time = 0;
	av_stream_out->duration = 0;
	av_stream_out->nb_frames = 0;
	av_stream_out->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;

	avio_open_dyn_buf (&av_format_context_out->pb);

	if (avformat_write_header (av_format_context_out, NULL) < 0) {
		avformat_free_context (av_format_context_out);
		g_free (drop_list->full_name);
		g_free (drop_list);
		return;
	}

	frame_duration = (av_stream_out->time_base.den / av_stream_out->avg_frame_rate.num) * av_stream_out->avg_frame_rate.den;

	packet.data = NULL;
	packet.size = 0;
	av_init_packet (&packet);

	while (av_read_frame (av_format_context_in, &packet) == 0) {
		if (packet.stream_index != stream_index) {
			av_packet_unref (&packet);
			continue;
		}

		packet.stream_index = av_stream_out->index;
		packet.pts = video_frame_count * frame_duration;
		packet.dts = packet.pts;
		packet.duration = frame_duration;
		av_interleaved_write_frame (av_format_context_out, &packet);

		video_frame_count++;
		remuxing_frames[hyperdeck->number].frame_count = video_frame_count;
	}

	avformat_close_input (&av_format_context_in);

	av_write_trailer (av_format_context_out);
	drop_list->ffmpeg_buffer_size = avio_close_dyn_buf (av_format_context_out->pb, &drop_list->ffmpeg_buffer);

g_mutex_lock (&hyperdeck->drop_mutex);
	drop_list->next = hyperdeck->drop_list_file;
	hyperdeck->drop_list_file = drop_list;
	if (hyperdeck->drop_thread == NULL) hyperdeck->drop_thread = g_thread_new (NULL, (GThreadFunc)drop_to_hyperdeck, hyperdeck);
g_mutex_unlock (&hyperdeck->drop_mutex);

	avformat_free_context (av_format_context_out);

DEBUG_HYPERDECK_S("hyperdeck_remux_file END")
}

void hyperdeck_transcode_file (hyperdeck_t* hyperdeck, drop_list_t *drop_list)
{
	AVFormatContext *av_format_context_in = drop_list->av_format_context_in;
	AVCodec *av_codec_in = drop_list->av_codec_in;
	int stream_index = drop_list->stream_index;
	AVFormatContext *av_format_context_out = NULL;
	AVCodecContext *av_codec_context_in, *av_codec_context_out;
	AVStream *av_stream_in, *av_stream_out;
	AVPacket packet_in, packet_out;
	AVFrame *frame_in, *frame_out;
	struct SwsContext *sws_context;
	const int *inv_table;
	int srcRange;
	int *table;
	int dstRange;
	int brightness;
	int contrast;
	int saturation;
	AVFilterContext *av_filter_context_in, *av_filter_context_out;
	AVFilterGraph *av_filter_graph;
	char filter_descr[256];
	int i, frame_duration, video_frame_count = 0;
	transition_task_t *transition_task;
DEBUG_HYPERDECK_S("hyperdeck_transcode_file")
	av_stream_in = av_format_context_in->streams[stream_index];
				
	av_codec_context_in = avcodec_alloc_context3 (av_codec_in);
	avcodec_parameters_to_context (av_codec_context_in, av_stream_in->codecpar);
	av_codec_context_in->time_base = av_stream_in->time_base;
	av_codec_context_in->sample_aspect_ratio = av_stream_in->sample_aspect_ratio;

g_mutex_lock (&avcodec_open2_mutex);
	avcodec_open2 (av_codec_context_in, av_codec_in, NULL);
g_mutex_unlock (&avcodec_open2_mutex);

	create_output_context (hyperdeck, &av_format_context_out, &av_codec_context_out, &av_stream_out, drop_list->creation_time);

	if (avformat_write_header (av_format_context_out, NULL) < 0) {
		avcodec_free_context (&av_codec_context_out);
		avformat_free_context (av_format_context_out);
		avcodec_free_context (&av_codec_context_in);
		g_free (drop_list->full_name);
		g_free (drop_list);
		return;
	}

	frame_duration = (av_stream_out->time_base.den / av_codec_context_out->framerate.num) * av_codec_context_out->framerate.den;

	packet_in.data = NULL;
	packet_in.size = 0;
	av_init_packet (&packet_in);

	packet_out.data = NULL;
	packet_out.size = 0;
	av_init_packet (&packet_out);

	frame_in = av_frame_alloc ();

	if (drop_list->pix_fmt_ok && drop_list->field_order_ok && drop_list->color_range_ok && drop_list->color_primaries_ok && drop_list->scale_ok) {
DEBUG_HYPERDECK_S ("Only codec")
		while (av_read_frame (av_format_context_in, &packet_in) == 0) {
			if (packet_in.stream_index != stream_index) {
				av_packet_unref (&packet_in);
				continue;
			}

			if (avcodec_send_packet (av_codec_context_in, &packet_in) < 0) {
				av_packet_unref (&packet_in);
				continue;
			}

			av_frame_unref (frame_in);
			if (avcodec_receive_frame (av_codec_context_in, frame_in) < 0) {
				av_packet_unref (&packet_in);
				continue;
			}

			frame_in->pts = video_frame_count * frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_in);

			avcodec_receive_packet (av_codec_context_out, &packet_out);
			packet_out.stream_index = av_stream_out->index;
			packet_out.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet_out);

			av_packet_unref (&packet_in);
			video_frame_count++;
			transcoding_frames[hyperdeck->number].frame_count = video_frame_count;
		}

		if (video_frame_count == 1) {
			transcoding_frames[hyperdeck->number].nb_frames = (int64_t)frequency;

/*			for (i = 0; i < NB_OF_TRANSITIONS; i++) {
				if (transitions[i].switched_on) {
					transcoding_frames[hyperdeck->number].nb_frames += transition_nb_frames * 2;
					if (transition_type == 1) transcoding_frames[hyperdeck->number].nb_frames += transition_nb_frames * 2;

					transition_task = g_malloc (sizeof (transition_task_t));
					transition_task->background_name = transitions[i].file_name;
					memcpy (transition_task->file_name, drop_list->file_name_out, drop_list->file_name_out_len);
					transition_task->file_name_len = drop_list->file_name_out_len;
					transition_task->suffix = transitions[i].suffix;
					transition_task->first_fresque_frame = frame_in;
					transition_task->nb_flux = 1;
					transition_task->first_hyperdeck = hyperdeck;
					memcpy (transition_task->creation_time, drop_list->creation_time, 32);

					transitions[i].thread[hyperdeck->number] = g_thread_new (NULL, (GThreadFunc)run_transition_task, transition_task);
				}
			}*/

			for (i = 1; i < (int)frequency; i++) {
				frame_in->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_in);

				avcodec_receive_packet (av_codec_context_out, &packet_out);
				packet_out.stream_index = av_stream_out->index;
				packet_out.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet_out);

				transcoding_frames[hyperdeck->number].frame_count++;
			}
		}

		av_frame_unref (frame_in);
	} else {
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

		if ((progressif && drop_list->field_order_ok /*&& drop_list->color_primaries_ok*/) || (drop_list->scale_ok && drop_list->field_order_ok && !drop_list->pix_fmt_ok)) {
DEBUG_HYPERDECK_S ("Progressif ou à la bonne taille et à la bonne priorité de trame")
			av_frame_get_buffer (frame_out, 0);

			sws_context = sws_getContext (av_codec_context_in->width, av_codec_context_in->height, av_codec_context_in->pix_fmt, hyperdeck_width, nb_lines, hyperdeck_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);

		if (av_codec_context_in->colorspace == AVCOL_SPC_BT709) inv_table = sws_getCoefficients (SWS_CS_ITU709);
		else if (av_codec_context_in->colorspace == AVCOL_SPC_BT470BG) inv_table = sws_getCoefficients (SWS_CS_ITU601);
		else if (av_codec_context_in->colorspace == AVCOL_SPC_SMPTE170M) inv_table = sws_getCoefficients (SWS_CS_SMPTE170M);
		else if (av_codec_context_in->colorspace == AVCOL_SPC_SMPTE240M) inv_table = sws_getCoefficients (SWS_CS_SMPTE240M);
		else if (av_codec_context_in->colorspace == AVCOL_SPC_FCC) inv_table = sws_getCoefficients (SWS_CS_FCC);
		else if ((av_codec_context_in->colorspace == AVCOL_SPC_BT2020_NCL) || (av_codec_context_in->colorspace == AVCOL_SPC_BT2020_CL)) inv_table = sws_getCoefficients (SWS_CS_BT2020);
		else inv_table = sws_getCoefficients (SWS_CS_DEFAULT);

		if (av_codec_context_in->color_range == AVCOL_RANGE_MPEG) srcRange = 0;
		else srcRange = 1;

printf ("sws_setColorspaceDetails = %d\n\n", sws_setColorspaceDetails (sws_context, inv_table, srcRange, hyperdeck_yuv2rgb_coefficients, 0, 0, 65536, 65536));

sws_getColorspaceDetails (sws_context, &inv_table, &srcRange, &table, &dstRange, &brightness, &contrast, &saturation);
printf ("inv_table: %d, %d, %d, %d\n", inv_table[0], inv_table[1], inv_table[2], inv_table[3]);
printf ("table: %d, %d, %d, %d\n", table[0], table[1], table[2], table[3]);
printf ("srcRange: %d, dstRange: %d, brightness: %d, contrast: %d, saturation: %d\n", srcRange, dstRange, brightness, contrast, saturation);

			while (av_read_frame (av_format_context_in, &packet_in) == 0) {
				if (packet_in.stream_index != stream_index) {
					av_packet_unref (&packet_in);
					continue;
				}

				if (avcodec_send_packet (av_codec_context_in, &packet_in) < 0) {
					av_packet_unref (&packet_in);
					continue;
				}

				av_frame_unref (frame_in);
				if (avcodec_receive_frame (av_codec_context_in, frame_in) < 0) {
					av_packet_unref (&packet_in);
					continue;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_in->data, frame_in->linesize, 0, av_codec_context_in->height, frame_out->data, frame_out->linesize);

				frame_out->pts = video_frame_count * frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet_out);
				packet_out.stream_index = av_stream_out->index;
				packet_out.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet_out);

				av_packet_unref (&packet_in);
				video_frame_count++;
				transcoding_frames[hyperdeck->number].frame_count = video_frame_count;
			}

			sws_freeContext (sws_context);
		} else {
DEBUG_HYPERDECK_S ("Filter_graph")
			if (!drop_list->field_order_ok && progressif) {
				if (drop_list->scale_ok) {
					if (drop_list->pix_fmt_ok) snprintf (filter_descr, 256, "kerndeint=order=1");
					else snprintf (filter_descr, 256, "kerndeint=order=1,format=pix_fmts=%s", av_get_pix_fmt_name (hyperdeck_pix_fmt));
				} else {
					if (drop_list->pix_fmt_ok) snprintf (filter_descr, 256, "kerndeint=order=1,scale=w=%d:h=%d:interl=0:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable", hyperdeck_width, nb_lines);
					else snprintf (filter_descr, 256, "kerndeint=order=1,scale=w=%d:h=%d:interl=0:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable,format=pix_fmts=%s", hyperdeck_width, nb_lines, av_get_pix_fmt_name (hyperdeck_pix_fmt));
				}
			} else if (!drop_list->field_order_ok && !progressif) {
				if (drop_list->scale_ok) {
					if (drop_list->pix_fmt_ok) snprintf (filter_descr, 256, "fieldorder=tff");
					else snprintf (filter_descr, 256, "fieldorder=tff,format=pix_fmts=%s", av_get_pix_fmt_name (hyperdeck_pix_fmt));
				} else {
					if (drop_list->pix_fmt_ok) snprintf (filter_descr, 256, "fieldorder=tff,scale=w=%d:h=%d:interl=1:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable", hyperdeck_width, nb_lines);
					else snprintf (filter_descr, 256, "fieldorder=tff,scale=w=%d:h=%d:interl=1:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable,format=pix_fmts=%s", hyperdeck_width, nb_lines, av_get_pix_fmt_name (hyperdeck_pix_fmt));
				}
			} else if (drop_list->pix_fmt_ok) snprintf (filter_descr, 256, "scale=w=%d:h=%d:interl=1:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable", hyperdeck_width, nb_lines);
				else snprintf (filter_descr, 256, "scale=w=%d:h=%d:interl=1:in_color_matrix=auto:out_color_matrix=auto:in_range=auto:out_range=tv:force_original_aspect_ratio=disable,format=pix_fmts=%s", hyperdeck_width, nb_lines, av_get_pix_fmt_name (hyperdeck_pix_fmt));

DEBUG_HYPERDECK_S (filter_descr)
			if ((av_filter_graph = create_filter_graph (&av_filter_context_in, &av_filter_context_out, av_codec_context_in, filter_descr)) == NULL) {
				av_frame_free (&frame_out);
				av_frame_free (&frame_in);
				avcodec_free_context (&av_codec_context_out);
				avformat_free_context (av_format_context_out);
				avcodec_free_context (&av_codec_context_in);
				g_free (drop_list->full_name);
				g_free (drop_list);
				return;
			}
DEBUG_HYPERDECK_S (avfilter_graph_dump (av_filter_graph, NULL))

			while (av_read_frame (av_format_context_in, &packet_in) == 0) {
				if (packet_in.stream_index != stream_index) {
					av_packet_unref (&packet_in);
					continue;
				}

				if (avcodec_send_packet (av_codec_context_in, &packet_in) < 0) {
					av_packet_unref (&packet_in);
					continue;
				}

				av_frame_unref (frame_in);
				if (avcodec_receive_frame (av_codec_context_in, frame_in) < 0) {
					av_packet_unref (&packet_in);
					continue;
				}

				if (av_buffersrc_add_frame (av_filter_context_in, frame_in) >= 0) {
					av_frame_unref (frame_out);
					av_buffersink_get_frame (av_filter_context_out, frame_out);
					frame_out->pts = video_frame_count * frame_duration;
					avcodec_send_frame (av_codec_context_out, frame_out);

					avcodec_receive_packet (av_codec_context_out, &packet_out);
					packet_out.stream_index = av_stream_out->index;
					packet_out.duration = frame_duration;
					av_interleaved_write_frame (av_format_context_out, &packet_out);
				}

				av_packet_unref (&packet_in);
				video_frame_count++;
				transcoding_frames[hyperdeck->number].frame_count = video_frame_count;
			}
		}

		if (video_frame_count == 1) {
			transcoding_frames[hyperdeck->number].nb_frames = (int64_t)frequency;

/*			for (i = 0; i < NB_OF_TRANSITIONS; i++) {
				if (transitions[i].switched_on) {
					transcoding_frames[hyperdeck->number].nb_frames += transition_nb_frames * 2;
					if (transition_type == 1) transcoding_frames[hyperdeck->number].nb_frames += transition_nb_frames * 2;

					transition_task = g_malloc (sizeof (transition_task_t));
					transition_task->background_name = transitions[i].file_name;
					memcpy (transition_task->file_name, drop_list->file_name_out, drop_list->file_name_out_len);
					transition_task->file_name_len = drop_list->file_name_out_len;
					transition_task->suffix = transitions[i].suffix;
					transition_task->first_fresque_frame = frame_in;
					transition_task->nb_flux = 1;
					transition_task->first_hyperdeck = hyperdeck;
					memcpy (transition_task->creation_time, drop_list->creation_time, 32);

					transitions[i].thread[hyperdeck->number] = g_thread_new (NULL, (GThreadFunc)run_transition_task, transition_task);
				}
			}*/

			for (i = 1; i < (int)frequency; i++) {
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet_out);
				packet_out.stream_index = av_stream_out->index;
				packet_out.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet_out);

				transcoding_frames[hyperdeck->number].frame_count++;
			}
		}

		av_frame_unref (frame_out);
		av_frame_free (&frame_out);
	}

	avformat_close_input (&av_format_context_in);

	av_write_trailer (av_format_context_out);
	drop_list->ffmpeg_buffer_size = avio_close_dyn_buf (av_format_context_out->pb, &drop_list->ffmpeg_buffer);

g_mutex_lock (&hyperdeck->drop_mutex);
	drop_list->next = hyperdeck->drop_list_file;
	hyperdeck->drop_list_file = drop_list;
	if (hyperdeck->drop_thread == NULL) hyperdeck->drop_thread = g_thread_new (NULL, (GThreadFunc)drop_to_hyperdeck, hyperdeck);
g_mutex_unlock (&hyperdeck->drop_mutex);

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		if (transitions[i].thread[hyperdeck->number] != NULL) {
			g_thread_join (transitions[i].thread[hyperdeck->number]);
			transitions[i].thread[hyperdeck->number] = NULL;
		}
	}

	av_frame_unref (frame_in);
	av_frame_free (&frame_in);
	avcodec_free_context (&av_codec_context_out);
	avformat_free_context (av_format_context_out);
	avcodec_free_context (&av_codec_context_in);
DEBUG_HYPERDECK_S("hyperdeck_transcode_file END")
}

int check_need_for_transcoding (hyperdeck_t* hyperdeck, drop_list_t *drop_list)
{
	AVFormatContext *av_format_context_in = NULL;
	AVCodec *av_codec_in = NULL;
	AVStream *av_stream_in;
	AVRational r_frame_rate;
	float frame_rate;
	AVCodecParameters *av_stream_in_codec_parameters;
DEBUG_HYPERDECK_S ("check_need_for_transcoding")

	if (avformat_open_input (&av_format_context_in, drop_list->full_name, NULL, NULL) < 0) return TRUE;

	if (avformat_find_stream_info (av_format_context_in, NULL) < 0) {
		avformat_close_input (&av_format_context_in);
		return 0;
	}

	drop_list->stream_index = av_find_best_stream (av_format_context_in, AVMEDIA_TYPE_VIDEO, -1, -1, &av_codec_in, 0);
	if (drop_list->stream_index < 0) {
		avformat_close_input (&av_format_context_in);
		return 0;
	}

	drop_list->av_format_context_in = av_format_context_in;
	drop_list->av_codec_in = av_codec_in;

	av_stream_in = av_format_context_in->streams[drop_list->stream_index];

	drop_list->nb_frames = av_stream_in->nb_frames;

	r_frame_rate = av_stream_in->r_frame_rate;
	frame_rate = ((int)(((float)r_frame_rate.num / (float)r_frame_rate.den) * 1000.0)) / 1000.0;
DEBUG_S("frame_rate")
DEBUG_F(frame_rate)
	av_stream_in_codec_parameters = av_stream_in->codecpar;

	drop_list->codec_ok = FALSE;
	drop_list->pix_fmt_ok = FALSE;
	drop_list->field_order_ok = FALSE;
	drop_list->color_range_ok = TRUE;
	drop_list->color_primaries_ok = TRUE;
	drop_list->scale_ok = FALSE;
	drop_list->nb_flux = 1;

	if (av_codec_in->id == hyperdeck_codec) {
		if ((hyperdeck_codec == AV_CODEC_ID_PRORES) && ( \
			(av_stream_in_codec_parameters->codec_tag == MKTAG ('a','p','c','o')) || \
			(av_stream_in_codec_parameters->codec_tag == MKTAG ('a','p','c','s')) || \
			(av_stream_in_codec_parameters->codec_tag == MKTAG ('a','p','c','n')) || \
			(av_stream_in_codec_parameters->codec_tag == MKTAG ('a','p','c','h')) || \
			(av_stream_in_codec_parameters->codec_tag == MKTAG ('a','p','4','h')))) drop_list->codec_ok = TRUE;
		if (hyperdeck_codec == AV_CODEC_ID_DNXHD) drop_list->codec_ok = TRUE;
	}

	if (av_stream_in_codec_parameters->format == hyperdeck_pix_fmt) drop_list->pix_fmt_ok = TRUE;

	if (((progressif) && ((av_stream_in_codec_parameters->field_order == AV_FIELD_PROGRESSIVE) || (av_stream_in_codec_parameters->field_order == AV_FIELD_UNKNOWN))) || \
		((!progressif) && ((av_stream_in_codec_parameters->field_order != AV_FIELD_BB) && \
						(av_stream_in_codec_parameters->field_order != AV_FIELD_BT)))) drop_list->field_order_ok = TRUE;

	if (av_stream_in_codec_parameters->color_range == AVCOL_RANGE_JPEG) drop_list->color_range_ok = FALSE;

	if ((av_stream_in_codec_parameters->color_primaries != AVCOL_PRI_UNSPECIFIED) && (av_stream_in_codec_parameters->color_primaries != hyperdeck_color_primaries)) drop_list->color_primaries_ok = FALSE;

	if (av_stream_in_codec_parameters->height == 480) {
		drop_list->format = "SD NTSC";

		if (av_stream_in_codec_parameters->width == 720) {
			if (nb_lines == 480) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 720) == 0) {
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 720;
			} else if ((av_stream_in_codec_parameters->width % 872) == 0) {		//square pixels
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 872;
			}
		}
	} else if (av_stream_in_codec_parameters->height == 576) {
		drop_list->format = "SD";

		if (av_stream_in_codec_parameters->width == 720) {
			if (nb_lines == 576) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 720) == 0) {
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 720;
			} else if ((av_stream_in_codec_parameters->width % 1024) == 0) {		//square pixels
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1024;
			} else if ((av_stream_in_codec_parameters->width % 1050) == 0) {		//square pixels (After Effects)
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1050;
			}
		}
	} else if (av_stream_in_codec_parameters->height == 720) {
		drop_list->format = "HD ready";

		if (av_stream_in_codec_parameters->width == 1280) {
			if (nb_lines == 720) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 1280) == 0) {	//square pixels
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1280;
			} else if ((av_stream_in_codec_parameters->width % 960) == 0) {
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 960;
			}
		}
	} else if (av_stream_in_codec_parameters->height == 1080) {
		drop_list->format = "HD";

		if (av_stream_in_codec_parameters->width == 1920) {
			if (nb_lines == 1080) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 1920) == 0) {
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1920;
			} else if ((av_stream_in_codec_parameters->width % 1440) == 0) {		//HDV PAL
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1440;
			} else if ((av_stream_in_codec_parameters->width % 1280) == 0) {		//HDV NTCS
				drop_list->nb_flux = av_stream_in_codec_parameters->width / 1280;
			}
		}
	} else if (av_stream_in_codec_parameters->height == 2160) {
		drop_list->format = "UHD 4K";

		if (av_stream_in_codec_parameters->width == 3840) {
			if (nb_lines == 2160) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 3840) == 0) drop_list->nb_flux = av_stream_in_codec_parameters->width / 3840;
		}
	} else if (av_stream_in_codec_parameters->height == 4320) {
		drop_list->format = "UHD 8K";

		if (av_stream_in_codec_parameters->width == 7680) {
			if (nb_lines == 4320) {
				drop_list->scale_ok = TRUE;

				if (drop_list->codec_ok) {
//					if (drop_list->pix_fmt_ok) {
						if (drop_list->field_order_ok) {
							if (drop_list->color_range_ok) {
								if ((memcmp (av_format_context_in->iformat->name, file_ext, 3) == 0) && (frame_rate == frequency)) {
									avformat_close_input (&av_format_context_in);
									return 1;
								} else return 2;
							}
						}
//					}
				}
			}
		} else {
			if ((av_stream_in_codec_parameters->width % 7680) == 0) drop_list->nb_flux = av_stream_in_codec_parameters->width / 7680;
		}
	}

	return 3;
}

gpointer hyperdeck_remux (hyperdeck_t* hyperdeck)
{
	drop_list_t *drop_list;
	int index = hyperdeck->number;
	time_t start_time, end_time;
	struct tm *tm;
	g_source_label_t *source_label;
DEBUG_HYPERDECK_S ("hyperdeck_remux")
	if (remuxing_frames[index].g_source_id != 0) g_source_remove (remuxing_frames[index].g_source_id);
	remuxing_frames[index].g_source_id = g_timeout_add (500, (GSourceFunc)g_source_update_remuxing_progress_bar, &remuxing_frames[index]);

g_mutex_lock (&hyperdeck->remuxing_mutex);
	while (hyperdeck->remuxing_list_file != NULL) {
		drop_list = hyperdeck->remuxing_list_file;
		hyperdeck->remuxing_list_file = drop_list->next;
g_mutex_unlock (&hyperdeck->remuxing_mutex);

		if (drop_list->nb_frames != 0) g_idle_add ((GSourceFunc)g_source_init_remuxing_progress_bar, &remuxing_frames[index]);
		remuxing_frames[index].nb_frames = drop_list->nb_frames;
		remuxing_frames[index].frame_count = 0;

		complete_file_name_out (drop_list);

		source_label = g_malloc (sizeof (g_source_label_t));
		sprintf (source_label->text, "%s --> %s (%s)", drop_list->file_name_in, drop_list->file_name_out, video_format_label);
		source_label->label = remuxing_frames[index].label;
		g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

		g_idle_add ((GSourceFunc)g_source_show_widget, remuxing_frames[index].frame);

		start_time = time (NULL);
		tm = localtime (&start_time);
		sprintf (drop_list->creation_time, "%4d-%02d-%02dT%02d:%02d:%02d.000000Z", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

		hyperdeck_remux_file (hyperdeck, drop_list);

		g_idle_add ((GSourceFunc)g_source_end_remuxing_progress_bar, &remuxing_frames[index]);

g_mutex_lock (&hyperdeck->remuxing_mutex);
	}

	g_source_remove (remuxing_frames[index].g_source_id);

	end_time = time (NULL);
	end_time -= start_time;
	end_time -= 4;

	if (end_time < 0)
		remuxing_frames[index].g_source_id = g_timeout_add (-end_time * 1000, (GSourceFunc)g_source_hide_remuxing_progress_bar, GINT_TO_POINTER (index));
	else {
		remuxing_frames[index].g_source_id = 0;
		g_idle_add ((GSourceFunc)g_source_hide_widget, remuxing_frames[index].frame);
	}

	g_idle_add ((GSourceFunc)g_source_consume_thread, hyperdeck->remuxing_thread);
	hyperdeck->remuxing_thread = NULL;
g_mutex_unlock (&hyperdeck->remuxing_mutex);

DEBUG_HYPERDECK_S ("hyperdeck_remux END")
	return NULL;
}

gpointer hyperdeck_transcode (hyperdeck_t* hyperdeck)
{
	drop_list_t *drop_list;
	int i, index = hyperdeck->number;
	time_t start_time, end_time;
	struct tm *tm;
	g_source_label_t *source_label;
DEBUG_HYPERDECK_S ("hyperdeck_transcode")
	if (transcoding_frames[index].g_source_id != 0) {
		g_source_remove (transcoding_frames[index].g_source_id);
		transcoding_frames[index].g_source_id = 0;
	}

g_mutex_lock (&hyperdeck->transcoding_mutex);
	while (hyperdeck->transcoding_list_file != NULL) {
		drop_list = hyperdeck->transcoding_list_file;
		hyperdeck->transcoding_list_file = drop_list->next;
g_mutex_unlock (&hyperdeck->transcoding_mutex);

		if (drop_list->nb_frames == 0) transcoding_frames[index].nb_frames = (int64_t)frequency;
		else transcoding_frames[index].nb_frames = drop_list->nb_frames;
		transcoding_frames[index].frame_count = 0;
		g_idle_add ((GSourceFunc)g_source_init_transcoding_progress_bar, &transcoding_frames[index]);

		source_label = g_malloc (sizeof (g_source_label_t));
		sprintf (source_label->text, "%s (%s)", drop_list->file_name_in, drop_list->format);
		source_label->label = transcoding_frames[index].src_file_name_label;
		g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

		start_time = time (NULL);
		tm = localtime (&start_time);
		sprintf (drop_list->creation_time, "%4d-%02d-%02dT%02d:%02d:%02d.000000Z", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

		if (drop_list->nb_flux == 1) {
			complete_file_name_out (drop_list);

			source_label = g_malloc (sizeof (g_source_label_t));
			sprintf (source_label->text, "%s (%s)", drop_list->file_name_out, video_format_label);
			source_label->label = transcoding_frames[index].dst_file_name_label[0];
			g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

			for (i = 1; i < NB_OF_HYPERDECKS; i++) g_idle_add ((GSourceFunc)g_source_hide_widget, transcoding_frames[index].dst_file_name_label[i]);

			g_idle_add ((GSourceFunc)g_source_show_widget, transcoding_frames[index].frame);

			hyperdeck_transcode_file (hyperdeck, drop_list);
		} else split_and_encode_fresque (hyperdeck, drop_list);

		transcoding_frames[index].frame_count = transcoding_frames[index].nb_frames;

g_mutex_lock (&hyperdeck->transcoding_mutex);
	}

	end_time = time (NULL);
	end_time -= start_time;
	end_time -= 4;

	if (end_time < 0)
		transcoding_frames[index].g_source_id = g_timeout_add (-end_time * 1000, g_source_hide_transcoding_progress_bar, GINT_TO_POINTER (index));
	else g_idle_add (g_source_hide_transcoding_progress_bar, GINT_TO_POINTER (index));

	g_idle_add ((GSourceFunc)g_source_consume_thread, hyperdeck->transcoding_thread);
	hyperdeck->transcoding_thread = NULL;
g_mutex_unlock (&hyperdeck->transcoding_mutex);

DEBUG_HYPERDECK_S ("hyperdeck_transcode END")
	return NULL;
}

void create_transcoding_frames (GtkBox *box)
{
	GtkWidget *box1, *box2, *box3, *scrolled_window;
	int i, j;
	char label[32];

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		sprintf (label, "HyperDeck n°%d: ré-encapsulage", hyperdecks[i].number + 1);
		remuxing_frames[i].frame = gtk_frame_new (label);
		gtk_frame_set_label_align (GTK_FRAME (remuxing_frames[i].frame), 0.03, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (remuxing_frames[i].frame), 5);
			box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			gtk_widget_set_margin_start (box1, 5);
			gtk_widget_set_margin_end (box1, 5);
			gtk_widget_set_margin_bottom (box1, 5);
				scrolled_window = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
					remuxing_frames[i].label = gtk_label_new (NULL);
					gtk_widget_set_margin_bottom (remuxing_frames[i].label, 5);
					gtk_container_add (GTK_CONTAINER (scrolled_window), remuxing_frames[i].label);
				gtk_box_pack_start (GTK_BOX (box1), scrolled_window, FALSE, FALSE, 0);

				remuxing_frames[i].progress_bar = gtk_progress_bar_new ();
				gtk_box_pack_start (GTK_BOX (box1), remuxing_frames[i].progress_bar, FALSE, FALSE, 0);
			gtk_container_add (GTK_CONTAINER (remuxing_frames[i].frame), box1);
		gtk_box_pack_start (box, remuxing_frames[i].frame, FALSE, FALSE, 0);

		sprintf (label, "HyperDeck n°%d: encodage", hyperdecks[i].number + 1);
		transcoding_frames[i].frame = gtk_frame_new (label);
		gtk_frame_set_label_align (GTK_FRAME (transcoding_frames[i].frame), 0.03, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (transcoding_frames[i].frame), 5);
			box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			gtk_widget_set_margin_start (box1, 5);
			gtk_widget_set_margin_end (box1, 5);
			gtk_widget_set_margin_bottom (box1, 5);
				scrolled_window = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
					box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					gtk_widget_set_margin_bottom (box2, 5);
						transcoding_frames[i].src_file_name_label = gtk_label_new (NULL);
						gtk_box_pack_start (GTK_BOX (box2), transcoding_frames[i].src_file_name_label, FALSE, FALSE, 0);

						gtk_box_pack_start (GTK_BOX (box2), gtk_label_new (" --> "), FALSE, FALSE, 0);

						box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
						for (j = 0; j < NB_OF_HYPERDECKS; j++) {
							transcoding_frames[i].dst_file_name_label[j] = gtk_label_new (NULL);
							gtk_box_pack_start (GTK_BOX (box3), transcoding_frames[i].dst_file_name_label[j], FALSE, FALSE, 0);
						}
						gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
					gtk_container_add (GTK_CONTAINER (scrolled_window), box2);
				gtk_box_pack_start (GTK_BOX (box1), scrolled_window, FALSE, FALSE, 0);

				transcoding_frames[i].progress_bar = gtk_progress_bar_new ();
				gtk_box_pack_start (GTK_BOX (box1), transcoding_frames[i].progress_bar, FALSE, FALSE, 0);
			gtk_container_add (GTK_CONTAINER (transcoding_frames[i].frame), box1);
		gtk_box_pack_start (box, transcoding_frames[i].frame, FALSE, FALSE, 0);

		remuxing_frames[i].g_source_id = 0;
		transcoding_frames[i].g_source_id = 0;
	}
}

