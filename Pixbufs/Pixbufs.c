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

#include <gdk/gdk.h>

/*
+-----+---+-----+---+-----+----------+------+--------------------------------------------------+------------------------+
|     |   |     |   |     |    6     |      |                        12                        |                        |
|     |S1I|     |S2I|     +----------+      +--------------------------------------------------+                        |
|     |   |     |   |     |    BP    |      |                                                  |                        |
|     +---+     +---+     +----------+      |                                                  |                        |
|     |   |     |   |     |    7     |      |                                                  |                        |
|     |   |     |   |     +----------+      |                                                  |                        |
|     |   |     |   |     |    BS    |      |                                                  |                        |
|     |   |     |   |     +----------+      |                                                  |                        |
|  1  |S1 |  3  |S2 |  5  |    8     |  11  |                                                  |           14           |
|     |   |     |   |     +----------+      |                                                  |                        |
|     |   |     |   |     |    BL    |      |                                                  |                        |
|     |   |     |   |     +----------+      |                                                  |                        |
|     |   |     |   |     |    9     |      |                                                  |                        |
|     +---+     +---+     +----------+      |                                                  |                        |
|     |   |     |   |     |   BDel   |      |                                                  |                        |
|     | 2 |     | 4 |     +----------+      +--------------------------------------------------+                        |
|     |   |     |   |     |    10    |      |                        13                        |                        |
+-----+---+-----+---+-----+----------+------+--------------------------------------------------+------------------------+
< 32  >   < 32  >   < 32  >          <  36  >                                                  <          146           >
      <15 >     <15 >     <    68    >      <                       324                        >

<      79       >
<           126           >
<                     194            >
<                                                          700                                                          >

Hauteurs:
total = 222
'S1I' = 'S2I' = 50
'S1' = 'S2' = 123
'2' = '4' = 49
'6' = 23
'Bx' = 27
'7' = '8' = '9' = 9
'10' = 24
'12' = '13' = 27
*/

/*
for x in 1 2 3 4 5 6 7 8 9 10 11 12 13 14; do gdk-pixbuf-csource --name=pixbuf_inline_$x --raw $x.png > $x.h; done
for x in S1NS S1S S1E S1F S2NS S2S S2E S2F BPOff BPOn BS; do gdk-pixbuf-csource --name=pixbuf_inline_$x --raw $x.png > $x.h; done
for x in BSOnLOn BSOnLOff BSOffLOff BSOffLOn BDel; do gdk-pixbuf-csource --name=pixbuf_inline_$x --raw $x.png > $x.h; done
gdk-pixbuf-csource --name=pixbuf_inline_Logo --raw Logo.png > Logo.h
gdk-pixbuf-csource --name=pixbuf_inline_Icon --raw Icon.png > Icon.h
*/

#include "1.h"
#include "2.h"
#include "3.h"
#include "4.h"
#include "5.h"
#include "6.h"
#include "7.h"
#include "8.h"
#include "9.h"
#include "10.h"
#include "11.h"
#include "12.h"
#include "13.h"
#include "14.h"
#include "S1NS.h"
#include "S1S.h"
#include "S1E.h"
#include "S1F.h"
#include "S2NS.h"
#include "S2S.h"
#include "S2E.h"
#include "S2F.h"
#include "BPOff.h"
#include "BPOn.h"
#include "BS.h"
#include "BSOnLOn.h"
#include "BSOnLOff.h"
#include "BSOffLOff.h"
#include "BSOffLOn.h"
#include "BDel.h"

#include "Up.h"
#include "Down.h"

#include "Logo.h"
#include "Icon.h"


GdkPixbuf *pixbuf_1, *pixbuf_2, *pixbuf_3, *pixbuf_4, *pixbuf_5, *pixbuf_6, *pixbuf_7, *pixbuf_8, *pixbuf_9;
GdkPixbuf *pixbuf_10, *pixbuf_11, *pixbuf_12, *pixbuf_13, *pixbuf_14;
GdkPixbuf *pixbuf_S1NS, *pixbuf_S1S, *pixbuf_S1E, *pixbuf_S1F, *pixbuf_S2NS, *pixbuf_S2S, *pixbuf_S2E, *pixbuf_S2F;
GdkPixbuf *pixbuf_BPOff, *pixbuf_BPOn, *pixbuf_BS;
GdkPixbuf *pixbuf_loop[4];
GdkPixbuf *pixbuf_BDel;

GdkPixbuf *pixbuf_Up, *pixbuf_Down;

GdkPixbuf *pixbuf_Logo;
GdkPixbuf *pixbuf_Icon;


void load_pixbufs (void)
{
	pixbuf_1 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_1), pixbuf_inline_1, FALSE, NULL);
	pixbuf_2 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_2), pixbuf_inline_2, FALSE, NULL);
	pixbuf_3 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_3), pixbuf_inline_3, FALSE, NULL);
	pixbuf_4 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_4), pixbuf_inline_4, FALSE, NULL);
	pixbuf_5 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_5), pixbuf_inline_5, FALSE, NULL);
	pixbuf_6 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_6), pixbuf_inline_6, FALSE, NULL);
	pixbuf_7 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_7), pixbuf_inline_7, FALSE, NULL);
	pixbuf_8 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_8), pixbuf_inline_8, FALSE, NULL);
	pixbuf_9 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_9), pixbuf_inline_9, FALSE, NULL);
	pixbuf_10 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_10), pixbuf_inline_10, FALSE, NULL);
	pixbuf_11 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_11), pixbuf_inline_11, FALSE, NULL);
	pixbuf_12 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_12), pixbuf_inline_12, FALSE, NULL);
	pixbuf_13 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_13), pixbuf_inline_13, FALSE, NULL);
	pixbuf_14 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_14), pixbuf_inline_14, FALSE, NULL);
	pixbuf_S1NS = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S1NS), pixbuf_inline_S1NS, FALSE, NULL);
	pixbuf_S1S = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S1S), pixbuf_inline_S1S, FALSE, NULL);
	pixbuf_S1E = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S1E), pixbuf_inline_S1E, FALSE, NULL);
	pixbuf_S1F = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S1F), pixbuf_inline_S1F, FALSE, NULL);
	pixbuf_S2NS = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S2NS), pixbuf_inline_S2NS, FALSE, NULL);
	pixbuf_S2S = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S2S), pixbuf_inline_S2S, FALSE, NULL);
	pixbuf_S2E = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S2E), pixbuf_inline_S2E, FALSE, NULL);
	pixbuf_S2F = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_S2F), pixbuf_inline_S2F, FALSE, NULL);
	pixbuf_BPOff = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BPOff), pixbuf_inline_BPOff, FALSE, NULL);
	pixbuf_BPOn = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BPOn), pixbuf_inline_BPOn, FALSE, NULL);
	pixbuf_BS = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BS), pixbuf_inline_BS, FALSE, NULL);
	pixbuf_loop[0] = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BSOnLOn), pixbuf_inline_BSOnLOn, FALSE, NULL);
	pixbuf_loop[1] = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BSOnLOff), pixbuf_inline_BSOnLOff, FALSE, NULL);
	pixbuf_loop[2] = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BSOffLOff), pixbuf_inline_BSOffLOff, FALSE, NULL);
	pixbuf_loop[3] = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BSOffLOn), pixbuf_inline_BSOffLOn, FALSE, NULL);
	pixbuf_BDel = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_BDel), pixbuf_inline_BDel, FALSE, NULL);

	pixbuf_Up = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_Up), pixbuf_inline_Up, FALSE, NULL);
	pixbuf_Down = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_Down), pixbuf_inline_Down, FALSE, NULL);

	pixbuf_Logo = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_Logo), pixbuf_inline_Logo, FALSE, NULL);
	pixbuf_Icon = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_Icon), pixbuf_inline_Icon, FALSE, NULL);
}

