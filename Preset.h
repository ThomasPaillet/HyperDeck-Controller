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

#ifndef __PRESET_H
#define __PRESET_H


#include "HyperDeck.h"


#define NB_OF_PRESETS 12
#define PRESETS_NAME_LENGTH 12


typedef struct preset_clip_s {
	hyperdeck_t *hyperdeck;
	int slot;
	char *name;

//config_presets_window
	GtkWidget *radio_button_slot_1, *radio_button_slot_2;
	GtkWidget *combo_box_text;
} preset_clip_t;

typedef struct preset_s {
	GtkWidget *button;
	gboolean switched_on;

	preset_clip_t clips[NB_OF_HYPERDECKS];

//config_presets_window
	GtkWidget *on_off;
	GtkEntryBuffer *entry_buffer;
} preset_t;


extern char *default_presets_name[NB_OF_PRESETS];

extern preset_t presets[NB_OF_PRESETS];

extern preset_t *preset_to_load;


void load_preset (void);

GtkWidget *create_presets_frame (void);

void init_presets (void);


#endif

