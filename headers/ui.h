/*
Copyright (C) 2025 Quinn Borrok

This file is part of Trackjack.

Trackjack is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Trackjack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see <https://www.gnu.org/licenses/>.

*/



#define ELEM_DIR 0
#define ELEM_FILE 1

void init_ui(void);
void update_msgbox(void);
void display_msg(char *msg);
//void display_msgbox(void);
void display_file_window(void);

void display_command_bar(char *);
void clear_command_bar(void);
void display_playback_bar(void);
void display_song_playback_bar(char *song_title);
void display_metadata_bar(char *album, char *artist, unsigned int, char *features);
void user_nav_up(void);
void user_nav_down(void);
void ui_open_dir(const char *dir_name);
void reset_cursor(void);

int fs_list_check_valid(int);
char *fs_list_find_name(int);

void free_all_msg(void);
void update_playback_bar(void);

char *retrieve_fs_element(_Bool *, int *);

void cleanup_ui(void);
