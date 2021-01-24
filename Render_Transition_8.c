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

#include <libswscale/swscale.h>

#include "HyperDeck.h"


void stripe_color_RGB_to_YUV_8 (void)
{
	gdouble val;
	int DR, DG, DB;

	val = transition_stripe_color.red * 216 + 16;
	DR = (int)val;
	if (val - DR >= 0.5) DR++;

	val = transition_stripe_color.green * 216 + 16;
	DG = (int)val;
	if (val - DG >= 0.5) DG++;

	val = transition_stripe_color.blue * 216 + 16;
	DB = (int)val;
	if (val - DB >= 0.5) DB++;

	val = 0.2126 * DR + 0.7152 * DG + 0.0722 * DB;
	stripe_color_Y_8 = (uint8_t)val;
	if (val - stripe_color_Y_8 >= 0.5) stripe_color_Y_8++;

	val = (-(0.2126 / 1.8556) * DR - (0.7152 / 1.8556) * DG + (0.9278 / 1.8556) * DB) * (224 / 219) + 128;
	stripe_color_U_8 = (uint8_t)val;
	if (val - stripe_color_U_8 >= 0.5) stripe_color_U_8++;

	val = ((0.7874 / 1.5748) * DR - (0.7152 / 1.5748) * DG - (0.0722 / 1.5748) * DB) * (224 / 219) + 128;
	stripe_color_V_8 = (uint8_t)val;
	if (val - stripe_color_V_8 >= 0.5) stripe_color_V_8++;
}

void render_transition_8 (hyperdeck_t* first_hyperdeck, hyperdeck_t* hyperdeck, drop_list_t *drop_list, AVFrame *background_frame, AVFrame *fresque_frame, int nb_flux, float *last_x, float sin_minus_7, float stride, float step, struct SwsContext *sws_context, AVFrame *frame_tmp, AVFrame *frame_out, char *creation_time, gboolean reverse)
{
	AVFormatContext *av_format_context_out = NULL;
	AVCodecContext *av_codec_context_out;
	AVStream *av_stream_out;
	AVPacket packet;
	gboolean transition_inv;
	int x, y, frame_duration, j;
	int background_frame_linesize, fresque_frame_linesize, frame_tmp_linesize;
	int x_background, x_fresque, x_frame_tmp, n_bytes, frame_nb, fade;
	float *last_x_2, x_float, x_float_2, dist;

	create_output_context (hyperdeck, &av_format_context_out, &av_codec_context_out, &av_stream_out, creation_time);

	if (avformat_write_header (av_format_context_out, NULL) < 0) {
DEBUG_HYPERDECK_S("avformat_write_header NOOK")
		avcodec_free_context (&av_codec_context_out);
		avformat_free_context (av_format_context_out);
		g_free (drop_list);
		return;
	}
DEBUG_HYPERDECK_S("avformat_write_header OK")

	packet.data = NULL;
	packet.size = 0;
	av_init_packet (&packet);

	frame_duration = (av_stream_out->time_base.den / av_codec_context_out->framerate.num) * av_codec_context_out->framerate.den;

	background_frame_linesize = background_frame->linesize[0];
	fresque_frame_linesize = fresque_frame->linesize[0];
	frame_tmp_linesize = frame_tmp->linesize[0];

	for (y = 0; y < nb_lines; y++) {
		x_frame_tmp = y * frame_tmp_linesize;
		
		if (reverse) x_background = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * background_frame_linesize;
		else x_background = hyperdeck->number * hyperdeck_width + y * background_frame_linesize;

		memcpy (frame_tmp->data[0] + x_frame_tmp, background_frame->data[0] + x_background, frame_tmp_linesize);
		memcpy (frame_tmp->data[1] + x_frame_tmp, background_frame->data[1] + x_background, frame_tmp_linesize);
		memcpy (frame_tmp->data[2] + x_frame_tmp, background_frame->data[2] + x_background, frame_tmp_linesize);
	}

	sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
	frame_out->pts = 0;

	avcodec_send_frame (av_codec_context_out, frame_out);

	avcodec_receive_packet (av_codec_context_out, &packet);
	packet.stream_index = av_stream_out->index;
	packet.duration = frame_duration;
	av_interleaved_write_frame (av_format_context_out, &packet);

	if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;

	if (reverse && transition_return_inv) {
		if (transition_direction == 0) transition_inv = TRUE;
		else transition_inv = FALSE;
	} else {
		if (transition_direction == 0) transition_inv = FALSE;
		else transition_inv = TRUE;
	}

	if (transition_type == 0) {
		if (transition_inv) {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) last_x[y] = sin_minus_7 * y - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y - (stride / 4) - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					y++;
					last_x[y] = sin_minus_7 * y + (stride / 4) - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;

					x_frame_tmp = y * frame_tmp_linesize;
					n_bytes = (int)last_x[y];

					if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + (y * fresque_frame_linesize);
					else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (y * fresque_frame_linesize);

					if (n_bytes == 1) {
						*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
						*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
						*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
					} else if (n_bytes >= 2) {
						memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
					}
				}
			}

			for (frame_nb = transition_nb_frames - 1; frame_nb > 1; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] + stride;

					if (x_float >= 0) {
						if (x_float < hyperdeck_width) {
							x = (int)x_float;
							dist = x_float - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) {
								x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
							} else {
								x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
							}

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;

							x--;
						} else x = hyperdeck_width - 1;

						if (x >= 0) {
							if ((int)last_x[y] <= 0) {
								x_frame_tmp = y * frame_tmp_linesize;
								n_bytes = x + 1;

								if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + y * fresque_frame_linesize;
								else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * fresque_frame_linesize;
							} else {
								x_frame_tmp = (int)last_x[y] + y * frame_tmp_linesize;
								n_bytes = x - (int)last_x[y] + 1;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)last_x[y]) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)last_x[y]) + (y * fresque_frame_linesize);
							}

							if (n_bytes == 1) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							} else if (n_bytes >= 2) {
								memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
							}
						}
					}
					last_x[y] = x_float;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}

			for (y = 0; y < nb_lines; y++) {
				x = hyperdeck_width - 1;

				if ((int)last_x[y] <= 0) {
					x_frame_tmp = y * frame_tmp_linesize;
					n_bytes = x + 1;

					if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + y * fresque_frame_linesize;
					else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * fresque_frame_linesize;
				} else {
					x_frame_tmp = (int)last_x[y] + y * frame_tmp_linesize;
					n_bytes = x - (int)last_x[y] + 1;

					if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)last_x[y]) + (y * fresque_frame_linesize);
					else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)last_x[y]) + (y * fresque_frame_linesize);
				}

				if (n_bytes == 1) {
					*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
					*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
					*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
				} else if (n_bytes >= 2) {
					memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
					memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
					memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
				}
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts += frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
		} else {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) + (stride / 4) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					y++;
					last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) - (stride / 4) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;

					x = last_x[y];
					n_bytes = hyperdeck_width - x;
					x_frame_tmp = x + y * frame_tmp_linesize;

					if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
					else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

					if (n_bytes == 1) {
						*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
						*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
						*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
					} else if (n_bytes >= 2) {
						memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
					}
				}
			}
			for (frame_nb = transition_nb_frames - 1; frame_nb > 1; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] - stride;

					if (x_float < hyperdeck_width) {
						if (x_float >= 0) {
							x = (int)x_float;
							dist = x_float - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) {
								x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
							} else {
								x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
							}

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);

							x++;
							x_frame_tmp++;
							x_fresque++;
						} else {
							x = 0;
							x_frame_tmp = y * frame_tmp_linesize;

							if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width) + (y * fresque_frame_linesize);
							else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width) + (y * fresque_frame_linesize);
						}

						if (x < hyperdeck_width) {
							if ((last_x[y] - hyperdeck_width) > 0) n_bytes = hyperdeck_width - x;
							else n_bytes = last_x[y] - x + 1;

							if (n_bytes == 1) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							} else if (n_bytes >= 2) {
								memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
							}
						}
					}
					last_x[y] = x_float;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}

			for (y = 0; y < nb_lines; y++) {
				x_frame_tmp = y * frame_tmp_linesize;

				if ((last_x[y] - hyperdeck_width) > 0) n_bytes = hyperdeck_width;
				else n_bytes = last_x[y] + 1;

				if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + (y * fresque_frame_linesize);
				else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (y * fresque_frame_linesize);

				if (n_bytes == 1) {
					*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
					*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
					*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
				} else if (n_bytes >= 2) {
					memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
					memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
					memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
				}
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts += frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
		}
	} if (transition_type == 1) {
		last_x_2 = g_malloc (nb_lines * sizeof (float));
		fade = (transition_nb_frames / nb_flux) * transition_stripe_width / 100;

		if (transition_inv) {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] - hyperdeck_width * transition_stripe_width / 100;
				}
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y - (stride / 4) - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] - hyperdeck_width * transition_stripe_width / 100;
					y++;
					last_x[y] = sin_minus_7 * y + (stride / 4) - 1 + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] - hyperdeck_width * transition_stripe_width / 100;

					x = (int)last_x[y];
					x_frame_tmp = x + y * frame_tmp_linesize;

					while (x >= 0) {
						*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8;
						*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8;
						*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8;

						x--;
						x_frame_tmp--;
					}
				}
			}

			for (frame_nb = transition_nb_frames - 2 + fade; frame_nb >= 0; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] + stride;

					if (x_float >= 0) {
						if (x_float < hyperdeck_width) {
							x = (int)x_float;
							dist = x_float - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
							else x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + stripe_color_Y_8 * dist;
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + stripe_color_U_8 * dist;
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + stripe_color_V_8 * dist;

							x--;
							x_frame_tmp--;
						} else {
							x = hyperdeck_width - 1;
							x_frame_tmp = x + y * frame_tmp_linesize;
						}

						while ((x >= 0) && (x >= (int)last_x[y])) {
							*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8;
							*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8;
							*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8;

							x--;
							x_frame_tmp--;
						}
					}
					last_x[y] = x_float;

					x_float_2 = last_x_2[y] + stride;

					if (x_float_2 >= 0) {
						if (x_float_2 < hyperdeck_width) {
							if (frame_nb <= fade) {
								dist = (float)frame_nb / (float)fade;
								for (x = (int)x_float_2; x < hyperdeck_width; x++) {
									x_frame_tmp = x + y * frame_tmp_linesize;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

									*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8 * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
									*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8 * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
									*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8 * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);
								}

								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = *(frame_tmp->data[0] + x_frame_tmp) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
								*(frame_tmp->data[1] + x_frame_tmp) = *(frame_tmp->data[1] + x_frame_tmp) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
								*(frame_tmp->data[2] + x_frame_tmp) = *(frame_tmp->data[2] + x_frame_tmp) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;
							} else {
								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8 * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
								*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8 * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
								*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8 * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;
							}

							x--;
						} else x = hyperdeck_width - 1;

						if (x >= 0) {
							if ((int)last_x_2[y] <= 0) {
								x_frame_tmp = y * frame_tmp_linesize;
								n_bytes = x + 1;

								if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + y * fresque_frame_linesize;
								else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * fresque_frame_linesize;
							} else {
								x_frame_tmp = (int)last_x_2[y] + y * frame_tmp_linesize;
								n_bytes = x - (int)last_x_2[y] + 1;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)last_x_2[y]) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)last_x_2[y]) + (y * fresque_frame_linesize);
							}

							if (n_bytes == 1) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							} else if (n_bytes >= 2) {
								memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
							}
						}
					}
					last_x_2[y] = x_float_2;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}
		} else {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] + hyperdeck_width * transition_stripe_width / 100;
				}
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) + (stride / 4) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] + hyperdeck_width * transition_stripe_width / 100;
					y++;
					last_x[y] = sin_minus_7 * y + stride * (transition_nb_frames - 1) - (stride / 4) + (first_hyperdeck->number - hyperdeck->number) * hyperdeck_width;
					last_x_2[y] = last_x[y] + hyperdeck_width * transition_stripe_width / 100;

					x = (int)last_x[y];
					x_frame_tmp = x + y * frame_tmp_linesize;

					while (x < hyperdeck_width) {
						*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8;
						*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8;
						*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8;

						x++;
						x_frame_tmp++;
					}
				}
			}
			for (frame_nb = transition_nb_frames - 2 + fade; frame_nb >= 0; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] - stride;

					if (x_float < hyperdeck_width) {
						if (x_float >= 0) {
							x = (int)x_float;
							dist = x_float - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
							else x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * dist + stripe_color_Y_8 * (1.0 - dist);
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * dist + stripe_color_U_8 * (1.0 - dist);
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * dist + stripe_color_V_8 * (1.0 - dist);

							x++;
							x_frame_tmp++;
						} else {
							x = 0;
							x_frame_tmp = y * frame_tmp_linesize;
						}

						while ((x < hyperdeck_width) && (x <= last_x[y])) {
							*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8;
							*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8;
							*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8;

							x++;
							x_frame_tmp++;
							x_fresque++;
						}
					}
					last_x[y] = x_float;

					x_float_2 = last_x_2[y] - stride;

/*					if (x_float_2 < hyperdeck_width) {
						if (x_float_2 >= 0) {
							if (frame_nb <= fade) {
								dist = (float)frame_nb / (float)fade;

								for (x = (int)x_float_2; x >= 0; x--) {
									x_frame_tmp = x + y * frame_tmp_linesize;

									if (reverse) x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
									else x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

									*(frame_tmp->data[0] + x_frame_tmp) = 235 * dist + *(background_frame->data[0] + x_background) * (1.0 - dist);
									*(frame_tmp->data[1] + x_frame_tmp) = 128 * dist + *(background_frame->data[1] + x_background) * (1.0 - dist);
									*(frame_tmp->data[2] + x_frame_tmp) = 128 * dist + *(background_frame->data[2] + x_background) * (1.0 - dist);
								}

								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = *(frame_tmp->data[0] + x_frame_tmp) * dist + *(background_frame->data[0] + x_background) * (1.0 - dist);
								*(frame_tmp->data[1] + x_frame_tmp) = *(frame_tmp->data[1] + x_frame_tmp) * dist + *(background_frame->data[1] + x_background) * (1.0 - dist);
								*(frame_tmp->data[2] + x_frame_tmp) = *(frame_tmp->data[2] + x_frame_tmp) * dist + *(background_frame->data[2] + x_background) * (1.0 - dist);
							} else {
								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
								else x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = 235 * dist + *(background_frame->data[0] + x_background) * (1.0 - dist);
								*(frame_tmp->data[1] + x_frame_tmp) = 128 * dist + *(background_frame->data[1] + x_background) * (1.0 - dist);
								*(frame_tmp->data[2] + x_frame_tmp) = 128 * dist + *(background_frame->data[2] + x_background) * (1.0 - dist);
							}


							x++;
							x_frame_tmp++;
							x_fresque++;
						} else {
							x = 0;
							x_frame_tmp = y * frame_tmp_linesize;

							if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + y * fresque_frame_linesize;
							else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * fresque_frame_linesize;
						}

						if (x < hyperdeck_width) {
							if ((last_x_2[y] - hyperdeck_width) > 0) n_bytes = hyperdeck_width - x;
							else n_bytes = last_x_2[y] - x + 1;

							if (n_bytes == 1) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							} else if (n_bytes >= 2) {
								memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
							}
						}
					}*/
					if (x_float_2 < hyperdeck_width) {
						if (x_float_2 >= 0) {
							if (frame_nb <= fade) {
								dist = (float)frame_nb / (float)fade;

								for (x = (int)x_float_2; x >= 0; x--) {
									x_frame_tmp = x + y * frame_tmp_linesize;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

									*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8 * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
									*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8 * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
									*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8 * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);
								}

								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = *(frame_tmp->data[0] + x_frame_tmp) * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
								*(frame_tmp->data[1] + x_frame_tmp) = *(frame_tmp->data[1] + x_frame_tmp) * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
								*(frame_tmp->data[2] + x_frame_tmp) = *(frame_tmp->data[2] + x_frame_tmp) * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);
							} else {
								x = (int)x_float_2;
								dist = x_float_2 - x;
								x_frame_tmp = x + y * frame_tmp_linesize;

								if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
								else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

								*(frame_tmp->data[0] + x_frame_tmp) = stripe_color_Y_8 * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
								*(frame_tmp->data[1] + x_frame_tmp) = stripe_color_U_8 * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
								*(frame_tmp->data[2] + x_frame_tmp) = stripe_color_V_8 * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);
							}

							x++;
							x_frame_tmp++;
							x_fresque++;
						} else {
							x = 0;
							x_frame_tmp = y * frame_tmp_linesize;

							if (reverse) x_fresque = hyperdeck->number * hyperdeck_width + y * fresque_frame_linesize;
							else x_fresque = (hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + y * fresque_frame_linesize;
						}

						if (x < hyperdeck_width) {
							if ((last_x_2[y] - hyperdeck_width) > 0) n_bytes = hyperdeck_width - x;
							else n_bytes = last_x_2[y] - x + 1;

							if (n_bytes == 1) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							} else if (n_bytes >= 2) {
								memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
								memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
							}
						}
					}
					last_x_2[y] = x_float_2;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}
		}

		g_free (last_x_2);

	} if (transition_type == 2) {
		if (!transition_inv) {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) last_x[y] = hyperdeck_width;
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = hyperdeck_width + (step / 4);
					y++;
					last_x[y] = hyperdeck_width - (step / 4);
				}
			}

			for (y = 0; y < nb_lines; y++) {
				x_float = last_x[y] - step;

				for (j = 0; j < transition_nb_shutters; j++) {
					x = (int)(x_float - j * stride);
					dist = x_float - j * stride - x;
					x_frame_tmp = x + y * frame_tmp_linesize;

					if (reverse) {
						x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
						x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
					} else {
						x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
						x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
					}

					*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
					*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
					*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);

					n_bytes = hyperdeck_width - j * stride - x;
					x_frame_tmp++;
					x_fresque++;

					if (n_bytes == 1) {
						*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
						*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
						*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
					} else if (n_bytes >= 2) {
						memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
					}
				}
				last_x[y] = x_float;
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts = frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;

			for (frame_nb = transition_nb_frames - 2; frame_nb > 1; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] - step;

					for (j = 0; j < transition_nb_shutters; j++) {
						x = (int)(x_float - j * stride);

						if (x >= hyperdeck_width - (j + 1) * stride) {
							dist = x_float - j * stride - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) {
								x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
							} else {
								x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
							}

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * dist + *(fresque_frame->data[0] + x_fresque) * (1.0 - dist);
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * dist + *(fresque_frame->data[1] + x_fresque) * (1.0 - dist);
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * dist + *(fresque_frame->data[2] + x_fresque) * (1.0 - dist);

							x++;
							x_frame_tmp++;
							x_fresque++;
						} else {
							x = hyperdeck_width - (j + 1) * stride;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
							else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
						}

						if (step > 1.0) {
							if (x < hyperdeck_width - (int)(j * stride)) {
								n_bytes = (int)step + 1;

								if (n_bytes <= 1) {
									*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
									*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
									*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
								} else if (n_bytes >= 2) {
									memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
									memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
									memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
								}
							}
						} else {
							if (x < hyperdeck_width - (int)(j * stride)) {
								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							}
						}
					}
					last_x[y] = x_float;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}

			for (y = 0; y < nb_lines; y++) {
				for (j = 0; j < transition_nb_shutters; j++) {
					x_frame_tmp = j * stride + y * frame_tmp_linesize;
					n_bytes = (int)last_x[y] - (hyperdeck_width - (int)stride) + 1;

					if (reverse) x_fresque = ((hyperdeck->number * hyperdeck_width) + j * stride) + (y * fresque_frame_linesize);
					else x_fresque = (((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width) + j * stride) + (y * fresque_frame_linesize);

					if (n_bytes <= 1) {
						*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
						*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
						*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
					} else if (n_bytes >= 2) {
						memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
					}
				}
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts += frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
		} else {
			if (progressif) {
				for (y = 0; y < nb_lines; y++) last_x[y] = 0.0;
			} else {
				for (y = 0; y < nb_lines; y++) {
					last_x[y] = -(step / 4);
					y++;
					last_x[y] = (step / 4);

					for (j = 0; j < transition_nb_shutters; j++) {
						x = (int)(j * stride);
						x_frame_tmp = x + y * frame_tmp_linesize;
						n_bytes = (int)last_x[y];

						if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
						else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

						if (n_bytes == 1) {
							*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
							*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
							*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
						} else if (n_bytes >= 2) {
							memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
							memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
							memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
						}
					}
				}
			}

			for (frame_nb = transition_nb_frames - 1; frame_nb > 1; frame_nb--) {
				for (y = 0; y < nb_lines; y++) {
					x_float = last_x[y] + step;

					for (j = 0; j < transition_nb_shutters; j++) {
						x = (int)(x_float + j * stride);
						if (x < hyperdeck_width) {
							dist = x_float + j * stride - x;
							x_frame_tmp = x + y * frame_tmp_linesize;

							if (reverse) {
								x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
							} else {
								x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
								x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
							}

							*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
							*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
							*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;

							x--;
						} else x = hyperdeck_width - 1;

						if (step > 1.0) {
							if (x >= (int)(j * stride)) {
								if ((int)last_x[y] <= 0) {
									x_frame_tmp = (int)(j * stride) + y * frame_tmp_linesize;
									n_bytes = x - (int)(j * stride) + 1;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)(j * stride)) + y * fresque_frame_linesize;
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)(j * stride)) + y * fresque_frame_linesize;
								} else {
									x_frame_tmp = (int)(last_x[y] + j * stride) + y * frame_tmp_linesize;
									n_bytes = x - (int)(last_x[y] + j * stride) + 1;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);
								}
								if (n_bytes == 1) {
									*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
									*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
									*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
								} else if (n_bytes >= 2) {
									memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
									memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
									memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
								}
							}
						} else {
							if (x >= (int)(j * stride)) {
								if ((int)last_x[y] <= 0) {
									x_frame_tmp = (int)(j * stride) + y * frame_tmp_linesize;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)(j * stride)) + y * fresque_frame_linesize;
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)(j * stride)) + y * fresque_frame_linesize;
								} else {
									x_frame_tmp = (int)(last_x[y] + j * stride) + y * frame_tmp_linesize;

									if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);
									else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);
								}

								*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
								*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
								*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
							}
						}
					}
					last_x[y] = x_float;
				}

				sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
				frame_out->pts += frame_duration;
				avcodec_send_frame (av_codec_context_out, frame_out);

				avcodec_receive_packet (av_codec_context_out, &packet);
				packet.stream_index = av_stream_out->index;
				packet.duration = frame_duration;
				av_interleaved_write_frame (av_format_context_out, &packet);

				if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
			}

			for (y = 0; y < nb_lines; y++) {
				for (j = 0; j < transition_nb_shutters; j++) {
					x = hyperdeck_width - 1 - (int)((transition_nb_shutters - 1 - j) * stride);
					x_frame_tmp = (int)(last_x[y] + j * stride) + y * frame_tmp_linesize;
					n_bytes = x - (int)(last_x[y] + j * stride) + 1;

					if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);
					else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + (int)(last_x[y] + j * stride)) + (y * fresque_frame_linesize);

					if (n_bytes == 1) {
						*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
						*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
						*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
					} else if (n_bytes >= 2) {
						memcpy (frame_tmp->data[0] + x_frame_tmp, fresque_frame->data[0] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[1] + x_frame_tmp, fresque_frame->data[1] + x_fresque, n_bytes);
						memcpy (frame_tmp->data[2] + x_frame_tmp, fresque_frame->data[2] + x_fresque, n_bytes);
					}
				}
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts += frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
		}
	} if (transition_type == 3) {
		for (frame_nb = 1; frame_nb < transition_nb_frames - 1; frame_nb++) {
			if (progressif) {
				dist = (float)frame_nb / (float)(transition_nb_frames - 1);

				for (y = 0; y < nb_lines; y++) {
					for (x = 0; x < hyperdeck_width; x++) {
						x_frame_tmp = x + y * frame_tmp_linesize;

						if (reverse) {
							x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
						} else {
							x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
						}

						*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
						*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
						*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;
					}
				}
			} else {
				x_float = 1.0 / (float)(transition_nb_frames - 1);
				x_float_2 = x_float / 4.0;
				dist = x_float * (float)frame_nb - x_float_2;

				for (y = 0; y < nb_lines; y+=2) {
					for (x = 0; x < hyperdeck_width; x++) {
						x_frame_tmp = x + y * frame_tmp_linesize;

						if (reverse) {
							x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
						} else {
							x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
						}

						*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
						*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
						*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;
					}
				}

				dist += x_float_2 + x_float_2;

				for (y = 1; y < nb_lines; y+=2) {
					for (x = 0; x < hyperdeck_width; x++) {
						x_frame_tmp = x + y * frame_tmp_linesize;

						if (reverse) {
							x_background = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
						} else {
							x_background = (hyperdeck->number * hyperdeck_width + x) + (y * background_frame_linesize);
							x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);
						}

						*(frame_tmp->data[0] + x_frame_tmp) = *(background_frame->data[0] + x_background) * (1.0 - dist) + *(fresque_frame->data[0] + x_fresque) * dist;
						*(frame_tmp->data[1] + x_frame_tmp) = *(background_frame->data[1] + x_background) * (1.0 - dist) + *(fresque_frame->data[1] + x_fresque) * dist;
						*(frame_tmp->data[2] + x_frame_tmp) = *(background_frame->data[2] + x_background) * (1.0 - dist) + *(fresque_frame->data[2] + x_fresque) * dist;
					}
				}
			}

			sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
			frame_out->pts += frame_duration;
			avcodec_send_frame (av_codec_context_out, frame_out);

			avcodec_receive_packet (av_codec_context_out, &packet);
			packet.stream_index = av_stream_out->index;
			packet.duration = frame_duration;
			av_interleaved_write_frame (av_format_context_out, &packet);

			if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
		}

		for (y = 0; y < nb_lines; y++) {
			for (x = 0; x < hyperdeck_width; x++) {
				x_frame_tmp = x + y * frame_tmp_linesize;

				if (reverse) x_fresque = (hyperdeck->number * hyperdeck_width + x) + (y * fresque_frame_linesize);
				else x_fresque = ((hyperdeck->number - first_hyperdeck->number) * hyperdeck_width + x) + (y * fresque_frame_linesize);

				*(frame_tmp->data[0] + x_frame_tmp) = *(fresque_frame->data[0] + x_fresque);
				*(frame_tmp->data[1] + x_frame_tmp) = *(fresque_frame->data[1] + x_fresque);
				*(frame_tmp->data[2] + x_frame_tmp) = *(fresque_frame->data[2] + x_fresque);
			}
		}

		sws_scale (sws_context, (const uint8_t * const*)frame_tmp->data, frame_tmp->linesize, 0, nb_lines, frame_out->data, frame_out->linesize);
		frame_out->pts += frame_duration;
		avcodec_send_frame (av_codec_context_out, frame_out);

		avcodec_receive_packet (av_codec_context_out, &packet);
		packet.stream_index = av_stream_out->index;
		packet.duration = frame_duration;
		av_interleaved_write_frame (av_format_context_out, &packet);

		if (hyperdeck == first_hyperdeck) transcoding_frames[first_hyperdeck->number].frame_count++;
	}

	av_write_trailer (av_format_context_out);
	drop_list->ffmpeg_buffer_size = avio_close_dyn_buf (av_format_context_out->pb, &drop_list->ffmpeg_buffer);

g_mutex_lock (&hyperdeck->drop_mutex);
	drop_list->next = hyperdeck->drop_list_file;
	hyperdeck->drop_list_file = drop_list;
	if (hyperdeck->drop_thread == NULL) hyperdeck->drop_thread = g_thread_new (NULL, (GThreadFunc)drop_to_hyperdeck, hyperdeck);
g_mutex_unlock (&hyperdeck->drop_mutex);

	avcodec_free_context (&av_codec_context_out);
	avformat_free_context (av_format_context_out);
}

