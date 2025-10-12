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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ncurses.h>

#include <playback.h>
#include <error_codes.h>
#include <error.h>


static unsigned int term_size_x, term_size_y;
static unsigned int file_window_size_x, file_window_size_y;
static unsigned int playback_bar_y;
static unsigned int metadata_bar_y;
static unsigned int command_bar_y;

static unsigned int message_box_x;
static unsigned int message_box_size_x, message_box_size_y;


static WINDOW *file_window;
static WINDOW *message_box;
static WINDOW *playback_bar;
static WINDOW *metadata_bar;
static WINDOW *command_bar;


static unsigned int current_file_window_display_offset = 0;

// This refers to which element of the file window the user is currently hovering over.
static unsigned int user_selected_element = 0;
static unsigned int user_y_pos = 0;


// Types for FS_ELEMENTS
#define ELEM_DIR 0
#define ELEM_FILE 1

typedef struct fs_elem_node {
  char **name;
  unsigned int line_count;
  _Bool type;
  struct fs_elem_node *next;
} FS_ELEMENT;

static char upstream_fs[] = "../";
static char *msg_ptr[] = {upstream_fs};

static FS_ELEMENT head = {.name = msg_ptr, .line_count = 1, .type = ELEM_DIR, .next = NULL};
static unsigned int file_list_depth = 0;


typedef struct msg_node {
  char **message;
  unsigned int line_count;
  struct msg_node *last;
} MSG;

static MSG *latest_msg = NULL;
static unsigned int msgbox_total_linecount = 0;

static _Bool update_track_duration = 0;


void init_ui(void) {
  getmaxyx(stdscr, term_size_y, term_size_x);
  term_size_y--;
  term_size_x--;

  file_window_size_x = term_size_x / 2;
  file_window_size_y = term_size_y - 4;

  playback_bar_y = file_window_size_y + 1;
  metadata_bar_y = playback_bar_y + 1;
  command_bar_y = term_size_y;

  message_box_x = file_window_size_x + 1;
  message_box_size_x = term_size_x - file_window_size_x;
  message_box_size_y = file_window_size_y;

  refresh();
  file_window = newwin(file_window_size_y, file_window_size_x, 0, 0);
  message_box = newwin(message_box_size_y, message_box_size_x, 0, message_box_x);
  playback_bar = newwin(1, term_size_x, playback_bar_y, 0);
  metadata_bar = newwin(1, term_size_x, metadata_bar_y, 0);
  command_bar = newwin(1, term_size_x, command_bar_y, 0);
  //nodelay(file_window, 1);
  //nodelay(message_box, 1);

  return;
}



void reset_cursor(void) {
  move(user_y_pos, 0);
  return;
}





void free_msg(MSG *msg) {
  int i;

  for(i = 0; i < msg->line_count; i++) {
    free(msg->message[i]);
  }
  free(msg->message);
  msgbox_total_linecount -= msg->line_count;

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
  int i, y = message_box_size_y - 1;

  MSG *temp = latest_msg;

  if(temp == NULL) {return;}
  werase(message_box);

  while(msgbox_total_linecount > message_box_size_y) {free_oldest_msg();}

  while(temp != NULL) {
    for(i = temp->line_count - 1; i >= 0; i--) {
      mvwprintw(message_box, y, 1, "%s", temp->message[i]);
      y--;
    }
    temp = temp->last;
  }

  wrefresh(message_box);

  return;
}



void display_msg(char *msg) {
  MSG *temp = malloc(sizeof(MSG));
  unsigned int length = strlen(msg);
  unsigned int remainder_buffer = 1;
  int i;

  if(length % message_box_size_x == 0) {remainder_buffer = 0;}
  unsigned int ptr_count = (length / message_box_size_x) + remainder_buffer;
  char **message_ptrs = malloc(ptr_count * sizeof(char *));

  for(i = 0; i < ptr_count; i++) {
    message_ptrs[i] = malloc(message_box_size_x + 1);
    strncpy(message_ptrs[i], msg, message_box_size_x);
    message_ptrs[i][message_box_size_x] = 0;
    msg += message_box_size_x;
  }

  temp->message = message_ptrs;
  temp->line_count = ptr_count;

  temp->last = latest_msg;
  latest_msg = temp;
  msgbox_total_linecount += ptr_count;

  return;
}



void free_fs_element(FS_ELEMENT *elem) {
  int i;
  for(i = 0; i < elem->line_count; i ++) {
    free(elem->name[i]);
  }
  free(elem->name);

  free(elem);
}


void free_fs_list(void) {
  if(head.next) {
    FS_ELEMENT *temp = head.next;
    FS_ELEMENT *next;

    while(temp != NULL) {
      next = temp->next;
      free_fs_element(temp);
      temp = next;
    }

    head.next = NULL;
  }
  file_list_depth = 0;

  return;
}



void add_fs_element(struct dirent *dir) {
  if(dir->d_type != DT_DIR && dir->d_type != DT_REG) {return;}
  FS_ELEMENT *new_elem = malloc(sizeof(FS_ELEMENT));

  if(dir->d_type == DT_DIR) {
    new_elem->type = ELEM_DIR;
  } else {new_elem->type = ELEM_FILE;}

  unsigned int remainder_buffer = 1;
  unsigned int length = strlen(dir->d_name);

  if(length % file_window_size_x == 0) {remainder_buffer = 0;}
  unsigned int ptr_count = (length / file_window_size_x) + remainder_buffer;
  char **name_ptrs = malloc(ptr_count * sizeof(char *));

  new_elem->line_count = ptr_count;
  char *name = dir->d_name;
  char *save_name;

  if(dir->d_type == DT_DIR) {
    name = calloc(strlen(dir->d_name) + 2, 1);
    sprintf(name, "%s/\0", dir->d_name);
    save_name = name;
  }

  int i;
  for(i = 0;i < ptr_count; i++) {
    name_ptrs[i] = malloc(file_window_size_x + 1);
    strncpy(name_ptrs[i], name, file_window_size_x);
    name_ptrs[i][file_window_size_x] = 0;
    name += file_window_size_x;
  }

  if(dir->d_type == DT_DIR) {free(save_name);}

  new_elem->name = name_ptrs;
  new_elem->next = NULL;

  FS_ELEMENT *temp = &head;

  while(temp->next != NULL) {
    temp = temp->next;
  }

  temp->next = new_elem;

  file_list_depth++;

  return;
}



void display_file_window(void) {
  // start_line_number refers to which line of the first element in the offset is to be displayed at the very top of the file window
  unsigned int start_line_number = 0;
  FS_ELEMENT *start_element = &head;

  // This refers to the number of lines above the file window which are cut off
  unsigned int undisplayed_lines = 0;

  unsigned int y = 0;

  werase(file_window);


  while(undisplayed_lines != current_file_window_display_offset) {
    if(start_element->line_count > current_file_window_display_offset - undisplayed_lines) {
      start_line_number = current_file_window_display_offset - undisplayed_lines;
      break;
    }
    else {
      undisplayed_lines += start_element->line_count;
      start_element = start_element->next;
    }
  }

  int i;

  for(i = start_line_number; i < start_element->line_count; i++) {
    mvwprintw(file_window, y, 0, "%s", start_element->name[i]);
    y++;
  }

  start_element = start_element->next;

  while(start_element) {
    if(start_element->line_count > file_window_size_y - y) {
      for(i = 0; i < file_window_size_y - y + 1; i++) {
        mvwprintw(file_window, y, 0, "%s", start_element->name[i]);
        y++;
      }
      break;
    }
    else {
      for(i = 0; i < start_element->line_count; i++) {
        mvwprintw(file_window, y, 0, "%s", start_element->name[i]);
        y++;
      }
      start_element = start_element->next;
    }
  }

  wrefresh(file_window);

}


void clear_command_bar(void) {
  werase(command_bar);
  wrefresh(command_bar);
  return;
}


void display_command_bar(char *msg) {
  int msg_length = 0;

  werase(command_bar);
  if(!msg) {
    mvwprintw(command_bar, 0, 0, " : ");
  }
  else {
    msg_length = strlen(msg);
    mvwprintw(command_bar, 0, 0, " : %s", msg);
  }

  move(term_size_y, 4 + msg_length);

  wrefresh(command_bar);
  return;
}



void display_metadata_bar(char *album, char *artist, unsigned int year, char *features) {
  werase(metadata_bar);
  mvwprintw(metadata_bar, 0, 0, "   %s - %s  %d  Artists: %s", album, artist, year, features);

  wrefresh(metadata_bar);

  return;
}


void display_song_playback_bar(char *song_title) {
  int song_dur = metadata_retrieve_int(META_TRACK_DURATION);

  mvwprintw(playback_bar, 0, 11, "                                                         ");
  mvwprintw(playback_bar, 0, 11, "%d:%2d  %s", song_dur / 60, song_dur % 60, song_title);

  wrefresh(playback_bar);
  return;
}

void display_playback_bar(void) {
  int play_pos = playback_read_clock();

  mvwprintw(playback_bar, 0, 0, "    %d:%2d / ", play_pos / 60, play_pos % 60);
  wrefresh(playback_bar);

  return;
}

FS_ELEMENT *find_fs_element(int index) {
  int i;
  FS_ELEMENT *ret = &head;

  for(i = 0; i < index; i++) {
    ret = ret->next;
  }

  return ret;
}


int fs_list_check_valid(int index) {
  if(index > file_list_depth) {return 1;}
  FS_ELEMENT *temp = find_fs_element(index);

  if(temp->type == 0) {return 1;}
  return 0;
}



char *fs_list_find_name(int index) {
  int i;
  FS_ELEMENT *elem = &head;

  for(i = 0; i < index; i++) {
    elem = elem->next;
  }

  char *ret = calloc((strlen(elem->name[0]) * elem->line_count) + 1, 1);
  char *ret_save = ret;
  for(i = 0; i < elem->line_count; i++) {
    sprintf(ret, "%s\0", elem->name[i]);
    ret += strlen(elem->name[i]);
  }

  return ret_save;
}



void user_nav_up(void) {

  if(user_selected_element == 0) {return;}
  user_selected_element--;
  FS_ELEMENT *selected = find_fs_element(user_selected_element);
  int diff = user_y_pos - selected->line_count;

  if(diff < 0) {
     // Scroll up if necessary
     current_file_window_display_offset -= selected->line_count - user_y_pos;
     user_y_pos = 0;
  }
  else {
    user_y_pos -= selected->line_count;
  }

  move(user_y_pos, 0);
  display_file_window();
}

void user_nav_down(void) {

  if(user_selected_element == file_list_depth) {return;}
  user_selected_element++;
  FS_ELEMENT *selected = find_fs_element(user_selected_element - 1);
  int save_prev_linecount = selected->line_count;
  selected = selected->next;

  if(user_y_pos + (save_prev_linecount - 1) + selected->line_count >= file_window_size_y) {
    // Scroll down if necessary
    current_file_window_display_offset += (user_y_pos + (save_prev_linecount - 1) + selected->line_count + 1) - file_window_size_y;
    user_y_pos = file_window_size_y - (selected->line_count);
  }
  else {
    user_y_pos += save_prev_linecount;
  }

  move(user_y_pos, 0);
  display_file_window();
}


char *retrieve_fs_element(_Bool *type, int *index) {
  FS_ELEMENT *elem = find_fs_element(user_selected_element);
  int i;

  *type = elem->type;
  *index = user_selected_element;

  char *name = malloc((strlen(elem->name[0]) * elem->line_count) + 1);
  char *save_name = name;

  for(i = 0; i < elem->line_count; i++) {
    sprintf(name, "%s", elem->name[i]);
    name += strlen(elem->name[i]);
  }

  return save_name;
}



void update_playback_bar(void) {
  static int temp = 99;
  if(temp != playback_read_clock()) {
    display_playback_bar();
    if(check_playback_active() == 0) {display_song_playback_bar(metadata_retrieve_str(META_TRACK_TITLE));}
    temp = playback_read_clock();
  }

  return;
}


int file_filter(const struct dirent *file) {
  if(file->d_type != DT_REG) {return 0;}
  if(file->d_name[0] == '.') {return 0;}

  return 1;
}

int dir_filter(const struct dirent *file) {
  // returning zero will exclude the entry from the namelist
  // nonzero will include the entry
  // idk why
  if(file->d_type != DT_DIR) {return 0;}
  if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {return 0;}
  if(file->d_name[0] == '.') {return 0;}

  return 1;
}





void ui_open_dir(const char *dir_name) {
  if(chdir(dir_name) != 0) {
    trackjack_error(JACK_ERR_OPENDIR, (LIB_ERROR)errno);
    return;
  }



  struct dirent **dir_namelist;
  struct dirent **file_namelist;


  // scandir() is used to seperate directories and files, and sort both alphabetically
  // I'm surprised the C standard library has this functionality,
  // I expected to have to implement sorting myself
  int dir_count = scandir(".", &dir_namelist, dir_filter, alphasort);
  if(dir_count < 0) {
    trackjack_error(JACK_ERR_OPENDIR, (LIB_ERROR)errno);
    return;
  }

  int file_count = scandir(".", &file_namelist, file_filter, alphasort);
  if(file_count < 0) {
    trackjack_error(JACK_ERR_OPENDIR, (LIB_ERROR)errno);
    return;
  }

  free_fs_list();

  int i;
  for(i = 0; i < dir_count; i++) {
    add_fs_element(dir_namelist[i]);
    free(dir_namelist[i]);
  }

  for(i = 0; i < file_count; i++) {
    add_fs_element(file_namelist[i]);
    free(file_namelist[i]);
  }

  free(dir_namelist);
  free(file_namelist);


  current_file_window_display_offset = 0;
  display_file_window();
  user_selected_element = 0;
  user_y_pos = 0;

  char success_msg[] = "Loaded directory: ";
  char *final_msg;

  if(strcmp(dir_name, ".") != 0) {
    final_msg = calloc(strlen(dir_name) + strlen(success_msg) + 1, 1);
    sprintf(final_msg, "%s%s", success_msg, dir_name);

    display_msg(final_msg);
    free(final_msg);
  }

  return;
}



void cleanup_ui(void) {
  delwin(file_window);
  delwin(message_box);
  delwin(playback_bar);
  delwin(metadata_bar);

  free_all_msg();
  free_fs_list();
}
