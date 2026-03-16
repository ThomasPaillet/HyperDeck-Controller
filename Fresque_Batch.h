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

#ifndef __FRESQUE_BATCH_H
#define __FRESQUE_BATCH_H


#include "HyperDeck.h"

#include <libavutil/frame.h>


typedef struct fresque_batch_s {
	gboolean initialized;

	char name[CLIP_NAME_LENGTH];
	int name_len;
	GtkWidget *list_box_row;
	gint index;

	GtkWidget *list_box;
	int nb_fresques;

	AVFrame *fresque_frame;
	int nb_flux;
	int first_hyperdeck_number;

	struct fresque_batch_s *next;
} fresque_batch_t;


extern fresque_batch_t *fresque_batches;

extern GMutex fresque_batch_mutex;

extern GtkWidget *add_transition_frame;


void initialize_fresque_batch (fresque_batch_t *fresque_batch);

void create_add_transition_frame (GtkBox *box);

void init_fresque_batch (void);


#endif

