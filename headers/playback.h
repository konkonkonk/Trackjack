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




void play_single(SONG *current_song);
void play_song(SONG *current_song);
void playback_checkstate(void);
unsigned int retrieve_playback_position(void);
int check_playback_active(void);
SONG *retrieve_song_playing(void);
unsigned int retrieve_song_duration(void);

int check_playback_state(void);
void pause_playback(void);
void unpause_playback(void);

int set_volume(unsigned int);
