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
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <song_def.h>
#include <playlist.h>
#include <playback.h>
#include <screen.h>
#include <clock.h>

#define KEY_CR 10
#define KEY_SPC 32
#define KEY_COLON 58


SONG *current_head;
int current_len;
int y = 0;


_Bool paused = 0;

void parse_cmd(char *command);

void remove_newline(char *buffer);

void init(void) {
  chdir("test_homedir/");

  initscr();
  curs_set(TRUE);
  keypad(stdscr, TRUE);
  init_clock();
  alutInit(NULL, NULL);
}

void main_loop(SONG *song_head, int len) {
  y = 0;
  int i;
  int ch = 0;
  SONG *buffer;
  SONG *temp_song;
  int temp, playback_position = 4;
  int song_duration = 0;

  nodelay(stdscr, 1);
  while(ch != KEY_COLON) {

    move(y, 0);
    refresh();

    ch = getch();

    switch(ch) {
      case KEY_UP:
        if(y > 0) {y--;}
        break;
      case KEY_DOWN:
        if(y < len-1) {y++;}
        break;
    }

    if(ch == KEY_CR) {
      buffer = song_head;
      for(i = 0; i < y; i++) {
        buffer = buffer->next;
      }
      play_song(buffer);
    } else if(ch == KEY_SPC) {
      if(check_playback_state()) {
        unpause_playback();
      } else {
        pause_playback();
      }
    }

    temp = retrieve_playback_position();
    if(temp != playback_position) {
      playback_position = temp;
      mvprintw(32, 4, "%d:%2d", playback_position / 60, playback_position % 60);
    }

    if((buffer = retrieve_song_playing()) != NULL && buffer != temp_song) {
      temp_song = buffer;
      song_duration = retrieve_song_duration();
      mvprintw(32, 10, "                                                                                ");
      mvprintw(32, 10, "%s  %d:%2d", temp_song->filename, song_duration / 60, song_duration % 60);
    }

    update_msgbox();

    playback_checkstate();

    sleep_until_next_tick();
  }
  nodelay(stdscr, 0);

}



int main(int argc, char **argv) {
  char *command = malloc(160);
  _Bool first_loop = 1;

  if(argc > 3) {
    fprintf(stderr, "Too many arguments.\nUse --help for help.\n");
    return -1;
  }

  if(argc > 1) {
    if(strcmp(argv[1], "--help") != 0 && strcmp(argv[1], "--script") != 0) {
      fprintf(stderr, "Unrecognized argument '%s'\n", argv[1]);
      return -2;
    }

    if(strcmp(argv[1], "--help") == 0) {
      printf("Trackjack plays music.\n--help to print this screen.\n--script to run a script.\n");
      return 0;
    }

    if(strcmp(argv[1], "--script") == 0 && argc == 3) {
      FILE *script = fopen(argv[2], "r");

      init();

      while(fgets(command, 50, script)) {
        remove_newline(command);
        parse_cmd(command);
      }

      fclose(script);
      return 0;
    }
  }

  init();

  COM_RELEASE *trackjack = read_com_release("Trackjack Intro", 0);
  display_com_release(trackjack);
  current_head = trackjack->track_head;
  current_len = trackjack->track_count;
  free_com_release(trackjack);


  while(1) {
    noecho();
    display_msgbox();

    if(first_loop) {
      display_msg("Welcome to Trackjack. Type \':\' to open command window. Use command \'h\' for help.");
      first_loop = 0;
    }

    main_loop(current_head, current_len);
    display_command_line();
    echo();
    getstr(command);
    if(strcmp(command, "q") == 0) {break;}

    parse_cmd(command);
    refresh();
  }

  free(command);

  free_all_msg();
  alutExit();
  endwin();
  return 0;
}
