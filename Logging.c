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

#include "Logging.h"

#include "f_sync.h"


gboolean logging = FALSE;

gboolean log_hyperdeck = FALSE;
gboolean log_osc = FALSE;

GMutex logging_mutex;

FILE *main_log_file;

FILE *hyperdeck_log_files[NB_OF_HYPERDECKS];
FILE *osc_log_file;

char *log_buffer = NULL;
int log_buffer_size = 0;


void log_string (const char *c_source_filename, const char *str, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %s\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, str);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_int (const char *c_source_filename, int i, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %d\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, i);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_long (const char *c_source_filename, long l, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %ld\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, l);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_float (const char *c_source_filename, float f, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %f\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, f);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_pointer (const char *c_source_filename, void *p, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %p\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, p);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_2_strings (const char *c_source_filename, const char *str1, const char *str2, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %s%s\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, str1, str2);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_string_int (const char *c_source_filename, const char *str, int i, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %s%d\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, str, i);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_string_long (const char *c_source_filename, const char *str, long l, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %s%ld\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, str, l);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_string_float (const char *c_source_filename, const char *str, float f, FILE *log_file)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] %s%f\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, str, f);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	F_SYNC (main_log_file);

	if (log_file != NULL) {
		fwrite (log_buffer, log_buffer_size, 1, log_file);
		F_SYNC (log_file);
	}

	g_date_time_unref (current_time);
}

void log_hyperdeck_string (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %s\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, str);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_int (const char *c_source_filename, hyperdeck_t *hyperdeck, int i)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %d\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, i);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_long (const char *c_source_filename, hyperdeck_t *hyperdeck, long l)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %ld\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, l);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_float (const char *c_source_filename, hyperdeck_t *hyperdeck, float f)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %f\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, f);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_pointer (const char *c_source_filename, hyperdeck_t *hyperdeck, void *p)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %p\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, p);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_2_strings (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str1, const char *str2)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %s%s\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, str1, str2);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_string_int (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, int i)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %s%d\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, str, i);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_string_long (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, long l)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %s%ld\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, str, l);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_hyperdeck_string_float (const char *c_source_filename, hyperdeck_t *hyperdeck, const char *str, float f)
{
	GDateTime *current_time;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [%s] HyperDeck %s (%s) %s%f\n\n", g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, c_source_filename, hyperdeck->name, hyperdeck->ip_address, str, f);

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, hyperdeck_log_files[hyperdeck->number]);

	F_SYNC (main_log_file);
	F_SYNC (hyperdeck_log_files[hyperdeck->number]);

	g_date_time_unref (current_time);
}

void log_osc_packet (const char *ip_address, const char *packet, int size)
{
	GDateTime *current_time;
	int i;

	current_time = g_date_time_new_now_local ();

	log_buffer_size = sprintf (log_buffer, "%02dh %02dm %02ds %03dms: [osc.c] Receive OSC packet from %s <--", \
			g_date_time_get_hour (current_time), g_date_time_get_minute (current_time), g_date_time_get_second (current_time), g_date_time_get_microsecond (current_time) / 1000, ip_address);

	for (i = 0; i < size; i++) {
		if ((31 < packet[i]) && (packet[i] < 127)) {
			sprintf (log_buffer + log_buffer_size, " %02X (%c)", packet[i], packet[i]);
			log_buffer_size += 7;
		} else {
			sprintf (log_buffer + log_buffer_size, " %02X", packet[i]);
			log_buffer_size += 3;
		}
	}

	fwrite (log_buffer, log_buffer_size, 1, main_log_file);
	fwrite (log_buffer, log_buffer_size, 1, osc_log_file);

	fwrite ("\n\n", 2, 1, main_log_file);
	fwrite ("\n\n", 2, 1, osc_log_file);

	F_SYNC (main_log_file);
	F_SYNC (osc_log_file);

	g_date_time_unref (current_time);
}

void init_logging (void)
{
	g_mutex_init (&logging_mutex);
}

void start_logging (void)
{
	GDateTime *current_time;
	int year, month, day, i;
	char log_file_name[56];

	g_mutex_lock (&logging_mutex);

	log_buffer = g_malloc (2176);

	current_time = g_date_time_new_now_local ();

	year = g_date_time_get_year (current_time);
	month = g_date_time_get_month (current_time);
	day = g_date_time_get_day_of_month (current_time);

	sprintf (log_file_name, "%04d-%02d-%02d_HyperDeck-Controller.log", year, month, day);
	main_log_file = fopen (log_file_name, "a");

	if (log_hyperdeck) {
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			sprintf (log_file_name, "%04d-%02d-%02d_HyperDeck_%d.log", year, month, day, i + 1);
			hyperdeck_log_files[i] = fopen (log_file_name, "a");
		}
	}

	if (log_osc) {
		sprintf (log_file_name, "%04d-%02d-%02d_OSC.log", year, month, day);
		osc_log_file = fopen (log_file_name, "a");
	}

	g_date_time_unref (current_time);

	logging = TRUE;

	g_mutex_unlock (&logging_mutex);
}

void start_hyperdeck_log (void)
{
	GDateTime *current_time;
	int year, month, day, i;
	char log_file_name[56];

	g_mutex_lock (&logging_mutex);

	if (logging) {
		current_time = g_date_time_new_now_local ();

		year = g_date_time_get_year (current_time);
		month = g_date_time_get_month (current_time);
		day = g_date_time_get_day_of_month (current_time);

		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			sprintf (log_file_name, "%04d-%02d-%02d_HyperDeck_%d.log", year, month, day, i + 1);
			hyperdeck_log_files[i] = fopen (log_file_name, "a");
		}

		g_date_time_unref (current_time);
	}

	log_hyperdeck = TRUE;

	g_mutex_unlock (&logging_mutex);
}

void start_osc_log (void)
{
	GDateTime *current_time;
	char log_file_name[56];

	g_mutex_lock (&logging_mutex);

	if (logging) {
		current_time = g_date_time_new_now_local ();

		sprintf (log_file_name, "%04d-%02d-%02d_OSC.log", g_date_time_get_year (current_time), g_date_time_get_month (current_time), g_date_time_get_day_of_month (current_time));
		osc_log_file = fopen (log_file_name, "a");

		g_date_time_unref (current_time);
	}

	log_osc = TRUE;

	g_mutex_unlock (&logging_mutex);
}

void stop_logging (void)
{
	int i;

	g_mutex_lock (&logging_mutex);

	if (logging) {
		logging = FALSE;

		fclose (main_log_file);

		g_free (log_buffer);
	}

	if (log_hyperdeck) {
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			fclose (hyperdeck_log_files[i]);
		}
	}

	if (log_osc) fclose (osc_log_file);

	g_mutex_unlock (&logging_mutex);
}

void stop_hyperdeck_log (void)
{
	int i;

	g_mutex_lock (&logging_mutex);

	log_hyperdeck = FALSE;

	if (logging) {
		for (i = 0; i < NB_OF_HYPERDECKS; i++) {
			fclose (hyperdeck_log_files[i]);
		}
	}

	g_mutex_unlock (&logging_mutex);
}

void stop_osc_log (void)
{
	g_mutex_lock (&logging_mutex);

	log_osc = FALSE;

	if (logging) fclose (osc_log_file);

	g_mutex_unlock (&logging_mutex);
}

