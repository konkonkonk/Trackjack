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
#include <playlist.h>


#define METADATA_X 38
#define MSGBOX_X 70
#define DASH_Y 21


#define MESSAGE_BOX_SIZE_X 55
#define MESSAGE_BOX_SIZE_Y 30


const char *release_type_list[] = {
  "LP",
  "EP",
  "Single"
};

typedef struct msg_node {
  char **message;
  unsigned int line_count;
  struct msg_node *last;
} MSG;


static MSG *latest_msg = NULL;
static unsigned int total_linecount = 0;


void display_playlist(PLAYLIST *list) {
  int y = 1;
  mvprintw(0, METADATA_X, "Playlist '%s'", list->name);
  mvprintw(1, METADATA_X, "Songs: %d", list->len);

  move(0, 0);
  SONG *buffer = list->head;
  printw("%s", buffer->filename);
  while(buffer->next != NULL) {
    if(y == 34) {break;}
    buffer = buffer->next;
    mvprintw(y, 0, "%s", buffer->filename);
    y++;
  }
  move(0, 0);
  refresh();

  return;
}


void display_com_release(COM_RELEASE *record) {
  int y = 1;
  mvprintw(0, METADATA_X, "%s '%s'", release_type_list[record->release_type], record->title);
  mvprintw(1, METADATA_X, "Artist: %s", record->artist);
  move(0, 0);

  SONG *buffer = record->track_head;
  printw("%s", buffer->filename);
  while(buffer->next != NULL) {
    buffer = buffer->next;
    mvprintw(y, 0, "%s", buffer->filename);
    y++;
  }

  move(0, 0);
  refresh();
  return;
}


void display_command_line(void) {
  mvprintw(34, 0, " :                                                             ");
  move(34, 3);
  refresh();
}

void free_msg(MSG *msg) {
  int i;

  for(i = 0; i < msg->line_count; i++) {
    free(msg->message[i]);
  }
  free(msg->message);
  total_linecount -= msg->line_count;

  free(msg);

  return;
}


void free_all_msg(void) {
  if(latest_msg == NULL) {return;}

  MSG *next = latest_msg->last;
  MSG *temp;

  free_msg(latest_msg);
  while(next != NULL) {
    temp = next;
    next = next->last;
    free_msg(temp);
  }

  return;
}


void free_oldest_msg(void) {
  MSG *temp = latest_msg;
  MSG *last_temp;

  while(temp->last != NULL) {
    last_temp = temp;
    temp = temp->last;
  }

  last_temp->last = NULL;

  free_msg(temp);
  return;
}


void update_msgbox(void) {
  int i, y = 30;

  MSG *temp = latest_msg;

  if(temp == NULL) {return;}

  if(total_linecount > MESSAGE_BOX_SIZE_Y) {free_oldest_msg();}

  while(temp != NULL) {
    for(i = temp->line_count - 1; i >= 0; i--) {
      mvprintw(y, MSGBOX_X, "                                                       ");
      mvprintw(y, MSGBOX_X, "%s", temp->message[i]);
      y--;
    }
    temp = temp->last;
  }

  return;
}



void display_msg(char *msg) {
  MSG *temp = malloc(sizeof(MSG));
  unsigned int length = strlen(msg);
  unsigned int remainder_buffer = 1;
  unsigned int message_index = 0;
  int i;

  if(length % MESSAGE_BOX_SIZE_X == 0) {remainder_buffer = 0;}
  unsigned int ptr_count = (length / MESSAGE_BOX_SIZE_X) + remainder_buffer;
  char **message_ptrs = malloc(ptr_count * sizeof(char *));

  for(i = 0; i < ptr_count; i++) {
    message_ptrs[i] = malloc(MESSAGE_BOX_SIZE_X + 1);
    strncpy(message_ptrs[i], msg, MESSAGE_BOX_SIZE_X);
    message_ptrs[i][MESSAGE_BOX_SIZE_X] = 0;
    msg += MESSAGE_BOX_SIZE_X;
  }

  temp->message = message_ptrs;
  temp->line_count = ptr_count;

  temp->last = latest_msg;
  latest_msg = temp;
  total_linecount += ptr_count;

  return;
}


void display_msgbox(void) {
  int y;

  for(y = 1; y <= 31; y++) {
    mvprintw(y, MSGBOX_X - 1, "|");
    mvprintw(y, MSGBOX_X + 55, "|");
  }
  mvprintw(31, MSGBOX_X, "_______________________________________________________");
  mvprintw(0, MSGBOX_X, "_______________________________________________________");

  return;
}
