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

#ifndef __FILE_H
#define __FILE_H


#include "HyperDeck.h"


#define REFRESH_WAITING_TIME 600


gboolean refresh_hyperdeck_list_of_clips (hyperdeck_t *hyperdeck);

gpointer purge_hyperdeck (hyperdeck_t *hyperdeck);

gboolean delete_clip (GtkWidget *event_box, GdkEventButton *event, hyperdeck_t *hyperdeck);

gboolean delete_fresque (GtkWidget *event_box, GdkEventButton *event);

void complete_file_name_out (drop_list_t *drop_list);

void add_drop_list_to_hyperdeck_transfert_queue (drop_list_t *drop_list, hyperdeck_t *hyperdeck);

void add_drop_list_to_hyperdeck_transfert_queue (drop_list_t *drop_list, hyperdeck_t *hyperdeck);

void hyperdeck_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, hyperdeck_t *hyperdeck);

void fresques_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time);

void init_file (void);


#endif

