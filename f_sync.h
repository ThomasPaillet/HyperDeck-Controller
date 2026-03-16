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

#ifndef __F_SYNC_H
#define __F_SYNC_H


#ifdef _WIN32
	#include <stdio.h>
	#include <io.h>

	#define F_SYNC(f) _commit (fileno (f));

#elif defined (__linux)
	#include <stdio.h>
	#include <unistd.h>

	#define F_SYNC(f) fsync (fileno (f));
#endif


#endif

