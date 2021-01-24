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

#include "HyperDeck.h"


gboolean g_source_hide_widget (GtkWidget *widget)
{
	gtk_widget_hide (widget);

	return G_SOURCE_REMOVE;
}

gboolean g_source_show_widget (GtkWidget *widget)
{
	gtk_widget_show (widget);

	return G_SOURCE_REMOVE;
}

gboolean g_source_label_set_text (g_source_label_t *source_label)
{
	gtk_label_set_text (GTK_LABEL (source_label->label), source_label->text);
	g_free (source_label);

	return G_SOURCE_REMOVE;
}

gboolean g_source_init_progress_bar (GtkWidget *progress_bar)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0.0);

	return G_SOURCE_CONTINUE;
}

gboolean g_source_update_remuxing_progress_bar (remuxing_frame_t *remuxing_frame)
{
	if (remuxing_frame->nb_frames != 0) gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (remuxing_frame->progress_bar), (double)remuxing_frame->frame_count / (double)remuxing_frame->nb_frames);
	else gtk_progress_bar_pulse (GTK_PROGRESS_BAR (remuxing_frame->progress_bar));

	return G_SOURCE_CONTINUE;
}

gboolean g_source_init_remuxing_progress_bar (remuxing_frame_t *remuxing_frame)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (remuxing_frame->progress_bar), 0.0);

	return G_SOURCE_REMOVE;
}

gboolean g_source_end_remuxing_progress_bar (remuxing_frame_t *remuxing_frame)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (remuxing_frame->progress_bar), 1.0);

	return G_SOURCE_REMOVE;
}

gboolean g_source_hide_remuxing_progress_bar (gpointer index)
{
	gtk_widget_hide (remuxing_frames[GPOINTER_TO_INT (index)].frame);

	remuxing_frames[GPOINTER_TO_INT (index)].g_source_id  = 0;

	return G_SOURCE_REMOVE;
}

gboolean g_source_update_transcoding_progress_bar (transcoding_frame_t *transcoding_frame)
{
	if (transcoding_frame->frame_count <= transcoding_frame->nb_frames) {
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (transcoding_frame->progress_bar), (gdouble)transcoding_frame->frame_count / (gdouble)transcoding_frame->nb_frames);
	} else gtk_progress_bar_pulse (GTK_PROGRESS_BAR (transcoding_frame->progress_bar));

	if (transcoding_frame->frame_count == transcoding_frame->nb_frames) return G_SOURCE_REMOVE;
	else return G_SOURCE_CONTINUE;
}

gboolean g_source_init_transcoding_progress_bar (transcoding_frame_t *transcoding_frame)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (transcoding_frame->progress_bar), 0.0);

	if (nb_lines <= 1080) g_timeout_add (500, (GSourceFunc)g_source_update_transcoding_progress_bar, transcoding_frame);
	else g_timeout_add (1000, (GSourceFunc)g_source_update_transcoding_progress_bar, transcoding_frame);

	return G_SOURCE_REMOVE;
}

gboolean g_source_hide_transcoding_progress_bar (gpointer index)
{
	gtk_widget_hide (transcoding_frames[GPOINTER_TO_INT (index)].frame);

	transcoding_frames[GPOINTER_TO_INT (index)].frame_count = transcoding_frames[GPOINTER_TO_INT (index)].nb_frames;
	transcoding_frames[GPOINTER_TO_INT (index)].g_source_id  = 0;

	return G_SOURCE_REMOVE;
}

gboolean g_source_update_hyperdeck_progress_bar (hyperdeck_t* hyperdeck)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (hyperdeck->progress_bar), hyperdeck->progress_bar_fraction);

	if (hyperdeck->progress_bar_fraction == 1.0) return G_SOURCE_REMOVE;
	else return G_SOURCE_CONTINUE;
}

gboolean g_source_init_hyperdeck_progress_bar (hyperdeck_t* hyperdeck)
{
	hyperdeck->progress_bar_fraction = 0.0;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (hyperdeck->progress_bar), 0.0);
	gtk_widget_show (hyperdeck->progress_bar);

	g_timeout_add (500, (GSourceFunc)g_source_update_hyperdeck_progress_bar, hyperdeck);

	return G_SOURCE_REMOVE;
}

gboolean g_source_hide_hyperdeck_progress_bar (hyperdeck_t* hyperdeck)
{
	hyperdeck->progress_bar_fraction = 1.0;
	gtk_widget_hide (hyperdeck->progress_bar);

	return G_SOURCE_REMOVE;
}

gboolean g_source_consume_thread (GThread *thread)
{
DEBUG_S("g_source_consume_thread")
	g_thread_join (thread);
//	*thread = NULL;

	return G_SOURCE_REMOVE;
}

void show_message_window (const gchar* message)
{
	GtkWidget *window, *box1, *box2, *widget;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (window), "Attention");
	gtk_window_set_default_size (GTK_WINDOW (window), 50, 50);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (box1, 5);
	gtk_widget_set_margin_end (box1, 5);
	gtk_widget_set_margin_bottom (box1, 5);
		widget = gtk_label_new (message);
		gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			widget = gtk_button_new_with_label ("Fermer");
			gtk_widget_set_margin_top (widget, 5);
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_set_center_widget (GTK_BOX (box2), widget);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
}

void save_hyperdeck_state (void)
{
	FILE *state_file;
	int i, j;
	fresque_batch_t *fresque_batch_tmp;

	state_file = fopen ("state.dat", "wb");
DEBUG_S("save_hyperdeck_state")

	for (i = 0; i < NB_OF_HYPERDECKS; i++) fwrite (&hyperdecks[i].default_preset_clip_id, sizeof (int), 1, state_file);

	if (feof (state_file)) { fclose (state_file); return; }

	for (fresque_batch_tmp = fresque_batches; fresque_batch_tmp != NULL; fresque_batch_tmp = fresque_batch_tmp->next) {
		fresque_batch_tmp->index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (fresque_batch_tmp->list_box_row));
DEBUG_D (fresque_batch_tmp->index)
	}
DEBUG_S ("fresques_list_box_num")
DEBUG_D (fresques_list_box_num)
	for (i = 0; i < fresques_list_box_num; i++) {
		for (fresque_batch_tmp = fresque_batches; fresque_batch_tmp != NULL; fresque_batch_tmp = fresque_batch_tmp->next) {
			if (fresque_batch_tmp->index == i) {
DEBUG_S(fresque_batch_tmp->name)
				fwrite (&fresque_batch_tmp->name_len, sizeof (int), 1, state_file);
				fwrite (fresque_batch_tmp->name, 1, fresque_batch_tmp->name_len, state_file);

				fwrite (&fresque_batch_tmp->fresque_frame->format, sizeof (int), 1, state_file);
				fwrite (&fresque_batch_tmp->fresque_frame->colorspace, sizeof (enum AVColorSpace), 1, state_file);
				fwrite (&fresque_batch_tmp->fresque_frame->width, sizeof (int), 1, state_file);
				fwrite (&fresque_batch_tmp->fresque_frame->height, sizeof (int), 1, state_file);
				for (j = 0; j < 3; j++) {
					fwrite (fresque_batch_tmp->fresque_frame->data[j], 1, fresque_batch_tmp->fresque_frame->linesize[j] * fresque_batch_tmp->fresque_frame->height, state_file);
				}
				fwrite (&fresque_batch_tmp->nb_flux, sizeof (int), 1, state_file);
				fwrite (&fresque_batch_tmp->first_hyperdeck_number, sizeof (int), 1, state_file);
				break;
			}
		}
	}
DEBUG_S("save_hyperdeck_state END")

	fclose (state_file);
}

void restore_hyperdeck_state (void)
{
	FILE *state_file;
	int i;
	fresque_batch_t *fresque_batch_tmp;

	if ((state_file = fopen ("state.dat", "rb")) == NULL) return;
DEBUG_S("restore_hyperdeck_state")

	for (i = 0; i < NB_OF_HYPERDECKS; i++) fread (&hyperdecks[i].default_preset_clip_id, sizeof (int), 1, state_file);

	while (TRUE) {
		fresque_batch_tmp = g_malloc (sizeof (fresque_batch_t));
		fread (&fresque_batch_tmp->name_len, sizeof (int), 1, state_file);
		if (feof (state_file)) { g_free (fresque_batch_tmp); break; }
		fread (fresque_batch_tmp->name, 1, fresque_batch_tmp->name_len, state_file);
		fresque_batch_tmp->name[fresque_batch_tmp->name_len] = '\0';

		fresque_batch_tmp->fresque_frame = av_frame_alloc ();
		fread (&fresque_batch_tmp->fresque_frame->format, sizeof (int), 1, state_file);
		fread (&fresque_batch_tmp->fresque_frame->colorspace, sizeof (enum AVColorSpace), 1, state_file);
		fread (&fresque_batch_tmp->fresque_frame->width, sizeof (int), 1, state_file);
		fread (&fresque_batch_tmp->fresque_frame->height, sizeof (int), 1, state_file);
		fresque_batch_tmp->fresque_frame->pict_type = AV_PICTURE_TYPE_I;
		av_frame_get_buffer (fresque_batch_tmp->fresque_frame, 0);

		for (i = 0; i < 3; i++) {
			fread (fresque_batch_tmp->fresque_frame->data[i], 1, fresque_batch_tmp->fresque_frame->linesize[i] * fresque_batch_tmp->fresque_frame->height, state_file);
		}
		fread (&fresque_batch_tmp->nb_flux, sizeof (int), 1, state_file);
		fread (&fresque_batch_tmp->first_hyperdeck_number, sizeof (int), 1, state_file);

		initialize_fresque_batch (fresque_batch_tmp);

		fresque_batch_tmp->next = fresque_batches;
		fresque_batches = fresque_batch_tmp;
	}

	fclose (state_file);

	state_file = fopen ("state.dat", "wb");
	fclose (state_file);
}

