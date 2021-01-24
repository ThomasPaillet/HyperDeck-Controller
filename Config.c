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
#include <libswscale/swscale.h>

#include "HyperDeck.h"
#include "HyperDeck_Protocol.h"


char *config_file_name = "HyperDeck.cfg";


#define NB_OF_VIDEO_FORMAT 23
#define DEFAULT_VIDEO_FORMAT 9

char* video_format_names[NB_OF_VIDEO_FORMAT] = { "NTSC", "PAL", "NTSCp", "PALp", \
										"720p50", "720p5994", "720p60", \
										"1080p23976", "1080p24", "1080p25", "1080p2997", "1080p30", \
										"1080i50", "1080i5994", "1080i60", \
										"4Kp23976", "4Kp24", "4Kp25", "4Kp2997", "4Kp30", \
										"4Kp50", "4Kp5994", "4Kp60" };
GtkWidget *video_format_combo_box_text;


#define NB_OF_FILE_FORMAT 12
#define DEFAULT_FILE_FORMAT 4

char* file_format_names[NB_OF_VIDEO_FORMAT] = { "QuickTimeProResHQ", "QuickTimeProRes", "QuickTimeProResLT", "QuickTimeProResProxy", "QuickTimeDNxHD220", "QuickTimeDNxHD145", "DNxHD220", "DNxHD145", "QuickTimeDNxHR_HQX", "QuickTimeDNxHR_SQ", "QuickTimeDNxHR_LB", "QuickTimeProRes4444" };
GtkWidget *file_format_combo_box_text;


GtkWidget *preset_lines[NB_OF_PRESETS][NB_OF_HYPERDECKS + 1];


#define NB_OF_TRANSITION_TYPES 4

GtkWidget *transition_type_radio_buttons[NB_OF_TRANSITION_TYPES];
char *transition_type_names[NB_OF_TRANSITION_TYPES] = { "Biais 7°", "Biais 7° + couleur", "Volets", "Fondu" };
GtkWidget *transition_direction_label, *transition_direction_radio_buttons[2], *transition_return_inv_check_button;
GtkWidget *transition_stripe_color_label, *transition_stripe_color_button;
GtkWidget *transition_stripe_width_label, *transition_stripe_width_scale, *transition_stripe_width_percent_label;
GtkWidget *transition_nb_shutters_label, *transition_nb_shutters_scale;
GtkWidget *transition_nb_frames_scale, *transition_nb_frames_label;
char transition_nb_frames_string[4];


void notify_active_switch_config_hyperdecks_window (GtkSwitch *on_off, GParamSpec *pspec, GtkWidget *line)
{
	gtk_widget_set_sensitive (line, gtk_switch_get_active (on_off));
}

void set_video_format (int video_fmt)
{
	switch (video_fmt) {
		case 0: nb_lines = 480;
			progressif = FALSE;
			frequency = 29.97;
			video_format_label = "SD NTSC";
			break;
		case 1: nb_lines = 576;
			progressif = FALSE;
			frequency = 25.0;
			video_format_label = "SD";
			break;
		case 2: nb_lines = 480;
			progressif = TRUE;
			frequency = 29.97;
			video_format_label = "SD NTSC";
			break;
		case 3: nb_lines = 576;
			progressif = TRUE;
			frequency = 25.0;
			video_format_label = "SD";
			break;
		case 4: nb_lines = 720;
			progressif = TRUE;
			frequency = 50.0;
			video_format_label = "HD ready";
			break;
		case 5: nb_lines = 720;
			progressif = TRUE;
			frequency = 59.94;
			video_format_label = "HD ready";
			break;
		case 6: nb_lines = 720;
			progressif = TRUE;
			frequency = 60.0;
			video_format_label = "HD ready";
			break;
		case 7: nb_lines = 1080;
			progressif = TRUE;
			frequency = 23.976;
			video_format_label = "HD";
			break;
		case 8: nb_lines = 1080;
			progressif = TRUE;
			frequency = 24.0;
			video_format_label = "HD";
			break;
		case 9: nb_lines = 1080;
			progressif = TRUE;
			frequency = 25.0;
			video_format_label = "HD";
			break;
		case 10: nb_lines = 1080;
			progressif = TRUE;
			frequency = 29.97;
			video_format_label = "HD";
			break;
		case 11: nb_lines = 1080;
			progressif = TRUE;
			frequency = 30.0;
			video_format_label = "HD";
			break;
		case 12: nb_lines = 1080;
			progressif = FALSE;
			frequency = 25.0;
			video_format_label = "HD";
			break;
		case 13: nb_lines = 1080;
			progressif = FALSE;
			frequency = 29.97;
			video_format_label = "HD";
			break;
		case 14: nb_lines = 1080;
			progressif = FALSE;
			frequency = 30.0;
			video_format_label = "HD";
			break;
		case 15: nb_lines = 2160;
			progressif = TRUE;
			frequency = 23.976;
			video_format_label = "UHD 4K";
			break;
		case 16: nb_lines = 2160;
			progressif = TRUE;
			frequency = 24.0;
			video_format_label = "UHD 4K";
			break;
		case 17: nb_lines = 2160;
			progressif = TRUE;
			frequency = 25.0;
			video_format_label = "UHD 4K";
			break;
		case 18: nb_lines = 2160;
			progressif = TRUE;
			frequency = 29.97;
			video_format_label = "UHD 4K";
			break;
		case 19: nb_lines = 2160;
			progressif = TRUE;
			frequency = 30.0;
			video_format_label = "UHD 4K";
			break;
		case 20: nb_lines = 2160;
			progressif = TRUE;
			frequency = 50.0;
			video_format_label = "UHD 4K";
			break;
		case 21: nb_lines = 2160;
			progressif = TRUE;
			frequency = 59.94;
			video_format_label = "UHD 4K";
			break;
		case 22: nb_lines = 2160;
			progressif = TRUE;
			frequency = 60.0;
			video_format_label = "UHD 4K";
			break;
	}

	if (nb_lines == 480) {
		hyperdeck_width = 720;
		hyperdeck_sample_aspect_ratio.num = 109;
		hyperdeck_sample_aspect_ratio.den = 90;
		hyperdeck_color_primaries = AVCOL_PRI_SMPTE170M;	//ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_SMPTE170M;		//ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_SMPTE170M);
	} else if (nb_lines == 576) {
		hyperdeck_width = 720;
		hyperdeck_sample_aspect_ratio.num = 118;	//64	//59
		hyperdeck_sample_aspect_ratio.den = 81;		//45	//54
		hyperdeck_color_primaries = AVCOL_PRI_BT470BG;	//ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_BT470BG;		//ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601		//AVCOL_SPC_SMPTE170M;		//ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU601);
	} else if (nb_lines == 720) {
		hyperdeck_width = 1280;
		hyperdeck_sample_aspect_ratio.num = 1;
		hyperdeck_sample_aspect_ratio.den = 1;
		hyperdeck_color_primaries = AVCOL_PRI_BT709;		//ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_BT709;			//ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);
	} else if (nb_lines == 1080) {
		hyperdeck_width = 1920;
		hyperdeck_sample_aspect_ratio.num = 1;
		hyperdeck_sample_aspect_ratio.den = 1;
		hyperdeck_color_primaries = AVCOL_PRI_BT709;		//ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_BT709;			//ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);
	} else if (nb_lines == 2160) {
		hyperdeck_width = 3840;
		hyperdeck_sample_aspect_ratio.num = 1;
		hyperdeck_sample_aspect_ratio.den = 1;
		hyperdeck_color_primaries = AVCOL_PRI_BT709;		//ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_BT709;			//ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);
//		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_BT2020);
	} else if (nb_lines == 4320) {
		hyperdeck_width = 7680;
		hyperdeck_sample_aspect_ratio.num = 1;
		hyperdeck_sample_aspect_ratio.den = 1;
		hyperdeck_color_primaries = AVCOL_PRI_BT709;		//ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
		hyperdeck_color_trc = AVCOL_TRC_BT709;			//ITU-R BT1361
		hyperdeck_colorspace = AVCOL_SPC_BT709;			//ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);
///		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_BT2020);
	}

	if (frequency == (float)23.976) {
		hyperdeck_time_base = (AVRational){ 1, 24000 };
		hyperdeck_framerate = (AVRational){ 24000, 1001 };
	} else if (frequency == (float)24.0) {
		hyperdeck_time_base = (AVRational){ 1, 24 };
		hyperdeck_framerate = (AVRational){ 24, 1 };
	} else if (frequency == (float)25.0) {
		hyperdeck_time_base = (AVRational){ 1, 25 };
		hyperdeck_framerate = (AVRational){ 25, 1 };
	} else if (frequency == (float)29.97) {
		hyperdeck_time_base = (AVRational){ 1, 30000 };
		hyperdeck_framerate = (AVRational){ 30000, 1001 };
	} else if (frequency == (float)30.0) {
		hyperdeck_time_base = (AVRational){ 1, 30 };
		hyperdeck_framerate = (AVRational){ 30, 1 };
	} else if (frequency == (float)50.0) {
		hyperdeck_time_base = (AVRational){ 1, 50 };
		hyperdeck_framerate = (AVRational){ 50, 1 };
	} else if (frequency == (float)59.94) {
		hyperdeck_time_base = (AVRational){ 1, 60000 };
		hyperdeck_framerate = (AVRational){ 60000, 1001 };
	} else if (frequency == (float)60.0) {
		hyperdeck_time_base = (AVRational){ 1, 60 };
		hyperdeck_framerate = (AVRational){ 60, 1 };
	}
}

void check_dnxhd_resolution (void)
{
	char msg[64];
	int msg_len, i;

	if ((nb_lines != 1080) && (nb_lines != 720)) {
		nb_lines = 1080;
		hyperdeck_width = 1920;
		hyperdeck_sample_aspect_ratio.num = 1;
		hyperdeck_sample_aspect_ratio.den = 1;
		hyperdeck_color_primaries = AVCOL_PRI_BT709;
		hyperdeck_color_trc = AVCOL_TRC_BT709;
		hyperdeck_colorspace = AVCOL_SPC_BT709;	
		hyperdeck_yuv2rgb_coefficients = sws_getCoefficients (SWS_CS_ITU709);
		video_format_label = "HD";

		g_free (video_format);
		video_format = g_malloc (16);

		if (progressif) {
			if (frequency == (float)23.976) strcpy (video_format, "1080p23976");
			else if (frequency == (float)24.0) strcpy (video_format, "1080p24");
			else if ((frequency == (float)25.0) || (frequency == (float)50.0)) {
				frequency = 25.0;
				hyperdeck_time_base = (AVRational){ 1, 25 };
				hyperdeck_framerate = (AVRational){ 25, 1 };
				strcpy (video_format, "1080p25");
			} else if ((frequency == (float)29.97) || (frequency == (float)59.94)) {
				frequency = 29.97;
				hyperdeck_time_base = (AVRational){ 1, 30000 };
				hyperdeck_framerate = (AVRational){ 30000, 1001 };
				strcpy (video_format, "1080p2997");
			} else /*if ((frequency == (float)30.0) || (frequency == (float)60.0))*/ {
				frequency = 30.0;
				hyperdeck_time_base = (AVRational){ 1, 30 };
				hyperdeck_framerate = (AVRational){ 30, 1 };
				strcpy (video_format, "1080p30");
			}
		} else {
			if ((frequency == (float)29.97) || (frequency == (float)59.94)) {
				frequency = 29.97;
				hyperdeck_time_base = (AVRational){ 1, 30000 };
				hyperdeck_framerate = (AVRational){ 30000, 1001 };
				strcpy (video_format, "1080i5994");
			} else if ((frequency == (float)30.0) ||(frequency == (float)60.0)) {
				frequency = 30.0;
				hyperdeck_time_base = (AVRational){ 1, 30 };
				hyperdeck_framerate = (AVRational){ 30, 1 };
				strcpy (video_format, "1080i60");
			} else {
				frequency = 25.0;
				hyperdeck_time_base = (AVRational){ 1, 25 };
				hyperdeck_framerate = (AVRational){ 25, 1 };
				strcpy (video_format, "1080i50");
			}
		}

		msg_len = sprintf (msg, msg_slot_select_video_format_, video_format);

		for (i = 0; i < NB_OF_HYPERDECKS; i++) if (hyperdecks[i].connected) send (hyperdecks[i].socket, msg, msg_len, 0);

		SLEEP (1)
	}
}

void set_dnxhd_bitrate (void)
{
	if (nb_lines == 720) {
		if (codec_quality == 1) {
			if (frequency == (float)23.976) dnxhd_bitrate = 90000000;
			else if (frequency == (float)24.0) dnxhd_bitrate = 90000000;
			else if (frequency == (float)25.0) dnxhd_bitrate = 90000000;
			else if (frequency == (float)29.97) dnxhd_bitrate = 110000000;
			else if (frequency == (float)30.0) dnxhd_bitrate = 110000000;
			else if (frequency == (float)50.0) dnxhd_bitrate = 180000000;
			else if (frequency == (float)59.94) dnxhd_bitrate = 220000000;
			else if (frequency == (float)60.0) dnxhd_bitrate = 220000000;
		} else if (codec_quality == 0) {
			if (frequency == (float)23.976) dnxhd_bitrate = 60000000;
			else if (frequency == (float)24.0) dnxhd_bitrate = 60000000;
			else if (frequency == (float)25.0) dnxhd_bitrate = 60000000;
			else if (frequency == (float)29.97) dnxhd_bitrate = 75000000;
			else if (frequency == (float)30.0) dnxhd_bitrate = 75000000;
			else if (frequency == (float)50.0) dnxhd_bitrate = 120000000;
			else if (frequency == (float)59.94) dnxhd_bitrate = 145000000;
			else if (frequency == (float)60.0) dnxhd_bitrate = 145000000;
		}
	} else if (nb_lines == 1080) {
		if (codec_quality == 1) {
			if (frequency == (float)23.976) dnxhd_bitrate = 175000000;
			else if (frequency == (float)24.0) dnxhd_bitrate = 175000000;
			else if (frequency == (float)25.0) dnxhd_bitrate = 185000000;
			else if (frequency == (float)29.97) dnxhd_bitrate = 220000000;
			else if (frequency == (float)30.0) dnxhd_bitrate = 220000000;
			else if (frequency == (float)50.0) dnxhd_bitrate = 365000000;
			else if (frequency == (float)59.94) dnxhd_bitrate = 440000000;
			else if (frequency == (float)60.0) dnxhd_bitrate = 440000000;
		} else if (codec_quality == 0) {
			if (frequency == (float)23.976) dnxhd_bitrate = 115000000;
			else if (frequency == (float)24.0) dnxhd_bitrate = 115000000;
			else if (frequency == (float)25.0) dnxhd_bitrate = 120000000;
			else if (frequency == (float)29.97) dnxhd_bitrate = 145000000;
			else if (frequency == (float)30.0) dnxhd_bitrate = 145000000;
			else if (frequency == (float)50.0) dnxhd_bitrate = 240000000;
			else if (frequency == (float)59.94) dnxhd_bitrate = 290000000;
			else if (frequency == (float)60.0) dnxhd_bitrate = 290000000;
		}
	}
}


#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>


void set_file_format (int file_fmt)
{
	switch (file_fmt) {
		case 0: file_ext = file_ext_mov;
			av_codec_out = av_codec_prores;
			hyperdeck_codec = AV_CODEC_ID_PRORES;
			codec_quality = 3;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			break;
		case 1: file_ext = file_ext_mov;
			av_codec_out = av_codec_prores;
			hyperdeck_codec = AV_CODEC_ID_PRORES;
			codec_quality = 2;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			break;
		case 2: file_ext = file_ext_mov;
			av_codec_out = av_codec_prores;
			hyperdeck_codec = AV_CODEC_ID_PRORES;
			codec_quality = 1;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			break;
		case 3: file_ext = file_ext_mov;
			av_codec_out = av_codec_prores;
			hyperdeck_codec = AV_CODEC_ID_PRORES;
			codec_quality = 0;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			break;
		case 4: file_ext = file_ext_mov;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 1;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			check_dnxhd_resolution ();
			break;
		case 5: file_ext = file_ext_mov;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 0;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P;
			check_dnxhd_resolution ();
			break;
		case 6: file_ext = file_ext_mxf;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 1;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			check_dnxhd_resolution ();
			break;
		case 7: file_ext = file_ext_mxf;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 0;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P;
			check_dnxhd_resolution ();
			break;
		case 8: file_ext = file_ext_mov;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 4;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P10LE;
			break;
		case 9: file_ext = file_ext_mov;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 3;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P;
			break;
		case 10: file_ext = file_ext_mov;
			av_codec_out = av_codec_dnxhd;
			hyperdeck_codec = AV_CODEC_ID_DNXHD;
			codec_quality = 2;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUV422P;
			break;
		case 11: file_ext = file_ext_mov;
			av_codec_out = av_codec_prores;
			hyperdeck_codec = AV_CODEC_ID_PRORES;
			codec_quality = 4;
			hyperdeck_pix_fmt = AV_PIX_FMT_YUVA444P10LE;
			break;
	} 
}

void config_hyperdecks_window_ok (GtkWidget *window)
{
	int i, j, ip[4];
	const gchar *entry_buffer_text;
	char msg[64];
	int msg_len;
	char *new_format;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		for (j = 0; j < 4; j++) {
			entry_buffer_text = gtk_entry_buffer_get_text (hyperdecks[i].ip_entry_buffer[j]);
			if (sscanf (entry_buffer_text, "%d", &ip[j]) != 1) break;
			else if (ip[j] < 0) break;
			else if (ip[j] > 254) break;
		}

		if (j == 4) {
			sprintf (hyperdecks[i].new_adresse_ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			hyperdecks[i].adresse_ip_is_valid = TRUE;
			if (strcmp (hyperdecks[i].new_adresse_ip, hyperdecks[i].adresse_ip) != 0) {
				if (hyperdecks[i].connected) disconnect_from_hyperdeck (&hyperdecks[i]);
				strcpy (hyperdecks[i].adresse_ip, hyperdecks[i].new_adresse_ip);
			}
		} else {
			hyperdecks[i].adresse_ip_is_valid = FALSE;
			hyperdecks[i].adresse_ip[0] = '\0';
			if (hyperdecks[i].connected) disconnect_from_hyperdeck (&hyperdecks[i]);
		}

		hyperdecks[i].switched_on = gtk_switch_get_active (GTK_SWITCH (hyperdecks[i].on_off));

		if (!hyperdecks[i].switched_on && hyperdecks[i].connected) disconnect_from_hyperdeck (&hyperdecks[i]);
	}

	new_format = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (video_format_combo_box_text));
	if (strcmp (new_format, video_format) != 0) {
		g_free (video_format);
		video_format = new_format;

		set_video_format (gtk_combo_box_get_active (GTK_COMBO_BOX (video_format_combo_box_text)));

		msg_len = sprintf (msg, msg_slot_select_video_format_, video_format);
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			if (hyperdecks[i].connected) send (hyperdecks[i].socket, msg, msg_len, 0);
		}
		SLEEP (1)

		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			if (hyperdecks[i].connected) {
				SEND (&hyperdecks[i], msg_clips_get)
				SEND (&hyperdecks[i], msg_transport_info)
			}
		}
	} else g_free (new_format);

	new_format = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (file_format_combo_box_text));
	if (strcmp (new_format, file_format) != 0) {
		g_free (file_format);
		file_format = new_format;

		set_file_format (gtk_combo_box_get_active (GTK_COMBO_BOX (file_format_combo_box_text)));

		msg_len = sprintf (msg, msg_configuration_file_format_, file_format);
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			if (hyperdecks[i].connected) send (hyperdecks[i].socket, msg, msg_len, 0);
		}
		SLEEP (1)

		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			if (hyperdecks[i].connected) {
				SEND (&hyperdecks[i], msg_clips_get)
				SEND (&hyperdecks[i], msg_transport_info)
			}
		}
	} else g_free (new_format);

	if ((hyperdeck_codec == AV_CODEC_ID_DNXHD) && (codec_quality <= 1)) set_dnxhd_bitrate ();

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (hyperdecks[i].switched_on && hyperdecks[i].adresse_ip_is_valid && !hyperdecks[i].connected)
			hyperdecks[i].connection_thread = g_thread_new (NULL, (GThreadFunc)connect_to_hyperdeck, &hyperdecks[i]);
	}

	write_config_file ();

	gtk_widget_destroy (window);
}

gboolean digit_key_press (GtkEntry *entry, GdkEventKey *event)
{
	if ((event->keyval >= GDK_KEY_0) && (event->keyval <= GDK_KEY_9)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_KP_0) && (event->keyval <= GDK_KEY_KP_9)) return GDK_EVENT_PROPAGATE;
	else if (event->state & GDK_CONTROL_MASK) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval == GDK_KEY_BackSpace) || (event->keyval == GDK_KEY_Tab) || (event->keyval == GDK_KEY_Clear) || (event->keyval == GDK_KEY_Return) || \
		(event->keyval == GDK_KEY_Escape) || (event->keyval == GDK_KEY_Delete) || (event->keyval == GDK_KEY_Cancel)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_Home) && (event->keyval <= GDK_KEY_Select)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_Execute) && (event->keyval <= GDK_KEY_Redo)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval == GDK_KEY_KP_Tab) || (event->keyval == GDK_KEY_KP_Enter)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_KP_Home) && (event->keyval <= GDK_KEY_KP_Delete)) return GDK_EVENT_PROPAGATE;
	else return GDK_EVENT_STOP;
}

void show_config_hyperdecks_window (void)
{
	int i, j, k, l;

	GtkWidget *config_hyperdecks_window, *box1, *frame, *grid, *box2, *widget, *button_ok, *button_cancel;
	GtkWidget *line[NB_OF_HYPERDECKS];
	char label_hyperdeck[16];
	GtkWidget *entry[NB_OF_HYPERDECKS][4];

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((gtk_widget_get_visible (remuxing_frames[i].frame)) || (gtk_widget_get_visible (transcoding_frames[i].frame)) || (gtk_widget_get_visible (hyperdecks[i].progress_bar))) {
			show_message_window ("Il n'est pas possible de modifier la configuration des boîtiers Hyperdeck pendant un encodage ou un transfert de fichier !");
			return;
		}
	}

	config_hyperdecks_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (config_hyperdecks_window), "Configuration");
	gtk_window_set_type_hint (GTK_WINDOW (config_hyperdecks_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size (GTK_WINDOW (config_hyperdecks_window), 200, 100);
	gtk_window_set_modal (GTK_WINDOW (config_hyperdecks_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (config_hyperdecks_window), GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (config_hyperdecks_window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Adresses IP");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_widget_set_margin_start (frame, 5);
		gtk_widget_set_margin_end (frame, 5);
		gtk_widget_set_margin_bottom (frame, 5);

		grid = gtk_grid_new ();
		gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
		gtk_grid_set_row_spacing (GTK_GRID (grid), 3);
		gtk_widget_set_margin_start (grid, 5);
		gtk_widget_set_margin_end (grid, 5);
		gtk_widget_set_margin_bottom (grid, 5);

		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			line[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

			hyperdecks[i].on_off = gtk_switch_new ();
			gtk_switch_set_active (GTK_SWITCH (hyperdecks[i].on_off), hyperdecks[i].switched_on);
			g_signal_connect (G_OBJECT (hyperdecks[i].on_off), "notify::active", G_CALLBACK (notify_active_switch_config_hyperdecks_window), line[i]);
			gtk_widget_set_sensitive (line[i], hyperdecks[i].switched_on);
			gtk_grid_attach (GTK_GRID (grid), hyperdecks[i].on_off, 0, i , 1, 1);

			gtk_grid_attach (GTK_GRID (grid), line[i], 1, i, 1, 1);

			sprintf (label_hyperdeck, "HyperDeck %d:", hyperdecks[i].number + 1);
			widget = gtk_label_new (label_hyperdeck);
			gtk_widget_set_margin_start (widget, 5);
			gtk_widget_set_margin_end (widget, 5);
			gtk_box_pack_start (GTK_BOX (line[i]), widget, FALSE, FALSE, 0);

			hyperdecks[i].ip_entry_buffer[0] = gtk_entry_buffer_new (NULL, -1);
			entry[i][0] = gtk_entry_new_with_buffer (hyperdecks[i].ip_entry_buffer[0]);
			gtk_entry_set_max_length (GTK_ENTRY (entry[i][0]), 3);
			gtk_entry_set_width_chars (GTK_ENTRY (entry[i][0]), 3);
			gtk_entry_set_alignment (GTK_ENTRY (entry[i][0]), 0.5);
			g_signal_connect (G_OBJECT (entry[i][0]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
			gtk_box_pack_start (GTK_BOX (line[i]), entry[i][0], FALSE, FALSE, 0);

			for (j = 1; j < 4; j++) {
				widget = gtk_label_new (".");
				gtk_box_pack_start (GTK_BOX (line[i]), widget, FALSE, FALSE, 0);

				hyperdecks[i].ip_entry_buffer[j] = gtk_entry_buffer_new (NULL, -1);
				entry[i][j] = gtk_entry_new_with_buffer (hyperdecks[i].ip_entry_buffer[j]);
				gtk_entry_set_max_length (GTK_ENTRY (entry[i][j]), 3);
				gtk_entry_set_width_chars (GTK_ENTRY (entry[i][j]), 3);
				gtk_entry_set_alignment (GTK_ENTRY (entry[i][j]), 0.5);
				g_signal_connect (G_OBJECT (entry[i][j]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
				gtk_box_pack_start (GTK_BOX (line[i]), entry[i][j], FALSE, FALSE, 0);
			}

			if (hyperdecks[i].adresse_ip_is_valid) {
				k = 0;
				for (j = 0; j < 3; j++) {
					l = 1;
					while (hyperdecks[i].adresse_ip[k+l] != '.') l++;
					gtk_entry_buffer_set_text (hyperdecks[i].ip_entry_buffer[j], hyperdecks[i].adresse_ip + k, l);
					k += l + 1;
				}
				l = 1;
				while (hyperdecks[i].adresse_ip[k+l] != '\0') l++;
				gtk_entry_buffer_set_text (hyperdecks[i].ip_entry_buffer[j], hyperdecks[i].adresse_ip + k, l);
			}
		}
		gtk_container_add (GTK_CONTAINER (frame), grid);
		gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		frame = gtk_frame_new ("Format");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_widget_set_margin_start (frame, 5);
		gtk_widget_set_margin_end (frame, 5);
		gtk_widget_set_margin_bottom (frame, 5);
			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
			gtk_widget_set_margin_start (box2, 5);
			gtk_widget_set_margin_end (box2, 5);
			gtk_widget_set_margin_bottom (box2, 5);
			gtk_box_set_spacing (GTK_BOX (box2), 5);
				widget = gtk_label_new ("Vidéo:");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

				video_format_combo_box_text = gtk_combo_box_text_new ();
				for (i = 0; i < NB_OF_VIDEO_FORMAT; i++) {
					gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (video_format_combo_box_text), video_format_names[i]);
					if (strcmp (video_format_names[i], video_format) == 0) gtk_combo_box_set_active (GTK_COMBO_BOX (video_format_combo_box_text), i);
				}
				gtk_box_pack_start (GTK_BOX (box2), video_format_combo_box_text, FALSE, FALSE, 0);

				widget = gtk_label_new ("Fichier:");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

				file_format_combo_box_text = gtk_combo_box_text_new ();
				for (i = 0; i < NB_OF_FILE_FORMAT; i++) {
					gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (file_format_combo_box_text), file_format_names[i]);
					if (strcmp (file_format_names[i], file_format) == 0) gtk_combo_box_set_active (GTK_COMBO_BOX (file_format_combo_box_text), i);
				}
				gtk_box_pack_start (GTK_BOX (box2), file_format_combo_box_text, FALSE, FALSE, 0);
			gtk_container_add (GTK_CONTAINER (frame), box2);
		gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_bottom (box2, 5);
		gtk_box_set_spacing (GTK_BOX (box2), 5);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			button_ok = gtk_button_new_with_label ("OK");
			g_signal_connect_swapped (G_OBJECT (button_ok), "clicked", G_CALLBACK (config_hyperdecks_window_ok), config_hyperdecks_window);
			gtk_box_pack_start (GTK_BOX (box2), button_ok, TRUE, TRUE, 0);

			button_cancel = gtk_button_new_with_label ("Annuler");
			g_signal_connect_swapped (G_OBJECT (button_cancel), "clicked", G_CALLBACK (gtk_widget_destroy), config_hyperdecks_window);
			gtk_box_pack_start (GTK_BOX (box2), button_cancel, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (config_hyperdecks_window), box1);

	gtk_window_set_resizable (GTK_WINDOW (config_hyperdecks_window), FALSE);
	gtk_widget_show_all (config_hyperdecks_window);
}

void notify_active_switch_config_presets_window (GtkSwitch *on_off, GParamSpec *pspec, GtkWidget **lines)
{
	int j;

	for (j = 0; j < NB_OF_HYPERDECKS; j++) {
		if (hyperdecks[j].switched_on) gtk_widget_set_sensitive (lines[j], gtk_switch_get_active (on_off));
	}

	gtk_widget_set_sensitive (lines[NB_OF_HYPERDECKS], gtk_switch_get_active (on_off));
}

void config_presets_window_ok (GtkWidget *window)
{
	int i, j;

	for (i = 0; i < NB_OF_PRESETS; i++) {
		gtk_button_set_label (GTK_BUTTON (presets[i].button), gtk_entry_buffer_get_text (presets[i].entry_buffer));
		presets[i].switched_on = gtk_switch_get_active (GTK_SWITCH (presets[i].on_off));
		gtk_widget_set_sensitive (presets[i].button, presets[i].switched_on);

		for (j = 0; j < NB_OF_HYPERDECKS; j++) {
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (presets[i].clips[j].radio_button_slot_1))) presets[i].clips[j].slot = 1;
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (presets[i].clips[j].radio_button_slot_2))) presets[i].clips[j].slot = 2;
			g_free (presets[i].clips[j].name);
			presets[i].clips[j].name = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (presets[i].clips[j].combo_box_text));
		}
	}

	write_config_file ();

	gtk_widget_destroy (window);
}

void radio_button_toggled (GtkToggleButton *button, preset_clip_t *clip)
{
	int k;
	const gchar *label;
	disk_list_t *disk_list_tmp;
	int combo_box_index;
	gboolean clip_is_here;

	if (!gtk_toggle_button_get_active (button)) return;

	clip_is_here = FALSE;

	gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (clip->combo_box_text));
	label = gtk_button_get_label (GTK_BUTTON (button));

	if (label[0] == '1') {
		disk_list_tmp = clip->hyperdeck->slot_1_disk_list;
		for (k = 0; disk_list_tmp != NULL; disk_list_tmp = disk_list_tmp->next, k++) {
			gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (clip->combo_box_text), disk_list_tmp->name);

			if ((clip->slot == 1) && (strcmp (clip->name, disk_list_tmp->name) == 0)) {
				clip_is_here = TRUE;
				combo_box_index = k;
			}
		}
		gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (clip->combo_box_text), " ");
		if (clip_is_here) gtk_combo_box_set_active (GTK_COMBO_BOX (clip->combo_box_text), k - combo_box_index);
		else gtk_combo_box_set_active (GTK_COMBO_BOX (clip->combo_box_text), 0);
	} else if (label[0] == '2') {
		disk_list_tmp = clip->hyperdeck->slot_2_disk_list;
		for (k = 0; disk_list_tmp != NULL; disk_list_tmp = disk_list_tmp->next, k++) {
			gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (clip->combo_box_text), disk_list_tmp->name);

			if ((clip->slot == 2) && (strcmp (clip->name, disk_list_tmp->name) == 0)) {
				clip_is_here = TRUE;
				combo_box_index = k;
			}
		}
		gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (clip->combo_box_text), " ");
		if (clip_is_here) gtk_combo_box_set_active (GTK_COMBO_BOX (clip->combo_box_text), k - combo_box_index);
		else gtk_combo_box_set_active (GTK_COMBO_BOX (clip->combo_box_text), 0);
	}
}

void show_config_presets_window (void)
{
	int i, j, k;

	GtkWidget *config_presets_window, *box1, *frame, *grid, *box2, *widget, *button_ok, *button_cancel;
	char label_hyperdeck[16];
	const char *preset_name;
	disk_list_t *disk_list_tmp;
	int combo_box_index;
	gboolean clip_is_here;

	config_presets_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (config_presets_window), "Configuration");
	gtk_window_set_type_hint (GTK_WINDOW (config_presets_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size (GTK_WINDOW (config_presets_window), 200, 200);
	gtk_window_set_modal (GTK_WINDOW (config_presets_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (config_presets_window), GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (config_presets_window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Presets");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_widget_set_margin_start (frame, 5);
		gtk_widget_set_margin_end (frame, 5);
		gtk_widget_set_margin_bottom (frame, 5);

		grid = gtk_grid_new ();
		gtk_grid_set_row_spacing (GTK_GRID (grid), 3);
		gtk_grid_set_column_spacing (GTK_GRID (grid), 3);
		gtk_widget_set_margin_start (grid, 5);
		gtk_widget_set_margin_end (grid, 5);
		gtk_widget_set_margin_bottom (grid, 5);

		for (j = 0; j < NB_OF_HYPERDECKS; j++) {
			sprintf (label_hyperdeck, "HyperDeck %d:", hyperdecks[j].number + 1);
			widget = gtk_label_new (label_hyperdeck);
			gtk_widget_set_sensitive (widget, hyperdecks[j].switched_on);
			gtk_grid_attach (GTK_GRID (grid), widget, 2 + j , 0, 1, 1);
		}

		for (i = 0; i < NB_OF_PRESETS; i++) {
			presets[i].on_off = gtk_switch_new ();
			gtk_switch_set_active (GTK_SWITCH (presets[i].on_off), presets[i].switched_on);
			g_signal_connect (presets[i].on_off, "notify::active", G_CALLBACK (notify_active_switch_config_presets_window), preset_lines[i]);
			gtk_grid_attach (GTK_GRID (grid), presets[i].on_off, 0, 1 + i, 1, 1);

			preset_name = gtk_button_get_label (GTK_BUTTON (presets[i].button));
			presets[i].entry_buffer = gtk_entry_buffer_new (preset_name, strlen (preset_name));
			preset_lines[i][NB_OF_HYPERDECKS] = gtk_entry_new_with_buffer (presets[i].entry_buffer);
			gtk_entry_set_max_length (GTK_ENTRY (preset_lines[i][NB_OF_HYPERDECKS]), PRESETS_NAME_LENGTH - 1);
			gtk_entry_set_width_chars (GTK_ENTRY (preset_lines[i][NB_OF_HYPERDECKS]), PRESETS_NAME_LENGTH - 1);
			gtk_widget_set_sensitive (preset_lines[i][NB_OF_HYPERDECKS], presets[i].switched_on);
			gtk_grid_attach (GTK_GRID (grid), preset_lines[i][NB_OF_HYPERDECKS], 1, 1 + i, 1, 1);

			for (j = 0; j < NB_OF_HYPERDECKS; j++) {
				preset_lines[i][j] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_sensitive (preset_lines[i][j], hyperdecks[j].switched_on);
				if (hyperdecks[j].switched_on) gtk_widget_set_sensitive (preset_lines[i][j], presets[i].switched_on);
				gtk_grid_attach (GTK_GRID (grid), preset_lines[i][j], 2 + j, 1 + i, 1, 1);

				widget = gtk_label_new ("slot:");
				gtk_widget_set_margin_end (widget, 3);
				gtk_box_pack_start (GTK_BOX (preset_lines[i][j]), widget, FALSE, FALSE, 0);

				presets[i].clips[j].radio_button_slot_1 = gtk_radio_button_new_with_label (NULL, "1");
				g_signal_connect (presets[i].clips[j].radio_button_slot_1, "toggled", G_CALLBACK (radio_button_toggled), &presets[i].clips[j]);
				gtk_box_pack_start (GTK_BOX (preset_lines[i][j]), presets[i].clips[j].radio_button_slot_1, TRUE, TRUE, 0);

				presets[i].clips[j].radio_button_slot_2 = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (presets[i].clips[j].radio_button_slot_1)), "2");
				g_signal_connect (presets[i].clips[j].radio_button_slot_2, "toggled", G_CALLBACK (radio_button_toggled), &presets[i].clips[j]);
				gtk_box_pack_start (GTK_BOX (preset_lines[i][j]), presets[i].clips[j].radio_button_slot_2, TRUE, TRUE, 0);

				widget = gtk_label_new ("clip:");
				gtk_widget_set_margin_end (widget, 3);
				gtk_box_pack_start (GTK_BOX (preset_lines[i][j]), widget, FALSE, FALSE, 0);

				presets[i].clips[j].combo_box_text = gtk_combo_box_text_new ();
				gtk_box_pack_start (GTK_BOX (preset_lines[i][j]), presets[i].clips[j].combo_box_text, FALSE, FALSE, 0);

				clip_is_here = FALSE;

				if (presets[i].clips[j].slot == 1) {
					disk_list_tmp = hyperdecks[j].slot_1_disk_list;
					for (k = 0; disk_list_tmp != NULL; disk_list_tmp = disk_list_tmp->next, k++) {
						gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (presets[i].clips[j].combo_box_text), disk_list_tmp->name);
						if (strcmp (presets[i].clips[j].name, disk_list_tmp->name) == 0) {
							clip_is_here = TRUE;
							combo_box_index = k;
						}
					}
					gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (presets[i].clips[j].combo_box_text), " ");
					if (clip_is_here) gtk_combo_box_set_active (GTK_COMBO_BOX (presets[i].clips[j].combo_box_text), k - combo_box_index);
					else gtk_combo_box_set_active (GTK_COMBO_BOX (presets[i].clips[j].combo_box_text), 0);
				} else if (presets[i].clips[j].slot == 2) {
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (presets[i].clips[j].radio_button_slot_2), TRUE);
				} else if (hyperdecks[j].slot_selected == 1) {
					disk_list_tmp = hyperdecks[j].slot_1_disk_list;
					while (disk_list_tmp != NULL) {
						gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (presets[i].clips[j].combo_box_text), disk_list_tmp->name);
						disk_list_tmp = disk_list_tmp->next;
					}
					gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (presets[i].clips[j].combo_box_text), " ");
					gtk_combo_box_set_active (GTK_COMBO_BOX (presets[i].clips[j].combo_box_text), 0);
				} else if (hyperdecks[j].slot_selected == 2) {
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (presets[i].clips[j].radio_button_slot_2), TRUE);
				}

				if (!hyperdecks[j].slot_1_is_mounted) gtk_widget_set_sensitive (presets[i].clips[j].radio_button_slot_1, FALSE);
				if (!hyperdecks[j].slot_2_is_mounted) gtk_widget_set_sensitive (presets[i].clips[j].radio_button_slot_2, FALSE);
			}
		}
		gtk_container_add (GTK_CONTAINER (frame), grid);
		gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_bottom (box2, 5);
		gtk_box_set_spacing (GTK_BOX (box2), 5);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			button_ok = gtk_button_new_with_label ("OK");
			g_signal_connect_swapped (button_ok, "clicked", G_CALLBACK (config_presets_window_ok), config_presets_window);
			gtk_box_pack_start (GTK_BOX (box2), button_ok, TRUE, TRUE, 0);

			button_cancel = gtk_button_new_with_label ("Annuler");
			g_signal_connect_swapped (button_cancel, "clicked", G_CALLBACK (gtk_widget_destroy), config_presets_window);
			gtk_box_pack_start (GTK_BOX (box2), button_cancel, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (config_presets_window), box1);

	gtk_window_set_resizable (GTK_WINDOW (config_presets_window), FALSE);
	gtk_widget_show_all (config_presets_window);
}

void config_transitions_window_ok (GtkWidget *window)
{
	int i;

	for (transition_type = 0; transition_type < NB_OF_TRANSITION_TYPES; transition_type++) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (transition_type_radio_buttons[transition_type]))) break;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (transition_direction_radio_buttons[0]))) transition_direction = 0;
	else transition_direction = 1;

	transition_return_inv = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (transition_return_inv_check_button));

	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (transition_stripe_color_button), &transition_stripe_color);
	stripe_color_RGB_to_YUV_8 ();
	stripe_color_RGB_to_YUV_16 ();

	transition_stripe_width = (int)gtk_range_get_value (GTK_RANGE (transition_stripe_width_scale));

	transition_nb_shutters = (int)gtk_range_get_value (GTK_RANGE (transition_nb_shutters_scale));

	transition_nb_frames = (int)gtk_range_get_value (GTK_RANGE (transition_nb_frames_scale));

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		transitions[i].switched_on = gtk_switch_get_active (GTK_SWITCH (transitions[i].on_off));

		strcpy (transitions[i].suffix, gtk_entry_buffer_get_text (transitions[i].entry_buffer));

		if (gtk_combo_box_get_active (GTK_COMBO_BOX (transitions[i].combo_box_text)) > 0) {
			g_free (transitions[i].file_name);
			transitions[i].file_name = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (transitions[i].combo_box_text));
		} else transitions[i].file_name[0] = '\0';
	}

	write_config_file ();

	gtk_widget_destroy (window);
}

void show_transition_parameters_widgets (int transition)
{
	switch (transition) {
		case 0: gtk_widget_show (transition_direction_label);
			gtk_widget_show (transition_direction_radio_buttons[0]);
			gtk_widget_show (transition_direction_radio_buttons[1]);
			gtk_widget_show (transition_return_inv_check_button);
			gtk_widget_hide (transition_stripe_color_label);
			gtk_widget_hide (transition_stripe_color_button);
			gtk_widget_hide (transition_stripe_width_label);
			gtk_widget_hide (transition_stripe_width_scale);
			gtk_widget_hide (transition_stripe_width_percent_label);
			gtk_widget_hide (transition_nb_shutters_label);
			gtk_widget_hide (transition_nb_shutters_scale);
			break;
		case 1: gtk_widget_show (transition_direction_label);
			gtk_widget_show (transition_direction_radio_buttons[0]);
			gtk_widget_show (transition_direction_radio_buttons[1]);
			gtk_widget_show (transition_return_inv_check_button);
			gtk_widget_show (transition_stripe_color_label);
			gtk_widget_show (transition_stripe_color_button);
			gtk_widget_show (transition_stripe_width_label);
			gtk_widget_show (transition_stripe_width_scale);
			gtk_widget_show (transition_stripe_width_percent_label);
			gtk_widget_hide (transition_nb_shutters_label);
			gtk_widget_hide (transition_nb_shutters_scale);
			break;
		case 2: gtk_widget_show (transition_direction_label);
			gtk_widget_show (transition_direction_radio_buttons[0]);
			gtk_widget_show (transition_direction_radio_buttons[1]);
			gtk_widget_show (transition_return_inv_check_button);
			gtk_widget_hide (transition_stripe_color_label);
			gtk_widget_hide (transition_stripe_color_button);
			gtk_widget_hide (transition_stripe_width_label);
			gtk_widget_hide (transition_stripe_width_scale);
			gtk_widget_hide (transition_stripe_width_percent_label);
			gtk_widget_show (transition_nb_shutters_label);
			gtk_widget_show (transition_nb_shutters_scale);
			break;
		case 3: gtk_widget_hide (transition_direction_label);
			gtk_widget_hide (transition_direction_radio_buttons[0]);
			gtk_widget_hide (transition_direction_radio_buttons[1]);
			gtk_widget_hide (transition_return_inv_check_button);
			gtk_widget_hide (transition_stripe_color_label);
			gtk_widget_hide (transition_stripe_color_button);
			gtk_widget_hide (transition_stripe_width_label);
			gtk_widget_hide (transition_stripe_width_scale);
			gtk_widget_hide (transition_stripe_width_percent_label);
			gtk_widget_hide (transition_nb_shutters_label);
			gtk_widget_hide (transition_nb_shutters_scale);
			break;
	}
}

void transition_type_radio_button_toggled (GtkToggleButton *button, gpointer transition)
{
	if (gtk_toggle_button_get_active (button)) show_transition_parameters_widgets (GPOINTER_TO_INT (transition));
}

void transition_nb_frames_value_changed (GtkRange *transition_nb_frames_scale)
{
	sprintf (transition_nb_frames_string, "%d", (int)gtk_range_get_value (GTK_RANGE (transition_nb_frames_scale)));
	gtk_label_set_text (GTK_LABEL (transition_nb_frames_label), transition_nb_frames_string);
}

void show_config_transitions_window (void)
{
	int i, j;

	GtkWidget *window, *box1, *box2, *box3, *box4, *frame, *grid, *widget;
	GtkWidget *line[NB_OF_TRANSITIONS];
	GSList *gslist;
	int combo_box_index;

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		for (j = 0; j < NB_OF_HYPERDECKS; j++) {
			if (transitions[i].thread[j] != NULL) {
				show_message_window ("Il n'est pas possible de modifier la configuration des transitions pendant un encodage !");
				return;
			}
		}
	}

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Configuration");
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Transitions");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_widget_set_margin_start (frame, 5);
		gtk_widget_set_margin_end (frame, 5);
		gtk_widget_set_margin_bottom (frame, 5);

		box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_margin_start (box3, 5);
		gtk_widget_set_margin_end (box3, 5);
		gtk_widget_set_margin_bottom (box3, 5);
		gtk_box_set_spacing (GTK_BOX (box3), 3);
			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
			gtk_box_set_spacing (GTK_BOX (box2), 3);
				widget = gtk_label_new ("Type:");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

				transition_type_radio_buttons[0] = gtk_radio_button_new_with_label (NULL, transition_type_names[0]);
				gtk_box_pack_start (GTK_BOX (box2), transition_type_radio_buttons[0], FALSE, FALSE, 0);

				for (i = 1; i < NB_OF_TRANSITION_TYPES; i++) {
					transition_type_radio_buttons[i] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (transition_type_radio_buttons[0]), transition_type_names[i]);
					gtk_box_pack_start (GTK_BOX (box2), transition_type_radio_buttons[i], FALSE, FALSE, 0);
				}

				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (transition_type_radio_buttons[transition_type]), TRUE);
				for (i = 0; i < NB_OF_TRANSITION_TYPES; i++) {
					g_signal_connect (G_OBJECT (transition_type_radio_buttons[i]), "toggled", G_CALLBACK (transition_type_radio_button_toggled), GINT_TO_POINTER (i));
				}
			gtk_box_pack_start (GTK_BOX (box3), box2, FALSE, FALSE, 0);

			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_box_set_spacing (GTK_BOX (box2), 3);
				transition_direction_label = gtk_label_new ("Direction:");
				gtk_box_pack_start (GTK_BOX (box2), transition_direction_label, FALSE, FALSE, 0);

				box4 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
					transition_direction_radio_buttons[0] = gtk_radio_button_new_with_label (NULL, "<--");
					gtk_box_pack_start (GTK_BOX (box4), transition_direction_radio_buttons[0], FALSE, FALSE, 0);
					transition_direction_radio_buttons[1] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (transition_direction_radio_buttons[0]), "-->");
					gtk_box_pack_start (GTK_BOX (box4), transition_direction_radio_buttons[1], FALSE, FALSE, 0);
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (transition_direction_radio_buttons[transition_direction]), TRUE);
				gtk_box_pack_start (GTK_BOX (box2), box4, FALSE, FALSE, 0);

				transition_return_inv_check_button = gtk_check_button_new_with_label ("Inverser au retour");
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (transition_return_inv_check_button), transition_return_inv);
				gtk_box_pack_start (GTK_BOX (box2), transition_return_inv_check_button, FALSE, FALSE, 0);

				transition_stripe_color_label = gtk_label_new ("Couleur:");
				gtk_box_pack_start (GTK_BOX (box2), transition_stripe_color_label, FALSE, FALSE, 0);

				transition_stripe_color_button = gtk_color_button_new_with_rgba (&transition_stripe_color);
				gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (transition_stripe_color_button), FALSE);
				gtk_box_pack_start (GTK_BOX (box2), transition_stripe_color_button, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box3), box2, FALSE, FALSE, 0);

			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_box_set_spacing (GTK_BOX (box2), 3);
				transition_stripe_width_label = gtk_label_new ("Largeur de la bande de couleur:");
				gtk_box_pack_start (GTK_BOX (box2), transition_stripe_width_label, FALSE, FALSE, 0);

				transition_stripe_width_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 10.0, 100.0, 1.0);
				gtk_scale_set_draw_value (GTK_SCALE (transition_stripe_width_scale), TRUE);
				gtk_scale_set_value_pos (GTK_SCALE (transition_stripe_width_scale), GTK_POS_RIGHT);
				gtk_range_set_value (GTK_RANGE (transition_stripe_width_scale), transition_stripe_width);
				gtk_box_pack_start (GTK_BOX (box2), transition_stripe_width_scale, TRUE, TRUE, 0);

				transition_stripe_width_percent_label = gtk_label_new ("%");
				gtk_box_pack_start (GTK_BOX (box2), transition_stripe_width_percent_label, FALSE, FALSE, 0);

				transition_nb_shutters_label = gtk_label_new ("Nombre de volets par Hyperdeck:");
				gtk_box_pack_start (GTK_BOX (box2), transition_nb_shutters_label, FALSE, FALSE, 0);

				transition_nb_shutters_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 1.0, 6.0, 1.0);
				gtk_scale_set_draw_value (GTK_SCALE (transition_nb_shutters_scale), TRUE);
				gtk_scale_set_value_pos (GTK_SCALE (transition_nb_shutters_scale), GTK_POS_RIGHT);
				gtk_range_set_value (GTK_RANGE (transition_nb_shutters_scale), transition_nb_shutters);
				gtk_box_pack_start (GTK_BOX (box2), transition_nb_shutters_scale, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box3), box2, FALSE, FALSE, 0);

			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				widget = gtk_label_new ("Durée:");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

				transition_nb_frames_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 5, 125, 1);
				gtk_widget_set_margin_start (transition_nb_frames_scale, 7);
				gtk_scale_set_draw_value (GTK_SCALE (transition_nb_frames_scale), FALSE);
				gtk_scale_set_value_pos (GTK_SCALE (transition_nb_frames_scale), GTK_POS_RIGHT);
				gtk_range_set_value (GTK_RANGE (transition_nb_frames_scale), transition_nb_frames);
				g_signal_connect (G_OBJECT (transition_nb_frames_scale), "value-changed", G_CALLBACK (transition_nb_frames_value_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box2), transition_nb_frames_scale, TRUE, TRUE, 0);

				sprintf (transition_nb_frames_string, "%d", transition_nb_frames);
				transition_nb_frames_label = gtk_label_new (transition_nb_frames_string);
				gtk_label_set_width_chars (GTK_LABEL (transition_nb_frames_label), 3);
				gtk_label_set_xalign (GTK_LABEL (transition_nb_frames_label), 1.0);
				gtk_box_pack_start (GTK_BOX (box2), transition_nb_frames_label, FALSE, FALSE, 0);

				widget = gtk_label_new (" images");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box3), box2, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box3);
		gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		frame = gtk_frame_new ("Fresques");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_widget_set_margin_start (frame, 5);
		gtk_widget_set_margin_end (frame, 5);
		gtk_widget_set_margin_bottom (frame, 5);

		grid = gtk_grid_new ();
		gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
		gtk_grid_set_row_spacing (GTK_GRID (grid), 3);
		gtk_grid_set_column_spacing (GTK_GRID (grid), 3);
		gtk_widget_set_margin_start (grid, 5);
		gtk_widget_set_margin_end (grid, 5);
		gtk_widget_set_margin_bottom (grid, 5);

		fill_background_slist ();

		for (i = 0; i < NB_OF_TRANSITIONS; i++) {
			line[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

			transitions[i].on_off = gtk_switch_new ();
			gtk_switch_set_active (GTK_SWITCH (transitions[i].on_off), transitions[i].switched_on);
			g_signal_connect (G_OBJECT (transitions[i].on_off), "notify::active", G_CALLBACK (notify_active_switch_config_hyperdecks_window), line[i]);
			gtk_widget_set_sensitive (line[i], transitions[i].switched_on);
			gtk_grid_attach (GTK_GRID (grid), transitions[i].on_off, 0, i , 1, 1);

				transitions[i].entry_buffer = gtk_entry_buffer_new (transitions[i].suffix, strlen (transitions[i].suffix));
				gtk_entry_buffer_set_max_length (GTK_ENTRY_BUFFER (transitions[i].entry_buffer), TRANSITION_SUFFIX_LENGTH - 1);
				widget = gtk_entry_new_with_buffer (transitions[i].entry_buffer);
				gtk_entry_set_width_chars (GTK_ENTRY (widget), TRANSITION_SUFFIX_LENGTH - 1);
				gtk_widget_set_margin_end (widget, 3);
				gtk_box_pack_start (GTK_BOX (line[i]), widget, FALSE, FALSE, 0);

				transitions[i].combo_box_text = gtk_combo_box_text_new ();

				combo_box_index = 0;
				for (gslist = background_slist, j = 1; gslist != NULL; gslist = gslist->next, j++) {
					gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (transitions[i].combo_box_text), gslist->data);
					if (strcmp (transitions[i].file_name, gslist->data) == 0) combo_box_index = j;
				}
				gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (transitions[i].combo_box_text), " ");

				gtk_combo_box_set_active (GTK_COMBO_BOX (transitions[i].combo_box_text), combo_box_index);

				gtk_box_pack_start (GTK_BOX (line[i]), transitions[i].combo_box_text, FALSE, FALSE, 0);
			gtk_grid_attach (GTK_GRID (grid), line[i], 1, i, 1, 1);
		}
		gtk_container_add (GTK_CONTAINER (frame), grid);
		gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		g_slist_free_full (background_slist, g_free);
		background_slist = NULL;
		background_slist_length = 0;

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_bottom (box2, 5);
		gtk_box_set_spacing (GTK_BOX (box2), 5);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			widget = gtk_button_new_with_label ("OK");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (config_transitions_window_ok), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, TRUE, TRUE, 0);

			widget = gtk_button_new_with_label ("Annuler");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
	show_transition_parameters_widgets (transition_type);
}

gboolean read_config_file (void)
{
	FILE *config_file;
	int i, j, k;
	char switched_on;
	int rien;
	int ip[4];
	char buffer[256];
	size_t n;
	gboolean return_value = TRUE;

	video_format = g_malloc (16);
	strcpy (video_format, video_format_names[DEFAULT_VIDEO_FORMAT]);

	file_format = g_malloc (32);
	strcpy (file_format, file_format_names[DEFAULT_FILE_FORMAT]);

	config_file = fopen (config_file_name, "r");
	if (config_file == NULL) return FALSE;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		k = fscanf (config_file, "%c HyperDeck %d: %d.%d.%d.%d\n", &switched_on, &rien, &ip[0], &ip[1], &ip[2], &ip[3]);
		if (k < 2) {
			for (j = i; j < NB_OF_HYPERDECKS; j++) hyperdecks[j].switched_on = FALSE;
			fclose (config_file);
			return FALSE;
		}
		if (switched_on == '0') hyperdecks[i].switched_on = FALSE;

		if (k == 6) {
			for (j = 0; j < 4; j++) {
				if (ip[j] < 0) break;
				else if (ip[j] > 254) break;
			}

			if (j == 4) {
				sprintf (hyperdecks[i].adresse_ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
				hyperdecks[i].adresse_ip_is_valid = TRUE;
			} else if (hyperdecks[i].switched_on) return_value = FALSE;
		} else if (hyperdecks[i].switched_on) return_value = FALSE;
	}

	if (fscanf (config_file, "Video format: %s\n", buffer) == 1) {
		n = strlen (buffer);
		if (n < 16) {
			for (i = 0; i < NB_OF_VIDEO_FORMAT; i++) {
				if (memcmp (video_format_names[i], buffer, n) == 0) {
					memcpy (video_format, buffer, n);
					video_format[n] = '\0';
					set_video_format (i);
					break;
				}
			}
		}
	}

	if (fscanf (config_file, "File format: %s\n", buffer) == 1) {
		n = strlen (buffer);
		if (n < 32) {
			for (i = 0; i < NB_OF_FILE_FORMAT; i++) {
				if (memcmp (file_format_names[i], buffer, n) == 0) {
					memcpy (file_format, buffer, n);
					file_format[n] = '\0';
					set_file_format (i);
					break;
				}
			}
		}
	}

	if ((hyperdeck_codec == AV_CODEC_ID_DNXHD) && (codec_quality <= 1)) set_dnxhd_bitrate ();

	for (i = 0; i < NB_OF_PRESETS; i++) {
		if (fscanf (config_file, "%c Preset %d: ", &switched_on, &rien) != 2) break;
		if (switched_on == '1') presets[i].switched_on = TRUE;

		for (k = 0; k < PRESETS_NAME_LENGTH; k++) {
			buffer[k] = fgetc (config_file);
			if (buffer[k] == '\n') {
				buffer[k] = '\0';
				gtk_button_set_label (GTK_BUTTON (presets[i].button), buffer);
				break;
			} else if (buffer[k] == EOF) {
				presets[i].switched_on = FALSE;
				break;
			}
		}

		for (j = 0; j < NB_OF_HYPERDECKS; j++) {
			if (fscanf (config_file, "HyperDeck %d: slot: %d, clip name:", &rien, &presets[i].clips[j].slot) != 2) break;
			if ((presets[i].clips[j].slot != 1) && (presets[i].clips[j].slot != 2)) presets[i].clips[j].slot = 1;
			fgetc (config_file);

			for (k = 0; k < CLIP_NAME_LENGTH; k++) {
				presets[i].clips[j].name[k] = fgetc (config_file);
				if (presets[i].clips[j].name[k] == '\n') {
					presets[i].clips[j].name[k] = '\0';
					break;
				} else if (presets[i].clips[j].name[k] == EOF) {
					presets[i].clips[j].name[0] = '\0';
					break;
				}
			}
		}
	}

	fscanf (config_file, "Transitions:\ntype: %d, direction: %d, inv: %d, stripe_color:%lf %lf %lf, stripe_width: %d, nb_shutters: %d, duration: %d\n", &transition_type, &transition_direction, &transition_return_inv, &transition_stripe_color.red, &transition_stripe_color.green, &transition_stripe_color.blue, &transition_stripe_width, &transition_nb_shutters, &transition_nb_frames);
	if ((transition_type < 0) || (transition_type >= NB_OF_TRANSITION_TYPES)) transition_type = 0;
	if ((transition_stripe_color.red < 0.0) || (transition_stripe_color.red > 1.0)) transition_stripe_color.red = 1.0;
	if ((transition_stripe_color.green < 0.0) || (transition_stripe_color.green > 1.0)) transition_stripe_color.green = 1.0;
	if ((transition_stripe_color.blue < 0.0) || (transition_stripe_color.blue > 1.0)) transition_stripe_color.blue = 1.0;
	stripe_color_RGB_to_YUV_8 ();
	stripe_color_RGB_to_YUV_16 ();
	if ((transition_stripe_width < 10) || (transition_stripe_width > 100)) transition_stripe_width = 100;
	if ((transition_nb_shutters < 1) || (transition_nb_shutters > 6)) transition_nb_shutters = 6;
	if ((transition_nb_frames < 5) || (transition_nb_frames > 125)) transition_nb_frames = 25;

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		if (fscanf (config_file, "%c ", &switched_on) != 1) break;
		if (switched_on == '1') transitions[i].switched_on = TRUE;

		for (j = 0; j < TRANSITION_SUFFIX_LENGTH; j++) {
			transitions[i].suffix[j] = fgetc (config_file);
			if (transitions[i].suffix[j] == '\n') { 
				transitions[i].suffix[j] = '\0';
				break;
			} else if (transitions[i].suffix[j] == EOF) {
				transitions[i].switched_on = FALSE;
				transitions[i].suffix[0] = '\0';
				break;
			}
		}

		for (j = 0; j < CLIP_NAME_LENGTH; j++) {
			transitions[i].file_name[j] = fgetc (config_file);
			if (transitions[i].file_name[j] == '\n') {
				transitions[i].file_name[j] = '\0';
				break;
			} else if (transitions[i].file_name[j] == EOF) {
				transitions[i].switched_on = FALSE;
				transitions[i].file_name[0] = '\0';
				break;
			}
		}
	}

	fclose (config_file);

	return return_value;
}

void write_config_file (void)
{
	FILE *config_file;
	int i, j, k, l;

	config_file = fopen (config_file_name, "w");

	for (i = 0, j = 1; i < NB_OF_HYPERDECKS; i++, j++) {
		if (hyperdecks[i].switched_on) fputc ('1', config_file);
		else fputc ('0', config_file);

		if (hyperdecks[i].adresse_ip_is_valid) fprintf (config_file, " HyperDeck %d: %s\n", j, hyperdecks[i].adresse_ip);
		else fprintf (config_file, " HyperDeck %d:\n", j);
	}

	fprintf (config_file, "Video format: %s\n", video_format);
	fprintf (config_file, "File format: %s\n", file_format);

	for (i = 0, j = 1; i < NB_OF_PRESETS; i++, j++) {
		if (presets[i].switched_on) fputc ('1', config_file);
		else fputc ('0', config_file);

		fprintf (config_file, " Preset %d: %s\n", j, gtk_button_get_label (GTK_BUTTON (presets[i].button)));

		for (k = 0, l = 1; k < NB_OF_HYPERDECKS; k++, l++)
			fprintf (config_file, "HyperDeck %d: slot: %d, clip name: %s\n", l, presets[i].clips[k].slot, presets[i].clips[k].name);
	}

	fprintf (config_file, "Transitions:\ntype: %d, direction: %d, inv: %d, stripe_color:%lf %lf %lf, stripe_width: %d, nb_shutters: %d, duration: %d\n", transition_type, transition_direction, (int)transition_return_inv, transition_stripe_color.red, transition_stripe_color.green, transition_stripe_color.blue, transition_stripe_width, transition_nb_shutters, transition_nb_frames);

	for (i = 0; i < NB_OF_TRANSITIONS; i++) {
		if (transitions[i].switched_on) fputc ('1', config_file);
		else fputc ('0', config_file);
		fputc (' ', config_file);
		fprintf (config_file, "%s\n", transitions[i].suffix);
		fprintf (config_file, "%s\n", transitions[i].file_name);
	}

	fclose (config_file);
}
