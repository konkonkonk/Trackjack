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





typedef struct tag_node {
  char *name;
  struct tag_node *next;
} TAGS;

typedef struct {
  char *name;
  int len;
  TAGS *tag_head;
  SONG *head;
} PLAYLIST;

typedef struct {
  uint8_t release_type;
  SONG *track_head;
  int track_count;
  char *artist;
  char *title;
} COM_RELEASE;

int get_list_len(SONG *head);

void remove_tag(TAGS *head, int);

void add_tag(TAGS *head, TAGS *new);

SONG *new_song(char *filename);

void add_song(SONG *head, SONG *new);

PLAYLIST *generate_playlist(SONG *head, char *name);

int write_playlist(PLAYLIST *list);

COM_RELEASE *read_com_release(char *filename, uint8_t);

PLAYLIST *read_playlist(char *filename);

void free_com_release(COM_RELEASE *rel);
