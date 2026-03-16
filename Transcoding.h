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

#ifndef __TRANSCODING_H
#define __TRANSCODING_H


#include "HyperDeck.h"


typedef struct remuxing_frame_s {
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *progress_bar;
	int64_t nb_frames;
	int64_t frame_count;
	guint g_source_id;
} remuxing_frame_t;

typedef struct transcoding_frame_s {
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


#endif

