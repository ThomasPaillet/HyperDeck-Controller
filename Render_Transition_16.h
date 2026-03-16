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

#ifndef __RENDER_TRANSITION_16_H
#define __RENDER_TRANSITION_16_H


#include "HyperDeck.h"


void stripe_color_RGB_to_YUV_16 (void);

void render_transition_16 (hyperdeck_t* first_hyperdeck, hyperdeck_t* hyperdeck, drop_list_t *drop_list, AVFrame *background_frame, AVFrame *fresque_frame, int nb_flux, float *last_x, float sin_minus_7, float stride, float step, struct SwsContext *sws_context, AVFrame *frame_tmp, AVFrame *frame_out, char *creation_time, gboolean reverse);


#endif

