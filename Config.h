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

#ifndef __CONFIG_H
#define __CONFIG_H


#include <glib.h>


void show_config_hyperdecks_window (void);

void show_config_presets_window (void);

void show_config_transitions_window (void);

void show_config_settings_window (void);

gboolean read_config_file (void);

void write_config_file (void);


#endif

