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




#define META_TRACK_TITLE 0
#define META_ABLUM_TITLE 1
#define META_ALBUM_ARTIST 2
#define META_TRACK_ARTISTS 3
#define MAX_META_TYPE_STR 3

#define META_TRACK_DURATION 4
#define META_YEAR 5
#define MAX_META_TYPE 5

#define MAX_VOLUME 180

void playback_init(void);

char *metadata_retrieve_str(int);
int metadata_retrieve_int(int);

int playback_read_clock(void);
void playback_cleanup(void);

void playback_update(void);
void playback_start(const char *);
void playback_queue(const char *);

int set_master_volume(unsigned int);
int check_playback_active(void);
int check_playback_state(void);
void playback_pause(void);
void playback_unpause(void);
