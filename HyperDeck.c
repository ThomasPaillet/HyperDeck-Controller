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


#ifdef DEBUG_HYPERDECK
	GMutex debug_mutex;
#ifdef _WIN32
	FILE *debug_file;
	FILE *debug_hyperdeck[NB_OF_HYPERDECKS];
	FILE *debug_drop_thread[NB_OF_HYPERDECKS];
#endif
#endif


hyperdeck_t hyperdecks[NB_OF_HYPERDECKS];

GtkWidget *main_window;

GtkCssProvider *css_provider;

PangoFontDescription *font_description;

GtkTargetEntry uri_list_target = {"text/uri-list", 0, 0};


gboolean select_slot (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck)
{
	hyperdeck->timeline_empty_retry = 5;

	if ((event_box == hyperdeck->slot_1_event_box) && (hyperdeck->slot_1_is_mounted)) SEND (hyperdeck, msg_select_slot_id_1)

	if ((event_box == hyperdeck->slot_2_event_box) && (hyperdeck->slot_2_is_mounted)) SEND (hyperdeck, msg_select_slot_id_2)

	return GDK_EVENT_STOP;
}

void play_button_clicked (GtkButton *button, hyperdeck_t *hyperdeck)
{
	send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
}

void stop_button_clicked (GtkButton *button, hyperdeck_t *hyperdeck)
{
	send (hyperdeck->socket, msg_stop, 5, 0);
}

void single_loop_button_clicked (GtkButton *button, hyperdeck_t *hyperdeck)
{
	hyperdeck->loop++;
	if (hyperdeck->loop == 4) hyperdeck->loop = 0;

	if (hyperdeck->play) send (hyperdeck->socket, msg_play_single_loop[hyperdeck->loop], msg_play_single_loop_len[hyperdeck->loop], 0);
	gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[hyperdeck->loop]);
}

void load_last_preset_and_purge (hyperdeck_t *hyperdeck)
{
	int msg_len;
	char msg_goto_clip_id_x[32];
	gboolean preset_exist;

	if (hyperdeck->default_preset_clip_id != 0) {
		send (hyperdeck->socket, msg_stop, 5, 0);

		msg_len = sprintf (msg_goto_clip_id_x, "%s%d\n", msg_goto_clip_id_, hyperdeck->default_preset_clip_id);
		send (hyperdeck->socket, msg_goto_clip_id_x, msg_len, 0);

		if (hyperdeck->loop != SINGLE_CLIP_TRUE_LOOP_TRUE) {
			hyperdeck->loop = SINGLE_CLIP_TRUE_LOOP_TRUE;
			gtk_image_set_from_pixbuf (GTK_IMAGE (hyperdeck->image_single_loop_button), pixbuf_loop[SINGLE_CLIP_TRUE_LOOP_TRUE]);
		}

		preset_exist = TRUE;
	} else preset_exist = FALSE;

	purge_hyperdeck (hyperdeck);

	if (preset_exist) send (hyperdeck->socket, msg_play_single_clip_true_loop_true, 35, 0);
}

void del_button_clicked (GtkButton *button, hyperdeck_t *hyperdeck)
{
	GtkWidget *window, *box1, *box2, *widget;
	char msg[64];

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (window), "Attention !");
	gtk_window_set_default_size (GTK_WINDOW (window), 10, 10);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box1), 5);
		widget = gtk_label_new ("Etes-vous sûr de vouloir supprimer tous les clips du lecteur");
		gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

		sprintf (msg, "HyperDeck n°%d, sauf ceux nécessaires aux \"Presets\" ?", hyperdeck->number + 1);
		widget = gtk_label_new (msg);
		gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_margin_top (box2, 5);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_box_set_spacing (GTK_BOX (box2), 5);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			widget = gtk_button_new_with_label ("Oui");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (load_last_preset_and_purge), hyperdeck);
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, TRUE, TRUE, 0);

			widget = gtk_button_new_with_label ("Non");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
}

void load_clip (GtkListBox *list_box, GtkListBoxRow *list_box_row, hyperdeck_t *hyperdeck)
{
	char msg[32];
	int msg_len;
	clip_list_t *clip_list_tmp;
	const gchar *name;

	if (list_box_row == NULL) return;

	name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))));

	for (clip_list_tmp = hyperdeck->list_of_clips; clip_list_tmp != NULL; clip_list_tmp = clip_list_tmp->next) {
		if (strcmp (clip_list_tmp->name, name) == 0) {
			send (hyperdeck->socket, msg_stop, 5, 0);
			deselect_fresque ();
			msg_len = sprintf (msg, "%s%d\n", msg_goto_clip_id_, clip_list_tmp->id);
			send (hyperdeck->socket, msg, msg_len, 0);
			break;
		}
	}
}

gboolean draw_image_14 (GtkWidget *widget, cairo_t *cr, hyperdeck_t *hyperdeck)
{
	PangoLayout *pl;

	if (!gtk_widget_is_sensitive (widget)) gtk_widget_set_opacity (widget, 0.5);
	else gtk_widget_set_opacity (widget, 1.0);

	gdk_cairo_set_source_pixbuf (cr, pixbuf_14, 0, 0);
	cairo_paint (cr);

	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.9);
	cairo_translate (cr, 123, 193);

	pl = pango_cairo_create_layout (cr);
	pango_layout_set_text (pl, hyperdeck->name, 1);
	pango_layout_set_font_description (pl, font_description);
	pango_cairo_show_layout (cr, pl);
	g_object_unref(pl);

	return GDK_EVENT_STOP;
}

void create_hyperdeck_window (hyperdeck_t *hyperdeck)
{
	GtkWidget *box_slot_1, *box_slot_2;
	GtkWidget *box_buttons, *box_list, *scrolled_window;

	GtkWidget *image_1, *image_2, *image_3, *image_4, *image_5, *image_6, *image_7;
	GtkWidget *image_8, *image_9, *image_10, *image_11, *image_12, *image_13, *image_14;

	GtkWidget *image_stop_button;
	GtkWidget *image_del_button;

	image_1 = gtk_image_new_from_pixbuf (pixbuf_1);
	image_2 = gtk_image_new_from_pixbuf (pixbuf_2);
	image_3 = gtk_image_new_from_pixbuf (pixbuf_3);
	image_4 = gtk_image_new_from_pixbuf (pixbuf_4);
	image_5 = gtk_image_new_from_pixbuf (pixbuf_5);
	image_6 = gtk_image_new_from_pixbuf (pixbuf_6);
	image_7 = gtk_image_new_from_pixbuf (pixbuf_7);
	image_8 = gtk_image_new_from_pixbuf (pixbuf_8);
	image_9 = gtk_image_new_from_pixbuf (pixbuf_9);
	image_10 = gtk_image_new_from_pixbuf (pixbuf_10);
	image_11 = gtk_image_new_from_pixbuf (pixbuf_11);
	image_12 = gtk_image_new_from_pixbuf (pixbuf_12);
	image_13 = gtk_image_new_from_pixbuf (pixbuf_13);
	image_14 = gtk_drawing_area_new ();
	gtk_widget_set_size_request (image_14, 146, 222);
	g_signal_connect (G_OBJECT (image_14), "draw", G_CALLBACK (draw_image_14), hyperdeck);

	hyperdeck->image_play_button = gtk_image_new_from_pixbuf (pixbuf_BPOff);
	image_stop_button = gtk_image_new_from_pixbuf (pixbuf_BS);
	hyperdeck->image_single_loop_button = gtk_image_new_from_pixbuf (pixbuf_loop[0]);
	image_del_button = gtk_image_new_from_pixbuf (pixbuf_BDel);

	hyperdeck->root_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), image_1, FALSE, FALSE, 0);

	box_slot_1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		hyperdeck->image_slot_1_indicator = gtk_image_new_from_pixbuf (pixbuf_S1NS);
		gtk_box_pack_start (GTK_BOX (box_slot_1), hyperdeck->image_slot_1_indicator, FALSE, FALSE, 0);

		hyperdeck->slot_1_event_box = gtk_event_box_new ();
		g_signal_connect (G_OBJECT (hyperdeck->slot_1_event_box), "button_press_event", G_CALLBACK (select_slot), hyperdeck);
			hyperdeck->image_slot_1 = gtk_image_new_from_pixbuf (pixbuf_S1E);
			gtk_container_add (GTK_CONTAINER (hyperdeck->slot_1_event_box), hyperdeck->image_slot_1);
		gtk_box_pack_start (GTK_BOX (box_slot_1), hyperdeck->slot_1_event_box, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_slot_1), image_2, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), box_slot_1, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), image_3, FALSE, FALSE, 0);

	box_slot_2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		hyperdeck->image_slot_2_indicator = gtk_image_new_from_pixbuf (pixbuf_S2NS);
		gtk_box_pack_start (GTK_BOX (box_slot_2), hyperdeck->image_slot_2_indicator, FALSE, FALSE, 0);

		hyperdeck->slot_2_event_box = gtk_event_box_new ();
		g_signal_connect (G_OBJECT (hyperdeck->slot_2_event_box), "button_press_event", G_CALLBACK (select_slot), hyperdeck);
			hyperdeck->image_slot_2 = gtk_image_new_from_pixbuf (pixbuf_S2E);
			gtk_container_add (GTK_CONTAINER (hyperdeck->slot_2_event_box), hyperdeck->image_slot_2);
		gtk_box_pack_start (GTK_BOX (box_slot_2), hyperdeck->slot_2_event_box, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_slot_2), image_4, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), box_slot_2, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), image_5, FALSE, FALSE, 0);

	box_buttons = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start (GTK_BOX (box_buttons), image_6, FALSE, FALSE, 0);

		hyperdeck->play_button = gtk_button_new ();
		gtk_button_set_image (GTK_BUTTON (hyperdeck->play_button), hyperdeck->image_play_button);
		gtk_style_context_add_provider (gtk_widget_get_style_context (hyperdeck->play_button), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_signal_connect (G_OBJECT (hyperdeck->play_button), "clicked", G_CALLBACK (play_button_clicked), hyperdeck);
		gtk_box_pack_start (GTK_BOX (box_buttons), hyperdeck->play_button, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_buttons), image_7, FALSE, FALSE, 0);

		hyperdeck->stop_button = gtk_button_new ();
		gtk_button_set_image (GTK_BUTTON (hyperdeck->stop_button), image_stop_button);
		gtk_style_context_add_provider (gtk_widget_get_style_context (hyperdeck->stop_button), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_signal_connect (G_OBJECT (hyperdeck->stop_button), "clicked", G_CALLBACK (stop_button_clicked), hyperdeck);
		gtk_box_pack_start (GTK_BOX (box_buttons), hyperdeck->stop_button, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_buttons), image_8, FALSE, FALSE, 0);

		hyperdeck->single_loop_button = gtk_button_new ();
		gtk_button_set_image (GTK_BUTTON (hyperdeck->single_loop_button), hyperdeck->image_single_loop_button);
		gtk_style_context_add_provider (gtk_widget_get_style_context (hyperdeck->single_loop_button), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_signal_connect (G_OBJECT (hyperdeck->single_loop_button), "clicked", G_CALLBACK (single_loop_button_clicked), hyperdeck);
		gtk_box_pack_start (GTK_BOX (box_buttons), hyperdeck->single_loop_button, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_buttons), image_9, FALSE, FALSE, 0);

		hyperdeck->del_button = gtk_button_new ();
		gtk_button_set_image (GTK_BUTTON (hyperdeck->del_button), image_del_button);
		gtk_style_context_add_provider (gtk_widget_get_style_context (hyperdeck->del_button), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_signal_connect (G_OBJECT (hyperdeck->del_button), "clicked", G_CALLBACK (del_button_clicked), hyperdeck);
		gtk_box_pack_start (GTK_BOX (box_buttons), hyperdeck->del_button, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_buttons), image_10, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), box_buttons, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), image_11, FALSE, FALSE, 0);

	box_list = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start (GTK_BOX (box_list), image_12, FALSE, FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			hyperdeck->list_box = gtk_list_box_new ();
			g_signal_connect (G_OBJECT (hyperdeck->list_box), "row-activated", G_CALLBACK (load_clip), hyperdeck);
			gtk_container_add (GTK_CONTAINER (scrolled_window), hyperdeck->list_box);
		gtk_box_pack_start (GTK_BOX (box_list), scrolled_window, TRUE, TRUE, 0);

		hyperdeck->progress_bar = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (box_list), hyperdeck->progress_bar, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (box_list), image_13, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), box_list, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hyperdeck->root_widget), image_14, FALSE, FALSE, 0);
}

void show_about_window (void)
{
	GtkWidget *about_window, *box, *widget;
	char gtk_version[64];

	about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (about_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (about_window), "A propos");
	gtk_window_set_default_size (GTK_WINDOW (about_window), 200, 150);
	gtk_window_set_modal (GTK_WINDOW (about_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (about_window), GTK_WINDOW (main_window));
	gtk_window_set_position (GTK_WINDOW (about_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (about_window), "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (box, 5);
	gtk_widget_set_margin_end (box, 5);
	gtk_widget_set_margin_bottom (box, 5);
		widget = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (widget), "<b>Contrôleur Blackmagic HyperDeck</b>");
		gtk_widget_set_margin_bottom (widget, 3);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Version 2.0");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_image_new_from_pixbuf (pixbuf_Logo);
		gtk_widget_set_margin_top (widget, 3);
		gtk_widget_set_margin_bottom (widget, 3);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Blackmagic HyperDeck Ethernet Protocol version: " HYPERDECK_PROTOCOL_VERSION);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		sprintf (gtk_version, "Compiled against GTK+ version: %d.%d.%d", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
		widget = gtk_label_new (gtk_version);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = get_libavformat_version ();
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = get_libavcodec_version ();
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = get_libavfilter_version ();
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Copyright (c) 2018-2021 Thomas Paillet");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("GNU General Public License version 3");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (about_window), box);

	gtk_window_set_resizable (GTK_WINDOW (about_window), FALSE);
	gtk_widget_show_all (about_window);
}

gboolean hyperdeck_main_quit (void)
{
	gtk_widget_hide (main_window);

	gtk_main_quit ();

	return GDK_EVENT_STOP;
}

gboolean main_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	gint index;
	GtkListBoxRow *list_box_row;
	fresque_batch_t *fresque_batch_tmp;

	if ((event->keyval == GDK_KEY_l) || (event->keyval == GDK_KEY_L)) {
		if (gtk_widget_get_sensitive (fresques_loop_button)) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fresques_loop_button), !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fresques_loop_button)));
		}
	} else if ((event->keyval == GDK_KEY_space) || (event->keyval == GDK_KEY_p) || (event->keyval == GDK_KEY_P)) {
		if (gtk_widget_get_sensitive (fresques_play_button)) gtk_button_clicked (GTK_BUTTON (fresques_play_button));
	} else if ((event->keyval == GDK_KEY_s) || (event->keyval == GDK_KEY_S)) {
		if (gtk_widget_get_sensitive (fresques_stop_button)) gtk_button_clicked (GTK_BUTTON (fresques_stop_button));
	} else if ((event->keyval == GDK_KEY_Up) && (current_fresque != NULL)) {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->list_box_row));

		if (current_fresque->parent_fresque_batch != NULL) {
			if (index == 0) {
				index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->parent_fresque_batch->list_box_row));

				if (index > 0) {
					gtk_list_box_unselect_row (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));

					list_box_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index - 1);

					if (G_OBJECT_TYPE (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))) == GTK_TYPE_FRAME) {
						fresque_batch_tmp = fresque_batches;
						while (GTK_LIST_BOX_ROW (fresque_batch_tmp->list_box_row) != list_box_row) fresque_batch_tmp = fresque_batch_tmp->next;
						g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresque_batch_tmp->list_box), fresque_batch_tmp->nb_fresques - 1), "activate");
					} else g_signal_emit_by_name (list_box_row, "activate");
				}
			} else g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), index - 1), "activate");
		} else {
			if (index > 0) {
				list_box_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index - 1);

				if (G_OBJECT_TYPE (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))) == GTK_TYPE_FRAME) {
					gtk_list_box_unselect_row (GTK_LIST_BOX (fresques_list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));

					fresque_batch_tmp = fresque_batches;
					while (GTK_LIST_BOX_ROW (fresque_batch_tmp->list_box_row) != list_box_row) fresque_batch_tmp = fresque_batch_tmp->next;
					g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresque_batch_tmp->list_box), fresque_batch_tmp->nb_fresques - 1), "activate");
				} else g_signal_emit_by_name (list_box_row, "activate");
			}
		}
	} else if ((event->keyval == GDK_KEY_Down) && (current_fresque != NULL)) {
		index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->list_box_row));

		if (current_fresque->parent_fresque_batch != NULL) {
			if (index < current_fresque->parent_fresque_batch->nb_fresques - 1) {
				g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), index + 1), "activate");
			} else {
				index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (current_fresque->parent_fresque_batch->list_box_row));
				list_box_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index + 1);

				if (list_box_row != NULL) {
					gtk_list_box_unselect_row (GTK_LIST_BOX (current_fresque->parent_fresque_batch->list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));

					if (G_OBJECT_TYPE (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))) == GTK_TYPE_FRAME) {
						fresque_batch_tmp = fresque_batches;
						while (GTK_LIST_BOX_ROW (fresque_batch_tmp->list_box_row) != list_box_row) fresque_batch_tmp = fresque_batch_tmp->next;
						g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresque_batch_tmp->list_box), 0), "activate");
					} else g_signal_emit_by_name (list_box_row, "activate");
				}
			}
		} else {
			list_box_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresques_list_box), index + 1);

			if (list_box_row != NULL) {
				if (G_OBJECT_TYPE (gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (list_box_row))))) == GTK_TYPE_FRAME) {
					gtk_list_box_unselect_row (GTK_LIST_BOX (fresques_list_box), GTK_LIST_BOX_ROW (current_fresque->list_box_row));

					fresque_batch_tmp = fresque_batches;
					while (GTK_LIST_BOX_ROW (fresque_batch_tmp->list_box_row) != list_box_row) fresque_batch_tmp = fresque_batch_tmp->next;
					g_signal_emit_by_name (gtk_list_box_get_row_at_index (GTK_LIST_BOX (fresque_batch_tmp->list_box), 0), "activate");
				} else g_signal_emit_by_name (list_box_row, "activate");
			}
		}
	} else if ((event->keyval == GDK_KEY_q) || (event->keyval == GDK_KEY_Q)) hyperdeck_main_quit ();

	return GDK_EVENT_STOP;
}

void create_main_window (void)
{
	int i;

	GtkWidget *main_box, *hyperdeck_box, *control_box;
	GtkWidget *menu_bar, *menu_config, *sub_menu_config, *menu_adresses_ip, *menu_presets, *menu_transitions, *separator, *menu_about;
	GtkWidget *frame;

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), "Contrôleur Blackmagic HyperDeck");
	gtk_window_set_icon (GTK_WINDOW (main_window), pixbuf_Icon);
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (hyperdeck_main_quit), NULL);
	g_signal_connect (G_OBJECT (main_window), "key-press-event", G_CALLBACK (main_window_key_press), NULL);

	main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		hyperdeck_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			create_hyperdeck_window (&hyperdecks[i]);
			gtk_widget_set_sensitive (hyperdecks[i].root_widget, FALSE);
			gtk_drag_dest_set (hyperdecks[i].root_widget, GTK_DEST_DEFAULT_ALL, &uri_list_target, 1, GDK_ACTION_COPY);
			g_signal_connect (G_OBJECT (hyperdecks[i].root_widget), "drag-data-received", G_CALLBACK (hyperdeck_drag_data_received), &hyperdecks[i]);
			gtk_box_pack_start (GTK_BOX (hyperdeck_box), hyperdecks[i].root_widget, FALSE, FALSE, 0);
		}
		gtk_box_pack_start (GTK_BOX (main_box), hyperdeck_box, FALSE, FALSE, 0);

		control_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			menu_bar = gtk_menu_bar_new ();
			menu_config = gtk_menu_item_new_with_label ("Configuration");
				sub_menu_config = gtk_menu_new ();
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_config), sub_menu_config);

				menu_adresses_ip = gtk_menu_item_new_with_label ("HyperDecks");
				gtk_container_add (GTK_CONTAINER (sub_menu_config), menu_adresses_ip);
				g_signal_connect (G_OBJECT (menu_adresses_ip), "activate", G_CALLBACK (show_config_hyperdecks_window), NULL);

				menu_presets = gtk_menu_item_new_with_label ("Presets");
				gtk_container_add (GTK_CONTAINER (sub_menu_config), menu_presets);
				g_signal_connect (G_OBJECT (menu_presets), "activate", G_CALLBACK (show_config_presets_window), NULL);

				menu_transitions = gtk_menu_item_new_with_label ("Transitions");
				gtk_container_add (GTK_CONTAINER (sub_menu_config), menu_transitions);
				g_signal_connect (G_OBJECT (menu_transitions), "activate", G_CALLBACK (show_config_transitions_window), NULL);

				separator = gtk_separator_menu_item_new ();
				gtk_container_add (GTK_CONTAINER (sub_menu_config), separator);

				menu_about = gtk_menu_item_new_with_label ("A propos");
				gtk_container_add (GTK_CONTAINER (sub_menu_config), menu_about);
				g_signal_connect (G_OBJECT (menu_about), "activate", G_CALLBACK (show_about_window), NULL);
			gtk_container_add (GTK_CONTAINER (menu_bar), menu_config);
			gtk_box_pack_start (GTK_BOX (control_box), menu_bar, FALSE, FALSE, 0);

			frame = create_presets_frame ();
			gtk_box_pack_start (GTK_BOX (control_box), frame, FALSE, FALSE, 0);

			frame = create_fresques_frame ();
			gtk_drag_dest_set (frame, GTK_DEST_DEFAULT_ALL, &uri_list_target, 1, GDK_ACTION_COPY);
			g_signal_connect (G_OBJECT (frame), "drag-data-received", G_CALLBACK (fresques_drag_data_received), NULL);
			gtk_box_pack_start (GTK_BOX (control_box), frame, TRUE, TRUE, 0);

			create_add_transition_frame (GTK_BOX (control_box));

			create_transcoding_frames (GTK_BOX (control_box));
		gtk_box_pack_start (GTK_BOX (main_box), control_box, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (main_window), main_box);

	gtk_window_set_resizable (GTK_WINDOW (main_window), FALSE);
}

#ifdef _WIN32
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
#elif defined (__linux)
int main (int argc, char** argv)
#endif
{
	int i, config_ok;
	GFile *file;

	DEBUG_INIT

	WSAInit ();	//_WIN32

#ifdef _WIN32
	gtk_init (NULL, NULL);
#ifndef DEBUG_HYPERDECK
	FreeConsole ();
#endif
#elif defined (__linux)
	gtk_init (&argc, &argv);
#endif

	css_provider = gtk_css_provider_new ();
	file = g_file_new_for_path ("Widgets.css");
	gtk_css_provider_load_from_file (css_provider, file, NULL);
	g_object_unref (file);

	font_description = pango_font_description_from_string ("Courier Bold 15");

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		hyperdecks[i].name[0] = (char)(49 + i);
		hyperdecks[i].name[1] = '\0';
		hyperdecks[i].number = i;

		hyperdecks[i].switched_on = TRUE;
		hyperdecks[i].adresse_ip[0] = '\0';
		hyperdecks[i].adresse_ip_is_valid = FALSE;
		hyperdecks[i].connected = TRUE;

		hyperdecks[i].connection_thread = NULL;
		g_mutex_init (&hyperdecks[i].connection_mutex);

		hyperdecks[i].slot_1_is_mounted = FALSE;
		hyperdecks[i].slot_2_is_mounted = FALSE;
		hyperdecks[i].disk_slot_id = 0;
		hyperdecks[i].slot_1_disk_list = NULL;
		hyperdecks[i].slot_2_disk_list = NULL;

		hyperdecks[i].slot_selected = 1;
		hyperdecks[i].clip_count = 0;
		hyperdecks[i].list_of_clips = NULL;

		hyperdecks[i].default_preset_clip_id = 0;

		hyperdecks[i].play = FALSE;
		hyperdecks[i].loop = SINGLE_CLIP_TRUE_LOOP_TRUE;

		hyperdecks[i].reboot = FALSE;

		hyperdecks[i].drop_list_file = NULL;
		g_mutex_init (&hyperdecks[i].drop_mutex);
		hyperdecks[i].drop_thread = NULL;

		hyperdecks[i].remuxing_list_file = NULL;
		g_mutex_init (&hyperdecks[i].remuxing_mutex);
		hyperdecks[i].remuxing_thread = NULL;

		hyperdecks[i].transcoding_list_file = NULL;
		g_mutex_init (&hyperdecks[i].transcoding_mutex);
		hyperdecks[i].transcoding_thread = NULL;

		g_mutex_init (&hyperdecks[i].ftp_mutex);

		hyperdecks[i].last_file_dropped = NULL;
		g_mutex_init (&hyperdecks[i].last_file_dropped_mutex);

		hyperdecks[i].timeline_empty_retry = 5;
	}

	init_fresque_batch ();

	init_file ();

	init_presets ();

	init_transitions ();

	init_hyperdeck_codec ();

	config_ok = read_config_file ();

	load_pixbufs ();

	create_main_window ();

	gtk_widget_show_all (main_window);

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		gtk_widget_hide (hyperdecks[i].progress_bar);
		gtk_widget_hide (transcoding_frames[i].frame);
		gtk_widget_hide (remuxing_frames[i].frame);
		gtk_widget_hide (add_transition_frame);
	}

	if (!config_ok) show_config_hyperdecks_window ();

	restore_hyperdeck_state ();

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (hyperdecks[i].switched_on && hyperdecks[i].adresse_ip_is_valid)
			hyperdecks[i].connection_thread = g_thread_new (NULL, (GThreadFunc)connect_to_hyperdeck, &hyperdecks[i]);
	}

	gtk_main ();

	for (i = 0; i < NB_OF_HYPERDECKS; i++) {
		if (hyperdecks[i].connected) disconnect_from_hyperdeck (&hyperdecks[i]);
	}

	save_hyperdeck_state ();

	for (i = 0; i < NB_OF_TRANSITIONS; i++) g_free (transitions[i].file_name);

	gtk_widget_destroy (main_window);

	pango_font_description_free (font_description);

	WSACleanup ();	//_WIN32

	return 0;
}
