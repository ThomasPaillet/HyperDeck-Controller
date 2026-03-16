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

#ifndef __HYPERDECK_CODEC_H
#define __HYPERDECK_CODEC_H


#include "HyperDeck.h"

#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>


#define HYPERDECK_CHROMA_LOCATION AVCHROMA_LOC_UNSPECIFIED


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

extern const AVCodec *av_codec_dnxhd, *av_codec_prores, *av_codec_out;

extern GMutex avcodec_open2_mutex;


void create_output_context (hyperdeck_t* hyperdeck, AVFormatContext **av_format_context, AVCodecContext **av_codec_context, AVStream **av_stream, char *creation_time);

AVFilterGraph* create_filter_graph (AVFilterContext **av_filter_context_in, AVFilterContext **av_filter_context_out, AVCodecContext *av_codec_context_in, const char *filter_descr);

GtkWidget* get_libavformat_version (void);
GtkWidget* get_libavcodec_version (void);
GtkWidget* get_libavfilter_version (void);

void init_hyperdeck_codec (void);


#endif

