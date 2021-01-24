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


fresque_t *fresques = NULL;

fresque_t *current_fresque = NULL;

GtkWidget *fresques_list_box;

int fresques_list_box_num = 0;

GtkWidget *fresques_stop_button, *fresques_play_button, *fresques_loop_button, *fresques_up_button, *fresques_down_button, *fresques_purge_button;

loop_t fresque_loop = SINGLE_CLIP_TRUE_LOOP_FALSE;


void clean_fresques (void)
{
	fresque_t *fresque_prev, *fresque_tmp;
	fresque_batch_t *fresque_batch_prev, *fresque_batch_tmp;
	int i, j;

	fresque_prev = NULL;
	fresque_tmp = fresques;

	while (fresque_tmp != NULL) {
		for (i = 0, j = 0; i < NB_OF_HYPERDECKS; i++)
			j += fresque_tmp->clips_id[i];

		if (j == 0) {
			gtk_widget_destroy (fresque_tmp->list_box_row);

			if (fresque_tmp->parent_fresque_batch != NULL) {
				fresque_tmp->parent_fresque_batch->nb_fresques--;

				if (fresque_tmp->parent_fresque_batch->nb_fresques == 0) {
					fresques_list_box_num--;
g_mutex_lock (&fresque_batch_mutex);
					if (fresque_tmp->parent_fresque_batch == fresque_batches) {
						fresque_batches = fresque_tmp->parent_fresque_batch->next;
						gtk_widget_destroy (fresque_tmp->parent_fresque_batch->list_box_row);

						av_frame_unref (fresque_tmp->parent_fresque_batch->fresque_frame);
						av_frame_free (&fresque_tmp->parent_fresque_batch->fresque_frame);

						g_free (fresque_tmp->parent_fresque_batch);
					} else {
						fresque_batch_prev = fresque_batches;
						fresque_batch_tmp = fresque_batches->next;

						while (fresque_batch_tmp != NULL) {
							if (fresque_batch_tmp == fresque_tmp->parent_fresque_batch) {
								fresque_batch_prev->next = fresque_batch_tmp->next;
								gtk_widget_destroy (fresque_batch_tmp->list_box_row);

								av_frame_unref (fresque_batch_tmp->fresque_frame);
								av_frame_free (&fresque_batch_tmp->fresque_frame);

								g_free (fresque_batch_tmp);
								break;
							}

							fresque_batch_prev = fresque_batch_tmp;
							fresque_batch_tmp = fresque_batch_tmp->next;
						}
					}
g_mutex_unlock (&fresque_batch_mutex);
				}
			} else fresques_list_box_num--;

			if (fresque_prev == NULL) {
				fresques = fresque_tmp->next;
				g_free (fresque_tmp);
				fresque_tmp = fresques;
			} else {
				fresque_prev->next = fresque_tmp->next;
				g_free (fresque_tmp);
				fresque_tmp = fresque_prev->next;
			}
		} else {
			fresque_prev = fresque_tmp;
			fresque_tmp = fresque_tmp->next;
		}
	}
}

void deselect_fresque (void)
{
	if (current_fresque != NULL) {
		if (current_fresque->parent_fresque_batch == NULL) gtk_list_box_select_row (GTK_LIST_BOX (fresques_list_box), NULL);
		else gtk_list_box_select_row (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), NULL);
		current_fresque = NULL;
	}
	gtk_widget_set_sensitive (fresques_loop_button, FALSE);
	gtk_widget_set_sensitive (fresques_play_button, FALSE);
	gtk_widget_set_sensitive (fresques_stop_button, FALSE);

	gtk_widget_set_sensitive (fresques_up_button, FALSE);
	gtk_widget_set_sensitive (fresques_down_button, FALSE);
}

void load_fresque (GtkListBox *list_box, GtkListBoxRow *list_box_row)
{
	GtkWidget *widget;
	const gchar *name;
	int i;
	int clips_id[NB_OF_HYPERDECKS];
	char msg_goto_clip_id_x[NB_OF_HYPERDECKS][32];
	int msg_len[NB_OF_HYPERDECKS];
	gboolean fresque_exist[NB_OF_HYPERDECKS];
	gboolean preset_exist[NB_OF_HYPERDECKS];

	if (list_box_row == NULL) return;

	widget = gtk_bin_get_child (GTK_BIN (list_box_row));

	if (current_fresque != NULL) {
		if (current_fresque->parent_fresque_batch != NULL)
			gtk_list_box_unselect_row (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));
		else gtk_list_box_unselect_row (GTK_LIST_BOX (fresques_list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));
	}

	name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (widget))));

	for (current_fresque = fresques; current_fresque != NULL; current_fresque = current_fresque->next)
		if (strcmp (current_fresque->name, name) == 0) break;

	if (current_fresque == NULL) return;

	memcpy (clips_id, current_fresque->clips_id, sizeof (int) * NB_OF_HYPERDECKS);

	gtk_widget_set_sensitive (fresques_loop_button, TRUE);
	gtk_widget_set_sensitive (fresques_play_button, TRUE);
	gtk_widget_set_sensitive (fresques_stop_button, TRUE);

	if (current_fresque->parent_fresque_batch != NULL) i = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->parent_fresque_batch->list_box_row));
	else i = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->list_box_row));

	if (i > 0) gtk_widget_set_sensitive (fresques_up_button, TRUE);
	else gtk_widget_set_sensitive (fresques_up_button, FALSE);

	if (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), i + 1) != NULL) gtk_widget_set_sensitive (fresques_down_button, TRUE);
	else gtk_widget_set_sensitive (fresques_down_button, FALSE);

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		fresque_exist[i] = FALSE;
		preset_exist[i] = FALSE;

		if (!hyperdecks[i].connected) continue;

		if (clips_id[i] == 0) {
			clips_id[i] = hyperdecks[i].default_preset_clip_id;
			if (clips_id[i] == 0) continue;
			else preset_exist[i] = TRUE;
		} else {
			fresque_exist[i] = TRUE;
			send (hyperdecks[i].socket, msg_stop, 5, 0);
		}

		msg_len[i] = sprintf (msg_goto_clip_id_x[i], "%s%d\n", msg_goto_clip_id_, clips_id[i]);

		if (preset_exist[i]) {
			if (hyperdecks[i].loop != SINGLE_CLIP_TRUE_LOOP_TRUE) {
				hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
			}
		} else if (hyperdecks[i].loop != fresque_loop) {
			hyperdecks[i].loop = fresque_loop;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[fresque_loop]);
		}
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((fresque_exist[i]) || (preset_exist[i])) send (hyperdecks[i].socket, msg_goto_clip_id_x[i], msg_len[i], 0);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (preset_exist[i]) send (hyperdecks[i].socket, msg_play_single_clip_true_loop_true, 35, 0);
	}
}

void fresques_loop_button_clicked (GtkToggleButton *button)
{
	int i;

	if (current_fresque == NULL) return;

	if (gtk_toggle_button_get_active (button)) fresque_loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
	else fresque_loop = SINGLE_CLIP_TRUE_LOOP_FALSE;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((!hyperdecks[i].connected) || (hyperdecks[i].slot_selected == 0)) continue;
		if (current_fresque->clips_id[i] == 0) continue;

		if (hyperdecks[i].loop == fresque_loop) continue;

		if (fresque_loop == SINGLE_CLIP_TRUE_LOOP_TRUE) {
			if (hyperdecks[i].play) SEND ((&hyperdecks[i]), msg_play_single_clip_true_loop_true)
			hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
		} else {
			if (hyperdecks[i].play) SEND ((&hyperdecks[i]), msg_play_single_clip_true_loop_false)
			hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_FALSE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_FALSE]);
		}
	}
}

void fresques_play_button_clicked (void)
{
	int i;
	gboolean fresque_exist[NB_OF_HYPERDECKS];

	if (current_fresque == NULL) return;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		fresque_exist[i] = FALSE;

		if ((!hyperdecks[i].connected) || (hyperdecks[i].slot_selected == 0) || (current_fresque->clips_id[i] == 0)) continue;

		if (hyperdecks[i].loop != fresque_loop) {
			hyperdecks[i].loop = fresque_loop;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[fresque_loop]);
		}
		fresque_exist[i] = TRUE;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (fresque_exist[i]) send (hyperdecks[i].socket, msg_play_single_loop[fresque_loop], msg_play_single_loop_len[fresque_loop], 0);
	}
}

void fresques_stop_button_clicked (void)
{
	int i;
	gboolean fresque_exist[NB_OF_HYPERDECKS];

	if (current_fresque == NULL) return;

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((!hyperdecks[i].connected) || (hyperdecks[i].slot_selected == 0) || (current_fresque->clips_id[i] == 0)) fresque_exist[i] = FALSE;
		else fresque_exist[i] = TRUE;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (fresque_exist[i]) send (hyperdecks[i].socket, msg_stop, 5, 0);
	}
}

void purge_all_hyperdecks (GtkWidget *window)
{
	int i, msg_len[NB_OF_HYPERDECKS];
	char msg_goto_clip_id_x[NB_OF_HYPERDECKS][32];
	gboolean preset_exist[NB_OF_HYPERDECKS];
	GThread *purge_thread[NB_OF_HYPERDECKS];

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((hyperdecks[i].connected) && (hyperdecks[i].default_preset_clip_id != 0)) {
			send (hyperdecks[i].socket, msg_stop, 5, 0);

			msg_len[i] = sprintf (msg_goto_clip_id_x[i], "%s%d\n", msg_goto_clip_id_, hyperdecks[i].default_preset_clip_id);

			if (hyperdecks[i].loop != SINGLE_CLIP_TRUE_LOOP_TRUE) {
				hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
				gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdecks[i].image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
			}

			preset_exist[i] = TRUE;
		} else preset_exist[i] = FALSE;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (preset_exist[i]) send (hyperdecks[i].socket, msg_goto_clip_id_x[i], msg_len[i], 0);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if ((hyperdecks[i].connected) && (hyperdecks[i].slot_selected != 0))
			purge_thread[i] = g_thread_new (NULL, (GThreadFunc)purge_hyperdeck, &hyperdecks[i]);
		else purge_thread[i] = NULL;
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (purge_thread[i] != NULL) g_thread_join (purge_thread[i]);
	}

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (preset_exist[i]) send (hyperdecks[i].socket, msg_play_single_clip_true_loop_true, 35, 0);
	}

	gtk_widget_destroy (window);
}

void show_fresques_purge_confirmation_window (void)
{
	GtkWidget *window, *box1, *box2, *widget;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (window), "Attention !");
	gtk_window_set_default_size (GTK_WINDOW (window), 10, 10);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box1), 5);
		widget = gtk_label_new ("Etes-vous sûr de vouloir supprimer l'ensemble des clips contenus dans");
		gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("tous les boitiers HyperDeck, sauf ceux nécessaires aux \"Presets\" ?");
		gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_margin_top (box2, 5);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_box_set_spacing (GTK_BOX (box2), 5);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			widget = gtk_button_new_with_label ("Oui");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (purge_all_hyperdecks), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, TRUE, TRUE, 0);

			widget = gtk_button_new_with_label ("Non");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
}

void fresques_up_button_clicked (void)
{
	int index;

	if (current_fresque->parent_fresque_batch != NULL)  {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->parent_fresque_batch->list_box_row));
		g_object_ref (current_fresque->parent_fresque_batch->list_box_row);
		gtk_container_remove (GTK_CONTAINER (fresques_list_box), current_fresque->parent_fresque_batch->list_box_row);
		gtk_list_box_insert (GTK_LIST_BOX (fresques_list_box), current_fresque->parent_fresque_batch->list_box_row, index - 1);
		g_object_unref (current_fresque->parent_fresque_batch->list_box_row);
	} else {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->list_box_row));
		g_object_ref (current_fresque->list_box_row);
		gtk_container_remove (GTK_CONTAINER (fresques_list_box), current_fresque->list_box_row);
		gtk_list_box_insert (GTK_LIST_BOX (fresques_list_box), current_fresque->list_box_row, index - 1);
		g_object_unref (current_fresque->list_box_row);
	}

	if (index == 1) gtk_widget_set_sensitive (fresques_up_button, FALSE);
	gtk_widget_set_sensitive (fresques_down_button, TRUE);
}

void fresques_down_button_clicked (void)
{
	int index;

	if (current_fresque->parent_fresque_batch != NULL)  {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->parent_fresque_batch->list_box_row));
		g_object_ref (current_fresque->parent_fresque_batch->list_box_row);
		gtk_container_remove (GTK_CONTAINER (fresques_list_box), current_fresque->parent_fresque_batch->list_box_row);
		gtk_list_box_insert (GTK_LIST_BOX (fresques_list_box), current_fresque->parent_fresque_batch->list_box_row, index + 1);
		g_object_unref (current_fresque->parent_fresque_batch->list_box_row);
	} else {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->list_box_row));
		g_object_ref (current_fresque->list_box_row);
		gtk_container_remove (GTK_CONTAINER (fresques_list_box), current_fresque->list_box_row);
		gtk_list_box_insert (GTK_LIST_BOX (fresques_list_box), current_fresque->list_box_row, index + 1);
		g_object_unref (current_fresque->list_box_row);
	}

	gtk_widget_set_sensitive (fresques_up_button, TRUE);
	if (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index + 2) == NULL) gtk_widget_set_sensitive (fresques_down_button, FALSE);
}

GtkWidget *create_fresques_frame (void)
{
	GtkWidget *fresques_frame;
	GtkWidget *box1, *box2, *box3;
	GtkWidget *scrolled_window;

	fresques_frame = gtk_frame_new ("Fresques");
	gtk_frame_set_label_align (GTK_FRAME (fresques_frame), 0.024, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (fresques_frame), 5);

	box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_start (box1, 5);
	gtk_widget_set_margin_end (box1, 5);
	gtk_widget_set_margin_bottom (box1, 5);
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
			fresques_list_box = gtk_list_box_new ();
			g_signal_connect (G_OBJECT (fresques_list_box), "row-activated", G_CALLBACK (load_fresque), NULL);
			gtk_container_add (GTK_CONTAINER (scrolled_window), fresques_list_box);
		gtk_box_pack_start (GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_margin_start (box2, 5);
			fresques_loop_button = gtk_toggle_button_new_with_label ("Loop");
			gtk_widget_set_margin_bottom (fresques_loop_button, 2);
			gtk_widget_set_sensitive (fresques_loop_button, FALSE);
			g_signal_connect (G_OBJECT (fresques_loop_button), "toggled", G_CALLBACK (fresques_loop_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box2), fresques_loop_button, FALSE, FALSE, 0);

			fresques_play_button = gtk_button_new_with_label ("Play");
			gtk_widget_set_margin_bottom (fresques_play_button, 2);
			gtk_widget_set_sensitive (fresques_play_button, FALSE);
			g_signal_connect (G_OBJECT (fresques_play_button), "clicked", G_CALLBACK (fresques_play_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box2), fresques_play_button, FALSE, FALSE, 0);

			fresques_stop_button = gtk_button_new_with_label ("Stop");
			gtk_widget_set_sensitive (fresques_stop_button, FALSE);
			g_signal_connect (G_OBJECT (fresques_stop_button), "clicked", G_CALLBACK (fresques_stop_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box2), fresques_stop_button, FALSE, FALSE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
				fresques_up_button = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (fresques_up_button), gtk_image_new_from_pixbuf (pixbuf_Up));
				gtk_widget_set_margin_bottom (fresques_up_button, 2);
				gtk_widget_set_sensitive (fresques_up_button, FALSE);
				g_signal_connect (G_OBJECT (fresques_up_button), "clicked", G_CALLBACK (fresques_up_button_clicked), NULL);
				gtk_box_pack_start (GTK_BOX (box3), fresques_up_button, FALSE, FALSE, 0);

				fresques_down_button = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (fresques_down_button), gtk_image_new_from_pixbuf (pixbuf_Down));
				gtk_widget_set_sensitive (fresques_down_button, FALSE);
				g_signal_connect (G_OBJECT (fresques_down_button), "clicked", G_CALLBACK (fresques_down_button_clicked), NULL);
				gtk_box_pack_start (GTK_BOX (box3), fresques_down_button, FALSE, FALSE, 0);
			gtk_box_set_center_widget (GTK_BOX (box2), box3);

			fresques_purge_button = gtk_button_new_with_label ("Purge");
			g_signal_connect (G_OBJECT (fresques_purge_button), "clicked", G_CALLBACK (show_fresques_purge_confirmation_window), NULL);
			gtk_box_pack_end (GTK_BOX (box2), fresques_purge_button, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (fresques_frame), box1);

	return fresques_frame;
}

