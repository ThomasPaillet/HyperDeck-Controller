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


char *video_format;
int nb_lines = 1080;		//480, 576, 720, 1080, 2160, 4320
gboolean progressif = TRUE;
float frequency = 25.0;		//23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0

int hyperdeck_width = 1920;
AVRational hyperdeck_sample_aspect_ratio = (AVRational){ 1, 1 };
enum AVColorPrimaries hyperdeck_color_primaries = AVCOL_PRI_BT709;		//ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
enum AVColorTransferCharacteristic hyperdeck_color_trc = AVCOL_TRC_BT709;	//ITU-R BT1361
enum AVColorSpace hyperdeck_colorspace = AVCOL_SPC_BT709;				//ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
const int *hyperdeck_yuv2rgb_coefficients;
AVRational hyperdeck_time_base = (AVRational){ 1, 25 };
AVRational hyperdeck_framerate = (AVRational){ 25, 1 };
char *video_format_label = "HD";

char *file_ext_mov = "mov";
char *file_ext_mxf = "mxf";

char *file_format;
char *file_ext;
enum AVCodecID hyperdeck_codec = AV_CODEC_ID_DNXHD;
int codec_quality = 0;
enum AVPixelFormat hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
int dnxhd_bitrate = 185000000;

AVCodec *av_codec_dnxhd, *av_codec_prores, *av_codec_out;


#define PRORES_ENCODER "prores_ks"									//"prores_aw"
char *prores_profiles[5] = { "proxy", "lt", "standard", "hq", "4444" };	//{ "0", "1", "2", "3", "4" };

char *dnx_profiles[5] = { "dnxhd", "dnxhd", "dnxhr_lb" , "dnxhr_sq", "dnxhr_hqx" };

GMutex avcodec_open2_mutex;


void create_output_context (hyperdeck_t* hyperdeck, AVFormatContext **av_format_context, AVCodecContext **av_codec_context, AVStream **av_stream, char *creation_time)
{
	char timecode[12];
	AVCodecContext *av_codec_context_out;
	AVStream *av_stream_out;
	AVDictionary *opts = NULL;

	sprintf (timecode, "%02d:00:00:00", hyperdeck->number + 1);

	avformat_alloc_output_context2 (av_format_context, NULL, file_ext, NULL);

	av_dict_set (&(*av_format_context)->metadata, "creation_time", creation_time, 0);

	av_codec_context_out = avcodec_alloc_context3 (av_codec_out);
	av_codec_context_out->pix_fmt = hyperdeck_pix_fmt;
	av_codec_context_out->chroma_sample_location = HYPERDECK_CHROMA_LOCATION;
	av_codec_context_out->color_range = AVCOL_RANGE_MPEG;	//the normal 219*2^(n-8) "MPEG" YUV ranges

	av_codec_context_out->height = nb_lines;
	av_codec_context_out->width = hyperdeck_width;
	av_codec_context_out->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;

	av_codec_context_out->color_primaries = hyperdeck_color_primaries;
	av_codec_context_out->color_trc = hyperdeck_color_trc;
	av_codec_context_out->colorspace = hyperdeck_colorspace;

	av_codec_context_out->time_base = hyperdeck_time_base;
	av_codec_context_out->framerate = hyperdeck_framerate;

	if (progressif) av_codec_context_out->field_order = AV_FIELD_PROGRESSIVE;
	else {
		av_codec_context_out->field_order = AV_FIELD_TB;
		av_codec_context_out->flags |= AV_CODEC_FLAG_INTERLACED_DCT;
	}

	if (hyperdeck_codec == AV_CODEC_ID_DNXHD) {
		av_codec_context_out->bit_rate = dnxhd_bitrate;
		av_dict_set (&opts, "profile", dnx_profiles[codec_quality], 0);
	} else {
		av_dict_set (&opts, "profile", prores_profiles[codec_quality], 0);
		av_dict_set (&opts, "vendor", "apl0", 0);
	}

/*	if (progressif) av_dict_set (&opts, "field_order", "progressive", 0);
	else {
		av_dict_set (&opts, "field_order", "tb", 0);
		av_dict_set (&opts, "flags", "ildct", 0);
	}*/
//	av_dict_set (&opts, "color_primaries", av_color_primaries_name (hyperdeck_color_primaries), 0);
//	av_dict_set (&opts, "color_trc", av_color_transfer_name (hyperdeck_color_trc), 0);
//	av_dict_set (&opts, "colorspace", av_color_space_name (hyperdeck_colorspace), 0);

g_mutex_lock (&avcodec_open2_mutex);
	avcodec_open2 (av_codec_context_out, av_codec_out, &opts);
g_mutex_unlock (&avcodec_open2_mutex);

	av_stream_out = avformat_new_stream (*av_format_context, av_codec_out);
	avcodec_parameters_from_context (av_stream_out->codecpar, av_codec_context_out);

	av_dict_set (&av_stream_out->metadata, "language", "eng", 0);

	if (hyperdeck_codec == AV_CODEC_ID_DNXHD) {
		if (codec_quality <= 1) av_dict_set (&av_stream_out->metadata, "encoder", "DNxHD 709", 0);
		else if (codec_quality == 2) av_dict_set (&av_stream_out->metadata, "encoder", "DNxHR LB", 0);		//"DNxHR/DNxHD"
		else if (codec_quality == 3) av_dict_set (&av_stream_out->metadata, "encoder", "DNxHR SQ", 0);
		else if (codec_quality == 4) av_dict_set (&av_stream_out->metadata, "encoder", "DNxHR HQX", 0);
	} else {
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

/*	if (progressif) av_stream_out->codecpar->field_order = AV_FIELD_PROGRESSIVE;
	else av_stream_out->codecpar->field_order = AV_FIELD_TB;*/

	avio_open_dyn_buf (&(*av_format_context)->pb);

	*av_codec_context = av_codec_context_out;
	*av_stream = av_stream_out;
}

AVFilterGraph* create_filter_graph (AVFilterContext **av_filter_context_in, AVFilterContext **av_filter_context_out, AVCodecContext *av_codec_context_in, const char *filter_descr)
{
DEBUG_S("create_filter_graph")
	AVFilterGraph *av_filter_graph;
	const AVFilter *av_filter_buffer, *av_filter_buffer_sink;
	AVFilterInOut *av_filter_in, *av_filter_out;
	char args[128];

	av_filter_graph = avfilter_graph_alloc ();

	av_filter_buffer = avfilter_get_by_name ("buffer");
	snprintf (args, 128, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d", av_codec_context_in->width, av_codec_context_in->height, av_codec_context_in->pix_fmt, hyperdeck_time_base.num, hyperdeck_time_base.den, av_codec_context_in->sample_aspect_ratio.num, av_codec_context_in->sample_aspect_ratio.den);
DEBUG_S (args)
	if (avfilter_graph_create_filter (av_filter_context_in, av_filter_buffer, "in", args, NULL, av_filter_graph) < 0) {
		avfilter_graph_free (&av_filter_graph);
		return NULL;
	}

	av_filter_buffer_sink = avfilter_get_by_name ("buffersink");
	avfilter_graph_create_filter (av_filter_context_out, av_filter_buffer_sink, "out", NULL, NULL, av_filter_graph);

	av_filter_in = avfilter_inout_alloc ();
	av_filter_in->name = av_strdup ("out");
	av_filter_in->filter_ctx = *av_filter_context_out;
	av_filter_in->pad_idx = 0;
	av_filter_in->next = NULL;
 
	av_filter_out = avfilter_inout_alloc ();
	av_filter_out->name = av_strdup ("in");
	av_filter_out->filter_ctx = *av_filter_context_in;
	av_filter_out->pad_idx = 0;
	av_filter_out->next = NULL;

DEBUG_S (filter_descr)
	if (avfilter_graph_parse_ptr (av_filter_graph, filter_descr, &av_filter_in, &av_filter_out, NULL) < 0) {
		avfilter_inout_free (&av_filter_out);
		avfilter_inout_free (&av_filter_in);
		avfilter_graph_free (&av_filter_graph);
		return NULL;
	}

	avfilter_graph_config (av_filter_graph, NULL);

	avfilter_inout_free (&av_filter_out);
	avfilter_inout_free (&av_filter_in);

DEBUG_S("create_filter_graph END")
	return av_filter_graph;
}

GtkWidget* get_libavformat_version (void)
{
	char ffmpeg_version[48];

	sprintf (ffmpeg_version, "Libavformat version: %d.%d.%d", LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO/*, avformat_license ()*/);

	return gtk_label_new (ffmpeg_version);
}

GtkWidget* get_libavcodec_version (void)
{
	char ffmpeg_version[48];

	sprintf (ffmpeg_version, "Libavcodec version: %d.%d.%d", LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO/*, avcodec_license ()*/);

	return gtk_label_new (ffmpeg_version);
}

GtkWidget* get_libavfilter_version (void)
{
	char ffmpeg_version[48];

	sprintf (ffmpeg_version, "Libavfilter version: %d.%d.%d", LIBAVFILTER_VERSION_MAJOR, LIBAVFILTER_VERSION_MINOR, LIBAVFILTER_VERSION_MICRO/*, avfilter_license ()*/);

	return gtk_label_new (ffmpeg_version);
}

void init_hyperdeck_codec (void)
{
	av_register_all ();
	avfilter_register_all ();

	av_codec_dnxhd = avcodec_find_encoder (AV_CODEC_ID_DNXHD);
	av_codec_prores = avcodec_find_encoder_by_name (PRORES_ENCODER);

	hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);

	file_ext = file_ext_mov;
	av_codec_out = av_codec_dnxhd;

	g_mutex_init (&avcodec_open2_mutex);
}

