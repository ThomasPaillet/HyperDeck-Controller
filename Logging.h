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

#ifndef __LOGGING_H
#define __LOGGING_H


#include "HyperDeck.h"

#include <stdio.h>


#define LOG_STRING(s) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_INT(i) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_LONG(l) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_long (__FILE__, l, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_FLOAT(f) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_float (__FILE__, f, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_POINTER(p) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_pointer (__FILE__, p, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_2_STRINGS(s,t) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_STRING_INT(s,i) \
if (logging) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, NULL); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_STRING(h,s) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_string (__FILE__, h, s); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_INT(h,i) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_int (__FILE__, h, i); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_LONG(h,l) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_long (__FILE__, h, l); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_FLOAT(h,f) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_float (__FILE__, h, f); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_POINTER(h,p) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_pointer (__FILE__, h, p); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_2_STRINGS(h,s,t) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_2_strings (__FILE__, h, s, t); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_STRING_INT(h,s,i) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_string_int (__FILE__, h, s, i); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_STRING_LONG(h,s,l) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_string_long (__FILE__, h, s, l); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_HYPERDECK_STRING_FLOAT(h,s,f) \
if (logging && log_hyperdeck) { \
	g_mutex_lock (&logging_mutex); \
	log_hyperdeck_string_float (__FILE__, h, s, f); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_STRING(s) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_INT(i) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_LONG(l) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_long (__FILE__, l, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_FLOAT(f) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_float (__FILE__, f, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_POINTER(p) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_pointer (__FILE__, p, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_2_STRINGS(s,t) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_STRING_INT(s,i) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_STRING_LONG(s,l) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_string_long (__FILE__, s, l, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_STRING_FLOAT(s,f) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_string_float (__FILE__, s, f, osc_log_file); \
	g_mutex_unlock (&logging_mutex); \
}

#define LOG_OSC_PACKET(a,p,s) \
if (logging && log_osc) { \
	g_mutex_lock (&logging_mutex); \
	log_osc_packet (inet_ntoa (a), p, s); \
	g_mutex_unlock (&logging_mutex); \
}


extern gboolean logging;

extern gboolean log_hyperdeck;
extern gboolean log_osc;

extern GMutex logging_mutex;

extern FILE *osc_log_file;


void log_string (const char *c_source_filename, const char *str, FILE *log_file);

void log_int (const char *c_source_filename, int i, FILE *log_file);

void log_long (const char *c_source_filename, long l, FILE *log_file);

void log_float (const char *c_source_filename, float f, FILE *log_file);

void log_pointer (const char *c_source_filename, void *p, FILE *log_file);

void log_2_strings (const char *c_source_filename, const char *str1, const char *str2, FILE *log_file);

void log_string_int (const char *c_source_filename, const char *str, int i, FILE *log_file);

void log_string_long (const char *c_source_filename, const char *str, long l, FILE *log_file);

void log_string_float (const char *c_source_filename, const char *str, float f, FILE *log_file);

void log_hyperdeck_string (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str);

void log_hyperdeck_int (const char *c_source_filename, hyperdeck_t *hyperdeck, int i);

void log_hyperdeck_long (const char *c_source_filename, hyperdeck_t *hyperdeck, long l);

void log_hyperdeck_float (const char *c_source_filename, hyperdeck_t *hyperdeck, float f);

void log_hyperdeck_pointer (const char *c_source_filename, hyperdeck_t *hyperdeck, void *p);

void log_hyperdeck_2_strings (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str1, const char *str2);

void log_hyperdeck_string_int (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, int i);

void log_hyperdeck_string_long (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, long l);

void log_hyperdeck_string_float (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, float f);

void log_osc_packet (const char *ip_address, const char *packet, int size);

void init_logging (void);

void start_logging (void);

void start_hyperdeck_log (void);

void start_osc_log (void);

void stop_logging (void);

void stop_hyperdeck_log (void);

void stop_osc_log (void);


#endif

