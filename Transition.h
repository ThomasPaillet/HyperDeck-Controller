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

#ifndef __TRANSITION_H
#define __TRANSITION_H


#include "Preset.h"


#define NB_OF_TRANSITIONS NB_OF_PRESETS
#define TRANSITION_SUFFIX_LENGTH 9


typedef struct transition_s {
	gboolean switched_on;
	char suffix[TRANSITION_SUFFIX_LENGTH];
	char *file_name;

//config_transitions_window
	GtkWidget *on_off;
	GtkEntryBuffer *entry_buffer;
	GtkWidget *combo_box_text;

	GThread *thread[NB_OF_HYPERDECKS];
} transition_t;

typedef struct transition_task_s {
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
extern uint8_t stripe_color_Y_8, stripe_color_U_8, stripe_color_V_8;
extern uint16_t stripe_color_Y_16, stripe_color_U_16, stripe_color_V_16;
extern int transition_stripe_width;
extern int transition_nb_shutters;
extern int transition_nb_frames;
extern transition_t transitions[NB_OF_TRANSITIONS];

extern GSList *background_slist;
extern int background_slist_length;


void fill_background_slist (void);

gpointer run_transition_task (transition_task_t *transition_task);

void init_transitions (void);


#endif

