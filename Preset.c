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

#include "HyperDeck.h"
#include "HyperDeck_Protocol.h"


char* default_presets_name[NB_OF_PRESETS] = {"Preset 1", "Preset 2", "Preset 3", "Preset 4", "Preset 5", "Preset 6", "Preset 7", "Preset 8"};

preset_t presets[NB_OF_PRESETS];

preset_t *preset_to_load = NULL;

int preset_to_load_count = 0;


void load_preset (void)
{
	int i;
	clip_list_t *clip_list_tmp;
	int clip_ids[NB_OF_HYPERDECKS];
	char msg_goto_clip_id_x[NB_OF_HYPERDECKS][32];
	int msg_len[NB_OF_HYPERDECKS];
	gboolean preset_exist[NB_OF_HYPERDECKS];

	if (preset_to_load_count > 1) {
		preset_to_load_count--;
		return;
	} else preset_to_load_count = 0;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		clip_ids[i] = 0;
		preset_exist[i] = FALSE;

		if ((!hyperdecks[i].connected) || (preset_to_load->clips[i].slot == 0)) continue;

		for (clip_list_tmp = hyperdecks[i].list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
			if (strcmp (preset_to_load->clips[i].name, clip_list_tmp->name) == 0) {
				clip_ids[i] = clip_list_tmp->id;
				break;
			}
		}

		if (clip_ids[i] != 0) {
			preset_exist[i] = TRUE;

			send (hyperdecks[i].socket, msg_stop, 5, 0);

			msg_len[i] = sprintf (msg_goto_clip_id_x[i], "%s%d\n", msg_goto_clip_id_, clip_ids[i]);

			hyperdecks[i].default_preset_clip_id = clip_ids[i];

			if (hyperdecks[i].loop != SINGLE_CLIP_TRUE_LOOP_TRUE) {
				hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
			}
		} else hyperdecks[i].default_preset_clip_id = 0;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (preset_exist[i]) send (hyperdecks[i].socket, msg_goto_clip_id_x[i], msg_len[i], 0);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (preset_exist[i]) send (hyperdecks[i].socket, msg_play_single_clip_true_loop_true, 35, 0);
	}

	preset_to_load = NULL;
}

void preset_button_clicked (GtkButton *button, preset_t *preset)
{
	int i;

	deselect_fresque ();

	preset_to_load = NULL;
	preset_to_load_count = 0;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((!hyperdecks[i].connected) || (preset->clips[i].slot == 0)) continue;

		if ((preset->clips[i].slot == 1) && (hyperdecks[i].slot_selected != 1) && (hyperdecks[i].slot_1_is_mounted)) {
			send (hyperdecks[i].socket, msg_select_slot_id_1, 24, 0);
			preset_to_load = preset;
			preset_to_load_count++;
		} else if ((preset->clips[i].slot == 2) && (hyperdecks[i].slot_selected != 2) && (hyperdecks[i].slot_2_is_mounted)) {
			send (hyperdecks[i].socket, msg_select_slot_id_2, 24, 0);
			preset_to_load = preset;
			preset_to_load_count++;
		}
	}

	if (preset_to_load == NULL) {
		preset_to_load = preset;
		preset_to_load_count = 1;
		load_preset ();
	} else SLEEP (2);
}

GtkWidget *create_presets_frame (void)
{
	int i;

	GtkWidget *frame, *box1, *box2;

	frame = gtk_frame_new ("Presets");
	gtk_frame_set_label_align (GTK_FRAME (frame), 0.024, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous (GTK_BOX (box1), TRUE);
	gtk_box_set_spacing (GTK_BOX (box1), 2);
	gtk_widget_set_margin_start (box1, 5);
	gtk_widget_set_margin_end (box1, 5);
	gtk_widget_set_margin_bottom (box1, 5);
		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
		gtk_box_set_spacing (GTK_BOX (box2), 2);
		for (i = 0; i < 4; i++) {
			gtk_widget_set_sensitive (presets[i].button, presets[i].switched_on);
			gtk_box_pack_start (GTK_BOX (box2), presets[i].button, TRUE, TRUE, 0);
		}
		gtk_container_add (GTK_CONTAINER (box1), box2);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
		gtk_box_set_spacing (GTK_BOX (box2), 2);
		for (i = 4; i < NB_OF_PRESETS; i++) {
			gtk_widget_set_sensitive (presets[i].button, presets[i].switched_on);
			gtk_box_pack_start (GTK_BOX (box2), presets[i].button, TRUE, TRUE, 0);
		}
		gtk_container_add (GTK_CONTAINER (box1), box2);
	gtk_container_add (GTK_CONTAINER (frame), box1);

	return frame;
}

void init_presets (void)
{
	int i, j;

	for (i = 0; i < NB_OF_PRESETS; i++) {
		presets[i].switched_on = FALSE;
		presets[i].button = gtk_button_new_with_label (default_presets_name[i]);
		g_signal_connect (presets[i].button, "clicked", G_CALLBACK (preset_button_clicked), &presets[i]);

		for (j = 0; j < NB_OF_HYPERDECKS; j++) {
			presets[i].clips[j].hyperdeck = &hyperdecks[j];
			presets[i].clips[j].slot = 1;
			presets[i].clips[j].name = g_malloc (CLIP_NAME_LENGTH);
			presets[i].clips[j].name[0] = '\0';
		}
	}
}

