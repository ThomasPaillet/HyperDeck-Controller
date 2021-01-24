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

#ifndef __HYPERDECK_PROTOCOL_H
#define __HYPERDECK_PROTOCOL_H


#define HYPERDECK_PROTOCOL_VERSION "1.8"


#define SEND(h,m) send ((h)->socket, m, strlen (m), 0);
//#define SEND(h,m) printf ("hyperdeck %d: %s: slot_%d: %s\n", (h)->number + 1, (h)->adresse_ip, (h)->slot_selected, m);


#define msg_remote_enable "remote: enable: true\n"					//enable remote control
#define msg_remote_override "remote: override: true\n"					//session override remote control

#define msg_configuration "configuration\n"							//query configuration settings
#define msg_configuration_file_format_ "configuration: file format: %s\n"	//switch to specific file format

#define msg_slot_select_video_format_ "slot select: video format: %s\n"	//select video format and reconstruct the timeline accordingly

#define msg_preview_enable_true "preview: enable: true\n"				//switch to preview mode (E to E)
#define msg_preview_enable_false "preview: enable: false\n"				//switch to output (instead of preview)

#define msg_notify_remote_true "notify: remote: true\n"				//set remote notifications
#define msg_notify_slot_true "notify: slot: true\n"					//set slot notifications
#define msg_notify_transport_true "notify: transport: true\n"			//set transport notifications
#define msg_notify_configuration_true "notify: configuration: true\n"		//set configuration notifications

/*#define msg_notify_remote_false "notify: remote: false\n"
#define msg_notify_slot_false "notify: slot: false\n"
#define msg_notify_transport_false "notify: transport: false\n"
#define msg_notify_configuration_false "notify: configuration: false\n"*/

#define msg_slot_info "slot info\n"								//query active slot
#define msg_slot_info_id_1 "slot info: slot id: 1\n"					//query slot 1
#define msg_slot_info_id_2 "slot info: slot id: 2\n"					//query slot 2

#define msg_select_slot_id_1 "slot select: slot id: 1\n"				//switch to slot 1
#define msg_select_slot_id_2 "slot select: slot id: 2\n"				//switch to slot 2

#define msg_disk_list_slot_id_1 "disk list: slot id: 1\n"				//query clip list on disk in slot 1
#define msg_disk_list_slot_id_2 "disk list: slot id: 2\n"				//query clip list on disk in slot 2

#define msg_clips_get "clips get\n"								//query all timeline clips

#define msg_clips_add_ "clips add: name: %s\n"						//append a clip to timeline

#define msg_clips_clear "clips clear\n"								//empty timeline clip list

#define msg_transport_info "transport info\n"						//query current activity

#define msg_goto_clip_id_ "goto: clip id: "							//goto clip id {n}

#define msg_goto_clip_start "goto: clip: start\n"						//goto start of clip

#define msg_play "play\n"										//play from current timecode

#define msg_stop "stop\n"										//stop playback or recording

#define msg_quit "quit\n"										//disconnect ethernet control

#define msg_uptime "uptime\n"										//return time since last boot

#define msg_play_single_clip_true_loop_true "play: single clip: true loop: true\n"
#define msg_play_single_clip_true_loop_false "play: single clip: true loop: false\n"
#define msg_play_single_clip_false_loop_false "play: single clip: false loop: false\n"
#define msg_play_single_clip_false_loop_true "play: single clip: false loop: true\n"


extern char* msg_play_single_loop[4];
extern int msg_play_single_loop_len[4];


gpointer connect_to_hyperdeck (hyperdeck_t* hyperdeck);

void disconnect_from_hyperdeck (hyperdeck_t* hyperdeck);


#endif

