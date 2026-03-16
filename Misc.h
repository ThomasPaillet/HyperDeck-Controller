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

#ifndef __MISC_H
#define __MISC_H


#include "HyperDeck.h"

#include "Transcoding.h"


typedef struct g_source_label_s {
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


#endif

