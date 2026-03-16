/*
 * copyright (c) 2026 Thomas Paillet <thomas.paillet@net-c.fr

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

#ifndef __PIXBUFS_H
#define __PIXBUFS_H


#include <gdk/gdk.h>


extern GdkPixbuf *pixbuf_1, *pixbuf_2, *pixbuf_3, *pixbuf_4, *pixbuf_5, *pixbuf_6, *pixbuf_7, *pixbuf_8, *pixbuf_9;
extern GdkPixbuf *pixbuf_10, *pixbuf_11, *pixbuf_12, *pixbuf_13, *pixbuf_14;
extern GdkPixbuf *pixbuf_S1NS, *pixbuf_S1S, *pixbuf_S1E, *pixbuf_S1F, *pixbuf_S2NS, *pixbuf_S2S, *pixbuf_S2E, *pixbuf_S2F;
extern GdkPixbuf *pixbuf_BPOff, *pixbuf_BPOn, *pixbuf_BS;
extern GdkPixbuf *pixbuf_loop[4];
extern GdkPixbuf *pixbuf_BDel;

extern GdkPixbuf *pixbuf_Up, *pixbuf_Down;

extern GdkPixbuf *pixbuf_Logo;
extern GdkPixbuf *pixbuf_Icon;


void load_pixbufs (void);


#endif

