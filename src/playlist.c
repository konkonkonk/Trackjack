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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>

#include <song_def.h>

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

extern const char *release_type_list[3];


int get_list_len(SONG *head) {
  int retVal = 1;
  SONG *buffer = head;

  while(buffer->next != NULL) {
    buffer = buffer->next;
    retVal++;
  }

  return retVal;
}

void remove_tag(TAGS *head, int index) {
  int i;
  TAGS *buffer = head;
  TAGS *bufferB;

  for(i = 0; i < index-1; i++) {
    buffer = buffer->next;
  }

  bufferB = buffer->next;
  buffer->next = bufferB->next;

  return;
}

SONG *new_song(char *filename) {
  SONG *new = malloc(sizeof(SONG));

  new->filename = strdup(filename);
  new->next = NULL;

  return new;
}

void add_song(SONG *head, SONG *new) {
  SONG *buffer = head;

  while(buffer->next != NULL) {
    buffer = buffer->next;
  }

  buffer->next = new;

  return;
}

void add_tag(TAGS *head, TAGS *new) {
  TAGS *buffer = head;

  while(buffer->next != NULL) {
    buffer = buffer->next;
  }

  buffer->next = new;

  return;
}

COM_RELEASE *generate_com_release(SONG *head, char *title, char *artist, uint8_t rel_type) {
  COM_RELEASE *retList = malloc(sizeof(COM_RELEASE));

  retList->track_head = head;
  retList->title = strdup(title);
  retList->artist = strdup(artist);
  retList->release_type = rel_type;
  retList->track_count = get_list_len(head);

  return retList;
}

PLAYLIST *generate_playlist(SONG *head, char *name) {
  PLAYLIST *newList = malloc(sizeof(PLAYLIST));

  newList->name = strdup(name);
  newList->len = get_list_len(head);
  newList->tag_head = NULL;
  newList->head = head;

  return newList;
}


int write_playlist(PLAYLIST *list) {
  char *play_name = malloc(sizeof(list->name) + 10);
  sprintf(play_name, "playlists/%s", list->name);

  FILE *newList = fopen(play_name, "w+");

  if(newList == NULL) {return -1;}

  fprintf(newList, "TRACKJACK PLAYLIST\n%s\n", list->name);
  fprintf(newList, "%d\n", list->len);

  TAGS *buffer = list->tag_head;

  fprintf(newList, "TAGS:\n");
  while(list->tag_head != NULL) {
    fprintf(newList, "%s\n", buffer->name);
    if(buffer->next == NULL) {break;}
    buffer = buffer->next;
  }

  fprintf(newList, "SONGS:\n");

  SONG *bufferB = list->head;

  while(1) {
    fprintf(newList, "%s\n", bufferB->filename);
    if(bufferB->next == NULL) {break;}
    bufferB = bufferB->next;
  }

  fclose(newList);
}

void remove_newline(char *buffer) {
  char *newline = strchr(buffer, '\n');
  if(newline) {*newline = 0;}
}



COM_RELEASE *read_com_release(char *filename, uint8_t rel_type) {
  COM_RELEASE *retlist = malloc(sizeof(COM_RELEASE));

  char *com_name = malloc(sizeof(filename) + 10);
  sprintf(com_name, "%s/%s", release_type_list[rel_type], filename);

  FILE *record = fopen(com_name, "r");
  char *buffer = malloc(50);
  int temp;


  fgets(buffer, 30, record);
  fgets(buffer, 30, record);
  remove_newline(buffer);

  retlist->title = strdup(buffer);
  retlist->release_type = rel_type;

  fgets(buffer, 30, record);
  remove_newline(buffer);
  retlist->artist = strdup(buffer);

  fscanf(record, "%d", &temp);
  fscanf(record, "%d", &temp);

  retlist->track_count = temp;

  fgets(buffer, 30, record);
  fgets(buffer, 30, record);
  fgets(buffer, 50, record);
  remove_newline(buffer);
  retlist->track_head = new_song(buffer);

  while(fgets(buffer, 50, record)) {
    remove_newline(buffer);
    add_song(retlist->track_head, new_song(buffer));
  }

  fclose(record);
  free(com_name);
  free(buffer);

  return retlist;
}




PLAYLIST *read_playlist(char *filename) {
  PLAYLIST *retList = malloc(sizeof(PLAYLIST));
  TAGS *tag_head;
  TAGS *tag_other;

  char *play_name = malloc(sizeof(filename) + 10);
  sprintf(play_name, "playlists/%s", filename);

  FILE *list = fopen(play_name, "r");
  char *buffer = malloc(30);
  int temp;
  _Bool ending = 0;

  fgets(buffer, 30, list);
  fgets(buffer, 30, list);
  remove_newline(buffer);

  retList->name = strdup(buffer);

  fscanf(list, "%d", &temp);

  retList->len = temp;

  fgets(buffer, 30, list);

  while(1) {
    fgets(buffer, 30, list);
    remove_newline(buffer);
    if(strcmp(buffer, "SONGS:") == 0) {break;}

    if(ending == 0) {
      tag_head = malloc(sizeof(TAGS));
      tag_head->name = strdup(buffer);
      tag_head->next = NULL;
      retList->tag_head = tag_head;
    } else {

      tag_other = malloc(sizeof(TAGS));
      tag_other->name = strdup(buffer);
      tag_other->next = NULL;
      add_tag(tag_head, tag_other);

    }

    ending = 1;
  }

  fgets(buffer, 30, list);
  remove_newline(buffer);
  retList->head = new_song(buffer);

  while(fgets(buffer, 30, list)) {
    remove_newline(buffer);
    add_song(retList->head, new_song(buffer));
  }

  fclose(list);

  return retList;
}



void free_com_release(COM_RELEASE *rel) {
  free(rel->artist);
  free(rel->title);
  free(rel);
  return;
}
