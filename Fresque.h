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

#ifndef __FRESQUE_H
#define __FRESQUE_H


#include "Fresque_Batch.h"


typedef struct fresque_s {
	const gchar *name;
	GtkWidget *list_box_row;

	int clips_id[NB_OF_HYPERDECKS];

	fresque_batch_t *parent_fresque_batch;

	struct fresque_s *next;
} fresque_t;


extern fresque_t *fresques;

extern fresque_t *current_fresque;

extern GtkWidget *fresques_list_box;

extern int fresques_list_box_num;

extern GtkWidget *fresques_stop_button, *fresques_play_button, *fresques_loop_button;


void clean_fresques (void);

void deselect_fresque (void);

void load_fresque (GtkListBox *list_box, GtkListBoxRow *list_box_row);

GtkWidget *create_fresques_frame (void);


#endif

