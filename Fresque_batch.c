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
#include <time.h>

#include "HyperDeck.h"


typedef struct add_transition_task_s {
	fresque_batch_t *previous_fresque_batch;
	fresque_batch_t *current_fresque_batch;
	int nb_flux;

	struct add_transition_task_s *next;
} add_transition_task_t;


GtkWidget *fresque_batch_menu;

GtkWidget *add_transition_frame;
GtkWidget *add_transition_label;
GtkWidget *add_transition_progress_bar;
/*int64_t nb_frames;
int64_t frame_count;*/
guint add_transition_g_source_id = 0;

fresque_batch_t *fresque_batches = NULL;

GMutex fresque_batch_mutex;

fresque_batch_t *current_fresque_batch = NULL;
fresque_batch_t *previous_fresque_batch = NULL;

add_transition_task_t *add_transition_tasks = NULL;

GThread *add_transition_thread = NULL;

GMutex add_transition_mutex;


gpointer add_transition (void)
{
	float *last_x;
	AVFrame *frame_tmp, *frame_out;
	struct SwsContext *sws_context;
	add_transition_task_t *add_transition_task_tmp;
	g_source_label_t *source_label;
	fresque_batch_t *background_batch = NULL;
	fresque_batch_t *fresque_batch = NULL;
	int nb_flux;
	float sin_minus_7, stride, step;
	time_t start_time, end_time;
	struct tm *tm;
	char creation_time[32];
	int i;
	hyperdeck_t* hyperdeck;
	drop_list_t *drop_list;

DEBUG_S ("add_transition")
	if (add_transition_g_source_id != 0) {
		g_source_remove (add_transition_g_source_id);
		add_transition_g_source_id = 0;
	}

	last_x = g_malloc (nb_lines * sizeof (float));

	frame_tmp = av_frame_alloc ();
	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) frame_tmp->format = AV_PIX_FMT_YUV444P;
	else frame_tmp->format = AV_PIX_FMT_YUV444P16LE;
	frame_tmp->width = hyperdeck_width;
	frame_tmp->height = nb_lines;
	frame_tmp->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_tmp, 0);

	frame_out = av_frame_alloc ();
	frame_out->format = hyperdeck_pix_fmt;
	frame_out->chroma_location = HYPERDECK_CHROMA_LOCATION;
	frame_out->key_frame = 1;
	frame_out->pict_type = AV_PICTURE_TYPE_I;
	if (progressif) {
		frame_out->interlaced_frame = 0;
		frame_out->top_field_first = 0;
	} else {
		frame_out->interlaced_frame = 1;
		frame_out->top_field_first = 1;
	}
	frame_out->color_primaries = hyperdeck_color_primaries;
	frame_out->color_trc = hyperdeck_color_trc;
	frame_out->colorspace = hyperdeck_colorspace;
	frame_out->color_range = AVCOL_RANGE_MPEG;	//the normal 219*2^(n-8) "MPEG" YUV ranges
	frame_out->width = hyperdeck_width;
	frame_out->height = nb_lines;
	frame_out->sample_aspect_ratio = hyperdeck_sample_aspect_ratio;
	av_frame_get_buffer (frame_out, 0);

	if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P)
		sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P, SWS_BILINEAR, NULL, NULL, NULL);
	else sws_context = sws_getContext (hyperdeck_width, nb_lines, AV_PIX_FMT_YUV444P16LE, hyperdeck_width, nb_lines, AV_PIX_FMT_YUV422P10LE, SWS_BILINEAR, NULL, NULL, NULL);

g_mutex_lock (&add_transition_mutex);
	while (add_transition_tasks != NULL) {
		add_transition_task_tmp = add_transition_tasks;
		add_transition_tasks = add_transition_tasks->next;
g_mutex_unlock (&add_transition_mutex);
		g_idle_add ((GSourceFunc)g_source_init_progress_bar, add_transition_progress_bar);

//g_timeout_add (1000, (GSourceFunc)g_source_update_transcoding_progress_bar, transcoding_frame);

		source_label = g_malloc (sizeof (g_source_label_t));
		sprintf (source_label->text, "%s --> %s", background_batch->name, fresque_batch->name);
		source_label->label = add_transition_label;
		g_idle_add ((GSourceFunc)g_source_label_set_text, source_label);

		g_idle_add ((GSourceFunc)g_source_show_widget, add_transition_frame);

		background_batch = add_transition_task_tmp->previous_fresque_batch;
		fresque_batch = add_transition_task_tmp->current_fresque_batch;
		nb_flux = add_transition_task_tmp->nb_flux;
		g_free (add_transition_task_tmp);

		if (nb_lines == 480) {
			if (transition_type == 0) {
				sin_minus_7 = -0.100626063;
				stride = (720 * nb_flux + (0.100626063 * 480)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.100626063;
				stride = (720 * nb_flux + (0.100626063 * 480)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 720 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		} else if (nb_lines == 576) {
			if (transition_type == 0) {
				sin_minus_7 = -0.085689382;
				stride = (720 * nb_flux + (0.085689382 * 576)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.085689382;
				stride = (720 * nb_flux + (0.085689382 * 576)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 720 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		} else if (nb_lines == 720) {
			if (transition_type == 0) {
				sin_minus_7 = -0.121869343;
				stride = (1280 * nb_flux + (0.121869343 * 720)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.121869343;
				stride = (1280 * nb_flux + (0.121869343 * 720)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 1280 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		} else if (nb_lines == 1080) {
			if (transition_type == 0) {
				sin_minus_7 = -0.121869343;
				stride = (1920 * nb_flux + (0.121869343 * 1080)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.121869343;
				stride = (1920 * nb_flux + (0.121869343 * 1080)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 1920 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		} else if (nb_lines == 2160) {
			if (transition_type == 0) {
				sin_minus_7 = -0.121869343;
				stride = (3840 * nb_flux + (0.121869343 * 2160)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.121869343;
				stride = (3840 * nb_flux + (0.121869343 * 2160)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 3840 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		} else if (nb_lines == 4320) {
			if (transition_type == 0) {
				sin_minus_7 = -0.121869343;
				stride = (7680 * nb_flux + (0.121869343 * 4320)) / (transition_nb_frames - 1);
			} else if (transition_type == 1) {
				sin_minus_7 = -0.121869343;
				stride = (7680 * nb_flux + (0.121869343 * 4320)) / (transition_nb_frames - 1);
			} else if (transition_type == 2) {
				stride = 7680 / transition_nb_shutters;
				step = stride / (float)(transition_nb_frames - 1);
			}
		}

		start_time = time (NULL);
		tm = localtime (&start_time);
		sprintf (creation_time, "%4d-%02d-%02dT%02d:%02d:%02d.000000Z", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

		for (i = 0, hyperdeck = hyperdecks + fresque_batch->first_hyperdeck_number; i < nb_flux; i++, hyperdeck++) {
			drop_list = g_malloc (sizeof (drop_list_t));
			drop_list->full_name = NULL;
			drop_list->file_name_in = NULL;
			memcpy (drop_list->file_name_out, fresque_batch->name, fresque_batch->name_len);
			drop_list->file_name_out[drop_list->file_name_out_len++] = '_';
			memcpy (drop_list->file_name_out + drop_list->file_name_out_len, background_batch->name, background_batch->name_len);
			drop_list->file_name_out[drop_list->file_name_out_len++] = (char)fresque_batch->first_hyperdeck_number + 49;
			complete_file_name_out (drop_list);

			if (hyperdeck_pix_fmt == AV_PIX_FMT_YUV422P) render_transition_8 (hyperdecks + fresque_batch->first_hyperdeck_number, hyperdeck, drop_list, background_batch->fresque_frame, fresque_batch->fresque_frame, nb_flux, last_x, sin_minus_7, stride, step, sws_context, frame_tmp, frame_out, creation_time, FALSE);
			else render_transition_16 (hyperdecks + fresque_batch->first_hyperdeck_number, hyperdeck, drop_list, background_batch->fresque_frame, fresque_batch->fresque_frame, nb_flux, last_x, sin_minus_7, stride, step, sws_context, frame_tmp, frame_out, creation_time, FALSE);
		}

g_mutex_lock (&add_transition_mutex);
	}
g_mutex_unlock (&add_transition_mutex);

	sws_freeContext (sws_context);
	av_frame_unref (frame_out);
	av_frame_free (&frame_out);
	av_frame_unref (frame_tmp);
	av_frame_free (&frame_tmp);
	g_free (last_x);

	end_time = time (NULL);
	end_time -= start_time;
	end_time -= 4;

/*	if (end_time < 0)
		add_transition_g_source_id = g_timeout_add (-end_time * 1000, g_source_hide_transcoding_progress_bar, NULL);
	else g_idle_add (g_source_hide_transcoding_progress_bar, NULL);*/

	g_idle_add ((GSourceFunc)g_source_consume_thread, add_transition_thread);
	add_transition_thread = NULL;
DEBUG_S ("add_transition END")
	return NULL;
}

void add_transition_stub (void)
{
	int nb_flux;
	add_transition_task_t *add_transition_task_tmp;
DEBUG_S ("add_transition_stub")
DEBUG_S ("current_fresque_batch->first_hyperdeck_number")
DEBUG_D (current_fresque_batch->first_hyperdeck_number)
DEBUG_S ("current_fresque_batch->nb_flux")
DEBUG_D (current_fresque_batch->nb_flux)
DEBUG_S ("previous_fresque_batch->first_hyperdeck_number")
DEBUG_D (previous_fresque_batch->first_hyperdeck_number)
DEBUG_S ("previous_fresque_batch->nb_flux")
DEBUG_D (previous_fresque_batch->nb_flux)
	nb_flux = current_fresque_batch->first_hyperdeck_number + current_fresque_batch->nb_flux;
	if (nb_flux > previous_fresque_batch->first_hyperdeck_number + previous_fresque_batch->nb_flux)
		nb_flux = previous_fresque_batch->first_hyperdeck_number + previous_fresque_batch->nb_flux;

	nb_flux -= current_fresque_batch->first_hyperdeck_number;
DEBUG_S ("nb_flux")
DEBUG_D (nb_flux)

	if ((previous_fresque_batch->first_hyperdeck_number <= current_fresque_batch->first_hyperdeck_number) && (nb_flux > 0)) {
DEBUG_S ("fresques overlap OK")
		add_transition_task_tmp = g_malloc (sizeof (add_transition_task_t));
		add_transition_task_tmp->previous_fresque_batch = previous_fresque_batch;
		add_transition_task_tmp->current_fresque_batch = current_fresque_batch;
		add_transition_task_tmp->nb_flux = nb_flux;

g_mutex_lock (&add_transition_mutex);
		add_transition_task_tmp->next = add_transition_tasks;
		add_transition_tasks = add_transition_task_tmp;
g_mutex_unlock (&add_transition_mutex);

		if (add_transition_thread == NULL) add_transition_thread = g_thread_new (NULL, (GThreadFunc)add_transition, NULL);
	} else show_message_window ("Les deux fresques ne se recouvrent pas correctement.");
DEBUG_S ("add_transition_stub END")
}

gboolean popup_fresque_batch_menu (GtkWidget *event_box, GdkEventButton *event)
{
	const gchar *name;
	gint index;
	GtkListBoxRow *list_box_row;
DEBUG_S ("popup_fresque_batch_menu")
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == GDK_BUTTON_SECONDARY) {
			name = gtk_frame_get_label (GTK_FRAME (gtk_bin_get_child (GTK_BIN (event_box))));

g_mutex_lock (&fresque_batch_mutex);
			for (current_fresque_batch = fresque_batches; current_fresque_batch != NULL; current_fresque_batch = current_fresque_batch->next) {
				if (strcmp (current_fresque_batch->name, name) == 0) {
DEBUG_S ("current_fresque_batch")
DEBUG_S (current_fresque_batch->name)
					index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque_batch->list_box_row));

					if (index > 0) {
						list_box_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index - 1);

						if (G_OBJECT_TYPE (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))) == GTK_TYPE_FRAME) {
							previous_fresque_batch = fresque_batches;
							while (GTK_LIST_BOX_ROW (previous_fresque_batch->list_box_row) != list_box_row) previous_fresque_batch = previous_fresque_batch->next;
DEBUG_S ("previous_fresque_batch")
DEBUG_S (previous_fresque_batch->name)
							gtk_menu_popup_at_pointer (GTK_MENU (fresque_batch_menu), NULL);
						}
					}
					break;
				}
			}
g_mutex_unlock (&fresque_batch_mutex);
			return GDK_EVENT_STOP;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gint fresque_batch_list_box_sort (GtkListBoxRow *row1, GtkListBoxRow *row2, fresque_batch_t *fresque_batch)
{
	return strcmp (gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (row1)))))), \
				gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (row2)))))));
}

void initialize_fresque_batch (fresque_batch_t *fresque_batch)
{
	GtkWidget *event_box, *frame, *list_box;
DEBUG_S ("initialize_fresque_batch")
DEBUG_S (fresque_batch->name)
	fresque_batch->nb_fresques = 0;

	event_box = gtk_event_box_new ();
	g_signal_connect (G_OBJECT (event_box), "button_press_event", G_CALLBACK (popup_fresque_batch_menu), NULL);
		frame = gtk_frame_new (fresque_batch->name);
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
			list_box = gtk_list_box_new ();
			gtk_list_box_set_sort_func (GTK_LIST_BOX (list_box), (GtkListBoxSortFunc)fresque_batch_list_box_sort, fresque_batch, NULL);
			g_signal_connect (G_OBJECT (list_box), "row-activated", G_CALLBACK (load_fresque), NULL);
			gtk_container_add (GTK_CONTAINER (frame), list_box);
		gtk_container_add (GTK_CONTAINER (event_box), frame);

	fresque_batch->list_box_row = gtk_list_box_row_new ();
	gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (fresque_batch->list_box_row), FALSE);
	gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (fresque_batch->list_box_row), FALSE);
	gtk_container_add (GTK_CONTAINER (fresque_batch->list_box_row), event_box);
	gtk_container_add (GTK_CONTAINER (fresques_list_box), fresque_batch->list_box_row);
	gtk_widget_show_all (fresque_batch->list_box_row);

	fresques_list_box_num++;

//	fresque_batch->index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (fresque_batch->list_box_row));
	fresque_batch->list_box = list_box;

	fresque_batch->initialized = TRUE;
}

void create_add_transition_frame (GtkBox *box)
{
	GtkWidget *box1, *scrolled_window;

	add_transition_frame = gtk_frame_new ("Ajoute une transition");
	gtk_frame_set_label_align (GTK_FRAME (add_transition_frame), 0.03, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (add_transition_frame), 5);
		box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_margin_start (box1, 5);
		gtk_widget_set_margin_end (box1, 5);
		gtk_widget_set_margin_bottom (box1, 5);
			scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
				add_transition_label = gtk_label_new (NULL);
				gtk_widget_set_margin_bottom (add_transition_label, 5);
				gtk_container_add (GTK_CONTAINER (scrolled_window),add_transition_label);
			gtk_box_pack_start (GTK_BOX (box1), scrolled_window, FALSE, FALSE, 0);

			add_transition_progress_bar = gtk_progress_bar_new ();
			gtk_box_pack_start (GTK_BOX (box1), add_transition_progress_bar, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (add_transition_frame), box1);
	gtk_box_pack_start (box, add_transition_frame, FALSE, FALSE, 0);
}

void init_fresque_batch (void)
{
	GtkWidget *menu_item;

	fresque_batch_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_label ("Ajouter une transition");
	gtk_container_add (GTK_CONTAINER (fresque_batch_menu), menu_item);
	g_signal_connect (menu_item, "activate", G_CALLBACK (add_transition_stub), NULL);
	gtk_widget_show_all (fresque_batch_menu);

	g_mutex_init (&fresque_batch_mutex);

	g_mutex_init (&add_transition_mutex);
}

